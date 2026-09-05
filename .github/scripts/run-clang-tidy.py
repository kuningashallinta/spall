# SPDX-FileCopyrightText: 2026 King Hallinta
# SPDX-License-Identifier: Apache-2.0

import argparse
import subprocess
import sys
from pathlib import Path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-directory", required=True)
    parser.add_argument("--clang-tidy", default="clang-tidy")
    arguments = parser.parse_args()

    root = Path(__file__).resolve().parents[2]
    build = Path(arguments.build_directory)
    database = build / "compile_commands.json"

    if not database.is_file():
        print(f"No compilation database at {database}.")
        print("Configure with -DCMAKE_EXPORT_COMPILE_COMMANDS=ON first.")
        return 1

    units = [path for path in sorted((root / "src").rglob("*.cpp")) if path.is_file()]

    if not units:
        print("Found no translation units to check.")
        return 1

    failures = []

    for path in units:
        completed = subprocess.run(
            [arguments.clang_tidy, "-p", str(build), "--quiet", str(path)]
        )

        if completed.returncode != 0:
            failures.append(path.relative_to(root))

    if failures:
        print(f"clang-tidy reported problems in {len(failures)} translation unit(s):")

        for path in failures:
            print(f"  {path}")

        return 1

    print(f"clang-tidy is clean over {len(units)} translation units.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
