//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkMovie__ReformattingMovie__h__
#define __TwkMovie__ReformattingMovie__h__
#include <TwkMovie/Movie.h>
#include <TwkMovie/ResamplingMovie.h>
#include <TwkMovie/dll_defs.h>

namespace TwkMovie
{

    /// A Movie filter which reformats images in preparation for writing

    ///
    /// Takes an existing movie object and reformates the frames according
    /// to settings. Most useful for wrapping an file movie object and
    /// handing to a MovieWriter.
    ///
    /// For audio, ReformattingMovie can deliver the audio at a given rate.
    /// If the fps of ReformattingMovie differs from the input movie, the
    /// audio will be adjusted to keep it synched.
    ///
    /// ReformattingMovie makes absolutely no guarantees about performance
    /// from frame to frame or audio sample to audio sample. Don't use
    /// this class for any real-time application. Its main use is for
    /// reformating movie data for output to a file where performance is
    /// not as important as simplicity.
    ///

    class TWKMOVIE_EXPORT ReformattingMovie : public Movie
    {
    public:
        //
        //  Types
        //

        typedef TwkFB::FrameBuffer::DataType DataType;
        typedef TwkFB::FrameBuffer::Orientation Orientation;
        typedef std::vector<std::string> ChannelMap;

        //
        //  Constructors
        //

        ReformattingMovie(Movie* inMovie);
        virtual ~ReformattingMovie();

        //
        //  FPS. This will mostly affect audio
        //

        void setFPS(float);

        //
        //  Audio rate and channels. The format is always 32 bit float.
        //

        void setAudio(float rate, size_t samples, ChannelsVector channels);

        //
        //  Time range
        //

        void setTimeRange(int start, int end);

        ///
        ///  Use fp32 for all computations
        ///

        void setUseFloatingPoint(bool b) { m_useFloat = b; }

        //
        //  Verbose
        //

        void setVerbose(bool b) { m_verbose = b; }

        ///
        ///  Convert to linear space from cineon log space on input
        ///

        void setInputLogSpace(bool b) { m_inlog = b; }

        ///
        ///  Convert to linear space from Arri logC space on input
        ///

        void setInputLogCSpace(bool b) { m_inlogc = b; }

        ///
        ///  Convert to linear space from red log space on input
        ///

        void setInputRedLogSpace(bool b) { m_inRedLog = b; }

        ///
        ///
        ///  Convert to linear space from red log film space on input
        ///

        void setInputRedLogFilmSpace(bool b) { m_inRedLogFilm = b; }

        ///  Convert to linear space from sRGB
        ///

        void setInputSRGB(bool b) { m_inSRGB = b; }

        ///
        ///  Apply gamma correction on input
        ///

        void setInputGamma(float f) { m_ingamma = f; }

        ///
        ///  (Un)Premultiply input
        ///

        void setInputPremultiply() { m_inpremult = true; }

        void setInputUnpremultiply() { m_inunpremult = true; }

        ///
        ///  Apply Relative exposure
        ///

        void setRelativeExposure(float f) { m_exposure = f; }

        ///
        ///  Scale FrameBuffer size
        ///

        void setFBScaling(float f);

        ///
        ///  Exact output resolution
        ///

        void setOutputResolution(int w, int h);

        ///
        ///  Channel remapping
        ///

        void setChannelMap(const ChannelMap& m) { m_channelMap = m; }

        //
        //  Flip/Flop
        //

        void setFlip(bool b) { m_flip = b; }

        void setFlop(bool b) { m_flop = b; }

        //
        //  Set preferred orientation
        //

        void setOutputOrientation(Orientation o) { m_orientation = o; }

        //
        //  Convert to Y RY BY
        //

        void convertToYRYBY(int Ysamples, int RYsamples, int BYsamples,
                            int Asamples = 1)
        {
            m_ysamples = Ysamples;
            m_rysamples = RYsamples;
            m_bysamples = BYsamples;
            m_asamples = Asamples;
        }

