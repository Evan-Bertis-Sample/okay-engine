# okay_where.py

from tools.tool_util import OkayToolUtil

def require_okay_project():
    return False


def main(args):
    print(OkayToolUtil.get_okay_parent_dir())