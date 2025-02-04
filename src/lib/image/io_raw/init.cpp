//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <IOraw/IOraw.h>
#include <iostream>
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
    __declspec(dllexport) void destroy(TwkFB::IOraw*);
#endif

    TwkFB::FrameBufferIO* create()
    {
        int threads = -1;
        double scale = -1.0;
        string primaries = "sRGB";
        int bruteForce = 0;
        if (const char* args = getenv("IORAW_ARGS"))
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
                desc.add_options()("threads",
                                   value<int>(&threads)->default_value(threads),
                                   "Specify the number of threads to use")(
                    "scale", value<double>(&scale)->default_value(scale),
                    "Scaling factor used to normalize raw data")(
                    "primaries",
                    value<string>(&primaries)->default_value(primaries),
                    "Set the primaries to use for the RAW source")(
                    "bruteForce",
                    value<int>(&bruteForce)->default_value(bruteForce),
                    "Attempt to load any file extension");

                variables_map vm;
                store(parse_command_line(argc, argv, desc), vm);
                notify(vm);
            }
            catch (std::exception& e)
            {
                cout << "ERROR: IORAW_ARGS: " << e.what() << endl;
                cout << "ERROR: IORAW_ARGS = \"" << args << "\"" << endl;
            }
            catch (...)
            {
                cout << "ERROR: bad IORAW_ARGS = \"" << args << "\"" << endl;
            }
        }

        return new TwkFB::IOraw(threads, scale, primaries, bruteForce);
    }

    void destroy(TwkFB::IOraw* plug) { delete plug; }

} // extern  "C"
