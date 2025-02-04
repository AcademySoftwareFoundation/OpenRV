//******************************************************************************
// Copyright (c) 2001-2007 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <MovieFB/MovieFB.h>
#include <MovieFB/MovieFBWriter.h>
#include <TwkFB/IO.h>
#include <TwkFB/Exception.h>
#include <TwkMovie/MovieIO.h>
#include <TwkMovie/Movie.h>
#include <TwkUtil/File.h>
#include <TwkUtil/FrameUtils.h>
#include <TwkUtil/PathConform.h>
#include <boost/filesystem/operations.hpp>
#include <algorithm>
#include <ctype.h>
#include <stl_ext/string_algo.h>
#include <iostream>
#include <stdlib.h>
#include <string>

#if defined(TWK_LITTLE_ENDIAN) || defined(__LITTLE_ENDIAN__)
#define AF_BYTEORDER AF_BYTEORDER_LITTLEENDIAN
#else
#define AF_BYTEORDER AF_BYTEORDER_BIGENDIAN
#endif

#if 0
#define DB_DOIT true
#define DB_GENERAL 0x01
#define DB_ALL 0xff

#define DB_LEVEL DB_ALL

#define DB(x)                  \
    if (DB_GENERAL & DB_LEVEL) \
    cerr << "MovieFB: " << x << endl
#define DBL(level, x)     \
    if (level & DB_LEVEL) \
    cerr << "MovieFB: " << x << endl
#else
#define DB_DOIT false
#define DB(x)
#define DBL(level, x)
#endif

namespace TwkMovie
{

    using namespace std;
    using namespace TwkFB;
    using namespace TwkUtil;
    using namespace TwkMovie;

    int MovieFBIO::m_readerCount = 0;

    MovieFB::MovieFB()
        : MovieReader()
        , m_frameInfoValid(false)
        , m_imgio(0)
    {
        pthread_mutex_init(&m_frameLock, 0);

        m_threadSafe = true;
    }

    MovieFB::~MovieFB() { pthread_mutex_destroy(&m_frameLock); }

    Movie* MovieFB::clone() const
    {
        MovieFB* m = new MovieFB();
        if (m_filename != "")
            m->open(m_filename, m_info);
        return m;
    }

    bool MovieFB::getImageInfo(const std::string& filename)
    {

        //
        //  Try the image Reader by extension
        //

        bool success = false;
        if (const FrameBufferIO* imgio = TwkFB::GenericIO::findByExtension(
                extension(m_imagePattern), FrameBufferIO::ImageRead))
        {
            try
            {
                imgio->getImageInfo(filename, m_imgInfo);
                m_imgio = imgio;
                success = true;
            }
            catch (const std::exception& exc)
            {
                cerr << "ERROR: plugin '" << imgio->identifier()
                     << "' cannot read '" << filename << "': " << exc.what()
                     << endl;
            }
            catch (...)
            {
                cerr << "ERROR: plugin '" << imgio->identifier()
                     << "' cannot read '" << filename << "'" << endl;
            }
        }

        //
        //  Try the image Reader by Brute Force
        //

        if (!success)
        {
            if (const FrameBufferIO* imgio = TwkFB::GenericIO::findByBruteForce(
                    filename, FrameBufferIO::ImageRead))
            {
                try
                {
                    imgio->getImageInfo(filename, m_imgInfo);
                    m_imgio = imgio;
                    success = true;
                }
                catch (const std::exception& exc)
                {
                    cerr << "ERROR: plugin '" << imgio->identifier()
                         << "' cannot read '" << filename << "': " << exc.what()
                         << endl;
                }
                catch (...)
                {
                    cerr << "ERROR: plugin '" << imgio->identifier()
                         << "' cannot read '" << filename << "'" << endl;
                }
            }
        }

        return success;
    }

