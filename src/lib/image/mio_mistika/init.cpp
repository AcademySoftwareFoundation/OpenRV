//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <MovieMistika/MovieMistika.h>
#include <iostream>
#include <vector>
#include <string>
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
    __declspec(dllexport) TwkMovie::MovieIO* create();
    __declspec(dllexport) void destroy(TwkMovie::MovieMistikaIO*);
#endif

    TwkMovie::MovieIO* create()
    {
        string format;

        if (const char* args = getenv("MOVIEMISTIKA_ARGS"))
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
                desc.add_options()("format",
                                   value<string>()->default_value(format), "");

                variables_map vm;
                store(parse_command_line(argc, argv, desc), vm);
                notify(vm);

                format = vm["format"].as<string>();
            }
            catch (std::exception& e)
            {
                cout << "ERROR: MOVIEMISTIKA_ARGS: " << e.what() << endl;
                cout << "ERROR: MOVIEMISTIKA_ARGS = \"" << args << "\"" << endl;
            }
            catch (...)
            {
                cout << "ERROR: bad MOVIEMISTIKA_ARGS = \"" << args << "\""
                     << endl;
            }
        }

        TwkMovie::MovieMistika::Format pixelFormat;
        if (format == "RGB8")
            pixelFormat = TwkMovie::MovieMistika::RGB8;
        else if (format == "RGBA8")
            pixelFormat = TwkMovie::MovieMistika::RGBA8;
        else if (format == "RGB8_PLANAR")
            pixelFormat = TwkMovie::MovieMistika::RGB8_PLANAR;
        else if (format == "RGB10_A2")
            pixelFormat = TwkMovie::MovieMistika::RGB10_A2;
        else if (format == "RGB16")
            pixelFormat = TwkMovie::MovieMistika::RGB16;
        else if (format == "RGBA16")
            pixelFormat = TwkMovie::MovieMistika::RGBA16;
        else if (format == "RGB16_PLANAR")
            pixelFormat = TwkMovie::MovieMistika::RGB16_PLANAR;
        else
            pixelFormat = TwkMovie::MovieMistika::RGB8_PLANAR;
        TwkMovie::MovieMistika::pixelFormat = pixelFormat;

        return new TwkMovie::MovieMistikaIO();
    }

    void destroy(TwkMovie::MovieMistikaIO* plug) { delete plug; }

} // extern  "C"
