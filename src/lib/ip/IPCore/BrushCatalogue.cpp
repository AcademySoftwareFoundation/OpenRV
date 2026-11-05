//
// Copyright (C) 2026 Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <IPCore/BrushCatalogue.h>
#include <TwkApp/Bundle.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace IPCore::Paint
{
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(BrushDefaults, spacingBias, spacingJitter, opacityJitter, radiusJitter, rotationJitter,
                                       rotateToStroke, angle, squish, hardness, flow)

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(BrushEntry, name, tip, params)

    namespace
    {
        std::filesystem::path brushAssetsDirectory()
        {
            const std::filesystem::path applicationRoot = TwkApp::Bundle::mainBundle()->top();

#if defined(PLATFORM_DARWIN)
            // Since DarwinBundle::top() is RV.app/, we need to keep brush assets under Contents/Resources for codesign.
            return applicationRoot / "Contents" / "Resources" / "assets" / "brushes";
#else
            return applicationRoot / "assets" / "brushes";
#endif
        }

        BrushCatalogue parseBrushCatalogue(const std::filesystem::path& path)
        {
            std::ifstream file(path);

            if (!file.is_open())
            {
                throw std::runtime_error("Cannot open " + path.string());
            }

            nlohmann::json catalogue = nlohmann::json::parse(file);
            const int version = catalogue.at("version").get<int>();
            constexpr int kCatalogueVersion = 2;

            if (version != kCatalogueVersion)
            {
                throw std::runtime_error("Unsupported catalogue version " + std::to_string(version) + " in " + path.string());
            }

            return {path.parent_path(), catalogue.at("brushes").get<std::vector<BrushEntry>>()};
        }
    } // namespace

    BrushCatalogue::BrushCatalogue(std::filesystem::path directory, std::vector<BrushEntry> entries)
        : m_directory(std::move(directory))
        , m_entries(std::move(entries))
    {
    }

    const BrushCatalogue& bundledCatalogue()
    {
        static const BrushCatalogue catalogue = []()
        {
            const std::filesystem::path directory = brushAssetsDirectory();

            try
            {
                return parseBrushCatalogue(directory / "catalogue.json");
            }
            catch (const std::exception& exception)
            {
                std::cerr << "WARNING: The brush catalogue could not be loaded. " << exception.what() << "\n";
                return BrushCatalogue{directory, {}};
            }
        }();

        return catalogue;
    }

    std::optional<BrushEntry> BrushCatalogue::brushEntry(std::string_view brushName) const
    {
        for (const auto& entry : m_entries)
        {
            if (entry.name == brushName)
            {
                return entry;
            }
        }

        return std::nullopt;
    }

    std::filesystem::path BrushCatalogue::tipPath(std::string_view tipFile) const
    {
        return m_directory / std::filesystem::path(tipFile).filename();
    }
} // namespace IPCore::Paint
