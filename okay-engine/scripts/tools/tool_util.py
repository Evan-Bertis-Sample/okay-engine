# tool_utils.py

import os
import enum
import subprocess
import pathlib


class OkayToolUtil:
    @staticmethod
    def get_root_dir():
        # parent's parent's parent's parent directory
        return pathlib.Path(__file__).resolve().parents[3]

    @staticmethod
    def get_okay_dir():
        return OkayToolUtil.get_root_dir() / "okay-engine"

    @staticmethod
    def get_okay_tool_dir():
        return OkayToolUtil.get_okay_dir() / "scripts" / "tools"

    @staticmethod
    def get_okay_work_dir(project_dir : str):
        # get where this file was executed
        return pathlib.Path(project_dir) / ".okay"
    
    @staticmethod
    def get_okay_parent_dir():
        return OkayToolUtil.get_okay_dir().parent

    @staticmethod
    def get_okay_build_dir(build_options: "OkayBuildOptions"):
        build_subfolder = build_options.target.lower() + "_" + build_options.build_type.name.lower()
        return OkayToolUtil.get_okay_work_dir(build_options.project_dir) / "build" / build_subfolder

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

