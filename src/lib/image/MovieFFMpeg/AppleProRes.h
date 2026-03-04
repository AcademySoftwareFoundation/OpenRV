//******************************************************************************
//
// Copyright (C) 2026 Apple, Inc., All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <ProResDecoder.h>

struct AppleProResContext
{
    PRPixelBuffer prpixbuf;
    PRDecoderRef prdecoder;
    PRPixelFormat pixfmt;
    AVPixelFormat avPixelFormat;

    void clearPRPixBuf() { memset(&prpixbuf, 0, sizeof(prpixbuf)); }

    AppleProResContext()
    {
        clearPRPixBuf();
        prdecoder = nullptr;
        pixfmt = kPRFormat_v216;
    }

    ~AppleProResContext()
    {
        if (prpixbuf.baseAddr)
        {
            av_freep(prpixbuf.baseAddr);
        }
        if (prdecoder)
        {
            PRCloseDecoder(prdecoder);
        }
    }

    AppleProResContext(const AppleProResContext&) = delete;

    AppleProResContext& operator=(const AppleProResContext& other)
    {
        if (this == &other)
        {
            return *this;
        }
        pixfmt = other.pixfmt;
        avPixelFormat = other.avPixelFormat;
        clearPRPixBuf();
        prdecoder = nullptr;
        return *this;
    }

    AppleProResContext(AppleProResContext&&) = delete;
    AppleProResContext& operator=(AppleProResContext&&) = delete;

    // Reads 2 big-Endian bytes from memory and returns a uint16_t
    uint16_t readBE(uint8_t* data)
    {
        uint16_t res = (((uint16_t)(*data)) << 8) | (uint16_t)(*(data + 1));
        return res;
    }

    int decode_frame(AVCodecContext* avctx, AVFrame* videoFrame, AVPacket* videoPacket);
};
