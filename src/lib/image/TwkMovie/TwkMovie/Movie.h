//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkMovie__Movie__h__
#define __TwkMovie__Movie__h__
#include <TwkMovie/dll_defs.h>
#include <TwkAudio/Audio.h>
#include <TwkAudio/AudioFormats.h>
#include <TwkFB/FrameBuffer.h>
#include <TwkFB/IO.h>
#include <iostream>
#include <pthread.h>
#include <string>
#include <vector>

namespace TwkMovie
{

    struct TWKMOVIE_EXPORT ChapterInfo
    {
        ChapterInfo()
            : startFrame(0)
            , endFrame(0)
        {
        }

        ChapterInfo(const ChapterInfo& c)
        {
            startFrame = c.startFrame;
            endFrame = c.endFrame;
            title = c.title;
        }

        ChapterInfo& operator=(const ChapterInfo& c)
        {
            proxy.copyFrom(&c.proxy);

            startFrame = c.startFrame;
            endFrame = c.endFrame;
            title = c.title;
            return *this;
        }

        int startFrame;
        int endFrame;
        std::string title;
        TwkFB::FrameBuffer proxy; /// maybe preview pixes and image attrs
    };

    typedef std::vector<ChapterInfo> ChapterVector;
    typedef TwkAudio::ChannelsVector ChannelsVector;

    /// MovieInfo is a description of the contents of a Movie

    ///
    /// This struct is returned by info() in Movie. Each movie class
    /// should fill it with correct descriptive data. The information
    /// provides audio and image description.
    ///

    struct TWKMOVIE_EXPORT MovieInfo : public TwkFB::FBInfo
    {
        MovieInfo()
            : FBInfo()
            , start(1)
            , end(0)
            , inc(0)
            , fps(0.0)
            , quality(1.0f)
            , audio(false)
            , audioSampleRate(0)
            , video(true)
            , slowRandomAccess(false)
            , privateData(0)
            , hasSlate(false)
        {
        }

        MovieInfo(const MovieInfo& i)
            : TwkFB::FBInfo(i)
        {
            video = i.video;
            start = i.start;
            end = i.end;
            inc = i.inc;
            fps = i.fps;
            quality = i.quality;
            audio = i.audio;
            audioSampleRate = i.audioSampleRate;
            audioChannels = i.audioChannels;
            slowRandomAccess = i.slowRandomAccess;
            privateData = i.privateData;
            audioLanguages = i.audioLanguages;
            textLanguages = i.textLanguages;
            chapters = i.chapters;
            slate = i.slate;
            hasSlate = i.hasSlate;
        }

        MovieInfo& operator=(const TwkFB::FBInfo& i)
        {
            TwkFB::FBInfo::operator=(i);

            privateData = 0;
            return *this;
        }

        MovieInfo& operator=(const MovieInfo& i)
        {
            TwkFB::FBInfo::operator=(i);

            video = i.video;
            start = i.start;
            end = i.end;
            inc = i.inc;
            fps = i.fps;
            quality = i.quality;
            audio = i.audio;
            audioSampleRate = i.audioSampleRate;
            audioChannels = i.audioChannels;
            slowRandomAccess = i.slowRandomAccess;
            privateData = i.privateData;
            audioLanguages = i.audioLanguages;
            textLanguages = i.textLanguages;
            chapters = i.chapters;
            hasSlate = i.hasSlate;
            slate = i.slate;
            return *this;
        }

        bool video;             /// true if movie produces images
        int start;              /// frame start of images
        int end;                /// frame end of images
        int inc;                /// frames between images
        float fps;              /// frames per second
        float quality;          /// unused
        bool audio;             /// true if movie produces audio
        double audioSampleRate; /// sample rate of returned audio
        bool
            slowRandomAccess; /// if true random (non-sequential) access is slow
        StringVector audioLanguages;  /// for choosing an audio track
        StringVector textLanguages;   /// for choosing a text track
        ChapterVector chapters;       /// chapter info if the movie has chapters
        ChannelsVector audioChannels; /// ordered vector of audio channels
        void* privateData;
        bool hasSlate;
        FBInfo slate; /// Slate Info
    };

    /// The Movie class is a base class for a Reader or Filter for images and
    /// audio.

    ///
    /// The Movie class provides two main APIs: one for getting one or
    /// more images at a specific frame, and one for filling an audio
    /// buffer at a specific time.
    ///
    /// A single Movie may have multiple views and/or multiple layers of
    /// images. The caller may request some combonation of these or may
    /// request that a stereo pair be produced (if available).
    ///
    /// Audio is returned in the native sampling rate, but must be float
    /// point samples. Filters and/or down stream code are responsible for
    /// getting the sampling rate correct for a given audio renderer.
    ///

    class TWKMOVIE_EXPORT Movie
    {
    public:
        //@{
        ///  Types
        ///