    void MovieFB::open(const string& filename, const MovieInfo& info,
                       const Movie::ReadRequest& request)
    {
        m_filename = filename;
        m_imagePattern = pathConform(filename);
        m_info = info;
        m_filehash = stl_ext::hash(filename);

        updateFrameInfo();

        //
        // Attempt to find a file with correct Image info.  Given that the first
        // frame can be a slate, we attempt to extract the movie info from the
        // following frames
        // 1. the 2nd frame
        // 2. the frame at 1/2 the sequence length
        // 3. the frame at 1/4 the sequence length
        // 4. the 1st frame
        // 5. any frame, searched in order if NONE of the above frames succeeded
        //

        ExistingFileList existingFiles = existingFilesInSequence(filename);

        int successFrame = -1;
        bool infoSuccess = false;
        int testFrames[4] = {1, static_cast<int>(existingFiles.size() / 2),
                             static_cast<int>(existingFiles.size() / 4), 0};
        std::set<int> testedFrames;
        for (int t = 0; t < 4 && !infoSuccess; ++t)
        {
            int testFrame = testFrames[t];
            if (!infoSuccess
                && testedFrames.find(testFrame) == testedFrames.end()
                && testFrame < existingFiles.size()
                && existingFiles[testFrame].exists)
            {
                testedFrames.insert(testFrame);
                infoSuccess = getImageInfo(existingFiles[testFrame].name);
                if (infoSuccess)
                    successFrame = testFrame;
            }
        }

        // Last Ditch effort, try and read all frames...
        if (!infoSuccess)
        {
            for (int testFrame = 0;
                 testFrame < existingFiles.size() && !infoSuccess; ++testFrame)
            {
                if (!infoSuccess
                    && testedFrames.find(testFrame) == testedFrames.end()
                    && testFrame < existingFiles.size()
                    && existingFiles[testFrame].exists)
                {
                    testedFrames.insert(testFrame);
                    infoSuccess = getImageInfo(existingFiles[testFrame].name);
                    if (infoSuccess)
                        successFrame = testFrame;
                }
            }
        }

        if (!infoSuccess)
        {
            if (m_imgio)
            {
                TWK_THROW_EXC_STREAM("Failed to read " << m_imagePattern
                                                       << " using "
                                                       << m_imgio->about());
            }
            else
            {

                TWK_THROW_EXC_STREAM("No plugin found can read "
                                     << m_imagePattern);
            }
        }
        else
        {
            std::cout << "INFO: Read image info from "
                      << existingFiles[successFrame].name << std::endl;
        }

        m_info = m_imgInfo;
        m_info.start = m_frames.empty() ? 1 : m_frames.front();
        m_info.end = m_frames.empty() ? 1 : m_frames.back();
        // m_info.inc   = guessIncrement( m_frames, 0, 3 );
        m_info.inc = 1;
        m_info.audio = false;
        m_info.video = true;
        m_info.fps = 0;

        m_info.channelInfos = m_imgInfo.channelInfos;
        m_info.viewInfos = m_imgInfo.viewInfos;

        // Check if there is a slate
        if (existingFiles[0].exists)
        {
            infoSuccess = getImageInfo(existingFiles[0].name);
        }
        else
        {
            infoSuccess = false;
        }

        // Assume the first frame is a slate if it is different than the rest of
        // the sequence. Note that we need to make sure that we are not
        // misinterpreting a sequence of images having different bounding boxes
        // as a slate so this is why we are NOT using the == operator here.
        if (infoSuccess)
        {
            const bool sameInfo =
                m_imgInfo.width == m_info.width
                && m_imgInfo.height == m_info.height
                && m_imgInfo.uncropWidth == m_info.uncropWidth
                && m_imgInfo.uncropHeight == m_info.uncropHeight
                && m_imgInfo.numChannels == m_info.numChannels
                && m_imgInfo.viewInfos.empty() == m_info.viewInfos.empty()
                && (m_imgInfo.viewInfos.empty()
                    || (m_imgInfo.viewInfos.front().name
                        == m_info.viewInfos.front().name));
            if (!sameInfo)
            {
                m_info.slate = m_imgInfo;
                m_info.hasSlate = true;
            }
        }

        //
        //  Compensate for "older" formats that don't know about uncrop.
        //

        if (!m_info.uncropWidth && !m_info.uncropHeight)
        {
            m_info.uncropWidth = m_info.width;
            m_info.uncropHeight = m_info.height;
        }

        //
        //  Try and find a frame rate attr. Try the DPX motion picture
        //  followed by tv header attrs and then the EXR fps attr. So far
        //  those are the only ones I know about.
        //
        //  Normalize FPS attrs to "FPS" attribute the presence of which
        //  means we found FPS metadata (and its value).
        //
        //  This 1.0 fps case for DPX is to get around (SGO) Mistika
        //  created DPX files that leave the fps set to 1 for MP and have
        //  the actual fps in TV.
        //

        if (m_info.proxy.hasAttribute("DPX-MP/FrameRate"))
        {
            if (TwkFB::FBAttribute* a =
                    m_info.proxy.findAttribute("DPX-MP/FrameRate"))
            {
                if (TwkFB::TypedFBAttribute<float>* ta =
                        dynamic_cast<TypedFBAttribute<float>*>(a))
                {
                    m_info.fps =
                        m_info.proxy.attribute<float>("DPX-MP/FrameRate");
                }
                else if (TwkFB::TypedFBAttribute<string>* ta =
                             dynamic_cast<TypedFBAttribute<string>*>(a))
                {
                    m_info.fps =
                        atof(m_info.proxy.attribute<string>("DPX-MP/FrameRate")
                                 .c_str());
                }
            }
            if (m_info.fps)
                m_info.proxy.newAttribute<float>("FPS", m_info.fps);
        }

        if ((m_info.fps == 0.0 || m_info.fps == 1.0)
            && m_info.proxy.hasAttribute("DPX-TV/FrameRate"))
        {
            if (TwkFB::FBAttribute* a =
                    m_info.proxy.findAttribute("DPX-TV/FrameRate"))
            {
                if (TwkFB::TypedFBAttribute<float>* ta =
                        dynamic_cast<TypedFBAttribute<float>*>(a))
                {
                    m_info.fps =
                        m_info.proxy.attribute<float>("DPX-TV/FrameRate");
                }
                else if (TwkFB::TypedFBAttribute<string>* ta =
                             dynamic_cast<TypedFBAttribute<string>*>(a))
                {
                    m_info.fps =
                        atof(m_info.proxy.attribute<string>("DPX-TV/FrameRate")
                                 .c_str());
                }
            }
            if (m_info.fps)
                m_info.proxy.newAttribute<float>("FPS", m_info.fps);
        }

        if (m_info.fps == 0.0
            && m_info.proxy.hasAttribute("EXR/framesPerSecond"))
        {
            if (TwkFB::FBAttribute* a =
                    m_info.proxy.findAttribute("EXR/framesPerSecond"))
            {
                if (TwkFB::TypedFBAttribute<float>* ta =
                        dynamic_cast<TypedFBAttribute<float>*>(a))
                {
                    m_info.fps =
                        m_info.proxy.attribute<float>("EXR/framesPerSecond");
                }
                else if (TwkFB::TypedFBAttribute<string>* ta =
                             dynamic_cast<TypedFBAttribute<string>*>(a))
                {
                    m_info.fps = atof(
                        m_info.proxy.attribute<string>("EXR/framesPerSecond")
                            .c_str());
                }
            }
            if (m_info.fps)
                m_info.proxy.newAttribute<float>("FPS", m_info.fps);
        }

        if (m_info.fps == 0.0 && m_info.proxy.hasAttribute("EXR/FPS"))
        {
            if (TwkFB::FBAttribute* a = m_info.proxy.findAttribute("EXR/FPS"))
            {
                if (TwkFB::TypedFBAttribute<float>* ta =
                        dynamic_cast<TypedFBAttribute<float>*>(a))
                {
                    m_info.fps = m_info.proxy.attribute<float>("EXR/FPS");
                }
                else if (TwkFB::TypedFBAttribute<string>* ta =
                             dynamic_cast<TypedFBAttribute<string>*>(a))
                {
                    m_info.fps =
                        atof(m_info.proxy.attribute<string>("EXR/FPS").c_str());
                }
            }
            if (m_info.fps)
                m_info.proxy.newAttribute<float>("FPS", m_info.fps);
        }

        if (m_info.fps != m_info.fps)
        {
            if (m_info.fps != m_info.fps)
                m_info.proxy.attribute<string>("WARNING:") =
                    "Bogus FPS -- set to 24.0";

            m_info.fps = 24;
        }

        //
        //  Allow fps==0.0 to pass through if there was no appropriate
        //  attribute; it'll pick up the session default fps later.
        //
    }

