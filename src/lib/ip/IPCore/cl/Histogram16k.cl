//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
// This is a 256 bin histogram.
// assuming the data 4 floats per pixel in [0,1] if not, <0 will be counted as 0, and >1 will be counted as 1
// also assuming there is no scanline padding
// Note alpha channel will not be processed into the histogram
//
#define HISTOGRAM_BIN_NO 256
#define NBANKS 4 // each work group uses 16 sub histograms to reduce mem conflicts
#define CHANNEL 3 // a histogram for each channel
__kernel
void histogram256_float4(read_only image2d_t image,
                         __global uint* histogram,
                         uint noVecPerWorkItem,
                         uint imageWidth,
                         uint imageHeight)
{
    __local uint subHistograms[CHANNEL * NBANKS * HISTOGRAM_BIN_NO];
    uint groupID         = get_group_id(0); // the ith work group
    uint groupID2        = get_group_id(1); // the ith work group
    uint globalID        = get_global_id(0); // global id into the image array, i.e., ith float4 in the array
    uint globalID2       = get_global_id(1); // global id into the image array, i.e., ith float4 in the array
    uint localID         = get_local_id(0); // thread id in a work group
    uint localID2        = get_local_id(1); // thread id in a work group
    uint groupNo         = get_global_size(0) / get_local_size(0);
    uint localSize       = get_local_size(0);
    uint localSize2      = get_local_size(1);
    uint bankOffset      = (localID + localSize * localID2) % NBANKS;
    // clear all subHistograms to 0
    for (uint i = localID; i < HISTOGRAM_BIN_NO; i+= localSize)
    {
        for (uint j = 0; j < CHANNEL * NBANKS; j++)
        {
            subHistograms[i * CHANNEL * NBANKS + j] = 0;
        }
    }
    
    //read and scatter
    const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;
    barrier(CLK_LOCAL_MEM_FENCE);
    for (uint i = 0, oy = globalID2; i < noVecPerWorkItem; i++, oy += localSize2)
    {
        uint ox = globalID;
        if (ox < imageWidth && oy < imageHeight) {
            float4 c = read_imagef(image, sampler, (int2)(ox, oy));
            c = max(0.0f, min(1.0f, c));
            uint r = c.x * 255;
            uint g = c.y * 255;
            uint b = c.z * 255;
            uint rLoc = (CHANNEL * r + 0) * NBANKS + bankOffset;
            uint gLoc = (CHANNEL * g + 1) * NBANKS + bankOffset;
            uint bLoc = (CHANNEL * b + 2) * NBANKS + bankOffset;
            (void)atomic_inc(subHistograms + rLoc);
            (void)atomic_inc(subHistograms + gLoc);
            (void)atomic_inc(subHistograms + bLoc);
        }
    }
    
    barrier(CLK_LOCAL_MEM_FENCE);
    // merge sub histograms into group histogram
    for (uint binIndex = localID; binIndex < HISTOGRAM_BIN_NO; binIndex += localSize)
    {
        uint c1 = 0;
        uint c2 = 0;
        uint c3 = 0;
        for (uint i = 0; i < NBANKS; i++)
        {
            c1 += subHistograms[(CHANNEL * binIndex + 0) * NBANKS + i];
            c2 += subHistograms[(CHANNEL * binIndex + 1) * NBANKS + i];
            c3 += subHistograms[(CHANNEL * binIndex + 2) * NBANKS + i];
        }
        uint index = ((groupID + groupNo * groupID2) * HISTOGRAM_BIN_NO + binIndex) * CHANNEL;
        histogram[index] = c1;
        histogram[index + 1] = c2;
        histogram[index + 2] = c3;
    }
}
//
// this kernel merges all work group histograms into the final histogram
// this kernel should be launched at 256(BINNO) work items per workgroup. and only 1 workgroup
__kernel __attribute__((reqd_work_group_size(HISTOGRAM_BIN_NO,1,1)))
void mergeHistograms256_float4(__global const uint* groupHistograms, // all group level histograms
                               write_only image2d_t histogram, // final histogram
                               uint histogramNo, // histogramNo is the number of work groups
                               float imgSize)
{
    uint globalID = get_global_id(0); // 0-255
    uint c1 = 0;
    uint c2 = 0;
    uint c3 = 0;
    for (uint i = 0; i < histogramNo; i++)
    {
        c1 += groupHistograms[(i * HISTOGRAM_BIN_NO + globalID) * CHANNEL];
        c2 += groupHistograms[(i * HISTOGRAM_BIN_NO + globalID) * CHANNEL + 1];
        c3 += groupHistograms[(i * HISTOGRAM_BIN_NO + globalID) * CHANNEL + 2];
    }
    write_imagef(histogram, (int2)(globalID, 0), (float4)(c1 / imgSize, c2 / imgSize, c3 / imgSize, 1.0f));
}
