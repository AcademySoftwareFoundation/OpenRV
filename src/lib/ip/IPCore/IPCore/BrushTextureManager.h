//
// Copyright (C) 2025 Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#pragma once
#include <IPCore/PaintCommand.h>
#include <mutex>
#include <string>
#include <unordered_map>

namespace IPCore
{
    namespace Paint
    {

        /// Resolved properties for one catalogue brush entry.
        struct BrushInfo
        {
            unsigned int textureId = 0; ///< GL texture name; 0 = procedural
            PolyLine::StampBlendMode blendMode = PolyLine::BlendNormal;
            bool softShader = false; ///< use soft Gaussian shader
            bool isStamp = false;    ///< true when the catalogue entry has a tip texture
            std::string tipFile;     ///< tip PNG filename relative to catalogue dir (empty = procedural)
        };

        /// Singleton that loads the brush catalogue JSON and uploads tip textures to GL.
        ///
        /// Usage:
        ///   // On the GL thread, once per application lifetime:
        ///   BrushTextureManager::instance().load();
        ///
        ///   // At stroke-creation time (any thread):
        ///   BrushInfo info = BrushTextureManager::instance().get(brushName);
        ///
        class BrushTextureManager
        {
        public:
            static BrushTextureManager& instance();

            /// Upload tip PNGs as GL textures for any catalogue entries that have them.
            /// Must be called on the active GL thread. Safe to call multiple times —
            /// only the first call has any effect.
            /// Catalogue metadata (isStamp, blendMode, softShader) is parsed lazily
            /// on the first call to get(), so this need not be called first.
            void load();

            /// Return resolved info for @p name, or a default BrushInfo if not found.
            /// Parses catalogue.json on the first call (any thread; no GL required).
            BrushInfo get(const std::string& name);

            /// Release all GL textures. Call before GL context teardown.
            void clear();

            bool isLoaded() const { return m_texturesLoaded; }

            /// Resolve the brush catalogue directory: RV_BRUSH_DIR env var, or the
            /// assets/brushes directory inside the application bundle.
            static std::string catalogueDir();

        private:
            BrushTextureManager() = default;

            ~BrushTextureManager() { clear(); }

            /// Parse catalogue.json and populate m_brushes metadata (no GL).
            /// Thread-safe via m_parseOnce; idempotent after first call.
            void parseCatalogue();

            std::unordered_map<std::string, BrushInfo> m_brushes;
            std::once_flag m_parseOnce;
            bool m_texturesLoaded{false};
        };

    } // namespace Paint
} // namespace IPCore