    bool MovieFB::fileAndIdAtFrame(int& frame, string& filename,
                                   ostream& idstream, bool nearby)
    {
        updateFrameInfo();

        if (m_frames.empty())
        {
            frame = 1;
            filename = m_filename;
            idstream << m_filename;
            return true;
        }

        if (frame < m_frames.front())
            frame = m_frames.front();
        if (frame > m_frames.back())
            frame = m_frames.back();

        FrameMap::const_iterator f = m_frameMap.find(frame);

        if (f == m_frameMap.end())
        {
            if (nearby)
            {
                CompareFrameFilePair cffp;
                FrameMap::const_iterator n = lower_bound(
                    m_frameMap.begin(), m_frameMap.end(),
                    FrameMap::value_type(frame, FrameFile("")), cffp);

                //
                //  n is the smallest frame > target frame, or there are
                //  no frames > target frame, so use the last frame we have.
                //
                if (n != m_frameMap.begin())
                    n--;

                frame = n->first;
                filename = n->second.fileName;
                idstream << n->second.fileName << "_" << n->second.generation;
                return false;
            }

            TWK_THROW_EXC_STREAM("really didn't find anything at frame "
                                 << frame << " for " << m_filename);
        }
        else
        {
            frame = f->first;
            filename = f->second.fileName;
            idstream << f->second.fileName << "_" << f->second.generation;
            return true;
        }

        return false;
    }

