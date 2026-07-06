#!/usr/bin/env python3
import enum
import hashlib
import os
import shlex
import shutil
import stat
import subprocess
import sys
import time
from pathlib import Path

from tools.proc_interface import OkayProcUtil
from tools.tool_util import OkayLogger, OkayLogType, OkayToolUtil
from watchdog.events import FileSystemEventHandler
from watchdog.observers import Observer


class DirectoryWatcherHandler(FileSystemEventHandler):
    def __init__(
        self,
        code_cb,
        asset_cb,
        code_change_cb,
        asset_dirs: list[Path],
        ignored_dirs: list[Path],
        debounce_seconds: float = 0.25,
    ):
        self._code_cb = code_cb
        self._asset_cb = asset_cb
        self._code_change_check = code_change_cb
        self._asset_dirs = [d.resolve() for d in asset_dirs]
        self._ignored_dirs = [d.resolve() for d in ignored_dirs]
        self._debounce_seconds = debounce_seconds
        self._last_call_at = {"code": 0.0, "asset": 0.0}

    def _contains_path(self, root: Path, path: Path) -> bool:
        try:
            path.relative_to(root)
            return True
        except ValueError:
            return False

    def _is_ignored(self, path: Path) -> bool:
        path = path.resolve()
        return any(self._contains_path(d, path) for d in self._ignored_dirs)

    def _is_asset(self, path: Path) -> bool:
        path = path.resolve()
        return any(self._contains_path(d, path) for d in self._asset_dirs)

    def _handle(self, event):
        paths = [Path(event.src_path)]
        if hasattr(event, "dest_path") and event.dest_path:
            paths.append(Path(event.dest_path))

        if any(self._is_ignored(p) for p in paths):
            return

        now = time.monotonic()
        event_type = "asset" if any(self._is_asset(p) for p in paths) else "code"

        if now - self._last_call_at[event_type] < self._debounce_seconds:
            return

        if event_type == "code" and not self._code_change_check():
            return

        self._last_call_at[event_type] = now

        if event_type == "asset":
            self._asset_cb()
        else:
            self._code_cb()

    def on_created(self, event):
        self._handle(event)

    def on_deleted(self, event):
        self._handle(event)

    def on_modified(self, event):
        self._handle(event)

    def on_moved(self, event):
        self._handle(event)


class OkayBuildType(enum.Enum):
    Debug = "debug"
    Release = "release"

    @classmethod
    def list(cls):
        return list(cls)

    @classmethod
    def names(cls):
        return [t.name for t in cls]

    @classmethod
    def from_string(cls, s: str):
        for t in cls:
            if t.name.lower() == s.lower():
                return t
        return None

    def __str__(self):
        return self.name


class OkayRuntimeType(enum.Enum):
    Editor = "editor"
    Runtime = "runtime"

    @classmethod
    def list(cls):
        return list(cls)

    @classmethod
    def names(cls):
        return [t.name for t in cls]

    @classmethod
    def from_string(cls, s: str):
        for t in cls:
            if t.name.lower() == s.lower():
                return t
        return None

    def __str__(self):
        return self.name


