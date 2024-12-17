//
//  Copyright (c) 2008 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <RvCommon/MediaDirModel.h>
#include <RvCommon/FileTypeTraits.h>
#include <TwkMovie/Movie.h>
#include <TwkMovie/MovieReader.h>
#include <TwkMovie/MovieIO.h>
#include <TwkFB/IO.h>
#include <TwkUtil/FrameUtils.h>
#include <TwkUtil/sgcHop.h>
#include <QtWidgets/QFileIconProvider>
#include <TwkQtCoreUtil/QtConvert.h>

#if 0
#define DB_ICON 0x01
#define DB_ALL 0xff

//  #define DB_LEVEL        (DB_ALL & (~ DB_ICON))
#define DB_LEVEL DB_ALL

#define DB(x) cerr << x << endl;
#define DBL(level, x)     \
    if (level & DB_LEVEL) \
        cerr << x << endl;
#else
#define DB(x)
#define DBL(level, x)
#endif

namespace Rv
{
    using namespace std;
    using namespace TwkMovie;
    using namespace TwkFB;
    using namespace TwkUtil;
    using namespace TwkQtCoreUtil;

    //
    //  On Windows, we may end up with a mix of cases, some from FS, some from
    //  user typing, etc.  We should always compare without regard for case.
    //
    static bool qstringEQ(const QString& s1, const QString& s2)
    {
#ifdef PLATFORM_WINDOWS
        return 0 == QString::compare(s1, s2, Qt::CaseInsensitive);
#else
        return 0 == QString::compare(s1, s2, Qt::CaseSensitive);
#endif
    }

    struct MediaDirItem
    {
        MediaDirItem(const QFileInfo& i, unsigned int detail,
                     unsigned int hidden, unsigned int sortFlags,
                     const FileTypeTraits* traits, size_t m_fileTraitsIndex,
                     MediaDirItem* p);
        ~MediaDirItem();

        void populate();
        int rows();
        int row();
        bool hasChildren();

        QFileInfo info;
        QIcon icon;
        QList<MediaDirItem*> children;
        MediaDirItem* parent;
        QList<QVariant> data;
        bool populated : 1;
        bool iconed : 1;
        unsigned int hidden : 1;
        unsigned int detail : 2;
        unsigned int sortFlags : 8;

        const FileTypeTraits* m_fileTraits;
        size_t m_fileTraitsIndex;
    };

    namespace
    {

        void fillData(QList<QVariant>& data, QFileInfo* fi)
        {
            if (fi)
            {
                const auto path = UTF8::qconvert(fi->absoluteFilePath());

                data << fi->owner() << fi->size()
                     << (QString(TwkUtil::isReadable(path.c_str()) ? "r" : "-")
                         + QString(TwkUtil::isWritable(path.c_str()) ? "w"
                                                                     : "-"))
                     << fi->lastModified();
            }
            else
                data << QString() << QString() << QString() << QString();
        }

    }; // namespace

