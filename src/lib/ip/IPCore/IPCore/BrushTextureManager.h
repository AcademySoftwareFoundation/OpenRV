//
// Copyright (C) 2025 Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#pragma once
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace IPCore
{
    namespace Paint
    {

        /// Caches GL textures for brush tip PNGs, keyed by filename.
        ///
        /// Tip files live in a flat directory (RV_BRUSH_DIR env var, or
        /// assets/brushes inside the application bundle) and are uploaded to GL
        /// lazily on first use. Per-stroke brush behavior (hardness, blend mode,
        /// which tip to use) comes from the stroke's own properties, set by the
        /// UI — this class only resolves a tip filename to a GL texture.
        ///
        /// Usage:
        ///   // On the GL thread, at render time:
        ///   unsigned int texId = BrushTextureManager::instance().getTexture(tipFile);
        ///
        ///   // For a UI tip picker, any thread, no GL required:
        ///   std::vector<std::string> tips = BrushTextureManager::instance().listTipFiles();
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

            /// List available tip PNG filenames in the brush tip directory, for
            /// UI pickers. No GL required; safe to call from any thread.
            std::vector<std::string> listTipFiles() const;

            /// Release all GL textures. Call before GL context teardown.
            void clear();

            /// Resolve the brush tip directory: RV_BRUSH_DIR env var, or the
            /// assets/brushes directory inside the application bundle
            /// (Contents/Resources on macOS).
            static std::string tipDir();

        private:
            BrushTextureManager() = default;

            ~BrushTextureManager() { clear(); }

            std::mutex m_mutex;
            std::unordered_map<std::string, unsigned int> m_textures;
        };

    } // namespace Paint
} // namespace IPCore
