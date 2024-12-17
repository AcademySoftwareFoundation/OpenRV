//******************************************************************************
// Copyright (c) 2001-2007 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef __RLAFORMAT_H__
#define __RLAFORMAT_H__

#include <TwkUtil/ByteSwap.h>
#include <TwkMath/Vec2.h>
#include <TwkMath/Mat44.h>
#include <TwkMath/Box.h>

namespace TwkFB
{

    const short RLA_MAGIC = 0xFFFE;
    const short RPF_MAGIC = 0xFFFD;

    struct RLAWindow
    {
        short left;
        short right;
        short bottom;
        short top;
    };

    struct RLAHeader
    {
        RLAWindow window;
        RLAWindow active_window;
        short frame;
        short storage_type;
        short num_chan;
        short num_matte;
        short num_aux;
        short revision;
        char gamma[16];
        char red_pri[24];
        char green_pri[24];
        char blue_pri[24];
        char white_pt[24];
        int job_num;
        char name[128];
        char desc[128];
        char program[64];
        char machine[32];
        char user[32];
        char date[20];
        char aspect[24];
        char aspect_ratio[8];
        char chan[32];
        short field;
        char time[12];
        char filter[32];
        short chan_bits;
        short matte_type;
        short matte_bits;
        short aux_type;
        short aux_bits;
        char aux[32];
        char space[36];
        int next;

        void conformByteOrder();
    };

    unsigned char* rlaDecodeScanline(unsigned char* input,
                                     unsigned char* output, int xFile,
                                     int xImage, int stride);

    // *****************************************************************************
    // Stuff for reading Discreet's poorly implemented and RLA "extensions"
    // *****************************************************************************

    enum MaxExtras
    {
        MAX_NO_EXTRAS = 0,
        MAX_GB_Z = 1 << 1,
        MAX_GB_MTL_ID = 1 << 2,
        MAX_GB_NODE_ID = 1 << 3,
        MAX_GB_UV = 1 << 4,
        MAX_GB_NORMAL = 1 << 5,
        MAX_GB_REALPIX = 1 << 6,
        MAX_GB_COVERAGE = 1 << 7,
        MAX_GB_BG = 1 << 8,
        MAX_GB_NODE_RENDER_ID = 1 << 9,
        MAX_GB_COLOR = 1 << 10,
        MAX_GB_TRANSP = 1 << 11,
        MAX_GB_VELOC = 1 << 12,
        MAX_GB_WEIGHT = 1 << 13,
        MAX_GB_MASK = 1 << 14,
        MAX_LAYER_DATA = 1 << 15,
        MAX_RENDER_INFO = 1 << 16,
        MAX_NODE_TABLE = 1 << 17,
        MAX_UNPREMULT = 1 << 18
    };

    MaxExtras getMaxExtras(const RLAHeader& h);

    enum MaxProjectionType
    {
        ProjPerspective = 0,
        ProjParallel = 1
    };

    struct MaxMatrix43
    {
        // In MAX, matrices have 4 rows of 3 columns. Usually, 3D matrices are
        // 4x4 instead of 4x3. It's also the case for MAX, but as the last
        // column is always [0 0 0 1] for such matrices, it's implicitly managed
        // by the class and functions dealing with it.
        //
        // The coordinate system used by max is a right-handed one, with
        // counter-clockwise angles when looking to the positive way of an axis.

        float m[4][3];
        int flags;

        TwkMath::Mat44f asMat44() const
        {
            return TwkMath::Mat44f(m[0][0], m[0][1], m[0][2], 0.0f, m[1][0],
                                   m[1][1], m[1][2], 0.0f, m[2][0], m[2][1],
                                   m[2][2], 0.0f, m[3][0], m[3][1], m[3][2],
                                   1.0f);
        }
    };

    struct MaxRECT
    {
        int left;
        int top;
        int right;
        int bottom;
    };

    typedef int MaxBOOL;

    struct MaxRenderInfo
    {
        MaxProjectionType projType;
        TwkMath::Vec2f projScale; // 3D to 2D projection scale factor
        TwkMath::Vec2f origin;    // screen origin
        MaxBOOL fieldRender;      // field rendered?
        MaxBOOL fieldOdd;         // if true, the first field is Odd lines

        // Render time and tranformations for the 2 fields, if field rendering.
        // If not, use renderTime[0], etc.
        int renderTime[2];
        MaxMatrix43 worldToCam[2];
        MaxMatrix43 camToWorld[2];
        MaxRECT region; // sub-region in image that was rendered if last
                        // render was a region render. Empty if not a
                        // region render. -- DS--7/13/00
    };

#endif // End #ifdef __RLAFORMAT_H__

} //  End namespace TwkFB
