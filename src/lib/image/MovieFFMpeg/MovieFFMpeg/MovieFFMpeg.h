//******************************************************************************
// Copyright (c) 2012 Tweak Inc.
// All rights reserved.
// 
// SPDX-License-Identifier: Apache-2.0
// 
//******************************************************************************
#ifndef __MovieFFMpeg__MovieFFMpeg__h__
#define __MovieFFMpeg__MovieFFMpeg__h__
#include <TwkMovie/MovieReader.h>
#include <TwkMovie/MovieWriter.h>
#include <TwkMovie/MovieIO.h>
#include <stdint.h>

//
// AVClass Forward Declaration Placeholders
//


class AVChannelLayout;
class AVClass;
class AVCodecContext;
class AVDictionary;
class AVFormatContext;
class AVFrame;
class AVOption;
class AVOutputFormat;
class AVPacket;
class AVRational;
class AVStream;

//
// TwkMovie Forward Declaration Placeholders
//

namespace TwkMovie
{

using namespace TwkAudio;

class MediaTiming;
class ReformattingMovie;
class TimingDetails;
class AudioState;
class AudioTrack;
class VideoTrack;
class ContextPool;

//
//  class MovieFFMpegIO
//

class MovieFFMpegIO : public MovieIO
{
  public:

    //
    // Shorthand Structs
    //

    enum Format
    { 
        RGB8,
        RGBA8,
        RGB8_PLANAR,
        RGB10_A2,
        A2_BGR10,
        RGB16,
        RGBA16,
        RGB16_PLANAR
    };

    //
    // Constructors
    //

    typedef bool (*CodecFilterFunction)(std::string, bool);
    typedef std::pair<std::string,unsigned int> MFFormat;
    typedef std::map<std::string,MFFormat> MFFormatMap;

    MovieFFMpegIO(CodecFilterFunction filter,
                  bool bruteForce,
                  int codecThreads,
                  std::string language,
                  double defaultFPS,
                  void (*registerCustomCodecs)()=nullptr);

    virtual ~MovieFFMpegIO();

    //
    // Movie API
    //

    virtual std::string about() const;
    virtual MovieReader* movieReader() const;
    virtual MovieWriter* movieWriter() const;
    virtual void getMovieInfo(const std::string &filename, MovieInfo&) const;

    //
    // Public Lookup Methods
    //

    bool codecIsAllowed(std::string name, bool forRead) const;
    int bruteForce() const;
    int codecThreads() const;
    std::string language() const;
    bool allowAudioOnly() const;
    double defaultFPS() const;
    MFFormatMap getFormats() const;

  private:

    //
    // Format Output Methods
    //

    void collectParameters(const AVClass* avClass, ParameterVector* eparams,
        ParameterVector* dparams, std::string codec,
        std::string prefix);

    //
    // Data Members
    //

  private:
    CodecFilterFunction m_codecFilter;
};

//
//  class MovieFFMpegReader
//

class MovieFFMpegReader : public MovieReader
{
  public:

    //
    // Shorthand Structs
    //

    typedef std::vector<AudioTrack*>   AudioTracks;
    typedef std::vector<VideoTrack*>   VideoTracks;

    //
    //  Constructors
    //

    MovieFFMpegReader(const MovieFFMpegIO* io);
    virtual ~MovieFFMpegReader();

    //
    //  MovieReader API
    //

    virtual void open(const std::string& filename, 
                      const MovieInfo& as = MovieInfo(),
                      const Movie::ReadRequest& request = Movie::ReadRequest());
    virtual bool canConvertAudioChannels() const;    
    void close();

    //
    //  Movie API
    //

    virtual void imagesAtFrame(const ReadRequest& request, FrameBufferVector&);
    virtual void identifiersAtFrame(const ReadRequest& request,
        IdentifierVector& ids);
    virtual size_t audioFillBuffer(const AudioReadRequest&, AudioBuffer&);
    virtual MovieReader* clone() const; 
    virtual void audioConfigure(const AudioConfiguration& config);

    virtual void scan();
    float scanProgress() const { return 1.0; }
    bool needsScan() const { return false; }

  private:

    //
    // Init Methods
    //

    void initializeAll();
    void initializeVideo(int height, int width);
    void initializeAudio();
    bool openAVFormat();
    bool openAVCodec(int index, AVCodecContext** avCodecContext);
    void findStreamInfo();

    //
    // Subtitle & Language Methods
    //

    std::string streamLang(int index);
    bool correctLang(int index);
    void readSubtitle(VideoTrack* track);
    
    //
    // Metadata & Lookup Methods
    //

    void collectPlaybackTiming(std::vector<bool> heroVideoTracks,
        std::vector<bool> heroAudioTracks);
    int64_t getFirstFrame(AVRational rate);
    void snagMetadata(AVDictionary* dict, std::string source, FrameBuffer* fb);
    bool snagColr(AVCodecContext* videoCodecContext, VideoTrack* track);
    FrameBuffer::Orientation snagOrientation(VideoTrack* track);
    bool snagVideoColorInformation(VideoTrack* track);
    std::string snagVideoFrameType(VideoTrack* track);
    void addVideoFBAttrs(VideoTrack* track);
    void finishTrackFBAttrs(FrameBuffer* fb, std::string view);
    void trackFromStreamIndex(int index, VideoTrack*& vTrack, AudioTrack*& aTrack);

    //
    // Audio Methods
    //

