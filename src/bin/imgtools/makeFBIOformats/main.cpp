//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include "../../utf8Main.h"

#include <TwkFB/IO.h>
#include <Gto/RawData.h>
#include <TwkUtil/File.h>
#include <TwkUtil/PathConform.h>
#include <ImfHeader.h>
#include <ImfThreading.h>
#include <iostream>

using namespace TwkFB;
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
    TwkFB::GenericIO::init(); // Initialize TwkFB::GenericIO plugins statics

    FileNameList files;
    string dir = argc == 2 ? argv[1] : ".";

    cout << "INFO: looking in " << dir << endl;

    if (filesInDirectory(pathConform(dir).c_str(), "io_*." EXT_PATTERN, files))
    {
        for (int i = 0; i < files.size(); i++)
        {
            string file = dir;
            file += "/";
            file += files[i];
            file = pathConform(file);

            if (FrameBufferIO* io = GenericIO::loadPlugin(file.c_str()))
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
    outfile += "formats.gto";
    outfile = pathConform(outfile);

    if (!writer.open(outfile.c_str(), Writer::CompressedGTO))
    {
        cerr << "ERROR: makeFBIOformats: can't open output file" << endl;
        return -1;
    }

    const GenericIO::Plugins& plugs = GenericIO::allPlugins();

    for (GenericIO::Plugins::const_iterator i = plugs.begin(); i != plugs.end();
         ++i)
    {

        FrameBufferIO* plugin = *i;

        string fname = prefix(plugin->pluginFile());
        writer.beginObject(fname.c_str(), "imageio", 2);

        const FrameBufferIO::ImageTypeInfos& infos =
            plugin->extensionsSupported();

        writer.beginComponent(":info");
        writer.property("identifier", Gto::String, 1);
        writer.property("sortkey", Gto::String, 1);
        writer.endComponent();

        for (int q = 0; q < infos.size(); q++)
        {
            const FrameBufferIO::ImageTypeInfo& info = infos[q];

            writer.beginComponent(info.extension.c_str());
            writer.property("description", Gto::String, 1);
            writer.property("capabilities", Gto::Int, 1);
            writer.property("codecs", Gto::String,
                            info.compressionSchemes.size(), 2);
            writer.property("encodeParams", Gto::String,
                            info.encodeParameters.size(), 2);
            writer.property("decodeParams", Gto::String,
                            info.decodeParameters.size(), 2);
            writer.endComponent();

            writer.intern(info.description);
            writer.intern(plugin->identifier());
            writer.intern(plugin->sortKey());

            for (int j = 0; j < info.compressionSchemes.size(); j++)
            {
                writer.intern(info.compressionSchemes[j].first);
                writer.intern(info.compressionSchemes[j].second);
            }

            for (int j = 0; j < info.encodeParameters.size(); j++)
            {
                writer.intern(info.encodeParameters[j].first);
                writer.intern(info.encodeParameters[j].second);
            }

            for (int j = 0; j < info.decodeParameters.size(); j++)
            {
                writer.intern(info.decodeParameters[j].first);
                writer.intern(info.decodeParameters[j].second);
            }
        }

        writer.endObject();
    }

    writer.beginData();

    for (GenericIO::Plugins::const_iterator i = plugs.begin(); i != plugs.end();
         ++i)
    {
        FrameBufferIO* plugin = *i;

        const FrameBufferIO::ImageTypeInfos& infos =
            plugin->extensionsSupported();

        int id = writer.lookup(plugin->identifier());
        int k = writer.lookup(plugin->sortKey());
        writer.propertyData(&id);
        writer.propertyData(&k);

        for (int q = 0; q < infos.size(); q++)
        {
            const FrameBufferIO::ImageTypeInfo& info = infos[q];

            int s = writer.lookup(info.description);
            vector<int> schemes;
            vector<int> eparams;
            vector<int> dparams;
            writer.propertyData(&s);
            writer.propertyData(&info.capabilities);

            for (int j = 0; j < info.compressionSchemes.size(); j++)
            {
                schemes.push_back(
                    writer.lookup(info.compressionSchemes[j].first));
                schemes.push_back(
                    writer.lookup(info.compressionSchemes[j].second));
            }

            for (int j = 0; j < info.encodeParameters.size(); j++)
            {
                eparams.push_back(
                    writer.lookup(info.encodeParameters[j].first));
                eparams.push_back(
                    writer.lookup(info.encodeParameters[j].second));
            }

            for (int j = 0; j < info.decodeParameters.size(); j++)
            {
                dparams.push_back(
                    writer.lookup(info.decodeParameters[j].first));
                dparams.push_back(
                    writer.lookup(info.decodeParameters[j].second));
            }

            if (schemes.empty())
            {
                writer.emptyProperty();
            }
            else
            {
                writer.propertyData(&schemes.front());
            }

            if (eparams.empty())
            {
                writer.emptyProperty();
            }
            else
            {
                writer.propertyData(&eparams.front());
            }

            if (dparams.empty())
            {
                writer.emptyProperty();
            }
            else
            {
                writer.propertyData(&dparams.front());
            }
        }
    }

    writer.endData();

    TwkFB::GenericIO::shutdown(); // Shutdown TwkFB::GenericIO plugins

    return 0;
}
