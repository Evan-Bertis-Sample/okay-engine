#!/usr/bin/env python3
"""
Okay Engine dispatcher for all `okay_*.py` sub-tools.
"""

import argparse
import importlib
import sys
from pathlib import Path
from tools import tool_util

TOOLS_FOLDER = "tools"
OKAY_ASCII_LOGO = r"""
       _                                  _            
  ___ | |__ __ _  _  _   ___  _ _   __ _ (_) _ _   ___ 
 / _ \| / // _` || || | / -_)| ' \ / _` || || ' \ / -_)
 \___/|_\_\\__,_| \_, | \___||_||_|\__, ||_||_||_|\___|
                  |__/             |___/               
"""


def discover_tools(tools_dir: Path) -> list[Path]:
    """
    Recursively find all Python files in `tools_dir` whose names start with 'okay_'.
    """
    return sorted(tools_dir.rglob("okay_*.py"))


def build_module_info(tool_path: Path, project_root: Path) -> tuple[str, str]:
    """
    Given the full path to a tool file, return
      (command_name, module_path)
    where:
      - command_name is the subcommand (filename without 'okay_' prefix)
      - module_path is the importable module string
    """
    relative = tool_path.relative_to(project_root).with_suffix("")
    module_path = relative.as_posix().replace("/", ".")
    command_name = tool_path.stem.removeprefix("okay_")
    return command_name, module_path


def main():
    project_root = Path(__file__).resolve().parent
    tools_dir = project_root / TOOLS_FOLDER

    if not tools_dir.is_dir():
        sys.stderr.write(f"Error: tools directory not found at {tools_dir}\n")
        sys.exit(1)

    tool_files = discover_tools(tools_dir)

    # Header
    print(OKAY_ASCII_LOGO)
    print("okay engine – an okay game engine for okay games.\n")

    # Set up top‑level parser
    parser = argparse.ArgumentParser(description="Dispatcher for all okay_ sub-tools.")
    subparsers = parser.add_subparsers(
        dest="tool", required=True, help="Available sub-commands"
    )

    # Dynamically import and register each sub-tool
    modules: dict[str, object] = {}
    for fp in tool_files:
        cmd, mod_path = build_module_info(fp, project_root)
        print(f"Loading module: {mod_path}")
        module = importlib.import_module(mod_path)

        # Create its subparser
        sub = subparsers.add_parser(cmd, help=module.__doc__ or "")
        if hasattr(module, "register_subparser"):
            module.register_subparser(sub)

        modules[cmd] = module
    
    print("") # Blank line for better readability

    args = parser.parse_args()

    # Dispatch
    tool_mod = modules[args.tool]
    if not hasattr(tool_mod, "main"):
        parser.error(f"Tool '{args.tool}' does not define a main(args) function.")

    # if this tool is not "init" require that the work directory exists
    if args.tool != "init" and not tool_util.OkayToolUtil.is_good_for_work():
        sys.stderr.write("Error: This command must be run in a valid Okay project directory.\n")
        sys.exit(1)

    # Call the main function of the selected tool

    tool_mod.main(args)


if __name__ == "__main__":
    main()
