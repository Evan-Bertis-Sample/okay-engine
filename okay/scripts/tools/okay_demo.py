# okay_demo.py

import os
import sys
from tools.build_util import OkayBuildOptions, OkayBuildType, OkayBuildUtil
from tools.tool_util import OkayToolUtil

def require_okay_project():
    return False

def register_subparser(subparser):
    OkayBuildOptions.add_subparser_args(subparser)
    subparser.add_argument(
        "--gdb",
        action="store_true",
        help="Run the project with gdb",
    )
    subparser.add_argument(
        "--proto-name",
        action="store",
        help="Override the default target/project name (demo)",
        default="demo"
    )



def main(args):
    build_options = OkayBuildOptions.from_args(args)
    project_dir = OkayToolUtil.get_root_dir() / "proto" / args.proto_name

    if not os.path.exists(project_dir):
        print(f"Error: The project directory {project_dir} does not exist")
        return
    
    build_options.project_dir = project_dir
    OkayBuildUtil.build_project(build_options)
    OkayBuildUtil.run_project(build_options, use_gdb=args.gdb)