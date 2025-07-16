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

class OkayLogType(enum.Enum):
    INFO = "INFO"
    WARNING = "WARNING"
    ERROR = "ERROR"

    def __str__(self):
        return self.value
    
    @staticmethod
    def get_prefix_color(log_type : "OkayLogType") -> str:
        if log_type == OkayLogType.INFO:
            return "\033[94m"
        elif log_type == OkayLogType.WARNING:
            return "\033[93m"
        elif log_type == OkayLogType.ERROR:
            return "\033[91m"
        return "\033[0m"

    @staticmethod
    def get_suffix_color(log_type : "OkayLogType") -> str:
        return "\033[0m"  # Reset color
    
    @staticmethod
    def get_log_prefix(log_type: "OkayLogType") -> str:
        return f"[{log_type.name}]"

class OkayLogger:
    @staticmethod
    def log(message: str, log_type: "OkayLogType" = OkayLogType.INFO):
        prefix = OkayLogType.get_prefix_color(log_type)
        suffix = OkayLogType.get_suffix_color(log_type)
        if log_type == OkayLogType.INFO:
            print(f"{prefix}[okay][{log_type.name}]{suffix} {message}")
        else:
            print(f"{prefix}[okay][{log_type.name}] {message}{suffix}")
