//******************************************************************************
// Copyright (c) 2006 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __MovieProcedural__MovieProcedural__h__
#define __MovieProcedural__MovieProcedural__h__
#include <string>
#include <sys/types.h>
#include <TwkFB/FrameBuffer.h>
#include <TwkMovie/MovieReader.h>
#include <TwkMovie/MovieIO.h>
#include <TwkUtil/Clock.h>

namespace TwkMovie
{

    //
    //  class IOMovieProcedural
    //
    //  Generates "canned" movies. The type of movie is a product of the
    //  "filename" of the movie. For example:
    //
    //      solid,red=0,green=0,blue=0,alpha=0start=1,end=1,inc=1,fps=24,depth=16f.movieproc
    //      smptebars,start=1,end=1000,audio=sine,freq=440.movieproc
    //
    //  image types are
    //  ---------------
    //  solid       - solid color
    //  black       - solid color (black)
    //  grey        - solid color (grey)
    //  white       - solid color (white)
    //  smptebars   - smpte color bars
    //  hramp       - horizonal black ramp
    //  hbramp      - horizonal black ramp
    //  vbramp      - vertical black ramp
    //  hwramp      - horizonal white ramp
    //  vwramp      - vertical white ramp
    //  srgbcolorchart  - SRGB macbeth chart
    //  acescolorchart  - ACES macbeth chart
    //  noise       - noise (turbulence) image
    //
    //  audio types are:
    //  ----------------
    //  sine        - sine wave
    //  sync        - periodic chirp
    //
    //  name values are:
    //  ---------------
    //  start   int             - start frame
    //  end     int             - end frame
    //  fps     float           - frame rate
    //  inc     int             - frame increment
    //  red     float           - red component (used by solid)
    //  green   float           - green component (used by solid)
    //  blue    float           - blue component (used by solid)
    //  alpha   float           - alpha component (used by solid)
    //  width   int             - image width
    //  height  int             - image height
    //  depth   int + i or f    - channel bit depth (float or int)
    //  audio   type            - audio type
    //  freq    float           - frequency for audio
    //  amp     float           - amplitude for audio
    //  rate    int             - audio rate in samples/second
    //  hpan    int             - horizonal pan pixels/frame
    //  flash                   - flash solid color frame every 'interval' secs.
    //  interval float          - interval in seconds; for use in flash
    //  filename base64string   - filename to spoof
    //  attr:NAME string        - sets attr NAME=string
    //
    //  Also
    //  ----
    //  480pFPS
    //  720pFPS
    //  1080pFPS
    //
    //  where the FPS is optional. E.g. grey,720p24.movieproc
    //

    class MovieProcedural : public MovieReader
    {
    public:
        typedef TwkFB::FrameBuffer FrameBuffer;
        typedef unsigned char Byte;

        //
        //  Constructors
        //

        MovieProcedural();
        ~MovieProcedural();

        //
        //  MovieReader API
        //

        virtual void
        open(const std::string& filename, const MovieInfo& as = MovieInfo(),
             const Movie::ReadRequest& request = Movie::ReadRequest());

        //
        //  Movie API

        virtual bool hasAudio() const;
        virtual bool canConvertAudioRate() const;
        virtual void imagesAtFrame(const ReadRequest& request,
                                   FrameBufferVector&);
        virtual void identifiersAtFrame(const ReadRequest& request,
                                        IdentifierVector&);
        virtual size_t audioFillBuffer(const AudioReadRequest&, AudioBuffer&);
        virtual void flush();

        //
        //  MovieProcedural API
        //

        void setErrorMessage(const std::string& s) { m_errorMessage = s; }

        void setFilename(const std::string& s) { m_filename = s; }

    private:
        void identifier(int frame, std::ostream&);
        void renderNoise(FrameBuffer&);
        void renderImage(const std::string&, FrameBuffer&);
        void renderSolid(FrameBuffer&, float r, float g, float b, float a);
        void renderHRamp(FrameBuffer&, float r0, float g0, float b0, float a0,
                         float r1, float g1, float b1, float a1);
        void renderSMPTEBars(FrameBuffer&);
        void renderSRGBMacbethColorChart(FrameBuffer&);
        void renderACESMacbethColorChart(FrameBuffer&);

    private:
        FrameBuffer m_img;
        FrameBuffer m_flashImg;
        std::string m_imageType;
        std::string m_audioType;
        std::string m_errorMessage;
        float m_red;
        float m_green;
        float m_blue;
        float m_alpha;
        float m_audioFreq;
        float m_audioAmp;
        int m_hpan;
        bool m_flash;
        float m_interval;
        TwkUtil::SystemClock::Time m_errorStartTime{0};
    };

    //
    //  IO class
    //

    class MovieProceduralIO : public MovieIO
    {
    public:
        MovieProceduralIO();
        virtual ~MovieProceduralIO();

        virtual std::string about() const;
        virtual MovieReader* movieReader() const;
        virtual MovieWriter* movieWriter() const;
        virtual void getMovieInfo(const std::string& filename,
                                  MovieInfo&) const;
    };

} // namespace TwkMovie

#endif // __MovieProcedural__MovieProcedural__h__
