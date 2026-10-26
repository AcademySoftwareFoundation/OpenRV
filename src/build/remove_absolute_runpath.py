#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# *****************************************************************************
# Copyright 2026 Autodesk, Inc. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# *****************************************************************************

"""Remove absolute RUNPATH and RPATH entries from the ELF files of an installed tree.

    python3 src/build/remove_absolute_runpath.py <install prefix>

The Linux counterpart of remove_absolute_rpath.py. A dependency built with an absolute search path --
its own install directory, or a dependency's, inside the build directory -- keeps it when RV is
installed, so an installed library still searches the machine it was built on, and on a machine where
that path happens to exist loads whatever is there. A plain Linux build of RV installs dozens of such
entries, in OpenImageIO among others.

Absolute entries are dropped. Entries relative to the file ($ORIGIN, $LIB, $PLATFORM) are kept. When
an entry is dropped and none relative to $ORIGIN is left, $ORIGIN is added: unlike macOS, where an
executable's rpaths apply to every library it loads, an ELF library's own RUNPATH is what finds its
dependencies, and in an installed RV they sit beside it.

The search path is rewritten in place in the dynamic string table, as chrpath does -- it only ever gets
shorter, padded with NULs -- so no tool is needed. A file is left alone, with a warning, if another
string in the table shares the search path's bytes, which a linker merging string tails may do, or if
its section headers are missing so that cannot be checked.
"""

import os
import struct
import sys

DT_NULL, DT_NEEDED, DT_STRTAB, DT_STRSZ, DT_SONAME, DT_RPATH, DT_RUNPATH = 0, 1, 5, 10, 14, 15, 29
PT_LOAD, PT_DYNAMIC = 1, 2
SHT_DYNSYM, SHT_GNU_VERDEF, SHT_GNU_VERNEED = 11, 0x6FFFFFFD, 0x6FFFFFFE


def rewritten(entries):
    """The entries to keep, or None if nothing changes."""
    kept = [entry for entry in entries if entry and not entry.startswith("/")]
    if len(kept) == len([entry for entry in entries if entry]):
        return None
    if not any(entry.startswith(("$ORIGIN", "${ORIGIN}")) for entry in kept):
        kept.insert(0, "$ORIGIN")
    return kept


class Elf:
    def __init__(self, data: bytes):
        if data[:4] != b"\x7fELF" or data[5] != 1:
            raise ValueError("not a little-endian ELF file")
        self.data = data
        self.wide = data[4] == 2
        if self.wide:
            self.phoff, self.shoff = struct.unpack_from("<QQ", data, 0x20)
            self.phentsize, self.phnum, self.shentsize, self.shnum = struct.unpack_from("<HHHH", data, 0x36)
        else:
            self.phoff, self.shoff = struct.unpack_from("<II", data, 0x1C)
            self.phentsize, self.phnum, self.shentsize, self.shnum = struct.unpack_from("<HHHH", data, 0x2A)

    def segments(self):
        for index in range(self.phnum):
            base = self.phoff + index * self.phentsize
            if self.wide:
                kind, _flags, offset, vaddr, _paddr, filesz = struct.unpack_from("<IIQQQQ", self.data, base)
            else:
                kind, offset, vaddr, _paddr, filesz = struct.unpack_from("<IIIII", self.data, base)
            yield kind, offset, vaddr, filesz

    def sections(self):
        for index in range(self.shnum):
            base = self.shoff + index * self.shentsize
            if self.wide:
                _name, kind, _flags, _addr, offset, size, link, _info, _align, entsize = struct.unpack_from(
                    "<IIQQQQIIQQ", self.data, base
                )
            else:
                _name, kind, _flags, _addr, offset, size, link, _info, _align, entsize = struct.unpack_from(
                    "<IIIIIIIIII", self.data, base
                )
            yield kind, offset, size, entsize

    def file_offset(self, vaddr):
        for kind, offset, start, size in self.segments():
            if kind == PT_LOAD and start <= vaddr < start + size:
                return vaddr - start + offset
        return None

    def dynamic(self):
        entry = 16 if self.wide else 8
        for kind, offset, _vaddr, size in self.segments():
            if kind == PT_DYNAMIC:
                for position in range(offset, offset + size, entry):
                    tag, value = struct.unpack_from("<qQ" if self.wide else "<iI", self.data, position)
                    if tag == DT_NULL:
                        return
                    yield tag, value
                return

    def string_references(self):
        """Offsets into the dynamic string table that something other than a dynamic entry names."""
        found = set()
        for kind, offset, size, entsize in self.sections():
            if kind == SHT_DYNSYM and entsize:
                for position in range(offset, offset + size, entsize):
                    found.add(struct.unpack_from("<I", self.data, position)[0])
            elif kind == SHT_GNU_VERNEED:
                position = offset
                while True:
                    _version, count, name, aux, following = struct.unpack_from("<HHIII", self.data, position)
                    found.add(name)
                    entry = position + aux
                    for _ in range(count):
                        _hash, _flags, _other, aux_name, aux_next = struct.unpack_from("<IHHII", self.data, entry)
                        found.add(aux_name)
                        entry += aux_next
                    if not following:
                        break
                    position += following
            elif kind == SHT_GNU_VERDEF:
                position = offset
                while True:
                    _version, _flags, _index, count, _hash, aux, following = struct.unpack_from(
                        "<HHHHIII", self.data, position
                    )
                    entry = position + aux
                    for _ in range(count):
                        aux_name, aux_next = struct.unpack_from("<II", self.data, entry)
                        found.add(aux_name)
                        entry += aux_next
                    if not following:
                        break
                    position += following
        return found


