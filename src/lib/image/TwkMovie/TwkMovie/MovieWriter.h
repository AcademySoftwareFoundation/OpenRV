//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkMovie__MovieWriter__h__
#define __TwkMovie__MovieWriter__h__
#include <TwkMovie/Movie.h>
#include <TwkMovie/dll_defs.h>
#include <TwkAudio/Audio.h>

namespace TwkMovie
{

    /// MovieWriter is a base class for writing movie files

    ///
    /// Unfortunately, there's is very little in common between movie
    /// formats when it comes to writing a file. One might go so far as to
    /// say that the only commonality is that the writing procedures in
    /// general require random access to the input data. Fortunately, we
    /// already have a common API for that (the Movie class).
    ///
    /// To write a movie from memory without input of a Movie from a file,
    /// you need to implement a Movie class that reads from internal data
    /// structures. This should not be terribly difficult.
    ///
    /// This is a base class which can be implemented to write a specific
    /// movie format. The writer class takes a Movie as input and writes
    /// it using the given codec to the filename specified. If you want to
    /// write a sub-range of a movie, use one of the adpater Movie classes
    /// to wrap an existing movie.
    ///
    ///

    class TWKMOVIE_EXPORT MovieWriter
    {
    public:
        //
        //  Types
        //

        typedef TwkFB::FrameBuffer FrameBuffer;
        typedef std::vector<FrameBuffer*> FrameBufferVector;
        typedef Movie::WriteRequest WriteRequest;
        typedef std::vector<const FrameBuffer*> ConstFrameBufferVector;
        typedef TwkFB::FrameBuffer::DataType DataType;
        typedef TwkFB::FrameBufferIO FrameBufferIO;
        typedef FrameBufferIO::Capabilities Capabilities;
        typedef float* AudioBuffer;
        typedef std::vector<float> AudioCache;
        typedef std::vector<int> Frames;
        typedef std::pair<std::string, std::string> StringPair;
        typedef std::vector<StringPair> StringPairVector;

        MovieWriter();
        virtual ~MovieWriter();

        ///
        /// Open the writer. The MovieInfo can indicate desired dataType, number
        /// of channels, resolution, etc. The returned MovieInfo indicates to
        /// the caller what the writer wants.
        ///
        /// This function will be called once before the write() function is
        /// called. If open() is called, the writer can expect that it will be
        /// provided with framebuffers in the requested format (when it calls
        /// imagesAtFrame().
        ///
        /// The default implementation just returns what its given.
        ///
        /// NB: if open() is not called, the writer should be prepared to handle
        /// FBs in any format.
        ///

        virtual MovieInfo open(const MovieInfo& info,
                               const std::string& filename,
                               const WriteRequest& request);

        ///
        /// The thread will return from the function when the movie as
        /// finished writing.
        ///

        virtual bool write(Movie* inMovie);

        ///
        /// There are two write() APIs here. The original API below is _only_
        /// called with a request and file name. The new API above is to call
        /// _both_ open() and simple write().
        ///
        /// MovieWriters should use one or the other API.
        ///

        virtual bool write(Movie* inMovie, const std::string& filename,
                           WriteRequest&) = 0;

        static bool reallyVerbose();
        static void setReallyVerbose(bool);

        WriteRequest m_request;
        std::string m_filename;
        MovieInfo m_info;
    };

} // namespace TwkMovie

#endif // __TwkMovie__MovieWriter__h__
