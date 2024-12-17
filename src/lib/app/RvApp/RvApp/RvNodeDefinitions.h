//
//  Copyright (c) 2013 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __RvApp__RvNodeDefinitions__h__
#define __RvApp__RvNodeDefinitions__h__
#include <iostream>
#include <IPCore/NodeManager.h>

namespace Rv
{

    void addRvNodeDefinitions(IPCore::NodeManager*);

    //
    //  Scale on all nodes
    //

    void setScaleOnAll(const IPCore::IPGraph& graph, float scale);

    //
    //  Log->Lin on all nodes
    //

    void setLogLinOnAll(const IPCore::IPGraph& graph, bool, int);

    //
    //  sRGB->Lin on all nodes
    //

    void setSRGBLinOnAll(const IPCore::IPGraph& graph, bool);
    //

    //  709->Lin on all nodes
    //

    void setRec709LinOnAll(const IPCore::IPGraph& graph, bool);

    //
    //  Gamma (const IPCore::IPGraph& graph, interactive) on all nodes
    //

    void setGammaOnAll(const IPCore::IPGraph& graph, float gamma);

    //
    //  Gamma (const IPCore::IPGraph& graph, file) on all nodes
    //

    void setFileGammaOnAll(const IPCore::IPGraph& graph, float gamma);

    //
    //  Exposure on all nodes
    //

    void setFileExposureOnAll(const IPCore::IPGraph& graph, float exposure);

    //
    //  Orientation on all nodes
    //

    void setFlipFlopOnAll(const IPCore::IPGraph& graph, bool setFlip, bool flip,
                          bool setFlop, bool flop);

    //
    //
    //

    void setChannelMapOnAll(const IPCore::IPGraph& graph,
                            const std::vector<std::string>& ch);

    //
    //  sets FormatIPNode's xfit and yfit on all inputs
    //

    void fitAllInputs(const IPCore::IPGraph& graph, int w, int h);

    //
    //  sets FormatIPNode's xresize and yresize on all inputs
    //

    void resizeAllInputs(const IPCore::IPGraph& graph, int w, int h);

} // namespace Rv

#endif // __RvApp__RvNodeDefinitions__h__