def fix(path: str):
    """The search path change made to ``path`` as (old, new), None if there was nothing to change; raises on a file to skip."""
    with open(path, "rb") as handle:
        data = bytearray(handle.read())
    elf = Elf(bytes(data))
    entries = dict()
    strtab = None
    string_tags = []
    for tag, value in elf.dynamic():
        if tag == DT_STRTAB:
            strtab = elf.file_offset(value)
        if tag in (DT_NEEDED, DT_SONAME, DT_RPATH, DT_RUNPATH):
            string_tags.append((tag, value))
    if strtab is None:
        return None

    changes = []
    for tag, value in string_tags:
        if tag not in (DT_RPATH, DT_RUNPATH):
            continue
        start = strtab + value
        end = data.index(b"\0", start)
        old = data[start:end].decode("utf-8", errors="surrogateescape")
        kept = rewritten(old.split(":"))
        if kept is None:
            continue
        new = ":".join(kept).encode("utf-8", errors="surrogateescape")
        if len(new) > end - start:
            raise ValueError(f"the new search path {new!r} would not fit in place of {old!r}")
        entries[(value, end - start)] = (old, new)
        changes.append((start, end, new))

    if not changes:
        return None
    if not elf.shnum:
        raise ValueError("no section headers, so a string shared with the search path could not be ruled out")
    others = elf.string_references() | {value for tag, value in string_tags if tag not in (DT_RPATH, DT_RUNPATH)}
    for value, length in entries:
        if any(value < other <= value + length for other in others):
            raise ValueError("another string shares the search path's bytes")

    for start, end, new in changes:
        data[start:end] = new + b"\0" * (end - start - len(new))
    with open(path, "r+b") as handle:
        handle.write(data)
    return [(old, new.decode("utf-8", errors="surrogateescape")) for old, new in entries.values()]


def main(argv=None) -> int:
    argv = sys.argv[1:] if argv is None else argv
    if len(argv) != 1:
        print(__doc__)
        return 2
    changed = skipped = 0
    for directory, _subdirectories, names in os.walk(argv[0]):
        for name in names:
            path = os.path.join(directory, name)
            if os.path.islink(path) or not os.path.isfile(path):
                continue
            with open(path, "rb") as handle:
                if handle.read(4) != b"\x7fELF":
                    continue
            try:
                result = fix(path)
            except (ValueError, struct.error, IndexError) as error:
                print(f"-- RUNPATH left unchanged in {path}: {error}")
                skipped += 1
                continue
            for old, new in result or []:
                print(f"-- RUNPATH of {path}: {old} -> {new}")
                changed += 1
    print(f"-- {changed} absolute search path(s) removed, {skipped} file(s) left unchanged")
    return 0


if __name__ == "__main__":
    sys.exit(main())