    void MovieFB::imagesAtFrame(const ReadRequest& mrequest,
                                FrameBufferVector& fbs)
    {
        updateFrameInfo();

        int frame = mrequest.frame;

        FrameBufferIO::ReadRequest request;
        request.allChannels = mrequest.allChannels;
        request.views = mrequest.views;
        request.layers = mrequest.layers;
        request.channels = mrequest.channels;
        request.parameters = mrequest.parameters;

        //
        //  May throw (which is fine). If the image is missing and it
        //  doesn't throw read whatever we got. (probably a nearby frame)
        //  then throw.
        //

        string filename;
        ostringstream idstr;

        bool found = fileAndIdAtFrame(frame, filename, idstr, mrequest.missing);

        if (!found && !mrequest.missing)
        {
            TWK_THROW_EXC_STREAM("Out of range frame " << mrequest.frame
                                                       << " in "
                                                       << basename(m_filename));
        }

        m_imgio->readImages(fbs, filename, request);

        //  We read something, and didn't throw, so update readtime.  Note that
        //  "frame" may now be other than request.frame, since we may have
        //  encountered a missing frame.

        pthread_mutex_lock(&m_frameLock);

        FrameMap::iterator i = m_frameMap.find(frame);
        if (i != m_frameMap.end())
        {
            i->second.readTime = std::time(0);
        }

        pthread_mutex_unlock(&m_frameLock);

        for (unsigned int i = 0; i < fbs.size(); i++)
        {
            FrameBuffer* fb = fbs[i];
            fb->setIdentifier("");
            fb->idstream() << idstr.str() << "/";

            if (fb->hasAttribute("View"))
            {
                fb->idstream() << fb->attribute<string>("View");
            }

            if (fb->hasAttribute("Layer"))
            {
                fb->idstream() << ":" << fb->attribute<string>("Layer");
            }

            if (fb->hasAttribute("Channel"))
            {
                fb->idstream() << ":" << fb->attribute<string>("Channel");
            }

            if (FBAttribute* a = fb->findAttribute("File"))
            {
                fb->deleteAttribute(a);
            }

            if (FBAttribute* a = fb->findAttribute("Sequence"))
            {
                fb->deleteAttribute(a);
            }

            fb->newAttribute("Sequence", m_imagePattern);
            fb->newAttribute("File", filename);
        }
    }

