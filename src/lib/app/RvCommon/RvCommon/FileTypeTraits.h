//
//  Copyright (c) 2008 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __RvCommon__FileTypeTraits__h__
#define __RvCommon__FileTypeTraits__h__
#include <iostream>
#include <QtCore/QtCore>
#include <QtGui/QtGui>
#include <QtWidgets/QFileIconProvider>

namespace Rv
{

    /// Used by RvFileDialog and MediaDirModel to inspect files, get icons

    ///
    /// FileTypeTraits principally takes the place of the usual list of
    /// regular expressions typically passed to a file dialog to express
    /// the accepted file types. In addition, it supplies icons, preview
    /// images, and file attribtues in a content aware manner. For
    /// example, known image file headers are read and their contents
    /// displayed in the preview.
    ///

    class FileTypeTraits
    {
    public:
        FileTypeTraits();
        virtual ~FileTypeTraits();

        ///
        /// Make a new FileTypeTraits object of the same type as the one
        /// being called.
        ///

        virtual FileTypeTraits* copyTraits() const;

        ///
        /// Return a human readable list of file types
        ///

        virtual QStringList typeDescriptions();

        ///
        /// Return true if the passed in filename is "known" by the type
        /// description corresponding to the index (which is relative to
        /// the list returned by typeDescriptions).
        ///

        virtual bool isKnown(size_t index, const QString&) const;

        ///
        /// Return attributes of file. The string list alternates between
        /// the attribute name and value (as a string). So its length
        /// should be even.
        ///

        virtual QStringList fileAttributes(const QString&) const;

        ///
        /// Return a suitable file icon for the passed in filename or file
        /// info. If you know the type of the file, pass it in.
        ///

        QIcon fileIcon(const QString&, QFileIconProvider::IconType knownType =
                                           QFileIconProvider::Trashcan) const;
        virtual QIcon fileInfoIcon(const QFileInfo&,
                                   QFileIconProvider::IconType knownType =
                                       QFileIconProvider::Trashcan) const;

        ///
        /// If a preview image is available for the file indicate that and
        /// possible produce it.
        ///

        virtual bool hasImage(const QString&) const;
        virtual QImage fileImage(const QString&) const;

        ///
        /// Icon Mode control whether QIconProvider is used or
        /// not. Looking up icons can be very time consuming on some
        /// platforms.
        ///

        enum IconMode
        {
            SystemIcons,
            GenericIcons,
            NoIcons
        };

        void setIconMode(IconMode);
        IconMode iconMode() const;

        ///
        /// Returns true if the traits are the sames, false otherwise
        /// Optimization - Optional
        /// Note: This is exclusively used for optimization purposes and is
        /// therefore optional. It is perfectly valid to return false all the
        /// time. In which case the new traits will be considered different from
        /// the current ones.
        ///
        virtual bool sameAs(const FileTypeTraits*) const { return false; }

    private:
        IconMode m_iconMode;
    };

} // namespace Rv

#endif // __RvCommon__FileTypeTraits__h__
