//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include "../../utf8Main.h"

#ifdef PLATFORM_DARWIN
#include <DarwinBundle/DarwinBundle.h>
#include <QtCore/QtCore>
#else
#include <QTBundle/QTBundle.h>
#endif

#include <stdlib.h>
#include <IOproxy/IOproxy.h>
#include <ImfThreading.h>
#include <MovieFB/MovieFB.h>
#include <MuTwkApp/MuInterface.h>
#include <PyTwkApp/PyInterface.h>
#include <TwkFB/TwkFBThreadPool.h>
#include <MovieProcedural/MovieProcedural.h>
#include <MovieProxy/MovieProxy.h>
#include <TwkDeploy/Deploy.h>
#include <TwkMovie/MovieIO.h>
#include <TwkContainer/GTOReader.h>
#include <TwkUtil/File.h>
#include <TwkUtil/FrameUtils.h>
#include <algorithm>
#include <arg.h>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <iterator>
#include <numeric>
#include <stl_ext/thread_group.h>
#include <boost/algorithm/string.hpp>
#include <ImfHeader.h>
#include <yaml-cpp/yaml.h>

extern void TwkMovie_GenericIO_setDebug(bool);
extern void TwkFB_GenericIO_setDebug(bool);

using namespace std;
using namespace TwkUtil;
using namespace TwkFB;
using namespace TwkMovie;
using namespace TwkContainer;
using namespace boost;

bool bruteForce = false;

vector<string> inputFiles;

int parseInFiles(int argc, char* argv[])
{
    for (int i = 0; i < argc; i++)
    {
        inputFiles.push_back(argv[i]);
    }

    return 0;
}

void debugSwitches(const char* name)
{
    if (name)
    {
        if (!strcmp(name, "plugins"))
        {
            TwkMovie_GenericIO_setDebug(true);
            TwkFB_GenericIO_setDebug(true);
        }
    }
}

void lsLong(const string& seq, vector<string>& parts)
{
    if (MovieReader* reader = TwkMovie::GenericIO::movieReader(seq, bruteForce))
    {
        try
        {
            reader->open(seq);
        }
        catch (std::exception& exc)
        {
            cerr << "ERROR: " << exc.what() << endl;
        }

        MovieInfo info = reader->info();
        ostringstream str;

        if (info.video)
        {
            str << info.width;
            parts.push_back(str.str());
            str.str("");
            str << info.height;
            parts.push_back(str.str());
            str.str("");
            int nc = info.numChannels;

            switch (info.dataType)
            {
            case TwkFB::FrameBuffer::UCHAR:
                str << "8i";
                break;
            case TwkFB::FrameBuffer::USHORT:
                str << "16i";
                break;
            case TwkFB::FrameBuffer::HALF:
                str << "16f";
                break;
            case TwkFB::FrameBuffer::FLOAT:
                str << "32f";
                break;
            case TwkFB::FrameBuffer::UINT:
                str << "32i";
                break;
            case TwkFB::FrameBuffer::DOUBLE:
                str << "64i";
                break;
            case TwkFB::FrameBuffer::PACKED_Cb8_Y8_Cr8_Y8:
                str << "8i";
                nc = 3;
                break;
            case TwkFB::FrameBuffer::PACKED_Y8_Cb8_Y8_Cr8:
                str << "8i";
                nc = 3;
                break;
            case TwkFB::FrameBuffer::PACKED_R10_G10_B10_X2:
            case TwkFB::FrameBuffer::PACKED_X2_B10_G10_R10:
                str << "10i";
                nc = 3;
                break;
            default:
                str << info.dataType;
            }

            parts.push_back(str.str());
            str.str("");
            str << nc;
            parts.push_back(str.str());
            str.str("");
            int nframes = info.end - info.start + 1;

            if (nframes > 0)
            {
                str << info.fps;
                parts.push_back(str.str());
                str.str("");
                str << nframes;
                parts.push_back(str.str());
                str.str("");
            }
            else
            {
                parts.push_back("");
                parts.push_back("");
            }
        }
        else
        {
            for (int i = 0; i < 6; i++)
                parts.push_back("");
        }

        if (info.audio)
        {
            if (info.audioChannels.size() == 0)
            {
                str << "-";
                parts.push_back(str.str());
                str.str("");
            }
            else
            {
                str << info.audioChannels.size();
                parts.push_back(str.str());
                str.str("");
            }
        }
        else
        {
            for (int i = 0; i < 1; i++)
                parts.push_back("");
        }

        delete reader;
    }
    else if (seq.substr(0, 14) == "RVImageSource|")
    {
        ostringstream str;
        vector<string> strs;
        boost::split(strs, seq, boost::is_any_of("|"));
        parts.push_back(strs[3]);                                  // width
        parts.push_back(strs[4]);                                  // height
        parts.push_back(strs[8] + ((strs[9] == "0") ? "i" : "f")); // depth+type

        str << strs[2].size(); // numChannels
        parts.push_back(str.str());

        string fps = "", frames = "";
        int nframes = atoi(strs[6].c_str()) - atoi(strs[5].c_str()) + 1;
        if (nframes > 0)
        {
            fps = strs[11];
            str.clear();
            str.str("");
            str << nframes;
            frames = str.str();
        }
        parts.push_back(fps);    // fps
        parts.push_back(frames); // frame count

        // no audio
        for (int i = 0; i < 1; i++)
            parts.push_back("");
    }

    parts.push_back(seq);
}

