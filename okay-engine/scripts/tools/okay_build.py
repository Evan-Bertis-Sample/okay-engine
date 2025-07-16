# okay_build.py

import os
import sys
from tools.build_util import OkayBuildOptions, OkayBuildType, OkayBuildUtil
from tools.tool_util import OkayToolUtil

def register_subparser(subparser):
    OkayBuildOptions.add_subparser_args(subparser)


def main(args):
    build_options = OkayBuildOptions.from_args(args)
    OkayBuildUtil.build_project(build_options)