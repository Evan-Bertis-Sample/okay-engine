from tools.tool_util import OkayToolUtil
import subprocess

def require_okay_project():
    return False

def main(args):
    okay_root = OkayToolUtil.get_root_dir()

    # now we can run a git pull command, but first ask the user if they want to
    print("Attempting to update Raxel")
    print("This will run a git pull command in the Raxel directory")
    print("Specifically, this will run 'git pull' in the following directory:")
    print(okay_root)

    # ask the user if they want to continue
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