    MediaDirItem::MediaDirItem(const QFileInfo& i, unsigned int d,
                               unsigned int h, unsigned int s,
                               const FileTypeTraits* traits,
                               size_t fileTraitsIndex, MediaDirItem* p)
        : info(i)
        , parent(p)
        , populated(false)
        , iconed(false)
        , sortFlags(s)
        , detail(d)
        , m_fileTraits(traits)
        , m_fileTraitsIndex(fileTraitsIndex)
        , hidden(h)
    {
        if (parent)
        {
            QString file = info.absoluteFilePath();
            string sfile = UTF8::qconvert(file);
            data << info.fileName();

            switch (detail)
            {
            case 0:
                break;
            case 1:
            {
                if (info.exists())
                    fillData(data, &info);
                else
                {
                    ExistingFileList files = existingFilesInSequence(
                        UTF8::qconvert(info.absoluteFilePath()));

                    if (files.size())
                    {
                        QDir dir = parent->info.absoluteDir();
                        QFileInfo fi = QFileInfo(dir.absoluteFilePath(
                            UTF8::qconvert(files.back().name)));
                        fillData(data, &fi);
                    }
                    else
                        fillData(data, 0);
                }
            }
            break;
            case 2:
                if (!info.isDir())
                {
                    if (MovieReader* reader =
                            TwkMovie::GenericIO::movieReader(sfile, false))
                    {
                        try
                        {
                            reader->open(sfile);
                            MovieInfo minfo = reader->info();
                            int nc = minfo.numChannels;

                            QString t;
                            switch (minfo.dataType)
                            {
                            case TwkFB::FrameBuffer::UCHAR:
                                t = "8i";
                                break;
                            case TwkFB::FrameBuffer::USHORT:
                                t = "16i";
                                break;
                            case TwkFB::FrameBuffer::HALF:
                                t = "16f";
                                break;
                            case TwkFB::FrameBuffer::FLOAT:
                                t = "32f";
                                break;
                            case TwkFB::FrameBuffer::UINT:
                                t = "32i";
                                break;
                            case TwkFB::FrameBuffer::DOUBLE:
                                t = "64i";
                                break;
                            case TwkFB::FrameBuffer::PACKED_Cb8_Y8_Cr8_Y8:
                            case TwkFB::FrameBuffer::PACKED_Y8_Cb8_Y8_Cr8:
                                t = "8i";
                                nc = 3;
                                break;
                            case TwkFB::FrameBuffer::PACKED_R10_G10_B10_X2:
                            case TwkFB::FrameBuffer::PACKED_X2_B10_G10_R10:
                                t = "10i";
                                nc = 3;
                                break;
                            default:
                                t = "?";
                            }

                            data << minfo.width << minfo.height << t << nc;
                            int nframes = minfo.end - minfo.start;
                            if (nframes > 0)
                                data << minfo.fps << nframes;
                        }
                        catch (std::exception& exc)
                        {
                            DB("MediaDirItem Exception!");
                            data << "?" << "?" << "?" << "?";
                        }
                    }
                }
                break;
            }
        }
        else
        {
            switch (detail)
            {
            case 0:
                data << "Name";
                break;
            case 1:
                data << "Name" << "Owner" << "Size" << "Perm" << "Modified";
                break;
            case 2:
                data << "Name" << "Width" << "Height" << "Type" << "Ch" << "FPS"
                     << "#Frms";
                break;
            }
        }
    }

    MediaDirItem::~MediaDirItem()
    {
        for (int i = 0; i < children.size(); i++)
            delete children[i];
    }

