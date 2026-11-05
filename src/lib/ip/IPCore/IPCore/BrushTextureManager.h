//
// Copyright (C) 2025 Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#pragma once
#include <mutex>
#include <string>
#include <unordered_map>

namespace IPCore
{
    namespace Paint
    {

        /// Caches GL textures for brush tip PNGs, keyed by filename.
        ///
        /// Which tip a stroke uses, and its default brush values, come from the BrushCatalogue. This class only resolves a tip filename to
        /// a GL texture.
        ///
        /// Usage:
        ///   // On the GL thread, at render time:
        ///   unsigned int texId = BrushTextureManager::instance().getTexture(tipFile);
        ///
        class BrushTextureManager
        {
        public:
            static BrushTextureManager& instance();

            /// Return the GL texture name for @p tipFile, uploading it from disk
            /// on first request and caching the result (including failures, as 0).
            /// Must be called on the active GL thread. Returns 0 if tipFile is
            /// empty or the PNG can't be loaded.
            unsigned int getTexture(const std::string& tipFile);

            /// Release all GL textures. Call before GL context teardown.
            void clear();

        private:
            BrushTextureManager() = default;

            ~BrushTextureManager() { clear(); }

            std::mutex m_mutex;
            std::unordered_map<std::string, unsigned int> m_textures;
        };

    } // namespace Paint
} // namespace IPCore
