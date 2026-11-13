//
// Copyright (C) 2026 Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <IPCore/BrushCatalogue.h>
#include <TwkApp/Bundle.h>
#include <TwkUtil/File.h>
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
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BrushDefaults, spacingBias, spacingJitter, opacityJitter, radiusJitter, rotationJitter,
                                                    rotateToStroke, angle, squish, hardness, flow)

    void from_json(const nlohmann::json& json, BrushEntry& entry) // NOLINT(misc-use-internal-linkage)
    {
        json.at("name").get_to(entry.name);
        json.at("tip").get_to(entry.tip);
        entry.params = json.value("params", BrushDefaults{});

        if (entry.name.empty() || entry.tip.empty())
        {
            throw std::invalid_argument("'name' and 'tip' cannot be empty");
        }
    }

    namespace
    {
        std::string brushAssetsDirectory()
        {
            const std::string applicationRoot = TwkApp::Bundle::mainBundle()->top();

#if defined(PLATFORM_DARWIN)
            // Since DarwinBundle::top() is RV.app/, we need to keep brush assets under Contents/Resources for codesign.
            return applicationRoot + "/Contents/Resources/assets/brushes";
#else
            return applicationRoot + "/assets/brushes";
#endif
        }

        BrushCatalogue parseBrushCatalogue(const std::string& directory)
        {
            const std::string path = directory + "/catalogue.json";
            std::ifstream file(UNICODE_C_STR(path.c_str()));

            if (!file.is_open())
            {
                throw std::runtime_error("Cannot open " + path);
            }

            nlohmann::json catalogue = nlohmann::json::parse(file);
            const int version = catalogue.at("version").get<int>();
            constexpr int kCatalogueVersion = 2;

            if (version != kCatalogueVersion)
            {
                throw std::runtime_error("Unsupported catalogue version " + std::to_string(version) + " in " + path);
            }

            std::vector<BrushEntry> entries;
            const nlohmann::json& brushes = catalogue.at("brushes");

            for (const auto& brush : brushes)
            {
                try
                {
                    entries.push_back(brush.get<BrushEntry>());
                }
                catch (const std::exception& exception)
                {
                    std::cerr << "WARNING: Skipping a brush catalogue entry." << exception.what() << "\n";
                }
            }

            return {directory, std::move(entries)};
        }
    } // namespace

    BrushCatalogue::BrushCatalogue(std::string directory, std::vector<BrushEntry> entries)
        : m_directory(std::move(directory))
        , m_entries(std::move(entries))
    {
    }

    const BrushCatalogue& bundledCatalogue()
    {
        static const BrushCatalogue catalogue = []()
        {
            const std::string directory = brushAssetsDirectory();

            try
            {
                return parseBrushCatalogue(directory);
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

    std::string BrushCatalogue::tipPath(const std::string& tipFile) const { return m_directory + "/" + tipFile; }
} // namespace IPCore::Paint