    void MediaDirItem::populate()
    {
        HOP_PROF_FUNC();

        if (populated)
            return;
        DB("MediaDirItem::populate info "
           << UTF8::qconvert(info.absoluteFilePath()));

        if (info.isDir())
        {
            QDir dir(info.absoluteFilePath());

            QDir::Filters filter = QDir::NoDotAndDotDot | QDir::AllEntries
                                   | QDir::System | QDir::AllDirs
                                   | (hidden ? QDir::Hidden : QDir::Filters(0));

            QFileInfoList infos =
                dir.entryInfoList(filter, (QDir::SortFlags)sortFlags);
            DB("    found files: " << infos.size());

#ifdef PLATFORM_DARWIN
            if (!hidden && dir.isRoot())
            {
                if (QFile::exists("/Volumes"))
                    infos.push_back(QFileInfo("/Volumes"));
                if (QFile::exists("/Network"))
                    infos.push_back(QFileInfo("/Network"));
            }
#endif

            DB("populate: " << dir.absolutePath().toUtf8().data());

            FileNameList ifiles;

            for (int i = 0; i < infos.size(); i++)
            {
                QString fname = infos[i].fileName();
                DB("    " << fname.toUtf8().data());
                ;

                //
                //  There needs to be a more robust way to do this: in
                //  order to identify directories, add a / onto the end of
                //  the file name. the GlobalExtensionPredicate will force
                //  sequencesInFileList to ignore those when grouping
                //  names together.
                //

                if (infos[i].isDir())
                    fname += "/";
                DB("    pushing info #" << i << " " << fname.toUtf8().data());
                ;
                ifiles.push_back(fname.toUtf8().data());
            }

            SequenceNameList seqs =
                sequencesInFileList(ifiles, GlobalExtensionPredicate);
            DB("    found sequences: " << seqs.size());

            //
            //  _don't_ use canonicalPath here, since this will expand symlinks.
            //
            QString dirName = dir.absolutePath();

            for (int i = 0; i < seqs.size(); i++)
            {
                string seq = seqs[i];

                //
                //  Undo our dir hack from above
                //

                if (seq[seq.size() - 1] == '/')
                    seq.erase(seq.size() - 1, 1);

                //
                // Qt 4.8 introduced a bug in using
                // QDir::absoluteFilePath(file). This is a work around and
                // likely a speed up generating the path directly.
                //

                QFileInfo sinfo(dirName + "/" + UTF8::qconvert(seq.c_str()));

                if (sinfo.isDir()
                    || m_fileTraits->isKnown(m_fileTraitsIndex,
                                             sinfo.absoluteFilePath()))
                {
                    DB("    pushing seq #"
                       << i << " " << sinfo.absoluteFilePath().toUtf8().data());
                    ;
                    children.push_back(new MediaDirItem(
                        sinfo, detail, hidden, sortFlags, m_fileTraits,
                        m_fileTraitsIndex, this));
                }
                else
                {
                    DB("   excluding seq #"
                       << i << sinfo.absoluteFilePath().toUtf8().data());
                }
            }
        }
        else if (!info.exists())
        {
            string name = info.absoluteFilePath().toUtf8().data();
            QDir dir = info.absoluteDir();
            ExistingFileList files = existingFilesInSequence(name);

            for (int i = 0; i < files.size(); i++)
            {
                QString path = dir.absoluteFilePath(files[i].name.c_str());
                QFileInfo sinfo(path);
                if (sinfo.isDir()
                    || m_fileTraits->isKnown(m_fileTraitsIndex,
                                             sinfo.absoluteFilePath()))
                {
                    DB("    pushing file #"
                       << i << " " << sinfo.absoluteFilePath().toUtf8().data());
                    ;
                    children.push_back(new MediaDirItem(
                        sinfo, detail, hidden, sortFlags, m_fileTraits,
                        m_fileTraitsIndex, this));
                }
                else
                {
                    DB("   excluding seq #"
                       << i << sinfo.absoluteFilePath().toUtf8().data());
                }
            }
        }

        populated = true;
        DB("    populate() finished");
    }

    int MediaDirItem::rows()
    {
        populate();
        return children.size();
    }

    bool MediaDirItem::hasChildren()
    {
        if (populated)
            return children.size() != 0;
        if (info.isDir() || !info.exists())
            return true;
        return false;
    }

    int MediaDirItem::row()
    {
        return parent ? parent->children.indexOf(this) : 0;
    }

    //----------------------------------------------------------------------

    MediaDirModel::MediaDirModel(const QDir& dir, FileTypeTraits* traits,
                                 Details d, QObject* parent)
        : QAbstractItemModel(parent)
        , m_detail(d)
        , m_root(0)
        , m_fileTraits(traits)
        , m_sortFlags(QDir::Name)
        , m_fileTraitsIndex(0)
    {
        setDirectory(dir.absolutePath(), d);
    }

    MediaDirModel::MediaDirModel(FileTypeTraits* traits, QObject* parent)
        : QAbstractItemModel(parent)
        , m_detail(NoDetails)
        , m_root(0)
        , m_fileTraits(traits)
        , m_hidden(false)
        , m_sortFlags(QDir::Name)
        , m_fileTraitsIndex(0)
    {
    }

