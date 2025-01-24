//
//  Copyright (c) 2008 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifdef PLATFORM_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#endif

// Note: Must be included first to prevent following compile error on Windows:
// fatal error C1189: #error:  gl.h included before glew.h
#include <IPCore/Session.h>

#include <RvCommon/MediaFileTypes.h>

#include <TwkFB/FrameBuffer.h>
#include <TwkMovie/MovieIO.h>
#include <TwkMovie/MovieReader.h>

#include <string>
#include <sstream>

namespace
{
    void addEventExts(const char* event, IPCore::Session* session,
                      QStringList& exts, QStringList& formats,
                      QSet<QString> extSet)
    {
        static constexpr auto EVENT_SEP = ':';
        static constexpr auto EXT_SEP = ';';

        std::istringstream filters(session->userGenericEvent(event, ""));
        std::string next;

        while (std::getline(filters, next, EVENT_SEP))
        {
            auto pos = next.find(EXT_SEP);
            if (pos != std::string::npos)
            {
                const auto subLen = pos + 1, nextLen = next.length();
                if (subLen < nextLen && next[subLen] != EXT_SEP
                    && next[subLen] != EVENT_SEP)
                {
                    auto ext = next.substr(0, subLen - 1);
                    auto format = next.substr(subLen, nextLen - subLen);

                    if (!ext.empty() && !format.empty())
                    {
                        exts.push_back(ext.c_str());
                        extSet.insert(ext.c_str());
                        formats.push_back(format.c_str());
                    }
                }
            }
        }
    }
} // namespace

namespace Rv
{
    using namespace TwkMovie;
    using namespace TwkFB;
    using namespace std;

    MediaFileTypes::MediaFileTypes(bool readable, bool writeable,
                                   const QString& defaultExt)
        : FileTypeTraits()
        , m_readable(readable)
        , m_writeable(writeable)
        , m_defaultExt(defaultExt)
    {
        m_formats.push_back("Any Media File");
        m_exts.push_back("&");

        m_formats.push_back("Any File");
        m_exts.push_back("*");

        m_formats.push_back("RV Session file");
        m_exts.push_back("rv");
        m_extSet.insert("rv");

        if (auto session = IPCore::Session::currentSession())
        {
            addEventExts("open-file-dialog-filters", session, m_exts, m_formats,
                         m_extSet);
        }

        const TwkMovie::GenericIO::Plugins& plugins =
            TwkMovie::GenericIO::allPlugins();

        for (TwkMovie::GenericIO::Plugins::const_iterator i = plugins.begin();
             i != plugins.end(); ++i)
        {
            const TwkMovie::MovieIO* plugin = *i;
            const MovieIO::MovieTypeInfos& exts = plugin->extensionsSupported();

            for (int q = 0; q < exts.size(); q++)
            {
                const MovieIO::MovieTypeInfo& info = exts[q];

                bool readsaudio = info.capabilities & MovieIO::MovieReadAudio;
                bool readsimages = info.capabilities & MovieIO::MovieRead;
                bool writesaudio = info.capabilities & MovieIO::MovieWriteAudio;
                bool writesimages = info.capabilities & MovieIO::MovieWrite;

                if ((readable && !(readsaudio || readsimages))
                    || (writeable && !(writesaudio || writesimages)))
                {
                    continue;
                }

                if (info.description != "")
                {
                    if (!m_extSet.contains(info.extension.c_str()))
                    {
                        m_extSet.insert(info.extension.c_str());
                        ostringstream str;
                        str << info.description << " (" << info.extension
                            << ")";
                        m_formats.push_back(str.str().c_str());
                        m_exts.push_back(info.extension.c_str());
                    }
                }
            }
        }

        for (TwkMovie::GenericIO::Plugins::const_iterator i = plugins.begin();
             i != plugins.end(); ++i)
        {
            const TwkMovie::MovieIO* plugin = *i;
            const MovieIO::MovieTypeInfos& exts = plugin->extensionsSupported();

            for (int q = 0; q < exts.size(); q++)
            {
                const MovieIO::MovieTypeInfo& info = exts[q];

                if ((readable && !(info.capabilities & MovieIO::MovieRead))
                    || (writeable
                        && !(info.capabilities & MovieIO::MovieWrite)))
                {
                    continue;
                }

                if (info.description == "")
                {
                    if (!m_extSet.contains(info.extension.c_str()))
                    {
                        m_extSet.insert(info.extension.c_str());
                        ostringstream str;
                        str << "Image Format Extension" << " ("
                            << info.extension << ")";
                        m_formats.push_back(str.str().c_str());
                        m_exts.push_back(info.extension.c_str());
                    }
                }
            }
        }
    }

    MediaFileTypes::MediaFileTypes(bool readable, bool writeable,
                                   const QList<QPair<QString, QString>>& files,
                                   const QString& defaultExt)
        : FileTypeTraits()
        , m_readable(readable)
        , m_writeable(writeable)
        , m_defaultExt(defaultExt)
    {
        m_formats.push_back("All Relevant File Types");
        m_exts.push_back("&");

        m_formats.push_back("Any File");
        m_exts.push_back("*");

        if (auto session = IPCore::Session::currentSession())
        {
            addEventExts("save-file-dialog-filters", session, m_exts, m_formats,
                         m_extSet);
        }

        for (size_t i = 0; i < files.size(); i++)
        {
            const QPair<QString, QString>& pair = files[i];

            m_extSet.insert(pair.first);
            m_exts.push_back(pair.first);
            m_formats.push_back(pair.second);
        }
    }

