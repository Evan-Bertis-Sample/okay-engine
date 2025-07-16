# tool_utils.py

import os
import enum
import subprocess


class OkayToolUtil:
    @staticmethod
    def get_root_dir():
        return os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))

    @staticmethod
    def get_okay_dir():
        return os.path.join(OkayToolUtil.get_root_dir(), "okay")

    @staticmethod
    def get_okay_tool_dir():
        return os.path.join(OkayToolUtil.get_okay_dir(), "scripts", "tools")

    @staticmethod
    def get_okay_work_dir(project_dir : str):
        # get where this file was executed
        return os.path.join(project_dir, ".okay")
    
    @staticmethod
    def get_okay_parent_dir():
        return os.path.dirname(OkayToolUtil.get_okay_dir())

    @staticmethod
    def execute_bash_script(script_path: str, args: list):
        # check if the script exists
        if not os.path.exists(script_path):
            print(f"Error: The script {script_path} does not exist")
            return
        
        print(f"Executing script: {script_path}")
        print(f"Arguments: {args}")

        # execute the script
        os.exec([script_path] + args, shell=True)