    MediaDirModel::~MediaDirModel()
    {
        delete m_root;
        delete m_fileTraits;
    }

    void MediaDirModel::setFileTraitsIndex(size_t i)
    {
        m_fileTraitsIndex = i;
        reload();
        layoutChanged(); // this will force a redraw of the view
    }

    void MediaDirModel::setFileTraits(FileTypeTraits* t)
    {
        delete m_fileTraits;
        m_fileTraits = t;
        reload();
        layoutChanged(); // this will force a redraw of the view
    }

    void MediaDirModel::reload()
    {
        if (!m_root)
            return;
        // QString dir = m_root->info.absolutePath();
        QString dir = m_root->info.absoluteFilePath();
        delete m_root;

        // const char* c = dir.toUtf8().data();
        // const char* o = odir.toUtf8().data();
        // cout << "reload = " << c << endl;
        m_root =
            new MediaDirItem(QFileInfo(dir), m_detail, m_hidden, m_sortFlags,
                             m_fileTraits, m_fileTraitsIndex, 0);
        m_root->populate();
        beginResetModel();
        endResetModel();
    }

    void MediaDirModel::setDirectory(const QDir& dir, Details d)
    {
        DB("MDM::setDirectory " << dir.absolutePath().toUtf8().data());
        delete m_root;
        m_detail = d;

        //
        // Make sure Qt treats this path as a directory by forcing the trailing
        // /
        //

        QString rootStr = dir.absolutePath();
        if (!rootStr.endsWith("/"))
            rootStr += "/";
        m_root =
            new MediaDirItem(QFileInfo(rootStr), (unsigned int)d, m_hidden,
                             m_sortFlags, m_fileTraits, m_fileTraitsIndex, 0);
        DB("    new item fileName "
           << m_root->info.absoluteFilePath().toUtf8().data());
        DB("    calling populate()");
        m_root->populate();
        DB("    calling reset()");
        beginResetModel();
        endResetModel();
        DB("MDM::setDirectory done");
    }

    int MediaDirModel::columnCount(const QModelIndex& parent) const
    {
        return m_root ? m_root->data.size() : 0;
    }

    QVariant MediaDirModel::data(const QModelIndex& index, int role) const
    {
        if (!m_root || !index.isValid())
            return QVariant();

        MediaDirItem* item =
            !index.isValid()
                ? m_root
                : static_cast<MediaDirItem*>(index.internalPointer());

        if (role == Qt::DisplayRole)
        {
            if (item->data.size() > index.column())
                return item->data[index.column()];
            else
                return QVariant();
        }
        else if (index.column() == 0 && role == Qt::DecorationRole)
        {
            if (!item->iconed)
            {
                item->iconed = true;

                if (!item->info.exists())
                {
                    QString name =
                        firstFileInPattern(
                            item->info.absoluteFilePath().toUtf8().data())
                            .c_str();
                    QFileInfo ninfo(
                        item->info.absoluteDir().absoluteFilePath(name));
                    item->icon = m_fileTraits->fileInfoIcon(ninfo);
                }
                else
                {
                    item->icon = m_fileTraits->fileInfoIcon(item->info);
                }
            }

            return item->icon;
        }
        else if (index.column() == 0 && role == Qt::UserRole)
        {
            return QVariant(item->info.absoluteFilePath());
        }
        else if (index.column() == 0 && role == Qt::ForegroundRole)
        {
            // if (!item->info.isDir() &&
            //     !m_fileTraits->isKnown(m_fileTraitsIndex,
            //     item->info.absoluteFilePath()))
            // {
            //     return QVariant(QBrush(QColor(128, 128, 128, 255)));
            // }

            return QVariant();
        }
        else
        {
            return QVariant();
        }
    }

