#!/usr/bin/env python3
"""
Generate and validate file headers for Okay Engine.
"""

import argparse
from pathlib import Path

HEADER_TEMPLATE = """\
// Copyright (c) 2026 Okay Engine contributors
// SPDX-License-Identifier: MIT
//
// {filename} - Okay Engine

"""

HEADER_LINES = 5


def expected_header(path: Path) -> str:
    return HEADER_TEMPLATE.format(filename=path.name)


def process_file(path: Path, check: bool = False) -> bool:
    """Return True if the file needs updating."""

    with path.open("r", encoding="utf-8", newline="") as f:
        content = f.read()

    newline = "\r\n" if "\r\n" in content else "\n"
    header = expected_header(path).replace("\n", newline)

    if content.startswith("// Copyright"):
        body = "".join(content.splitlines(keepends=True)[HEADER_LINES:])
    else:
        body = content

    updated = header + body

    if content == updated:
        return False

    if not check:
        with path.open("w", encoding="utf-8", newline="") as f:
            f.write(updated)

    return True


def main():
    parser = argparse.ArgumentParser(
        description="Generate Okay Engine file headers."
    )
    parser.add_argument(
        "root",
        type=Path,
        help="Root directory to recursively scan.",
    )
    parser.add_argument(
        "--check",
        action="store_true",
        help="Validate headers without modifying files.",
    )

    args = parser.parse_args()
    root = args.root.resolve()

    if not root.is_dir():
        parser.error(f"Not a directory: {root}")

    changed = 0

    for path in root.rglob("*.hpp"):
        if "vendor" in path.relative_to(root).parts:
            continue

        if process_file(path, args.check):
            print(f"{'Invalid' if args.check else 'Updated'}: {path.relative_to(root)}")
            changed += 1

    print(f"{changed} file(s) {'invalid' if args.check else 'updated'}.")

    if args.check and changed:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
