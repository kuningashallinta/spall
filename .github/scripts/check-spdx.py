# SPDX-FileCopyrightText: 2026 King Hallinta
# SPDX-License-Identifier: Apache-2.0

import re
import sys
from pathlib import Path

SUFFIXES = (".h", ".inl", ".cpp")
COPYRIGHT = re.compile(r"^// SPDX-FileCopyrightText: \d{4} \S")
LICENSE = "// SPDX-License-Identifier: Apache-2.0"


def sources(root):
    for directory in ("src", "include"):
        for path in sorted((root / directory).rglob("*")):
            if path.suffix in SUFFIXES and path.is_file():
                yield path


def main():
    root = Path(__file__).resolve().parents[2]
    files = list(sources(root))

    if not files:
        print("Found no source files to check.")
        return 1

    missing = []

    for path in files:
        with path.open(encoding="utf-8") as source:
            head = [source.readline().rstrip("\r\n"), source.readline().rstrip("\r\n")]

        if not COPYRIGHT.match(head[0]) or head[1] != LICENSE:
            missing.append(path.relative_to(root))

    if missing:
        print(f"Missing or malformed SPDX header in {len(missing)} file(s):")

        for path in missing:
            print(f"  {path}")

        return 1

    print(f"All {len(files)} source files carry the SPDX header.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
