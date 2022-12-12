//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
vec4 ResizeDownSampleDerivative( const in inputImage in0,
                                 const in outputImage win )
{
    // Compute the sum of absolute derivates of texcoords in x/y.
    vec2 absDer = fwidth( in0.st );

    // Get the max out of x/y abs derivatives.
    float maxAbsDer = max( absDer.x, absDer.y );

    // We mimic a GL_LINEAR_MIPMAP_NEAREST implementation.
    // We divide by 2 because the kernel is ~symmetrical (we assume that the
    // center is neglictable)
    int KERNEL_SIZE = int( maxAbsDer / 2.0 );

    // Clamp the kernel in a "reasonable" range, yet equivalent to a mipmap 
    // level of >5.
    // 
    // KERNEL_SIZE == 0  --> 1x1 (no-op, mipmap level 0)
    // KERNEL_SIZE == 1  --> 3x3 (> mipmap level 1)
    // KERNEL_SIZE == 2  --> 5x5 (> mipmap level 2)
    // KERNEL_SIZE == 3  --> 7x7
    // KERNEL_SIZE == 4  --> 9x9 (> mipmap level 3)
    // KERNEL_SIZE == 5  --> 11x11 
    // KERNEL_SIZE == 6  --> 13x13 
    // KERNEL_SIZE == 7  --> 15x15 
    // KERNEL_SIZE == 8  --> 17x17 (> mipmap level 4)
    // KERNEL_SIZE == 9  --> 19x19 
    // KERNEL_SIZE == 10 --> 21x21 
    // KERNEL_SIZE == 11 --> 23x23 
    // KERNEL_SIZE == 12 --> 25x25 
    // KERNEL_SIZE == 13 --> 27x27 
    // KERNEL_SIZE == 14 --> 29x29 
    // KERNEL_SIZE == 15 --> 31x31 
    // KERNEL_SIZE == 16 --> 33x33 (> mipmap level 5)

    if ( KERNEL_SIZE < 0 )
    {
        KERNEL_SIZE = 0;
    }
    else if ( KERNEL_SIZE > 16 )
    {
        KERNEL_SIZE = 16;
    }

    // cumulate all the sample colors
    vec4 averageColor=vec4(0.0,0.0,0.0,0.0);
    for ( int x = -KERNEL_SIZE ; x <= KERNEL_SIZE ; x++ )
    {
        for ( int y = -KERNEL_SIZE ; y <= KERNEL_SIZE ; y++ )
        {
            averageColor+= in0( vec2 ( x , y ) );
        }
    }

    // divide by the number of samples
    float nbSamples = float( ( KERNEL_SIZE * 2 + 1 )* ( KERNEL_SIZE * 2 + 1 ) );
    averageColor /= vec4(nbSamples);

    return averageColor;
}

