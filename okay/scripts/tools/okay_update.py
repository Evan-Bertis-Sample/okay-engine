from tools.tool_util import OkayToolUtil
import subprocess

def require_okay_project():
    return False

def register_subparser(sp):
    sp.add_argument("-y", "--yes", action="store_true", help="Automatically confirm the update without prompting")

def main(args):
    okay_root = OkayToolUtil.get_root_dir()

    # ask the user if they want to continue
    if not args.yes:
        print("Attempting to update Raxel")
        print("This will run a git pull command in the Raxel directory")
        print("Specifically, this will run 'git pull' in the following directory:")
        print(okay_root)

        while True:
            user_input = input("Continue? (y/n): ")
            if user_input.lower() == "y":
                break
            elif user_input.lower() == "n":
                print("Exiting")
                return
            else:
                print("Invalid input")

    # run the git pull command
    print("Running git pull")
    try:
        subprocess.run(["git", "pull"], cwd=okay_root, check=True)
    except subprocess.CalledProcessError as e:
        print(f"Error running git pull: {e}")
        return
