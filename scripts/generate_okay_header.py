#!/usr/bin/env python3

import argparse
from collections import defaultdict
from pathlib import Path


HEADER_GUARD = "__OKAY_H__"


def collect_headers(core_dir: Path, output_file: Path) -> list[Path]:
    headers: list[Path] = []

    for path in core_dir.rglob("*.hpp"):
        if path.resolve() == output_file.resolve():
            continue
        headers.append(path)

    headers.sort(key=lambda p: p.as_posix())
    return headers


def group_headers(repo_root: Path, headers: list[Path]) -> dict[str, list[str]]:
    groups: dict[str, list[str]] = defaultdict(list)

    for header in headers:
        rel = header.relative_to(repo_root).as_posix()  # okay/core/...

        group = header.parent.relative_to(repo_root).as_posix()
        groups[group].append(rel)

    return dict(sorted(groups.items(), key=lambda item: item[0]))


def generate_header_text(grouped_headers: dict[str, list[str]]) -> str:
    lines: list[str] = []

    lines.append(f"#ifndef {HEADER_GUARD}")
    lines.append(f"#define {HEADER_GUARD}")
    lines.append("")
    lines.append("// This file is auto-generated. Do not edit manually.")
    lines.append("")

    first = True
    for group, headers in grouped_headers.items():
        if not first:
            lines.append("")
        first = False

        lines.append(f"// {group}")
        for h in headers:
            lines.append(f"#include <{h}>")

    lines.append("")
    lines.append(f"#endif  // {HEADER_GUARD}")
    lines.append("")

    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Generate okay/okay.hpp from headers in okay/core/"
    )

    parser.add_argument(
        "--repo-root",
        type=Path,
        default=Path(__file__).resolve().parent.parent,
        help="Path to repo root (default: inferred from script location)",
    )

    parser.add_argument(
        "--core-dir",
        type=Path,
        default=None,
        help="Path to okay/core directory (default: <repo-root>/okay/core)",
    )

    parser.add_argument(
        "--output",
        type=Path,
        default=None,
        help="Output file (default: <repo-root>/okay/okay.hpp)",
    )

    args = parser.parse_args()

    repo_root = args.repo_root.resolve()
    core_dir = (args.core_dir or (repo_root / "okay" / "core")).resolve()
    output_file = (args.output or (repo_root / "okay" / "okay.hpp")).resolve()

    if not core_dir.exists():
        raise FileNotFoundError(f"Core directory not found: {core_dir}")

    headers = collect_headers(core_dir, output_file)
    grouped = group_headers(repo_root, headers)
    content = generate_header_text(grouped)

    output_file.write_text(content, encoding="utf-8", newline="\n")

    print(f"Generated: {output_file}")
    print(f"{len(headers)} headers across {len(grouped)} groups.")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())