//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
// jn 10, in 100000
// grits opt 12/18/2002: 221.340u 0.000s 3:41.38 99.9%   0+0k 0+0io 313pf+0w
//			 (unsafe, no 1-bit ref, setjmp always)
// grits opt 12/31/2002: 242.640u 0.000s 4:02.65 99.9%   0+0k 0+0io 316pf+0w
//			 (safe, no 1-bit ref, setjmp always)


// jn 10, in 10000, 65000
// 101.540u 0.400s 1:58.04 86.3%   0+0k 0+0io 0pf+0w
// 6500
// 106.560u 1.110s 2:04.31 86.6%   0+0k 0+3io 0pf+0w

use runtime;
use math_util;

// \: gcc (void;)
// {
//     print("gc: " +
//           "collected " + gc_collected_objects() +
//           ", scanned " + gc_scanned_objects() + "\n");
// }

//runtime.set_collect_threshold(6500);
//runtime.set_aux_collect_threshold(1024 * 11);
// runtime.call_on_collect(gcc);

module: garbage {

int jn = 70;
int in = 100;

 \: test (void; int jn=7, int in=100)
 {

     for (int j=0; j < jn; j++)
     {
         int delta = in / 2;
         //in += delta / 2 - math_util.random(delta);

         for (int i=0; i < in; i++)
         {
             string s = "i = " + i + "\n";
             string t = s + s + s + s + s + s + s + s + s + s + s + s + s + s + s;

             for (int q=1; q < t.size(); q++)
             {
                 string y = t.substr(0,q);
             }
         }
     }
 }

}

garbage.test(10,700);
