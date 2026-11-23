# okay_build.py

import os
import sys
from tools.build_util import OkayBuildOptions, OkayBuildType, OkayBuildUtil
from tools.tool_util import OkayToolUtil, OkayLogger, OkayLogType

def register_subparser(subparser):
    OkayBuildOptions.add_subparser_args(subparser)


def main(args):
    build_options = OkayBuildOptions.from_args(args)

    if not OkayToolUtil.is_good_for_work(args.project_dir):
        OkayLogger.log(f"Cannot build project! {args.project_dir} is not an okay project!", OkayLogType.ERROR);
        return

    OkayBuildUtil.build_project(build_options)
