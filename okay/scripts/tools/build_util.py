#!/usr/bin/env python3
from pathlib import Path
import subprocess
import shutil
import hashlib
from tools.tool_util import OkayToolUtil, OkayLogType, OkayLogger
import enum
import os
import sys
import stat
import shlex

class OkayBuildType(enum.Enum):
    Debug = "Debug"
    Release = "Release"

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
        target : str,
        build_type: OkayBuildType = OkayBuildType.Release,
        project_name: str = None,
        compiler: str = "g++",
        generator : str = "auto",
        user_asset_dir : Path = None
    ):
        self.project_dir = project_dir.resolve()
        self.target = target
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
            default="g++",
            help="C/C++ compiler to use (default: gcc)",
        )
        sp.add_argument(
            "--target",
            type=str,
            default="native",
            help="CMake target to build (default: native)",
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
            build_type=bt,
            project_name=args.project_name,
            target=args.target,
            compiler=args.compiler,
            generator=args.generator
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

        args = [
            "cmake",
            "-G", self._decide_generator(),
            "-S", str(OkayToolUtil.get_okay_cmake_dir()),
            "-B", str(self.build_dir),
            f"-DPROJECT={self.project_name}",
            f"-DOKAY_PROJECT_NAME={self.project_name}",
            f"-DOKAY_TARGET={self.target}",
            f"-DOKAY_PROJECT_ROOT_DIR={abs_prj}",
            f"-DCMAKE_BUILD_TYPE={self.build_type.value}",
            f"-DOKAY_BUILD_TYPE={self.build_type.value}",
            # these should be relative to the build dir
            f"-DOKAY_ENGINE_ASSET_ROOT={self.rel_to_build_dir(self.packaged_engine_asset_dir)}",
            f"-DOKAY_GAME_ASSET_ROOT={self.rel_to_build_dir(self.packaged_game_asset_dir)}",
        ]

        # Only set compilers when we’re *not* using Visual Studio
        gens = set(self._decide_generator())
        using_vs = "Visual Studio 17 2022" in gens
        if not using_vs:
            c_compiler = self.compiler if self.compiler != "g++" else "gcc"
            args += [f"-DCMAKE_C_COMPILER={c_compiler}",
                    f"-DCMAKE_CXX_COMPILER={self.compiler}"]

        # Cross-compile for Raspberry Pi if desired via a toolchain (see below)
        toolchain = os.environ.get("OKAY_TOOLCHAIN_FILE", "")
        if self.target.lower() in ("rpi", "raspberrypi") and toolchain:
            args += [f"-DCMAKE_TOOLCHAIN_FILE={toolchain}"]

        cmd = subprocess.list2cmdline(args)  # uses "..." quoting

        return cmd 
    
    @property
    def cmake_build_cmd(self) -> str:
        cmd = ["cmake", "--build", str(self.build_dir), "--target", self.project_name]
        return subprocess.list2cmdline(cmd)

    @property
    def executable(self) -> Path:
        exe = f"{self.project_name}.exe" if sys.platform == "win32" else self.project_name
        return self.build_dir / exe

    def validate_dirs(self, *, need_build_dir: bool = False) -> bool:
        if not self.project_dir.is_dir():
            OkayLogger.log(f"Project directory not found: {self.project_dir}", OkayLogType.ERROR)
            return False
        if need_build_dir and not self.build_dir.is_dir():
            OkayLogger.log(f"Build directory not found: {self.build_dir}", OkayLogType.ERROR)
            return False
        return True


def _sha256_of_files(dirs: list[Path], exts: set[str]) -> hashlib._hashlib.HASH:
    h = hashlib.sha256()
    for d in dirs:
        for f in d.rglob("*"):
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
    def package_assets(src_dir : Path, dest_dir : Path):
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
    def build_project(options: OkayBuildOptions):
        if not options.validate_dirs():
            return
        options.build_dir.mkdir(parents=True, exist_ok=True)

        OkayLogger.log("Packaging assets…", OkayLogType.INFO)
        OkayBuildUtil.package_assets(options.user_asset_dir, options.packaged_game_asset_dir)
        OkayBuildUtil.package_assets(options.engine_asset_dir, options.packaged_engine_asset_dir)

        OkayLogger.log("Executing command: " + options.cmake_configure_cmd, OkayLogType.INFO)
        cmake_dir = OkayToolUtil.get_okay_cmake_dir()
        OkayLogger.log(f"Working directory: {cmake_dir}", OkayLogType.INFO)

        try:
            subprocess.run(
                options.cmake_configure_cmd,
                check=True,
                cwd=cmake_dir,
                shell=True
            )
        except subprocess.CalledProcessError as e:
            OkayLogger.log(f"CMake configure failed: {e}", OkayLogType.ERROR)
            return

        OkayLogger.log(f"Building   -> {options.cmake_build_cmd}", OkayLogType.INFO)
        try:
            subprocess.run(
                options.cmake_build_cmd,
                check=True,
                cwd=OkayToolUtil.get_okay_dir(),
                shell=True
            )
        except subprocess.CalledProcessError as e:
            width = shutil.get_terminal_size().columns
            OkayLogger.log("\n" + "=" * width + "\n", OkayLogType.ERROR)
            OkayLogger.log(f"Build failed: {e}", OkayLogType.ERROR)
            return

        OkayBuildUtil.write_checksum_file(options)
        OkayLogger.log(f"Build complete – executable at {options.executable}", OkayLogType.INFO)

    @staticmethod
    def run_project(
        options: OkayBuildOptions, use_gdb: bool = False, allow_dirty: bool = False
    ):
        if not options.validate_dirs(need_build_dir=True):
            return
        if not options.executable.exists():
            OkayLogger.log(f"Executable not found: {options.executable}", OkayLogType.ERROR)
            return

        if not allow_dirty and not OkayBuildUtil.checksums_valid(options):
            OkayLogger.log("Project changed since last build – please rebuild.", OkayLogType.WARNING)
            OkayLogger.log("    okay build", OkayLogType.INFO)
            OkayLogger.log("    okay sc\n", OkayLogType.INFO)
            OkayLogger.log("…continuing anyway…\n", OkayLogType.WARNING)

        cmd = ["gdb", str(options.executable)] if use_gdb else [str(options.executable)]
        OkayLogger.log(f"Running -> {' '.join(cmd)}", OkayLogType.INFO)
        try:
            # give the executable permission to run and read/write because
            # future build steps may need to modify the executable
            permissions = stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH | stat.S_IWRITE | stat.S_IREAD
            os.chmod(options.executable, permissions)
            subprocess.run(cmd, check=True, cwd=options.build_dir, shell=True)
        except subprocess.CalledProcessError as e:
            OkayLogger.log(f"Runtime error: {e}", OkayLogType.ERROR)

    @staticmethod
    def compile_shaders(options: OkayBuildOptions):
        script = Path(OkayToolUtil.get_okay_tool_dir()) / "okay_sc.sh"
        if not script.exists():
            OkayLogger.log(f"Shader compiler not found: {script}", OkayLogType.ERROR)
            return
        cmd = [
            "bash",
            str(script),
            str(options.project_dir),
            OkayToolUtil.get_okay_dir(),
        ]
        OkayLogger.log(f"Compiling shaders -> {' '.join(cmd)}", OkayLogType.INFO)
        os.execvp(cmd[0], cmd)
