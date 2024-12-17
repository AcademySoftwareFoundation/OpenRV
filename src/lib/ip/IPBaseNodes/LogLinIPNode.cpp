//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <IPBaseNodes/LogLinIPNode.h>

namespace IPCore
{
    using namespace TwkContainer;
    using namespace TwkMath;

    LogLinIPNode::LogLinIPNode(const std::string& name)
        : IPNode(name)
    {
        init();
    }

    void LogLinIPNode::init()
    {
        setProtocol("loglin");

        IntProperty* ip;
        ip = createProperty<IntProperty>("converter", "type");
        ip->resize(1);
        ip->front() = 0;

        ip = createProperty<IntProperty>("parameters", "referenceBlack");
        ip->resize(1);
        ip->front() = 95;

        ip = createProperty<IntProperty>("parameters", "referenceWhite");
        ip->resize(1);
        ip->front() = 685;

#if 0
    FloatProperty* fp;

    fp = createProperty<FloatProperty>("parameters", "redOffset");
    fp->resize(1);
    fp->front() = 0.0;

    fp = createProperty<FloatProperty>("parameters", "greenOffset");
    fp->resize(1);
    fp->front() = 0.0;

    fp = createProperty<FloatProperty>("parameters", "blueOffset");
    fp->resize(1);
    fp->front() = 0.0;

    fp = createProperty<FloatProperty>("parameters", "displayGamma");
    fp->resize(1);
    fp->front() = 1.7;

    fp = createProperty<FloatProperty>("parameters", "softClip");
    fp->resize(1);
    fp->front() = 0.0;
#endif
    }

    LogLinIPNode::~LogLinIPNode() {}

    void LogLinIPNode::evaluate(IPState& state, int frame)
    {
        IPNode::evaluate(state, frame);
        state.logtype = 0;

        if (IntProperty* active = property<IntProperty>("node", "active"))
        {
            if (IntProperty* type = property<IntProperty>("converter", "type"))
            {
                state.logtype = type->empty() ? 0 : type->front();
            }

            if (IntProperty* white =
                    property<IntProperty>("parameters", "referenceWhite"))
            {
                state.cinRefWhite = white->empty() ? 685 : white->front();
            }

            if (IntProperty* black =
                    property<IntProperty>("parameters", "referenceBlack"))
            {
                state.cinRefBlack = black->empty() ? 95 : black->front();
            }
        }
    }

} // namespace IPCore
