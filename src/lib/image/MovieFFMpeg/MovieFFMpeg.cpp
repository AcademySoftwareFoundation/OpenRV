//******************************************************************************
// Copyright (c) 2012 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <MovieFFMpeg/MovieFFMpeg.h>
#include <TwkFB/Operations.h>
#include <TwkExc/Exception.h>
#include <TwkMovie/Exception.h>
#include <TwkMovie/Movie.h>
#include <TwkMovie/ReformattingMovie.h>
#include <TwkAudio/Audio.h>
#include <TwkAudio/Interlace.h>
#include <TwkFB/FastMemcpy.h>
#include <TwkUtil/EnvVar.h>
#include <TwkUtil/Timer.h>
#include <TwkUtil/PathConform.h>
#include <TwkUtil/File.h>
#include <TwkUtil/sgcHop.h>
#include <iostream>
#include <iomanip>
#include <math.h>
#include <algorithm>
#include <array>
#include <stdlib.h>
#include <stl_ext/stl_ext_algo.h>
#include <stl_ext/string_algo.h>
#include <string>
#include <set>
#include <limits>
#include <cmath>
#include <boost/filesystem.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/algorithm/string.hpp>
#include <mp4v2Utils/mp4v2Utils.h>
#include <string>
#include <cstring>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libavutil/timecode.h>
#include <libavutil/display.h>
#include <libswscale/swscale.h>
    // #include <libavcodec/ass_split.h>
}

static ENVVAR_BOOL(evUseUploadedMovieForStreaming,
                   "RV_SHOTGRID_USE_UPLOADED_MOVIE_FOR_STREAMING", false);

namespace TwkMovie
{

    using namespace std;
    using namespace TwkUtil;
    using namespace TwkFB;
    using namespace TwkMovie;
    using namespace TwkAudio;

#define TWK_AVFORMAT_PROBESIZE UINT_MAX
#define RV_OUTPUT_FFMPEG_FMT AV_PIX_FMT_RGBA64
#define FPS_PRECISION_LIMIT 1000 // 5 sig figs
#define RV_OUTPUT_VIDEO_CODEC "mjpeg"
#define RV_OUTPUT_AUDIO_CODEC "pcm_s16be"

#if 0
#define DB_LOG_LEVEL AV_LOG_ERROR
#define DB_GENERAL 0x001
#define DB_VIDEO 0x002
#define DB_AUDIO 0x004
#define DB_METADATA 0x008
#define DB_AUDIO_SAMPLES 0x010
#define DB_TIMING 0x020
#define DB_WRITE 0x040
#define DB_DURATION 0x080
#define DB_SUBTITLES 0x100
#define DB_TIMESTAMPS 0x200
#define DB_MOST 0x3ef
#define DB_ALL 0xfff
#define DB_LEVEL (DB_GENERAL | DB_MOST)

#define DB(x)                                                                  \
    if (DB_GENERAL & DB_LEVEL)                                                 \
    {                                                                          \
        stringstream msg;                                                      \
        msg << "INFO [" << this << "," << setw(7) << setfill('0')              \
            << this->m_dbline++ << setw(0) << "]: MovieFFMpeg: " << x << endl; \
        cerr << msg.str();                                                     \
    }

#define DBL(level, x)                                             \
    if (level & DB_LEVEL)                                         \
    {                                                             \
        stringstream msg;                                         \
        msg << "INFO [" << this << "," << setw(7) << setfill('0') \
            << this->m_dbline++ << setw(0) << "," << level        \
            << "]: MovieFFMpeg: " << x << endl;                   \
        cerr << msg.str();                                        \
    }

#else
#define DB_LOG_LEVEL AV_LOG_WARNING
#define DB_LEVEL 0x0
#define DB(x)
#define DBL(level, x)
#endif

    //----------------------------------------------------------------------
    //
    // Helper Structs & Classes
    //
    //----------------------------------------------------------------------

    //
    // The MovieTimer and TimingDetails debugging helper classes that are used
    // to keep track of time spent in various parts of the seeking and decoding
    // loops.
    //

    struct MovieTimer
    {
        MovieTimer()
            : duration(0)
            , runs(0)
        {
            timer = new TwkUtil::Timer();
        };

        ~MovieTimer() { delete timer; };

        void start() { timer->start(); };

        double stop()
        {
            duration += timer->elapsed();
            runs++;
            return timer->elapsed();
        }

        TwkUtil::Timer* timer;
        double duration;
        int runs;
    };

    struct TimingDetails
    {
        TimingDetails() {};

        ~TimingDetails()
        {
            map<string, MovieTimer*>::iterator timerItr;
            for (timerItr = m_timers.begin(); timerItr != m_timers.end();
                 timerItr++)
            {
                delete m_timers[timerItr->first];
            }
        };

        void startTimer(string name)
        {
            if (m_timers.find(name) == m_timers.end())
            {
                m_timers[name] = new MovieTimer();
            }
            m_timers[name]->start();
        };

        double pauseTimer(string name) { return m_timers[name]->stop(); };

        string summary()
        {
            ostringstream summary;
            summary << "timing summary (time/runs = avg) ";
            map<string, MovieTimer*>::iterator timerItr;
            for (timerItr = m_timers.begin(); timerItr != m_timers.end();
                 timerItr++)
            {
                string name = timerItr->first;
                summary << name << ": " << m_timers[name]->duration << " / "
                        << m_timers[name]->runs << " = "
                        << m_timers[name]->duration
                               / float(m_timers[name]->runs)
                        << " ";
            }
            return summary.str();
        };

        map<string, MovieTimer*> m_timers;
    };

    struct AudioState
    {
        AudioState() { layout = UnknownLayout; };

        ~AudioState() {};

        ChannelsMap chmap;
        ChannelsVector channels;
        int channelsPerTrack;
        Layout layout;
    };

    //
    // AudioTrack and VideoTrack are used by both MovieFFMpegReader and
    // MovieFFMpegWriter to store additional information about the AVStreams in
    // each file. These objects contain special AVClasses that are used to both
    // decode and encode as appropriate for reading and writing.
    //

    struct AudioTrack
    {
        AudioTrack()
            : audioPacket(0)
            , audioFrame(0)
            , lastDecodedAudio(AV_NOPTS_VALUE)
            , lastEncodedAudio(0)
            , bufferStart(AV_NOPTS_VALUE)
            , bufferEnd(AV_NOPTS_VALUE)
            , bufferLength(0)
            , isOpen(false)
            , numChannels(0)
            , start(0)
            , desired(0)
            , bufferPointer(0)
            , avCodecContext(0)
        {
            audioFrame = av_frame_alloc();
            audioPacket = av_packet_alloc();
        };

        ~AudioTrack()
        {
            if (audioPacket)
                av_packet_free(&audioPacket);
            if (audioFrame)
                av_frame_free(&audioFrame);
        };

        AudioTrack& initFrom(const AudioTrack* t)
        {
            number = t->number;
            numChannels = t->numChannels;
            return *this;
        }

        int number;
        int numChannels;
        bool isOpen;
        int64_t lastDecodedAudio;
        int64_t lastEncodedAudio;
        int64_t bufferStart;
        int64_t bufferEnd;
        int64_t bufferLength;
        AVPacket* audioPacket;
        AVFrame* audioFrame;
        SampleTime start;
        SampleTime desired;
        float* bufferPointer;
        AVCodecContext* avCodecContext;
    };

    struct VideoTrack
    {
        VideoTrack()
            : videoPacket(0)
            , videoFrame(0)
            , lastDecodedVideo(-1)
            , lastEncodedVideo(0)
            , imgConvertContext(0)
            , isOpen(false)
            , rotate(false)
            , colrType("")
            , avCodecContext(0)
        {
            videoFrame = av_frame_alloc();
            videoPacket = av_packet_alloc();
            inPicture = av_frame_alloc();
            outPicture = av_frame_alloc();
        };

        ~VideoTrack()
        {
            //
            //  Be paranoid: get rid of these pointers that were borrowed
            //  from AVFrame. ffmpeg doesn't touch them on deletion, but
            //  it also doesn't say it won't
            //

            for (size_t i = 0; i < AV_NUM_DATA_POINTERS; i++)
            {
                videoFrame->data[i] = 0;
                videoFrame->linesize[i] = 0;
            }

            if (imgConvertContext)
                sws_freeContext(imgConvertContext);
            if (videoPacket)
                av_packet_free(&videoPacket);
            if (videoFrame)
                av_frame_free(&videoFrame);
            av_frame_free(&inPicture);
            av_frame_free(&outPicture);
        };

        VideoTrack& initFrom(const VideoTrack* t)
        {
            fb.copyFrom(&t->fb);

            name = t->name;
            number = t->number;
            rotate = t->rotate;
            return *this;
        }

        string name;
        int number;
        bool isOpen;
        bool rotate;
        int lastDecodedVideo;
        int lastEncodedVideo;
        set<int64_t> tsSet;
        FrameBuffer fb;
        struct SwsContext* imgConvertContext;
        AVPacket* videoPacket;
        AVFrame* videoFrame;
        AVFrame* inPicture;
        AVFrame* outPicture;
        string colrType;
        AVCodecContext* avCodecContext;
    };

    //
    //  Manage limited pool of open contexts.
    //
    //  ContextPool does not open codecs, but does close them if a Reservation
    //  object is requested, the requested context is closed, and the
    //  ContextPool size is at it's max.
    //

    class ContextPool
    {
    public:
        ContextPool(int poolSize)
            : m_maxOpenThreads(poolSize)
            , m_currentOpenThreads(0)
        {
        }

    private:
        //
        //  Wrapper for AV codec context
        //

        struct Context
        {
            Context()
                : reader(0)
                , streamIndex(-1)
                , avContext(0)
                , vTrack(0)
                , aTrack(0)
                , reserved(false)
                , inOpenList(false) {};

            MovieFFMpegReader* reader;
            int streamIndex;
            AVCodecContext* avContext;
            VideoTrack* vTrack;
            AudioTrack* aTrack;

            std::list<Context*>::iterator listIterator;

            bool reserved;
            bool inOpenList;
        };

    public:
        //
        //  State/lock object for reserved contexts.  Contexts are reserved by
        //  MovieFFMpeg during use, cannot be closed while reserved.
        //

        class Reservation
        {
        public:
            Reservation(MovieFFMpegReader* reader, int streamIndex);
            ~Reservation();

        private:
            Context* m_context;
            int m_dbline;
            int m_dblline;
        };

        //
        //  MovieFFMpeg calls flushContext() when it is going to close the
        //  context itself.
        //

        static void flushContext(MovieFFMpegReader* reader, int streamIndex);

    private:
        typedef std::pair<MovieFFMpegReader*, int> ContextKey;
        typedef std::map<ContextKey, Context> ContextMap;
        typedef std::list<Context*> ContextList;
        typedef boost::mutex Mutex;
        typedef boost::lock_guard<Mutex> LockGuard;

        ContextMap m_contextMap;
        ContextList m_openContexts;
        Mutex m_mutex;
        int m_maxOpenThreads;
        int m_currentOpenThreads;
    };

    //
    //  Global pool object:
    //

    ContextPool* globalContextPool = 0;

    void ContextPool::flushContext(MovieFFMpegReader* reader, int streamIndex)
    {
        if (!globalContextPool)
            return;

        ContextPool& gcp = *globalContextPool;

        LockGuard lock(gcp.m_mutex);

        ContextMap::iterator i =
            gcp.m_contextMap.find(ContextKey(reader, streamIndex));

        if (i == gcp.m_contextMap.end())
            return;

        Context& context = i->second;

        if (context.inOpenList)
        {
            gcp.m_openContexts.erase(context.listIterator);
            gcp.m_currentOpenThreads -= context.avContext->thread_count;
        }

        gcp.m_contextMap.erase(i);
    }

    ContextPool::Reservation::Reservation(MovieFFMpegReader* reader,
                                          int streamIndex)
        : m_context(0)
        , m_dbline(0)
        , m_dblline(0)
    {
        if (!globalContextPool)
            return;

        ContextPool& gcp = *globalContextPool;

        LockGuard lock(gcp.m_mutex);

        //
        //  Look up Context object, possibly creating an empty one at this
        //  point.
        //

        Context& context = gcp.m_contextMap[ContextKey(reader, streamIndex)];
        m_context = &context;

        context.reserved = true;
        context.reader = reader;
        context.streamIndex = streamIndex;

        //
        //  If it's already in the list move it to the front now.
        //

        if (context.inOpenList)
        {
            gcp.m_openContexts.erase(context.listIterator);
            gcp.m_openContexts.push_front(&context);
            context.listIterator = gcp.m_openContexts.begin();
        }

        //
        //  Make sure there is room in the list, in case we are about to open
        //  this context.
        //

        while (gcp.m_currentOpenThreads >= gcp.m_maxOpenThreads)
        {
            Context& closeContext = *gcp.m_openContexts.back();
            gcp.m_openContexts.pop_back();

            closeContext.inOpenList = false;

            if (closeContext.reserved)
            {
                //
                //  XXX Should never happen since reserved Contexts get pushed
                //  to front of list, but how do we ensure it never happens ?
                //

                cout << "ERROR: Attempted to reuse reserved context! ("
                     << closeContext.reader->filename() << ")" << endl;
            }
            else if (closeContext.avContext)
            {
                DB("closing " << closeContext.reader << " "
                              << closeContext.reader->filename() << ", stream "
                              << closeContext.streamIndex << ", threads "
                              << closeContext.avContext->thread_count << endl);

                gcp.m_currentOpenThreads -=
                    closeContext.avContext->thread_count;

                avcodec_free_context(&closeContext.avContext);

                if (closeContext.vTrack)
                    closeContext.vTrack->isOpen = false;
                if (closeContext.aTrack)
                    closeContext.aTrack->isOpen = false;
            }
        }
    }

    ContextPool::Reservation::~Reservation()
    {
        if (!globalContextPool)
            return;

        ContextPool& gcp = *globalContextPool;

        LockGuard lock(gcp.m_mutex);

        Context& context = *m_context;

        context.reserved = false;

        //
        //  Make sure the context still exists and we aren't closing.
        //

        ContextKey key(context.reader, context.streamIndex);
        if (gcp.m_contextMap.find(key) == gcp.m_contextMap.end())
            return;

        //
        //  If this is the first time we've encountered this Context, it's only
        //  now that it corresponds to an actual AVCodecContext, so look that up
        //  and add to Context struct.  Also find and remember corresponding
        //  Track.
        //

        if (!context.avContext)
        {
            AVStream* avStream =
                context.reader->m_avFormatContext->streams[context.streamIndex];

            context.reader->trackFromStreamIndex(
                context.streamIndex, context.vTrack, context.aTrack);
            if (context.vTrack)
            {
                context.avContext = context.vTrack->avCodecContext;
            }
            else if (context.aTrack)
            {
                context.avContext = context.aTrack->avCodecContext;
            }
        }

        //
        //  If the context is not open at this point, something went wrong.
        //  Otherwise we want to push it to the front of the open list, adding
        //  it first if necessary.
        //

        if (!context.avContext)
        {
            if (context.inOpenList)
            {
                gcp.m_openContexts.erase(context.listIterator);
                context.inOpenList = false;
            }
        }
        else if (gcp.m_openContexts.empty()
                 || gcp.m_openContexts.front() != &context)
        {
            if (context.inOpenList)
            //
            //  We're going to add it to the front of the list, so remove it
            //  from it's current location.
            //
            {
                gcp.m_openContexts.erase(context.listIterator);
            }
            else
            //
            //  It's not in the list, so it's threads are not accounted for yet
            //  in global thread count, so do that.
            //
            {
                gcp.m_currentOpenThreads += context.avContext->thread_count;
            }

            gcp.m_openContexts.push_front(&context);

            context.inOpenList = true;
            context.listIterator = gcp.m_openContexts.begin();
        }
    }

    namespace
    {

        //----------------------------------------------------------------------
        //
        // Static Lookups
        //
        //----------------------------------------------------------------------

        //
        //  Put anything we know about in here: some of these we don't
        //  actually support. But just in case ....
        //

        const char* slowRandomAccessCodecsArray[] = {"3iv2",
                                                     "3ivd",
                                                     "ap41",
                                                     "avc1",
                                                     "div1",
                                                     "div2",
                                                     "div3",
                                                     "div4",
                                                     "div5",
                                                     "div6",
                                                     "divx",
                                                     "dnxhd",
                                                     "dx50",
                                                     "h263",
                                                     "h264",
                                                     "i263",
                                                     "iv31",
                                                     "iv32",
                                                     "m4s2",
                                                     "mp42",
                                                     "mp43",
                                                     "mp4s",
                                                     "mp4v",
                                                     "mpeg4",
                                                     "mpg1",
                                                     "mpg3",
                                                     "mpg4",
                                                     "pim1",
                                                     "s263",
                                                     "svq1",
                                                     "svq3",
                                                     "u263",
                                                     "vc1",
                                                     "vc1_vdpau",
                                                     "vc1image",
                                                     "viv1",
                                                     "wmv3",
                                                     "wmv3_vdpau",
                                                     "wmv3image",
                                                     "xith",
                                                     "xvid",
                                                     "libdav1d",
                                                     0};

        const char* supportedEncodingCodecsArray[] = {
            "dvvideo", "libx264", "mjpeg", "pcm_s16be", "rawvideo", 0};

        const char* metadataFieldsArray[] = {
            "album",    "album_artist", "artist",      "author",  "comment",
            "composer", "copyright",    "description", "encoder", "episode_id",
            "genre",    "grouping",     "lyrics",      "network", "rotate",
            "show",     "synopsis",     "title",       "track",   "year",
            0};

        const char* ignoreMetadataFieldsArray[] = {
            "major_brand", "minor_version", "compatible_brands", "handler_name",
            "vendor_id", "language",
            "duration", // We ignore it here, since its explicitly added
                        // elsewhere.
            0};

        //----------------------------------------------------------------------
        //
        // Static Helpers
        //
        //----------------------------------------------------------------------

        string avErr2Str(int errNum)
        {
            char errBuf[AV_ERROR_MAX_STRING_SIZE];
            av_make_error_string(&errBuf[0], AV_ERROR_MAX_STRING_SIZE, errNum);
            return string(errBuf);
        }

        void avLogCallback(void* ptr, int level, const char* fmt, va_list vargs)
        {
            if ((string(fmt).substr(0, 51)
                 == "Encoder did not produce proper pts, making some up.")
                || (string(fmt).substr(0, 47)
                    == "No accelerated colorspace conversion found from")
                || (string(fmt).substr(0, 28) == "Increasing reorder buffer to")
                || (string(fmt).substr(0, 34)
                    == "sample aspect ratio already set to")
                || (string(fmt).substr(0, 67)
                    == "deprecated pixel format used, make sure you did set "
                       "range correctly")
                || (string(fmt).substr(0, 20) == "overread end of atom")
                || (string(fmt).substr(0, 19) == "Timecode frame rate")
                || (string(fmt).substr(0, 32)
                    == "unsupported color_parameter_type"))
            {
                return;
            }

            ostringstream message;
            if (level > av_log_get_level())
            {
                return;
            }
            else if (level > AV_LOG_WARNING)
            {
                message << "INFO";
            }
            else if (level == AV_LOG_WARNING)
            {
                message << "WARNING";
            }
            else
            {
                message << "ERROR";
            }
#if DB_GENERAL & DB_LEVEL
            message << " [" << level << "]: ";
            message << "MovieFFMpeg";
#endif
            message << ": " << string(fmt);
            vprintf(message.str().c_str(), vargs);
        }

        bool codecHasSlowAccess(string name)
        {
            boost::algorithm::to_lower(name);
            for (const char** p = slowRandomAccessCodecsArray; *p; p++)
            {
                if (*p == name)
                {
                    return true;
                }
            }
            return false;
        };

        bool isMP4format(AVFormatContext* avFormatContext)
        {
            return avFormatContext != nullptr
                   && avFormatContext->iformat != nullptr
                   && avFormatContext->iformat->name != nullptr
                   && strstr(avFormatContext->iformat->name, "mp4") != nullptr;
        }

        bool isMOVformat(AVFormatContext* avFormatContext)
        {
            return avFormatContext != nullptr
                   && avFormatContext->iformat != nullptr
                   && avFormatContext->iformat->name != nullptr
                   && strstr(avFormatContext->iformat->name, "mov") != nullptr;
        }

        int64_t findBestTS(int64_t goalTS, double frameDur, VideoTrack* track,
                           bool finalPacket)
        {
            //
            //  The timestamps we have collected from unordered packets give us
            //  a view into the future timestamps for the frames we _will_
            //  decode. This helps us "predict the future" for interframe codecs
            //  and do a better job of finding the best timestamp match for RV's
            //  frame request.
            //
            //  Below we look through the timestamps we know are coming to find
            //  the one that is closest to the goalTS that represents the RV
            //  requested frame
            //

            int64_t smallest = -1;
            map<int64_t, int64_t> diffs;
            for (set<int64_t>::iterator ts = track->tsSet.begin();
                 ts != track->tsSet.end(); ts++)
            {
                int64_t diff = abs(goalTS - *ts);
                if (diff < smallest || smallest == -1)
                    smallest = diff;
                diffs[diff] = *ts;
            }
            return (smallest != -1
                    && (smallest < (frameDur * 0.5) || finalPacket))
                       ? diffs[smallest]
                       : goalTS;
        }

        bool fpsEquals(double& fps, double f)
        {
            //
            //  We get fps from all kinds of places, may have varying sig figs,
            //  so compare to "standard" values loosely.
            //

            if (fabs(fps - f) < 0.01)
            {
                fps = f;
                return true;
            }
            return false;
        }

        AVPixelFormat getBestAVFormat(AVPixelFormat native)
        {
            const AVPixFmtDescriptor* desc = av_pix_fmt_desc_get(native);
            int bitSize = desc->comp[0].depth - desc->comp[0].shift;
            bool hasAlpha = true; //(desc->flags & AV_PIX_FMT_FLAG_ALPHA);
            return (hasAlpha)
                       ? ((bitSize > 8) ? AV_PIX_FMT_RGBA64 : AV_PIX_FMT_RGBA)
                       : ((bitSize > 8) ? AV_PIX_FMT_RGB48 : AV_PIX_FMT_RGB24);
        }

        AVPixelFormat getBestRVFormat(AVPixelFormat native)
        {
            //
            // This method is primarily designed to be run on pixel formats for
            // which RV cannot natively make use of. Right now that primarily
            // means 10-bit YUV & YUVA data, but could also include anything we
            // haven't found samples of or we simply don't have any anologous
            // FrameBuffer format.
            //
            // NOTE: The one type of formats we do natively support that can
            // pass through this unchanged are the 8-bit YUV & YUVA formats.
            //

            const AVPixFmtDescriptor* desc = av_pix_fmt_desc_get(native);
            int bitSize = desc->comp[0].depth - desc->comp[0].shift;
            bool hasAlpha = (desc->flags & AV_PIX_FMT_FLAG_ALPHA);
            bool isPlanar = (desc->flags & AV_PIX_FMT_FLAG_PLANAR);
            bool isRGB = (desc->flags & AV_PIX_FMT_FLAG_RGB);
            AVPixelFormat best = AV_PIX_FMT_NONE;

            // Planar YUV+
            if (isPlanar && !isRGB)
            {
                if (bitSize == 8)
                {
                    best = native;
                }
                else if (bitSize < 8)
                {
                    best =
                        (hasAlpha) ? AV_PIX_FMT_YUVA444P : AV_PIX_FMT_YUV444P;
                }
                else if (bitSize > 8)
                {
                    int log2w, log2h;
                    av_pix_fmt_get_chroma_sub_sample(native, &log2w, &log2h);
                    int usampling = int(pow(2.0f, log2w));
                    int vsampling = int(pow(2.0f, log2h));
                    int hfourcc = 4 / (usampling * vsampling);
                    switch (hfourcc)
                    {
                    case 0:
                        best = (hasAlpha) ? AV_PIX_FMT_YUVA420P16
                                          : AV_PIX_FMT_YUV420P16;
                        break;
                    case 1:
                    case 2:
                        best = (hasAlpha) ? AV_PIX_FMT_YUVA422P16
                                          : AV_PIX_FMT_YUV422P16;
                        break;
                    case 4:
                    default:
                        best = (hasAlpha) ? AV_PIX_FMT_YUVA444P16
                                          : AV_PIX_FMT_YUV444P16;
                        break;
                    }
                }
            }
            else // Everything else
            {
                best =
                    (hasAlpha)
                        ? ((bitSize > 8) ? AV_PIX_FMT_RGBA64 : AV_PIX_FMT_RGBA)
                        : ((bitSize > 8) ? AV_PIX_FMT_RGB48 : AV_PIX_FMT_RGB24);
            }

            return best;
        }

        bool isMetadataField(string check)
        {
            for (const char** p = metadataFieldsArray; *p; p++)
            {
                if (*p == check)
                    return true;
            }
            return false;
        }

