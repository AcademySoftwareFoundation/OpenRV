//******************************************************************************
// Copyright (c) 2001-2007 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <IOrla/rlaFormat.h>
#include <string.h>
#include <TwkUtil/ByteSwap.h>

namespace TwkFB
{

    // *****************************************************************************
    void RLAHeader::conformByteOrder()
    {
#if defined(TWK_LITTLE_ENDIAN) || defined(__LITTLE_ENDIAN__)

        TwkUtil::swapShorts(&window.left, 14); // swaps window.left -> revision
        TwkUtil::swapWords(&job_num, 1);
        TwkUtil::swapShorts(&field, 1);
        TwkUtil::swapShorts(&chan_bits, 5); // swaps chan_bits -> aux_bits
        TwkUtil::swapWords(&next, 1);

#endif
    }

    // *****************************************************************************
    // Decodes one run-encoded channel from input buffer.
    unsigned char* rlaDecodeScanline(unsigned char* input,
                                     unsigned char* output, int xFile,
                                     int xImage, int stride)
    {
        int x = xFile;
        int useX = 0;
        int xMax = xFile < xImage ? xFile : xImage;

        unsigned char* out = (unsigned char*)output;
        while (x > 0)
        {
            int count = *(signed char*)input++;
            if (count >= 0)
            {
                // Repeat pixel value (count + 1) times.
                while (count-- >= 0)
                {
                    if (useX < xImage)
                    {
                        *out = *input;
                        out += stride;
                    }
                    --x;
                    useX++;
                }
                ++input;
            }
            else
            {
                // Copy (-count) unencoded values.
                for (count = -count; count > 0; --count)
                {
                    if (useX < xImage)
                    {
                        *out = *input;
                        out += stride;
                    }
                    input++;
                    --x;
                    useX++;
                }
            }
        }
        return input;
    }

    // *****************************************************************************
    MaxExtras getMaxExtras(const RLAHeader& h)
    {
        const char* p = strstr(h.program, "3DStudioMAX");
        if (!p)
        {
            p = strstr(h.program, "3ds max");
            if (!p)
                return MAX_NO_EXTRAS;
        }

        unsigned int extras = MAX_NO_EXTRAS;
        while (*p != '\0')
        {
            switch (*p)
            {
            case 'Z':
                extras |= MAX_GB_Z;
                break;
            case 'E':
                extras |= MAX_GB_MTL_ID;
                break;
            case 'O':
                extras |= MAX_GB_NODE_ID;
                break;
            case 'U':
                extras |= MAX_GB_UV;
                break;
            case 'N':
                extras |= MAX_GB_NORMAL;
                break;
            case 'R':
                extras |= MAX_GB_REALPIX;
                break;
            case 'C':
                extras |= MAX_GB_COVERAGE;
                break;
            case 'B':
                extras |= MAX_GB_BG;
                break;
            case 'I':
                extras |= MAX_GB_NODE_RENDER_ID;
                break;
            case 'G':
                extras |= MAX_GB_COLOR;
                break;
            case 'T':
                extras |= MAX_GB_TRANSP;
                break;
            case 'V':
                extras |= MAX_GB_VELOC;
                break;
            case 'W':
                extras |= MAX_GB_WEIGHT;
                break;
            case 'M':
                extras |= MAX_GB_MASK;
                break;
            case 'L':
                extras |= MAX_LAYER_DATA;
                break;
            case 'P':
                extras |= MAX_RENDER_INFO;
                break;
            case 'D':
                extras |= MAX_NODE_TABLE;
                break;
            case 'A':
                extras |= MAX_UNPREMULT;
                break;
            }
            ++p;
        }

        return (MaxExtras)extras;
    }

} //  End namespace TwkFB