        //
        //  Convert to YUV
        //

        void convertToYUV(int Ysamples, int Usamples, int Vsamples)
        {
            m_ysamples = Ysamples;
            m_usamples = Usamples;
            m_vsamples = Vsamples;
        }

        //
        //  ACES output gamut
        //

        void setOutput709toACES(bool b) { m_output709toACES = b; }

        void setOutputWhite(float x, float y)
        {
            m_outWhiteX = x;
            m_outWhiteY = y;
        }

        //
        //  (Un)Premultiply output
        //

        void setOutputPremultiply() { m_outpremult = true; }

        void setOutputUnpremultiply() { m_outunpremult = true; }

        ///
        ///  Output in Cineon log space
        ///

        void setOutputLogSpace(bool b) { m_outlog = b; }

        ///
        ///  Output to sRGB
        ///

        void setOutputSRGB(bool b) { m_outSRGB = b; }

        ///
        ///  Output to Rec709
        ///

        void setOutputRec709(bool b) { m_outRec709 = b; }

        ///
        ///  Output to Arri LogC
        ///

        void setOutputLogC(bool b) { m_outLogC = b; }

        //
        ///
        ///  Output to Arri LogC EI (Exposure Index)
        ///

        void setOutputLogCEI(float f) { m_outLogCEI = f; }

        ///
        ///  Output in Red Log space
        ///

        void setOutputRedLog(bool b) { m_outRedLog = b; }

        ///
        ///  Output in Red Log Film space
        ///

        void setOutputRedLogFilm(bool b) { m_outRedLogFilm = b; }

        //  Output Gamma
        //

        void setOutputGamma(float f) { m_outgamma = f; }

        //
        //  Output Format
        //

        void setOutputFormat(FrameBuffer::DataType t) { m_outtype = t; }

        //
        //  Movie API
        //

        virtual bool hasAudio() const;
        virtual void imagesAtFrame(const ReadRequest& request,
                                   FrameBufferVector& fbs);
        virtual void identifiersAtFrame(const ReadRequest& request,
                                        IdentifierVector& ids);
        virtual void audioConfigure(const AudioConfiguration& conf);
        virtual size_t audioFillBuffer(const AudioReadRequest&, AudioBuffer&);
        virtual void flush();
        virtual Movie* clone() const;

    private:
        void identifier(std::ostream&);

    private:
        Movie* m_movie;
        float m_scale;
        bool m_useFloat;
        bool m_verbose;
        bool m_inlog;
        bool m_inlogc;
        bool m_inRedLog;
        bool m_inRedLogFilm;
        float m_ingamma;
        bool m_flip;
        bool m_flop;
        bool m_inpremult;
        bool m_inunpremult;
        bool m_inSRGB;
        bool m_outSRGB;
        bool m_outRec709;
        bool m_outLogC;
        float m_outLogCEI;
        bool m_outRedLog;
        bool m_outRedLogFilm;
        bool m_outpremult;
        bool m_outunpremult;
        bool m_output709toACES;
        float m_outWhiteX;
        float m_outWhiteY;
        float m_exposure;
        int m_ysamples;
        int m_rysamples;
        int m_bysamples;
        int m_usamples;
        int m_vsamples;
        int m_asamples;
        bool m_outlog;
        float m_outgamma;
        int m_xsize;
        int m_ysize;
        int m_startFrame;
        int m_endFrame;
        ChannelMap m_channelMap;
        DataType m_outtype;
        float m_fps;
        float m_audioRate;
        ChannelsVector m_audioChannels;
        size_t m_audioSamples;
        AudioBuffer m_temp0;
        AudioBuffer m_temp1;
        ResamplingMovie* m_astate;
        Orientation m_orientation;
    };

} // namespace TwkMovie

#endif // __TwkMovie__ReformattingMovie__h__
