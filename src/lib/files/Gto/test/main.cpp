//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Gto/Writer.h>
#include <Gto/Reader.h>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>

using namespace std;

float fdata[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
int idata[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

void write(const char* filename)
{
    cout << "writing " << filename << endl;
    Gto::Writer writer;
    writer.open(filename);

    writer.beginObject("test", "data", 0);
    writer.beginComponent("component_1");
    writer.property("property_1", Gto::Float, 10);
    writer.property("property_2", Gto::Float, 10);
    writer.property("property_3", Gto::Int, 10);
    writer.endComponent();
    writer.endObject();

    writer.beginObject("test2", "data", 0);
    writer.beginComponent("component_1");
    writer.property("property_1", Gto::Float, 10);
    writer.property("property_2", Gto::Float, 10);
    writer.property("property_3", Gto::Int, 10);
    writer.property("property_4", Gto::Int, 10);
    writer.endComponent();
    writer.endObject();

    writer.beginData();
    writer.propertyData(fdata);
    writer.propertyData(fdata);
    writer.propertyData(idata);

    writer.propertyData(fdata);
    writer.propertyData(fdata);
    writer.propertyData(idata);
    writer.propertyData(idata);
    writer.endData();
}

void read(const char* filename)
{
    cout << "reading " << filename << endl;
    Gto::Reader reader;
    reader.open(filename);
}

int main(int, char**)
{
    struct stat s;
    write("test.gto");
    read("test.gto");
    unlink("test.gto");

    if (stat("big_endian.gto", &s) != -1)
        read("big_endian.gto");
    if (stat("little_endian.gto", &s) != -1)
        read("little_endian.gto");
}
