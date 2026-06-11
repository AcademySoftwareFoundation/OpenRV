//
// Copyright (C) 2025 Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <IPCore/BrushTextureManager.h>
#include <TwkApp/Bundle.h>
#include <TwkGLF/GL.h>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <png.h>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <mutex>
#include <vector>

namespace IPCore
{
    namespace Paint
    {

        // ── helpers ───────────────────────────────────────────────────────────────────

        static PolyLine::StampBlendMode blendFromString(const QString& s)
        {
            if (s == "additive")
                return PolyLine::BlendAdditive;
            if (s == "marker")
                return PolyLine::BlendMarker;
            if (s == "eraser")
                return PolyLine::BlendEraser;
            return PolyLine::BlendNormal;
        }

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

        std::string BrushTextureManager::catalogueDir()
        {
            if (const char* env = std::getenv("RV_BRUSH_DIR"))
                return env;
            if (TwkApp::Bundle* b = TwkApp::Bundle::mainBundle())
                return b->top() + "/assets/brushes";
            return "";
        }

        void BrushTextureManager::parseCatalogue()
        {
            std::call_once(m_parseOnce,
                           [this]()
                           {
                               const QString qdir = QString::fromStdString(catalogueDir());
                               const QString catPath = qdir + "/catalogue.json";

                               QFile f(catPath);
                               if (!f.open(QIODevice::ReadOnly))
                               {
                                   std::cerr << "[BrushTextureManager] catalogue not found: " << catPath.toStdString() << "\n";
                                   return;
                               }

                               QJsonParseError err;
                               const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
                               if (doc.isNull())
                               {
                                   std::cerr << "[BrushTextureManager] JSON parse error: " << err.errorString().toStdString() << "\n";
                                   return;
                               }

                               for (const QJsonValue& v : doc.object().value("brushes").toArray())
                               {
                                   const QJsonObject entry = v.toObject();
                                   const std::string name = entry.value("name").toString().toStdString();
                                   const QString tip = entry.value("tip").toString();
                                   const QString blend = entry.value("blend").toString("normal");
                                   const bool soft = entry.value("soft").toBool(false);

                                   BrushInfo& info = m_brushes[name];
                                   info.blendMode = blendFromString(blend);
                                   info.softShader = soft;
                                   info.isStamp = !tip.isEmpty();
                                   info.tipFile = tip.toStdString();
                               }
                           }); // call_once
        }

        void BrushTextureManager::load()
        {
            if (m_texturesLoaded)
                return;
            m_texturesLoaded = true;

            parseCatalogue();

            const QString qdir = QString::fromStdString(catalogueDir());
            for (auto& kv : m_brushes)
            {
                if (kv.second.isStamp && !kv.second.tipFile.empty() && !kv.second.textureId)
                {
                    const std::string tipPath = (qdir + "/" + QString::fromStdString(kv.second.tipFile)).toStdString();
                    kv.second.textureId = uploadGrayscalePng(tipPath);
                    if (!kv.second.textureId)
                        std::cerr << "[BrushTextureManager] failed to load tip: " << tipPath << "\n";
                }
            }
        }

        BrushInfo BrushTextureManager::get(const std::string& name)
        {
            parseCatalogue();
            const auto it = m_brushes.find(name);
            return (it != m_brushes.end()) ? it->second : BrushInfo{};
        }

        void BrushTextureManager::clear()
        {
            for (auto& kv : m_brushes)
            {
                if (kv.second.textureId)
                    glDeleteTextures(1, &kv.second.textureId);
                kv.second.textureId = 0;
            }
            m_texturesLoaded = false;
        }

    } // namespace Paint
} // namespace IPCore