    void MovieFB::identifiersAtFrame(const ReadRequest& request,
                                     IdentifierVector& ids)
    {
        //
        //  There are two things about this function that are tricky:
        //
        //  1) You have to mimic exactly what imagesAtFrame() is doing
        //  when it creates identifier strings via the returned image
        //  attributes for View and Layer and Channels.
        //
        //  2) You also have to mimic what the IOexr reader is doing when it
        //  tries to read the (channel * layer * view) images (including the
        //  default view and layer cases) in the sense that detectable illegal
        //  combinations should not be returned.
        //

        const StringVector& views = m_info.views;
        const StringVector& layers = m_info.layers;
        const FBInfo::ChannelInfoVector& channelInfos = m_info.channelInfos;

        int frame = request.frame;
        bool searchNames = !request.views.empty();

        string filename;
        ostringstream idstr;

        if (!fileAndIdAtFrame(frame, filename, idstr, request.missing))
        {
            if (!request.missing)
                TWK_THROW_EXC_STREAM("id failed");
        }

        string baseid = idstr.str() + "/";

        for (unsigned int i = 0;
             (i == 0 && request.views.empty()) || i < request.views.size(); i++)
        {
            ostringstream id;

            const string& viewName =
                (m_info.hasSlate && request.frame == m_info.start)
                    ? m_info.slate.defaultView
                : request.views.empty() ? m_info.defaultView
                                        : request.views[i];

            id << baseid;

            if (find(views.begin(), views.end(), viewName) != views.end())
            {
                id << viewName;
            }
            else if (viewName != "")
            {
                //
                //  Not in our list of views
                //

                cout << "WARNING: ignoring request view " << viewName << endl;
                continue;
            }

            if (!request.layers.empty())
            {
                for (unsigned int i = 0; i < request.layers.size(); i++)
                {
                    const string& layerName = request.layers[i];
                    if (!layerName.empty())
                    {
                        id << ":";

                        if (find(layers.begin(), layers.end(), layerName)
                            != layers.end())
                        {
                            id << layerName;
                        }
                    }
                }
            }

            if (!request.channels.empty())
            {
                for (unsigned int i = 0; i < request.channels.size(); i++)
                {
                    const string& channelName = request.channels[i];
                    if (!channelName.empty())
                    {
                        id << ":";

                        for (unsigned int j = 0; j < channelInfos.size(); j++)
                        {
                            if (channelInfos[j].name == channelName)
                            {
                                id << channelName;
                                break;
                            }
                        }
                    }
                }
            }

            ids.push_back(id.str());
        }
    }

