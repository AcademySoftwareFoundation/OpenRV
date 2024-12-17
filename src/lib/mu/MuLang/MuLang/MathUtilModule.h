#ifndef __MuLang__MathUtilModule__h__
#define __MuLang__MathUtilModule__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/Module.h>
#include <Mu/Node.h>

namespace Mu
{
    class Context;

    class MathUtilModule : public Module
    {
    public:
        MathUtilModule(Context*);
        ~MathUtilModule();

        //
        //  Load function is called when the symbol is added to the
        //  context.
        //

        virtual void load();

        //
        //  Function Nodes
        //

        static NODE_DECLARAION(clamp, float);
        static NODE_DECLARAION(step, float);
        static NODE_DECLARAION(smoothstep, float);
        static NODE_DECLARAION(linstep, float);
        static NODE_DECLARAION(hermite, float);
        static NODE_DECLARAION(lerp, float);
        static NODE_DECLARAION(lerp2f, Vector2f);
        static NODE_DECLARAION(lerp3f, Vector3f);
        static NODE_DECLARAION(lerp4f, Vector4f);
        static NODE_DECLARAION(degrees, float);
        static NODE_DECLARAION(radians, float);
        static NODE_DECLARAION(randomf2, float);
        static NODE_DECLARAION(randomf, float);
        static NODE_DECLARAION(random, int);
        static NODE_DECLARAION(gauss, float);
        static NODE_DECLARAION(seed, void);
        static NODE_DECLARAION(sphrand, Vector3f);
        static NODE_DECLARAION(rotate, Vector3f);

        static NODE_DECLARAION(noise1, float);
        static NODE_DECLARAION(noise2, float);
        static NODE_DECLARAION(noise3, float);
        static NODE_DECLARAION(dnoise1, float);
        static NODE_DECLARAION(dnoise2, Vector2f);
        static NODE_DECLARAION(dnoise3, Vector3f);
    };

} // namespace Mu

#endif // __MuLang__MathUtilModule__h__
