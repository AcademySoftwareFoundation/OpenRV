//
//  Copyright (c) 2008 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __RvCommon__MediaDirModel__h__
#define __RvCommon__MediaDirModel__h__
#include <iostream>
#include <QtCore/QtCore>

namespace Rv
{
    struct MediaDirItem;
    class FileTypeTraits;

    /// An item model which handles a directory tree (like QDirModel) for media
    /// files

    ///
    /// MediaDirModel acts similarily to QDirModel except that it will also
    /// handle sequences as pseudo directories and show media file information
    /// in columns like rvls -l
    ///

    class MediaDirModel : public QAbstractItemModel
    {
        Q_OBJECT

    public:
        //
        //  Constructors
        //

        enum Details
        {
            NoDetails,
            BasicFileDetails,
            MediaDetails
        };

        MediaDirModel(const QDir& dir,
                      FileTypeTraits* traits, // becomes owned by MediaDirModel
                      Details, QObject* parent = 0);

        MediaDirModel(FileTypeTraits* traits, // becomes owned by MediaDirModel
                      QObject* parent = 0);

        virtual ~MediaDirModel();

        void setDirectory(const QDir& dir, Details);

        FileTypeTraits* fileTraits() const { return m_fileTraits; }

        void setFileTraitsIndex(size_t);
        void setFileTraits(FileTypeTraits*);

        void setShowHiddenFiles(bool);

        //
        //  Minimum API
        //

        virtual int
        columnCount(const QModelIndex& parent = QModelIndex()) const;
        virtual QVariant data(const QModelIndex& index,
                              int role = Qt::DisplayRole) const;
        virtual QModelIndex
        index(int row, int column,
              const QModelIndex& parent = QModelIndex()) const;
        virtual QModelIndex parent(const QModelIndex& index) const;
        virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
        virtual QVariant headerData(int section, Qt::Orientation orientation,
                                    int role = Qt::DisplayRole) const;

        virtual bool
        hasChildren(const QModelIndex& index = QModelIndex()) const;

        virtual Qt::ItemFlags flags(const QModelIndex& index) const;
        virtual QMap<int, QVariant> itemData(const QModelIndex& index) const;

        void setSortFlags(QDir::SortFlags);
        QString absoluteFilePath(const QModelIndex&) const;
        QModelIndex indexOfPath(const QFileInfo&);
        void reload();

    private:
        QDir::SortFlags m_sortFlags;
        MediaDirItem* m_root;
        Details m_detail;
        bool m_hidden;
        FileTypeTraits* m_fileTraits;
        size_t m_fileTraitsIndex;
    };

} // namespace Rv

#endif // __RvCommon__MediaDirModel__h__
