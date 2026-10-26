#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# *****************************************************************************
# Copyright 2026 Autodesk, Inc. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# *****************************************************************************

"""Self-verifying tests for remove_absolute_runpath.py.

Run directly:

    python3 src/build/test_remove_absolute_runpath.py

Synthetic ELF files, built here byte by byte, check the rewrite on every platform: absolute entries
go, relative ones stay, $ORIGIN is added when nothing relative is left, and a file whose search path
shares bytes with another string is left alone. On Linux a real library is also built with an
absolute RUNPATH, fixed, and loaded by a program after its original directory is removed.

Exits non-zero if any check fails.
"""

import os
import shutil
import struct
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import remove_absolute_runpath as module  # noqa: E402

FAILURES = []


def check(description: str, condition: bool, detail: str = "") -> None:
    if condition:
        print(f"  ok   {description}")
    else:
        print(f"  FAIL {description}{(': ' + detail) if detail else ''}")
        FAILURES.append(description)


def synth_elf(path: str, runpath: str, symbol_inside_runpath: bool = False, section_headers: bool = True) -> str:
    """A 64-bit shared object with a dynamic section naming libz.so.1 and the given RUNPATH, and one dynamic symbol."""
    strings = b"\0libz.so.1\0" + runpath.encode() + b"\0sym\0"
    needed = 1
    runpath_at = strings.index(runpath.encode())
    symbol_name = runpath_at + 1 if symbol_inside_runpath else strings.index(b"sym\0")

    strtab, dynamic, dynsym, sections = 0x200, 0x400, 0x480, 0x500
    data = bytearray(0x600)
    data[0:16] = b"\x7fELF\x02\x01\x01" + b"\0" * 9
    struct.pack_into(
        "<HHIQQQIHHHHHH",
        data,
        16,
        3,
        62,
        1,
        0,
        64,
        sections if section_headers else 0,
        0,
        64,
        56,
        2,
        64,
        2 if section_headers else 0,
        0,
    )
    # One PT_LOAD mapping the whole file at address 0, and PT_DYNAMIC.
    struct.pack_into("<IIQQQQQQ", data, 64, module.PT_LOAD, 5, 0, 0, 0, len(data), len(data), 0x1000)
    struct.pack_into("<IIQQQQQQ", data, 64 + 56, module.PT_DYNAMIC, 6, dynamic, dynamic, dynamic, 64, 64, 8)
    data[strtab : strtab + len(strings)] = strings
    for index, (tag, value) in enumerate(
        ((module.DT_NEEDED, needed), (module.DT_STRTAB, strtab), (module.DT_RUNPATH, runpath_at), (module.DT_NULL, 0))
    ):
        struct.pack_into("<qQ", data, dynamic + index * 16, tag, value)
    # A null symbol and one named symbol.
    struct.pack_into("<I", data, dynsym + 24, symbol_name)
    if section_headers:
        struct.pack_into("<IIQQQQIIQQ", data, sections + 64, 0, module.SHT_DYNSYM, 0, dynsym, dynsym, 48, 0, 1, 8, 24)
    with open(path, "wb") as handle:
        handle.write(bytes(data))
    return path


def runpath_of(path: str) -> str:
    with open(path, "rb") as handle:
        data = handle.read()
    elf = module.Elf(data)
    values = dict(elf.dynamic())
    start = elf.file_offset(values[module.DT_STRTAB]) + values[module.DT_RUNPATH]
    return data[start : data.index(b"\0", start)].decode()


def test_rewrite_rules() -> None:
    print("search path rewriting")
    check("relative entries alone are unchanged", module.rewritten(["$ORIGIN", "$ORIGIN/../lib"]) is None)
    check(
        "an absolute entry is dropped",
        module.rewritten(["$ORIGIN/../lib", "/build/_build/RV_DEPS_ZLIB/install/lib"]) == ["$ORIGIN/../lib"],
    )
    check(
        "$ORIGIN is added when nothing relative is left",
        module.rewritten(["/build/_build/RV_DEPS_ZLIB/install/lib"]) == ["$ORIGIN"],
    )
    check("${ORIGIN} counts as relative to the file", module.rewritten(["${ORIGIN}/x", "/abs"]) == ["${ORIGIN}/x"])