    QModelIndex MediaDirModel::index(int row, int column,
                                     const QModelIndex& i) const
    {
        if (!m_root || !hasIndex(row, column, i))
            return QModelIndex();

        MediaDirItem* p = !i.isValid()
                              ? m_root
                              : static_cast<MediaDirItem*>(i.internalPointer());

        MediaDirItem* c = p->children[row];
        return c ? createIndex(row, column, c) : QModelIndex();
    }

    QModelIndex MediaDirModel::parent(const QModelIndex& index) const
    {
        if (m_root && index.isValid())
        {
            MediaDirItem* item =
                static_cast<MediaDirItem*>(index.internalPointer());
            if (item->parent == m_root || item->parent == 0)
                return QModelIndex();
            return createIndex(item->parent->row(), 0, item->parent);
        }
        else
        {
            return QModelIndex();
        }
    }

    int MediaDirModel::rowCount(const QModelIndex& index) const
    {
        if (!m_root || index.column() > 0)
            return 0;

        MediaDirItem* item =
            !index.isValid()
                ? m_root
                : static_cast<MediaDirItem*>(index.internalPointer());

        return item->rows();
    }

    QVariant MediaDirModel::headerData(int section, Qt::Orientation orientation,
                                       int role) const
    {
        if (m_root && orientation == Qt::Horizontal && role == Qt::DisplayRole)
        {
            return m_root->data[section];
        }
        else
        {
            return QVariant();
        }
    }

    bool MediaDirModel::hasChildren(const QModelIndex& index) const
    {
        if (!m_root || index.column() > 0)
            return 0;

        MediaDirItem* item =
            !index.isValid()
                ? m_root
                : static_cast<MediaDirItem*>(index.internalPointer());

        return item->hasChildren();
    }

    QString MediaDirModel::absoluteFilePath(const QModelIndex& index) const
    {
        if (!m_root || index.column() > 0)
            return 0;

        MediaDirItem* item =
            !index.isValid()
                ? m_root
                : static_cast<MediaDirItem*>(index.internalPointer());

        return item->info.absoluteFilePath();
    }

    Qt::ItemFlags MediaDirModel::flags(const QModelIndex& index) const
    {
        Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);