string lsAlignedLongOuput(vector<vector<string>>& listing, bool x2 = true)
{
    ostringstream out;
    vector<size_t> spacing;
    if (listing.empty())
        return "";
    spacing.resize(listing.front().size());

    for (int i = 0; i < spacing.size(); i++)
    {
        for (int q = 1; q < listing.size(); q++)
        {
            if (listing[q].size() > 1)
                spacing[i] = max(spacing[i], listing[q][i].size());
        }
    }

    for (int i = 0; i < spacing.size(); i++)
    {
        for (int q = 0; q < listing.size(); q++)
        {
            if (listing[q].size() > 1)
                if (spacing[i] > 0)
                    spacing[i] = max(spacing[i], listing[q][i].size());
        }
    }

    out << setfill(' ');

    for (int q = 0; q < listing.size(); q++)
    {
        const vector<string>& entry = listing[q];

        if (entry.size() > 1)
        {
            for (int i = 0; i < entry.size(); i++)
            {
                if (spacing[i] == 0)
                    continue;
                if (i == spacing.size() - 1 || i == 1 || i == 3)
                    out << left;
                else
                    out << right;

                if (x2 && i == 1 && entry[i] != "")
                    out << " x ";
                else
                    out << "   ";

                if (i == entry.size() - 1)
                    out << entry[i];
                else
                    out << setw(spacing[i]) << entry[i] << setw(0);
            }

            out << endl;
        }
    }

    return out.str();
}

