//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//either i0 or i1 is chosen randomly at every fragment. 
// The value of p is used as a probability factor for choosing the i0. 
vec4 main(const in inputImage in0, const in inputImage in1)
{
	float p = 0.5;
	float noiseScale = 1.0;
	float noise = (noise1(vec2(in0.st)) + 1.0) * 0.5; // noise is in [0, 1]
	if (noise < p)
            return in1();
	else
            return in0();
}
