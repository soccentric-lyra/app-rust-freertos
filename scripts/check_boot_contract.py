#!/usr/bin/env python3
#
# check_boot_contract.py - verify the BOOT layout constants in the C platform header
#
# Author:  Sandesh Ghimire <sandesh@soccentric.com>
# Usage:   scripts/check_boot_contract.py [--header PATH]
#
# Fails when platform/boot/boot_contract.h and the BOOT layout have drifted.
#

import argparse
import pathlib
import re
import sys

EXPECTED = {
    "APP_SLOT_BASE": 0x0800C000,
    "APP_SLOT_SIZE": 0x000F4000,
    "APP_META_A_ADDR": 0x08008000,
    "APP_META_B_ADDR": 0x08108000,
    "APP_META_VERIFIED_OFFSET": 0x100,
    "APP_META_ACTIVATED_OFFSET": 0x120,
    "APP_META_CONFIRMED_OFFSET": 0x140,
    "APP_META_REJECTED_OFFSET": 0x160,
}

PATTERN = re.compile(
    r"^\s*#define\s+([A-Z0-9_]+)\s+(0x[0-9A-Fa-f]+|\d+)"
)


def read_defines(path: pathlib.Path) -> dict[str, int]:
    defines: dict[str, int] = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        match = PATTERN.match(line)
        if match:
            defines[match.group(1)] = int(match.group(2), 0)
    return defines


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--header",
        default="platform/boot/boot_contract.h",
        help="boot contract header (default: %(default)s)",
    )
    args = parser.parse_args()

    path = pathlib.Path(args.header)
    if not path.is_file():
        print(f"boot contract: {path} not found", file=sys.stderr)
        return 1

    defines = read_defines(path)
    failed = False
    for name, want in EXPECTED.items():
        if name not in defines:
            print(f"boot contract: {name} is missing from {path}", file=sys.stderr)
            failed = True
        elif defines[name] != want:
            print(
                f"boot contract: {name} is 0x{defines[name]:08X}, "
                f"expected 0x{want:08X}",
                file=sys.stderr,
            )
            failed = True

    if failed:
        return 1
    print("boot contract: slot, metadata and record offsets match the BOOT layout")
    return 0


if __name__ == "__main__":
    sys.exit(main())
