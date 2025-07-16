# okay_run.py

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
    build_options = OkayBuildOptions.from_args(args)
    OkayBuildUtil.run_project(build_options, use_gdb=args.gdb)