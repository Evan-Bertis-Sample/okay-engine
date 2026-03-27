#!/usr/bin/env python3

import argparse
import json
from pathlib import Path
from typing import Iterable


DEFAULT_OUTPUT = "compile_commands.json"


def require_okay_project():
    return False


def register_subparser(subparser):
    subparser.add_argument(
        "--project-dir",
        default=".",
        help="Root directory to search from",
    )
    subparser.add_argument(
        "--output",
        default=DEFAULT_OUTPUT,
        help="Path to write merged compile_commands.json",
    )
    subparser.add_argument(
        "--overwrite",
        action="store_true",
        help="Overwrite the output file if it already exists",
    )
    subparser.add_argument(
        "--quiet",
        action="store_true",
        help="Suppress per-file logging",
    )


def find_compile_commands(project_root: Path) -> Iterable[Path]:
    """
    Find all compile_commands.json files that live somewhere under a .okay/build directory.
    """
    for path in project_root.rglob("compile_commands.json"):
        parts = path.parts
        try:
            okay_idx = parts.index(".okay")
        except ValueError:
            continue

        # Require `.okay/build/.../compile_commands.json`
        if okay_idx + 2 >= len(parts):
            continue
        if parts[okay_idx + 1] != "build":
            continue

        yield path


def load_entries(path: Path) -> list[dict]:
    with path.open("r", encoding="utf-8") as f:
        data = json.load(f)

    if not isinstance(data, list):
        raise ValueError(f"{path} does not contain a JSON list")

    entries = []
    for i, entry in enumerate(data):
        if not isinstance(entry, dict):
            raise ValueError(f"{path}: entry {i} is not an object")

        if "file" not in entry:
            raise ValueError(f"{path}: entry {i} is missing 'file'")

        entries.append(entry)

    return entries


def normalize_file_key(entry: dict, db_path: Path) -> str:
    file_path = Path(entry["file"])

    if file_path.is_absolute():
        return str(file_path.resolve())

    directory = entry.get("directory")
    if directory:
        return str((Path(directory) / file_path).resolve())

    return str((db_path.parent / file_path).resolve())


def main(args):
    project_root = Path(args.project_dir).resolve()
    output_path = Path(args.output).resolve()

    if output_path.exists() and not args.overwrite:
        while True:
            choice = input(f"{output_path} exists. Overwrite? [y/n]: ").strip().lower()
            if choice == "y":
                break
            if choice == "n":
                return

    db_paths = sorted(find_compile_commands(project_root))

    if not db_paths:
        print("No compile_commands.json files found under .okay/build/")
        return

    merged_by_file: dict[str, dict] = {}
    total_entries = 0

    for db_path in db_paths:
        if not args.quiet:
            print(f"Loading {db_path.relative_to(project_root)}")

        try:
            entries = load_entries(db_path)
        except Exception as e:
            print(f"Skipping {db_path}: {e}")
            continue

        total_entries += len(entries)

        for entry in entries:
            key = normalize_file_key(entry, db_path)
            merged_by_file[key] = entry

    merged_entries = list(merged_by_file.values())

    output_path.write_text(
        json.dumps(merged_entries, indent=2) + "\n",
        encoding="utf-8",
        newline="\n",
    )

    print(f"Wrote {output_path}")
    print(f"Found {len(db_paths)} compilation database(s)")
    print(f"Loaded {total_entries} total entr{'y' if total_entries == 1 else 'ies'}")
    print(
        f"Wrote {len(merged_entries)} unique file entr{'y' if len(merged_entries) == 1 else 'ies'}"
    )
