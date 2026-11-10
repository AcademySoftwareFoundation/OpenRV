//
// Copyright (C) 2026 Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace IPCore::Paint
{
    struct BrushDefaults
    {
        float spacingBias = 1.0f;
        float spacingJitter = 0.0f;
        float opacityJitter = 0.0f;
        float radiusJitter = 0.0f;
        float rotationJitter = 0.0f;
        bool rotateToStroke = false;
        float angle = 0.0f;
        float squish = 1.0f;
        float hardness = 1.0f;
        float flow = 1.0f;
    };

    struct BrushEntry
    {
        std::string name;
        std::string tip;
        BrushDefaults params;
    };

    class BrushCatalogue
    {
    public:
        BrushCatalogue(std::string directory, std::vector<BrushEntry> entries);
        [[nodiscard]] std::optional<BrushEntry> brushEntry(std::string_view brushName) const;

        [[nodiscard]] const std::vector<BrushEntry>& entries() const { return m_entries; }

        [[nodiscard]] std::string tipPath(const std::string& tipFile) const;

    private:
        std::string m_directory;
        std::vector<BrushEntry> m_entries;
    };

    const BrushCatalogue& bundledCatalogue();

} // namespace IPCore::Paint
