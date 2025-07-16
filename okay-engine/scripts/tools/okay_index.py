# okay_index.py

from tools.tool_util import OkayToolUtil

import os
import json
from tools.build_util import OkayBuildOptions, OkayBuildType, OkayBuildUtil
from tools.tool_util import OkayToolUtil

def register_subparser(subparser):
    OkayBuildOptions.add_subparser_args(subparser)

    # option to force overwrite the c_cpp_properties.json file
    subparser.add_argument(
        "--overwrite",
        action="store_true",
        help="Overwrite the c_cpp_properties.json file",
    )


CPP_PROPERTIES_PATH = ".vscode/c_cpp_properties.json"

def create_base_cpp_properties_object():
    okay_includes = []
    okay_includes.append(OkayToolUtil.get_okay_parent_dir())

    obj = {}
    obj["configurations"] = [
        {
            "name": "okay",
            "includePath": okay_includes,
            "defines": [],
        }
    ]
    return obj

def main(args):
    print("Regenerating c_cpp_properties.json file")

    build_options = OkayBuildOptions()
    build_options.set_build_type(OkayBuildType.from_string(args.build_type))
    # if the project directory is not specified (default is "."), then use the current directory
    project_dir = args.project_dir
    if project_dir == ".":
        project_dir = os.getcwd()

    cpp_properties_file = os.path.join(project_dir, CPP_PROPERTIES_PATH)

    os.makedirs(os.path.dirname(cpp_properties_file), exist_ok=True)

    with open(cpp_properties_file, "w") as f:
        properties = None

        if os.path.exists(cpp_properties_file):
            try:
                properties = json.load(f)
            except Exception as e:
                print("Error: Could not load c_cpp_properties.json file")
                print(e)

                if not args.overwrite:
                    # now query the user if they just want to overwrite the file
                    while (True):
                        choice = input("Do you want to overwrite the file? [y/n]: ")
                        if choice.lower() == "y":
                            break
                        elif choice.lower() == "n":
                            return
                        else:
                            print("Invalid choice")

        if properties == None:
            properties = create_base_cpp_properties_object()
        else:
            additional_properties = create_base_cpp_properties_object()
            if "configurations" in properties:
                properties["configurations"].extend(additional_properties["configurations"])
            else:
                properties["configurations"] = additional_properties["configurations"]

        f.write(json.dumps(properties, indent=4))
        
    print("Regenerated c_cpp_properties.json file, which should allow for intellisense to work")
    print("Please refresh VSCode to see the changes, if you don't see them immediately")