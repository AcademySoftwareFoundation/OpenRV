//
// Copyright (C) 2025 Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <IPCore/BrushTextureManager.h>
#include <TwkApp/Bundle.h>
#include <TwkGLF/GL.h>
#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QStringList>
#include <png.h>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <vector>

namespace IPCore
{
    namespace Paint
    {

        // ── helpers ───────────────────────────────────────────────────────────────────

        /// Load an 8-bit grayscale PNG and upload it as a GL_LUMINANCE texture.
        /// Returns the GL texture name, or 0 on failure.
        static unsigned int uploadGrayscalePng(const std::string& path)
        {
            FILE* fp = std::fopen(path.c_str(), "rb");
            if (!fp)
            {
                std::cerr << "[BrushTextureManager] cannot open: " << path << "\n";
                return 0;
            }

            png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
            if (!png)
            {
                std::fclose(fp);
                return 0;
            }

            png_infop info = png_create_info_struct(png);
            if (!info)
            {
                png_destroy_read_struct(&png, nullptr, nullptr);
                std::fclose(fp);
                return 0;
            }

            if (setjmp(png_jmpbuf(png)))
            {
                std::cerr << "[BrushTextureManager] libpng error reading: " << path << "\n";
                png_destroy_read_struct(&png, &info, nullptr);
                std::fclose(fp);
                return 0;
            }

            png_init_io(png, fp);
            png_read_info(png, info);

            // Normalise to 8-bit grayscale regardless of source format
            png_set_strip_16(png);
            png_set_expand(png);
            png_set_rgb_to_gray_fixed(png, 1, -1, -1);
            png_set_strip_alpha(png);
            png_read_update_info(png, info);

            const int w = static_cast<int>(png_get_image_width(png, info));
            const int h = static_cast<int>(png_get_image_height(png, info));

            std::vector<std::vector<png_byte>> rows(h, std::vector<png_byte>(w));
            std::vector<png_bytep> rowPtrs(h);
            for (int y = 0; y < h; ++y)
                rowPtrs[y] = rows[y].data();
            png_read_image(png, rowPtrs.data());

            png_destroy_read_struct(&png, &info, nullptr);
            std::fclose(fp);

            // Flatten row-per-row data into a contiguous buffer
            std::vector<unsigned char> pixels(w * h);
            for (int y = 0; y < h; ++y)
                std::memcpy(pixels.data() + y * w, rows[y].data(), w);

            GLuint texId = 0;
            glGenTextures(1, &texId);
            glBindTexture(GL_TEXTURE_2D, texId);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, w, h, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, pixels.data());
            glBindTexture(GL_TEXTURE_2D, 0);

            return static_cast<unsigned int>(texId);
        }

        // ── BrushTextureManager ───────────────────────────────────────────────────────

        BrushTextureManager& BrushTextureManager::instance()
        {
            static BrushTextureManager s;
            return s;
        }

        std::string BrushTextureManager::tipDir()
        {
            if (const char* env = std::getenv("RV_BRUSH_DIR"))
                return env;
            if (TwkApp::Bundle* b = TwkApp::Bundle::mainBundle())
            {
#if defined(PLATFORM_DARWIN)
                // DarwinBundle::top() is RV.app/; keep brush assets under Contents/Resources for codesign.
                return b->top() + "/Contents/Resources/assets/brushes";
#else
                return b->top() + "/assets/brushes";
#endif
            }
            return "";
        }

        unsigned int BrushTextureManager::getTexture(const std::string& tipFile)
        {
            if (tipFile.empty())
                return 0;

            std::lock_guard<std::mutex> lock(m_mutex);
            const auto it = m_textures.find(tipFile);
            if (it != m_textures.end())
                return it->second;

            const QString path = QString::fromStdString(tipDir()) + "/" + QString::fromStdString(tipFile);
            const unsigned int texId = uploadGrayscalePng(path.toStdString());
            if (!texId)
                std::cerr << "[BrushTextureManager] failed to load tip: " << path.toStdString() << "\n";

            m_textures[tipFile] = texId; // cache failures too, so a bad path isn't retried every frame
            return texId;
        }

        std::vector<std::string> BrushTextureManager::listTipFiles() const
        {
            std::vector<std::string> files;
            const QDir dir(QString::fromStdString(tipDir()));
            for (const QFileInfo& fi : dir.entryInfoList(QStringList() << "*.png", QDir::Files, QDir::Name))
                files.push_back(fi.fileName().toStdString());
            return files;
        }

        void BrushTextureManager::clear()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            for (auto& kv : m_textures)
            {
                if (kv.second)
                {
                    GLuint texId = kv.second;
                    glDeleteTextures(1, &texId);
                }
            }
            m_textures.clear();
        }

    } // namespace Paint
} // namespace IPCore