string lsExtended(const string& seq, bool yaml = false)
{
    YAML::Emitter out_yaml;
    out_yaml.SetIndent(4);
    out_yaml << YAML::BeginMap; // Outter Map
    out_yaml << YAML::Key << seq;
    out_yaml << YAML::Value << YAML::BeginMap; // Sequence Map
    ostringstream out;
    if (MovieReader* reader = TwkMovie::GenericIO::movieReader(seq, bruteForce))
    {
        try
        {
            reader->open(seq);
        }
        catch (std::exception& exc)
        {
            cerr << "ERROR: " << exc.what() << endl;
        }

        const MovieInfo& minfo = reader->info();
        const FrameBuffer& fb = minfo.proxy;

        const FrameBuffer::AttributeVector& attrs = fb.attributes();
        vector<vector<string>> listing;

        out_yaml << YAML::Key << "Attributes";
        out_yaml << YAML::Value << YAML::BeginMap; // Attribute Map
        for (int i = 0; i < attrs.size(); i++)
        {
            listing.resize(listing.size() + 1);
            listing.back().resize(2);

            listing.back()[0] = attrs[i]->name();
            listing.back()[1] = attrs[i]->valueAsString();

            out_yaml << YAML::Key << attrs[i]->name();
            out_yaml << YAML::Value << attrs[i]->valueAsString();
        }
        out_yaml << YAML::EndMap; // Attribute Map End

        if (minfo.video)
        {
            ostringstream str;
            listing.resize(listing.size() + 1);
            listing.back().resize(2);
            listing.back()[0] = "Channels";

            out_yaml << YAML::Key << "Channels";
            out_yaml << YAML::Value << YAML::BeginSeq; // Channel Sequencce

            for (int i = 0; i < minfo.channelInfos.size(); i++)
            {
                if (i)
                    str << ", ";
                str << minfo.channelInfos[i].name;
                out_yaml << minfo.channelInfos[i].name;
            }
            out_yaml << YAML::EndSeq; // Channel Sequence End

            listing.back()[1] = str.str();

            str.str("");
            listing.resize(listing.size() + 1);
            listing.back().resize(2);
            listing.back()[0] = "Resolution";

            str << minfo.uncropWidth << " x " << minfo.uncropHeight << ", ";
            str << minfo.numChannels << "ch, ";

            out_yaml << YAML::Key << "Resolution";
            out_yaml << YAML::Value << YAML::BeginMap; // Resolution Map

            out_yaml << YAML::Key << "Width";
            out_yaml << YAML::Value << minfo.uncropWidth;

            out_yaml << YAML::Key << "Height";
            out_yaml << YAML::Value << minfo.uncropHeight;

            out_yaml << YAML::Key << "Channels";
            out_yaml << YAML::Value << minfo.numChannels;

            out_yaml << YAML::Key << "Depth";
            out_yaml << YAML::Value << YAML::BeginMap; // Depth Map
            switch (minfo.dataType)
            {
            case TwkFB::FrameBuffer::UCHAR:
                str << "8 bits/ch";
                out_yaml << YAML::Key << "Bits";
                out_yaml << YAML::Value << 8;
                out_yaml << YAML::Key << "Type";
                out_yaml << YAML::Value << "int";
                break;
            case TwkFB::FrameBuffer::USHORT:
                str << "16 bits/ch";
                out_yaml << YAML::Key << "Bits";
                out_yaml << YAML::Value << 16;
                out_yaml << YAML::Key << "Type";
                out_yaml << YAML::Value << "int";
                break;
            case TwkFB::FrameBuffer::HALF:
                str << "16 bits/ch floating point";
                out_yaml << YAML::Key << "Bits";
                out_yaml << YAML::Value << 16;
                out_yaml << YAML::Key << "Type";
                out_yaml << YAML::Value << "float";
                break;
            case TwkFB::FrameBuffer::FLOAT:
                str << "32 bits/ch floating point";
                out_yaml << YAML::Key << "Bits";
                out_yaml << YAML::Value << 32;
                out_yaml << YAML::Key << "Type";
                out_yaml << YAML::Value << "float";
                break;
            case TwkFB::FrameBuffer::UINT:
                str << "32 bits/ch";
                out_yaml << YAML::Key << "Bits";
                out_yaml << YAML::Value << 32;
                out_yaml << YAML::Key << "Type";
                out_yaml << YAML::Value << "int";
                break;
            case TwkFB::FrameBuffer::DOUBLE:
                str << "64 bits/ch floating point";
                out_yaml << YAML::Key << "Bits";
                out_yaml << YAML::Value << 64;
                out_yaml << YAML::Key << "Type";
                out_yaml << YAML::Value << "float";
                break;
            case TwkFB::FrameBuffer::PACKED_Cb8_Y8_Cr8_Y8:
            case TwkFB::FrameBuffer::PACKED_Y8_Cb8_Y8_Cr8:
                str << "8 bits/ch";
                out_yaml << YAML::Key << "Bits";
                out_yaml << YAML::Value << 8;
                out_yaml << YAML::Key << "Type";
                out_yaml << YAML::Value << "int";
                break;
            case TwkFB::FrameBuffer::PACKED_R10_G10_B10_X2:
            case TwkFB::FrameBuffer::PACKED_X2_B10_G10_R10:
                str << "10 bits/ch";
                out_yaml << YAML::Key << "Bits";
                out_yaml << YAML::Value << 10;
                out_yaml << YAML::Key << "Type";
                out_yaml << YAML::Value << "int";
                break;
            default:
                str << "?";
                out_yaml << YAML::Key << "Bits";
                out_yaml << YAML::Value << "?";
                out_yaml << YAML::Key << "Type";
                out_yaml << YAML::Value << "?";
            }
            out_yaml << YAML::EndMap; // Depth Map End
            out_yaml << YAML::EndMap; // Resolution Map End

            listing.back()[1] = str.str();
        }

        reverse(listing.begin(), listing.end());

        out << seq << ":" << endl << endl;
        out << lsAlignedLongOuput(listing, false);
        out << endl << endl;

        delete reader;
    }
    else if (seq.substr(0, 14) == "RVImageSource|")
    {
        vector<string> strs;
        boost::split(strs, seq, boost::is_any_of("|"));

        vector<vector<string>> listing;
        ostringstream str;
        listing.resize(listing.size() + 1);
        listing.back().resize(2);
        listing.back()[0] = "Channels";

        out_yaml << YAML::Key << "Channels";
        out_yaml << YAML::Value << YAML::BeginSeq; // Channel Sequence

        for (int i = 0; i < strs[2].size(); i++)
        {
            if (i)
                str << ", ";
            str << strs[2][i];
            out_yaml << strs[2][i];
        }

        out_yaml << YAML::EndSeq; // End Channel Sequence

        listing.back()[1] = str.str();

        str.str("");
        listing.resize(listing.size() + 1);
        listing.back().resize(2);
        listing.back()[0] = "Resolution";

        str << strs[3] << " x " << strs[4] << ", ";
        str << strs[2].size() << "ch, ";
        str << strs[8] << " bits/ch"
            << ((strs[9] == "0") ? "" : " floating point");

        out_yaml << YAML::Key << "Resolution";
        out_yaml << YAML::Value << YAML::BeginMap; // Resolution Map

        out_yaml << YAML::Key << "Width";
        out_yaml << YAML::Value << strs[3];

        out_yaml << YAML::Key << "Height";
        out_yaml << YAML::Value << strs[4];

        out_yaml << YAML::Key << "Channels";
        out_yaml << YAML::Value << strs[2].size();

        out_yaml << YAML::Key << "Depth";
        out_yaml << YAML::Value << YAML::BeginMap; // Depth Map

        out_yaml << YAML::Key << "Bits";
        out_yaml << YAML::Value << strs[8];
        out_yaml << YAML::Key << "Type";
        out_yaml << YAML::Value << ((strs[9] == "0") ? "int" : "float");
        out_yaml << YAML::EndMap; // End Depth Map
        out_yaml << YAML::EndMap; // End Resolution Map

        listing.back()[1] = str.str();

        reverse(listing.begin(), listing.end());

        out << seq << ":" << endl << endl;
        out << lsAlignedLongOuput(listing, false);
        out << endl << endl;
    }

    out_yaml << YAML::EndMap; // Sequence Map
    out_yaml << YAML::EndMap; // Outter Map
    out_yaml << YAML::Newline;

    return yaml ? out_yaml.c_str() : out.str();
}

