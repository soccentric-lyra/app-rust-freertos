#!/usr/bin/env python3
#
# check_ffi.py - compare the C platform prototypes with the Rust extern block
#
# Author:  Sandesh Ghimire <sandesh@soccentric.com>
# Usage:   scripts/check_ffi.py
#
# cbindgen/bindgen would generate both sides, but this project deliberately
# keeps the C side hand-written so the platform layer stays small and
# readable.  Instead, this check fails when a Rust `extern "C"` declaration has
# no matching C prototype with the same argument count, or when the two sides
# disagree about a name.  It catches the common drift (a parameter added on one
# side only) that a normal build would silently accept.
#

import pathlib
import re
import sys

C_HEADER = pathlib.Path("platform/platform.h")
RUST_SIDE = pathlib.Path("src/platform/plat.rs")

C_PROTO = re.compile(r"\b(plat_[A-Za-z0-9_]+)\s*\(([^;{}]*)\)\s*;", re.DOTALL)
RUST_FN = re.compile(
    r"\bpub\s+fn\s+(plat_[A-Za-z0-9_]+)\s*\(([^;{}]*?)\)\s*(?:->\s*[^;]+)?;",
    re.DOTALL,
)
RUST_FFI_BLOCK = re.compile(r"mod\s+ffi\s*\{(.*?)\}", re.DOTALL)


def strip_c_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.DOTALL)
    text = re.sub(r"//[^\n]*", "", text)
    # Drop GCC attributes such as __attribute__((noreturn)) so the argument
    # list is the only parenthesised group left after a prototype name.
    return re.sub(r"__attribute__\s*\(\(.*?\)\)", "", text, flags=re.DOTALL)


def count_args(arg_text: str) -> int:
    arg_text = " ".join(arg_text.split())
    if not arg_text or arg_text == "void":
        return 0
    depth = 0
    count = 1
    for char in arg_text:
        if char in "([":
            depth += 1
        elif char in ")]":
            depth -= 1
        elif char == "," and depth == 0:
            count += 1
    return count


def main() -> int:
    if not C_HEADER.is_file() or not RUST_SIDE.is_file():
        print("ffi: platform header or Rust platform module is missing", file=sys.stderr)
        return 1

    c_text = strip_c_comments(C_HEADER.read_text(encoding="utf-8"))
    c_functions = {
        name: count_args(args) for name, args in C_PROTO.findall(c_text)
    }

    rust_text = RUST_SIDE.read_text(encoding="utf-8")
    block = RUST_FFI_BLOCK.search(rust_text)
    if not block:
        print("ffi: no extern block found in src/platform/plat.rs", file=sys.stderr)
        return 1
    rust_functions = {
        name: count_args(args) for name, args in RUST_FN.findall(block.group(1))
    }

    failed = False
    for name, rust_args in sorted(rust_functions.items()):
        if name not in c_functions:
            print(f"ffi: Rust declares {name}, C header does not", file=sys.stderr)
            failed = True
        elif c_functions[name] != rust_args:
            print(
                f"ffi: {name} has {rust_args} Rust arguments but "
                f"{c_functions[name]} C parameters",
                file=sys.stderr,
            )
            failed = True

    if failed:
        return 1
    print(
        f"ffi: {len(rust_functions)} Rust extern functions match the C prototypes"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
