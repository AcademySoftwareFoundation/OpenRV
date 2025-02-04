//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <IOexr/IOexr.h>
#include <iostream>
#include <vector>
#include <string>
#include <ImfHeader.h>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>

using namespace TwkFB;
using namespace std;
using namespace boost::program_options;
using namespace boost::algorithm;
using namespace boost;

extern "C"
{

#ifdef PLATFORM_WINDOWS
    __declspec(dllexport) TwkFB::FrameBufferIO* create();
    __declspec(dllexport) void destroy(IOexr*);
#endif

    FrameBufferIO* create()
    {
        int rgbaOnly = 0;
        int convertYRYBY = 0;
        int inherit = 1;
        int planar3channel = 1;
        int noOneChannel = 0;
        int stripAlpha = 0;
        int writeMethod = (int)IOexr::MultiViewWriter;
        int ioMethod = 1;
        int ioSize = 61440;
        int ioMaxAsync = 16;
        int readWindowIsDisplayWindow = 0;
        int readWindow = (int)IOexr::DataInsideDisplayWindow;

        Imf::staticInitialize();

        if (const char* args = getenv("IOEXR_ARGS"))
        {
            try
            {
                vector<string> buffer;
                split(buffer, args, is_any_of(" "), token_compress_on);

                vector<const char*> newargs(buffer.size() + 1);
                newargs[0] = "";
                for (size_t i = 0; i < buffer.size(); i++)
                    newargs[i + 1] = buffer[i].c_str();
                const char** argv = &newargs.front();
                int argc = newargs.size();

                options_description desc("");
                desc.add_options()(
                    "rgbaOnly", value<int>(&rgbaOnly)->default_value(rgbaOnly),
                    "Always use EXR default RGBA reader")(
                    "convertYRYBY",
                    value<int>(&convertYRYBY)->default_value(convertYRYBY),
                    "Convert Y-RY-BY images to RGBA")(
                    "planar3channel",
                    value<int>(&planar3channel)->default_value(planar3channel),
                    "Data Window to read")(
                    "inherit", value<int>(&inherit)->default_value(inherit),
                    "Guess RGBA channel names")(
                    "noOneChannel",
                    value<int>(&noOneChannel)->default_value(noOneChannel),
                    "Data Window to read")(
                    "readWindow",
                    value<int>(&readWindow)->default_value(readWindow),
                    "Data Window to read")(
                    "readWindowIsDisplayWindow",
                    value<int>(&readWindowIsDisplayWindow)
                        ->default_value(readWindowIsDisplayWindow),
                    "Read Window is Display Window")(
                    "stripAlpha",
                    value<int>(&stripAlpha)->default_value(stripAlpha),
                    "Strip alpha channel from EXRs for speed")(
                    "ioMethod", value<int>(&ioMethod)->default_value(ioMethod),
                    "I/O Method")("ioSize",
                                  value<int>(&ioSize)->default_value(ioSize),
                                  "I/O Max async read size")(
                    "ioMaxAsync",
                    value<int>(&ioMaxAsync)->default_value(ioMaxAsync),
                    "I/O Max ASync Requests")(
                    "writeMethod",
                    value<int>(&writeMethod)->default_value(writeMethod),
                    "Write Method");

                variables_map vm;
                store(parse_command_line(argc, argv, desc), vm);
                notify(vm);
            }
            catch (std::exception& e)
            {
                cout << "ERROR: IOEXR_ARGS: " << e.what() << endl;
                cout << "ERROR: IOEXR_ARGS = \"" << args << "\"" << endl;
            }
            catch (...)
            {
                cout << "ERROR: bad IOEXR_ARGS = \"" << args << "\"" << endl;
            }
        }

        return new IOexr(
            rgbaOnly, convertYRYBY, planar3channel, inherit, noOneChannel,
            stripAlpha, readWindowIsDisplayWindow,
            (IOexr::ReadWindow)readWindow, (IOexr::WriteMethod)writeMethod,
            (StreamingFrameBufferIO::IOType)ioMethod, ioSize, ioMaxAsync);
    }

    void destroy(IOexr* plug) { delete plug; }

} // extern  "C"