        if (m_root && index.column() == 0)
        {
            if (index.isValid())
            {
                MediaDirItem* item =
                    static_cast<MediaDirItem*>(index.internalPointer());

                if (!item->info.isDir()
                    && !m_fileTraits->isKnown(m_fileTraitsIndex,
                                              item->info.absoluteFilePath()))
                {
                    return Qt::NoItemFlags;
                }

                return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled
                       | defaultFlags;
            }
            else
            {
                return Qt::ItemIsDropEnabled | defaultFlags;
            }
        }
        else
        {
            return defaultFlags;
        }
    }

    QModelIndex MediaDirModel::indexOfPath(const QFileInfo& info)
    {
        DB("MDM::indexOfPath " << info.absoluteFilePath().toUtf8().data());
        if (!m_root || !info.exists())
        {
            DB("    returning from indexOfPath() 1");
            return QModelIndex();
        }
        DB("    columnCount() " << columnCount());

        if (info.isRoot())
        {
            if (!qstringEQ(info.absoluteFilePath(),
                           m_root->info.absoluteFilePath()))
            {
                DB("    root differs -- setting dir");
                setDirectory(info.absoluteFilePath(), Details(m_root->detail));
                return createIndex(m_root->row(), 0, m_root);
            }
            else
            {
                DB("    returning from indexOfPath() 2");
                return createIndex(m_root->row(), 0, m_root);
            }
        }

        MediaDirItem* item = m_root;
        if (1 == columnCount())
        //
        //  Then we're attached to a columnView, and we want to reset
        //  m_root only if the actual root changes (IE we were in "c:/"
        //  and now were'e in "//" or something")
        //
        {
            QDir dir(info.absoluteFilePath());
            QStringList parts;
            do
            {
                DB("    pushing " << dir.dirName().toUtf8().data());
                parts.push_front(dir.dirName());
                dir.cdUp();
            } while (!dir.isRoot());

            //
            // Make sure this path is compared as a directory by forcing the
            // trailing /
            //

            QString dirPath(dir.absolutePath());
            if (!dirPath.endsWith("/"))
                dirPath += "/";
            DB("    dir is at " << dirPath.toUtf8().data());

            if (!qstringEQ(m_root->info.absoluteFilePath(), dirPath))
            {
                DB("    not at correct root -- setting dir")
                DB("    m_root is at "
                   << UTF8::qconvert(m_root->info.absoluteFilePath()));
                DB("    new root is " << UTF8::qconvert(dirPath));
                setDirectory(dirPath, Details(m_root->detail));
                return createIndex(m_root->row(), 0, m_root);
            }

            for (int i = 0; i < parts.size(); i++)
            {
                MediaDirItem* matchItem = 0;
                item->populate();
                QString p1 = parts[i];
                DB("    part " << i << ": " << UTF8::qconvert(p1));
                DB("        item "
                   << UTF8::qconvert(item->info.absoluteFilePath()));
                DB("        has children " << item->hasChildren());

                for (int q = 0; !matchItem && q < item->children.size(); q++)
                {
                    MediaDirItem* child = item->children[q];

                    if (qstringEQ(child->info.fileName(), p1))
                    {
                        DB("        child matches");
                        matchItem = child;
                    }
                    else if (!child->info.exists())
                    {
                        //
                        //  This is a sequence
                        //
                        DB("    child "
                           << UTF8::qconvert(child->info.absoluteFilePath())
                           << " does not exist, assumint it is a sequence");
                        for (int j = 0;
                             !matchItem && j < child->children.size(); j++)
                        {
                            MediaDirItem* gchild = child->children[j];

                            if (qstringEQ(gchild->info.fileName(), p1))
                            {
                                DB("        gchild matches");
                                matchItem = gchild;
                            }
                        }
                    }
                }

                if (!matchItem)
                {
                    //
                    //  Check for a "hidden" (in the Qt sense) directory
                    //

                    if (item->info.isDir())
                    {
                        QDir idir(item->info.absoluteFilePath());

                        if (idir.exists(p1))
                        {
                            //
                            //  Add a child for the hidden item
                            //

                            QFileInfo sinfo(idir.absoluteFilePath(p1));
                            item->children.push_back(new MediaDirItem(
                                sinfo, item->detail, item->hidden,
                                item->sortFlags, m_fileTraits,
                                m_fileTraitsIndex, item));
                            DB("        hidden match");
                            matchItem = item->children.back();
                        }
                    }
                }

                if (!matchItem)
                {
                    //
                    //  Not necessarily an error to not find a match, since
                    //  this may be a partial sequence spec, and there for
                    //  not appearing in the lists.
                    //
                    //  Note that all callers of this method should be
                    //  checking isValid() on the returned index.
                    //
                    /*
                    cerr << "ERROR: MediaDirItem::" << __FUNCTION__ << endl;
                    cerr << "ERROR:     dir = " <<
                    dir.absolutePath().toUtf8().data() << endl; cerr << "ERROR:
                    info = " << info.absoluteFilePath().toUtf8().data() << endl;
                    cerr << "ERROR:     parts.size() = " << parts.size() <<
                    endl; cerr << "ERROR:     i = " << i << endl;
                    */
                    DB("        no match found, returning");
                    return QModelIndex();
                }

                item = matchItem;
            }
        }
        else
        //
        //  We're attached to a non-columnView, so we want to rese
        //  m_root if the incoming dir is not equal to that of m_root.
        //
        {
            QString dirPath =
                info.isDir() ? info.absoluteFilePath() : info.absolutePath();

            //
            // Make sure this path is compared as a directory by forcing the
            // trailing /
            //

            if (!dirPath.endsWith("/"))
                dirPath += "/";

            if (!qstringEQ(m_root->info.absoluteFilePath(), dirPath))
            {
                DB("    not at correct root -- setting dir")
                DB("    m_root is at "
                   << m_root->info.absoluteFilePath().toUtf8().data());
                DB("    new root is " << dirPath.toUtf8().data());
                setDirectory(dirPath, Details(m_root->detail));
            }
            //
            //  At this point m_root should be set to the dir part of the
            //  incomming path, or to the path itself if it's a dir.
            //  So we just need to find the baseName in this dir.
            //
            //  But if this is a dir, we won't find the baseName
            //  (because it's the dir) so just return the default index.

            if (info.isDir())
                return QModelIndex();

            item = 0;
            MediaDirItem* matchItem = 0;
            QString baseName = info.fileName();
            DB("    baseName " << baseName.toUtf8().data());
            m_root->populate();

            for (int q = 0; !matchItem && q < m_root->children.size(); q++)
            {
                MediaDirItem* child = m_root->children[q];
                DB("    child[" << q << "] "
                                << child->info.fileName().toUtf8().data());

                if (qstringEQ(child->info.fileName(), baseName))
                {
                    matchItem = child;
                }
                else if (!child->info.exists())
                {
                    //
                    //  This is a sequence
                    //

                    for (int j = 0; !matchItem && j < child->children.size();
                         j++)
                    {
                        MediaDirItem* gchild = child->children[j];

                        if (qstringEQ(gchild->info.fileName(), baseName))
                        {
                            matchItem = gchild;
                        }
                    }
                }
            }

            if (!matchItem)
            {
                //
                //  Check for a "hidden" (in the Qt sense) directory
                //

                if (m_root->info.isDir())
                {
                    QDir idir(m_root->info.absoluteFilePath());

                    if (idir.exists(baseName))
                    {
                        //
                        //  Add a child for the hidden item
                        //

                        QFileInfo sinfo(idir.absoluteFilePath(baseName));
                        m_root->children.push_back(new MediaDirItem(
                            sinfo, m_root->detail, m_root->hidden,
                            m_root->sortFlags, m_fileTraits, m_fileTraitsIndex,
                            m_root));
                        matchItem = m_root->children.back();
                    }
                }
            }

            if (!matchItem)
            {
                //
                //  Not necessarily an error to not find a match, since
                //  this may be a partial sequence spec, and there for
                //  not appearing in the lists.
                //
                //  Note that all callers of this method should be
                //  checking isValid() on the returned index.
                //
                /*
                cerr << "ERROR: MediaDirItem::" << __FUNCTION__ << endl;
                cerr << "ERROR:     dir = " <<
                dir.absolutePath().toUtf8().data() << endl; cerr << "ERROR: info
                = " << info.absoluteFilePath().toUtf8().data() << endl; cerr <<
                "ERROR:     parts.size() = " << parts.size() << endl; cerr <<
                "ERROR:     i = " << i << endl;
                */
                return QModelIndex();
            }
            item = matchItem;
        }

        DB("    match found, row "
           << item->row() << " info "
           << UTF8::qconvert(item->info.absoluteFilePath()));
        return createIndex(item->row(), 0, item);
    }

    QMap<int, QVariant> MediaDirModel::itemData(const QModelIndex& index) const
    {
        QMap<int, QVariant> roles;

        if (m_root && index.column() == 0)
        {
            for (int i = 0; i <= Qt::UserRole; ++i)
            {
                QVariant variantData = data(index, i);
                if (variantData.type() != QVariant::Invalid)
                    roles.insert(i, variantData);
            }
        }

        return roles;
    }

    void MediaDirModel::setSortFlags(QDir::SortFlags f)
    {
        if (m_sortFlags != f)
        {
            m_sortFlags = f;
            reload();
        }
    }

    void MediaDirModel::setShowHiddenFiles(bool b)
    {
        if (m_hidden != b)
        {
            m_hidden = b;
            if (m_root)
                setDirectory(m_root->info.absolutePath(),
                             Details(m_root->detail));
        }
    }

} // namespace Rv
