//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <MovieFFMpeg/MovieFFMpeg.h>

#include <TwkFB/IO.h>
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

// NOTE ON PATENT LICENSING:
//
//   Legal Note: Some of the codecs listed below contain proprietary
//   algorithms that are protected under intellectual property
//   rights. Please check with your legal department whether you have
//   the proper licenses and rights to use these codecs. TWEAK is not
//   responsible for any unlicensed use of these codecs.
//
//   Upshot: You should not remove entries from this list if you don't
//   actually have that license.
//

static const char* disallowedCodecsArray[] = {
#if !defined(__FFMPEG_ENABLE_NON_FREE_DECODER_ac3)
    "ac3",
#endif
#if !defined(__FFMPEG_ENABLE_NON_FREE_DECODER_hevc)
    "hevc",
#endif
#if !defined(__FFMPEG_ENABLE_NON_FREE_DECODER_mpeg2video)
    "mpeg2video",
#endif
#if !defined(__FFMPEG_ENABLE_NON_FREE_DECODER_prores)
    "prores",
#endif
#if !defined(__FFMPEG_ENABLE_NON_FREE_DECODER_prores_aw)
    "prores_aw",
#endif
#if !defined(__FFMPEG_ENABLE_NON_FREE_DECODER_prores_ks)
    "prores_ks",
#endif
#if !defined(__FFMPEG_ENABLE_NON_FREE_DECODER_prores_lgpl)
    "prores_lgpl",
#endif
#if !defined(__FFMPEG_ENABLE_NON_FREE_DECODER_svq1)
    "svq1",
#endif
#if !defined(__FFMPEG_ENABLE_NON_FREE_DECODER_svq3)
    "svq3",
#endif
    0};

extern "C"
{

#ifdef PLATFORM_WINDOWS
    __declspec(dllexport) TwkMovie::MovieIO* create();
    __declspec(dllexport) void destroy(TwkMovie::MovieFFMpegIO*);
    __declspec(dllexport) bool codecIsAllowed(std::string, bool);
#endif

    static bool codecIsAllowed(std::string name, bool forRead = true)
    {
        for (const char** p = disallowedCodecsArray; *p; p++)
        {
            if (*p == name)
            {
                return false;
            }
        }
        return true;
    };

    TwkMovie::MovieIO* create()
    {
        int bruteForce = 0;
        int codecThreads = 0;
        double defaultFPS = 24.0;
        string language = "eng";
        if (const char* args = getenv("MOVIEFFMPEG_ARGS"))
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
                    "bruteForce",
                    value<int>(&bruteForce)->default_value(bruteForce),
                    "Attempt to load any file extension")(
                    "codecThreads",
                    value<int>(&codecThreads)->default_value(codecThreads),
                    "Thread count to pass to ffmpeg on codec open")(
                    "language",
                    value<string>(&language)->default_value(language),
                    "Language for audio and subtitles (default: eng)")(
                    "defaultFPS",
                    value<double>(&defaultFPS)->default_value(defaultFPS),
                    "For cases where there is no way to determine FPS");

                variables_map vm;
                store(parse_command_line(argc, argv, desc), vm);
                notify(vm);
            }
            catch (std::exception& e)
            {
                cout << "ERROR: MOVIEFFMPEG_ARGS: " << e.what() << endl;
                cout << "ERROR: MOVIEFFMPEG_ARGS = \"" << args << "\"" << endl;
            }
            catch (...)
            {
                cout << "ERROR: bad MOVIEFFMPEG_ARGS = \"" << args << "\""
                     << endl;
            }
        }

        return new TwkMovie::MovieFFMpegIO(codecIsAllowed, bruteForce,
                                           codecThreads, language, defaultFPS);
    }

    void destroy(TwkMovie::MovieFFMpegIO* plug) { delete plug; }

} // extern  "C"
