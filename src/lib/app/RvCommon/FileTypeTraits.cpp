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

        list << "Created"
             << info.created().toString(Qt::DefaultLocaleShortDate);
        list << "Modified"
             << info.lastModified().toString(Qt::DefaultLocaleShortDate);

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
