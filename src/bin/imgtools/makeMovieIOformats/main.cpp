//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include "../../utf8Main.h"

#ifdef _MSC_VER
//
//  We are targetting at least XP, (IE no windows95, etc).
//
#define WINVER 0x0501
#define _WIN32_WINNT 0x0501
#include <windows.h>
#include <winnt.h>
#include <wincon.h>
#include <pthread.h>
//
//  NOTE: win_pthreads, which supplies implement.h, seems
//  targetted at an earlier version of windows (pre-XP).  If you
//  include implement.h here, it won't compile.  But as far as I
//  can tell, it's not needed, so just leave it out.
//
//  #include <implement.h>
#endif

#include <TwkMovie/MovieIO.h>
#include <Gto/RawData.h>
#include <TwkUtil/File.h>
#include <TwkUtil/PathConform.h>
#include <ImfHeader.h>
#include <ImfThreading.h>
#include <iostream>

using namespace TwkMovie;
using namespace TwkUtil;
using namespace Gto;
using namespace std;

#ifdef PLATFORM_DARWIN
#define EXT_PATTERN "dylib"
#endif

#ifdef PLATFORM_WINDOWS
#define EXT_PATTERN "dll"
#endif

#ifdef PLATFORM_LINUX
#define EXT_PATTERN "so"
#endif

int utf8Main(int argc, char** argv)
{
    if (argc > 2)
    {
        cout << "Usage: " << argv[0] << " [directory]" << endl;
        return -1;
    }

    Imf::staticInitialize();
    Imf::setGlobalThreadCount(0);

    FileNameList files;
    string dir = argc == 2 ? argv[1] : ".";

    cout << "INFO: looking in " << dir << endl;

    if (filesInDirectory(pathConform(dir).c_str(), "mio_*." EXT_PATTERN, files))
    {
        for (int i = 0; i < files.size(); i++)
        {
            string file = dir;
            file += "/";
            file += files[i];
            file = pathConform(file);

            if (MovieIO* io = GenericIO::loadPlugin(file.c_str()))
            {
                cout << "INFO: loaded " << file << endl;

                GenericIO::addPlugin(io);
            }
            else
            {
                cerr << "WARNING: ignoring " << file << endl;
            }
        }
    }

    Writer writer;
    string outfile = dir;
    if (outfile[outfile.size() - 1] != '/')
        outfile += "/";
    outfile += "movieformats.gto";
    outfile = pathConform(outfile);

    if (!writer.open(outfile.c_str(), Writer::CompressedGTO))
    {
        cerr << "ERROR: makeMovieIOformats: can't open output file" << endl;
        return -1;
    }

    const GenericIO::Plugins& plugs = GenericIO::allPlugins();

    for (GenericIO::Plugins::const_iterator i = plugs.begin(); i != plugs.end();
         ++i)
    {
        MovieIO* plugin = *i;

        string fname = prefix(plugin->pluginFile());
        writer.beginObject(fname.c_str(), "movieio", 2);

        const MovieIO::MovieTypeInfos& infos = plugin->extensionsSupported();

        writer.beginComponent(":info");
        writer.property("identifier", Gto::String, 1);
        writer.property("sortkey", Gto::String, 1);
        writer.endComponent();

        writer.intern(plugin->identifier());
        writer.intern(plugin->sortKey());

        for (int q = 0; q < infos.size(); q++)
        {
            const MovieIO::MovieTypeInfo& info = infos[q];

            writer.beginComponent(info.extension.c_str());
            writer.property("description", Gto::String, 1);
            writer.property("capabilities", Gto::Int, 1);
            writer.property("video_codecs", Gto::String, info.codecs.size(), 2);
            writer.property("audio_codecs", Gto::String,
                            info.audioCodecs.size(), 2);
            writer.property("encode_parameters", Gto::String,
                            info.encodeParameters.size(), 3);
            writer.property("decode_parameters", Gto::String,
                            info.decodeParameters.size(), 3);
            writer.endComponent();

            writer.intern(info.description);

            for (int j = 0; j < info.codecs.size(); j++)
            {
                writer.intern(info.codecs[j].first);
                writer.intern(info.codecs[j].second);
            }

            for (int j = 0; j < info.audioCodecs.size(); j++)
            {
                writer.intern(info.audioCodecs[j].first);
                writer.intern(info.audioCodecs[j].second);
            }

            for (int j = 0; j < info.encodeParameters.size(); j++)
            {
                writer.intern(info.encodeParameters[j].name);
                writer.intern(info.encodeParameters[j].description);
                writer.intern(info.encodeParameters[j].codec);
            }

            for (int j = 0; j < info.decodeParameters.size(); j++)
            {
                writer.intern(info.decodeParameters[j].name);
                writer.intern(info.decodeParameters[j].description);
                writer.intern(info.decodeParameters[j].codec);
            }
        }

        writer.endObject();
    }

    writer.beginData();

    for (GenericIO::Plugins::const_iterator i = plugs.begin(); i != plugs.end();
         ++i)
    {
        MovieIO* plugin = *i;

        const MovieIO::MovieTypeInfos& infos = plugin->extensionsSupported();

        int id = writer.lookup(plugin->identifier());
        int k = writer.lookup(plugin->sortKey());
        writer.propertyData(&id);
        writer.propertyData(&k);

        for (int q = 0; q < infos.size(); q++)
        {
            const MovieIO::MovieTypeInfo& info = infos[q];

            int s = writer.lookup(info.description);
            vector<int> vcodecs;
            vector<int> acodecs;
            vector<int> eparams;
            vector<int> dparams;
            writer.propertyData(&s);
            writer.propertyData(&info.capabilities);

            for (int j = 0; j < info.codecs.size(); j++)
            {
                vcodecs.push_back(writer.lookup(info.codecs[j].first));
                vcodecs.push_back(writer.lookup(info.codecs[j].second));
            }

            for (int j = 0; j < info.audioCodecs.size(); j++)
            {
                acodecs.push_back(writer.lookup(info.audioCodecs[j].first));
                acodecs.push_back(writer.lookup(info.audioCodecs[j].second));
            }

            for (int j = 0; j < info.audioCodecs.size(); j++)
            {
                acodecs.push_back(writer.lookup(info.audioCodecs[j].first));
                acodecs.push_back(writer.lookup(info.audioCodecs[j].second));
            }

            for (int j = 0; j < info.encodeParameters.size(); j++)
            {
                eparams.push_back(writer.lookup(info.encodeParameters[j].name));
                eparams.push_back(
                    writer.lookup(info.encodeParameters[j].description));
                eparams.push_back(
                    writer.lookup(info.encodeParameters[j].codec));
            }

            for (int j = 0; j < info.decodeParameters.size(); j++)
            {
                dparams.push_back(writer.lookup(info.decodeParameters[j].name));
                dparams.push_back(
                    writer.lookup(info.decodeParameters[j].description));
                dparams.push_back(
                    writer.lookup(info.decodeParameters[j].codec));
            }

            if (vcodecs.empty())
                writer.emptyProperty();
            else
                writer.propertyData(&vcodecs.front());

            if (acodecs.empty())
                writer.emptyProperty();
            else
                writer.propertyData(&acodecs.front());

            if (eparams.empty())
                writer.emptyProperty();
            else
                writer.propertyData(&eparams.front());

            if (dparams.empty())
                writer.emptyProperty();
            else
                writer.propertyData(&dparams.front());
        }
    }

    writer.endData();

    return 0;
}