        bool ignoreMetadataField(const string& check)
        {
            for (const char** p = ignoreMetadataFieldsArray; *p; p++)
            {
                if (*p == check)
                    return true;
            }
            return false;
        }

        void report(string message, bool warn = false)
        {
            string warning = (warn) ? "WARNING: " : "INFO: ";
            cout << warning << "MovieFFMpeg: " << message << endl;
        }

        void rowColumnSwap(unsigned char* in, int w, int h, unsigned char* out)
        {
            for (int i = 0; i < h; ++i)
            {
                unsigned char* rp = in + (i * w);
                unsigned char* rpLim = rp + w;
                unsigned char* cp = out + h - 1 - i;

                do
                {
                    *cp = *rp;
                    //
                    //  h is _width_ of out buffer
                    //
                    cp += h;
                } while (++rp < rpLim);
            }
        }

        void validateTimestamps(AVPacket* pkt, AVStream* stm,
                                AVCodecContext* context, int64_t frameCount,
                                bool isAudio = false)

        {
            if (pkt->pts == AV_NOPTS_VALUE && !isAudio
                && !(context->codec->capabilities & AV_CODEC_CAP_DELAY))
                pkt->pts = frameCount;

            if (pkt->pts != AV_NOPTS_VALUE)
                pkt->pts =
                    av_rescale_q(pkt->pts, context->time_base, stm->time_base);
            if (pkt->dts != AV_NOPTS_VALUE)
                pkt->dts =
                    av_rescale_q(pkt->dts, context->time_base, stm->time_base);
            if (pkt->duration > 0 && isAudio)
                pkt->duration = av_rescale_q(pkt->duration, context->time_base,
                                             stm->time_base);

            // Video AVPacket's duration needs to be initialized starting with
            // FFmpeg 4.4.3 as the automatic computing of the frame duration
            // code was removed from FFmpeg. Otherwise the exported media will
            // have an incorrect duration (-1 frame) which will result in a
            // slightly higher (incorrect) frame rate.
            if (pkt->duration == 0 && !isAudio)
            {
                pkt->duration =
                    av_rescale_q(1, context->time_base, stm->time_base);
            }
        }

        void copyImage(AVFrame* dst, const AVFrame* src,
                       const AVPixelFormat pix_fmt, int width, int height)
        {
            HOP_PROF_FUNC();

            // Following code is a multithreaded version of the code found in
            // image_copy() from imgutils.
            const AVPixFmtDescriptor* desc = av_pix_fmt_desc_get(pix_fmt);
            if (desc && !(desc->flags & AV_PIX_FMT_FLAG_HWACCEL))
            {
                if (desc->flags & AV_PIX_FMT_FLAG_PAL)
                {
                    // copy the palette
                    FastMemcpy(dst->data[1], src->data[1], 4 * 256);
                }
                else
                {
                    int planes_nb = 0;
                    for (int i = 0; i < desc->nb_components; i++)
                    {
                        planes_nb = FFMAX(planes_nb, desc->comp[i].plane + 1);
                    }

                    for (int i = 0; i < planes_nb; i++)
                    {
                        int h = height;
                        ptrdiff_t bwidth =
                            av_image_get_linesize(pix_fmt, width, i);
                        if (bwidth < 0)
                        {
                            av_log(NULL, AV_LOG_ERROR,
                                   "av_image_get_linesize failed\n");
                            return;
                        }
                        if (i == 1 || i == 2)
                        {
                            h = AV_CEIL_RSHIFT(height, desc->log2_chroma_h);
                        }
                        FastMemcpy_MP(dst->data[i], src->data[i], bwidth * h);
                    }
                }
            }
        }

    } // namespace

    //----------------------------------------------------------------------
    //
    // MovieFFMpegReader Class
    //
    //----------------------------------------------------------------------

    MovieFFMpegReader::MovieFFMpegReader(const MovieFFMpegIO* io)
        : MovieReader()
        , m_avFormatContext(0)
        , m_audioTracks(0)
        , m_videoTracks(0)
        , m_timingDetails(0)
        , m_timecodeTrack(-1)
        , m_formatStartFrame(0)
        , m_io(io)
        , m_dbline(0)
        , m_dblline(0)
        , m_multiTrackAudio(false)
        , m_audioState(0)
    {

        //
        // This FFMpeg plugin is not threadsafe and doesnt need to be for the
        // moment. FFMpeg itself is threaded already. If it was thread safe; the
        // video and audio temp buffers would hv to changed to use a thread safe
        // "memory pool" type functionality.
        //

        m_threadSafe = false;

#if DB_TIMING & DB_LEVEL
        m_timingDetails = new TimingDetails();
#endif
    }

    MovieFFMpegReader::~MovieFFMpegReader() { close(); }

    void MovieFFMpegReader::close()
    {
#if DB_TIMING & DB_LEVEL
        if (!m_cloning)
        {
            DBL(DB_TIMING, m_timingDetails->summary());
            delete m_timingDetails;
        }
#endif

        //
        // Delete AudioTracks/VideoTracks and their data
        //

        for (unsigned int i = 0; i < m_audioTracks.size(); i++)
        {
            AudioTrack* track = m_audioTracks[i];
            if (track->isOpen)
            {
                avcodec_free_context(&track->avCodecContext);
            }
            ContextPool::flushContext(this, track->number);
            delete track;
        }
        m_audioTracks.resize(0);

        for (unsigned int i = 0; i < m_videoTracks.size(); i++)
        {
            VideoTrack* track = m_videoTracks[i];
            if (track->isOpen)
            {
                avcodec_free_context(&track->avCodecContext);
            }
            ContextPool::flushContext(this, track->number);
            delete track;
        }
        m_videoTracks.resize(0);

        for (map<int, int>::iterator i = m_subtitleMap.begin();
             i != m_subtitleMap.end(); ++i)
        {
            if (i->second)
                ContextPool::flushContext(this, i->first);
            //  XXX We should be closing these streams too once we are opening
            //  them
        }

        if (m_avFormatContext)
            avformat_close_input(&m_avFormatContext);
    }

    void MovieFFMpegReader::audioConfigure(const AudioConfiguration& config)
    {
        if (m_audioState && m_audioState->layout == config.layout)
            return;
        if (m_audioState)
            delete m_audioState;
        m_audioState = new AudioState();
        m_audioState->layout = config.layout;

        if (canConvertAudioChannels())
        {
            m_audioState->channels = layoutChannels(config.layout);
            m_audioState->channelsPerTrack = m_audioState->channels.size();
            // We only make use of chmap when the in/out channels are not
            // identical. so that in translateAVAudio() we can do a fast copy of
            // the data from in to out buffers since there is no mix of any kind
            // required.
            if (m_info.audioChannels != m_audioState->channels)
            {
                initChannelsMap(m_info.audioChannels, m_audioState->channels,
                                m_audioState->chmap);
            }
        }
        else
        {
            m_audioState->channelsPerTrack = m_audioTracks[0]->numChannels;
            m_audioState->channels = layoutChannels(
                channelLayouts(m_audioState->channelsPerTrack).front());
            ;
            // No chmap required here since in/out channels are identical
            // because this plugin is set to not do any audio channels
            // conversion.
        }

        DBL(DB_AUDIO,
            "audioConfigure! -- "
                << m_filename << " layout: " << m_audioState->layout
                << " mapSize: " << m_audioState->chmap.size()
                << " trkChans: " << m_audioState->channels.size()
                << " chansPerTrk: " << m_audioState->channelsPerTrack);
    }

    bool MovieFFMpegReader::canConvertAudioChannels() const
    {
        // If each audio channel has its own track, then there is no performance
        // gain to be made by attempting to mix channels in the plugin.
        return !m_multiTrackAudio;
    }

    MovieReader* MovieFFMpegReader::clone() const
    {
        MovieFFMpegReader* mov = new MovieFFMpegReader(m_io);
        mov->m_cloning = true;
        if (m_filename != "")
        {
            mov->open(m_filename, m_info, m_request);
            for (int v = 0; v < m_videoTracks.size(); v++)
            {
                VideoTrack* track = new VideoTrack;
                track->initFrom(m_videoTracks[v]);
                mov->m_videoTracks.push_back(track);
            }
            for (int a = 0; a < m_audioTracks.size(); a++)
            {
                AudioTrack* track = new AudioTrack;
                track->initFrom(m_audioTracks[a]);
                mov->m_audioTracks.push_back(track);
            }
            mov->m_timecodeTrack = m_timecodeTrack;
            mov->m_formatStartFrame = m_formatStartFrame;
            mov->m_subtitleMap = m_subtitleMap;
            mov->m_multiTrackAudio = m_multiTrackAudio;
        }
        mov->m_cloning = false;
        return mov;
    }

    void MovieFFMpegReader::open(const string& filename, const MovieInfo& info,
                                 const Movie::ReadRequest& request)
    {
        m_filename = filename;
        m_info = info;
        m_request = request;

        if (m_cloning)
            return;

#if DB_TIMING & DB_LEVEL
        m_timingDetails->startTimer("initializeAll");
#endif

        initializeAll();

#if DB_TIMING & DB_LEVEL
        double initializeAllDuration =
            m_timingDetails->pauseTimer("initializeAll");
        DBL(DB_TIMING, "file: " << m_filename
                                << " initializeAll: " << initializeAllDuration);
#endif
    }

    bool MovieFFMpegReader::openAVFormat()
    {
        const bool filepathIsURL = TwkUtil::pathIsURL(m_filename);
        const bool fileExists =
            !filepathIsURL && TwkUtil::fileExists(m_filename.c_str());
        if (!filepathIsURL && !fileExists)
        {
            TWK_THROW_EXC_STREAM("Could not locate '" << m_filename
                                                      << "' on disk.");
        }

        // Make sure ffmpeg treats files on disk as files
        string safe_path = (fileExists) ? "file:" + m_filename : m_filename;

        if (filepathIsURL && evUseUploadedMovieForStreaming.getValue())
        {
            boost::replace_all(safe_path, "#.mp4", "");
        }

        // Check for cookies for streaming links
        AVDictionary* fmtOptions = NULL;
        if (filepathIsURL)
        {
            for (int i = 0; i < m_request.parameters.size(); i++)
            {
                const string& name = m_request.parameters[i].first;
                const string& value = m_request.parameters[i].second;
                if (name == "cookies")
                {
                    av_dict_set(&fmtOptions, "cookies", value.c_str(), 0);
                    av_dict_set_int(&fmtOptions, "seekable", 1, 0);
                    av_dict_set_int(&fmtOptions, "reconnect", 1, 0);
                    av_dict_set_int(&fmtOptions, "multiple_requests", 1, 0);
                }
                else if (name == "headers")
                {
                    av_dict_set(&fmtOptions, "headers", value.c_str(), 0);
                    av_dict_set_int(&fmtOptions, "seekable", 1, 0);
                    av_dict_set_int(&fmtOptions, "reconnect", 1, 0);
                    av_dict_set_int(&fmtOptions, "multiple_requests", 1, 0);
                }
            }
        }

        // Open the file
        const int ret = avformat_open_input(&m_avFormatContext,
                                            safe_path.c_str(), 0, &fmtOptions);
        if (ret != 0)
            TWK_THROW_EXC_STREAM("Failed to open "
                                 << m_filename
                                 << " for reading: " << avErr2Str(ret));

        return true;
    }

    void MovieFFMpegReader::trackFromStreamIndex(int index, VideoTrack*& vTrack,
                                                 AudioTrack*& aTrack)
    {
        vTrack = 0;
        aTrack = 0;

        for (int i = 0; i < m_videoTracks.size(); ++i)
        {
            if (m_videoTracks[i]->number == index)
            {
                vTrack = m_videoTracks[i];
                return;
            }
        }

        for (int i = 0; i < m_audioTracks.size(); ++i)
        {
            if (m_audioTracks[i]->number == index)
            {
                aTrack = m_audioTracks[i];
                return;
            }
        }
        //
        //  Not an error if we don't find track, since we may ask before the
        //  track is constructed.
        //
    }

    bool MovieFFMpegReader::openAVCodec(int index,
                                        AVCodecContext** avCodecContext)
    {
        // Make sure the format is opened
        if (m_avFormatContext == 0)
        {
            openAVFormat();
            findStreamInfo();
        }

        // Get the codec context
        AVStream* avStream = m_avFormatContext->streams[index];
        if (*avCodecContext && avcodec_is_open(*avCodecContext))
            return true;

        // Find the decoder for the av stream
        const AVCodec* avCodec = 0;
        switch (avStream->codecpar->codec_id)
        {
        case AV_CODEC_ID_H264:
        case AV_CODEC_ID_H265:
            // we want to read SPS and PPS NALs at the beginning of H.264 and
            // H.264 stream. Sometime this packet are only once at beginning
            m_mustReadFirstFrame = true;
            // no break;

        default:
            avCodec = avcodec_find_decoder(avStream->codecpar->codec_id);
            break;
        }
        if (avCodec == 0)
        {
            cout << "ERROR: MovieFFMpeg: Unsupported codec_id '"
                 << avStream->codecpar->codec_id << "' in " << m_filename
                 << endl;
            return false;
        }

        *avCodecContext = avcodec_alloc_context3(avCodec);
        if (!*avCodecContext)
        {
            cout << "ERROR: MovieFFMpeg: Failed to allocate codec context '"
                 << avCodec->name << "' for " << m_filename << endl;
            return false;
            ;
        }

        // Copy codec parameters from input stream to output codec context
        if (avcodec_parameters_to_context(*avCodecContext, avStream->codecpar)
            < 0)
        {
            cout << "ERROR: MovieFFMpeg: Failed to copy '" << avCodec->name
                 << "' codec parameters to decoder context for " << m_filename
                 << endl;
            avcodec_free_context(avCodecContext);
            return false;
        }

        // Open the codec
        (*avCodecContext)->thread_count = m_io->codecThreads();
        if (avcodec_open2(*avCodecContext, avCodec, 0) < 0)
        {
            cout << "ERROR: MovieFFMpeg: Failed to open codec '"
                 << avCodec->name << "' for " << m_filename << endl;
            avcodec_free_context(avCodecContext);
            return false;
        }

        // Check if this is a valid Video Pixel Format
        if ((*avCodecContext)->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            AVPixelFormat nativeFormat = (*avCodecContext)->pix_fmt;
            const AVPixFmtDescriptor* desc = av_pix_fmt_desc_get(nativeFormat);
            if (desc == NULL)
            {
                cout << "ERROR: MovieFFMpeg: Invalid pixel format! "
                     << m_filename << endl;
                avcodec_free_context(avCodecContext);
                return false;
            }
        }

        //
        //  We may be _re-opening_ the context here so be sure we don't rely on
        //  previous state
        //

        VideoTrack* vTrack;
        AudioTrack* aTrack;
        trackFromStreamIndex(index, vTrack, aTrack);

        if (vTrack)
            vTrack->lastDecodedVideo = -1;
        if (aTrack)
            aTrack->lastDecodedAudio = AV_NOPTS_VALUE;

        // Make sure to only use allowed codecs
        if (!m_io->codecIsAllowed((*avCodecContext)->codec->name, true))
        {
            cout << "ERROR: MovieFFMpeg: Unallowed codec '"
                 << (*avCodecContext)->codec->name << "' in " << m_filename
                 << endl;
            avcodec_free_context(avCodecContext);
            return false;
        }

        return true;
    }

    int64_t MovieFFMpegReader::getFirstFrame(AVRational rate)
    {
        //
        // XXX I am working from the assumption that timestamp 0 is true start
        // of every type of media. Therefore if we get a positive timestamp for
        // the format start time, then we have to assume that the source's start
        // is offset by the given positive value.
        //

        m_formatStartFrame =
            max(int64_t(0),
                int64_t(0.49
                        + av_q2d(rate) * double(m_avFormatContext->start_time)
                              / double(AV_TIME_BASE)));
        int64_t firstFrame = max(int64_t(m_formatStartFrame), int64_t(1));

        for (int i = 0; i < m_avFormatContext->nb_streams; i++)
        {
            AVStream* tsStream = m_avFormatContext->streams[i];

            AVRational tcRate = {tsStream->time_base.den,
                                 tsStream->time_base.num};

            if (isMOVformat(m_avFormatContext))
            {
                tcRate = tsStream->avg_frame_rate;
            }

            AVDictionaryEntry* tcrEntry;
            tcrEntry = av_dict_get(tsStream->metadata, "reel_name", NULL, 0);
            if (tcrEntry != NULL)
            {
                ostringstream tcReel;
                tcReel << tcrEntry->value;
                m_info.proxy.newAttribute("Timecode/ReelName", tcReel.str());
            }

            AVDictionaryEntry* tcEntry;
            tcEntry = av_dict_get(tsStream->metadata, "timecode", NULL, 0);
            if (tcEntry != NULL)
            {
                // Correct wrong frame rates that seem to be generated by some
                // codecs
                if (tcRate.num > 1000 && tcRate.den == 1)
                {
                    tcRate.den = 1000;
                }

                AVTimecode avTimecode;
                av_timecode_init_from_string(&avTimecode, tcRate,
                                             tcEntry->value, m_avFormatContext);

                // Add the timecode attributes to the movie info
                ostringstream tcStart, tcFR, tcFlags;
                tcStart << tcEntry->value << " (" << avTimecode.start << ")";
                m_info.proxy.newAttribute("Timecode/Start", tcStart.str());
                tcFR << avTimecode.fps;
                m_info.proxy.newAttribute("Timecode/FrameRate", tcFR.str());
                if (avTimecode.flags & AV_TIMECODE_FLAG_DROPFRAME)
                {
                    tcFlags << "Drop ";
                }
                if (avTimecode.flags & AV_TIMECODE_FLAG_24HOURSMAX)
                {
                    tcFlags << "24-Hour Max Counter ";
                }
                if (avTimecode.flags & AV_TIMECODE_FLAG_ALLOWNEGATIVE)
                {
                    tcFlags << "Allow Negative";
                }
                m_info.proxy.newAttribute("Timecode/Flags", tcFlags.str());
                m_timecodeTrack = i;

                firstFrame = avTimecode.start;
            }
        }
        return firstFrame;
    }

    void MovieFFMpegReader::snagMetadata(AVDictionary* dict, string source,
                                         FrameBuffer* fb)
    {
        bool filter = true;
#if DB_LEVEL & DB_METADATA
        filter = false;
#endif
        AVDictionaryEntry* tag = 0;
        while ((tag = av_dict_get(dict, "", tag, AV_DICT_IGNORE_SUFFIX)))
        {
            // We force the key to lower case, since the mkv file format has all
            // its metadata in uppercase, and mov and mp4 are in lowercase.
            string lckey = tag->key;
            for (int i = 0; i < lckey.length(); i++)
                lckey[i] = tolower(lckey[i]);

            // Then we make a version of the key with first letter caps for
            // display only.
            string key = lckey;
            key[0] = toupper(key[0]);

            ostringstream attrKey, attrValue;
            attrKey << source << "/" << key;
            attrValue << tag->value;
            DBL(DB_METADATA, attrKey.str() << ": " << attrValue.str());
            if (filter && ignoreMetadataField(string(lckey)))
                continue;

            fb->newAttribute(attrKey.str(), attrValue.str());
        }
    }

    bool MovieFFMpegReader::snagColr(AVCodecContext* videoCodecContext,
                                     VideoTrack* track)
    {
        bool foundIndividualValues = false;

        //
        // Repair missing color information in DNxHD files. ffmpeg will
        // read the ACLR atom for these files but some may have been
        // incorrectly created.
        //

        if (videoCodecContext->codec_id == AV_CODEC_ID_DNXHD)
        {
            // DNxHD defaults to Rec709
            if (videoCodecContext->color_primaries == AVCOL_PRI_UNSPECIFIED)
            {
                videoCodecContext->color_primaries = AVCOL_PRI_BT709;
            }
            if (videoCodecContext->colorspace == AVCOL_SPC_UNSPECIFIED)
            {
                videoCodecContext->colorspace = AVCOL_SPC_BT709;
            }
            foundIndividualValues = true;
        }

        //
        // Use mp4v2Utils to read information out of COLR atom if present
        //

        void* fileHandle;
        if (isMP4format(m_avFormatContext)
            && mp4v2Utils::readFile(m_filename, fileHandle))
        {
            int streamIndex = track->number;
            mp4v2Utils::getColrType(fileHandle, streamIndex, track->colrType);
            if (track->colrType == "nclc")
            {
                uint64_t prim, xfer, mtrx;
                mp4v2Utils::getNCLCValues(fileHandle, streamIndex, prim, xfer,
                                          mtrx);

                videoCodecContext->color_primaries = AVColorPrimaries(prim);
                videoCodecContext->color_trc =
                    AVColorTransferCharacteristic(xfer);
                videoCodecContext->colorspace = AVColorSpace(mtrx);

                foundIndividualValues = true;
            }
            else if (track->colrType == "nclx")
            {
                foundIndividualValues = true;
            }
            else if (track->colrType == "prof")
            {
                //
                // XXX Pixar prefers strange old behavior of attempting to apply
                // incorrect color information if a prof Profile is detected
                //

                if (getenv("TWK_MIOFFMPEG_IGNORE_ICC_PROFILE"))
                {
                    foundIndividualValues = true;
                }
                else
                {
                    unsigned char* profile = NULL;
                    uint32_t size = 0;
                    mp4v2Utils::getPROFValues(fileHandle, streamIndex, profile,
                                              size);

                    track->fb.setICCprofile((void*)profile, size);
                    track->fb.setTransferFunction(
                        TwkFB::ColorSpace::ICCProfile());
                    track->fb.setPrimaryColorSpace(
                        TwkFB::ColorSpace::ICCProfile());
                }
            }
            mp4v2Utils::closeFile(fileHandle);
        }

        return foundIndividualValues;
    }

    FrameBuffer::Orientation
    MovieFFMpegReader::snagOrientation(VideoTrack* track)
    {
        AVStream* videoStream = m_avFormatContext->streams[track->number];
        AVCodecContext* videoCodecContext = track->avCodecContext;
        AVPixelFormat nativeFormat = videoCodecContext->pix_fmt;
        const AVPixFmtDescriptor* desc = av_pix_fmt_desc_get(nativeFormat);
        bool yuvPlanar = (!(desc->flags & AV_PIX_FMT_FLAG_ALPHA)
                          && (desc->flags & AV_PIX_FMT_FLAG_PLANAR)
                          && !(desc->flags & AV_PIX_FMT_FLAG_RGB));

        int rotation = 0;
        AVDictionaryEntry* rotEntry;

        // Trying to get rotation from metadata
        rotEntry = av_dict_get(videoStream->metadata, "rotate", NULL, 0);
        rotation = (rotEntry) ? atoi(rotEntry->value) : 0;

        // If rotation metadata not in metadata, try to get it from side data
        if (!rotEntry && videoCodecContext->nb_coded_side_data > 0)
        {
            double rotationFromSideData = 0;
            for (int i = 0; i < videoCodecContext->nb_coded_side_data; ++i)
            {
                const AVPacketSideData* sd =
                    &videoStream->codecpar->coded_side_data[i];
                if (sd->type == AV_PKT_DATA_DISPLAYMATRIX)
                {
                    rotationFromSideData =
                        av_display_rotation_get((int32_t*)sd->data);
                }
            }

            // Getting rid of negative rotation metadata
            rotation = rotationFromSideData < 0
                           ? lround(rotationFromSideData) + 360
                           : lround(rotationFromSideData);

            // Setting rotation
            char charRotation[5]; // Expecting a number between -360 and 360
                                  // (inclusive)
            sprintf(charRotation, "%d", rotation);
            if (av_dict_set(&videoStream->metadata, "rotate", charRotation, 0)
                < 0)
            {
                cout << "ERROR: Unable to rotate video, unable to parse "
                        "rotation metadata."
                     << endl;
            }
            else
            {
                m_info.proxy.attribute<string>("Rotation") = charRotation;
            }
        }

        bool rotate = false;
        switch (rotation)
        {
        case 270:
        case -90:
            track->fb.setOrientation(FrameBuffer::BOTTOMRIGHT);
            track->rotate = yuvPlanar;
            rotate = true;
            break;
        case 180:
        case -180:
            track->fb.setOrientation(FrameBuffer::BOTTOMRIGHT);
            break;
        case 90:
        case -270:
            track->fb.setOrientation(FrameBuffer::TOPLEFT);
            track->rotate = yuvPlanar;
            rotate = true;
            break;
        case 0:
        default:
            track->fb.setOrientation(FrameBuffer::TOPLEFT);
            break;
        }

        if (!yuvPlanar && rotate)
        {
            report("Rotation only supported in YUV formats at this time", true);
        }

        return track->fb.orientation();
    }