        typedef std::vector<float> SampleVector;
        typedef TwkAudio::Time Time;
        typedef TwkFB::FrameBuffer FrameBuffer;
        typedef TwkFB::FrameBuffer::DataType DataType;
        typedef TwkFB::FrameBufferIO FrameBufferIO;
        typedef FrameBufferIO::Capabilities Capabilities;
        typedef std::vector<int> Frames;
        typedef std::vector<FrameBuffer*> FrameBufferVector;
        typedef std::string Identifier;
        typedef std::vector<Identifier> IdentifierVector;
        typedef std::vector<std::string> StringVector;
        typedef TwkAudio::AudioBuffer AudioBuffer;

        //@}

        /// Passed as a parameter when asking for images from a Movie

        ///
        /// The ReadRequest object is the parameter used to obtain images
        /// at a specific frame. Additional parameteric information may be
        /// added to this struct over time.
        ///
        /// If stereo is true and the Movie has stereo views, it should
        /// return only those views. If stereo is true and specific views
        /// are requested, these should be returned as the stereo views
        /// (as appropriate).
        ///
        /// If missing is true, the request was already tried and failed
        /// (because of an exception). This is assumed to mean that the frame
        /// is missing or unreadable. When missing is true the imagesAtFrame()
        /// is expected to provide *something* to view (usually a hold of a
        /// nearby frame) or to throw again indicating that its a no-go. For
        /// identifiersAtFrame() a substitute identifier should be returned.
        /// Note that if you ignore missing the code will still work, but a
        /// blank error frame will be shown.
        ///
        /// If specific views or layers are asked for, the Movie should
        /// return those views/layers if the exist.
        ///
        ///

        struct ReadRequest : public TwkFB::FrameBufferIO::ReadRequest
        {
            ReadRequest(int frame_ = 0, bool stereo_ = false)
                : TwkFB::FrameBufferIO::ReadRequest()
                , frame(frame_)
                , stereo(stereo_)
                , missing(false)
            {
            }

            int frame;    /// the frame at which to get images
            bool stereo;  /// indicates a stereo context
            bool missing; /// the frame failed -- provide a stand-in
        };

        ///
        /// The WriteRequest struct is built off of the
        /// TwkFB::FrameBufferIO::WriteRequest struct. In addition to
        /// image format information, the request indicates audio
        /// parameters and time based information for image writing.
        ///
        /// The TwkFB::FrameBufferIO::WriteRequest also has an "args"
        /// field (StringVector) which may contain writer specific
        /// arguments.
        ///

        struct WriteRequest : public TwkFB::FrameBufferIO::WriteRequest
        {
            WriteRequest()
                : TwkFB::FrameBufferIO::WriteRequest()
                , audioQuality(1.0)
                , audioRate(TWEAK_AUDIO_DEFAULT_SAMPLE_RATE)
                , audioChannels(2)
                , timeRangeOverride(false)
                , fps(0)
                , threads(0)
                , stereo(false)
                , verbose(false)
            {
            }

            std::string codec;      /// use codec when writing
            std::string audioCodec; /// use codec when writing
            float audioQuality;     /// general quality knob (0->1)
            size_t audioChannels;   /// defaults to 2
            double audioRate; /// defaults to TWEAK_AUDIO_DEFAULT_SAMPLE_RATE
            bool timeRangeOverride; /// override input movie time range
            Frames frames;          /// if override is true, output these frames
            size_t threads;         /// could be 0 (not specified)
            float fps;              /// if override is true, fps
            bool verbose;           /// output % done, etc
            StringVector views;     /// which views to write (empty=all)
            bool stereo;            /// output stereo movie
            std::string comments;   /// additional output comments
            std::string copyright;  /// output copyright
        };

        /// AudioReadRequest is the parameter used to get audio data from a
        /// movie

        ///
        /// The AudioReadRequest struct holds all of the parameters needed
        /// to request audio data. Additional fields may be added in the
        /// future.
        ///
        /// All of the request parameters are in Time since the Movie is
        /// allowed to return audio data at any rate it chooses. (Ideally,
        /// the movie returns data at the requested rate when possible).
        ///

        struct AudioReadRequest
        {
            AudioReadRequest(Time start_, Time duration_, size_t margin_ = 0)
                : startTime(start_)
                , duration(duration_)
                , margin(margin_)
            {
            }

            Time
                startTime; /// The start time relative to the movie being called
            Time duration; /// The amount of data requested
            size_t margin; /// margin in samples
            std::string audioLanguage;
        };

        struct AudioConfiguration
        {
            AudioConfiguration()
                : rate(0.0)
                , layout(TwkAudio::Stereo_2)
                , bufferSize(0)
            {
            }

            AudioConfiguration(TwkAudio::Time deviceRate,
                               TwkAudio::Layout chanFormat, size_t numSamples)
                : rate(deviceRate)
                , layout(chanFormat)
                , bufferSize(numSamples)
            {
            }

            bool operator==(const AudioConfiguration& b) const
            {
                return layout == b.layout && rate == b.rate
                       && bufferSize == b.bufferSize;
            }

            TwkAudio::Layout layout;
            TwkAudio::Time rate;
            size_t bufferSize;
        };

        //
        //  Constructors
        //