class OkayBuildOptions:
    def __init__(
        self,
        project_dir: Path,
        target: str,
        runtime_type: OkayRuntimeType = OkayRuntimeType.Runtime,
        build_type: OkayBuildType = OkayBuildType.Release,
        project_name: str = None,
        compiler: str = "g++",
        generator: str = "auto",
        user_asset_dir: Path = None,
    ):
        self.project_dir = project_dir.resolve()
        self.target = target
        self.runtime_type = runtime_type
        self.build_type = build_type
        self.project_name = project_name or self.project_dir.name
        self.compiler = compiler
        self.generator = generator
        self.user_asset_dir = user_asset_dir or (self.project_dir / "assets")

    @classmethod
    def add_subparser_args(cls, sp):
        sp.add_argument(
            "--project-dir",
            type=Path,
            default=Path("."),
            help="Path to your project root",
        )
        sp.add_argument(
            "--runtime-type",
            choices=OkayRuntimeType.names(),
            default=OkayRuntimeType.Editor.name,
            help="CMake build type",
        )
        sp.add_argument(
            "--build-type",
            choices=OkayBuildType.names(),
            default=OkayBuildType.Debug.name,
            help="CMake build type",
        )
        sp.add_argument(
            "--project-name",
            help="Override the default target/project name",
        )
        sp.add_argument(
            "--compiler",
            type=str,
            default="clang",
            help="C/C++ compiler to use (default: clang)",
        )
        sp.add_argument(
            "--target",
            type=str,
            default="native",
            help="Platform to target (default: native)",
        )
        sp.add_argument(
            "--generator",
            choices=["auto", "ninja", "mingw-makefiles", "unix-makefiles", "vs"],
            default="auto",
            help="CMake generator to use (default: auto)",
        )

        # user asset dirs
        sp.add_argument(
            "--asset-dir",
            type=Path,
            default="assets",
            help="Path to your project assets",
        )

    @classmethod
    def get_host_platform(cls) -> str:
        plat = sys.platform
        if plat.startswith("linux"):
            return "linux"
        if plat == "darwin":
            return "macos"
        if plat in ("win32", "cygwin"):
            return "windows"
        return "unknown"

    @classmethod
    def from_args(cls, args) -> "OkayBuildOptions":
        bt = OkayBuildType.from_string(args.build_type) or OkayBuildType.Debug

        # if this is native, set target to the platform
        if args.target.lower() == "native":
            plat = OkayBuildOptions.get_host_platform()
            args.target = plat

        return cls(
            project_dir=args.project_dir.resolve(),
            runtime_type=args.runtime_type,
            build_type=bt,
            project_name=args.project_name,
            target=args.target,
            compiler=args.compiler,
            generator=args.generator,
        )

    def _decide_generator(self) -> str:
        gen = self.generator.lower()
        if gen == "ninja":
            return "Ninja"
        if gen == "mingw-makefiles":
            return "MinGW Makefiles"
        if gen == "unix-makefiles":
            return "Unix Makefiles"

        # auto
        # Prefer Ninja if found
        if shutil.which("ninja"):
            return "Ninja"
        # On Windows without Ninja, prefer MinGW Makefiles if you’re using GCC
        if os.name == "nt" and self.compiler in ("g++", "gcc"):
            return "MinGW Makefiles"
        # Fallback
        return "Unix Makefiles"

    @property
    def build_dir(self) -> Path:
        # build directory is in the .okay folder
        path = Path(OkayToolUtil.get_okay_work_dir(self.project_dir)) / "build"
        # make sure the path exists
        path.mkdir(parents=True, exist_ok=True)
        # turn this into the absolute path
        path = path.resolve()
        return path / f"{self.target.lower()}_{self.build_type.name.lower()}"

    @property
    def packaged_engine_asset_dir(self) -> Path:
        return Path(self.build_dir / "engine" / "assets")

    @property
    def packaged_game_asset_dir(self) -> Path:
        return Path(self.build_dir / "game" / "assets")

    @property
    def engine_asset_dir(self) -> Path:
        return Path(OkayToolUtil.get_okay_dir()) / "assets"

    def rel_to_build_dir(self, p: Path) -> str:
        return os.path.relpath(p.resolve(), self.build_dir).replace("\\", "/")

    @property
    def cmake_configure_cmd(self) -> str:
        okay_root = OkayToolUtil.get_okay_cmake_dir()
        rel_prj = os.path.relpath(self.project_dir, okay_root).replace("\\", "/")
        abs_prj = (Path(okay_root) / rel_prj).resolve().as_posix()

        generator = self._decide_generator()

        args = [
            "cmake",
            "-G",
            generator,
            "-S",
            str(OkayToolUtil.get_okay_cmake_dir()),
            "-B",
            str(self.build_dir),
            f"-DPROJECT={self.project_name}",
            f"-DOKAY_PROJECT_NAME={self.project_name}",
            f"-DOKAY_TARGET={self.target}",
            f"-DOKAY_PROJECT_ROOT_DIR={abs_prj}",
            f"-DCMAKE_BUILD_TYPE={self.build_type.value}",
            f"-DOKAY_BUILD_TYPE={self.build_type.value}",
            "-DCMAKE_C_COMPILER_LAUNCHER=ccache",
            "-DCMAKE_CXX_COMPILER_LAUNCHER=ccache",
            f"-DOKAY_ENGINE_ASSET_ROOT={self.rel_to_build_dir(self.packaged_engine_asset_dir)}",
            f"-DOKAY_GAME_ASSET_ROOT={self.rel_to_build_dir(self.packaged_game_asset_dir)}",
        ]

        using_vs = "Visual Studio" in generator

        if not using_vs:
            compiler = self.compiler.lower()

            if compiler in ("g++", "gcc"):
                c_compiler = "gcc"
                cxx_compiler = "g++"
                rc_compiler = "windres"

            elif compiler in ("clang++", "clang"):
                c_compiler = "clang"
                cxx_compiler = "clang++"
                rc_compiler = "windres" if os.name == "nt" else None

                if os.name == "nt":
                    resolved_c = shutil.which(c_compiler) or ""
                    resolved_cxx = shutil.which(cxx_compiler) or ""

                    if "program files" in resolved_c.lower() or "program files" in resolved_cxx.lower():
                        raise RuntimeError(
                            "clang/clang++ resolved to the standalone LLVM install instead of "
                            "MSYS2/MinGW clang. Put the MSYS2 clang directory earlier on PATH, "
                            "or build with --compiler g++."
                        )

            else:
                c_compiler = self.compiler
                cxx_compiler = self.compiler
                rc_compiler = None

            args += [
                f"-DCMAKE_C_COMPILER={c_compiler}",
                f"-DCMAKE_CXX_COMPILER={cxx_compiler}",
            ]

        toolchain = os.environ.get("OKAY_TOOLCHAIN_FILE", "")
        if self.target.lower() in ("rpi", "raspberrypi") and toolchain:
            args += [f"-DCMAKE_TOOLCHAIN_FILE={toolchain}"]

        return subprocess.list2cmdline(args)

    @property
    def cmake_build_cmd(self) -> str:
        cmd = [
            "cmake",
            "--build",
            str(self.build_dir),
            "--target",
            "okay_runtime",
            "--parallel",
            str(os.cpu_count()),
        ]
        return subprocess.list2cmdline(cmd)


    @property
    def cmake_build_game_cmd(self) -> str:
        cmd = [
            "cmake",
            "--build",
            str(self.build_dir),
            "--target",
            self.project_name,
            "--parallel",
            str(os.cpu_count()),
        ]
        return subprocess.list2cmdline(cmd)

    @property
    def executable(self) -> Path:
        exe = (
            f"{self.project_name}.exe" if sys.platform == "win32" else self.project_name
        )
        return self.build_dir / exe

    def validate_dirs(self, *, need_build_dir: bool = False) -> bool:
        if not self.project_dir.is_dir():
            OkayLogger.log(
                f"Project directory not found: {self.project_dir}", OkayLogType.ERROR
            )
            return False
        if need_build_dir and not self.build_dir.is_dir():
            OkayLogger.log(
                f"Build directory not found: {self.build_dir}", OkayLogType.ERROR
            )
            return False
        return True