    void MovieFB::updateFrameInfo()
    {
        if (m_frameInfoValid)
            return;

        //
        //  Lock frame structures first
        //

        pthread_mutex_lock(&m_frameLock);

        //
        //  Perhaps someone else updated the frame structures while
        //  this thread was waiting on the lock.
        //

        if (m_frameInfoValid)
        {
            pthread_mutex_unlock(&m_frameLock);
            return;
        }

        DB("updateFrameInfo pattern '" << m_imagePattern << "'");

        //
        //  XXX I think this needs to be rewritten.  This is what we're doing:
        //
        //  * find existing files in input sequencePattern (m_imagePattern)
        //  * deduce _new_ sequencePattern from existing files
        //  * split that new pattern into time-range etc
        //  * initilize m_frames from new patterns time-range
        //  * foreach frame in m_frames, assume that there is correspponding
        //    entry in existing list (even though that came from a different
        //    sequence pattern, and potentially add that to the m_frameMap
        //
        //  What I think we should do:
        //
        //  * get existing file list from original pattern
        //  * foreach existing file, figure out what frame it is, and add
        //    to both m_frames and m_frameMap
        //
        //  or something like that (there are some subtlties that I
        //  don't have time to figure out at the moment).
        //

        m_frames.clear();
        FrameMap oldFrameMap = m_frameMap;
        m_frameMap.clear();

        DB("    calling existingFilesInSequence'");
        ExistingFileList files = existingFilesInSequence(m_imagePattern);

        if (files.empty())
        {
            m_frameInfoValid = true;
            pthread_mutex_unlock(&m_frameLock);
            TWK_THROW_EXC_STREAM("No files matched " << m_imagePattern);
        }

        string seqName = "";
        string timeStr = "";
        DB("    calling sequencePattern'");
        SequencePattern tmpPattern = sequencePattern(files, false);
        DB("    sequencePattern returns '" << tmpPattern << "'");

        //
        // Initialize the timeStr by checking if this is splitable
        //
        bool split = splitSequenceName(tmpPattern, timeStr, seqName);

        if (files.size() == 1 && m_imagePattern == files[0].name)
        //
        // If this is one on disk frame simply snag it and guess the frame
        // number
        //
        {
            TwkUtil::FrameList frames = frameRange(timeStr);
            if (frames.size() > 0)
                m_frames.push_back(frames[0]);
            else
                m_frames.push_back(1);
            DB("    only one 'existing' frame, number " << m_frames[0] << endl);
        }
        else if (split)
        //
        // Next see if this is a range of frames
        //
        {
            DB("    split succeeded timeStr '" << timeStr << "'");
            m_frames = frameRange(timeStr);
        }
        else
        //
        // Otherwise grab whatever we can
        //
        {
            m_frames.push_back(1);
        }

        //
        //  If we just have one frame/file and it really exists, just add it
        //  with the frame number we got from frameRange() above.  Otherwise use
        //  the frame numbers reported by existingFilesInSequence().
        //
        //  XXX See above comment.  This is slightly improved, but should all be
        //  re-written to clarify and in particular remove m_frames.
        //

        if (files.size() == 1 && m_frames.size() == 1 && files[0].exists)
        {
            DB("    adding " << m_frames[0] << " -> '" << files[0].name << "'");
            m_frameMap[m_frames[0]] = FrameFile(files[0].name);
        }
        else
        {
            for (int i = 0; i < files.size(); ++i)
            {
                if (files[i].exists)
                {
                    DB("    i " << i << ": adding " << files[i].frame << " -> '"
                                << files[i].name << "'");
                    m_frameMap[files[i].frame] = FrameFile(files[i].name);
                }
            }
        }

        //
        //  For any file from new map that is in old map and corresponds to same
        //  file and has been read in the past, check now to see if it has been
        //  modified since, and in that case update the generation number. Since
        //  the new generation number goes into the ID, this will force a reload
        //  the next time we want to show or cache that frame.
        //

        for (FrameMap::iterator i = m_frameMap.begin(); i != m_frameMap.end();
             ++i)
        {
            FrameMap::iterator oldI = oldFrameMap.find(i->first);

            if (oldI != oldFrameMap.end()
                && oldI->second.fileName == i->second.fileName)
            {
                //  The file may have been read more than once already, so adopt
                //  that generation/readTime.

                i->second.generation = oldI->second.generation;
                i->second.readTime = oldI->second.readTime;

                //  If the file has been written since we last read it, inc
                //  generation. Note that we don't even check if we've never
                //  read this file.

                if (i->second.readTime != std::time_t(0))
                {
                    boost::filesystem::path p(UNICODE_STR(i->second.fileName));

                    if (boost::filesystem::last_write_time(p)
                        > i->second.readTime)
                    {
                        ++(i->second.generation);
                    }
                }
            }
        }

        m_frameInfoValid = true;

        pthread_mutex_unlock(&m_frameLock);
    }