    bool MovieFFMpegReader::snagVideoColorInformation(VideoTrack* track)
    {
        AVStream* videoStream = m_avFormatContext->streams[track->number];
        AVCodecContext* videoCodecContext = track->avCodecContext;

        // Check and see if we have any data about the color
        bool hasColorData =
            (snagColr(videoCodecContext, track)
             || ((videoCodecContext->color_primaries != AVCOL_PRI_UNSPECIFIED)
                 && (videoCodecContext->color_trc != AVCOL_TRC_UNSPECIFIED)
                 && (videoCodecContext->colorspace != AVCOL_SPC_UNSPECIFIED)));

        if (hasColorData)
        {
            //  Default to rec709
            float white[2] = {0.3127, 0.3290};
            float red[2] = {0.640, 0.330};
            float green[2] = {.3, .6};
            float blue[2] = {.15, .06};

            ostringstream cspace;
            switch (videoCodecContext->color_primaries)
            {
            case AVCOL_PRI_BT709: // = 1
                // ITU-R BT1361
                // IEC 61966-2-4
                // SMPTE RP177 Annex B
                track->fb.setPrimaryColorSpace(ColorSpace::Rec709());
                cspace << "ITU-R BT709 (1)";
                break;
            case AVCOL_PRI_UNSPECIFIED: // = 2
                cspace << "UNSPECIFIED (2)";
                break;
            case AVCOL_PRI_BT470M: // = 4
                cspace << "ITU-R BTM470M (4)";
                white[0] = 0.310;
                white[1] = 0.316;
                red[0] = 0.670;
                red[1] = 0.330;
                green[0] = 0.210;
                green[1] = 0.710;
                blue[0] = 0.140;
                blue[1] = 0.080;
                break;
            case AVCOL_PRI_BT470BG: // = 5
                // ITU-R BT601-6 625
                // ITU-R BT1358 625
                // ITU-R BT1700 625 PAL & SECAM
                cspace << "ITU-R BTM470BG (5)";
                green[0] = 0.29;
                track->fb.setPrimaryColorSpace(ColorSpace::Rec709());
                break;
            case AVCOL_PRI_SMPTE170M: // = 6
                // ITU-R BT601-6 525
                // ITU-R BT1358 525
                // ITU-R BT1700 NTSC
                cspace << "SMPTE-170M (6)";
                red[0] = 0.630;
                red[1] = 0.340;
                green[0] = 0.310;
                green[1] = 0.595;
                blue[0] = 0.155;
                blue[1] = 0.070;
                track->fb.setPrimaryColorSpace(ColorSpace::SMPTE_C());
                break;
            case AVCOL_PRI_SMPTE240M: // = 7
                // functionally identical to above
                cspace << "SMPTE-240M (7)";
                red[0] = 0.630;
                red[1] = 0.340;
                green[0] = 0.310;
                green[1] = 0.595;
                blue[0] = 0.155;
                blue[1] = 0.070;
                track->fb.setPrimaryColorSpace(ColorSpace::SMPTE_C());
                break;
            case AVCOL_PRI_FILM: // = 8
                cspace << "FILM (8)";
                break;
            default:
                cspace << "UNKNOWN (" << videoCodecContext->color_primaries
                       << ")";
                break;
            }

            ostringstream transfer;
            switch (videoCodecContext->color_trc)
            {
            case AVCOL_TRC_BT709: // = 1
                // ITU-R BT1361
                transfer << "ITU-R BT709 (1)";
                track->fb.setTransferFunction(ColorSpace::Rec709());
                break;
            case AVCOL_TRC_UNSPECIFIED: // = 2
                transfer << "UNSPECIFIED (2)";
                break;
            case AVCOL_TRC_GAMMA22: // = 4
                // ITU-R BT470M
                // ITU-R BT1700 625 PAL & SECAM
                transfer << "GAMMA 2.2 (4)";
                track->fb.setTransferFunction("Gamma 2.2");
                track->fb.attribute<float>(ColorSpace::Gamma()) = 2.2;
                break;
            case AVCOL_TRC_GAMMA28: // = 5
                // ITU-R BT470BG
                transfer << "GAMMA 2.8 (5)";
                track->fb.setTransferFunction("Gamma 2.8");
                track->fb.attribute<float>(ColorSpace::Gamma()) = 2.8;
                break;
            case AVCOL_TRC_SMPTE240M: // = 7
                transfer << "SMPTE-240M (7)";
                track->fb.setTransferFunction(ColorSpace::SMPTE240M());
                break;
            default:
                transfer << "UNKNOWN (" << videoCodecContext->color_trc << ")";
                break;
            }

            ostringstream matrix;
            switch (videoCodecContext->colorspace)
            {
            case AVCOL_SPC_RGB: // = 0
                // Unknown
                matrix << "RGB (0)";
                break;
            case AVCOL_SPC_BT709: // = 1
                // ITU-R BT1361
                // IEC 61966-2-4 xvYCC709
                // SMPTE RP177 Annex B
                matrix << "ITU-R BT709 (1)";
                track->fb.setConversion(ColorSpace::Rec709());
                break;
            case AVCOL_SPC_UNSPECIFIED: // = 2
                matrix << "UNSPECIFIED (2)";
                break;
            case AVCOL_SPC_FCC: // = 4
                // Unknown
                matrix << "FCC (4)";
                break;
            case AVCOL_SPC_BT470BG: // = 5
                // ITU-R BT601-6 625
                // ITU-R BT1358 625
                // ITU-R BT1700 625 PAL & SECAM
                // IEC 61966-2-4 xvYCC601
                matrix << "ITU-R BT470BG (5)";
                track->fb.setConversion(ColorSpace::Rec601());
                break;
            case AVCOL_SPC_SMPTE170M: // = 6
                // ITU-R BT601-6 525
                // ITU-R BT1358 525
                // ITU-R BT1700 NTSC
                // functionally identical to above
                matrix << "SMPTE-170M (6)";
                track->fb.setConversion(ColorSpace::Rec601());
                break;
            case AVCOL_SPC_SMPTE240M: // = 7
                matrix << "SMPTE-240M (7)";
                track->fb.setConversion(ColorSpace::SMPTE240M());
                break;
            case AVCOL_SPC_YCOCG: // = 8
                // Used by Dirac, VC-2 and H.264 FRext, see ITU-T SG16
                matrix << "YCoCg (8)";
                break;
            default:
                matrix << "UNKNOWN (" << videoCodecContext->colorspace << ")";
            }

            switch (videoCodecContext->color_range)
            {
            case AVCOL_RANGE_UNSPECIFIED: // = 0
                break;
            case AVCOL_RANGE_MPEG: // = 1
                // the normal 219*2^(n-8) "MPEG" YUV ranges
                track->fb.attribute<string>(ColorSpace::Range()) =
                    ColorSpace::VideoRange();
                break;
            case AVCOL_RANGE_JPEG: // = 2
                // the normal 2^n-1 "JPEG" YUV ranges
                track->fb.attribute<string>(ColorSpace::Range()) =
                    ColorSpace::FullRange();
                break;
            default:
                break;
            }

            switch (videoCodecContext->chroma_sample_location)
            {
            case AVCHROMA_LOC_UNSPECIFIED: // = 0
                break;
            case AVCHROMA_LOC_LEFT: // = 1
                // mpeg2/4, h264 default
                track->fb.attribute<string>(ColorSpace::ChromaPlacement()) =
                    ColorSpace::Left();
                break;
            case AVCHROMA_LOC_CENTER: // = 2
                // mpeg1, jpeg, h263
                track->fb.attribute<string>(ColorSpace::ChromaPlacement()) =
                    ColorSpace::Center();
                break;
            case AVCHROMA_LOC_TOPLEFT: // = 3
                // DV
                track->fb.attribute<string>(ColorSpace::ChromaPlacement()) =
                    ColorSpace::TopLeft();
                break;
            case AVCHROMA_LOC_TOP: // = 4
                track->fb.attribute<string>(ColorSpace::ChromaPlacement()) =
                    ColorSpace::Top();
                break;
            case AVCHROMA_LOC_BOTTOMLEFT: // = 5
                track->fb.attribute<string>(ColorSpace::ChromaPlacement()) =
                    ColorSpace::BottomLeft();
                break;
            case AVCHROMA_LOC_BOTTOM: // = 6
                track->fb.attribute<string>(ColorSpace::ChromaPlacement()) =
                    ColorSpace::Bottom();
                break;
            default:
                break;
            }

            // XXX Not sure if these values are still coming from COLR atom
            track->fb.setPrimaries(white[0], white[1], red[0], red[1], green[0],
                                   green[1], blue[0], blue[1]);
            string prefix = (track->colrType == "") ? "Codec/" : "COLR/";
            track->fb.newAttribute(prefix + "Matrix", matrix.str());
            track->fb.newAttribute(prefix + "Transfer", transfer.str());
            track->fb.newAttribute(prefix + "Primaries", cspace.str());
        }
        else if (track->colrType != "prof")
        {
            //
            //  Some of the codecs need to have more info reported in order to
            //  display correctly.
            //

            string name = string(videoCodecContext->codec->name);
            if (name == "dnxhd")
            {
                //
                // XXX This may no longer be required
                //

                track->fb.attribute<string>("ColorSpace/Note") =
                    "FFMPEG provides Rec601, expected Rec709";
                track->fb.setConversion(ColorSpace::Rec601());
                track->fb.setRange(ColorSpace::VideoRange());
                track->fb.setPrimaryColorSpace(ColorSpace::Rec709());
            }
            else if (name == "jpegls" || name == "ljpeg" || name == "mjpeg"
                     || name == "mjpegb")
            {
                track->fb.setConversion(ColorSpace::Rec601());
                track->fb.setRange(ColorSpace::FullRange());
                track->fb.setPrimaryColorSpace(ColorSpace::Rec709());
            }
            else if (name == "h264")
            {
                //
                //  XXX should come back to this; what about h263, etc ?
                //

                track->fb.setConversion(ColorSpace::Rec709());
                track->fb.setRange(ColorSpace::VideoRange());
                track->fb.setPrimaryColorSpace(ColorSpace::Rec709());
            }

            //
            //  The current existing behavior in RV if no transfer
            //  function is explicitly set on fb is to default to sRGB.
            //

            if (const char* c = getenv("RV_DEFAULT_QTCOLR_TRANSFER"))
            {
                track->fb.setTransferFunction(c);
            }
        }

        // If any information was found in the COLR atom list it here
        if (track->colrType != "")
        {
            track->fb.newAttribute("COLR/ParameterType", track->colrType);
        }

        return hasColorData;
    }

    string MovieFFMpegReader::snagVideoFrameType(VideoTrack* track)
    {
        string typeStr;
        switch (track->videoFrame->pict_type)
        {
        case AV_PICTURE_TYPE_NONE:
            typeStr = "None";
            break;
        case AV_PICTURE_TYPE_I:
            typeStr = "I Frame";
            break;
        case AV_PICTURE_TYPE_P:
            typeStr = "P Frame";
            break;
        case AV_PICTURE_TYPE_B:
            typeStr = "B Frame";
            break;
        case AV_PICTURE_TYPE_S:
            typeStr = "S Frame";
            break;
        case AV_PICTURE_TYPE_SI:
            typeStr = "SI Frame";
            break;
        case AV_PICTURE_TYPE_SP:
            typeStr = "SP Frame";
            break;
        case AV_PICTURE_TYPE_BI:
            typeStr = "BI Frame";
            break;
        default:
            typeStr = "Unknown";
            break;
        }
        track->fb.attribute<string>("FrameType") = typeStr;
        return typeStr;
    }

    bool MovieFFMpegReader::correctLang(int index)
    {
        string stmLang = streamLang(index);
        return (stmLang == m_io->language() || stmLang == "und");
    }

    string MovieFFMpegReader::streamLang(int index)
    {
        AVStream* avStream = m_avFormatContext->streams[index];

        string lang = "und";
        AVDictionaryEntry* tcEntry = 0;
        tcEntry = av_dict_get(avStream->metadata, "language", NULL, 0);
        if (tcEntry != NULL)
            lang = string(tcEntry->value);
        return lang;
    }

    void MovieFFMpegReader::findStreamInfo()
    {
        //
        // Open the format context and look for video and audio tracks. If we
        // find video tracks store the stream location and save the view. For
        // now assume there is only one audio track since unlike video audio can
        // be multichannel.
        //

        m_avFormatContext->probesize = TWK_AVFORMAT_PROBESIZE;
        if (avformat_find_stream_info(m_avFormatContext, 0) < 0)
            TWK_THROW_EXC_STREAM(
                "Failed to open stream for reading: " << m_filename);
    }