def _sha256_of_files(dirs: list[Path], exts: set[str]) -> hashlib._hashlib.HASH:
    h = hashlib.sha256()

    for d in dirs:
        for f in d.rglob("*"):
            if ".okay" in f.parts:
                continue

            if f.suffix.lower() in exts:
                with f.open("rb") as fp:
                    for chunk in iter(lambda: fp.read(65536), b""):
                        h.update(chunk)

    return h


class OkayBuildUtil:
    SOURCE_EXTS = {".c", ".cpp", ".h", ".hpp", "cmakelists.txt"}
    SHADER_EXTS = {".vert", ".frag", ".comp", ".geom", ".tesc", ".tese", ".glsl"}

    @staticmethod
    def generate_checksums(options: OkayBuildOptions) -> tuple[str, str]:
        roots = [options.project_dir, Path(OkayToolUtil.get_okay_dir())]
        src_h = _sha256_of_files(roots, OkayBuildUtil.SOURCE_EXTS).hexdigest()
        shd_h = _sha256_of_files(roots, OkayBuildUtil.SHADER_EXTS).hexdigest()
        return src_h, shd_h

    @staticmethod
    def get_checksum_file(project_dir: Path) -> Path:
        return OkayToolUtil.get_okay_work_dir(project_dir) / "checksum.txt"

    @staticmethod
    def write_checksum_file(options: OkayBuildOptions):
        options.build_dir.mkdir(parents=True, exist_ok=True)
        src, shd = OkayBuildUtil.generate_checksums(options)
        OkayBuildUtil.get_checksum_file(options.project_dir).write_text(f"{src}\n{shd}")

    @staticmethod
    def read_stored_checksums(options: OkayBuildOptions) -> tuple[str, str] | None:
        f = OkayBuildUtil.get_checksum_file(options.project_dir)
        if not f.exists():
            return None
        lines = f.read_text().splitlines()
        return (lines[0], lines[1]) if len(lines) >= 2 else None

    @staticmethod
    def checksums_valid(options: OkayBuildOptions) -> bool:
        stored = OkayBuildUtil.read_stored_checksums(options)
        if stored is None:
            return False
        return stored == OkayBuildUtil.generate_checksums(options)

    @staticmethod
    def package_assets(src_dir: Path, dest_dir: Path):
        # for now, this is a simple implementation that will copy
        # all files from the source directory to the destination directory
        # in the future, this will probably package assets and encrypt them
        if src_dir.exists() == False:
            OkayLogger.log(f"Source directory not found: {src_dir}", OkayLogType.ERROR)
            return

        if dest_dir.exists():
            shutil.rmtree(dest_dir)

        shutil.copytree(src_dir, dest_dir, dirs_exist_ok=True)

    @staticmethod
    def build_project(options: OkayBuildOptions) -> bool:
        if not options.validate_dirs():
            return False

        options.build_dir.mkdir(parents=True, exist_ok=True)

        OkayLogger.log("Packaging assets…", OkayLogType.INFO)
        OkayBuildUtil.package_assets(
            options.user_asset_dir, options.packaged_game_asset_dir
        )
        OkayBuildUtil.package_assets(
            options.engine_asset_dir, options.packaged_engine_asset_dir
        )

        OkayLogger.log(
            "Executing command: " + options.cmake_configure_cmd, OkayLogType.INFO
        )
        cmake_dir = OkayToolUtil.get_okay_cmake_dir()
        OkayLogger.log(f"Working directory: {cmake_dir}", OkayLogType.INFO)

        try:
            subprocess.run(
                options.cmake_configure_cmd, check=True, cwd=cmake_dir, shell=True
            )
        except subprocess.CalledProcessError as e:
            OkayLogger.log(f"CMake configure failed: {e}", OkayLogType.ERROR)
            return False

        OkayLogger.log(f"Building   -> {options.cmake_build_cmd}", OkayLogType.INFO)
        try:
            subprocess.run(
                options.cmake_build_cmd,
                check=True,
                cwd=OkayToolUtil.get_okay_dir(),
                shell=True,
            )
        except subprocess.CalledProcessError as e:
            width = shutil.get_terminal_size().columns
            OkayLogger.log("\n" + "=" * width + "\n", OkayLogType.ERROR)
            OkayLogger.log(f"Build failed: {e}", OkayLogType.ERROR)
            return False

        OkayBuildUtil.write_checksum_file(options)
        OkayLogger.log(
            f"Build complete – executable at {options.executable}", OkayLogType.INFO
        )
        return True

    @staticmethod
    def rebuild_game_dll(options: OkayBuildOptions) -> bool:
        if not options.validate_dirs(need_build_dir=True):
            return False

        OkayLogger.log("Packaging assets…", OkayLogType.INFO)
        OkayBuildUtil.package_assets(
            options.user_asset_dir,
            options.packaged_game_asset_dir,
        )
        OkayBuildUtil.package_assets(
            options.engine_asset_dir,
            options.packaged_engine_asset_dir,
        )

        OkayLogger.log(
            f"Rebuilding game DLL -> {options.cmake_build_game_cmd}",
            OkayLogType.INFO,
        )

        try:
            subprocess.run(
                options.cmake_build_game_cmd,
                check=True,
                cwd=OkayToolUtil.get_okay_dir(),
                shell=True,
            )
        except subprocess.CalledProcessError as e:
            width = shutil.get_terminal_size().columns
            OkayLogger.log("\n" + "=" * width + "\n", OkayLogType.ERROR)
            OkayLogger.log(f"Game DLL rebuild failed: {e}", OkayLogType.ERROR)
            return False

        built_game_dll = OkayBuildUtil.get_built_game_dll(options)

        if not built_game_dll.exists():
            OkayLogger.log(
                f"Game DLL rebuild completed, but output was not found: {built_game_dll}",
                OkayLogType.ERROR,
            )
            return False

        OkayBuildUtil.write_checksum_file(options)

        OkayLogger.log(
            f"Game DLL rebuild complete – output at {built_game_dll}",
            OkayLogType.INFO,
        )
        return True

    @staticmethod
    def get_built_game_dll(options: OkayBuildOptions) -> Path:
        name = options.project_name

        if sys.platform == "win32":
            return options.build_dir / f"lib{name}.dll"

        if sys.platform == "darwin":
            return options.build_dir / f"lib{name}.dylib"

        return options.build_dir / f"lib{name}.so"


    @staticmethod
    def get_hot_reload_dir(options: OkayBuildOptions) -> Path:
        return options.build_dir / "hot_reload"


    @staticmethod
    def get_game_dll(options: OkayBuildOptions, reload_count: int) -> Path:
        name = options.project_name
        hot_reload_dir = OkayBuildUtil.get_hot_reload_dir(options)

        if sys.platform == "win32":
            return hot_reload_dir / f"lib{name}_{reload_count}.dll"

        if sys.platform == "darwin":
            return hot_reload_dir / f"lib{name}_{reload_count}.dylib"

        return hot_reload_dir / f"lib{name}_{reload_count}.so"


    @staticmethod
    def get_game_dll_pattern(options: OkayBuildOptions) -> str:
        name = options.project_name

        if sys.platform == "win32":
            return f"lib{name}_*.dll"

        if sys.platform == "darwin":
            return f"lib{name}_*.dylib"

        return f"lib{name}_*.so"


    @staticmethod
    def get_reload_count_from_dll(options: OkayBuildOptions, dll_path: Path) -> int | None:
        name = options.project_name
        stem = dll_path.stem

        prefix = f"lib{name}_"

        if not stem.startswith(prefix):
            return None

        suffix = stem[len(prefix):]

        if not suffix.isdigit():
            return None

        return int(suffix)


    @staticmethod
    def get_latest_reload_count(options: OkayBuildOptions) -> int:
        hot_reload_dir = OkayBuildUtil.get_hot_reload_dir(options)

        if not hot_reload_dir.exists():
            return -1

        max_count = -1
        pattern = OkayBuildUtil.get_game_dll_pattern(options)

        for dll_path in hot_reload_dir.glob(pattern):
            count = OkayBuildUtil.get_reload_count_from_dll(options, dll_path)

            if count is not None:
                max_count = max(max_count, count)

        return max_count


    @staticmethod
    def copy_game_dll(options: OkayBuildOptions, reload_count: int) -> bool:
        built_game_dll = OkayBuildUtil.get_built_game_dll(options)
        hot_reload_dir = OkayBuildUtil.get_hot_reload_dir(options)
        reload_game_dll = OkayBuildUtil.get_game_dll(options, reload_count)

        if not built_game_dll.exists():
            OkayLogger.log(
                f"Built game DLL not found: {built_game_dll}",
                OkayLogType.ERROR,
            )
            return False

        try:
            hot_reload_dir.mkdir(parents=True, exist_ok=True)
            shutil.copy2(built_game_dll, reload_game_dll)
        except OSError as e:
            OkayLogger.log(
                f"Failed to copy game DLL into hot reload directory: {e}",
                OkayLogType.ERROR,
            )
            return False

        OkayLogger.log(
            f"Copied game DLL -> {reload_game_dll}",
            OkayLogType.INFO,
        )
        return True


    @staticmethod
    def copy_next_game_dll(options: OkayBuildOptions) -> int | None:
        next_reload_count = OkayBuildUtil.get_latest_reload_count(options) + 1

        if not OkayBuildUtil.copy_game_dll(options, next_reload_count):
            return None

        return next_reload_count


    @staticmethod
    def prepare_hot_reload_dir(options: OkayBuildOptions) -> bool:
        hot_reload_dir = OkayBuildUtil.get_hot_reload_dir(options)

        try:
            if hot_reload_dir.exists():
                shutil.rmtree(hot_reload_dir)

            hot_reload_dir.mkdir(parents=True, exist_ok=True)
        except OSError as e:
            OkayLogger.log(
                f"Failed to reset hot reload directory: {hot_reload_dir}: {e}",
                OkayLogType.ERROR,
            )
            return False

        return OkayBuildUtil.copy_game_dll(options, 0)

    @staticmethod
    def run_project(
        options: OkayBuildOptions,
        use_gdb: bool = False,
        allow_dirty: bool = False,
        hot_reload: bool = True,
    ):
        if not options.validate_dirs(need_build_dir=True):
            return
        if not options.executable.exists():
            OkayLogger.log(
                f"Executable not found: {options.executable}", OkayLogType.ERROR
            )
            return

        if not allow_dirty and not OkayBuildUtil.checksums_valid(options):
            OkayLogger.log(
                "Project changed since last build – please rebuild.",
                OkayLogType.WARNING,
            )
            OkayLogger.log("    okay build", OkayLogType.INFO)
            OkayLogger.log("    okay sc\n", OkayLogType.INFO)
            OkayLogger.log("…continuing anyway…\n", OkayLogType.WARNING)

        if hot_reload and not OkayBuildUtil.prepare_hot_reload_dir(options):
            return

        cmd = ["gdb", str(options.executable)] if use_gdb else [str(options.executable)]

        try:
            permissions = (
                stat.S_IXUSR
                | stat.S_IXGRP
                | stat.S_IXOTH
                | stat.S_IWRITE
                | stat.S_IREAD
            )
            os.chmod(options.executable, permissions)

            observers = []
            if hot_reload:
                OkayLogger.log("Attaching reload watchdog...", OkayLogType.INFO)
                observers = OkayBuildUtil.attach_reload_watchdog(options)

            OkayLogger.log(f"Running -> {' '.join(cmd)}", OkayLogType.INFO)
            subprocess.run(cmd, check=True, cwd=options.build_dir, shell=True)

            for obs in observers:
                obs.stop()

        except subprocess.CalledProcessError as e:
            OkayLogger.log(f"Runtime error: {e}", OkayLogType.ERROR)

    @staticmethod
    def attach_reload_watchdog(options: OkayBuildOptions):
        last_checksum = OkayBuildUtil.generate_checksums(options)

        def code_change_since_last_event() -> bool:
            nonlocal last_checksum

            checksum = OkayBuildUtil.generate_checksums(options)
            if checksum == last_checksum:
                return False

            last_checksum = checksum
            return True

        event_handler = DirectoryWatcherHandler(
            code_cb=lambda: OkayBuildUtil.reload_application(options),
            asset_cb=lambda: OkayBuildUtil.reload_assets(options),
            code_change_cb=code_change_since_last_event,
            asset_dirs=[
                options.user_asset_dir,
                options.project_dir / "assets",
                options.engine_asset_dir,
            ],
            ignored_dirs=[
                options.project_dir / ".okay",
                Path(OkayToolUtil.get_okay_parent_dir()) / ".okay",
            ],
        )

        dirs = [options.project_dir, OkayToolUtil.get_okay_parent_dir()]

        observers = []

        for dir in dirs:
            observer = Observer()
            observer.schedule(event_handler, path=str(dir), recursive=True)
            observer.start()
            observers.append(observer)

        return observers

    @staticmethod
    def reload_assets(options: OkayBuildOptions):
        OkayLogger.log("Hot reloading assets!", OkayLogType.INFO)

        OkayBuildUtil.package_assets(
            options.user_asset_dir,
            options.packaged_game_asset_dir,
        )
        OkayBuildUtil.package_assets(
            options.engine_asset_dir,
            options.packaged_engine_asset_dir,
        )

        OkayProcUtil.send_hot_reload_assets()

    @staticmethod
    def reload_application(options: OkayBuildOptions):
        OkayLogger.log("Hot reloading application!", OkayLogType.INFO)

        if not OkayBuildUtil.rebuild_game_dll(options):
            OkayLogger.log("Hot reload build failed.", OkayLogType.ERROR)
            return

        reload_count = OkayBuildUtil.copy_next_game_dll(options)

        if reload_count is None:
            return

        OkayProcUtil.send_hot_reload_code(reload_count.to_bytes(2, byteorder='little'))
