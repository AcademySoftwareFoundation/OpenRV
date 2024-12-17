//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <MovieProxy/MovieProxy.h>
#include <Gto/RawData.h>
#include <TwkMovie/MovieIO.h>
#include <TwkUtil/File.h>
#include <string>
#include <vector>
#include <stdlib.h>

#ifdef PLATFORM_DARWIN
#define SO_EXTENSION ".dylib"
#endif

#ifdef PLATFORM_WINDOWS
#define SO_EXTENSION ".dll"
#endif

#ifdef PLATFORM_LINUX
#define SO_EXTENSION ".so"
#endif

namespace TwkMovie
{

    using namespace std;
    using namespace TwkUtil;
    using namespace Gto;
    using namespace std;

    static vector<string> getPluginFileList(const string& pathVar)
    {
        string pluginPath;

        if (getenv(pathVar.c_str()))
        {
#ifdef PLATFORM_DARWIN
            string pluginDir = getenv(pathVar.c_str());
            string resourceDir = pluginDir + "/../../Resources";
            pluginPath = resourceDir + ":" + pluginDir;
#else
            pluginPath = getenv(pathVar.c_str());
#endif
        }
        else
        {
            pluginPath = "/usr/local/lib:/usr/lib:/lib";
        }

        return findInPath("^movieformats.gto$", pluginPath);
    }

    static bool loadedProxies = false;

    void loadProxyPlugins(const std::string& envVar)
    {
        if (!loadedProxies)
        {
            vector<string> pluginFiles = getPluginFileList(envVar);
            if (pluginFiles.empty())
                return;

            for (int i = 0; i < pluginFiles.size(); i++)
            {
                RawDataBaseReader reader;

                if (reader.open(pluginFiles[i].c_str()))
                {
                    RawDataBase* db = reader.dataBase();

                    for (size_t q = 0; q < db->objects.size(); q++)
                    {
                        const Object* o = db->objects[q];
                        string fname = dirname(pluginFiles[i]);
#ifdef PLATFORM_DARWIN
                        // If the gto file is in the Resources dir
                        // then change the fname of the plugin
                        // to look in the PlugIns subdir dir.
                        // This is because for code signed pkg like
                        // Crank, the gto has to live outside
                        // of PlugIns dir in the Resources dir.
                        if (basename(fname) == "Resources")
                        {
                            fname += "/../PlugIns/MovieFormats";
                        }
#endif
                        if (fname[fname.size() - 1] != '/')
                            fname += "/";
                        fname += o->name;
                        fname += SO_EXTENSION;

                        string identifier;
                        string sortKey;

                        for (size_t j = 0; j < o->components.size(); j++)
                        {
                            const Component* c = o->components[j];
                            string name = c->name;

                            if (name == ":info")
                            {
                                for (size_t x = 0; x < c->properties.size();
                                     x++)
                                {
                                    const Property* p = c->properties[x];

                                    if (p->name == "identifier")
                                        identifier = (p->stringData)[0];
                                    else if (p->name == "sortkey")
                                        sortKey = (p->stringData)[0];
                                }
                            }
                        }

                        ProxyMovieIO* io =
                            new ProxyMovieIO(identifier, sortKey, fname);

                        for (size_t j = 0; j < o->components.size(); j++)
                        {
                            const Component* c = o->components[j];

                            string name = c->name;

                            if (name == ":info")
                                continue;

                            string desc;
                            unsigned int caps;
                            MovieIO::StringPairVector audio_codecs;
                            MovieIO::StringPairVector video_codecs;
                            MovieIO::ParameterVector eparams;
                            MovieIO::ParameterVector dparams;

                            for (size_t x = 0; x < c->properties.size(); x++)
                            {
                                const Property* p = c->properties[x];

                                if (p->name == "description")
                                {
                                    desc = (p->stringData)[0];
                                }
                                else if (p->name == "capabilities")
                                {
                                    caps = (p->int32Data)[0];
                                }
                                else if (p->name == "audio_codecs")
                                {
                                    for (int i = 0; i < p->size * p->dims.x;
                                         i += 2)
                                    {
                                        audio_codecs.push_back(
                                            make_pair((p->stringData)[i],
                                                      (p->stringData)[i + 1]));
                                    }
                                }
                                else if (p->name == "video_codecs")
                                {
                                    for (int i = 0; i < p->size * p->dims.x;
                                         i += 2)
                                    {
                                        video_codecs.push_back(
                                            make_pair((p->stringData)[i],
                                                      (p->stringData)[i + 1]));
                                    }
                                }
                                else if (p->name == "encode_parameters")
                                {
                                    for (int i = 0; i < p->size * p->dims.x;
                                         i += 3)
                                    {
                                        eparams.push_back(MovieIO::Parameter(
                                            (p->stringData)[i],
                                            (p->stringData)[i + 1],
                                            (p->stringData)[i + 2]));
                                    }
                                }
                                else if (p->name == "decode_parameters")
                                {
                                    for (int i = 0; i < p->size * p->dims.x;
                                         i += 3)
                                    {
                                        dparams.push_back(MovieIO::Parameter(
                                            (p->stringData)[i],
                                            (p->stringData)[i + 1],
                                            (p->stringData)[i + 2]));
                                    }
                                }
                            }

                            io->add(name, desc, caps, video_codecs,
                                    audio_codecs, eparams, dparams);
                        }

                        GenericIO::addPlugin(io);
                    }
                }
                else
                {
                    continue;
                }
            }

            loadedProxies = true;
        }
    }

} // namespace TwkMovie