    ChannelsVector idAudioChannels(AVChannelLayout layout, int numChannels);
    SampleTime oneTrackAudioFillBuffer(AudioTrack* track);
    int decodeAudioForBuffer(AudioTrack* track);
    template <typename T> int translateAVAudio(AudioTrack* track, double max, int offset);
    
    //
    // Video Methods
    //

    FrameBuffer* decodeImageAtFrame(int inframe, VideoTrack* track);
    FrameBuffer* configureYUVPlanes(FrameBuffer::DataType dataType, int width,
        int height, int rowSpan, int rowSpanUV, int usampling, int vsampling,
        bool addAlpha, FrameBuffer::Orientation orientation);
    void identifier(int frame, std::ostream&);

    // Seek to the requested frame and perform drain if requested.
    // This function also flush the buffers and clear the timestamp track list.
    void seekToFrame(int inframe, double frameDur, AVStream* videoStream,
        VideoTrack* track);

    // Read a packet from the video stream and updates the timestamp track
    // list if necessary.
    // Returns true if it reaches the last packet.
    bool readPacketFromStream(const int inframe, VideoTrack* track);

    // Send a packet as input to the decoder using avcodec_send_packet().
    // Note: This method can only be used with the new FFMpeg API.
    void sendPacketToDecoder(VideoTrack* track);

    // Find the image with the closest timestamp to the goal timestamp, based
    // on the requested frame number and frame duration.
    // This function uses the new FFMpeg API.
    // i.e. It uses avcodec_send_packet() and avcodec_receive_frame() rather
    //      than avcodec_decode_video2().
    bool findImageWithBestTimestamp(int inframe, double frameDur,
                                    AVStream* videoStream, VideoTrack* track);

    // check if the input format is jpeg_pipe or png_pipe
    bool isImageFormat(const char* iformat);

    //
    // Data Members
    //

    AVFormatContext*                   m_avFormatContext;
    AudioTracks                        m_audioTracks;
    VideoTracks                        m_videoTracks;
    std::map<int,int>                  m_subtitleMap;
    int                                m_timecodeTrack;
    int64_t                            m_formatStartFrame;
    TimingDetails*                     m_timingDetails;
    const MovieFFMpegIO*               m_io;
    int                                m_dbline;
    int                                m_dblline;
    bool                               m_multiTrackAudio;
    AudioState*                        m_audioState;
    bool                               m_cloning {false};
    bool                               m_mustReadFirstFrame{false};

    friend class ContextPool;
};

//
//  class MovieFFMpegWriter
//

class MovieFFMpegWriter : public MovieWriter
{
  public:

    //
    // Shorthand Structs
    //

    typedef std::vector<AudioTrack*> AudioTracks;
    typedef std::vector<VideoTrack*> VideoTracks;

    //
    // Constructors
    //

    MovieFFMpegWriter(const MovieFFMpegIO* io);
    virtual ~MovieFFMpegWriter();

    //
    // MovieWriter API
    //

    virtual MovieInfo open(const MovieInfo& info, const std::string& filename,
        const WriteRequest& request);
    virtual bool write(Movie* inMovie, const std::string& filename,
        WriteRequest& request);
    virtual bool write(Movie* inMovie);

  private:

    //
    // Init Methods
    //

    void addTrack(bool isVideo, std::string codec, 
        bool removeAppliedCodecParametersFromTheList = true);
    void addChapter(int id, int startFrame, int endFrame, std::string title);
    void collectWriteInfo(std::string videoCodec, std::string audioCodec);
    void initRefMovie(ReformattingMovie* refMovie);
    void validateCodecs(std::string* videoCodec, std::string* audioCodec);
    std::string getWriterCodec(std::string type, std::vector<std::string> guesses);

    //
    // Arg & Option Methods
    //

    bool setOption(const AVOption* opt, void* avObj, const std::string value);
    void applyCodecParameters(AVCodecContext* avCodecContext, 
        bool removeAppliedCodecParametersFromTheList = true);
    void applyFormatParameters();

    //
    // Audio Methods
    //

    void encodeAudio(AVCodecContext* audioCodecContext, AVFrame* frame, AVPacket* pkt, 
                     AVStream* audioStream, SampleTime* nsamps, int64_t lastEncAudio);
    bool fillAudio(Movie* inMovie, double overflow, bool lastPass);
    template <typename T> void translateRVAudio(int audioChannels,
        TwkAudio::AudioBuffer* audioBuffer, double max, int offset,
        bool planar);

    //
    // Video Methods
    //

    void encodeVideo(AVCodecContext* ctx, AVFrame* frame, AVPacket* pkt, AVStream* stream, int lastEncVideo);
    void fillVideo(FrameBufferVector fbs, int trackIndex, int frameIndex, bool lastPass);
    void initVideoTrack(AVStream* avStream);

    //
    // Data Members
    //

    const MovieFFMpegIO*               m_io;
    int                                m_timeScale;
    bool                               m_writeAudio;
    bool                               m_writeVideo;
    int                                m_duration;
    std::string                        m_reelName;
    std::vector<int>                   m_frames;
    bool                               m_canControlRequest;
    bool                               m_verbose;
    std::map<std::string,std::string>  m_parameters;
    const AVOutputFormat*              m_avOutputFormat;
    AVFormatContext*                   m_avFormatContext;
    SampleTime                         m_audioFrameSize;
    void*                              m_audioSamples;
    double                             m_lastAudioTime;
    AudioTracks                        m_audioTracks;
    VideoTracks                        m_videoTracks;
    int                                m_dbline;
    int                                m_dblline;
};

} // TwkMovie

#endif // __MovieFFMpeg__MovieFFMpeg__h__

