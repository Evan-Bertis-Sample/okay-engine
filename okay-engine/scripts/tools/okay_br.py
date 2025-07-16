# okay_br.py

import os
import sys
from tools.build_util import OkayBuildOptions, OkayBuildType, OkayBuildUtil
from tools.tool_util import OkayToolUtil

def register_subparser(subparser):
    OkayBuildOptions.add_subparser_args(subparser)

    subparser.add_argument(
        "--gdb",
        action="store_true",
        help="Run the project with gdb",
    )

def main(args):
    build_options = OkayBuildOptions()
    build_options.set_build_type(OkayBuildType.from_string(args.build_type))
    # if the project directory is not specified (default is "."), then use the current directory
    project_dir = args.project_dir
    if project_dir == ".":
        project_dir = os.getcwd()
    build_options.set_project_dir(project_dir)

    OkayBuildUtil.build_project(build_options)
    OkayBuildUtil.compile_shaders(project_dir)
    OkayBuildUtil.run_project(build_options, use_gdb=args.gdb)