    MediaFileTypes::MediaFileTypes(const MediaFileTypes* other)
    {
        m_extSet = other->m_extSet;
        m_exts = other->m_exts;
        m_formats = other->m_formats;
        m_readable = other->m_readable;
        m_writeable = other->m_writeable;
        m_defaultExt = other->m_defaultExt;
    }

    MediaFileTypes::~MediaFileTypes() {}

    size_t MediaFileTypes::defaultIndex() const
    {
        for (size_t i = 0; i < m_exts.size(); i++)
        {
            if (m_exts[i] == m_defaultExt)
                return i;
        }

        return 0;
    }

    FileTypeTraits* MediaFileTypes::copyTraits() const
    {
        return new MediaFileTypes(this);
    }

    QStringList MediaFileTypes::typeDescriptions() { return m_formats; }

    QString MediaFileTypes::extension(int index) { return m_exts[index]; }

    QString MediaFileTypes::description(int index) { return m_formats[index]; }

    bool MediaFileTypes::isKnown(size_t index, const QString& file) const
    {
        if (index >= m_exts.size())
            return true;
        if (m_exts[index] == "*")
            return true;

        QString ext = file.split('.').last();
        if (m_exts[index] == "&")
        {
            return m_extSet.contains(ext) || m_extSet.contains(ext.toLower())
                   || m_extSet.contains("*");
        }
        else
        {
            return m_exts[index] == ext || m_exts[index] == ext.toLower();
        }
    }

    QStringList MediaFileTypes::fileAttributes(const QString& file) const
    {
        QStringList list = Parent::fileAttributes(file);
        string filename = file.toUtf8().data();

        try
        {
            if (MovieReader* reader =
                    TwkMovie::GenericIO::movieReader(filename, false))
            {
                reader->open(filename);
                const MovieInfo& i = reader->info();

                if (i.video)
                {
                    QString dtype;

                    switch (i.dataType)
                    {
                    case FrameBuffer::UCHAR:
                        dtype = "8 bit";
                        break;
                    case FrameBuffer::USHORT:
                        dtype = "16 bit";
                        break;
                    case FrameBuffer::HALF:
                        dtype = "16 bit float";
                        break;
                    case FrameBuffer::FLOAT:
                        dtype = "32 bit float";
                        break;
                    case FrameBuffer::DOUBLE:
                        dtype = "64 bit float";
                        break;
                    case FrameBuffer::PACKED_R10_G10_B10_X2:
                        dtype = "10 bit packed";
                        break;
                    case FrameBuffer::PACKED_X2_B10_G10_R10:
                        dtype = "10 bit packed";
                        break;
                    case FrameBuffer::PACKED_Y8_Cb8_Y8_Cr8:
                        dtype = "YCbCr packed";
                        break;
                    case FrameBuffer::PACKED_Cb8_Y8_Cr8_Y8:
                        dtype = "YCbCr packed";
                        break;
                    default:
                        dtype = "-";
                    }

                    list << "Geometry"
                         << QString("%1 x %2, %3 ch %4")
                                .arg(i.width)
                                .arg(i.height)
                                .arg(i.numChannels)
                                .arg(dtype);

                    if (i.views.size() > 1)
                    {
                        list << "Views";

                        QString views;
                        for (int q = 0; q < i.views.size(); q++)
                        {
                            if (q)
                                views += ", ";
                            views += i.views[q].c_str();
                        }

                        list << views;
                    }

                    if (i.layers.size() > 1)
                    {
                        list << "Layers";

                        QString layers;
                        for (int q = 0; q < i.layers.size(); q++)
                        {
                            if (q)
                                layers += ", ";
                            layers += i.layers[q].c_str();
                        }

                        list << layers;
                    }

                    if (i.start != i.end)
                    {
                        list << "Frames"
                             << QString("%1 - %2").arg(i.start).arg(i.end);
                        list << "FPS" << QString("%1").arg(i.fps);
                    }
                }

                if (i.audio)
                {
                    list << "Audio Sample Rate"
                         << QString("%1").arg(i.audioSampleRate);
                    list << "Audio Channels"
                         << QString("%1").arg(i.audioChannels.size());
                }

                if (!i.proxy.attributes().empty())
                {
                    const FrameBuffer::AttributeVector& attrs =
                        i.proxy.attributes();

                    for (int q = 0; q < attrs.size(); q++)
                    {
                        list << attrs[q]->name().c_str();
                        list << attrs[q]->valueAsString().c_str();
                    }
                }

                delete reader;
            }
        }
        catch (...)
        {
            list << "<font color=red><u>NOTE</u>:</font>"
                 << "File attributes cannot be read: may be damaged";
        }

        return list;
    }

    QIcon MediaFileTypes::fileIcon(const QString& file) const
    {
        QFileInfo info(file);
        return fileInfoIcon(info);
    }

    QIcon MediaFileTypes::fileInfoIcon(const QFileInfo& info) const
    {
        static QFileIconProvider provider;

#if 0
    if (info.isDir())
    {
        return provider.icon(QFileIconProvider::Folder);
    }
    else 
    {
        return provider.icon(QFileIconProvider::File);
    }
#endif

        return provider.icon(info);
    }

    bool MediaFileTypes::hasImage(const QString&) const { return false; }

    QImage MediaFileTypes::fileImage(const QString&) const { return QImage(); }

    bool MediaFileTypes::sameAs(const FileTypeTraits* traits) const
    {
        const MediaFileTypes* t = dynamic_cast<const MediaFileTypes*>(traits);
        return ((t != nullptr) && (iconMode() == t->iconMode())
                && (m_readable == t->m_readable)
                && (m_writeable == t->m_writeable) && (m_extSet == t->m_extSet)
                && (m_exts == t->m_exts) && (m_formats == t->m_formats)
                && (m_defaultExt == t->m_defaultExt));
    }

} // namespace Rv
