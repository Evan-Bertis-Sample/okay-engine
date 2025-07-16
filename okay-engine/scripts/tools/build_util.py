#!/usr/bin/env python3
from pathlib import Path
import subprocess
import shutil
import hashlib
from tools.tool_util import OkayToolUtil
import enum
import os
import sys


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


class OkayBuildOptions:
    def __init__(
        self,
        project_dir: Path,
        target : str,
        build_type: OkayBuildType = OkayBuildType.Release,
        project_name: str = None,
        compiler: str = "gcc"
    ):
        self.project_dir = project_dir.resolve()
        self.build_type = build_type
        self.project_name = project_name or self.project_dir.name
        self.compiler = compiler

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

    @classmethod
    def from_args(cls, args) -> "OkayBuildOptions":
        bt = OkayBuildType.from_string(args.build_type) or OkayBuildType.Debug
        return cls(
            project_dir=args.project_dir,
            build_type=bt,
            project_name=args.project_name,
        )

    @property
    def build_dir(self) -> Path:
        return Path(OkayToolUtil.get_okay_work_dir(self.project_dir)) / "build"

    @property
    def cmake_configure_cmd(self) -> list[str]:
        okay_root = OkayToolUtil.get_okay_dir()
        rel_prj = str(self.project_dir.relative_to(okay_root)).replace("\\", "/")
        return [
            "cmake",
            "-G",
            "Unix Makefiles",
            "-S",
            ".",
            "-B",
            str(self.build_dir),
            f"-DPROJECT={self.project_name}",
            f"-DOKAY_PROJECT_ROOT_DIR={rel_prj}",
            f"-DCMAKE_BUILD_TYPE={self.build_type.value}",
            f"-DCMAKE_C_COMPILER={self.compiler}",
            f"-DCMAKE_CXX_COMPILER={self.compiler}",
        ]

    @property
    def cmake_build_cmd(self) -> list[str]:
        return ["cmake", "--build", str(self.build_dir), "--target", self.project_name]

    @property
    def executable(self) -> Path:
        exe = f"{self.project_name}.exe" if os.name == "nt" else self.project_name
        return self.build_dir / exe

    def validate_dirs(self, *, need_build_dir: bool = False) -> bool:
        if not self.project_dir.is_dir():
            print(f"✗ Project directory not found: {self.project_dir}")
            return False
        if need_build_dir and not self.build_dir.is_dir():
            print(f"✗ Build directory not found: {self.build_dir}")
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
    def write_checksum_file(options: OkayBuildOptions):
        options.build_dir.mkdir(parents=True, exist_ok=True)
        src, shd = OkayBuildUtil.generate_checksums(options)
        (options.build_dir / "checksum.txt").write_text(f"{src}\n{shd}")

    @staticmethod
    def read_stored_checksums(options: OkayBuildOptions) -> tuple[str, str] | None:
        f = options.build_dir / "checksum.txt"
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
    def build_project(options: OkayBuildOptions):
        if not options.validate_dirs():
            return
        options.build_dir.mkdir(parents=True, exist_ok=True)

        print(f"Configuring → {' '.join(options.cmake_configure_cmd)}")
        try:
            subprocess.run(
                options.cmake_configure_cmd,
                check=True,
                cwd=OkayToolUtil.get_okay_dir(),
            )
        except subprocess.CalledProcessError as e:
            print(f"✗ CMake configure failed: {e}")
            return

        print(f"Building   → {' '.join(options.cmake_build_cmd)}")
        try:
            subprocess.run(
                options.cmake_build_cmd,
                check=True,
                cwd=OkayToolUtil.get_okay_dir(),
            )
        except subprocess.CalledProcessError as e:
            width = shutil.get_terminal_size().columns
            print("\n" + "=" * width + "\n")
            print(f"✗ Build failed: {e}")
            return

        OkayBuildUtil.write_checksum_file(options)
        print(f"✓ Build complete – executable at {options.executable}")

    @staticmethod
    def run_project(
        options: OkayBuildOptions, use_gdb: bool = False, allow_dirty: bool = False
    ):
        if not options.validate_dirs(need_build_dir=True):
            return
        if not options.executable.exists():
            print(f"✗ Executable not found: {options.executable}")
            return

        if not allow_dirty and not OkayBuildUtil.checksums_valid(options):
            print("\033[91m✗ Project changed since last build – please rebuild.\033[0m")
            print("    okay build")
            print("    okay sc\n")
            print("…continuing anyway…\n")

        cmd = ["gdb", str(options.executable)] if use_gdb else [str(options.executable)]
        print(f"Running → {' '.join(cmd)}")
        try:
            subprocess.run(cmd, check=True, cwd=options.build_dir)
        except subprocess.CalledProcessError as e:
            print(f"✗ Runtime error: {e}")

    @staticmethod
    def compile_shaders(options: OkayBuildOptions):
        script = Path(OkayToolUtil.get_okay_tool_dir()) / "okay_sc.sh"
        if not script.exists():
            print(f"✗ Shader compiler not found: {script}")
            return
        cmd = [
            "bash",
            str(script),
            str(options.project_dir),
            OkayToolUtil.get_okay_dir(),
        ]
        print(f"Compiling shaders → {' '.join(cmd)}")
        os.execvp(cmd[0], cmd)
