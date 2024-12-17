//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __MovieSideCar__MovieSideCarIO__h__
#define __MovieSideCar__MovieSideCarIO__h__
#include <iostream>
#include <TwkMovie/MovieIO.h>

namespace TwkMovie
{

    //
    //  SideCar IO class
    //
    //  Since the derived sidecar objects have no dependencies (other than the
    //  name of the binary) They're all defined here. E.g. the MovieR3DSideCar
    //  just defines the types of files the MovieSideCar can handle, it doesn't
    //  actually know anything about RED files.
    //

    class MovieSideCarIO : public MovieIO
    {
    public:
        MovieSideCarIO(const std::string& pathToSideCar,
                       bool cloneable = false);
        virtual ~MovieSideCarIO();

        virtual std::string about() const;
        virtual MovieReader* movieReader() const;
        virtual MovieWriter* movieWriter() const;
        virtual void getMovieInfo(const std::string& filename,
                                  MovieInfo&) const;

    private:
        std::string m_sidecarPath;
        bool m_cloneable;
    };

    //
    //  RED sidecar. This one is useful because RED's SDK isn't really
    //  reentrant or they purposefully prevent parallel execution of their
    //  code. Because the file format has no frame interdependencies we can
    //  launch a bunch of processes to do the work. This can have some
    //  significant performance gains with more than two cores.
    //

    class MovieR3DSideCarIO : public MovieSideCarIO
    {
    public:
        MovieR3DSideCarIO(const std::string& pathToSideCar);
        virtual ~MovieR3DSideCarIO();
        virtual std::string about() const;
    };

    //
    //  QT7 sidecar. This was the reason to do the sidecar in the first
    //  place. This makes it possible for win64 to talk to 32 bit QT7.
    //

    class MovieQT7SideCarIO : public MovieSideCarIO
    {
    public:
        MovieQT7SideCarIO(const std::string& pathToSideCar);
        virtual ~MovieQT7SideCarIO();
        virtual std::string about() const;
    };

    //
    //  LQT (libquicktime) sidecar is really only useful for testing
    //

    class MovieLQTSideCarIO : public MovieSideCarIO
    {
    public:
        MovieLQTSideCarIO(const std::string& pathToSideCar);
        virtual ~MovieLQTSideCarIO();
        virtual std::string about() const;
    };

} // namespace TwkMovie

#endif // __MovieSideCar__MovieSideCarIO__h__
