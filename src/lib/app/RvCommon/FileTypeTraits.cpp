//
//  Copyright (c) 2008 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <RvCommon/FileTypeTraits.h>

namespace Rv
{
    using namespace std;

    FileTypeTraits::FileTypeTraits()
        : m_iconMode(SystemIcons)
    {
    }

    FileTypeTraits::~FileTypeTraits() {}

    FileTypeTraits* FileTypeTraits::copyTraits() const
    {
        FileTypeTraits* t = new FileTypeTraits();
        t->setIconMode(m_iconMode);
        return t;
    }

    QStringList FileTypeTraits::typeDescriptions()
    {
        QStringList list;
        list.append("Any File");
        return list;
    }

    bool FileTypeTraits::isKnown(size_t index, const QString&) const
    {
        return true;
    }

    QStringList FileTypeTraits::fileAttributes(const QString& file) const
    {
        QStringList list;
        QFileInfo info(file);

#if defined(RV_VFX_CY2023)
        list << "Created"
             << info.birthTime().toString(Qt::DefaultLocaleShortDate);
        list << "Modified"
             << info.lastModified().toString(Qt::DefaultLocaleShortDate);
#else
        // created() is deprecated.
        // birthTime() function to get the time the file was created.
        // metadataChangeTime() to get the time its metadata was last changed.
        // lastModified() to get the time it was last modified.
        QLocale locale = QLocale::system();
        list << "Created"
             << locale.toString(info.birthTime(), QLocale::ShortFormat);
        list << "Modified"
             << locale.toString(info.lastModified(), QLocale::ShortFormat);
#endif

        size_t s = info.size();

        if (s < 1024)
        {
            list << "Size" << QString("%1 bytes").arg(info.size());
        }
        else if (s < 1024 * 1024)
        {
            list << "Size" << QString("%1 Kb").arg(info.size() / 1024);
        }
        else if (s < 1024 * 1024 * 1024)
        {
            list << "Size" << QString("%1 Mb").arg(info.size() / 1024 / 1024);
        }
        else if (s < size_t(1024) * size_t(1024) * size_t(1024) * size_t(1024))
        {
            list << "Size"
                 << QString("%1 Gb").arg(info.size() / 1024 / 1024 / 1024);
        }

        return list;
    }

    QIcon FileTypeTraits::fileIcon(const QString& file,
                                   QFileIconProvider::IconType knownType) const
    {
        if (m_iconMode == NoIcons)
            return QIcon();

        QFileInfo info(file);
        return fileInfoIcon(info, knownType);
    }

    QIcon
    FileTypeTraits::fileInfoIcon(const QFileInfo& info,
                                 QFileIconProvider::IconType knownType) const
    {
        static QFileIconProvider provider;

        if (m_iconMode == NoIcons)
            return QIcon();

        if (QFileIconProvider::Trashcan != knownType)
            return provider.icon(knownType);

        if (m_iconMode == GenericIcons)
        {
            if (info.isDir())
            {
                return provider.icon(QFileIconProvider::Folder);
            }
            else
            {
                return provider.icon(QFileIconProvider::File);
            }
        }

        if (!info.exists())
        {
            return provider.icon(QFileIconProvider::Folder);
        }

    
#ifndef PLATFORM_WINDOWS
    #if defined(RV_VFX_CY2024)
        // Adding more heuristics to find the right icon for a file based on the
        // MIME type. The following heuristics should work for Qt 5, but in
        // order to keep the same behavior as before, this code will only run
        // with Qt 6 and above.

        QMimeDatabase mimeDb;
        QMimeType mime = mimeDb.mimeTypeForFile(info);

        // Search for an icon based on the MIME type. (e.g. for a PNG, it would
        // be image-png)
        QString iconName = mime.iconName();
        QIcon icon = QIcon::fromTheme(iconName);
        if (!icon.isNull())
        {
            return icon;
        }

        // Search for an icon based on the MIME generic type. (e.g. for a PNG,
        // it would be image-x-generic)
        QString genericIconName = mime.genericIconName();
        icon = QIcon::fromTheme(genericIconName);
        if (!icon.isNull())
        {
            return icon;
        }
    #endif
#endif

        return provider.icon(info);
    }

    bool FileTypeTraits::hasImage(const QString&) const { return false; }

    QImage FileTypeTraits::fileImage(const QString&) const { return QImage(); }

    void FileTypeTraits::setIconMode(IconMode mode) { m_iconMode = mode; }

    FileTypeTraits::IconMode FileTypeTraits::iconMode() const
    {
        return m_iconMode;
    }

} // namespace Rv