def test_synthetic_files(directory: str) -> None:
    print("synthetic ELF files")
    path = synth_elf(os.path.join(directory, "libmixed.so"), "/build/_build/RV_DEPS_ZLIB/install/lib:$ORIGIN/../lib")
    size = os.path.getsize(path)
    result = module.fix(path)
    check(
        "the change is reported",
        result == [("/build/_build/RV_DEPS_ZLIB/install/lib:$ORIGIN/../lib", "$ORIGIN/../lib")],
        str(result),
    )
    check(
        "the absolute entry is gone and the relative one kept", runpath_of(path) == "$ORIGIN/../lib", runpath_of(path)
    )
    check("the file size is unchanged", os.path.getsize(path) == size)
    with open(path, "rb") as handle:
        check("the build path is no longer in the file", b"/build/_build" not in handle.read())
    check("a second pass changes nothing", module.fix(path) is None)

    path = synth_elf(os.path.join(directory, "libabsolute.so"), "/home/build/_build/RV_DEPS_ZLIB/install/lib")
    module.fix(path)
    check("an only-absolute search path becomes $ORIGIN", runpath_of(path) == "$ORIGIN", runpath_of(path))

    path = synth_elf(
        os.path.join(directory, "libshared.so"), "/build/_build/RV_DEPS_ZLIB/install/lib", symbol_inside_runpath=True
    )
    raised = False
    try:
        module.fix(path)
    except ValueError as error:
        raised = "shares" in str(error)
    check(
        "a search path sharing bytes with a symbol name is left alone",
        raised and runpath_of(path) == "/build/_build/RV_DEPS_ZLIB/install/lib",
    )

    path = synth_elf(
        os.path.join(directory, "libnosections.so"), "/build/_build/RV_DEPS_ZLIB/install/lib", section_headers=False
    )
    raised = False
    try:
        module.fix(path)
    except ValueError as error:
        raised = "section headers" in str(error)
    check(
        "a file without section headers is left alone",
        raised and runpath_of(path) == "/build/_build/RV_DEPS_ZLIB/install/lib",
    )

    tree = os.path.join(directory, "tree")
    os.makedirs(os.path.join(tree, "lib"))
    synth_elf(os.path.join(tree, "lib", "liba.so"), "/abs/lib")
    with open(os.path.join(tree, "lib", "notes.txt"), "w", encoding="utf-8") as handle:
        handle.write("/abs/lib\n")
    result = subprocess.run(
        [sys.executable, os.path.join(HERE, "remove_absolute_runpath.py"), tree], capture_output=True, text=True
    )
    check(
        "the tree walk rewrites ELF files",
        result.returncode == 0 and runpath_of(os.path.join(tree, "lib", "liba.so")) == "$ORIGIN",
        result.stdout,
    )
    with open(os.path.join(tree, "lib", "notes.txt"), encoding="utf-8") as handle:
        check("files that are not ELF are not touched", handle.read() == "/abs/lib\n")


LIBRARY = 'extern "C" int value() { return 7; }\n'
MAIN = 'extern "C" int value();\nint main() { return value() == 7 ? 0 : 1; }\n'


def test_real_library(directory: str) -> None:
    print("a real library on Linux")
    if not sys.platform.startswith("linux") or shutil.which("c++") is None:
        print("  skip not on Linux with a C++ compiler")
        return
    original = os.path.join(directory, "original")
    installed = os.path.join(directory, "installed")
    os.makedirs(original)
    os.makedirs(installed)
    with open(os.path.join(directory, "value.cpp"), "w", encoding="utf-8") as handle:
        handle.write(LIBRARY)
    with open(os.path.join(directory, "main.cpp"), "w", encoding="utf-8") as handle:
        handle.write(MAIN)
    # The library depends on nothing of its own, so the program carries the absolute search path: a dependent whose
    # RUNPATH names where the library was built, as an RV dependency's names the build directory.
    subprocess.run(
        ["c++", "-shared", "-fPIC", "-o", os.path.join(original, "libvalue.so"), os.path.join(directory, "value.cpp")],
        check=True,
    )
    subprocess.run(
        [
            "c++",
            "-o",
            os.path.join(installed, "program"),
            os.path.join(directory, "main.cpp"),
            f"-L{original}",
            "-lvalue",
            f"-Wl,-rpath,{original},--enable-new-dtags",
        ],
        check=True,
    )
    shutil.copy(os.path.join(original, "libvalue.so"), installed)
    module.fix(os.path.join(installed, "program"))
    shutil.rmtree(original)
    environment = {key: value for key, value in os.environ.items() if key != "LD_LIBRARY_PATH"}
    result = subprocess.run([os.path.join(installed, "program")], env=environment, capture_output=True, text=True)
    check(
        "the program loads the library beside it once the absolute path is gone", result.returncode == 0, result.stderr
    )
    if shutil.which("readelf"):
        dynamic = subprocess.run(
            ["readelf", "-d", os.path.join(installed, "program")], capture_output=True, text=True
        ).stdout
        check("readelf shows only $ORIGIN", "[$ORIGIN]" in dynamic and original not in dynamic, dynamic[-400:])


def main() -> int:
    print("remove_absolute_runpath.py")
    test_rewrite_rules()
    with tempfile.TemporaryDirectory() as directory:
        test_synthetic_files(directory)
        test_real_library(directory)
    if FAILURES:
        print(f"\n{len(FAILURES)} check(s) failed:")
        for failure in FAILURES:
            print(f"  - {failure}")
        return 1
    print("\nall RUNPATH checks passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
