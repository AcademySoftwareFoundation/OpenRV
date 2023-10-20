//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef __MovieRV__MovieRV__h__
#define __MovieRV__MovieRV__h__
#include <TwkGLFMesa/OSMesaVideoDevice.h>
#include <TwkApp/EventNode.h>
#include <TwkMovie/MovieIO.h>
#include <TwkMovie/MovieReader.h>
#include <TwkAudio/Audio.h>
#include <string>
#include <pthread.h>

namespace Rv {
class RvSession;
}

namespace TwkMovie {

/// Wrap an RvApp Session as a Movie 

///
/// A MovieReader object which uses all of MovieRV to "render" the frames
/// of an .rv file into a FrameBuffer. This can then be passed to a
/// MovieWriter object (for example) to write to disk. 
///
/// The results can be filtered like any other Movie object, but the class
/// defines a frame buffer object to render to of any resolution, so there
/// is little point in it.
///
/// MovieRV uses OSMesa to render full 32 bit float images without clamping
///

class MovieRV : public TwkMovie::MovieReader,
                public TwkApp::EventNode
{
public:
    //
    //  Types
    //

    typedef TwkGLF::OSMesaVideoDevice OSMesaVideoDevice;

    //
    //  Constructors
    //

    MovieRV();
    virtual ~MovieRV();

    //
    //  There is some synchronization required to use MovieRV with
    //  Mesa and glew. Call initThreading() before using and
    //  destroyThreading() when done.
    //

    static void initThreading();
    static void destroyThreading();

    static void uninit() {}

    //
    //  Get the OSMesaFrameBuffer
    //

    TwkGLF::OSMesaVideoDevice* glDevice() const { return m_device; }

    //
    //  MovieReader API
    //

    virtual void open(const std::string& filename, 
                      const TwkMovie::MovieInfo& as = TwkMovie::MovieInfo(),
                      const Movie::ReadRequest& request = Movie::ReadRequest());

    //
    //  Open from an existing Rv Session. If w and h are 0, the size
    //  will be the max size of the session. The MovieRV will own the
    //  session after you pass it in.
    //

    void open(Rv::RvSession* session, 
              const TwkMovie::MovieInfo& as,
              TwkAudio::ChannelsVector audioChannels,
              double audioRate = TWEAK_AUDIO_DEFAULT_SAMPLE_RATE,
              size_t audioPacketSize = 512);

    //
    //  Movie API
    //
    //  imageAtFrame() is special for a MovieRV. The passed in
    //  FrameBuffer will determine the output frame buffer for GL. So
    //  passing in an 8 bit buffer will result in direct 8 bit output
    //  from GL. 
    //

    virtual void imagesAtFrame(const ReadRequest&, FrameBufferVector&);
    virtual void identifiersAtFrame(const ReadRequest&, IdentifierVector&);
    virtual size_t audioFillBuffer(const AudioReadRequest&, AudioBuffer&);
    virtual void flush();
    virtual Movie* clone() const;
    Rv::RvSession* session() { return m_session; }

protected:
    void identifier(int frame, std::ostream&);

private:
    Rv::RvSession*            m_session;
    std::string               m_idstring;
    TwkAudio::ChannelsVector  m_audioChannels;
    double                    m_audioRate;
    size_t                    m_audioPacketSize;
    pthread_t                 m_thread;
    static OSMesaVideoDevice* m_device;
    mutable bool              m_audioInit;
};

//
//  IO class
//

class MovieRVIO : public TwkMovie::MovieIO
{
public:
    MovieRVIO();
    virtual ~MovieRVIO();

    virtual std::string about() const;
    virtual TwkMovie::MovieReader* movieReader() const;
    virtual TwkMovie::MovieWriter* movieWriter() const;
    virtual TwkMovie::MovieInfo getMovieInfo(const std::string& filename) const;
};

} // TwkMovie

#endif // __MovieRV__MovieRV__h__
