//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __MovieFB__MovieFB__h__
#define __MovieFB__MovieFB__h__
#include <TwkFB/FrameBuffer.h>
#include <TwkMovie/Movie.h>
#include <TwkMovie/MovieIO.h>
#include <TwkUtil/FileSequence.h>
#include <TwkUtil/FrameUtils.h>
#include <string>
#include <map>
#include <sys/types.h>
#include <ctime>

namespace TwkMovie
{

    //
    //  class IOMovieFB
    //
    //  Provides a sequence of TwkFB readable images + an optional audio
    //  file readable by any registered audio Movie format.
    //

    class MovieFB : public MovieReader
    {
    public:
        //
        //  Types
        //
        class FrameFile
        {
        public:
            FrameFile()
                : fileName("")
                , generation(0)
                , readTime(0)
            {
            }

            FrameFile(std::string nm)
                : fileName(nm)
                , generation(0)
                , readTime(0)
            {
            }

            FrameFile(std::string nm, int gen, std::time_t rt)
                : fileName(nm)
                , generation(gen)
                , readTime(rt)
            {
            }

            std::string fileName; //  File corresponding to frame
            int generation; //  Increases when the frame pixels are out-of-date
            std::time_t readTime; //  Time at which this file was last read
        };

        typedef std::map<int, FrameFile>
            FrameMap; // Map of frame number -> filename

        struct CompareFrameFilePair
        {
            CompareFrameFilePair() {}

            bool operator()(const FrameMap::value_type& a,
                            const FrameMap::value_type& b)
            {
                return a.first < b.first;
            }
        };

        //
        //  Constructors
        //

        MovieFB();
        ~MovieFB();

        //
        //
        //

        const std::string& imagePattern() const { return m_imagePattern; }

        //
        //  MovieReader API
        //

        virtual void
        open(const std::string& filename, const MovieInfo& as = MovieInfo(),
             const Movie::ReadRequest& request = Movie::ReadRequest());

        //
        //  Movie API
        //

        virtual void imagesAtFrame(const ReadRequest&, FrameBufferVector&);
        virtual void identifiersAtFrame(const ReadRequest&, IdentifierVector&);

        virtual Movie* clone() const;

        //
        //  Invalidate any info about which frames exist on disk
        //
        virtual void invalidateFileSystemInfo() { m_frameInfoValid = false; }

    private:
        bool fileAndIdAtFrame(int& frame, std::string&, std::ostream&, bool);
        void updateFrameInfo();
        bool getImageInfo(const std::string& filename);

    protected:
        std::string m_sequencePattern;
        std::string m_imagePattern;

        FrameMap m_frameMap;
        TwkUtil::FrameList m_frames;

        mutable pthread_mutex_t m_frameLock;

        const FrameBufferIO* m_imgio;
        TwkFB::FBInfo m_imgInfo;
        unsigned long m_filehash;
        bool m_frameInfoValid;
    };

    //
    //  IO class
    //

    class MovieFBIO : public MovieIO
    {
    public:
        MovieFBIO();
        virtual ~MovieFBIO();

        virtual std::string about() const;
        virtual MovieReader* movieReader() const;
        virtual MovieWriter* movieWriter() const;
        virtual void getMovieInfo(const std::string& filename,
                                  MovieInfo&) const;

        static int readerCount() { return m_readerCount; }

    private:
        static int m_readerCount;
    };

} // namespace TwkMovie

#endif // __MovieFB__MovieFB__h__
