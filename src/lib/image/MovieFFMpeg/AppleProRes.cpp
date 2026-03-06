//******************************************************************************
//
// Copyright (C) 2026 Apple, Inc., All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkFB/FastMemcpy.h>
#include <TwkFB/FastConversion.h>
#include <assert.h>

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
}

#include <AppleProRes.h>

// Returns the number of threads to use in the Apple ProRes decoder.
static int getNumProresThreads()
{
    // Only evaluate this value once.
    static int proresDecoderThreads = -1;
    if (proresDecoderThreads == -1)
    {
        const char* evPrefValue = getenv("ORV_PREF_GLOBAL_PRORES_DECODER_THREADS");
        if (evPrefValue && atoi(evPrefValue) >= 0)
        {
            proresDecoderThreads = atoi(evPrefValue);
        }
        else
        {
            // 0 = All available threads according to the number of
            // processors detected in the system
            proresDecoderThreads = 0;
        }
    }
    return proresDecoderThreads;
}

int AppleProResContext::decode_frame(AVCodecContext* avctx, AVFrame* videoFrame, AVPacket* videoPacket)
{
    int ret = 0;

    // Read info from the frame header including, width height, and color details.  First make sure we have enough
    // bytes for everything we want to read. The colorspace is at offset 24 (16 bytes past the 8 byte header size).
    // Any other errors will be caught when trying to decode the frame, so for now, just check the size.
    if (videoPacket->size < 25)
    {
        return AVERROR(EINVAL);
    }
    // Skip over the first 8 bytes which is the header size
    uint8_t* header_chunk = (uint8_t*)videoPacket->data + 8;
    uint16_t version = readBE(header_chunk + 2);
    uint16_t width = readBE(header_chunk + 8);
    uint16_t height = readBE(header_chunk + 10);

    // Color primaries, transfer function, color space, and color range
    videoFrame->color_primaries = static_cast<AVColorPrimaries>(header_chunk[14]);
    videoFrame->color_trc = static_cast<AVColorTransferCharacteristic>(header_chunk[15]);
    videoFrame->colorspace = static_cast<AVColorSpace>(header_chunk[16]);
    videoFrame->color_range = AVCOL_RANGE_MPEG;

    videoFrame->width = width;
    videoFrame->height = height;
    videoFrame->format = avctx->pix_fmt;
    avctx->sw_pix_fmt = avctx->pix_fmt;

    // Make sure our PixBuf is of the correct format and has sufficient memory/alignment
    int minBytes = PRBytesPerRowNeededInPixelBuffer(videoFrame->width, pixfmt, kPRFullSize);
    if ((prpixbuf.baseAddr == NULL) || (prpixbuf.rowBytes < minBytes) || (prpixbuf.width != videoFrame->width)
        || (prpixbuf.height != videoFrame->height) || (prpixbuf.format != pixfmt))
    {
        prpixbuf.rowBytes = minBytes;
        prpixbuf.width = videoFrame->width;
        prpixbuf.height = videoFrame->height;
        prpixbuf.format = pixfmt;
        if (prpixbuf.baseAddr)
        {
            av_free(prpixbuf.baseAddr);
            prpixbuf.baseAddr = NULL;
        }
        prpixbuf.baseAddr = (unsigned char*)av_malloc(prpixbuf.rowBytes * prpixbuf.height);
        if (NULL == prpixbuf.baseAddr)
        {
            return AVERROR(ENOMEM);
        }
    }

    // Make sure we have memory in which to decode.  We assume that if the
    // linesize[0] isn't initialized that we haven't gotten the buffer, yet.
    if (!videoFrame->linesize[0])
    {
        av_frame_get_buffer(videoFrame, 0);
    }

    // Allocate a decoder if we don't have one
    if (!prdecoder)
    {
        prdecoder = PROpenDecoder(getNumProresThreads(), NULL);
        if (NULL == prdecoder)
        {
            return AVERROR(ENOMEM);
        }
    }

    // Now decode the frame
    int bytesDecoded = PRDecodeFrame(prdecoder, videoPacket->data, videoPacket->size, &prpixbuf, kPRFullSize, 0);
    if (bytesDecoded != videoPacket->size)
    {
        av_log(avctx, AV_LOG_ERROR, "Expected to decode %d bytes but decoded %d\n", videoPacket->size, bytesDecoded);
        assert(0);
        return AVERROR(EINVAL);
    }
    else
    {
        // We're assuming v216 is used for 4:2:2 formats and y416 for 4:4:4:4 formats. Convert from the packed format that
        // ProRes uses to the planar format that are used elsewhere by FFMpeg
        if (pixfmt == kPRFormat_v216)
        {
            packedUYVY16_to_planarYUV16_MP(minBytes, videoFrame->height, reinterpret_cast<uint16_t*>(prpixbuf.baseAddr),
                                           reinterpret_cast<uint16_t*>(videoFrame->data[0]),
                                           reinterpret_cast<uint16_t*>(videoFrame->data[1]),
                                           reinterpret_cast<uint16_t*>(videoFrame->data[2]), videoFrame->linesize[0],
                                           videoFrame->linesize[1], videoFrame->linesize[2]);
        }
        else if (pixfmt == kPRFormat_y416)
        {
            packedUVYA16_to_planarYUVA16_MP(
                minBytes, videoFrame->height, reinterpret_cast<uint64_t*>(prpixbuf.baseAddr),
                reinterpret_cast<uint16_t*>(videoFrame->data[0]), reinterpret_cast<uint16_t*>(videoFrame->data[1]),
                reinterpret_cast<uint16_t*>(videoFrame->data[2]), reinterpret_cast<uint16_t*>(videoFrame->data[3]), videoFrame->linesize[0],
                videoFrame->linesize[1], videoFrame->linesize[2], videoFrame->linesize[3]);
        }
        else
        {
            av_log(avctx, AV_LOG_ERROR, "Expected either v216 (value %d) or y416 (value %d) format but got %d\n", kPRFormat_v216,
                   kPRFormat_y416, pixfmt);
            assert(0);
        }
    }

    av_packet_unref(videoPacket);

    return 0;
}
