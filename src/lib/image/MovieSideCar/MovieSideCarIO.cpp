//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <MovieSideCar/MovieSideCarIO.h>
#include <MovieSideCar/MovieSideCar.h>
#include <TwkExc/Exception.h>
#include <TwkFB/Exception.h>
#include <boost/filesystem.hpp>
#include <boost/functional/hash.hpp>

namespace TwkMovie
{
    using namespace std;
    using namespace boost::program_options;
    using namespace boost::algorithm;
    using namespace boost::filesystem;
    using namespace TwkFB;
    using namespace boost;

    MovieSideCarIO::MovieSideCarIO(const string& pathToSideCar, bool cloneable)
        : m_sidecarPath(pathToSideCar)
        , m_cloneable(cloneable)
    {
    }

    MovieSideCarIO::~MovieSideCarIO()
    {
        //
        // The plugin is begining unloaded so...
        // shutdown and sidecars that are running.
        //
        TwkMovie::shutdownMovieSideCars();
    }

    string MovieSideCarIO::about() const { return "Tweak SideCar Bridge"; }

    MovieReader* MovieSideCarIO::movieReader() const
    {
        return new MovieSideCar(m_sidecarPath, m_cloneable);
    }

    MovieWriter* MovieSideCarIO::movieWriter() const { return 0; }

    void MovieSideCarIO::getMovieInfo(const std::string& filename,
                                      MovieInfo& info) const
    {
        //
        //  This is brute force -- in order to read the info we need to start
        //  up a sidecar process and open the file. At that point the reader's
        //  info will be accurate and we can just scarf it.
        //

        path filepath(filename);

        if (filepath.has_extension() && filepath.extension() != "sidecar")
        {
            TWK_THROW_STREAM(IOException, "Not a sidecar: " << filename);
        }

        MovieSideCar reader(m_sidecarPath, m_cloneable);
        reader.open(filename);
        info = reader.info();
    }

    //----------------------------------------------------------------------

    MovieR3DSideCarIO::MovieR3DSideCarIO(const string& p)
        : MovieSideCarIO(p, true)
    {
        unsigned int capabilities = MovieIO::MovieRead | MovieIO::MovieReadAudio
                                    | MovieIO::AttributeRead;

        StringPairVector video;
        StringPairVector audio;
        ParameterVector eparams;
        ParameterVector dparams;

        addType("r3d", "RED Movie", capabilities, video, audio, eparams,
                dparams);
    }

    MovieR3DSideCarIO::~MovieR3DSideCarIO() {}

    string MovieR3DSideCarIO::about() const
    {
        // DONT INCLUDE RED HEADERS HERE
        return "RED SDK";
    }

    //----------------------------------------------------------------------

    MovieQT7SideCarIO::MovieQT7SideCarIO(const string& p)
        : MovieSideCarIO(p, false)
    {
        unsigned int capabilities = MovieIO::MovieRead | MovieIO::MovieReadAudio
                                    | MovieIO::AttributeRead;

        StringPairVector video;
        StringPairVector audio;
        ParameterVector eparams;
        ParameterVector dparams;

        addType("mov", "Quicktime Movie", capabilities, video, audio, eparams,
                dparams);
        addType("qt", "Quicktime Movie", capabilities, video, audio, eparams,
                dparams);
        addType("avi", "Windows Movie", capabilities, video, audio);
        addType("mp4", "MPEG-4 Movie", capabilities, video, audio);
        addType("dv", "Digial Video (DV) Movie", capabilities, video, audio);
        addType("3gp", "3GP Phone Movie", capabilities, video, audio);
        addType("gif", MovieIO::MovieRead | MovieIO::AttributeRead);
    }

    MovieQT7SideCarIO::~MovieQT7SideCarIO() {}

    string MovieQT7SideCarIO::about() const { return "Apple Quicktime 7"; }

    //----------------------------------------------------------------------

    MovieLQTSideCarIO::MovieLQTSideCarIO(const string& p)
        : MovieSideCarIO(p, false)
    {
        unsigned int capabilities = MovieIO::MovieRead | MovieIO::MovieReadAudio
                                    | MovieIO::MovieBruteForceIO
                                    | MovieIO::AttributeRead;

        StringPairVector video;
        StringPairVector audio;
        ParameterVector eparams;
        ParameterVector dparams;

        addType("mov", "Quicktime Movie", capabilities, video, audio, eparams,
                dparams);
        addType("qt", "Quicktime Movie", capabilities, video, audio, eparams,
                dparams);
        addType("avi", "Windows Movie", capabilities, video, audio);
        addType("mp4", "MPEG-4 Movie", capabilities, video, audio);
        addType("dv", "Digial Video (DV) Movie", capabilities, video, audio);
    }

    MovieLQTSideCarIO::~MovieLQTSideCarIO() {}

    string MovieLQTSideCarIO::about() const { return "libquicktime side car"; }

} // namespace TwkMovie
