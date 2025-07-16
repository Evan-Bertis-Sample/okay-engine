# okay_clean.py

import os
import sys
from tools import tool_util
import shutil

def register_subparser(subparser):
    # add a flag to specify if we should remove the work directory even if it is not empty
    subparser.add_argument(
        "--force",
        action="store_true",
        help="Force remove the work directory even if it is not empty",
    )

def main(args):
    okay_dir = tool_util.OkayToolUtil.get_okay_dir()
    okay_work_dir = tool_util.OkayToolUtil.get_okay_work_dir(os.getcwd())
    
    # check if the work directory exists
    if not os.path.exists(okay_work_dir):
        print(f"Error: The work directory {okay_work_dir} does not exist")
        return
    
    # remove the work directory, even if it is not empty
    print(f"Removing work directory: {okay_work_dir}")
    try:
        if args.force:
            shutil.rmtree(okay_work_dir)
        else:
            os.rmdir(okay_work_dir)
    except OSError as e:
        print(f"Error removing work directory: {e}")
        return