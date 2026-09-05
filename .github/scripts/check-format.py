# SPDX-FileCopyrightText: 2026 King Hallinta
# SPDX-License-Identifier: Apache-2.0

import argparse
import subprocess
import sys
from pathlib import Path

SUFFIXES = (".h", ".inl", ".cpp")


def sources(root):
    for directory in ("src", "include"):
        for path in sorted((root / directory).rglob("*")):
            if path.suffix in SUFFIXES and path.is_file():
                yield path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--clang-format", default="clang-format")
    arguments = parser.parse_args()

    root = Path(__file__).resolve().parents[2]
    files = list(sources(root))

    if not files:
        print("Found no source files to check.")
        return 1

    violations = []

    for path in files:
        completed = subprocess.run(
            [arguments.clang_format, "--dry-run", "-Werror", "--style=file", str(path)]
        )

        if completed.returncode != 0:
            violations.append(path.relative_to(root))

    if violations:
        print(f"clang-format reported {len(violations)} file(s) needing reformatting:")

        for path in violations:
            print(f"  {path}")

        return 1

    print(f"clang-format is clean over {len(files)} files.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
