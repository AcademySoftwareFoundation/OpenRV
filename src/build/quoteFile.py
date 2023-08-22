#!/usr/bin/env python3
#
# Copyright (c) 2001, Tweak Films
# Copyright (c) 2008, Tweak Software
#
# SPDX-License-Identifier: Apache-2.0
#
# Quote a text file in C++ form for inclusion into a C++ source file
#
import sys
import string

index = 1
varlist = []

while sys.argv[index] == "-s":
    assign = sys.argv[index + 1]
    parts = str.split(assign, "=")
    varlist.append(parts)
    index += 2

infilename = sys.argv[index]
outfilename = sys.argv[index + 1]
cppname = sys.argv[index + 2]

infile = open(infilename, "r")
outfile = open(outfilename, "w")
lines = infile.readlines()

outfile.write("//\n")
outfile.write("// **** DO NOT EDIT ****\n")
outfile.write("// This file -- " + outfilename + " -- was automatically generated\n")
outfile.write("// from the file " + infilename + " by quoteFile.\n")
outfile.write("//\n")

outfile.write("const char* " + cppname + " = ")

for line in lines:
    l = line.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n")
    for subst in varlist:
        l = l.replace(subst[0], subst[1])
    outfile.write('"' + l + '"\n')

outfile.write(";\n")
outfile.close()

# Local Variables:
# mode: python
# end:
