//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <IOgto/IOgto.h>
#include <iostream>
#include <stdlib.h>
#include <vector>
#include <string>

using namespace TwkFB;
using namespace std;
using namespace boost::program_options;
using namespace boost::algorithm;
using namespace boost;

extern "C"
{

#ifdef PLATFORM_WINDOWS
    __declspec(dllexport) TwkFB::FrameBufferIO* create();
    __declspec(dllexport) void destroy(TwkFB::IOgto*);
#endif

    FrameBufferIO* create()
    {
        int ioMethod = 0;
        int ioSize = 61440;
        int ioMaxAsync = 16;

        if (const char* args = getenv("IOGTO_ARGS"))
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
                    "ioMethod", value<int>(&ioMethod)->default_value(ioMethod),
                    "I/O Method")("ioSize",
                                  value<int>(&ioSize)->default_value(ioSize),
                                  "I/O Max async read size")(
                    "ioMaxAsync",
                    value<int>(&ioMaxAsync)->default_value(ioMaxAsync),
                    "I/O Max ASync Requests");

                variables_map vm;
                store(parse_command_line(argc, argv, desc), vm);
                notify(vm);
            }
            catch (std::exception& e)
            {
                cout << "ERROR: IOGTO_ARGS: " << e.what() << endl;
                cout << "ERROR: IOGTO_ARGS = \"" << args << "\"" << endl;
            }
            catch (...)
            {
                cout << "ERROR: bad IOGTO_ARGS = \"" << args << "\"" << endl;
            }
        }

        return new IOgto((StreamingFrameBufferIO::IOType)ioMethod, ioSize,
                         ioMaxAsync);
    }

    void destroy(IOgto* plug) { delete plug; }

} // extern  "C"
