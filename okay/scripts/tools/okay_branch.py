from tools.tool_util import OkayToolUtil
import subprocess

def require_okay_project():
    return False

def register_subparser(sp):
    # list or switch
    sp.add_argument(
        "--list",
        action="store_true",
        help="List available branches",
    )

    sp.add_argument(
        "--switch",
        type=str,
        help="Switch to a specific branch",
    )

def main(args):
    okay_root = OkayToolUtil.get_root_dir()

    if not args.list and not args.switch:
        print("No action specified. Use --list to list branches or --switch to switch branches.")
        return

    # run git branch, and get the possible branches
    if args.list:
        try :
            subprocess.run(["git", "branch"], cwd=okay_root, check=True)
        except subprocess.CalledProcessError as e:
            print(f"Error running git branch: {e}")
            return
        
    if args.switch:
        print(f"Switching to branch {args.switch}")
        try:
            subprocess.run(["git", "switch", args.switch], cwd=okay_root, check=True)
        except subprocess.CalledProcessError as e:
            print(f"Error running git switch: {e}")
            return