        Movie();
        virtual ~Movie();

        ///
        /// The MovieInfo struct should be filled out by the Movie class
        /// when opened. Once it is filled in it should not change.
        ///

        const MovieInfo& info() const { return m_info; }

        ///
        /// A Movie is thread safe only when it is reentrant and can
        /// handle multiple threads. This is highly unlikely, so don't
        /// make this true unless you really know what you're doing.
        ///

        bool isThreadSafe() const { return m_threadSafe; }

        ///
        /// This is a hint that audio is completely separate from
        /// imagery. For example, this usually isn't true of most movie
        /// file formats. If you movie reader handles audio as a separate
        /// file system node (somehow) than this might be true.
        ///

        bool isAudioAsync() const { return m_audioAsync; }

        //
        //  Movie API.
        //

        virtual bool hasAudio() const; /// returns value from MovieInfo
        virtual bool hasVideo() const; /// returns value from MovieInfo
        virtual bool canConvertAudioRate() const;     /// defaults to false
        virtual bool canConvertAudioChannels() const; /// defaults to false

        ///
        ///  Get frame(s), if open for reading, it may restructure the
        ///  FBs. This is usually the main thread calling this function,
        ///  but there are no guarantees.
        ///

        virtual void imagesAtFrame(const ReadRequest&, FrameBufferVector& fbs);

        ///
        ///  Identifier at frame constructs the identifier string for a
        ///  given frame of the movie. This is the same identifier that
        ///  imageAtFrame() should put on the FB. Output the identifier to
        ///  the ostream.
        ///

        virtual void identifiersAtFrame(const ReadRequest&, IdentifierVector&);

        ///
        ///  In the case of a movie using another movie objects and
        ///  assembling them, its useful to just get the attributes.  This
        ///  is especially useful with audio (using an audio movie to
        ///  create a sound track for example).
        ///

        virtual void attributesAtFrame(const ReadRequest&,
                                       FrameBufferVector& fbs);

        ///
        ///  Fill a buffer with audio data. This is the lowest level
        ///  accessor function for audio. Fill the buffer to the requested
        ///  size or less. Returns the number of samples actually
        ///  provided. This function will most likely be called by a
        ///  worker thread that is independent of the thread that calls
        ///  imageAtFrame() so make sure its all thread safe.
        ///
        ///  The AudioBuffer should be filled with interleaved samples
        ///  where each audio frame contains numChannels floats.
        ///
        ///  The function should return the number of non-margin samples
        ///  read. Do not include the margins (if there are any) in the
        ///  return value.
        ///

        virtual size_t audioFillBuffer(const AudioReadRequest&, AudioBuffer&);

        ///
        /// Called prior to first call to audioFillBuffer(). May be called
        /// at any time after as well. This function indicates that future
        /// calls to audioFillBuffer() will have the indicated rate and
        /// channels.
        ///
        /// Note that the only way to guarantee that audioFillBuffer() is
        /// not called while a thread is in audioConfigure() is to
        /// implement a lock in the derived class.
        ///

        virtual void audioConfigure(const AudioConfiguration&);

        ///
        ///  Flush any internal caching. (Assume the disk data has
        ///  changed). This function may not do anything.
        ///

        virtual void flush();

        ///
        /// clone the movie object. If it references external resources
        /// (like a file) open the file too. This is used to create
        /// multiple input points for multi-threaded readers
        ///

        virtual Movie* clone() const;

        ///
        /// This is a convenience function for MovieWriter
        /// subclasses. Given an audio buffer and a set of frames, this
        /// function will assemble the audio for those frame from this
        /// movie as a continuous audio stream. The buffer will be in the
        /// movie's native audio rate.
        ///

        void cacheAudioForFrames(AudioBuffer& buffer, const Frames& frames);

        //
        //  Invalidate any cached info about directory contents, modification
        //  times, file existance, etc held by this Movie.
        //

        virtual void invalidateFileSystemInfo() { return; }

        //
        //  "values" are named fields in the derived classes. Since we
        //  can't look into their headers but we know they exist use this
        //  dynamic API to get at them
        //

        virtual bool getBoolAttribute(const std::string& name) const;
        virtual void setBoolAttribute(const std::string& name, bool value);
        virtual int getIntAttribute(const std::string& name) const;
        virtual void setIntAttribute(const std::string& name, int value);
        virtual std::string getStringAttribute(const std::string& name) const;
        virtual void setStringAttribute(const std::string& name,
                                        const std::string& value);
        virtual double getDoubleAttribute(const std::string& name) const;
        virtual void setDoubleAttribute(const std::string& name,
                                        double value) const;

    private:
        Movie(const Movie& copy) {} /// Copy forbidden

        Movie& operator=(const Movie& copy) { return *this; }; /// = forbidden

    protected:
        MovieInfo m_info;  /// to be filled in by derived class
        bool m_threadSafe; /// to be filled in by derived class
        bool m_audioAsync; /// to be filled in by derived class
    };

} // namespace TwkMovie

#endif // __TwkMovie__Movie__h__