vector<string> readSession(string path)
{
    vector<string> allfiles;
    string thispath;
    GTOReader reader = new GTOReader();
    GTOReader::Containers nodes = reader.read(path.c_str());
    for (int n = 0; n < nodes.size(); n++)
    {
        if (nodes[n]->protocol() == "RVFileSource")
        {
            PropertyContainer::Components comps = nodes[n]->components();
            for (int c = 0; c < comps.size(); c++)
            {
                if (comps[c]->name() != "media")
                    continue;

                Component::Container props = comps[c]->properties();
                for (int p = 0; p < props.size(); p++)
                {
                    if (props[p]->name() != "movie")
                        continue;

                    vector<string> movs;
                    string movs_string = props[p]->valueAsString();
                    split(movs, movs_string, is_any_of(string(" ")));
                    for (int m = 0; m < movs.size(); m++)
                    {
                        thispath = movs[m];
                        if (movs[m].size() > 13
                            && movs[m].substr(0, 14) == "${RV_PATHSWAP_")
                        {
                            int endpos = movs[m].find("}");
                            if (endpos == string::npos)
                                continue;
                            string envvar = movs[m].substr(2, endpos - 2);
                            char* pathswap = getenv(envvar.c_str());
                            if (pathswap != NULL)
                            {
                                thispath.replace(0, endpos + 1, pathswap);
                            }
                        }
                    }
                }
            }
            allfiles.push_back(thispath);
        }
        else if (nodes[n]->protocol() == "RVImageSource")
        {
            ostringstream imgsrc;
            string name = "", channels = "";
            int uncropWidth = 0, uncropHeight = 0, start = 0, end = 0, inc = 0,
                bitsPerChannel = 0, isFloat = 0;
            float pixelAspect = 0.0, fps = 0.0;
            PropertyContainer::Components comps = nodes[n]->components();
            for (int c = 0; c < comps.size(); c++)
            {
                if (comps[c]->name() != "media" && comps[c]->name() != "image")
                    continue;

                Component::Container props = comps[c]->properties();
                for (int p = 0; p < props.size(); p++)
                {
                    string pName = props[p]->name();
                    if (pName == "name")
                        name = props[p]->valueAsString();
                    else if (pName == "channels")
                        channels = props[p]->valueAsString();
                    else if (pName == "uncropWidth")
                        uncropWidth =
                            reinterpret_cast<IntProperty*>(props[p])->front();
                    else if (pName == "uncropHeight")
                        uncropHeight =
                            reinterpret_cast<IntProperty*>(props[p])->front();
                    else if (pName == "start")
                        start =
                            reinterpret_cast<IntProperty*>(props[p])->front();
                    else if (pName == "end")
                        end = reinterpret_cast<IntProperty*>(props[p])->front();
                    else if (pName == "inc")
                        inc = reinterpret_cast<IntProperty*>(props[p])->front();
                    else if (pName == "bitsPerChannel")
                        bitsPerChannel =
                            reinterpret_cast<IntProperty*>(props[p])->front();
                    else if (pName == "float")
                        isFloat =
                            reinterpret_cast<IntProperty*>(props[p])->front();
                    else if (pName == "pixelAspect")
                        pixelAspect =
                            reinterpret_cast<FloatProperty*>(props[p])->front();
                    else if (pName == "fps")
                        fps =
                            reinterpret_cast<FloatProperty*>(props[p])->front();
                }
            }
            imgsrc << "RVImageSource|" << name << "|" << channels << "|"
                   << uncropWidth << "|" << uncropHeight << "|" << start << "|"
                   << end << "|" << inc << "|" << bitsPerChannel << "|"
                   << isFloat << "|" << pixelAspect << "|" << fps << "|";

            thispath = imgsrc.str();
            imgsrc.str("");
            allfiles.push_back(thispath);
        }
    }
    return allfiles;
}

