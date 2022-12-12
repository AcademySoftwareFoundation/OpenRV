//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

BAILOUT := 16;
MAX_ITERATIONS := 1000;



\: mandelbrot(int; float x, float y)
{
    let cr = y - 0.5,
        ci = x,
        zi = 0.0,
        zr = 0.0;

    for (int i=0; i <= MAX_ITERATIONS; i++)
    {
        let temp = zr * zi,
            zr2 = zr * zr,
            zi2 = zi * zi;

        zr = zr2 - zi2 + cr;
        zi = temp + temp + ci;
 		  
        if ((zi2 + zr2) > BAILOUT) return i;
    }

    return 0;
}


\: doit (void;)
{
    for (float y = -39.0; y < 39; y += 1.0)
    {
        print("\n");

        for (float x = -39.0; x < 39; x += 1.0)
        {
            let i = mandelbrot(x/40.0, y/40.0);
        
            if (i == 0) print("*");
            else print(" ");
        }
    }
}

doit();
