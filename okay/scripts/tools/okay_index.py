
from pathlib import Path

from tools.build_util import OkayBuildOptions, OkayBuildType
from tools.tool_util import OkayToolUtil

CLANGD_PATH = ".clangd"


def register_subparser(subparser):
    OkayBuildOptions.add_subparser_args(subparser)
    subparser.add_argument(
        "--overwrite",
        action="store_true",
        help="Overwrite the .clangd file",
    )


def _abs_posix(p: Path) -> str:
    return p.resolve().as_posix()


def main(args):
    build_options = OkayBuildOptions.from_args(args)

    workspace_root = Path(OkayToolUtil.get_okay_parent_dir()).resolve()
    clangd_file = workspace_root / CLANGD_PATH

    if clangd_file.exists() and not args.overwrite:
        while True:
            choice = input(f"{clangd_file} exists. Overwrite? [y/n]: ").lower()
            if choice == "y":
                break
            if choice == "n":
                return

    include_subpaths = [
        ".",                          # <okay/...>
        "okay/vendor/glm",             # <glm/...>
        "okay/vendor/glfw/include",    # <GLFW/...>
        "okay/vendor/glad",            # <glad/...>
    ]

    lines = []
    lines.append("CompileFlags:")
    lines.append("  Add:")
    lines.append("    - -std=gnu++20")

    for sp in include_subpaths:
        inc = _abs_posix(workspace_root / sp)
        lines.append(f"    - -I{inc}")

    clangd_file.write_text(
        "\n".join(lines) + "\n",
        encoding="utf-8",
        newline="\n",
    )

    print(f"Wrote {clangd_file}")
    print("Restart clangd to apply changes")