int utf8Main(int argc, char** argv)
{
    TwkFB::ThreadPool::initialize();
    const QCoreApplication qapp(argc, argv);

#ifdef PLATFORM_DARWIN
    TwkApp::DarwinBundle bundle("RV", MAJOR_VERSION, MINOR_VERSION,
                                REVISION_NUMBER);
#else
    TwkApp::QTBundle bundle("rv", MAJOR_VERSION, MINOR_VERSION,
                            REVISION_NUMBER);
    (void)bundle.top();
#endif

    int showVersion = 0;
    int a = 0;
    int s = 0;
    int b = 0;
    int minseq = 3;
    int nr = 0;
    int l = 0;
    int x = 0;
    int ns = 0;
    int showFormats = 0;
    int yaml = 0;
    char* debugString = 0;
    char* outputFile = 0;

    //
    //  Call the deploy functions
    //

    TWK_DEPLOY_APP_OBJECT dobj(MAJOR_VERSION, MINOR_VERSION, REVISION_NUMBER,
                               argc, argv, RELEASE_DESCRIPTION,
                               "HEAD=" GIT_HEAD);

    if (arg_parse(
            argc, argv, "", "\nUsage: rvls list movies and image sequences\n",
            "", ARG_SUBR(parseInFiles),
            "Input sequence patterns, images, movies, or directories ", "-a",
            ARG_FLAG(&a), "Show hidden files", "-s", ARG_FLAG(&s),
            "Show sequences only (no non-sequence member files)", "-l",
            ARG_FLAG(&l), "Show long listing", "-x", ARG_FLAG(&x),
            "Show extended attributes and image structure", "-b", ARG_FLAG(&b),
            "Use brute force if no reader found", "-o %S", &outputFile,
            "Output log file. Results will be printed to stdout by default",
            "-nr", ARG_FLAG(&nr), "Do not show frame ranges", "-ns",
            ARG_FLAG(&ns), "Do not infer sequences (list each file separately)",
            "-min %d", &minseq,
            "Minimum number of files considered a sequence (default=%d)",
            minseq, "-formats", ARG_FLAG(&showFormats),
            "List image/movie formats", "-yaml", ARG_FLAG(&yaml),
            "Output in YAML format. (-x only)", "-version",
            ARG_FLAG(&showVersion), "Show rvls version number", "-debug %S",
            &debugString, "Debug category (only 'plugins' for now)", NULL)
        < 0)
    {
        exit(-1);
    }

    bruteForce = b ? true : false;
    bool nonmatching = !s ? true : false;
    bool showranges = nr ? false : true;

    try
    {
        debugSwitches(debugString);
    }
    catch (const std::exception& e)
    {
        cerr << "ERROR: during initialization: " << e.what() << endl;
        exit(-1);
    }

    if (showVersion)
    {
        cout << MAJOR_VERSION << "." << MINOR_VERSION << "." << REVISION_NUMBER
             << endl;

        exit(0);
    }

    bundle.addPathToEnvVar("OIIO_LIBRARY_PATH", bundle.appPluginPath("OIIO"));
    TwkApp::Bundle::PathVector licfiles = bundle.licenseFiles("license", "gto");

    try
    {
        TwkFB::loadProxyPlugins("TWK_FB_PLUGIN_PATH");
        TwkMovie::loadProxyPlugins("TWK_MOVIE_PLUGIN_PATH");
    }
    catch (...)
    {
        cerr << "WARNING: a problem occured while loading image plugins."
             << endl;
        cerr << "         some plugins may not have been loaded." << endl;
    }

    ostringstream out;

    TwkMovie::GenericIO::addPlugin(new MovieFBIO());
    TwkMovie::GenericIO::addPlugin(new MovieProceduralIO());

    try
    {
        TwkApp::initMu(nullptr);
        TwkApp::initPython();
    }
    catch (const std::exception& e)
    {
        cerr << "ERROR: during initialization: " << e.what() << '\n';
        exit(-1);
    }

    if (showFormats)
    {
        TwkMovie::GenericIO::outputFormats();
        exit(0);
    }

    TwkFB::GenericIO::compileExtensionSet(predicateFileExtensions());

    if (inputFiles.empty())
    {
        inputFiles.push_back(".");
    }

    try
    {
        vector<string> allfiles;

        for (int i = 0; i < inputFiles.size(); i++)
        {
            string path = inputFiles[i];

            if (isDirectory(path.c_str()))
            {
                if (path[path.size() - 1] != '/')
                    path.append("/");
                vector<string> files;

                if (filesInDirectory(path.c_str(), files))
                {
                    for (int q = 0; q < files.size(); q++)
                    {
                        if (files[q].size() && files[q][0] == '.' && !a)
                            continue;
                        string file = path;
                        file += files[q];
                        allfiles.push_back(file);
                    }
                }
            }
            else if (path.size() > 2
                     && path.substr(path.size() - 3, path.size() - 1) == ".rv")
            {
                allfiles = readSession(path);
            }
            else
            {
                allfiles.push_back(path);
            }
        }

        SequenceNameList seqs;

        if (ns)
        {
            seqs = allfiles;
        }
        else
        {
            SequencePredicate sPred =
                (bruteForce) ? AnySequencePredicate : GlobalExtensionPredicate;
            seqs = sequencesInFileList(allfiles, sPred, nonmatching, showranges,
                                       minseq);
        }
        std::sort(seqs.begin(), seqs.end());

        if (l)
        {
            vector<vector<string>> listing(seqs.size() + 1);
            listing.front().push_back("w");
            listing.front().push_back("h");
            listing.front().push_back("typ");
            listing.front().push_back("#ch");
            listing.front().push_back("fps");
            listing.front().push_back("#fr");
            listing.front().push_back("#ach");
            listing.front().push_back("file");

            for (int i = 0; i < seqs.size(); i++)
            {
                lsLong(seqs[i], listing[i + 1]);
            }

            out << lsAlignedLongOuput(listing);
        }
        else if (x)
        {
            for (int i = 0; i < seqs.size(); i++)
            {
                out << lsExtended(seqs[i], yaml);
            }
        }
        else
        {
            copy(seqs.begin(), seqs.end(), ostream_iterator<string>(out, "\n"));
        }
    }
    catch (std::exception& exc)
    {
        cerr << "ERROR: " << exc.what() << endl;
        exit(-1);
    }
    catch (...)
    {
        cerr << "ERROR: uncaught exception" << endl;
        exit(-1);
    }

    if (outputFile)
    {
        ofstream output(outputFile);
        output << out.str();
        output.close();
    }
    else
    {
        cout << out.str();
    }

    TwkFB::ThreadPool::shutdown();

    return 0;
}