    MovieFBIO::MovieFBIO()
        : MovieIO("Frame Sequence", "u")
    {
        typedef FrameBufferIO::ImageTypeInfos ImageInfos;
        typedef FrameBufferIO::ImageTypeInfo ImageInfo;

        const TwkFB::GenericIO::Plugins& plugins =
            TwkFB::GenericIO::allPlugins();

        for (TwkFB::GenericIO::Plugins::const_iterator i = plugins.begin();
             i != plugins.end(); ++i)
        {
            const FrameBufferIO* io = *i;
            const ImageInfos& infos = io->extensionsSupported();

            for (unsigned int q = 0; q < infos.size(); q++)
            {
                const ImageInfo& info = infos[q];

                unsigned int capabilities = 0;
                if (info.capabilities & FrameBufferIO::ImageRead)
                    capabilities |= MovieIO::MovieRead;
                if (info.capabilities & FrameBufferIO::ImageWrite)
                    capabilities |= MovieIO::MovieWrite;
                if (info.capabilities & FrameBufferIO::BruteForceIO)
                    capabilities |= MovieIO::MovieBruteForceIO;

                StringPairVector audioCodecs;
                ParameterVector eparams;
                ParameterVector dparams;

                for (size_t i = 0; i < info.decodeParameters.size(); i++)
                {
                    const StringPair& p = info.decodeParameters[i];
                    dparams.push_back(Parameter(p.first, p.second, ""));
                }

                for (size_t i = 0; i < info.encodeParameters.size(); i++)
                {
                    const StringPair& p = info.encodeParameters[i];
                    eparams.push_back(Parameter(p.first, p.second, ""));
                }

                addType(info.extension, info.description, capabilities,
                        info.compressionSchemes, audioCodecs, dparams, eparams);
            }
        }
    }

    MovieFBIO::~MovieFBIO() {}

    std::string MovieFBIO::about() const
    {
        return "Tweak Image I/O Movie Sequence";
    }

    MovieReader* MovieFBIO::movieReader() const
    {
        ++m_readerCount;
        return new MovieFB();
    }

    MovieWriter* MovieFBIO::movieWriter() const { return new MovieFBWriter(); }

    void MovieFBIO::getMovieInfo(const std::string& filename,
                                 MovieInfo& minfo) const
    {
        string fname = firstFileInPattern(filename);
        string ext = extension(fname);

        if (const FrameBufferIO* io =
                TwkFB::GenericIO::findByExtension(extension(filename)))
        {
            try
            {
                io->getImageInfo(fname, minfo);
            }
            catch (...)
            {
                if (io = TwkFB::GenericIO::findByBruteForce(fname))
                {
                    io->getImageInfo(filename, minfo);
                }
                else
                {
                    TWK_THROW_STREAM(IOException,
                                     "Unsupported file: " << filename);
                }
            }
        }
        else
        {
            if (const FrameBufferIO* io =
                    TwkFB::GenericIO::findByBruteForce(fname))
            {
                io->getImageInfo(fname, minfo);
            }
            else
            {
                TWK_THROW_STREAM(IOException, "Unsupported file: " << filename);
            }
        }
    }

} // namespace TwkMovie
