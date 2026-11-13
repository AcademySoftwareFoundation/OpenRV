//
// Copyright (C) 2026 Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <IPCore/BrushTextureManager.h>
#include <IPCore/BrushCatalogue.h>
#include <TwkGLF/GL.h>
#include <TwkUtil/File.h>
#include <png.h>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <utility>
#include <vector>

namespace IPCore::Paint
{

    namespace
    {

        struct BrushTipImage
        {
            int width;
            int height;
            std::vector<png_byte> pixels;
        };

        std::vector<png_byte> readFileBytes(const std::string& path)
        {
            std::ifstream file(UNICODE_C_STR(path.c_str()), std::ios::binary);
            return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
        }

        std::optional<BrushTipImage> decodeBrushTipPng(const std::string& path)
        {
            const std::vector<png_byte> fileBytes = readFileBytes(path);
            if (fileBytes.empty())
            {
                std::cerr << "[BrushTextureManager] cannot read: " << path << "\n";
                return std::nullopt;
            }

            png_image pngImage{};
            pngImage.version = PNG_IMAGE_VERSION;

            if (png_image_begin_read_from_memory(&pngImage, fileBytes.data(), fileBytes.size()) == 0)
            {
                std::cerr << "[BrushTextureManager] libpng error reading: " << path << ": " << std::data(pngImage.message) << "\n";
                png_image_free(&pngImage);
                return std::nullopt;
            }

            pngImage.format = PNG_FORMAT_RGBA;
            std::vector<png_byte> rgba(PNG_IMAGE_SIZE(pngImage));
            if (png_image_finish_read(&pngImage, nullptr, rgba.data(), 0, nullptr) == 0)
            {
                std::cerr << "[BrushTextureManager] libpng error reading: " << path << ": " << std::data(pngImage.message) << "\n";
                return std::nullopt;
            }

            constexpr size_t kRgbaChannelCount = 4;
            std::vector<png_byte> red(rgba.size() / kRgbaChannelCount);
            for (size_t pixel = 0; pixel < red.size(); ++pixel)
            {
                red[pixel] = rgba[pixel * kRgbaChannelCount];
            }

            return BrushTipImage{static_cast<int>(pngImage.width), static_cast<int>(pngImage.height), std::move(red)};
        }

        unsigned int uploadLuminanceTexture(const BrushTipImage& image)
        {
            GLuint textureId = 0;
            glGenTextures(1, &textureId);
            glBindTexture(GL_TEXTURE_2D, textureId);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, image.width, image.height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, image.pixels.data());
            glBindTexture(GL_TEXTURE_2D, 0);

            return static_cast<unsigned int>(textureId);
        }

        std::optional<unsigned int> uploadBrushTipPng(const std::string& path)
        {
            const std::optional<BrushTipImage> image = decodeBrushTipPng(path);
            if (!image.has_value())
            {
                return std::nullopt;
            }

            return uploadLuminanceTexture(*image);
        }

    } // namespace

    BrushTextureManager& BrushTextureManager::instance()
    {
        static BrushTextureManager singleton;
        return singleton;
    }

    unsigned int BrushTextureManager::getTexture(const std::string& tipFile)
    {
        if (tipFile.empty())
        {
            return 0;
        }

        std::scoped_lock lock(m_mutex);
        const auto cached = m_textures.find(tipFile);
        if (cached != m_textures.end())
        {
            return cached->second;
        }

        const std::string path = bundledCatalogue().tipPath(tipFile);
        const std::optional<unsigned int> uploadedPng = uploadBrushTipPng(path);
        if (!uploadedPng.has_value())
        {
            std::cerr << "[BrushTextureManager] failed to load tip: " << path << "\n";
        }

        const unsigned int textureId = uploadedPng.value_or(0);
        m_textures[tipFile] = textureId; // cache failures too, so a bad path isn't retried every frame
        return textureId;
    }

    void BrushTextureManager::clear()
    {
        std::scoped_lock lock(m_mutex);
        for (const auto& [tipFile, cachedTextureId] : m_textures)
        {
            if (cachedTextureId != 0)
            {
                GLuint textureId = cachedTextureId;
                glDeleteTextures(1, &textureId);
            }
        }
        m_textures.clear();
    }

} // namespace IPCore::Paint
