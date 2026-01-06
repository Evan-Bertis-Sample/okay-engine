# okay_clean.py

import os
import sys
from tools import tool_util
import shutil

def require_okay_project():
    return False

def main(args):
    # initialize the work directory, if needed
    # if it already exists, just say that it exists
    okay_work_dir = tool_util.OkayToolUtil.get_okay_work_dir(os.getcwd())
    if not os.path.exists(okay_work_dir):
        os.makedirs(okay_work_dir)
        print(f"Initialized okay project")
    else:
        print(f"Okay project already initialized")
    