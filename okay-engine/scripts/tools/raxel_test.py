# okay_build.py

import os
import sys
from tools.build_util import OkayBuildOptions, OkayBuildType, OkayBuildUtil
from tools.tool_util import OkayToolUtil

TEST_APPLICATION = "okay-test"

def register_subparser(subparser):
    subparser.add_argument(
        "--gdb",
        action="store_true",
        help="Run the project with gdb",
    )


def main(args):
    build_options = OkayBuildOptions()
    build_options.set_build_type(OkayBuildType.Debug)
    # if the project directory is not specified (default is "."), then use the current directory
    project_dir = OkayToolUtil.get_root_dir() + os.path.sep + TEST_APPLICATION

    if not os.path.exists(project_dir):
        print(f"Error: The project directory {project_dir} does not exist")
        sys.exit(1)

    build_options.set_project_dir(project_dir)
    OkayBuildUtil.build_project(build_options)
    OkayBuildUtil.run_project(build_options, use_gdb=args.gdb)