//******************************************************************************
// Copyright (c) 2008 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __LUT__ReadLUT__h__
#define __LUT__ReadLUT__h__
#include <string>
#include <vector>
#include <TwkMath/Mat44.h>
#include <TwkMath/Vec3.h>

namespace LUT
{

    /// All the data that can possibly be returned by a LUT reader

    struct LUTData
    {
        typedef std::pair<float, float> MappingPair;
        typedef std::vector<MappingPair> ChannelMap;
        typedef std::vector<float> Data;
        typedef TwkMath::Mat44f Mat44f;
        typedef TwkMath::Vec3i Vec3i;
        typedef std::vector<size_t> Dimensions;

        std::string name;

        Mat44f inMatrix;      // applied in place of or before prelut
        ChannelMap prelut[3]; // shapes data if non-linear
        Dimensions dimensions;
        Data prelutData;         // resampled preluts
        Data data;               // actual lut data
        Mat44f outMatrix;        // output matrix if lut is HDR out
        float conditioningGamma; // gamma applied pre-prelut (only CSP at the
                                 // moment)

        LUTData()
            : conditioningGamma(1.0)
        {
        }
    };

    /// Is the file a LUT file?

    bool isLUTFile(const std::string& filename);

    /// Generic read a LUT file format

    ///
    /// Reads a LUT file which may include an input matrix, 3 channel
    /// LUTs, and a 3D LUT. The output is assumed to be in the range [0,1]
    /// in all channels.
    ///
    /// The calling function should leave the various struct fields
    /// blank. They will be filled in by the file readers. In the most
    /// general case, the LUT will contain an arbitrary set of HDR channel
    /// luts which map unbounded values to the [0,1] range. The 3D LUT is
    /// assumed to operate in the [0,1] range only.
    ///
    /// The MappingPairs in the ChannelMaps may not be uniformly
    /// distributed. Call makeUniform() after the LUT is read if you need
    /// uniform entries.
    ///
    /// \param<filename> the file to read.
    ///

    void readFile(const std::string& filename, LUTData& data);

    ///
    /// Resample 3D LUT to a power of two for best performance on a
    /// graphics card. Has no effect on non-3D luts.
    ///

    void resamplePowerOfTwo(LUTData& data);

    ///
    /// Resample prelut data into a new LUT (with no pre-lut data). The
    /// LUT is a channel LUT with evenly spaced values. You supply the
    /// number of output samples.
    ///

    void compilePreLUT(LUTData& inlut, size_t nsamples);

    ///
    /// Simplify existing preluts to a matrix if possible. Returns true if the
    /// prelut can be represented as a matrix (or if its not there at all)
    ///

    bool simplifyPreLUT(LUTData& lut);

    ///
    /// Normalize the LUT data. Returns the scale factor
    ///

    float normalize(LUTData& lut, bool powerOfTwoRounded = false);

} // namespace LUT

#endif // __LUT__ReadLUT__h__
