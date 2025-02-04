//******************************************************************************
// Copyright (c) 2015 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __mp4v2Utils__h__
#define __mp4v2Utils__h__
#include <string>
#include <stdint.h>

namespace mp4v2Utils
{

    //
    //  File Management
    //
    bool readFile(std::string filename, void*& fileHandle);
    bool modifyFile(std::string filename, void*& fileHandle);
    bool closeFile(void*& fileHandle);

    //
    //  COLR Reading
    //
    bool getFOURCC(void* fileHandle, int streamIndex, std::string& fourcc);
    bool assembleColrAtomName(void* fileHandle, int streamIndex,
                              std::string& atomName);
    bool getColrType(void* fileHandle, int streamIndex, std::string& colrType);
    bool getNCLCValues(void* fileHandle, int streamIndex, uint64_t& prim,
                       uint64_t& xfer, uint64_t& mtrx);
    bool getPROFValues(void* fileHandle, int streamIndex,
                       unsigned char*& profile, uint32_t& size);

    //
    //  COLR Writing
    //
    bool addColrAtom(void* fileHandle, int streamIndex, uint64_t prim,
                     uint64_t xfer, uint64_t mtrx);

    //
    //  AVdn Reading
    //
    bool getAVdnRange(void* fileHandle, int streamIndex, uint64_t& range);

    //
    //  Tmcd Reading/Writing
    //
    bool getTmcdName(void* fileHandle, int streamIndex, std::string& reelName);
    bool setTmcdName(void* fileHandle, int streamIndex, std::string reelName);

} // namespace mp4v2Utils

#endif // __mp4v2Utils__h__
