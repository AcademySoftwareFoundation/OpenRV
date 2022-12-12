//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
/*
 * The Great Computer Language Shootout
 * http://shootout.alioth.debian.org/
 *
 * contributed by Christoph Bauer
 *
 */

use math;
use math_util;

Vec := vector float[3];

global let SolarMass = 4.0 * float(math.pi) * float(math.pi),
         DaysPerYear = 365.24;

class: Planet 
{
    Vec pos;
    Vec vel;
    float mass;
}

\: advance (Planet[] bodies, float dt)
{
    let nbodies = bodies.size();

    for (int i = 0; i < nbodies; i++) 
    {
        Planet b = bodies[i];

        for (int j = i + 1; j < nbodies; j++) 
        {
            let b2 = bodies[j],
                dp = b.pos - b2.pos,
                d  = mag(dp),
                m  = dt / (d * d * d);

            b.vel  -= dp * b2.mass * m;
            b2.vel += dp * b.mass * m;
        }
    }
    
    for_each (b; bodies) b.pos += dt * b.vel;
}

\: energy (float; Planet[] bodies)
{
    let en = 0.0,
        nbodies = bodies.size();

    for (int i = 0; i < nbodies; i++) 
    {
        let b = bodies[i];
        en += 0.5 * b.mass * dot(b.vel, b.vel);

        for (int j = i + 1; j < nbodies; j++) 
        {
            let b2 = bodies[j],
                dp = b.pos - b2.pos;

            en -= (b.mass * b2.mass) / mag(dp);
        }
    }

    en;
}

\: offset_momentum (Planet[] bodies)
{
    let p = Vec(0.0);
    for_each (b; bodies) p += b.vel * b.mass;
    bodies[0].vel = -p / SolarMass;
}

global Planet[] bodies = 
{
    {                               /* sun */
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, SolarMass
    },
    {                               /* jupiter */
        {4.84143144246472090e+00, -1.16032004402742839e+00, -1.03622044471123109e-01},
        Vec(1.66007664274403694e-03, 7.69901118419740425e-03, -6.90460016972063023e-05) * DaysPerYear,
        9.54791938424326609e-04 * SolarMass
    },
    {                               /* saturn */
        {8.34336671824457987e+00, 4.12479856412430479e+00, -4.03523417114321381e-01},
        Vec(-2.76742510726862411e-03, 4.99852801234917238e-03, 2.30417297573763929e-05) * DaysPerYear,
        2.85885980666130812e-04 * SolarMass
    },
    {                               /* uranus */
        {1.28943695621391310e+01, -1.51111514016986312e+01, -2.23307578892655734e-01},
        Vec(2.96460137564761618e-03, 2.37847173959480950e-03, -2.96589568540237556e-05) * DaysPerYear,
        4.36624404335156298e-05 * SolarMass
    },
    {                               /* neptune */
        {1.53796971148509165e+01, -2.59193146099879641e+01, 1.79258772950371181e-01},
        Vec(2.68067772490389322e-03, 1.62824170038242295e-03, -9.51592254519715870e-05) * DaysPerYear,
        5.15138902046611451e-05 * SolarMass
    }
};

\: start (int n)
{
  offset_momentum(bodies);          print("%.9f\n" % energy(bodies));
  repeat (n) advance(bodies, 0.01); print("%.9f\n" % energy(bodies));
}

start(100);
