//******************************************************************************
// Copyright (c) 2026 Autodesk Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <IPCore/MediaCache.h>
#include <IPCore/IPGraph.h>
#include <algorithm>
#include <cstddef>
#include <limits>

namespace IPCore
{
    MediaCache::MediaCache(IPGraph* graph):
        m_graph{graph},
        m_mode{CacheMode::UNBOUNDED},
        m_maxBytes(std::numeric_limits<size_t>::max()),
        m_currentBytes{0},
        m_displayFrame(std::numeric_limits<int>::min())
    {
    }

    MediaCache::~MediaCache() = default;

}
