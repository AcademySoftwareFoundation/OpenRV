//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

use math;
use math_util;
v3 := vector float[3];
v2 := vector float[2];

assert( smoothstep(0, 1, .25) == 0.15625 );
assert( linstep(0, 1, .25) == .25 );
assert( cbrt(8.0) == 2.0 );
assert( hypot(5.0, 12.0) == 13.0 );
assert( math.abs(acos(0) * 2.0 - math.pi) < 0.00001 );

noise(1.1);
noise(v2(1.1,2.2));
noise(v3(1.2,8.2,3.3));
