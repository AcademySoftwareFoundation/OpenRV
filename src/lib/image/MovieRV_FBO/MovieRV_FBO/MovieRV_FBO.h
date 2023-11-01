//******************************************************************************
// Copyright (c) 2007 Tweak Inc. 
// All rights reserved.
// 
// SPDX-License-Identifier: Apache-2.0
// 
//******************************************************************************
#ifndef __RvApp__MovieRV_FBO__h__
#define __RvApp__MovieRV_FBO__h__
#include <TwkMovie/MovieReader.h>
#include <TwkMovie/MovieIO.h>
#include <TwkApp/EventNode.h>
#include <TwkAudio/Audio.h>
#include <string>

namespace TwkGLF { class FBOVideoDevice; }

namespace Rv { class RvSession; }

namespace TwkFB {
//
//  MovieRV
//
//  A MovieReader object which uses all of RvApp to "render" the
//  frames of an .rv file into a FrameBuffer. This can then be passed
//  to a MovieWriter object (for example) to write to disk.
//
//  The results can be filtered like any other Movie object, but the
//  class defines a frame buffer object to render to of any
//  resolution, so there is little point in it.
//

class MovieRV : public TwkMovie::MovieReader,
                public TwkApp::EventNode
{
  public:
    //
    //  Types
    //

    //
    //  Constructors
    //

    MovieRV();
    virtual ~MovieRV();

    Rv::RvSession* session() const { return m_session; }

    //
    //  Available after first call to imagesAtFrame
    //

    TwkGLF::GLVideoDevice* device() const { return m_device; }
    static void uninit();

    //
    //  MovieReader API
    //

    virtual void open(const std::string& filename, 
                      const TwkMovie::MovieInfo& as = TwkMovie::MovieInfo(),
                      const Movie::ReadRequest& request = Movie::ReadRequest());

    void open(Rv::RvSession*, 
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
    virtual bool audioCacheSlice();
    virtual void blockingCacheAudio();
    virtual void flush();


  protected:
    void identifier(int frame, std::ostream&);

  private:
    Rv::RvSession*                m_session;
    std::string                   m_idstring;
    static TwkGLF::GLVideoDevice* m_device;
    pthread_t                     m_thread;
    TwkAudio::ChannelsVector      m_audioChannels;
    double                        m_audioRate;
    size_t                        m_audioPacketSize;
    mutable bool                  m_audioInit;
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

} // Rv

#endif // __RvApp__MovieRV_FBO__h__