    void MovieFFMpegReader::initializeAll()
    {
        openAVFormat();

        //
        // For some formats the streams are not partially located at all. In
        // that case we need to find them first.
        //

        bool foundStreams = false;
        if (m_avFormatContext->nb_streams == 0)
        {
            findStreamInfo();
            foundStreams = true;
        }

        //
        // These series of checks are an attempt to cover for bad or missing
        // data regarding the resolution.
        //

        int height = 0;
        int width = 0;
        string lang = "und";
        map<string, set<int>> chLangMap;
        map<pair<int, int>, vector<int>> resTrackMap;
        map<pair<string, int>, vector<int>> chTrackMap;
        vector<bool> heroVideoTracks;
        vector<bool> heroAudioTracks;
        for (int i = 0; i < m_avFormatContext->nb_streams; i++)
        {
            heroVideoTracks.push_back(false);
            heroAudioTracks.push_back(false);
            AVStream* tsStream = m_avFormatContext->streams[i];

            if (tsStream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            {
                //
                // Due to a bug in how some decoders reset the width and height
                // dimensions of the media we are now storing them immediately
                // after reading the file header for the first time.
                //

                height = max(height, tsStream->codecpar->height);
                width = max(width, tsStream->codecpar->width);
                pair<int, int> key(tsStream->codecpar->height,
                                   tsStream->codecpar->width);
                resTrackMap[key].push_back(i);
            }
            else if (tsStream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
            {
                string tsLang = streamLang(i);
                int numChans = tsStream->codecpar->ch_layout.nb_channels;

                // Keep a sorted set of channel counts for each language
                chLangMap[tsLang].insert(numChans);

                // Look for a language (prefer English)
                lang = (lang == m_io->language()) ? lang : tsLang;

                //
                // The goal here is to save the first stream/track index
                // encountered for each language + channel count pair found in
                // the file, with one exception. If the file has all of the
                // audio broken out into a single channel in each track then we
                // want to save all of the track indicies and later merge the
                // samples when rendering audio.
                //

                pair<string, int> key(tsLang, numChans);
                if (chTrackMap[key].size() == 0 || numChans == 1)
                {
                    chTrackMap[key].push_back(i);
                }
            }
        }

        if (!foundStreams)
            findStreamInfo();

        //
        // Get the extension support capability
        //

        string ext = boost::filesystem::extension(m_filename);
        if (ext[0] == '.')
            ext.erase(0, 1);
        MovieFFMpegIO::MFFormatMap formats = m_io->getFormats();
        MovieFFMpegIO::MFFormatMap::iterator formatItr = formats.find(ext);
        int fmtCap = MovieIO::MovieRead | MovieIO::MovieReadAudio;
        if (formatItr != formats.end())
            fmtCap = formatItr->second.second;

        //
        // Mark hero video tracks by resolution
        //

        pair<int, int> maxRes(height, width);
        for (vector<int>::iterator trk = resTrackMap[maxRes].begin();
             trk != resTrackMap[maxRes].end(); trk++)
        {
            heroVideoTracks[*trk] = fmtCap & MovieIO::MovieRead;
        }

        //
        // Try to find a 2 channel stereo track from the target language. In
        // abscence of stereo use the lowest track count found to search for
        // valid audio tracks.
        //

        if (chLangMap.size() > 0)
        {
            int bestChans = (chLangMap[lang].find(2) != chLangMap[lang].end())
                                ? 2
                                : *chLangMap[lang].begin();
            pair<string, int> bestAudio(lang, bestChans);
            for (vector<int>::iterator trk = chTrackMap[bestAudio].begin();
                 trk != chTrackMap[bestAudio].end(); trk++)
            {
                heroAudioTracks[*trk] = fmtCap & MovieIO::MovieReadAudio;
            }
        }

        //
        // Find the start, end, and fps
        //

        collectPlaybackTiming(heroVideoTracks, heroAudioTracks);

        for (int i = 0; i < m_avFormatContext->nb_streams; i++)
        {
            AVStream* tsStream = m_avFormatContext->streams[i];

            //
            // Search for audio, video, and subtitle streams
            //

            switch (tsStream->codecpar->codec_type)
            {
            case AVMEDIA_TYPE_VIDEO:
                DBL(DB_METADATA, "Video Track: " << i);
                if (heroVideoTracks[i])
                {
                    ContextPool::Reservation reserve(this, i);
                    VideoTrack* track = new VideoTrack;
                    if (openAVCodec(i, &track->avCodecContext))
                    {
                        track->number = i;

                        ostringstream trackName;
                        trackName << "track " << m_videoTracks.size() + 1;
                        track->name = trackName.str();
                        track->isOpen = true;
                        m_videoTracks.push_back(track);

                        FBInfo::ViewInfo vinfo;
                        vinfo.name = trackName.str();
                        m_info.viewInfos.push_back(vinfo);
                        m_info.views.push_back(trackName.str());
                        ostringstream trk;
                        trk << "Track" << i;

                        snagMetadata(tsStream->metadata, trk.str(),
                                     &m_info.proxy);
                    }
                    else
                    {
                        delete track;
                    }
                }
                break;
            case AVMEDIA_TYPE_AUDIO:
                DBL(DB_METADATA, "Audio Track: " << i);
                if (heroAudioTracks[i])
                {
                    ContextPool::Reservation reserve(this, i);
                    AudioTrack* track = new AudioTrack;
                    if (openAVCodec(i, &track->avCodecContext))
                    {
                        track->number = i;
                        track->isOpen = true;
                        m_audioTracks.push_back(track);
                        ostringstream trk;
                        trk << "Track" << i;

                        snagMetadata(tsStream->metadata, trk.str(),
                                     &m_info.proxy);
                    }
                    else
                    {
                        delete track;
                    }
                }
                break;
            case AVMEDIA_TYPE_ATTACHMENT:
                DBL(DB_METADATA, "Attachment Track: " << i);
                break;
            case AVMEDIA_TYPE_DATA:
                DBL(DB_METADATA, "Data Track: " << i);
                break;
            case AVMEDIA_TYPE_NB:
                DBL(DB_METADATA, "NB Track: " << i);
                break;
            case AVMEDIA_TYPE_SUBTITLE:
                DBL(DB_METADATA, "Subtitle Track: " << i);
                {
                    // TODO Need to allocate memory now in ffmpeg6
                    // #if DB_LEVEL & DB_SUBTITLES
                    //    ContextPool::Reservation reserve(this, i);
                    //    if (openAVCodec(i))
                    //    {
                    //        m_subtitleMap[i] = 1;
                    //    }
                    // #endif
                }
                break;
            case AVMEDIA_TYPE_UNKNOWN:
            default:
                DBL(DB_METADATA, "Unknown Track: " << i);
                break;
            }
        }
        if (m_videoTracks.empty() && m_audioTracks.empty())
        {
            TWK_THROW_EXC_STREAM("Unable to find a media track.");
        }

        // Get the Artist, Album, Description, etc
        snagMetadata(m_avFormatContext->metadata, "Movie", &m_info.proxy);

        // Set resonable defaults for the video size in case this is audio only
        m_info.video = false;
        m_info.width = 800;
        m_info.height = 120;
        m_info.uncropWidth = 800;
        m_info.uncropHeight = 120;

        // Capture video metadata information for the frame buffer attributes
        if (m_videoTracks.size() > 0)
            initializeVideo(height, width);

// Initialize subtitles
#if DB_LEVEL & DB_SUBTITLES
        if (m_subtitleMap.size() > 0)
        {
            m_info.proxy.newAttribute("SubtitleTracks", m_subtitleMap.size());
        }
#endif

        // Look for chapters
        m_info.chapters.resize(m_avFormatContext->nb_chapters);
        for (int c = 0; c < m_avFormatContext->nb_chapters; c++)
        {
            AVChapter* chapter = m_avFormatContext->chapters[c];
            double start = double(chapter->start) * av_q2d(chapter->time_base);
            m_info.chapters[c].startFrame = start * m_info.fps + m_info.start;
            double end = double(chapter->end) * av_q2d(chapter->time_base);
            m_info.chapters[c].endFrame = end * m_info.fps + m_info.start;
            if ((c + 1) < m_avFormatContext->nb_chapters)
            {
                m_info.chapters[c].endFrame -= 1;
            }

            ostringstream chapterPrefix;
            chapterPrefix << "Chapter" << c;
            AVDictionaryEntry* tcEntry;
            tcEntry = av_dict_get(chapter->metadata, "title", NULL, 0);
            string title = chapterPrefix.str();
            if (tcEntry != NULL)
                title = string(tcEntry->value);
            m_info.chapters[c].title = title;
            snagMetadata(chapter->metadata, chapterPrefix.str(), &m_info.proxy);
        }

        // Capture audio metadata information for the frame buffer attributes
        if (m_audioTracks.size() > 0)
            initializeAudio();
        string hasAudio = string((m_audioTracks.size() > 0) ? "Yes" : "No");
        m_info.proxy.newAttribute("Audio", hasAudio);

        //
        // If we are handling video in this file then we need to copy all of the
        // frame buffer attributes we have collected so far back to the
        // placeholder frame buffer of each video track. That way later on when
        // we are collecting frames we can assign these attributes back to the
        // returned frame buffers.
        //

        if (m_videoTracks.size() > 0)
        {
            // Finish populating per track framebuffer attributes
            for (unsigned int i = 0; i < m_videoTracks.size(); i++)
            {
                VideoTrack* track = m_videoTracks[i];
                m_info.proxy.appendAttributesAndPrefixTo(&track->fb, "");
                addVideoFBAttrs(track);
            }

            //
            // This copies the attributes from the first track framebuffer to
            // the shared generic framebuffer which gets passed to source_setup
            // and is used to determine initial color settings.
            //

            m_videoTracks[0]->fb.copyAttributesTo(&m_info.proxy);
        }
        else
        {
            // Add shared timing/name info to the proxy if this is audio only
            finishTrackFBAttrs(&m_info.proxy, "");
        }
    }

    void MovieFFMpegReader::initializeVideo(int height, int width)
    {
        //
        // Check each video track for dimensions and timing information.
        // Initialize per track decode packet and frame. Create any swscale
        // resize context for each track in case we cannot handle the native
        // pixel format later.
        //

        bool slowRandomAccess = false;
        for (unsigned int i = 0; i < m_videoTracks.size(); i++)
        {
            VideoTrack* track = m_videoTracks[i];
            AVStream* videoStream = m_avFormatContext->streams[track->number];
            AVCodecContext* videoCodecContext = track->avCodecContext;

            // Tell RV to restrict caching to one thread
            bool slowTrackRandomAccess =
                (codecHasSlowAccess(videoCodecContext->codec->name)
                 || TwkUtil::pathIsURL(m_filename));
            slowRandomAccess = slowTrackRandomAccess || slowRandomAccess;

            // Make sure the orientation/rotation matches for each track
            m_info.proxy.setOrientation(snagOrientation(track));
            if (i > 0 && m_info.proxy.orientation() != m_info.orientation)
            {
                TWK_THROW_EXC_STREAM(
                    "Streams/Tracks with mixed rotations are unsupported");
            }
            m_info.orientation = m_info.proxy.orientation();
        }
        if (slowRandomAccess)
        {
            m_info.proxy.attribute<string>("Note") =
                "Movie Has Slow Random Access";
        }
        m_info.slowRandomAccess = slowRandomAccess;

        // Set the shared MovieInfo. XXX Default to first video track
        AVStream* firstVideoStream =
            m_avFormatContext->streams[m_videoTracks[0]->number];
        AVCodecContext* firstVideoCodecContext =
            m_videoTracks[0]->avCodecContext;
        AVPixelFormat nativeFormat = firstVideoCodecContext->pix_fmt;
        const AVPixFmtDescriptor* desc = av_pix_fmt_desc_get(nativeFormat);
        int bitSize = desc->comp[0].depth - desc->comp[0].shift;
        m_info.numChannels = desc->nb_components;
        m_info.dataType =
            (bitSize > 8) ? FrameBuffer::USHORT : FrameBuffer::UCHAR;

        // Set the channel specific information
        string fmtname = string(av_get_pix_fmt_name(nativeFormat));
        set<char> visited;
        int c = 0;
        for (string::iterator it = fmtname.begin();
             it != fmtname.end() && c < m_info.numChannels; ++it)
        {
            if (!isdigit(*it) && visited.find(*it) == visited.end())
            {
                FBInfo::ChannelInfo cinfo;
                cinfo.name = toupper(*it);
                cinfo.type = m_info.dataType;
                m_info.channelInfos.push_back(cinfo);
                visited.insert(*it);
                c++;
            }
        }

        //
        // Account for any reductions in the dimensions
        //
        // NOTE: For some codec/container combinations the initial dimensions
        // reported are the intended display resolution, yet the dimensions
        // reported after reading the first frame represent what is actually
        // stored in the file. If the height or width reduce in size then we
        // must guard against over reads by storing the new smaller sizes.
        //

        double cWidth = firstVideoCodecContext->width;
        double cHeight = firstVideoCodecContext->height;
        double pixelAspect = av_q2d(firstVideoStream->sample_aspect_ratio);
        if ((width > cWidth && cWidth > 0) || (height > cHeight && cHeight > 0))
        {
            // Repair the pixel aspect ratio if misreported or broken
            if (pixelAspect <= 0 && height > 0)
            {
                double displayAspect = double(width) / double(height);
                double storedAspect = cWidth / cHeight;
                pixelAspect = displayAspect / storedAspect;
            }

            //
            // XXX It is possible from the logic above that cWidth > width or
            // cHeight > height, but we are going to trust ffmpeg and assume
            // that if either is smaller than we should update both dimensions
            //

            if (cWidth > width || cHeight > height)
            {
                ostringstream msg;
                msg << "Expected resolution " << width << "x" << height
                    << " but found " << cWidth << "x" << cHeight
                    << ". Updating to new dimensions.";
                report(msg.str(), true);
            }
            width = cWidth;
            height = cHeight;
        }

        // If incoming values are zero use the new dimensions
        width = (width == 0) ? cWidth : width;
        height = (height == 0) ? cHeight : height;

        // If we were not able to find a reasonable PA assume 1
        m_info.pixelAspect = (pixelAspect > 0) ? pixelAspect : 1.0;

        // Get the imagery specifics
        m_info.width = m_videoTracks[0]->rotate ? height : width;
        m_info.height = m_videoTracks[0]->rotate ? width : height;
        m_info.uncropWidth = m_info.width;
        m_info.uncropHeight = m_info.height;
        m_info.uncropX = 0;
        m_info.uncropY = 0;

        m_info.video = true;
    }

    void MovieFFMpegReader::initializeAudio()
    {
        vector<string> audioLangs;
        int numChannels = 0;
        double audioSampleRate = 0.0;
        int64_t audioInfoLength;
        string audioLanguage;
        string audioCodec;
        AVSampleFormat audioFormat;
        ChannelsVector audioChannels;
        for (int i = m_audioTracks.size() - 1; i >= 0; i--)
        {
            AudioTrack* track = m_audioTracks[i];
            AVStream* audioStream = m_avFormatContext->streams[track->number];
            AVCodecContext* audioCodecContext = track->avCodecContext;
            if (audioSampleRate != 0
                && audioSampleRate != audioCodecContext->sample_rate)
            {
                TWK_THROW_EXC_STREAM(
                    "Audio sample rate must match for each track!");
            }
            audioSampleRate = audioCodecContext->sample_rate;
            if (numChannels != 0
                && numChannels != audioCodecContext->ch_layout.nb_channels)
            {
                TWK_THROW_EXC_STREAM(
                    "Audio channel count must match for each track!");
            }
            numChannels = audioCodecContext->ch_layout.nb_channels;
            track->numChannels = numChannels;

            double timebase = av_q2d(audioStream->time_base);
            int64_t duration = audioStream->duration;
            audioInfoLength =
                (duration > 0)
                    ? int64_t(duration * timebase * audioSampleRate + 0.49)
                    : int64_t(double(m_avFormatContext->duration)
                                  / double(AV_TIME_BASE) * audioSampleRate
                              + 0.49);
            audioFormat = audioCodecContext->sample_fmt;
            audioLanguage = streamLang(track->number);
            audioCodec = string(audioCodecContext->codec->long_name);

            DBL(DB_AUDIO,
                "Audio track "
                    << i << " start_time: " << audioStream->start_time
                    << " num frames: " << audioStream->nb_frames
                    << " frame size: " << audioCodecContext->frame_size
                    << " duration: " << duration << " length: "
                    << audioInfoLength << " sample_rate: " << audioSampleRate
                    << " channels: " << numChannels
                    << " timebase: " << timebase);

            // Get the input source channels
            AVChannelLayout layout = audioCodecContext->ch_layout;
            audioChannels = idAudioChannels(layout, numChannels);

            // XXX Assume this thread will never decode audio
            avcodec_free_context(&audioCodecContext);
            track->isOpen = false;
        }

        // Add audio metadata assuming it is shared for all tracks
        char temp[80];
        sprintf(temp, "%g kHz", audioSampleRate / 1000.0);
        m_info.audioSampleRate = audioSampleRate;
        m_info.proxy.newAttribute("AudioSamplingRate", string(temp));
        sprintf(temp, "%lld", audioInfoLength);
        m_info.proxy.newAttribute("AudioSamples", string(temp));
        sprintf(temp, "%d", 8 * av_get_bytes_per_sample(audioFormat));
        m_info.proxy.newAttribute("AudioBitsPerSample", string(temp));
        m_info.proxy.newAttribute("AudioSampleFormat",
                                  string(av_get_sample_fmt_name(audioFormat)));
        m_info.proxy.newAttribute("AudioCodec", audioCodec);
        m_info.proxy.newAttribute("AudioLanguage", audioLanguage);
        m_info.audioLanguages.push_back(audioLanguage);

        m_multiTrackAudio = (m_audioTracks.size() > 1 && numChannels == 1);

        // If this is ProRes or another inconvertible codec then group the
        // channels
        if (!canConvertAudioChannels())
        {
            numChannels *= m_audioTracks.size();
            audioChannels = layoutChannels(channelLayouts(numChannels).front());
        }
        m_info.audioChannels = audioChannels;

        // Return named audio channels where possible
        ostringstream channelStr;
        channelStr << numChannels << " total (" << m_audioTracks.size()
                   << " tracks)";
        for (int ch = 0; ch < audioChannels.size(); ch++)
        {
            channelStr << "\n" << channelString(audioChannels[ch]);
        }
        m_info.proxy.newAttribute("AudioChannels", channelStr.str());

        // Remove unused PixelAspectRatio
        m_info.proxy.deleteAttribute(
            m_info.proxy.findAttribute("PixelAspectRatio"));

        m_info.audio = true;
    }

    void MovieFFMpegReader::addVideoFBAttrs(VideoTrack* track)
    {
        AVCodecContext* videoCodecContext = track->avCodecContext;

        // Initialize the frame type
        snagVideoFrameType(track);

        // Get the colr-esk information
        snagVideoColorInformation(track);

        track->fb.setPixelAspectRatio(m_info.pixelAspect);
        track->fb.newAttribute(
            "VideoPixelFormat",
            string(av_get_pix_fmt_name(videoCodecContext->pix_fmt)));
        track->fb.newAttribute("VideoCodec",
                               string(videoCodecContext->codec->long_name));

        ostringstream attr;
        attr << m_videoTracks.size();
        track->fb.newAttribute("VideoTracks", attr.str());

        // Add the general timing/naming info
        finishTrackFBAttrs(&track->fb, track->name);
    }

    void MovieFFMpegReader::finishTrackFBAttrs(FrameBuffer* fb, string view)
    {
        ostringstream attr;
        attr.clear();
        attr.str("");
        attr << m_info.fps;
        fb->newAttribute("FPS", attr.str());

        attr.clear();
        attr.str("");
        int frameCount = m_info.end - m_info.start + 1;
        attr << frameCount << " frames, " << (double(frameCount) / m_info.fps)
             << " sec";
        fb->newAttribute("Duration", attr.str());
        if (view != "")
            fb->newAttribute("View", view);
        fb->newAttribute("File", m_filename);
        fb->newAttribute("Sequence", m_filename);
    }

    void MovieFFMpegReader::scan()
    {
        // NO LONGER NEEDED
    }

    void MovieFFMpegReader::collectPlaybackTiming(vector<bool> heroVideoTracks,
                                                  vector<bool> heroAudioTracks)
    {
        double formatTimeDur =
            double(m_avFormatContext->duration) / double(AV_TIME_BASE);

        DBL(DB_DURATION,
            "format start time: " << m_avFormatContext->start_time
                                  << " format dur (secs): " << formatTimeDur
                                  << " (" << m_avFormatContext->duration << "/"
                                  << AV_TIME_BASE << ")");

        //
        // As we walk the streams we will look out for video. When we encounter
        // video streams we will do some checks to make sure the timing matches
        // between tracks/streams.
        //

        AVRational rate = av_d2q(0.0, INT_MAX);
        int64_t frames = 0;
        Time audioLength = 0;
        double timeDuration = 0;
        for (int i = 0; i < m_avFormatContext->nb_streams; i++)
        {

            //
            // If the number of frames in the stream is wrong use the average or
            // real frame rate of the media stream. If the duration of the
            // stream is wrong use the value we calculated for the entire format
            // container.
            //

            AVStream* tsStream = m_avFormatContext->streams[i];
            double realFPS = av_q2d(tsStream->r_frame_rate);
            double avgFPS = av_q2d(tsStream->avg_frame_rate);
            double codecFPS = (avgFPS > 0) ? avgFPS : realFPS;
            double stmTB = av_q2d(tsStream->time_base);
            int64_t stmFrames = tsStream->nb_frames;
            int64_t fmtFrames = int(formatTimeDur * codecFPS + 0.49);
            int64_t stmDuration = tsStream->duration;
            double frameDur = 1.0 / (stmTB * codecFPS);

            stmFrames =
                (stmFrames <= 0 && formatTimeDur > 0 && codecFPS > 0)
                    ? int(floor(formatTimeDur / (stmTB * double(frameDur))
                                + 0.5))
                    : stmFrames;
            stmFrames = (fmtFrames > 0) ? min(fmtFrames, stmFrames) : stmFrames;

            stmDuration = (stmDuration <= 0 && formatTimeDur > 0 && stmTB > 0)
                              ? int64_t(formatTimeDur / stmTB)
                              : stmDuration;

            DBL(DB_TIMESTAMPS,
                "stm: " << i << " stmFrames: " << stmFrames
                        << " stmDuration: " << stmDuration
                        << " stmTB: " << stmTB << " realFPS: " << realFPS
                        << " avgFPS: " << avgFPS << " codecFPS: " << codecFPS
                        << " frameDur: " << frameDur);

            if (tsStream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO
                && heroVideoTracks[i] && stmFrames > 0)
            {

                timeDuration = max(timeDuration, stmDuration * stmTB);
                frames = max(frames, stmFrames);
                AVRational tsRate = (avgFPS > 0) ? tsStream->avg_frame_rate
                                                 : tsStream->r_frame_rate;
                rate = (av_q2d(tsRate) > av_q2d(rate)) ? tsRate : rate;

                DBL(DB_TIMESTAMPS,
                    "stm: " << i << " fps: " << codecFPS << " frames: "
                            << frames << " frameDur: " << frameDur
                            << " timeDuration: " << timeDuration
                            << " start_time: " << tsStream->start_time);
            }
            // check if we are reading an image instead of a video
            else if (tsStream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO
                     && heroVideoTracks[i]
                     && isImageFormat(m_avFormatContext->iformat->name))
            {
                timeDuration = 1;
                frames = 1;
            }
            else if (tsStream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO
                     && heroAudioTracks[i])
            {
                if (stmDuration > 0 && stmTB > 0)
                {
                    // Notes about the values used here as specified by the
                    // FFmpeg documentation
                    // (https://www.ffmpeg.org/doxygen/3.0/structAVStream.html#a4e04af7a5a4d8298649850df798dd0bc)
                    // stmDuration = AVStream::duration = duration of the
                    // stream, in stream time base stmTB = AVStream::time_base =
                    // unit of time (in seconds)
                    const double durationInSeconds = stmDuration * stmTB;
                    audioLength = max(audioLength, durationInSeconds);
                }
                else
                {
                    double sampleRate = tsStream->codecpar->sample_rate;
                    if (stmFrames > 0 && sampleRate > 0)
                    {
                        double stmLength = double(stmFrames) / sampleRate;
                        audioLength = max(audioLength, stmLength);
                    }
                }
            }
        }
        if (timeDuration == 0)
        {
            if (audioLength > 0)
                timeDuration = audioLength;
            else
                timeDuration = formatTimeDur;
        }
        if (timeDuration <= 0)
        {
            TWK_THROW_EXC_STREAM(
                "Could not determine playback timing: " << m_filename);
        }
        if (frames == 0)
        {
            frames = timeDuration * m_io->defaultFPS();
            rate.num = m_io->defaultFPS();
            rate.den = 1.0f;
        }
        if (frames <= 0)
        {
            TWK_THROW_EXC_STREAM("Unable to locate frames: " << m_filename);
        }

        //
        // Set the MovieInfo based on the calculated frame rate
        //

        m_info.start = getFirstFrame(rate);
        m_info.end = m_info.start + frames - 1;
        m_info.inc = 1;
        m_info.fps =
            floor(av_q2d(rate) * FPS_PRECISION_LIMIT) / FPS_PRECISION_LIMIT;
    }

    bool MovieFFMpegReader::isImageFormat(const char* format)
    {
        if (!std::strcmp(format, "png_pipe")
            || !std::strcmp(format, "jpeg_pipe"))
        {
            return true;
        }
        return false;
    }

    ChannelsVector MovieFFMpegReader::idAudioChannels(AVChannelLayout layout,
                                                      int numChannels)
    {
        uint64_t unmasked;
        ChannelsVector chans;

        // Layout can have max 64 channels; one bit per channel. See
        // avutil/channel_layout.h
        for (int i = 0; i < 64 && chans.size() < numChannels; i++)
        {
            unmasked = layout.u.mask & (1ULL << i);

            if (!unmasked)
                continue;

            switch (unmasked)
            {
            case AV_CH_FRONT_LEFT:
                chans.push_back(FrontLeft);
                break;
            case AV_CH_FRONT_RIGHT:
                chans.push_back(FrontRight);
                break;
            case AV_CH_FRONT_CENTER:
                chans.push_back(FrontCenter);
                break;
            case AV_CH_LOW_FREQUENCY:
                chans.push_back(LowFrequency);
                break;
            case AV_CH_BACK_LEFT:
                chans.push_back(BackLeft);
                break;
            case AV_CH_BACK_RIGHT:
                chans.push_back(BackRight);
                break;
            case AV_CH_FRONT_LEFT_OF_CENTER:
                chans.push_back(FrontLeftOfCenter);
                break;
            case AV_CH_FRONT_RIGHT_OF_CENTER:
                chans.push_back(FrontRightOfCenter);
                break;
            case AV_CH_BACK_CENTER:
                chans.push_back(BackCenter);
                break;
            case AV_CH_SIDE_LEFT:
                chans.push_back(SideLeft);
                break;
            case AV_CH_SIDE_RIGHT:
                chans.push_back(SideRight);
                break;
            case AV_CH_TOP_CENTER:
                chans.push_back(FrontCenter);
                break; // GUESS
            case AV_CH_TOP_FRONT_LEFT:
                chans.push_back(LeftHeight);
                break; // GUESS
            case AV_CH_TOP_FRONT_CENTER:
                chans.push_back(FrontCenter);
                break; // GUESS
            case AV_CH_TOP_FRONT_RIGHT:
                chans.push_back(RightHeight);
                break; // GUESS
            case AV_CH_TOP_BACK_LEFT:
                chans.push_back(BackLeft);
                break; // GUESS
            case AV_CH_TOP_BACK_CENTER:
                chans.push_back(BackCenter);
                break; // GUESS
            case AV_CH_TOP_BACK_RIGHT:
                chans.push_back(BackRight);
                break; // GUESS
            case AV_CH_STEREO_LEFT:
                chans.push_back(FrontLeft);
                break; // GUESS
            case AV_CH_STEREO_RIGHT:
                chans.push_back(FrontRight);
                break; // GUESS
            case AV_CH_WIDE_LEFT:
                chans.push_back(SideLeft);
                break; // GUESS
            case AV_CH_WIDE_RIGHT:
                chans.push_back(SideRight);
                break; // GUESS
            case AV_CH_SURROUND_DIRECT_LEFT:
                chans.push_back(SideLeft);
                break; // GUESS
            case AV_CH_SURROUND_DIRECT_RIGHT:
                chans.push_back(SideRight);
                break; // GUESS
            case AV_CH_LOW_FREQUENCY_2:
                chans.push_back(LowFrequency);
                break; // GUESS
            default:
                continue;
            }

            AVChannel avChannel =
                av_channel_layout_channel_from_index(&layout, i);
            std::array<char, 64> chName;
            std::array<char, 256> chDesc;

            // av_channel_name and av_channel_description functions
            // return amount of bytes needed to hold the output string,
            // or a negative AVERROR on failure.
            if (av_channel_name(chName.data(), chName.size(), avChannel) > 0
                && av_channel_description(chDesc.data(), chDesc.size(),
                                          avChannel)
                       > 0)
            {
                DBL(DB_AUDIO, "Audio ch " << (chans.size() - 1) << " is: '"
                                          << chName.data() << "-"
                                          << chDesc.data());
            }
        }

        // If we don't know any of the channels then assume the most common
        // layout
        if (chans.empty())
        {
            chans = layoutChannels(channelLayouts(numChannels).front());
        }

        // Rest of the channels are unknown.
        for (int ch = chans.size(); ch < numChannels; ch++)
        {
            chans.push_back(UnknownChannel);
        }

        return chans;
    }

    size_t MovieFFMpegReader::audioFillBuffer(const AudioReadRequest& request,
                                              AudioBuffer& buffer)
    {
        DBL(DB_AUDIO, "AUDIO_FILL_BUFFER " << m_filename);

        double sourceRate = m_info.audioSampleRate;
        SampleTime margin = request.startTime == 0 ? 0 : request.margin;
        Time formatStart = m_formatStartFrame / m_info.fps;
        SampleTime start =
            timeToSamples(request.startTime + formatStart, sourceRate) - margin;
        SampleTime num =
            timeToSamples(request.duration, sourceRate) + margin * 2;

        //
        // If this source never built its AudioState from the actual graph audio
        // state, then we need to do so now. This most commonly happens when a
        // new source is added after RV has been open for a while. The
        // conditions in which we need to force audioConfigure are when the
        // m_audioState is not valid or if we are in control of the output
        // channel mix and the audio state we have does not match what was
        // requested.
        //

        if (!m_audioState
            || (canConvertAudioChannels()
                && m_audioState->channels != buffer.channels()))
        {
            Layout layout = channelLayout(buffer.channels());
            Movie::AudioConfiguration conf(buffer.rate(), layout, num);
            audioConfigure(conf);
        }

        ChannelsVector channels = (canConvertAudioChannels())
                                      ? buffer.channels()
                                      : m_info.audioChannels;

        if (channels.empty())
        {
            cerr << "WARNING: unsupported number of audio tracks." << endl;
            return 0;
        }

        buffer.reconfigure(num, channels, sourceRate, request.startTime);
        buffer.zero();

        DBL(DB_AUDIO,
            "margin: " << margin << " start: " << start << " num: " << num
                       << " convert: " << canConvertAudioChannels()
                       << " bufChans: " << buffer.channels().size()
                       << " startTime: " << request.startTime << " duration: "
                       << request.duration << " srcRate: " << sourceRate
                       << " srcChans: " << m_info.audioChannels.size()
                       << " channelsPerTrack: "
                       << m_audioState->channelsPerTrack);

        vector<TwkAudio::SampleVector> chbuffers(m_audioTracks.size());
        SampleTime maxCollected = 0;
        for (int i = 0; i < m_audioTracks.size(); i++)
        {
            if (start < 0)
                continue;
            AudioTrack* track = m_audioTracks[i];
            ContextPool::Reservation reserve(this, track->number);
            track->isOpen = openAVCodec(track->number, &track->avCodecContext);
            if (!track->isOpen)
            {
                cout << "ERROR: Unable to read from audio stream: "
                     << track->number << endl;
                continue;
            }
            AVStream* audioStream = m_avFormatContext->streams[track->number];

            DBL(DB_AUDIO,
                "nb_frames: " << audioStream->nb_frames
                              << " timebase: " << av_q2d(audioStream->time_base)
                              << " start: " << start);

            chbuffers[i].resize(num * m_audioState->channelsPerTrack);
            SampleTime collected = 0;
            SampleTime retrieved = 0;
            track->start = start;
            track->desired = num;
            track->bufferPointer = &(chbuffers[i].front());
            do
            {
                retrieved = oneTrackAudioFillBuffer(track);
                collected += retrieved;
                track->desired -= retrieved;
                track->start += retrieved;
                track->bufferPointer +=
                    m_audioState->channelsPerTrack * retrieved;

                DBL(DB_AUDIO, "track: " << track->number
                                        << " numChans: " << track->numChannels
                                        << " collected: " << collected
                                        << " retrieved: " << retrieved
                                        << " readBeginning: " << track->start
                                        << " desired: " << track->desired);
            } while (track->desired > 0 && retrieved > 0);
            maxCollected = max(collected, maxCollected);

#if DB_AUDIO & DB_LEVEL
            if (m_audioTracks.size() > 1)
            {
                DBL(DB_AUDIO, "finished track " << track->number << "\n");
            }
#endif
        }

        //
        // Note: When there is only one AudioTrack this call to interlace
        // merely executes a memcpy of the pre-interlaced single track audio
        // into the final output buffer.
        //

        size_t sampsPerTrack = maxCollected * m_audioState->channelsPerTrack;
        interlace(chbuffers, buffer.pointer(), 0, sampsPerTrack);

        DBL(DB_AUDIO, "Done (" << maxCollected << "/" << num
                               << ") and heading home!!!\n");

        return maxCollected;
    }

    SampleTime MovieFFMpegReader::oneTrackAudioFillBuffer(AudioTrack* track)
    {
        AVStream* audioStream = m_avFormatContext->streams[track->number];
        AVCodecContext* audioCodecContext = track->avCodecContext;
        double timebase = av_q2d(audioStream->time_base);

        DBL(DB_AUDIO,
            "start: " << track->start << " timebase: " << timebase
                      << " lastDecodedAudio: " << track->lastDecodedAudio
                      << " bufferLength: " << track->bufferLength
                      << " bufferStart: " << track->bufferStart
                      << " bufferEnd: " << track->bufferEnd);

        if (track->start > (track->bufferEnd + track->bufferLength)
            || track->start < track->lastDecodedAudio
            || track->lastDecodedAudio == AV_NOPTS_VALUE
            || (m_audioTracks.size() > 1 && track->start > track->bufferEnd))
        {
            int64_t seekTarget = int64_t(double(track->start - 1)
                                         / (m_info.audioSampleRate * timebase));

            DBL(DB_AUDIO, "seekTarget: " << seekTarget);

            avcodec_flush_buffers(audioCodecContext);
            if (av_seek_frame(m_avFormatContext, track->number, seekTarget,
                              AVSEEK_FLAG_BACKWARD)
                < 0)
            {
                // Try from the start if targeted seek fails
                if (av_seek_frame(m_avFormatContext, -1,
                                  m_avFormatContext->start_time, 0)
                    < 0)
                {
                    TWK_THROW_EXC_STREAM(
                        "av_seek_frame failed in audio stream.");
                }
            }
            track->lastDecodedAudio = AV_NOPTS_VALUE;
            track->bufferStart = AV_NOPTS_VALUE;
            track->bufferEnd = AV_NOPTS_VALUE;
        }
        else
        {
            if (track->start <= track->bufferEnd)
            {
                DBL(DB_AUDIO, "draining remainder start: "
                                  << track->start
                                  << " desired: " << track->desired
                                  << " bufferStart: " << track->bufferStart
                                  << " bufferEnd: " << track->bufferEnd);

                return decodeAudioForBuffer(track);
            }
        }

        //
        // Keep collecting audio samples while the samples we are looking for
        // are in the range we are requesting.
        //

        int frameFinished = 0;
        bool finalPacket = false;
        do
        {
            // If the current packet is depleted or we think this is the last
            // packet and we want to make sure it is
            if (track->start > track->bufferEnd || finalPacket)
            {
                DBL(DB_AUDIO, "searching for audio");

                // If there is nothing left to read return zeros
                if (finalPacket)
                    return track->desired;

                // Loop to make sure we only read from the correct AVStream
                do
                {
                    DBL(DB_AUDIO, "freeing audio packet from stream: "
                                      << track->audioPacket->stream_index);
                    av_packet_unref(track->audioPacket);
                    if (av_read_frame(m_avFormatContext, track->audioPacket)
                        < 0)
                    {
                        finalPacket = true;
                    }
                } while ((track->audioPacket->stream_index != track->number)
                         && !finalPacket);
            }

            DBL(DB_AUDIO, "pkt: " << track->audioPacket->stream_index
                                  << " trk: " << track->number
                                  << " dts: " << track->audioPacket->dts
                                  << " pts: " << track->audioPacket->pts
                                  << " size: " << track->audioPacket->size
                                  << " dur: " << track->audioPacket->duration
                                  << " final: " << finalPacket);

            avcodec_send_packet(audioCodecContext, track->audioPacket);
            // Loop we normally pass through once unless draining the last
            // packet
            do
            {
                frameFinished =
                    avcodec_receive_frame(audioCodecContext, track->audioFrame)
                    == 0;
                if (!frameFinished)
                {
                    break;
                }
                if (frameFinished)
                {
                    track->bufferLength = track->audioFrame->nb_samples;
                    track->bufferStart = track->bufferEnd + 1;
                    track->bufferEnd += track->bufferLength;
                }

                DBL(DB_AUDIO,
                    "pkt: " << track->audioPacket->stream_index
                            << " frmDts: " << track->audioFrame->pkt_dts
                            << " frmPts: " << track->audioFrame->pts
                            << " frmSize: " << track->audioFrame->pkt_size
                            << " bufferStart: " << track->bufferStart
                            << " bufferEnd: " << track->bufferEnd
                            << " bufferLength: " << track->bufferLength
                            << " last: " << track->lastDecodedAudio
                            << " start: " << track->start
                            << " desired: " << track->desired
                            << " frameFinished: " << frameFinished
                            << " frmDur: " << track->audioFrame->pkt_duration
                            << " nb_samples: " << track->audioFrame->nb_samples
                            << " sampleRate: " << track->audioFrame->sample_rate
                            << " linesize: " << track->audioFrame->linesize[0]
                            << " size: " << track->audioPacket->size);

                if (track->lastDecodedAudio == AV_NOPTS_VALUE && frameFinished
                    && track->audioFrame->pkt_dts != AV_NOPTS_VALUE)
                {
                    track->bufferStart =
                        int64_t((track->audioFrame->pkt_dts * timebase
                                 * m_info.audioSampleRate)
                                + 0.49);
                    track->bufferEnd =
                        track->bufferStart + track->bufferLength - 1;
                    track->lastDecodedAudio = track->bufferStart - 1;

                    DBL(DB_AUDIO,
                        "last: " << track->lastDecodedAudio
                                 << " bufferStart: " << track->bufferStart
                                 << " bufferEnd: " << track->bufferEnd
                                 << " bufferLength: " << track->bufferLength
                                 << " start: " << track->start
                                 << " desired: " << track->desired);

                    if (track->bufferStart > track->start + track->desired)
                        return track->desired;
                    if (track->bufferStart > track->start)
                    {
                        return track->bufferStart - track->start;
                    }
                    if (track->bufferEnd < track->start)
                    {
                        frameFinished = false;
                        track->lastDecodedAudio = AV_NOPTS_VALUE;
                    }
                }
            } while (!frameFinished && finalPacket);
        } while (!frameFinished);

        //
        // If we have a completed read and have collected some samples to
        // save, then capture what we've read.
        //

        return decodeAudioForBuffer(track);
    }

    int MovieFFMpegReader::decodeAudioForBuffer(AudioTrack* track)
    {
        AVCodecContext* audioCodecContext = track->avCodecContext;
        AVSampleFormat audioFormat = audioCodecContext->sample_fmt;

        int collected = 0;
        switch (audioFormat)
        {
        case AV_SAMPLE_FMT_U8:
        case AV_SAMPLE_FMT_U8P:
            collected = translateAVAudio<unsigned char>(track, CHAR_MAX, 127);
            break;
        case AV_SAMPLE_FMT_S16:
        case AV_SAMPLE_FMT_S16P:
            collected = translateAVAudio<short>(track, SHRT_MAX, 0);
            break;
        case AV_SAMPLE_FMT_S32:
        case AV_SAMPLE_FMT_S32P:
            collected = translateAVAudio<int>(track, INT_MAX, 0);
            break;
        case AV_SAMPLE_FMT_FLT:
        case AV_SAMPLE_FMT_FLTP:
            collected = translateAVAudio<float>(track, 1.0, 0);
            break;
        case AV_SAMPLE_FMT_DBL:
        case AV_SAMPLE_FMT_DBLP:
            collected = translateAVAudio<double>(track, 1.0, 0);
            break;
        case AV_SAMPLE_FMT_NONE:
        default:
            TWK_THROW_EXC_STREAM("Unsupported audio format.");
        }
        return collected;
    }

    FrameBuffer* MovieFFMpegReader::configureYUVPlanes(
        FrameBuffer::DataType dataType, int width, int height, int rowSpan,
        int rowSpanUV, int usampling, int vsampling, bool addAlpha,
        FrameBuffer::Orientation orientation)
    {
        DBL(DB_VIDEO, "dataType: " << dataType << " rowSpan: " << rowSpan
                                   << " rowSpanUV: " << rowSpanUV
                                   << " usampling: " << usampling
                                   << " vsampling: " << vsampling);

        //
        // Pad the height and width of the Y Plane to make sure its dimensions
        // are euqally divisable by 8.
        //

        int rowModulo = 8;
        int extraScanlines =
            height % rowModulo == 0 ? 0 : (rowModulo - height % rowModulo);
        int colWidth = (dataType == FrameBuffer::UCHAR) ? 1 : 2;
        int extraScanlinePixels = rowSpan / colWidth - width;

        DBL(DB_VIDEO,
            "rowModulo: " << rowModulo << " extraScanlines: " << extraScanlines
                          << " extraScanlinePixels: " << extraScanlinePixels);

        FrameBuffer::StringVector YChannelName(1, "Y");
        FrameBuffer* Y =
            new FrameBuffer(FrameBuffer::PixelCoordinates, width, height, 0, 1,
                            dataType, NULL, &YChannelName, orientation, true,
                            extraScanlines, extraScanlinePixels);

        //
        // Divide the width and height of the source resultion by the respective
        // sampling factor. If you have a 42* pixel format than the width of U
        // and V planes will be half of the resolution of the Y plane. Make sure
        // to pad the dimensions of the U and V planes to be equally divisable
        // by 8.
        //

        int uvWidth = (width + usampling - 1) / usampling;
        int uvHeight = (height + vsampling - 1) / vsampling;
        int uvExtraScanlines =
            uvHeight % rowModulo == 0 ? 0 : rowModulo - (uvHeight % rowModulo);
        int uvExtraScanlinePixels = rowSpanUV / colWidth - uvWidth;

        DBL(DB_VIDEO,
            "uvWidth: " << uvWidth << " uvHeight: " << uvHeight
                        << " uvExtraScanlines: " << uvExtraScanlines
                        << " uvExtraScanlinePixels: " << uvExtraScanlinePixels);

        FrameBuffer::StringVector UChannelName(1, "U");
        FrameBuffer* U =
            new FrameBuffer(FrameBuffer::PixelCoordinates, uvWidth, uvHeight, 0,
                            1, dataType, NULL, &UChannelName, orientation, true,
                            uvExtraScanlines, uvExtraScanlinePixels);

        FrameBuffer::StringVector VChannelName(1, "V");
        FrameBuffer* V =
            new FrameBuffer(FrameBuffer::PixelCoordinates, uvWidth, uvHeight, 0,
                            1, dataType, NULL, &VChannelName, orientation, true,
                            uvExtraScanlines, uvExtraScanlinePixels);

        Y->appendPlane(U);
        Y->appendPlane(V);

        if (addAlpha)
        {
            FrameBuffer::StringVector AChannelName(1, "A");
            FrameBuffer* A =
                new FrameBuffer(FrameBuffer::PixelCoordinates, width, height, 0,
                                1, dataType, NULL, &AChannelName, orientation,
                                true, extraScanlines, extraScanlinePixels);
            Y->appendPlane(A);
        }

        return Y;
    }

    void MovieFFMpegReader::seekToFrame(const int inframe,
                                        const double frameDur,
                                        AVStream* videoStream,
                                        VideoTrack* track)
    {
#if DB_TIMING & DB_LEVEL
        m_timingDetails->startTimer("seek");
#endif

        // To make seeking a bit more robust, step back 3 frames from the
        // intended frame. This is because FFmpeg internally seeks with the dts
        // timestamp and not the pts timestamp, so we need to have a bit of a
        // buffer.
        const int64_t seekTarget = int64_t((inframe - 3) * frameDur);

        DBL(DB_VIDEO, "seekTarget: " << seekTarget
                                     << " last: " << track->lastDecodedVideo
                                     << " inMinus1: " << inframe - 1);

        avcodec_send_packet(track->avCodecContext, nullptr);

        int ret = 0;
        while (ret != AVERROR_EOF)
            ret =
                avcodec_receive_frame(track->avCodecContext, track->videoFrame);

        avcodec_flush_buffers(track->avCodecContext);
        if (av_seek_frame(m_avFormatContext, track->number, seekTarget,
                          AVSEEK_FLAG_BACKWARD)
            < 0)
        {
            // Try from the start if targeted seek fails
            if (av_seek_frame(m_avFormatContext, -1,
                              m_avFormatContext->start_time, 0)
                < 0)
            {
                TWK_THROW_EXC_STREAM("av_seek_frame failed in video stream.");
            }
        }

        //
        // The last decoded video is no longer relevant nor is the surrounding
        // list of timestamps.
        //
        track->lastDecodedVideo = -1;
        track->tsSet.clear();

#if DB_TIMING & DB_LEVEL
        double seekDuration = m_timingDetails->pauseTimer("seek");
        DBL(DB_TIMING, "frame: " << inframe << " seekTime: " << seekDuration);
#endif
    }

    bool MovieFFMpegReader::readPacketFromStream(const int inframe,
                                                 VideoTrack* track)
    {
        bool finalPacket = false;

        // Loop to make sure we only read from the correct AVStream
        do
        {
            DBL(DB_VIDEO, "freeing video packet from stream: "
                              << track->videoPacket->stream_index);
            av_packet_unref(track->videoPacket);
            HOP_PROF("av_read_frame()");
            if (av_read_frame(m_avFormatContext, track->videoPacket) < 0)
            {
                // If this is still in the range of valid frames we are at the
                // last packet
                if ((inframe > 0) && (inframe + m_info.start - 1 <= m_info.end))
                {
                    finalPacket = true;
                }
                else
                {
                    TWK_THROW_EXC_STREAM(
                        "av_read_frame failed in video stream.");
                }
            }
#if DB_LEVEL & DB_SUBTITLES
            if (m_subtitleMap.find(track->videoPacket->stream_index)
                    != m_subtitleMap.end()
                && correctLang(track->videoPacket->stream_index))
            {
                readSubtitle(track);
            }
#endif
        } while ((track->videoPacket->stream_index != track->number)
                 && !finalPacket);

        // If we get a valid timestamp here then we should add it to
        // the track's list of nearby timestamps.
        if (track->videoPacket->pts != AV_NOPTS_VALUE)
            track->tsSet.insert(track->videoPacket->pts);
        else if (track->videoPacket->dts != AV_NOPTS_VALUE)
            track->tsSet.insert(track->videoPacket->dts);

#if DB_LEVEL & DB_VIDEO
        ostringstream tss;
        for (set<int64_t>::iterator p = track->tsSet.begin();
             p != track->tsSet.end(); p++)
        {
            tss << " " << *p;
        }
        DBL(DB_VIDEO, "tss: " << tss.str());
#endif

        return finalPacket;
    }

    void MovieFFMpegReader::sendPacketToDecoder(VideoTrack* track)
    {
        int ret =
            avcodec_send_packet(track->avCodecContext, track->videoPacket);
        if (ret < 0)
        {
            if (ret == AVERROR(EAGAIN))
            {
                // This occurs when we need to read more output before sending a
                // new one. However, since we fully drain the output in each
                // decode call, this should never happen.
                TWK_THROW_EXC_STREAM("avcodec_send_packet failed in video "
                                     "stream: AVERROR(EAGAIN)");
            }
            else
            {
                TWK_THROW_EXC_STREAM(
                    "avcodec_send_packet failed in video stream");
            }
        }
    }

    bool MovieFFMpegReader::findImageWithBestTimestamp(int inframe,
                                                       double frameDur,
                                                       AVStream* videoStream,
                                                       VideoTrack* track)
    {
        // The goal timestamp is the same as the seek target adjusted
        // for pts vs dts.
        const int64_t goalTS = (inframe - 1) * frameDur;

        // If it is the first time we try to decode the stream, we need
        // to start by reading a packet.
        if (track->tsSet.empty())
        {
            // First we need to read a packet from the stream.
            readPacketFromStream(inframe, track);

            // Then we need to send the packet as input to the decoder.
            sendPacketToDecoder(track);
        }
        // Look for the best (closest to the goal) timestamp in the updated
        // list.
        int64_t bestTS = findBestTS(goalTS, frameDur, track, false);

        bool searching = true;
        while (searching)
        {
            HOP_PROF("avcodec_receive_frame()");
            int ret =
                avcodec_receive_frame(track->avCodecContext, track->videoFrame);
            if (ret < 0)
            {
                if (ret == AVERROR(EAGAIN))
                {
                    // This is valid. This occurs when we need more input.
                    // Let's supply a new packet to the codec and update the
                    // best timestamp.
                    readPacketFromStream(inframe, track);
                    sendPacketToDecoder(track);
                    bestTS = findBestTS(goalTS, frameDur, track, false);
                    continue;
                }
                else
                {
                    // Let's throw when other errors occur.
                    TWK_THROW_EXC_STREAM("Could not decode video: rcv error");
                }
            }
            else
            {
                // Figure out if we got a good timestamp. Whether we are using
                // pts ot dts for this format calculate the frame this decode
                // represents for RV.
                const int64_t pktPTS = track->videoFrame->pts;
                const int64_t pktDTS = track->videoFrame->pkt_dts;
                const int64_t lastTS =
                    (pktPTS == AV_NOPTS_VALUE) ? pktDTS : pktPTS;
                track->lastDecodedVideo = int(double(lastTS) / frameDur + 1.49);

                // If the last timestamp is now equal to or greater than either
                // the best possible timestamp for inframe or the last of the
                // stream then we are no longer searching.
                searching = (lastTS < bestTS);

#if DB_VIDEO & DB_LEVEL
                int64_t startPTS = (videoStream->start_time != AV_NOPTS_VALUE)
                                       ? videoStream->start_time
                                       : 0;
#endif
                DBL(DB_VIDEO, "frameDur: " << frameDur
                                           << " startPTS: " << startPTS
                                           << " done: " << int(searching));
                DBL(DB_VIDEO,
                    "pktPTS: " << pktPTS << " pktDTS: " << pktDTS
                               << " goalTS: " << goalTS << " goal: "
                               << int(double(goalTS) / frameDur + 1.49)
                               << " bestTS: " << bestTS << " best: "
                               << int(double(bestTS) / frameDur + 1.49)
                               << " lastTS: " << lastTS
                               << " last: " << track->lastDecodedVideo);
            }
        }
        return true;
    }

    FrameBuffer* MovieFFMpegReader::decodeImageAtFrame(int inframe,
                                                       VideoTrack* track)
    {
        if (m_mustReadFirstFrame)
        {
            // Force reading of the first frame of the stream to ensure
            // header (SPS and PPS) packets to be read.
            m_mustReadFirstFrame = false;
            if (1 != inframe)
            {
                auto firstFrame = decodeImageAtFrame(1, track);
                if (firstFrame)
                    delete firstFrame;
            }
        }

        //
        // Get timing basics
        //
        HOP_PROF_FUNC();

        AVStream* videoStream = m_avFormatContext->streams[track->number];
        AVCodecContext* videoCodecContext = track->avCodecContext;
        double frameDur = double(videoStream->time_base.den)
                          / (double(videoStream->time_base.num) * m_info.fps);

        DBL(DB_VIDEO, "stream duration: "
                          << videoStream->duration
                          << " num frames: " << videoStream->nb_frames
                          << " time base: " << av_q2d(videoStream->time_base)
                          << " (" << videoStream->time_base.num << "/"
                          << videoStream->time_base.den
                          << ") start time: " << videoStream->start_time
                          << " frameDur: " << frameDur);

        // Seek to requested frame if necessary
        // If this is not the frame after the last one we decoded then we need
        // to seek to the relevant place and start decoding there. Note that
        // unnecessary seeking, especially during long GOP decoding, has a
        // significant negative impact on performances. Therefore we will not
        // seek unless it is required. Optimization: We will not seek if the
        // last decoded frame is within a Group Of Picture (GOP) size distance.
        // Note that videoCodecContext->gop_size is 0 for intra-frame
        // compression Note that we also check for inter-frame compression
        // codecs (m_info.slowRandomAccess) since we cannot blindly rely on
        // videoCodecContext->gop_size because it is initialized by default by
        // FFmpeg with a default value of 12 even for intra-frame compression
        // codecs (such as Apple Pro Res for example).
        const int nearFrameThreshold =
            (m_info.slowRandomAccess && videoCodecContext->gop_size != 0)
                ? videoCodecContext->gop_size
                : 1;
        if (track->lastDecodedVideo == -1 || track->lastDecodedVideo >= inframe
            || track->lastDecodedVideo < (inframe - nearFrameThreshold))
        {
            seekToFrame(inframe, frameDur, videoStream, track);
        }

        //
        // Find the best suitable frame
        //

#if DB_TIMING & DB_LEVEL
        m_timingDetails->startTimer("decode");
#endif

        const bool frameFinished =
            findImageWithBestTimestamp(inframe, frameDur, videoStream, track);

        // Remove earlier timestamps
        int64_t prune = (inframe - 2) * frameDur;
        set<int64_t>::iterator pruneIT;
        for (pruneIT = track->tsSet.begin(); pruneIT != track->tsSet.end();
             pruneIT++)
        {
            if (*pruneIT > prune)
            {
                if (pruneIT != track->tsSet.begin())
                    pruneIT--;
                break;
            }
        }
        track->tsSet.erase(track->tsSet.begin(), pruneIT);

#if DB_TIMING & DB_LEVEL
        double decodeDuration = m_timingDetails->pauseTimer("decode");
        DBL(DB_TIMING,
            "frame: " << inframe << " decodeTime: " << decodeDuration);
#endif

        const int width = (track->rotate) ? m_info.height : m_info.width;
        const int height = (track->rotate) ? m_info.width : m_info.height;

        // Did we get what we came here for?
        if (!frameFinished || track->videoFrame->width < width
            || track->videoFrame->height < height)
        {
            TWK_THROW_EXC_STREAM("Unable to get frame at: " << inframe);
        }

        //
        // Either we got something we can handle natively or we need to scale or
        // convert it
        //

        snagVideoFrameType(track);

        //
        // track->videoFrame - AVFrame to read the track->videoPacket into
        // outFrame          - AVFrame linked to output FrameBuffer to copy into
        //

        AVFrame* outFrame = 0;
        outFrame = av_frame_alloc();

        //
        // Determine the native pixel format and make an effort to reconfigure
        // the frame buffer we plan to return to match so we don't have to scale
        // or convert any decoded frames. If the format is unknown or not
        // handled then we will have to convert so keep track if the format
        // needs conversion.
        //

        AVPixelFormat nativeFormat = videoCodecContext->pix_fmt;
        const AVPixFmtDescriptor* desc = av_pix_fmt_desc_get(nativeFormat);
        int bitSize = desc->comp[0].depth - desc->comp[0].shift;
        int numPlanes = 0;
        bool hasAlpha = (desc->flags & AV_PIX_FMT_FLAG_ALPHA);
        bool isPlanar = (desc->flags & AV_PIX_FMT_FLAG_PLANAR);
        bool isRGB = (desc->flags & AV_PIX_FMT_FLAG_RGB);
        bool convertFormat = false;
        FrameBuffer::DataType dataType =
            (bitSize > 8) ? FrameBuffer::USHORT : FrameBuffer::UCHAR;
        FrameBuffer::StringVector chans(3);
        FrameBuffer* out = 0;
        av_image_fill_arrays(outFrame->data, outFrame->linesize, 0,
                             nativeFormat, width, height, 1);

        // Note: We're about to use offset_plus1 here to derive the RGB channel
        // ordering. Here is the definition of offset_plus1 according to the
        // FFmpeg documentation: Number of elements before the component of the
        // first pixel + 1. Elements are *bytes*. For an RGB24 format, you will
        // get offset_plus1 = {1,2,3} For an RGB48 format, you will get
        // offset_plus1 = {1,3,5}
        const int nbElementsPerChannel = max(1, (desc->comp[0].depth + 7) / 8);

        switch (nativeFormat)
        {
        case AV_PIX_FMT_RGBA64:
        case AV_PIX_FMT_BGRA64:
        case AV_PIX_FMT_ARGB:
        case AV_PIX_FMT_RGBA:
        case AV_PIX_FMT_ABGR:
        case AV_PIX_FMT_BGRA:
            chans.resize(4);
            {
                const int maxOffset = static_cast<int>(chans.size()) - 1;
                int offset3 =
                    max(0, min(maxOffset,
                               desc->comp[3].offset / nbElementsPerChannel));
                chans[offset3] = "A";
            }
        case AV_PIX_FMT_RGB48:
        case AV_PIX_FMT_BGR48:
        case AV_PIX_FMT_RGB24:
        case AV_PIX_FMT_BGR24:
        {
            const int maxOffset = static_cast<int>(chans.size()) - 1;
            int offset0 = max(
                0, min(maxOffset, desc->comp[0].offset / nbElementsPerChannel));
            int offset1 = max(
                0, min(maxOffset, desc->comp[1].offset / nbElementsPerChannel));
            int offset2 = max(
                0, min(maxOffset, desc->comp[2].offset / nbElementsPerChannel));
            chans[offset0] = "R";
            chans[offset1] = "G";
            chans[offset2] = "B";
        }
            out = new FrameBuffer(width, height, chans.size(), dataType, NULL,
                                  &chans);
            break;
        default:
            nativeFormat = getBestRVFormat(nativeFormat);
            if (isPlanar && !isRGB)
            {
                convertFormat = (bitSize != 8);
                numPlanes = av_pix_fmt_count_planes(nativeFormat);
                int log2w, log2h;
                av_pix_fmt_get_chroma_sub_sample(nativeFormat, &log2w, &log2h);
                int usampling = int(pow(2.0f, log2w));
                int vsampling = int(pow(2.0f, log2h));
                out = configureYUVPlanes(
                    dataType, width, height, outFrame->linesize[0],
                    outFrame->linesize[1], usampling, vsampling, hasAlpha,
                    track->fb.orientation());
            }
            else
            {
                convertFormat = true;
                av_image_fill_arrays(outFrame->data, outFrame->linesize, 0,
                                     nativeFormat, width, height, 1);
                out = new FrameBuffer(width, height, (hasAlpha) ? 4 : 3,
                                      dataType);
            }
            break;
        }

        // Assign the AVFrame data to our frame buffer
        outFrame->data[0] = out->pixels<unsigned char>();
        FrameBuffer* fb = out->nextPlane();
        for (int p = 1; p < numPlanes && fb; p++, fb = fb->nextPlane())
        {
            outFrame->data[p] = fb->pixels<unsigned char>();
        }

        if (convertFormat)
        {
            DBL(DB_VIDEO, "Converting video format");

            // Reuse or allocate a new image conversion context
            AVPixelFormat original = videoCodecContext->pix_fmt;
            AVPixelFormat conv = getBestRVFormat(original);
            HOP_PROF("sws_getCachedContext()");
            // Note: If track->imgConvertContext is NULL, sws_getCachedContext
            // just calls sws_getContext() to get a new context. Otherwise, it
            // checks if the parameters are the ones already saved in context.
            // If that is the case, it returns the current context. Otherwise,
            // it frees context and gets a new context with the new parameters.
            track->imgConvertContext = sws_getCachedContext(
                track->imgConvertContext, width, height, original, width,
                height, conv, SWS_BICUBIC, 0, 0, 0);
            if (track->imgConvertContext == 0)
            {
                TWK_THROW_EXC_STREAM("Can't initialize conversion context!");
            }

            HOP_PROF("sws_scale()");
            sws_scale(track->imgConvertContext, track->videoFrame->data,
                      track->videoFrame->linesize, 0, height, outFrame->data,
                      outFrame->linesize);
        }
        else
        {
            av_image_copy(outFrame->data, outFrame->linesize,
                          track->videoFrame->data, track->videoFrame->linesize,
                          videoCodecContext->pix_fmt, width, height);
        }

        // Clean up
        av_frame_free(&outFrame);

        //
        // XXX For now we will handle any rotation in software. Eventually we
        // plan to handle this with the renderer in hardware.
        //

        if (track->rotate)
        {
            HOP_PROF("track->rotate");
            FrameBuffer* Y = out->firstPlane();
            FrameBuffer* U = out->nextPlane();
            FrameBuffer* V = U->nextPlane();

            FrameBuffer::StringVector YChannelName(1, "Y");
            FrameBuffer* Yrot =
                new FrameBuffer(FrameBuffer::PixelCoordinates, Y->height(),
                                Y->width(), 0, 1, dataType, NULL, &YChannelName,
                                track->fb.orientation(), true, 0, 0);

            FrameBuffer::StringVector UChannelName(1, "U");
            FrameBuffer* Urot =
                new FrameBuffer(FrameBuffer::PixelCoordinates, U->height(),
                                U->width(), 0, 1, dataType, NULL, &UChannelName,
                                track->fb.orientation(), true, 0, 0);

            FrameBuffer::StringVector VChannelName(1, "V");
            FrameBuffer* Vrot =
                new FrameBuffer(FrameBuffer::PixelCoordinates, V->height(),
                                V->width(), 0, 1, dataType, NULL, &VChannelName,
                                track->fb.orientation(), true, 0, 0);

            rowColumnSwap(Y->pixels<unsigned char>(), Y->width(), Y->height(),
                          Yrot->pixels<unsigned char>());
            rowColumnSwap(U->pixels<unsigned char>(), U->width(), U->height(),
                          Urot->pixels<unsigned char>());
            rowColumnSwap(V->pixels<unsigned char>(), V->width(), V->height(),
                          Vrot->pixels<unsigned char>());

            Yrot->appendPlane(Urot);
            Yrot->appendPlane(Vrot);

            delete out;
            out = Yrot;
        }

        //
        // Add Timecode if necessary
        //

        if (m_timecodeTrack != -1)
        {
            AVStream* tsStream = m_avFormatContext->streams[m_timecodeTrack];
            AVDictionaryEntry* tcEntry;
            tcEntry = av_dict_get(tsStream->metadata, "timecode", NULL, 0);
            AVRational rate = {tsStream->time_base.den,
                               tsStream->time_base.num};

            if (isMOVformat(m_avFormatContext))
            {
                rate = tsStream->avg_frame_rate;
            }

            // Correct wrong frame rates that seem to be generated by some
            // codecs
            if (rate.num > 1000 && rate.den == 1)
            {
                rate.den = 1000;
            }

            AVTimecode avTimecode;
            av_timecode_init_from_string(&avTimecode, rate, tcEntry->value,
                                         m_avFormatContext);
            char tcString[80];
            av_timecode_make_string(&avTimecode, tcString, inframe - 1);
            track->fb.attribute<string>("Timecode") = string(tcString);
        }

        return out;
    }

    void MovieFFMpegReader::imagesAtFrame(const ReadRequest& request,
                                          FrameBufferVector& fbs)
    {
        int64_t inframe = request.frame;
        if (inframe < m_info.start)
            inframe = m_info.start;
        if (inframe > m_info.end)
            inframe = m_info.end;
        inframe = inframe - m_info.start + 1;

#if DB_TIMING & DB_LEVEL
        m_timingDetails->startTimer("imagesAtFrame");
#endif

        DBL(DB_VIDEO, "IMAGES_AT_FRAME: " << inframe);

        //
        // Find the track corresponding to the requested view/eye and decode it
        //

        int64_t decodeFrame = inframe + m_formatStartFrame;
        bool searchNames = !request.views.empty();
        for (unsigned int i = 0; i < m_videoTracks.size(); i++)
        {
            VideoTrack* track = m_videoTracks[i];
            ContextPool::Reservation reserve(this, track->number);
            track->isOpen = openAVCodec(track->number, &track->avCodecContext);
            if (!track->isOpen)
            {
                cout << "ERROR: Unable to read from audio stream: "
                     << track->number << endl;
                continue;
            }

            if (!searchNames && !request.stereo && i > 0)
                break;
            if (request.stereo && i > 1)
                break;

            bool found = true;
            if (request.views.size())
            {
                found = false;

                for (unsigned int q = 0; q < request.views.size(); q++)
                {
                    if (request.views[q] == track->name)
                    {
                        found = true;
                        break;
                    }
                }
            }

            if (!found)
                continue;

            FrameBuffer* out = decodeImageAtFrame(decodeFrame, track);
            fbs.push_back(out);

            track->fb.appendAttributesAndPrefixTo(out, "");
            out->setIdentifier("");
            identifier(inframe, out->idstream());
            out->idstream() << "/" << i;
            out->setOrientation(track->fb.orientation());
            out->setPixelAspectRatio(track->fb.pixelAspectRatio());
        }

#if DB_TIMING & DB_LEVEL
        double loopDuration = m_timingDetails->pauseTimer("imagesAtFrame");
        DBL(DB_TIMING,
            "frame: " << inframe << " imagesAtFrame time: " << loopDuration);
#endif

        DBL(DB_VIDEO, "Got a frame and buggin' out!!!\n");
    }

    void MovieFFMpegReader::readSubtitle(VideoTrack* track)
    {
        //    openAVCodec(track->videoPacket->stream_index);
        //    AVCodecContext* subtitleCodecContext =
        //        m_avFormatContext->streams[track->videoPacket->stream_index]->codec;
        //    ASSSplitContext* assSplitContext = ff_ass_split(
        //        (const char*) subtitleCodecContext->subtitle_header);
        //    ASS* ass = (ASS*)assSplitContext;
        //    AVSubtitle subtitle;
        //    int subtitleFinished = 0;
        //    if (0 < avcodec_decode_subtitle2(subtitleCodecContext,
        //            &subtitle, &subtitleFinished, track->videoPacket))
        //    {
        //        ostringstream text;
        //        for (int r = 0; r < subtitle.num_rects; r++)
        //        {
        //            AVSubtitleRect* rect = subtitle.rects[r];
        //            switch (rect->type)
        //            {
        //                case SUBTITLE_ASS:
        //                    {
        //                        ASSDialog* dialog = ff_ass_split_dialog(
        //                            assSplitContext, rect->ass, 0, NULL);
        //                        DBL (DB_SUBTITLES, "'" << dialog->text <<
        //                        "'"); text << dialog->text << " ";
        //                    }
        //                    break;
        //                case SUBTITLE_TEXT:
        //                    text << rect->text << " ";
        //                    break;
        //                default:
        //                    ostringstream message;
        //                    message << "Unhandled type of subtitle: '" <<
        //                    rect->type << "'"; report(message.str(), true);
        //                    break;
        //            }
        //        }
        //        DBL (DB_SUBTITLES, "freeing subtitle");
        //        avsubtitle_free(&subtitle);
        //        track->fb.newAttribute("SubtitleText", text.str());
        //        track->fb.newAttribute("SubtitleLanguage",
        //            streamLang(track->videoPacket->stream_index));
        //    }
    }

    void MovieFFMpegReader::identifiersAtFrame(const ReadRequest& request,
                                               IdentifierVector& ids)
    {
        int frame = request.frame;
        if (frame < m_info.start)
            frame = m_info.start;
        if (frame > m_info.end)
            frame = m_info.end;
        frame = frame - m_info.start + 1;

        //
        // Search for the requested view and tack on the identifier and track
        // num
        //

        bool searchNames = !request.views.empty();

        for (unsigned int i = 0; i < m_videoTracks.size(); i++)
        {
            VideoTrack* track = m_videoTracks[i];
            bool found = true;

            if (!searchNames && !request.stereo && i > 0)
                break;
            if (request.stereo && i > 1)
                break;

            if (request.views.size())
            {
                found = false;

                for (unsigned int q = 0; q < request.views.size(); q++)
                {
                    if (request.views[q] == track->name)
                    {
                        found = true;
                        break;
                    }
                }
            }

            if (!found)
                continue;

            ostringstream str;
            identifier(frame, str);
            str << "/" << i;
            ids.push_back(str.str());
        }
    }

    void MovieFFMpegReader::identifier(int frame, ostream& o)
    {
        frame = frame + m_info.start - 1;
        if (frame < m_info.start)
            frame = m_info.start;
        if (frame > m_info.end)
            frame = m_info.end;
        o << frame << ":" << m_filename;
    }

    template <typename T>
    int MovieFFMpegReader::translateAVAudio(AudioTrack* track, double maximum,
                                            int offset)
    {
        //
        // This templated method handles planar and non-planar audio collecting
        // for RV. There are many switches in this code to handle these two
        // possible layouts:
        //
        // Case #1 Planar:
        // framePointers[0] = ch0Sample0, ch0Sample1, ... ch0SampleM
        // framePointers[1] = ch1Sample0, ch1Sample1, ... ch1SampleM
        // ...
        // framePointers[N] = chNSample0, chNSample1, ... chNSampleM
        //
        // Case #2 Non-Planar:
        // framePointers[0] = ch0Sample0, ch1Sample0, ... chNSample0,
        //                    ch0Sample1, ch1Sample1, ... chNSample1,
        //                    ch0SampleM, ch1SampleM, ... chNSampleM
        //

        AVCodecContext* audioCodecContext = track->avCodecContext;
        AVSampleFormat audioFormat = audioCodecContext->sample_fmt;
        bool planar = av_sample_fmt_is_planar(audioFormat);

        SampleTime goalStart = track->start;
        SampleTime goalEnd = track->start + track->desired - 1;
        int channels = track->numChannels;
        int skipSamples = goalStart - track->bufferStart;
        int desired = track->bufferLength - skipSamples
                      - max(int(track->bufferEnd - goalEnd), 0);
        const int numPlanes = (planar) ? channels : 1;
        int chansPerPlane = (planar) ? 1 : channels;
        vector<T*> framePointers(numPlanes);
        for (int ch = 0; ch < numPlanes; ch++)
        {
            framePointers[ch] = ((T*)(track->audioFrame->extended_data[ch]));
            framePointers[ch] += skipSamples * chansPerPlane;
        }

        DBL(DB_AUDIO,
            "track: " << track->number << " is_planar: " << planar
                      << " goalStart: " << goalStart << " goalEnd: " << goalEnd
                      << " bufStart: " << track->bufferStart
                      << " bufEnd: " << track->bufferEnd << " bufferLength: "
                      << track->bufferLength << " skipSamples: " << skipSamples
                      << " desired: " << desired
                      << " chmap: " << m_audioState->chmap.size());

        float typeFactor = (offset != 0) ? 2.0 / maximum : 1.0 / maximum;
        float* outBuffer = track->bufferPointer;
        if (m_audioState->chmap.empty())
        {
            // Since the chmap is empty we assume the in/out channel layouts
            // match. See audioConfigure() in this regard.
            for (int s = 0; s < desired; s++, goalStart++)
            {
                for (int och = 0; och < m_audioState->channels.size(); och++)
                {
                    int p = (planar) ? och : 0;
                    int chIdx = (och - p) + (s * chansPerPlane);
                    const float sample =
                        float(framePointers[p][chIdx] - (T)offset) * typeFactor;
                    (*outBuffer++) = sample;

                    DBL(DB_AUDIO_SAMPLES, "sample(" << p << "/" << och << "/"
                                                    << chIdx
                                                    << "): " << sample);
                }
            }
        }
        else
        {
            for (int s = 0; s < desired; s++, goalStart++)
            {
                for (int och = 0; och < m_audioState->channels.size(); och++)
                {
                    const ChannelMixState& cmixState =
                        m_audioState->chmap[m_audioState->channels[och]];

                    float lMix = 0.0f;
                    const std::vector<ChannelState>& leftChs = cmixState.lefts;
                    for (int n = 0; n < leftChs.size(); n++)
                    {
                        const ChannelState& chState = leftChs[n];
                        int p = (planar) ? chState.index : 0;
                        int chIdx = (chState.index - p) + (s * chansPerPlane);
                        const float sample =
                            float(framePointers[p][chIdx] - (T)offset)
                            * typeFactor;
                        lMix += chState.weight * sample;

                        DBL(DB_AUDIO_SAMPLES,
                            "sample(" << p << "/" << chState.index << "/"
                                      << chIdx << "): " << sample);
                    }

                    float rMix = 0.0f;
                    const std::vector<ChannelState>& rightChs =
                        cmixState.rights;

                    for (int n = 0; n < rightChs.size(); ++n)
                    {
                        const ChannelState& chState = rightChs[n];
                        int p = (planar) ? chState.index : 0;
                        int chIdx = (chState.index - p) + (s * chansPerPlane);
                        const float sample =
                            float(framePointers[p][chIdx] - (T)offset)
                            * typeFactor;
                        rMix += chState.weight * sample;

                        DBL(DB_AUDIO_SAMPLES,
                            "sample(" << p << "/" << chState.index << "/"
                                      << chIdx << "): " << sample);
                    }

                    (*outBuffer++) = lMix + rMix;
                }
            }
        }
        track->lastDecodedAudio = goalStart - 1;

        DBL(DB_AUDIO, "floatsCollected: " << desired << " lastDecodedAudio: "
                                          << track->lastDecodedAudio);

        return desired;
    }

    //----------------------------------------------------------------------
    //
    // MovieFFMpegWriter Class
    //
    //----------------------------------------------------------------------

    MovieFFMpegWriter::MovieFFMpegWriter(const MovieFFMpegIO* io)
        : m_io(io)
        , m_avOutputFormat(0)
        , m_avFormatContext(0)
        , m_writeAudio(false)
        , m_writeVideo(false)
        , m_canControlRequest(true)
        , m_audioFrameSize(0)
        , m_lastAudioTime(0)
        , m_audioSamples(0)
        , m_duration(0)
        , m_timeScale(0)
        , m_reelName("")
        , m_dbline(0)
        , m_dblline(0)
    {
    }

    MovieFFMpegWriter::~MovieFFMpegWriter() {}

    void MovieFFMpegWriter::addTrack(
        bool isVideo, string codec,
        bool removeAppliedCodecParametersFromTheList /*=true*/)
    {
        const AVCodec* avCodec = avcodec_find_encoder_by_name(codec.c_str());

        if (!avCodec)
        {
            TWK_THROW_EXC_STREAM("Unsupported or unable to find codec named: '"
                                 << codec << "'");
        }

        AVStream* avStream = avformat_new_stream(m_avFormatContext, avCodec);
        if (!avStream)
        {
            TWK_THROW_EXC_STREAM("Could not allocate video stream");
        }

        avStream->id = m_avFormatContext->nb_streams - 1;
        AVCodecContext* avCodecContext = avcodec_alloc_context3(avCodec);
        if (isVideo)
        {
            avStream->time_base.num = m_duration;
            avStream->time_base.den = m_timeScale;
            avCodecContext->time_base.num = m_duration;
            avCodecContext->time_base.den = m_timeScale;
            avCodecContext->codec_id = avCodec->id;
            avCodecContext->codec_type = AVMEDIA_TYPE_VIDEO;
            avCodecContext->thread_count = m_request.threads;
            avCodecContext->width = m_info.width;
            avCodecContext->height = m_info.height;
            avCodecContext->color_primaries = AVCOL_PRI_BT709; // 1
            //
            //  XXX should come back to this.  should transfer function also be
            //  something else for mjpeg ?  Maybe need user control.
            //
            avCodecContext->color_trc = AVCOL_TRC_BT709; // 1

            //
            //  Confirm pix_fmt setting now, before we set the "colorspace"
            //  since we need to know if pix_fmt is RGB.
            //

            if (m_parameters.find("pix_fmt") != m_parameters.end())
            {
                avCodecContext->pix_fmt =
                    av_get_pix_fmt(m_parameters["pix_fmt"].c_str());
                if (removeAppliedCodecParametersFromTheList)
                {
                    m_parameters.erase("pix_fmt");
                }
            }
            else
            {
                avCodecContext->pix_fmt = (avCodec->pix_fmts)
                                              ? avCodec->pix_fmts[0]
                                              : RV_OUTPUT_FFMPEG_FMT;

                if (m_request.verbose)
                {
                    ostringstream message;
                    message
                        << "No pix_fmt specified. Using: "
                        << string(av_get_pix_fmt_name(avCodecContext->pix_fmt));
                    report(message.str());
                }
            }

            //
            //  Pretty much everything should be written with rec709 conversion
            //  matrix except jpeg, or RGB pixel formats
            //

            if (codec == "jpegls" || codec == "ljpeg" || codec == "mjpeg"
                || codec == "mjpegb" || codec == "v210")
            {
                avCodecContext->colorspace = AVCOL_SPC_SMPTE170M; // 6
            }
            else
            {
                const AVPixFmtDescriptor* desc =
                    av_pix_fmt_desc_get(avCodecContext->pix_fmt);
                bool isRGB = (desc && (desc->flags & AV_PIX_FMT_FLAG_RGB));

                if (isRGB)
                    avCodecContext->colorspace = AVCOL_SPC_RGB; // 0
                else
                    avCodecContext->colorspace = AVCOL_SPC_BT709; // 1
            }

            avCodecContext->sample_aspect_ratio =
                av_d2q(m_request.pixelAspect, INT_MAX);

            AVPixelFormat requestFormat =
                (m_canControlRequest) ? getBestAVFormat(avCodecContext->pix_fmt)
                                      : RV_OUTPUT_FFMPEG_FMT;
            const AVPixFmtDescriptor* desc = av_pix_fmt_desc_get(requestFormat);
            int bitSize = desc->comp[0].depth - desc->comp[0].shift;
            bool hasAlpha = (desc->flags & AV_PIX_FMT_FLAG_ALPHA);

            // XXX might need to set avCodecContext->bits_per_raw_sample to
            // match something appropriate for the pix_fmt

            m_info.pixelAspect = m_request.pixelAspect;
            m_info.orientation = FrameBuffer::TOPLEFT;
            m_info.numChannels = (hasAlpha) ? 4 : 3;
            m_info.dataType =
                (bitSize > 8) ? FrameBuffer::USHORT : FrameBuffer::UCHAR;

            // Backwards compatibility for quality setting
            if (codec == "mjpeg")
            {
                avCodecContext->flags |= AV_CODEC_FLAG_QSCALE;
                avCodecContext->global_quality =
                    pow(100000, 1.0 - m_request.quality);
            }

            VideoTrack* track = new VideoTrack;
            track->number = avStream->id;
            track->avCodecContext = avCodecContext;
            m_videoTracks.push_back(track);

            // Set the reel name if provided
            if (m_reelName != "")
            {
                av_dict_set(&avStream->metadata, "reel_name",
                            m_reelName.c_str(), 0);
            }
        }
        else
        {
            avStream->time_base.num = 1;
            avStream->time_base.den = m_info.audioSampleRate;
            avCodecContext->time_base.num = 1;
            avCodecContext->time_base.den = m_info.audioSampleRate;
            avCodecContext->codec_id = avCodec->id;
            avCodecContext->codec_type = AVMEDIA_TYPE_AUDIO;
            avCodecContext->sample_rate = m_info.audioSampleRate;
            avCodecContext->ch_layout.nb_channels = m_info.audioChannels.size();
            av_channel_layout_default(&(avCodecContext->ch_layout),
                                      avCodecContext->ch_layout.nb_channels);

            if (m_parameters.find("sample_fmt") != m_parameters.end())
            {
                avCodecContext->sample_fmt =
                    av_get_sample_fmt(m_parameters["sample_fmt"].c_str());
                if (removeAppliedCodecParametersFromTheList)
                {
                    m_parameters.erase("sample_fmt");
                }
            }
            else
            {
                avCodecContext->sample_fmt = avCodec->sample_fmts[0];
            }

            AudioTrack* track = new AudioTrack;
            track->number = avStream->id;
            track->avCodecContext = avCodecContext;
            m_audioTracks.push_back(track);

            // Step back a frame for AAC pre-roll
            if (codec == "aac")
                track->audioFrame->pts = -2048;
        }

        // Some formats want stream headers to be separate.
        if (m_avFormatContext->oformat->flags & AVFMT_GLOBALHEADER)
        {
            avCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }

        // Enable experimental compliance if the codec supports it
        if (avCodec->capabilities & AV_CODEC_CAP_EXPERIMENTAL)
        {
            avCodecContext->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
        }

        // Set any relevant codec options
        applyCodecParameters(avCodecContext,
                             removeAppliedCodecParametersFromTheList);

        int ret = avcodec_open2(avCodecContext, avCodec, NULL);
        if (ret < 0)
        {
            TWK_THROW_EXC_STREAM("Could not open codec: " << avErr2Str(ret));
        }

        // Copy codec parameters from AVCodecContext to AVStream.
        if (avcodec_parameters_from_context(avStream->codecpar, avCodecContext)
            < 0)
        {
            TWK_THROW_EXC_STREAM(
                "Failed to copy codec parameters from context to stream");
        }

        // Post codec open initializations
        if (isVideo)
            initVideoTrack(avStream);
        else
        {
            // If there is no preferred sample size in this codec then use 2048
            m_audioFrameSize = (avCodecContext->frame_size)
                                   ? avCodecContext->frame_size
                                   : 2048;
            if (m_audioSamples)
                av_freep(&m_audioSamples);
            m_audioSamples = av_malloc(
                avCodecContext->ch_layout.nb_channels * m_audioFrameSize
                * av_get_bytes_per_sample(avCodecContext->sample_fmt));
        }
    }

    void MovieFFMpegWriter::applyCodecParameters(
        AVCodecContext* avCodecContext,
        bool removeAppliedCodecParametersFromTheList /*=true*/)
    {
        //
        // Find and apply parameters for AVCodecContext (*cc:) and AVCodec
        // (*c:). Keep track of what we applied to remove from further
        // processing.
        //

        vector<string> applied;
        for (map<string, string>::iterator left = m_parameters.begin();
             left != m_parameters.end(); left++)
        {
            string name = left->first;
            string value = left->second;

            void* avObj = NULL;
            const AVOption* opt = NULL;
            if (name.substr(1, 3) == "cc:")
            {
                avObj = (void*)avCodecContext;
                string sub = name.substr(4);
                opt = av_opt_find(avObj, sub.c_str(), NULL, 0, 0);
            }
            else if (name.substr(1, 2) == "c:")
            {
                avObj = avCodecContext->priv_data;
                string sub = name.substr(3);
                opt = av_opt_find(avObj, sub.c_str(), NULL, 0, 0);
            }
            if (opt != NULL && avObj != NULL)
            {
                DBL(DB_WRITE, "Found option: " << opt->name << " for name: "
                                               << name << " value: " << value);

                if (setOption(opt, avObj, value))
                    applied.push_back(left->first);
            }
        }

        //
        // Remove what we applied because we will report what was unmatched
        // later
        //
        if (removeAppliedCodecParametersFromTheList)
        {
            for (vector<string>::iterator erase = applied.begin();
                 erase != applied.end(); erase++)
            {
                m_parameters.erase(*erase);
            }
        }
    }

    void MovieFFMpegWriter::applyFormatParameters()
    {
        //
        // Keep track of if we set the comment and copyright, because we have
        // default values for these fields if they are not set explicitly by the
        // user.
        //

        bool comment = false;
        bool copyright = false;
        for (int i = 0; i < m_request.parameters.size(); i++)
        {
            const string& name = m_request.parameters[i].first;
            const string& value = m_request.parameters[i].second;

            DBL(DB_WRITE, "parameter name: " << name << " value: " << value);

            //
            // Handle tweak specific parameter options
            //

            if (name.substr(0, 7) == "output/"
                || name.substr(0, 8) == "duration"
                || name.substr(0, 9) == "timescale"
                || name.substr(0, 17) == "libx264autoresize")
                continue;

            //
            // If the timecode string has no ":" in it then it treat it as a
            // frame number.
            //

            if (name == "timecode")
            {
                string tcval = value.c_str();
                if (value.find(":") == string::npos)
                {
                    int frame = atoi(value.c_str());
                    AVTimecode avTimecode;
                    AVRational rate = av_d2q(m_info.fps, INT_MAX);
                    av_timecode_init(&avTimecode, rate, 0, 0,
                                     m_avFormatContext);
                    char tcString[80];
                    av_timecode_make_string(&avTimecode, tcString, frame);
                    tcval = string(tcString);
                }
                int ret = av_dict_set(&m_avFormatContext->metadata, "timecode",
                                      tcval.c_str(), 0);
                if (ret < 0)
                {
                    TWK_THROW_EXC_STREAM("Unable to set timecode from: '"
                                         << value << "'");
                }
                continue;
            }

            //
            // Look for a reel name to add to the timecode track tmcd atom
            // metadata
            //

            if (name == "reelname")
            {
                m_reelName = value.c_str();
                continue;
            }

            //
            // Try treating everything else as metadata
            //

            if (isMetadataField(name))
            {
                int ret = av_dict_set(&m_avFormatContext->metadata,
                                      name.c_str(), value.c_str(), 0);
                if (ret < 0)
                {
                    TWK_THROW_EXC_STREAM("Unable to set " << name << " from: '"
                                                          << value << "'");
                }
                if (name == "comment")
                    comment = true;
                if (name == "copyright")
                    copyright = true;
                continue;
            }

            //
            // Handle setting the options of the format
            //

            void* avObj = NULL;
            const AVOption* opt = NULL;
            if (name.substr(0, 2) == "f:")
            {
                avObj = (void*)m_avFormatContext;
                string sub = name.substr(2);
                opt = av_opt_find(avObj, sub.c_str(), NULL, 0, 0);
            }
            else if (name.substr(0, 3) == "of:")
            {
                avObj = m_avFormatContext->priv_data;
                string sub = name.substr(3);
                opt = av_opt_find(avObj, sub.c_str(), NULL, 0, 0);
            }
            if (opt == NULL || avObj == NULL || !setOption(opt, avObj, value))
            {
                m_parameters[name] = value;
            }
        }

        //
        // Make sure we use the request comment and copyright if not already set
        //

        if (!comment)
        {
            int ret = av_dict_set(&m_avFormatContext->metadata, "comment",
                                  m_request.comments.c_str(), 0);
            if (ret < 0)
            {
                TWK_THROW_EXC_STREAM("Unable to set comment from: '"
                                     << m_request.comments << "'");
            }
        }
        if (!copyright)
        {
            int ret = av_dict_set(&m_avFormatContext->metadata, "copyright",
                                  m_request.copyright.c_str(), 0);
            if (ret < 0)
            {
                TWK_THROW_EXC_STREAM("Unable to set copyright from: '"
                                     << m_request.copyright << "'");
            }
        }
    }

    void MovieFFMpegWriter::collectWriteInfo(string videoCodec,
                                             string audioCodec)
    {
        //
        //  Create a list of all the frames to output from the input
        //  frames and ranges.
        //

        if (m_info.video)
        {
            m_info.fps = m_request.fps ? m_request.fps : m_info.fps;
            if (m_request.timeRangeOverride)
            {
                bool outOfRange = false;

                for (int i = 0; i < m_request.frames.size(); i++)
                {
                    int f = m_request.frames[i];
                    if (f < m_info.start || f > m_info.end)
                    {
                        outOfRange = true;
                        continue;
                    }
                    m_frames.push_back(f);
                }

                if (outOfRange)
                {
                    report("Ignoring frames that are out of input range", true);
                }
            }
            else
            {
                for (int f = m_info.start; f <= m_info.end; f += m_info.inc)
                {
                    m_frames.push_back(f);
                }
            }

            //
            //  Try and use final cut preferred values
            //  http://developer.apple.com/mac/library/qa/qa2005/qa1447.html
            //

            int duration = 0;
            int timeScale = 0;
            int movflagsIdx = -1;
            bool x264Resize = false;
            for (int i = 0; i < m_request.parameters.size(); i++)
            {
                const string& name = m_request.parameters[i].first;
                const string& value = m_request.parameters[i].second;

                if (name == "timescale")
                    timeScale = atoi(value.c_str());
                else if (name == "duration")
                    duration = atoi(value.c_str());
                else if (name == "libx264autoresize")
                    x264Resize = true;
                else if (name == "of:movflags")
                    movflagsIdx = i;
            }

            // XXX For now force H.264 writing to include COLR atom
            if (videoCodec == "libx264")
            {
                if (movflagsIdx == -1)
                {
                    m_request.parameters.push_back(
                        StringPair("of:movflags", "write_colr"));
                }
                else
                    m_request.parameters[movflagsIdx].second += "|write_colr";
            }

            double fps = m_info.fps;
            if (timeScale && duration)
            {
            }
            else if (fpsEquals(fps, 60))
            {
                timeScale = 6000;
                duration = 100;
            }
            else if (fpsEquals(fps, 59.94))
            {
                timeScale = 5994;
                duration = 100;
            }
            else if (fpsEquals(fps, 50))
            {
                timeScale = 5000;
                duration = 100;
            }
            else if (fpsEquals(fps, 30))
            {
                timeScale = 3000;
                duration = 100;
            }
            else if (fpsEquals(fps, 29.97))
            {
                timeScale = 30000;
                duration = 1001;
            }
            else if (fpsEquals(fps, 25))
            {
                timeScale = 2500;
                duration = 100;
            }
            else if (fpsEquals(fps, 24))
            {
                timeScale = 2400;
                duration = 100;
            }
            else if (fpsEquals(fps, 23.98) || fpsEquals(fps, 23.97599))
            {
                timeScale = 23976;
                duration = 1000;
            }
            else
            {
                // Must add 0.001 so that 29.969... gets properly cropped
                // to 29.97
                timeScale = (fps + 0.001) * 1000;
                duration = timeScale / fps;
            }
            m_timeScale = timeScale;
            m_duration = duration;

            if (x264Resize)
            {
                int wOff2 = m_info.width % 2;
                int hOff2 = m_info.height % 2;
                m_info.width += (m_info.width % 4 > 1) ? wOff2 : -wOff2;
                m_info.height += (m_info.height % 4 > 1) ? hOff2 : -hOff2;
                int wOff4 = m_info.width % 4;
                int hOff4 = m_info.height % 4;
                if (wOff4 != 0 && hOff4 != 0)
                {
                    if (wOff4 >= hOff4)
                        m_info.width += wOff4;
                    else
                        m_info.height += hOff4;
                }
            }
        }
        if (m_info.audio)
        {
            unsigned int audioChannels = m_request.audioChannels
                                             ? m_request.audioChannels
                                             : m_info.audioChannels.size();
            Time audioSampleRate = m_request.audioRate ? m_request.audioRate
                                                       : m_info.audioSampleRate;

            m_info.audioChannels =
                layoutChannels(channelLayouts(audioChannels).front());
            m_info.audioSampleRate = audioSampleRate;

            if (audioCodec == "aac")
            {
                m_lastAudioTime =
                    -samplesToTime(size_t(1024), m_info.audioSampleRate);
            }
        }

        DBL(DB_WRITE,
            "video: " << m_info.video << " start: " << m_info.start
                      << " end: " << m_info.end << " inc: " << m_info.inc
                      << " fps: " << m_info.fps << " audio: " << m_info.audio
                      << " audioChannels: " << m_info.audioChannels.size()
                      << " audioSampleRate: " << m_info.audioSampleRate
                      << " timeScale: " << m_timeScale << " duration: "
                      << m_duration << " frames: " << m_frames.size());
    }

    void MovieFFMpegWriter::encodeAudio(AVCodecContext* audioCodecContext,
                                        AVFrame* frame, AVPacket* pkt,
                                        AVStream* audioStream,
                                        SampleTime* nsamps,
                                        int64_t lastEncAudio)
    {
        int ret = avcodec_send_frame(audioCodecContext, frame);
        if (ret < 0)
        {
            TWK_THROW_EXC_STREAM(
                "Error encoding audio frame: " << avErr2Str(ret));
        }

        while (ret >= 0)
        {
            ret = avcodec_receive_packet(audioCodecContext, pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            {
                break;
            }

            if (ret < 0)
            {
                TWK_THROW_EXC_STREAM(
                    "Error encoding audio frame: " << avErr2Str(ret));
            }
            if (!ret && pkt->size)
            {
                pkt->stream_index = audioStream->index;
                validateTimestamps(pkt, audioStream, audioCodecContext, 0,
                                   true);
                ret = av_interleaved_write_frame(m_avFormatContext, pkt);
                if (ret < 0)
                {
                    TWK_THROW_EXC_STREAM(
                        "Error while writing audio frame: " << avErr2Str(ret));
                }

                // Update the last time sample in seconds we encoded
                if (nsamps)
                {
                    m_lastAudioTime +=
                        samplesToTime(*nsamps, m_info.audioSampleRate);
                }
            }
            frame->pts = lastEncAudio;
        }
    }

    bool MovieFFMpegWriter::fillAudio(Movie* inMovie, double overflow,
                                      bool lastPass)
    {
        //
        //  Compute the start and end samples for this audio
        //  chunk. Make sure to handle the case near the end of
        //  the file where we have to output any left-over audio.
        //

        bool audioFinished = false;
        SampleTime nsamps = m_audioFrameSize;
        double audioOffset = double(m_frames[0] - m_info.start) / m_info.fps;
        if (lastPass && overflow < 0 && m_lastAudioTime > audioOffset)
        {
            nsamps += timeToSamples(overflow, m_info.audioSampleRate) - 1;
            audioFinished = true;
        }
        if (nsamps <= 0)
            return audioFinished;

        Time audioLength = samplesToTime(nsamps, m_info.audioSampleRate);
        Movie::AudioReadRequest audioRequest(m_lastAudioTime + audioOffset,
                                             audioLength);
        TwkAudio::AudioBuffer audioBuffer(nsamps, m_info.audioChannels,
                                          m_info.audioSampleRate,
                                          m_lastAudioTime + audioOffset);
        inMovie->audioFillBuffer(audioRequest, audioBuffer);

        if (m_request.verbose)
        {
            ostringstream message;
            message << "Writing " << audioBuffer.size() << " audio samples";
            report(message.str());
        }

        AudioTrack* track = m_audioTracks[0];
        AVStream* audioStream = m_avFormatContext->streams[track->number];
        AVCodecContext* audioCodecContext = track->avCodecContext;
        AVSampleFormat audioFormat = audioCodecContext->sample_fmt;
        int audioChannels = audioCodecContext->ch_layout.nb_channels;
        int sampleSize = av_get_bytes_per_sample(audioFormat);
        int totalOutputSize = audioBuffer.size() * audioChannels * sampleSize;
        bool planar = av_sample_fmt_is_planar(audioFormat);

        DBL(DB_WRITE,
            "nsamps: " << nsamps << " planar: " << planar
                       << " bufSize: " << audioBuffer.size()
                       << " audioChannels: " << audioChannels
                       << " bufChannels: " << audioBuffer.numChannels()
                       << " audioSampleRate: " << m_info.audioSampleRate
                       << " bufRate: " << audioBuffer.rate()
                       << " audioLength: " << audioLength
                       << " bufLength: " << audioBuffer.duration()
                       << " bufTotalSize: " << audioBuffer.sizeInBytes()
                       << " totalOutputSize: " << totalOutputSize
                       << " m_lastAudioTime: " << m_lastAudioTime
                       << " bufStart: " << audioBuffer.startTime()
                       << " overflow: " << overflow);

        switch (audioFormat)
        {
        case AV_SAMPLE_FMT_U8:
        case AV_SAMPLE_FMT_U8P:
            translateRVAudio<unsigned char>(audioChannels, &audioBuffer,
                                            CHAR_MAX, 127, planar);
            break;
        case AV_SAMPLE_FMT_S16:
        case AV_SAMPLE_FMT_S16P:
            translateRVAudio<short>(audioChannels, &audioBuffer, SHRT_MAX, 0,
                                    planar);
            break;
        case AV_SAMPLE_FMT_S32:
        case AV_SAMPLE_FMT_S32P:
            translateRVAudio<int>(audioChannels, &audioBuffer, INT_MAX, 0,
                                  planar);
            break;
        case AV_SAMPLE_FMT_FLT:
        case AV_SAMPLE_FMT_FLTP:
            translateRVAudio<float>(audioChannels, &audioBuffer, 1.0, 0,
                                    planar);
            break;
        case AV_SAMPLE_FMT_DBL:
        case AV_SAMPLE_FMT_DBLP:
            translateRVAudio<double>(audioChannels, &audioBuffer, 1.0, 0,
                                     planar);
            break;
        case AV_SAMPLE_FMT_NONE:
        default:
            TWK_THROW_EXC_STREAM("Unsupported audio format.");
        }

        //
        // It is critical to set the number of samples in the audio AVFrame
        // before calling fill frame and encode.
        //

        track->audioFrame->nb_samples = nsamps;
        track->audioFrame->format = audioFormat;
        track->audioFrame->ch_layout.nb_channels =
            audioCodecContext->ch_layout.nb_channels;
        track->audioFrame->ch_layout = audioCodecContext->ch_layout;
        avcodec_fill_audio_frame(track->audioFrame, audioChannels, audioFormat,
                                 (uint8_t*)m_audioSamples, totalOutputSize, 0);

        //
        // Make sure the frame has a valid pts to seed the packet.
        //

        if (track->audioFrame->pts == AV_NOPTS_VALUE)
        {
            track->audioFrame->pts = track->lastEncodedAudio;
        }
        track->lastEncodedAudio =
            track->audioFrame->pts + track->audioFrame->nb_samples;

        encodeAudio(audioCodecContext, track->audioFrame, track->audioPacket,
                    audioStream, &nsamps, track->lastDecodedAudio);
        if (lastPass)
        {
            // It is important to call encodeAudio (above) to make sure that all
            // frames as been sent to the encoder before entering draining mode
            // by passing NULL. Send a NULL frame to enter draining mode.
            encodeAudio(audioCodecContext, NULL, track->audioPacket,
                        audioStream, NULL, track->lastDecodedAudio);
            // Returned value as no value at this point.
        }

        return audioFinished;
    }

    void MovieFFMpegWriter::encodeVideo(AVCodecContext* ctx, AVFrame* frame,
                                        AVPacket* pkt, AVStream* stream,
                                        int lastEncVideo)
    {
        int ret = avcodec_send_frame(ctx, frame);
        if (ret < 0)
        {
            TWK_THROW_EXC_STREAM(
                "Error encoding video frame: " << avErr2Str(ret));
        }

        while (ret >= 0)
        {
            ret = avcodec_receive_packet(ctx, pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            {
                return;
            }

            pkt->stream_index = stream->index;
            validateTimestamps(pkt, stream, ctx, lastEncVideo);
            ret = av_interleaved_write_frame(m_avFormatContext, pkt);
            if (ret != 0)
            {
                TWK_THROW_EXC_STREAM(
                    "Error while writing video frame: " << avErr2Str(ret));
            }
        }
    }

    void MovieFFMpegWriter::fillVideo(FrameBufferVector fbs, int trackIndex,
                                      int frameIndex, bool lastPass)
    {
        VideoTrack* track = m_videoTracks[trackIndex];
        AVStream* avStream = m_avFormatContext->streams[track->number];
        AVCodecContext* videoCodecContext = track->avCodecContext;

        int fbIndex =
            (trackIndex > fbs.size() - 1) ? fbs.size() - 1 : trackIndex;
        FrameBuffer* fb = fbs[fbIndex];

        //
        // Re-orient the framebuffer to TOPLEFT
        //

        if (fb->orientation() == FrameBuffer::NATURAL
            || fb->orientation() == FrameBuffer::BOTTOMRIGHT)
        {
            flip(fb);
        }
        if (fb->orientation() == FrameBuffer::TOPRIGHT
            || fb->orientation() == FrameBuffer::BOTTOMRIGHT)
        {
            flop(fb);
        }

        //
        // Convert the incoming framebuffer data to the required codec pixel
        // format
        //

        // track->inPicture.data[0] = fb->pixels<unsigned char>();

        uint8_t* pixels[AV_NUM_DATA_POINTERS];
        memset(pixels, 0, sizeof(pixels));
        int linesizes[AV_NUM_DATA_POINTERS];
        memset(linesizes, 0, sizeof(linesizes));

        pixels[0] = fb->pixels<uint8_t>(); // AKA unsigned char
        linesizes[0] = fb->scanlinePaddedSize();

        sws_scale(track->imgConvertContext, pixels, linesizes, 0,
                  videoCodecContext->height, track->outPicture->data,
                  track->outPicture->linesize);

        // Send/Receive encoding and decoding API overview
        // https://ffmpeg.org/doxygen/6.0/group__lavc__encdec.html

        // Set the PTS before sending the frame to the encoder.
        track->lastEncodedVideo = track->videoFrame->pts;
        track->videoFrame->pts = frameIndex;

        encodeVideo(videoCodecContext, track->videoFrame, track->videoPacket,
                    avStream, track->lastEncodedVideo);
        if (lastPass)
        {
            // It is important to call encodeVideo (above) to make sure that all
            // frames as been sent to the encoder before entering draining mode
            // by passing NULL. Send a NULL frame to enter draining mode.
            encodeVideo(videoCodecContext, NULL, track->videoPacket, avStream,
                        track->lastEncodedVideo);
        }

        DBL(DB_WRITE,
            "requested chans: " << m_info.numChannels
                                << " dataType: " << m_info.dataType
                                << " returned chans: " << fb->numChannels()
                                << " dataType: " << fb->dataType());
    }

    void MovieFFMpegWriter::initRefMovie(ReformattingMovie* refMovie)
    {
        if (m_info.video)
        {
            vector<string> chmap;
            chmap.push_back("R");
            chmap.push_back("G");
            chmap.push_back("B");
            chmap.push_back("A");
            refMovie->setChannelMap(chmap);
            refMovie->setOutputOrientation(m_info.orientation);
            refMovie->setOutputResolution(m_info.width, m_info.height);
            refMovie->setOutputFormat(m_info.dataType);
            refMovie->setFPS(m_info.fps);
        }
        if (m_info.audio)
        {
            refMovie->setAudio(m_info.audioSampleRate, m_audioFrameSize,
                               m_info.audioChannels);
        }
    }

    void MovieFFMpegWriter::initVideoTrack(AVStream* avStream)
    {
        DBL(DB_WRITE, "initVideoTrack: " << avStream->id);

        VideoTrack* track = m_videoTracks[avStream->id];
        AVCodecContext* videoCodecContext = track->avCodecContext;
        AVPixelFormat srcFmt = (m_canControlRequest)
                                   ? getBestAVFormat(videoCodecContext->pix_fmt)
                                   : RV_OUTPUT_FFMPEG_FMT;
        AVPixelFormat dstFmt = videoCodecContext->pix_fmt;

        track->imgConvertContext = sws_getCachedContext(
            NULL, videoCodecContext->width, videoCodecContext->height, srcFmt,
            videoCodecContext->width, videoCodecContext->height, dstFmt,
            SWS_BICUBIC, NULL, NULL, NULL);

        if (track->imgConvertContext == 0)
        {
            TWK_THROW_EXC_STREAM("Cannot initialize the conversion context!");
        }

        //
        //  NOTE: AVFrame buffers are allocated by this function
        //

        track->outPicture->format = videoCodecContext->pix_fmt;
        track->outPicture->width = videoCodecContext->width;
        track->outPicture->height = videoCodecContext->height;

        int ret = av_frame_get_buffer(track->outPicture, 0);

        if (ret < 0)
        {
            TWK_THROW_EXC_STREAM(
                "Could not allocate picture: " << avErr2Str(ret));
        }

        //
        //  NOTE: AVFrame buffers are allocated by this function
        //

        track->inPicture->format = srcFmt;
        track->inPicture->width = videoCodecContext->width;
        track->inPicture->height = videoCodecContext->height;

        ret = av_frame_get_buffer(track->inPicture, 0);

        if (ret < 0)
        {
            TWK_THROW_EXC_STREAM(
                "Could not allocate temporary picture: " << avErr2Str(ret));
        }

        // Copy data and linesize picture pointers to frame
        //*((AVPicture *)track->videoFrame) = track->outPicture;
        //
        //  Be paranoid and assert that these structs are in fact
        //  similar. The way the ffmpeg structures are defined, there's no
        //  relationship between AVFrame and AVPictire as far as the
        //  compiler is concerned. I.e. you'd expect AVFrame to have an
        //  AVPicture struct as its first field but instead it has
        //  compatible internal fields similar to AVPicture; its
        //  practically an accident.
        //
        //  If when upgrading ffmpeg this is no longer true we have a
        //  problem.
        //

        assert(sizeof(track->videoFrame->data)
               == sizeof(track->outPicture->data));
        assert(sizeof(track->videoFrame->linesize)
               == sizeof(track->outPicture->linesize));

        for (size_t i = 0; i < AV_NUM_DATA_POINTERS; i++)
        {
            track->videoFrame->data[i] = track->outPicture->data[i];
            track->videoFrame->linesize[i] = track->outPicture->linesize[i];
        }

        // Init frame settings from video codec context
        track->videoFrame->color_range = videoCodecContext->color_range;
        track->videoFrame->colorspace = videoCodecContext->colorspace;
        track->videoFrame->quality = videoCodecContext->global_quality;
        track->videoFrame->format = videoCodecContext->pix_fmt;
        track->videoFrame->height = videoCodecContext->height;
        track->videoFrame->width = videoCodecContext->width;
        track->videoFrame->pts = 0;

        int* inv_table = 0;
        int* table = 0;
        int srcRange = -1;
        int dstRange = -1;
        int brightness = -1;
        int contrast = -1;
        int saturation = -1;

        int spret = sws_getColorspaceDetails(
            track->imgConvertContext, &inv_table, &srcRange, &table, &dstRange,
            &brightness, &contrast, &saturation);

        DBL(DB_WRITE, "(BEFORE) Color Details "
                          << endl
                          << "\ttable " << table[0] << " " << table[1] << " "
                          << table[2] << " " << table[3] << endl
                          << "\tinv   " << inv_table[0] << " " << inv_table[1]
                          << " " << inv_table[2] << " " << inv_table[3] << endl
                          << "\tsrcRange " << srcRange << " dstRange "
                          << dstRange);
        //
        //  Be sure any rgb->yuv coefficients are the right onees for the output
        //  colorspace.
        //

        spret = sws_setColorspaceDetails(
            track->imgConvertContext,
            sws_getCoefficients(videoCodecContext->colorspace), srcRange,
            sws_getCoefficients(videoCodecContext->colorspace),
            videoCodecContext->color_range - 1, // SwsContext uses 1 less
            brightness, contrast, saturation);

#if DB_WRITE & DB_LEVEL
        spret = sws_getColorspaceDetails(
            track->imgConvertContext, (int**)(&inv_table), &srcRange,
            (int**)(&table), &dstRange, &brightness, &contrast, &saturation);
#endif

        DBL(DB_WRITE, "(AFTER) Color Details "
                          << endl
                          << "\ttable " << table[0] << " " << table[1] << " "
                          << table[2] << " " << table[3] << endl
                          << "\tinv   " << inv_table[0] << " " << inv_table[1]
                          << " " << inv_table[2] << " " << inv_table[3] << endl
                          << "\tsrcRange " << srcRange << " dstRange "
                          << dstRange);
    }

    bool MovieFFMpegWriter::setOption(const AVOption* opt, void* avObj,
                                      const string value)
    {
        int setopt = -1;
        istringstream invalue(value);
        string flagVal;
        switch (opt->type)
        {
        case AV_OPT_TYPE_FLAGS:
            const AVOption* flagOpt;
            int64_t current, hexVal;
            size_t location, last;
            location = last = 0;

            if (value.substr(0, 2) == "0x") // Check for hex
            {
                sscanf(value.c_str(), "0x%llx", &hexVal);
                setopt = av_opt_set_int(avObj, opt->name, hexVal, 0);
                break;
            }
            do
            {
                location = value.find("|", last);
                flagVal = value.substr(last, location - last);
                flagOpt = av_opt_find(avObj, flagVal.c_str(), opt->unit, 0, 0);
                if (flagOpt == NULL)
                    break;
                if (av_opt_get_int(avObj, opt->name, 0, &current) != 0)
                    break;
                setopt = av_opt_set_int(avObj, opt->name,
                                        current | flagOpt->default_val.i64, 0);
                last = location + 1;
            } while (location != string::npos);
            break;
        case AV_OPT_TYPE_CONST:
        case AV_OPT_TYPE_INT:
        case AV_OPT_TYPE_INT64:
            int integer;
            invalue >> integer;
            setopt = av_opt_set_int(avObj, opt->name, integer, 0);
            break;
        case AV_OPT_TYPE_DOUBLE:
        case AV_OPT_TYPE_FLOAT:
            double floatDouble;
            invalue >> floatDouble;
            setopt = av_opt_set_double(avObj, opt->name, floatDouble, 0);
            break;
        case AV_OPT_TYPE_RATIONAL:
            double quotient;
            invalue >> quotient;
            AVRational rational;
            rational = av_d2q(quotient, INT_MAX);
            setopt = av_opt_set_q(avObj, opt->name, rational, 0);
            break;
        case AV_OPT_TYPE_STRING:
        case AV_OPT_TYPE_IMAGE_SIZE:
        case AV_OPT_TYPE_PIXEL_FMT:
        case AV_OPT_TYPE_SAMPLE_FMT:
        case AV_OPT_TYPE_BINARY:
        default:
            setopt = av_opt_set(avObj, opt->name, value.c_str(), 0);
            break;
        }
        if (setopt != 0)
        {
            ostringstream message;
            message << "Unable to set '" << opt->name << "' to '" << value
                    << "'.";
            report(message.str(), true);
            return false;
        }
        DBL(DB_WRITE, "Setting '" << opt->name << "' to '" << value << "'.");
        return true;
    }

    template <typename T>
    void MovieFFMpegWriter::translateRVAudio(int outChannels,
                                             TwkAudio::AudioBuffer* inBuffer,
                                             double maximum, int offset,
                                             bool planar)
    {
        //
        // This templated method takes interleaved float audio samples from a
        // TwkAudio::AudioBuffer and converts them to the type desired by the
        // audio econder. If the requested format is planar then the samples are
        // also de-interleaved. For example LRLRLRLR becomes LLLLRRRR for
        // stereo.
        //
        // If the number of input channels does not match the number of
        // requested output channels then the input channels are mixed down and
        // evenly shared with all output channels.
        //
        // NOTE: Currently restrictions outside this method enforce 1 or 2
        // output channels and all inputs have 2 channels.
        //

        float* bufferPointer = inBuffer->pointer();
        int totalNumSamples = inBuffer->size();
        int totalDesiredSamples = totalNumSamples * outChannels;
        const int numPlanes = (planar) ? outChannels : 1;
        vector<T*> framePointers(numPlanes);
        for (int ch = 0; ch < numPlanes; ch++)
        {
            framePointers[ch] =
                (planar) ? (T*)m_audioSamples + (totalNumSamples * ch)
                         : (T*)m_audioSamples + ch;
        }

        float typeFactor = (offset != 0) ? 0.5 * maximum : maximum;
        int written = 0;
        int inChannels = inBuffer->numChannels();

        if (outChannels != inChannels)
        {
            while (written < totalDesiredSamples)
            {
                //
                // If the input channel count does not match the output then mix
                // the channels and copy the merged value to all output
                // channels.
                //

                float merge = 0;
                for (int b = 0; b < inChannels; b++, bufferPointer++)
                {
                    merge += (float)(((*bufferPointer * typeFactor) + offset)
                                     / inChannels);
                }
                for (int c = 0; c < outChannels; c++)
                {
                    int ch = (planar) ? c : 0;
                    *framePointers[ch]++ = (T)merge;
                    written++;
                }
            }
        }
        else
        {
            while (written < totalDesiredSamples)
            {
                for (int c = 0; c < outChannels; c++, bufferPointer++)
                {
                    int ch = (planar) ? c : 0;
                    *framePointers[ch]++ =
                        (T)((*bufferPointer * typeFactor) + offset);
                    written++;
                }
            }
        }
    }

    void MovieFFMpegWriter::validateCodecs(string* videoCodec,
                                           string* audioCodec)
    {
        //
        // Look for a good video or audio codec starting with what the user
        // requested followed by our preferred default and lastly the default
        // for the output format.
        //

        if (m_writeVideo)
        {
            vector<string> guesses;
            guesses.push_back(m_request.codec);
            guesses.push_back(RV_OUTPUT_VIDEO_CODEC);
            guesses.push_back(string(
                avcodec_get_name(m_avFormatContext->oformat->video_codec)));
            *videoCodec = getWriterCodec("video", guesses);
        }

        if (m_writeAudio)
        {
            vector<string> guesses;
            guesses.push_back(m_request.audioCodec);
            guesses.push_back(RV_OUTPUT_AUDIO_CODEC);
            guesses.push_back(string(
                avcodec_get_name(m_avFormatContext->oformat->audio_codec)));
            *audioCodec = getWriterCodec("audio", guesses);
        }

        DBL(DB_WRITE, "video codec: '" << videoCodec << "' audio codec: "
                                       << audioCodec << "'");
    }

    string MovieFFMpegWriter::getWriterCodec(std::string type,
                                             vector<string> guesses)
    {
        int avType =
            (type == "video") ? AVMEDIA_TYPE_VIDEO : AVMEDIA_TYPE_AUDIO;
        for (int i = 0; i < guesses.size(); i++)
        {
            string guess = guesses[i];
            if (guess == "")
                continue;

            // Check if we allow this (aka we are licensed)
            if (!m_io->codecIsAllowed(guess, false))
            {
                if (i == 0)
                    TWK_THROW_EXC_STREAM("Unsupported codec: " << guess);
                continue;
            }

            // Check if this is the right type for the track/stream
            const AVCodec* avCodec =
                avcodec_find_encoder_by_name(guess.c_str());
            if (!avCodec || avCodec->type != avType)
            {
                if (i == 0)
                    TWK_THROW_EXC_STREAM("Invalid " << type
                                                    << " codec: " << guess);
                continue;
            }

            // Check if the output format support it
            if (!avformat_query_codec(m_avOutputFormat, avCodec->id,
                                      FF_COMPLIANCE_NORMAL))
            {
                if (i == 0)
                    TWK_THROW_EXC_STREAM(
                        "Format does not support codec: " << guess);
                continue;
            }

            return guess;
        }
        TWK_THROW_EXC_STREAM("Failed to determine codec for " << type);
    }

    void MovieFFMpegWriter::addChapter(int id, int startFrame, int endFrame,
                                       string title)
    {
        AVChapter* chapter;
        chapter = (AVChapter*)av_mallocz(sizeof(AVChapter));
        if (!chapter)
            report("Unable to create chapter.", true);
        chapter->id = id;

        //
        // XXX This math here is a bit strange. We use 1 / (2 * FPS) for the
        // chapter time base. This allows us to round up to the nearest frame
        // for both the start and end chapter time values. The reason for this
        // is that it appears ffmpeg looses precision if you use the exact frame
        // number you want for the start and end timestamps, if the time base
        // matches the inverse of the FPS exactly. Therefore you can see below
        // that 1 is added to the double of each frame number. When divided by
        // the time base the effect is the same as using chapter start frame +
        // 0.5 or chapter end frame + 0.5. The result is that precision loss is
        // avoided.
        //

        chapter->time_base.num = m_duration;
        chapter->time_base.den = m_timeScale * 2;
        chapter->start = int64_t(startFrame * 2 + 1);
        chapter->end = int64_t(endFrame * 2 + 1);
        av_dict_set(&chapter->metadata, "title", title.c_str(), 0);
        m_avFormatContext->chapters[m_avFormatContext->nb_chapters++] = chapter;
    }

    MovieInfo MovieFFMpegWriter::open(const MovieInfo& info,
                                      const string& filename,
                                      const WriteRequest& request)
    {
        //
        //  Get all the interesting info about the input and write request
        //

        m_info = MovieWriter::open(info, filename, request);
        if (m_request.verbose)
            av_log_set_level(AV_LOG_INFO);
        if (reallyVerbose())
            av_log_set_level(AV_LOG_DEBUG);

        DBL(DB_WRITE,
            "verbose: " << request.verbose << " filename: " << filename);

        // Check if this is a supported output format
        string ext = boost::filesystem::extension(m_filename);
        if (ext[0] == '.')
            ext.erase(0, 1);
        avformat_alloc_output_context2(&m_avFormatContext, NULL, NULL,
                                       m_filename.c_str());
        if (!m_avFormatContext)
        {
            report(
                "Could not deduce output format from file extension using MOV",
                true);
            ext = "mov";
            avformat_alloc_output_context2(&m_avFormatContext, NULL, "mov",
                                           m_filename.c_str());
        }
        if (!m_avFormatContext)
        {
            TWK_THROW_EXC_STREAM("Unable to create output format");
        }
        m_avOutputFormat = m_avFormatContext->oformat;

        // Check if the output supports video and/or audio
        MovieFFMpegIO::MFFormatMap formats = m_io->getFormats();
        MovieFFMpegIO::MFFormatMap::iterator formatItr = formats.find(ext);
        if (formatItr == formats.end())
        {
            TWK_THROW_EXC_STREAM("Unsupported format: " << ext);
        }
        m_writeVideo = formatItr->second.second & MovieIO::MovieWrite;
        m_writeAudio =
            formatItr->second.second & MovieIO::MovieWriteAudio && m_info.audio;

        string videoCodecString = "";
        string audioCodecString = "";
        validateCodecs(&videoCodecString, &audioCodecString);
        collectWriteInfo(videoCodecString, audioCodecString);
        applyFormatParameters();

        // Add the tracks
        if (m_writeVideo)
        {
            if (m_request.stereo)
            {
                // Note: When adding the first track of a stereo clip, we make
                // sure NOT to erase the applied codec parameters when adding
                // that first track otherwise the same parameters won't be
                // applied to the second one.
                addTrack(true /*isVideo*/, videoCodecString,
                         false /*removeAppliedCodecParametersFromTheList*/);
                addTrack(true /*isVideo*/, videoCodecString);
            }
            else
            {
                addTrack(true /*isVideo*/, videoCodecString);
            }
        }
        if (m_writeAudio)
            addTrack(false, audioCodecString);

        // Add any chapters
        if (m_info.chapters.size() > 0)
        {
            AVChapter** chapters;
            chapters = (AVChapter**)av_realloc_f(
                m_avFormatContext->chapters, m_info.chapters.size(),
                sizeof(*m_avFormatContext->chapters));
            if (!chapters)
                report("Unable to add chapters", true);
            m_avFormatContext->chapters = chapters;
            for (int c = 0; c < m_info.chapters.size(); c++)
            {
                ChapterInfo ch = m_info.chapters[c];

                // Skip out of range chapters
                if (ch.startFrame > m_info.end || ch.endFrame < m_info.start)
                    continue;

                // Make sure that the start and end are in range
                int start = (ch.startFrame < m_info.start)
                                ? 0
                                : ch.startFrame - m_info.start;
                int end = (ch.endFrame > m_info.end)
                              ? m_info.end - m_info.start
                              : ch.endFrame - m_info.start;

                // End times are frame exclusive until the last chapter
                if (end + m_info.start < m_info.end)
                    end++;
                addChapter(c, start, end, ch.title);
            }
        }

        // Print warning if any parameters went unmatched
        for (map<string, string>::iterator missed = m_parameters.begin();
             missed != m_parameters.end(); missed++)
        {
            ostringstream message;
            message << "Could not match option: '" << missed->first << "'";
            report(message.str(), true);
        }

        // Open the output file, if needed
        if (!(m_avFormatContext->oformat->flags & AVFMT_NOFILE))
        {
            int ret = avio_open(&m_avFormatContext->pb, m_filename.c_str(),
                                AVIO_FLAG_WRITE);
            if (ret < 0)
            {
                TWK_THROW_EXC_STREAM("Could not open '" << m_filename << "'. "
                                                        << avErr2Str(ret));
            }
        }
        return m_info;
    }

    bool MovieFFMpegWriter::write(Movie* inMovie)
    {
        int ret = avformat_write_header(m_avFormatContext, NULL);
        if (ret < 0)
        {
            TWK_THROW_EXC_STREAM(
                "Error occurred when opening output file: " << avErr2Str(ret));
        }

        //
        // Setup the audio configuration of the source if there is audio.
        //

        if (m_info.audio)
        {
            Movie::AudioConfiguration conf(m_info.audioSampleRate, Stereo_2,
                                           m_audioFrameSize);
            inMovie->audioConfigure(conf);
        }

        //
        // This is the main encoding loop. For every frame to be written first
        // we write any audio up to that frame, then we write the video frame,
        // and lastly we write a stereo frame if necessary.
        //

        bool audioFinished = false;
        double totalAudioLength = double(m_frames.size()) / m_info.fps;
        double audioFrameLength =
            samplesToTime(m_audioFrameSize, m_info.audioSampleRate);
        for (int q = 0; q < m_frames.size(); q++)
        {
            const int f = m_frames[q];

            //
            // If we are not done writing audio and the next frame is comes
            // after the last samples we wrote, then better write some more.
            //

            while (m_writeAudio && !audioFinished
                   && ((q + 1) > (m_lastAudioTime * m_info.fps)))
            {
                double overflow =
                    totalAudioLength - (m_lastAudioTime + audioFrameLength);
                bool finalAudio = (overflow <= 0);
                audioFinished = fillAudio(inMovie, overflow, finalAudio);
            }

            //
            // Get the frame buffers from the reformating movie and write them
            //

            bool lastPass = q == (m_frames.size() - 1);
            FrameBufferVector fbs;
            inMovie->imagesAtFrame(Movie::ReadRequest(f, m_request.stereo),
                                   fbs);
            if (m_writeVideo)
                fillVideo(fbs, 0, q, lastPass);
            if (m_writeVideo && m_request.stereo)
                fillVideo(fbs, 1, q, lastPass);

            for (int v = 0; v < fbs.size(); v++)
                delete fbs[v];

            if (m_request.verbose)
            {
                float percent =
                    int(float(q) / float(m_frames.size() - 1) * 10000.0)
                    / float(100.0);
                ostringstream message;
                message << "Writing frame " << f << " (" << percent
                        << "% done)";
                report(message.str());
            }
        }
        av_write_trailer(m_avFormatContext);

        //
        // Clean up and close out
        //

        for (unsigned int i = 0; i < m_audioTracks.size(); i++)
        {
            AudioTrack* track = m_audioTracks[i];
            avcodec_free_context(&track->avCodecContext);
            delete track;
        }
        for (unsigned int i = 0; i < m_videoTracks.size(); i++)
        {
            VideoTrack* track = m_videoTracks[i];
            avcodec_free_context(&track->avCodecContext);
            delete track;
        }
        if (m_audioSamples)
            av_freep(&m_audioSamples);
        if (m_avFormatContext)
        {
            avformat_close_input(&m_avFormatContext);
            avformat_free_context(m_avFormatContext);
        }

        return true;
    }

    bool MovieFFMpegWriter::write(Movie* inMovie, const string& filename,
                                  WriteRequest& request)
    {
        m_canControlRequest = false;

        open(inMovie->info(), filename, request);

        ReformattingMovie* refMovie = new ReformattingMovie(inMovie);
        initRefMovie(refMovie);

        return write(refMovie);
    }

    //----------------------------------------------------------------------
    //
    // MovieFFMpegIO Class
    //
    //----------------------------------------------------------------------

    MovieFFMpegIO::MovieFFMpegIO(CodecFilterFunction codecFilter,
                                 bool bruteForce, int codecThreads,
                                 string language, double defaultFPS,
                                 void (*registerCustomCodecs)() /*=nullptr*/)
        : MovieIO("MovieFFMpeg", "v2")
        , m_codecFilter(codecFilter)
    {
        // Store if we are attempting bruteForce
        setIntAttribute("bruteForce", int(bruteForce));

        // Store the codec thread count
        setIntAttribute("codecThreads", codecThreads);

        // Store the language
        setStringAttribute("language", language);

        // Store a default frame rate just in case
        setDoubleAttribute("defaultFPS", defaultFPS);

        // Register custom codecs if requested (optional)
        if (registerCustomCodecs)
            registerCustomCodecs();

        // Init av
        avformat_network_init();

        // Tell library to use our logging callback
        av_log_set_callback(avLogCallback);
        av_log_set_level(DB_LOG_LEVEL);

        //
        // Add metadata fields, timecode, timescale, and duration
        //

        ParameterVector eparams;
        ParameterVector dparams;
        for (const char** p = metadataFieldsArray; *p; p++)
        {
            eparams.push_back(
                MovieIO::Parameter(*p, "string Metadata field", ""));
        }
        eparams.push_back(MovieIO::Parameter(
            "timecode",
            "string 'hh:mm:ss:ff' non-drop and 'hh:mm:ss;ff' drop or a frame #",
            ""));
        eparams.push_back(MovieIO::Parameter(
            "reelname",
            "string tape/reel name of the timecode (requires timecode)", ""));
        eparams.push_back(MovieIO::Parameter(
            "timescale", "integer literal timescale value", ""));
        eparams.push_back(MovieIO::Parameter(
            "duration", "integer literal duration value", ""));
        eparams.push_back(MovieIO::Parameter(
            "pix_fmt", "string video pixel format (i.e. yuv420p)", ""));
        eparams.push_back(MovieIO::Parameter(
            "sample_fmt", "string audio sample format (i.e. s16)", ""));
        eparams.push_back(
            MovieIO::Parameter("libx264autoresize",
                               "integer value of 1 will automatically alter "
                               "dimensions for H.264 encoding",
                               ""));
        collectParameters(avformat_get_class(), &eparams, &dparams, "", "f:");

        ParameterVector veparams;
        ParameterVector vdparams;
        collectParameters(avcodec_get_class(), &veparams, &vdparams, "",
                          "vcc:");

        ParameterVector aeparams;
        ParameterVector adparams;
        collectParameters(avcodec_get_class(), &aeparams, &adparams, "",
                          "acc:");

        //
        // Walk through all supported formats and codecs and filter out those we
        // don't allow
        //

        map<string, int> supported;
        for (const char** c = supportedEncodingCodecsArray; *c; c++)
        {
            supported[*c] = 1;
        }

        StringPairVector video;
        StringPairVector audio;
        MFFormatMap formats = getFormats();
        for (MFFormatMap::iterator formatsItr = formats.begin();
             formatsItr != formats.end(); formatsItr++)
        {
            video.clear();
            audio.clear();
            ParameterVector separams = eparams;
            ParameterVector sdparams = dparams;

            //
            //  Look for the formats we support
            //

            string fakefile = "test." + formatsItr->first;
            const AVOutputFormat* avOutputFormat =
                av_guess_format(NULL, fakefile.c_str(), NULL);
            if (avOutputFormat && avOutputFormat->priv_class)
            {
                collectParameters(avOutputFormat->priv_class, &separams,
                                  &sdparams, "", "f:");
            }

            //
            //  Collect codec specific information
            //

            map<string, int> codecs;
            void* iter = NULL;
            const AVCodec* codec;
            while ((codec = av_codec_iterate(&iter)))
            {
                if (codecs.find(codec->name) == codecs.end()
                    && codecIsAllowed(codec->name, false)
                    && av_codec_is_encoder(codec) != 0
                    && supported.find(codec->name) != supported.end())
                {
                    string prefix = "";
                    StringPair desc(codec->name, codec->long_name);
                    if (codec->type == AVMEDIA_TYPE_VIDEO
                        && formatsItr->second.second & MovieIO::MovieWrite)
                    {
                        video.push_back(desc);
                        prefix = "vc:";
                    }
                    else if (codec->type == AVMEDIA_TYPE_AUDIO
                             && formatsItr->second.second
                                    & MovieIO::MovieWriteAudio)
                    {
                        audio.push_back(desc);
                        prefix = "ac:";
                    }
                    if (prefix != "" && codec->priv_class)
                    {
                        collectParameters(codec->priv_class, &separams,
                                          &sdparams, codec->name, prefix);
                    }
                    codecs[codec->name] = 1;
                }
            }
            if (formatsItr->second.second & MovieIO::MovieWrite)
            {
                separams.insert(separams.end(), veparams.begin(),
                                veparams.end());
                sdparams.insert(sdparams.end(), vdparams.begin(),
                                vdparams.end());
            }
            if (formatsItr->second.second & MovieIO::MovieWriteAudio)
            {
                separams.insert(separams.end(), aeparams.begin(),
                                aeparams.end());
                sdparams.insert(sdparams.end(), adparams.begin(),
                                adparams.end());
            }
            addType(formatsItr->first, formatsItr->second.first,
                    formatsItr->second.second, video, audio, separams,
                    sdparams);
        }

        // Note : No longer using the global context pool (since FFmpeg 6.0)
        // Rationale: The global context pool was based on the premise that a
        // context could be opened and closed multiple times. However, with
        // FFmpeg 6.0, this premise is no longer valid and was causing crashes.
        // As per the FFmpeg 6 documentation:
        // https://ffmpeg.org/doxygen/trunk/deprecated.html: "Opening and
        // closing a codec context multiple times is not supported anymore – use
        // multiple codec contexts instead."

        // if (!globalContextPool)
        // {
        //     int poolSize = 500;
        //     if (const char* c = getenv("TWK_MOVIEFFMPEG_CONTEXT_POOL_SIZE"))
        //     {
        //         int poolSize = atoi(c);
        //     }
        //     if (poolSize > 0) globalContextPool = new ContextPool(poolSize);
        //     else
        //     {
        //         report("Disabling mio_ffmpeg context thread pool.", true);
        //     }
        // }
    }

    MovieFFMpegIO::~MovieFFMpegIO()
    {
        //  XXX delete context pool ?
    }

    string MovieFFMpegIO::about() const
    {
        ostringstream str;
        str << "ffmpeg: avformat version " << LIBAVFORMAT_VERSION_MAJOR << "."
            << LIBAVFORMAT_VERSION_MINOR << "." << LIBAVFORMAT_VERSION_MICRO
            << ", avcodec version " << LIBAVCODEC_VERSION_MAJOR << "."
            << LIBAVCODEC_VERSION_MINOR << "." << LIBAVCODEC_VERSION_MICRO
            << ", avutil version " << LIBAVUTIL_VERSION_MAJOR << "."
            << LIBAVUTIL_VERSION_MINOR << "." << LIBAVUTIL_VERSION_MICRO
            << ", swscale version " << LIBSWSCALE_VERSION_MAJOR << "."
            << LIBSWSCALE_VERSION_MINOR << "." << LIBSWSCALE_VERSION_MICRO;

        return str.str();
    }

    void MovieFFMpegIO::collectParameters(const AVClass* avClass,
                                          MovieIO::ParameterVector* eparams,
                                          MovieIO::ParameterVector* dparams,
                                          string codec, string prefix)

    {
        const AVOption* o = NULL;
        while ((o = av_opt_next(&avClass, o)))
        {
            if (o->type != AV_OPT_TYPE_CONST)
            {
                string description = "";
                string name = prefix + o->name;
                switch (o->type)
                {
                case AV_OPT_TYPE_BINARY:
                    description += "hexadecimal string";
                    break;
                case AV_OPT_TYPE_STRING:
                    description += "string";
                    break;
                case AV_OPT_TYPE_INT:
                case AV_OPT_TYPE_INT64:
                    description += "integer";
                    break;
                case AV_OPT_TYPE_FLOAT:
                case AV_OPT_TYPE_DOUBLE:
                    description += "float";
                    break;
                case AV_OPT_TYPE_RATIONAL:
                    description += "rational number";
                    break;
                case AV_OPT_TYPE_FLAGS:
                    description += "flags";
                    break;
                default:
                    description += "value";
                    break;
                }
                if (o->help)
                    description += " " + string(o->help);
                if (o->unit)
                {
                    const AVOption* u = NULL;
                    description += " (";
                    bool first = true;
                    while ((u = av_opt_next(&avClass, u)))
                    {
                        if (u->type == AV_OPT_TYPE_CONST && u->unit
                            && !strcmp(u->unit, o->unit))
                        {
                            if (!first)
                                description += ", ";
                            // Skip help for now
                            // string help = u->help ? string(u->help) : "";
                            description += string(u->name);
                            first = false;
                        }
                    }
                    description += ")";
                }
                if (o->flags & AV_OPT_FLAG_ENCODING_PARAM)
                {
                    eparams->push_back(
                        MovieIO::Parameter(name.c_str(), description, codec));
                }
                if (o->flags & AV_OPT_FLAG_DECODING_PARAM)
                {
                    dparams->push_back(
                        MovieIO::Parameter(name.c_str(), description, codec));
                }
            }
        }
    }

    MovieFFMpegIO::MFFormatMap MovieFFMpegIO::getFormats() const
    {
        unsigned int audcap = MovieIO::MovieReadAudio | MovieIO::MovieWriteAudio
                              | MovieIO::AttributeRead
                              | MovieIO::AttributeWrite;

        unsigned int vidcap = MovieIO::MovieRead | MovieIO::MovieWrite | audcap;

        if (this->bruteForce())
        {
            vidcap |= MovieIO::MovieBruteForceIO;
            audcap |= MovieIO::MovieBruteForceIO;
        }

        MovieFFMpegIO::MFFormatMap formats;

        // Video
        formats["avi"] = make_pair("Audio Video Interleave", vidcap);
        formats["flv"] = make_pair("Flash Video", vidcap);
        formats["m3u8"] = make_pair("M3U8 Stream Metadata", vidcap);
        formats["m4v"] = make_pair("iTunes Video Format (from MPEG-4)", vidcap);
        formats["mkv"] = make_pair("Matroska Video", vidcap);
        formats["mov"] = make_pair("Quicktime Movie", vidcap);
        formats["mp4"] = make_pair("MPEG-4 Movie Container", vidcap);
        formats["mpg"] = make_pair("MPEG Format", vidcap);
        formats["mxf"] = make_pair("Material eXchange Format", vidcap);
        //    formats["ogv"]  = make_pair("OGG Video", vidcap);
        //    formats["webm"] = make_pair("WEBM Video", vidcap);

        // Audio
        //    formats["aac"]  = make_pair("Advanced Audio Codec", audcap);
        formats["aif"] = make_pair("Apple AIFF audio file", audcap);
        formats["aifc"] = make_pair("Apple AIFC compressed audio file", audcap);
        formats["aiff"] = make_pair("Apple AIFF audio file", audcap);
        formats["au"] = make_pair("SUN Micosystems audio file", audcap);
        formats["mp3"] = make_pair("MPEG-3 Audio Container", audcap);
        formats["ogg"] = make_pair("Ogg audio file", audcap);
        formats["snd"] = make_pair("NeXT audio file", audcap);
        formats["wav"] = make_pair("Microsoft WAVE audio file", audcap);

        return formats;
    }

    void MovieFFMpegIO::getMovieInfo(const string& filename,
                                     MovieInfo& minfo) const
    {
        MovieFFMpegReader mov(this);
        mov.open(filename);
        minfo = mov.info();
    }

    string MovieFFMpegIO::language() const
    {
        return getStringAttribute("language");
    }

    int MovieFFMpegIO::bruteForce() const
    {
        return getIntAttribute("bruteForce");
    }

    int MovieFFMpegIO::codecThreads() const
    {
        return getIntAttribute("codecThreads");
    }

    MovieReader* MovieFFMpegIO::movieReader() const
    {
        return new MovieFFMpegReader(this);
    }

    MovieWriter* MovieFFMpegIO::movieWriter() const
    {
        return new MovieFFMpegWriter(this);
    }

    bool MovieFFMpegIO::codecIsAllowed(string name, bool forRead) const
    {
#ifdef SKIP_CODEC_CHECKS
        return true;
#else
        return (*m_codecFilter)(name, forRead);
#endif
    }

    double MovieFFMpegIO::defaultFPS() const
    {
        return getDoubleAttribute("defaultFPS");
    }
} // namespace TwkMovie
