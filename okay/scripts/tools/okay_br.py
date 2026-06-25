# okay_br.py

import os
import sys

from tools.build_util import OkayBuildOptions, OkayBuildType, OkayBuildUtil
from tools.tool_util import OkayLogger, OkayLogType, OkayToolUtil


def register_subparser(subparser):
    OkayBuildOptions.add_subparser_args(subparser)

    subparser.add_argument(
        "--gdb",
        action="store_true",
        help="Run the project with gdb",
    )


def main(args):
    build_options = OkayBuildOptions.from_args(args)
    if OkayBuildUtil.build_project(build_options):
        OkayBuildUtil.run_project(build_options, use_gdb=args.gdb)
    else:
        OkayLogger.log("Failed to build project!", OkayLogType.ERROR)
