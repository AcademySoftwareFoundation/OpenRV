//******************************************************************************
// Copyright (c) 2026 Autodesk Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef __IP__Core__MediaCache__h__
#define __IP__Core__MediaCache__h__

#include <cstddef>
#include <map>
#include <string>
#include <cstdint>

namespace IPCore
{
    class IPGraph;

    enum class CacheMode: std::uint8_t {
        UNBOUNDED,
        BOUNDED
    };

    using MediaMap = std::map<std::string, std::string>;

    class MediaCache
    {
        public:

        MediaCache(IPGraph* graph);
        virtual ~MediaCache();

        bool add(std::string fileUrl);

        bool isFrameCached(int frame);

        private:

        // Where we're currently displaying
        int m_displayFrame;
        IPGraph* m_graph;

        size_t m_maxBytes;
        size_t m_currentBytes;
        CacheMode m_mode;
        // Used as a map between IDs and file paths
        MediaMap m_map;
    };
}


#endif