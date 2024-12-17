//
//  Copyright (c) 2008 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __RvCommon__MediaFileTypes__h__
#define __RvCommon__MediaFileTypes__h__
#include <iostream>
#include <RvCommon/RvFileDialog.h>
#include <RvCommon/FileTypeTraits.h>

namespace Rv
{

    class MediaFileTypes : public FileTypeTraits
    {
    public:
        typedef FileTypeTraits Parent;

        MediaFileTypes(bool readable, bool writeable,
                       const QString& defaultExt = "");

        MediaFileTypes(bool readable, bool writeable,
                       const QList<QPair<QString, QString>>& files,
                       const QString& defaultExt = "");

        virtual ~MediaFileTypes();

        virtual FileTypeTraits* copyTraits() const;

        size_t defaultIndex() const;

        virtual QStringList typeDescriptions();
        virtual QString extension(int index);
        virtual QString description(int index);
        virtual bool isKnown(size_t index, const QString&) const;
        virtual QStringList fileAttributes(const QString&) const;

        QIcon fileIcon(const QString&) const;
        virtual QIcon fileInfoIcon(const QFileInfo&) const;

        virtual bool hasImage(const QString&) const;
        virtual QImage fileImage(const QString&) const;

        bool sameAs(const FileTypeTraits*) const override;

    protected:
        MediaFileTypes(const MediaFileTypes*);

    private:
        bool m_readable;
        bool m_writeable;
        QSet<QString> m_extSet;
        QStringList m_exts;
        QStringList m_formats;
        QString m_defaultExt;
    };

} // namespace Rv

#endif // __RvCommon__MediaFileTypes__h__
