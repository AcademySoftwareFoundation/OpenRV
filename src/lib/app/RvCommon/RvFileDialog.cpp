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

#include <RvCommon/RvFileDialog.h>
#include <RvCommon/FileTypeTraits.h>
#include <RvCommon/QTUtils.h>
#include <RvCommon/MediaDirModel.h>
#include <RvCommon/RvPreferences.h>
#include <TwkUtil/FrameUtils.h>
#include <TwkUtil/PathConform.h>
#include <iostream>
#include <stl_ext/string_algo.h>
#include <QtCore/QDir>
#include <QtCore/QSet>
#include <QtGui/QtGui>
#include <QtWidgets/QCompleter>
#include <QtWidgets/QMenu>
#include <QtWidgets/QDirModel>

namespace Rv
{
    using namespace std;
    using namespace TwkUtil;

    class

        QMap<QString, QIcon>
            RvFileDialog::m_iconMap;
    QStringList RvFileDialog::m_visited;

#if 0
#define DB_ICON 0x01
#define DB_ALL 0xff

#define DB_LEVEL (DB_ALL & (~DB_ICON))
//#define DB_LEVEL        DB_ALL

#define DB(x) cerr << x << endl;
#define DBL(level, x)     \
    if (level & DB_LEVEL) \
        cerr << x << endl;
#else
#define DB(x)
#define DBL(level, x)
#endif

    //----------------------------------------------------------------------

    //----------------------------------------------------------------------

    RvFileDialog::RvFileDialog(QWidget* parent, FileTypeTraits* traits,
                               Role role, Qt::WindowFlags flags,
                               QString settingsGroup)
        : QDialog(parent, flags)
        , m_building(false)
        , m_fileMode(OneExistingFile)
        , m_viewMode(NoViewMode)
        , m_role(OpenFileRole)
        , m_filters(QDir::NoDotAndDotDot | QDir::AllEntries)
        , m_sidePanelAccept(false)
        , m_detailFileModel(0)
        , m_detailMediaModel(0)
        , m_columnModel(0)
        , m_fileTraits(0)
        , m_settingsGroup(settingsGroup)
        , m_viewModeLocked(false)
        , m_watcher(0)
    {
        DB("RvFileDialog Constructor");

        m_ui.setupUi(this);
        m_ui.searchButton->setVisible(false);
        m_ui.contentFrame->setMaximumHeight(0);

        //
        //   XXX hide the new folder button for now, since it's always
        //   disabled.
        m_ui.newFolderButton->hide();
        if (!traits)
            traits = new FileTypeTraits();

        RV_QSETTINGS;
        settings.beginGroup(m_settingsGroup);
        if (settings.contains("dialogGeometry"))
        {
            setGeometry(settings.value("dialogGeometry").toRect());
        }
        centerOverApp();

#ifdef PLATFORM_WINDOWS
        int iconMode =
            settings.value("iconMode", FileTypeTraits::NoIcons).toInt();
#else
        int iconMode =
            settings.value("iconMode", FileTypeTraits::SystemIcons).toInt();
#endif

        switch (iconMode)
        {
        default:
        case 0:
            traits->setIconMode(FileTypeTraits::SystemIcons);
            break;
        case 1:
            traits->setIconMode(FileTypeTraits::GenericIcons);
            break;
        case 2:
            traits->setIconMode(FileTypeTraits::NoIcons);
            break;
        }
        setFileTypeTraits(traits);

        if (m_visited.empty())
        {
            m_visited = settings.value("visited", QStringList()).toStringList();
        }

        int vmode = settings.value("viewModeNew", -1).toInt();
        int smode = settings.value("sortMethod", -1).toInt();
        bool showHiddenFiles =
            settings.value("showHiddenFiles", false).toBool();

        m_allowAutoRefresh = settings.value("allowAutoRefresh", false).toBool();
        m_ui.autoRefreshCheckbox->setCheckState(
            (m_allowAutoRefresh) ? Qt::Checked : Qt::Unchecked);

        setWindowTitle("Select Files/Directories");
        setSizeGripEnabled(true);
        m_reloadTimer = new QTimer(this);
        m_sidePanelTimer = new QTimer(this);
        m_columnViewTimer = new QTimer(this);
        m_reloadTimer->setSingleShot(true);
        m_sidePanelTimer->setSingleShot(true);
        m_columnViewTimer->setSingleShot(true);
        m_columnViewTimer->start(100);
        m_ui.buttonBox->setStandardButtons(
            m_role == OpenFileRole
                ? (QDialogButtonBox::Open | QDialogButtonBox::Cancel)
                : (QDialogButtonBox::Save | QDialogButtonBox::Cancel));

        m_ui.sidePanelList->setSelectionMode(
            QAbstractItemView::SingleSelection);
        loadSidePanel();

        while (QWidget* w = m_ui.viewStack->currentWidget())
            delete w;

        m_sidePanelPopup = new QMenu(this);
        QAction* spGo = m_sidePanelPopup->addAction("Go to Location");
        QAction* spUse = m_sidePanelPopup->addAction("Use Selection");
        m_sidePanelPopup->addSeparator();
        QAction* spRemove =
            m_sidePanelPopup->addAction("Remove Item from Panel");

        m_configPopup = new QMenu(this);
        m_cpSystem = m_configPopup->addAction("System Icons");
        m_cpGeneric = m_configPopup->addAction("Generic Icons");
        m_cpNone = m_configPopup->addAction("No Icons");
        m_configPopup->addSeparator();
        m_cpHidden = m_configPopup->addAction("Show Hidden Files");
        m_cpSystem->setCheckable(true);
        m_cpGeneric->setCheckable(true);
        m_cpNone->setCheckable(true);
        m_cpHidden->setCheckable(true);
        m_cpHidden->setChecked(showHiddenFiles);
        QActionGroup* cpGroup = new QActionGroup(this);
        cpGroup->addAction(m_cpSystem);
        cpGroup->addAction(m_cpGeneric);
        cpGroup->addAction(m_cpNone);
        m_ui.configButton->setMenu(m_configPopup);

        QIcon backIcon = colorAdjustedIcon(":images/back_32x32.png");
        QIcon forwardIcon = colorAdjustedIcon(":images/forwd_32x32.png");
        QIcon configIcon = colorAdjustedIcon(":images/confg_32x32.png");

        m_ui.previousButton->setDefaultAction(new QAction(backIcon, "<", this));
        m_ui.nextButton->setDefaultAction(new QAction(forwardIcon, ">", this));
        m_ui.configButton->setIcon(configIcon);

        switch (iconMode)
        {
        default:
        case 0:
            m_cpSystem->setChecked(true);
            break;
        case 1:
            m_cpGeneric->setChecked(true);
            break;
        case 2:
            m_cpNone->setChecked(true);
            break;
        }

        m_columnView = new QColumnView(this);
        QWidget* preview = new QWidget();
        m_columnPreview = new QTextEdit();
        QLayout* layout = new QVBoxLayout();
        layout->addWidget(m_columnPreview);
        preview->setLayout(layout);
        m_columnPreview->setReadOnly(true);
        QSizePolicy p(QSizePolicy::Ignored, QSizePolicy::Preferred);
        p.setHeightForWidth(true);
        m_columnPreview->setSizePolicy(p);
        preview->setSizePolicy(p);
        m_columnView->setPreviewWidget(preview);

        m_detailTree = new QTreeView(this);
        m_detailTree->setExpandsOnDoubleClick(false);
        m_detailTree->setAlternatingRowColors(true);

        m_detailTree->setDragEnabled(true);
        m_columnView->setDragEnabled(true);

        QList<int> widths;
        for (int i = 0; i < 25; i++)
            widths << 175;
        m_columnView->setColumnWidths(widths);

        m_columnModel = new MediaDirModel(m_fileTraits->copyTraits());
        m_columnView->setModel(m_columnModel);
        m_columnView->setSelectionBehavior(QAbstractItemView::SelectItems);

        setFileMode(m_fileMode);

        m_detailMediaModel = new MediaDirModel(m_fileTraits->copyTraits());
        m_detailFileModel = new MediaDirModel(m_fileTraits->copyTraits());
        m_detailTree->setModel(m_detailFileModel);

        m_detailMediaModel->setShowHiddenFiles(showHiddenFiles);
        m_detailFileModel->setShowHiddenFiles(showHiddenFiles);
        m_columnModel->setShowHiddenFiles(showHiddenFiles);

        m_ui.viewStack->addWidget(m_columnView);
        m_ui.viewStack->addWidget(m_detailTree);
        m_ui.viewStack->setCurrentIndex(0);
        m_ui.currentPath->installEventFilter(this);

        // #ifdef PLATFORM_DARWIN
        //  this is nice, but it makes a mess out of the slider if one becomes
        //  visible
        // m_ui.sidePanelList->setStyleSheet(QString::fromUtf8("background:
        // rgb(235, 240, 248)"));
        // #endif
        m_ui.sidePanelList->installEventFilter(this);

        connect(spGo, SIGNAL(triggered(bool)), this, SLOT(sidePanelGo(bool)));
        connect(spRemove, SIGNAL(triggered(bool)), this,
                SLOT(sidePanelRemove(bool)));
        connect(spUse, SIGNAL(triggered(bool)), this,
                SLOT(sidePanelAccept(bool)));

        connect(cpGroup, SIGNAL(triggered(QAction*)), this,
                SLOT(iconViewChanged(QAction*)));
        connect(m_cpHidden, SIGNAL(triggered(bool)), this,
                SLOT(hiddenViewChanged(bool)));

        connect(m_ui.previousButton, SIGNAL(triggered(QAction*)), this,
                SLOT(prevButtonTrigger(QAction*)));

        connect(m_ui.nextButton, SIGNAL(triggered(QAction*)), this,
                SLOT(nextButtonTrigger(QAction*)));

        connect(m_ui.sidePanelList->model(),
                SIGNAL(rowsInserted(const QModelIndex&, int, int)), this,
                SLOT(sidePanelInserted(const QModelIndex&, int, int)));

        connect(m_ui.sidePanelList, SIGNAL(itemClicked(QListWidgetItem*)), this,
                SLOT(sidePanelClick(QListWidgetItem*)));

        connect(m_ui.sidePanelList, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
                this, SLOT(sidePanelDoubleClick(QListWidgetItem*)));

        connect(m_ui.sidePanelList,
                SIGNAL(customContextMenuRequested(const QPoint&)), this,
                SLOT(sidePanelPopup(const QPoint&)));

        connect(m_reloadTimer, SIGNAL(timeout()), this, SLOT(reloadTimeout()));

        connect(m_sidePanelTimer, SIGNAL(timeout()), this,
                SLOT(sidePanelTimeout()));

        connect(m_columnViewTimer, SIGNAL(timeout()), this,
                SLOT(columnViewTimeout()));

        connect(m_ui.sortCombo, SIGNAL(currentIndexChanged(int)), this,
                SLOT(sortComboChanged(int)));

        connect(m_ui.viewCombo, SIGNAL(currentIndexChanged(int)), this,
                SLOT(viewComboChanged(int)));

        connect(m_ui.pathCombo, SIGNAL(currentIndexChanged(int)), this,
                SLOT(pathComboChanged(int)));

        connect(m_ui.currentPath, SIGNAL(returnPressed()), this,
                SLOT(inputPathEntry()));

        connect(m_detailTree, SIGNAL(doubleClicked(const QModelIndex&)), this,
                SLOT(doubleClickAccept(const QModelIndex&)));

        connect(m_detailTree, SIGNAL(collapsed(const QModelIndex&)), this,
                SLOT(resizeColumns(const QModelIndex&)));

        connect(m_detailTree, SIGNAL(expanded(const QModelIndex&)), this,
                SLOT(resizeColumns(const QModelIndex&)));

        connect(m_columnView, SIGNAL(doubleClicked(const QModelIndex&)), this,
                SLOT(doubleClickAccept(const QModelIndex&)));

        connect(m_columnView->selectionModel(),
                SIGNAL(selectionChanged(const QItemSelection&,
                                        const QItemSelection&)),
                this,
                SLOT(columnSelectionChanged(const QItemSelection&,
                                            const QItemSelection&)));

        connect(m_detailTree->selectionModel(),
                SIGNAL(selectionChanged(const QItemSelection&,
                                        const QItemSelection&)),
                this,
                SLOT(treeSelectionChanged(const QItemSelection&,
                                          const QItemSelection&)));

        connect(m_columnView, SIGNAL(updatePreviewWidget(const QModelIndex&)),
                this, SLOT(columnUpdatePreview(const QModelIndex&)));

        connect(m_ui.reloadButton, SIGNAL(clicked(bool)), this,
                SLOT(reload(bool)));

        connect(m_ui.autoRefreshCheckbox, SIGNAL(stateChanged(int)), this,
                SLOT(autoRefreshChanged(int)));

        connect(m_ui.fileTypeCombo, SIGNAL(currentIndexChanged(int)), this,
                SLOT(fileTypeChanged(int)));

        QCompleter* completer = new QCompleter(this);
        completer->setModel(new QDirModel(completer));
        completer->setCompletionMode(QCompleter::InlineCompletion);
        m_ui.currentPath->setCompleter(completer);
        m_ui.currentPath->setText("");

        if (vmode != -1)
        {
            m_ui.viewCombo->setCurrentIndex(vmode);

            //
            //  Need this because the combo will default to index 0 and thus
            //  provide no signal. The signal is required to intialize the
            //  column view.
            //

            if (vmode == ColumnView)
            {
                setViewMode(ColumnView);
            }
        }
        else
        {
            setViewMode(DetailedFileView);
        }

        if (smode != -1)
            m_ui.sortCombo->setCurrentIndex(smode);
    }

    RvFileDialog::~RvFileDialog() { delete m_fileTraits; }

    void RvFileDialog::setFileTypeTraits(FileTypeTraits* traits)
    {
        const bool sameAsCurrent = m_fileTraits && m_fileTraits->sameAs(traits);

        delete m_fileTraits;
        m_fileTraits = traits;

        // Nothing to do if the traits are the same as the current ones
        if (sameAsCurrent)
            return;

        m_ui.fileTypeCombo->clear();

        QStringList list = m_fileTraits->typeDescriptions();

        for (size_t i = 0; i < list.size(); i++)
        {
            m_ui.fileTypeCombo->addItem(list[i]);
        }

        if (m_columnModel)
        {
            m_columnModel->setFileTraits(traits->copyTraits());
            m_detailFileModel->setFileTraits(traits->copyTraits());
            m_detailMediaModel->setFileTraits(traits->copyTraits());
        }

        setFileTypeIndex(0);
    }

    void RvFileDialog::setFileTypeIndex(size_t index)
    {
        m_ui.fileTypeCombo->setCurrentIndex(index);
    }

    void RvFileDialog::setFileMode(FileMode m)
    {
        DB("setFileMode " << m);
        m_fileMode = m;

        if (m_fileMode == ManyExistingFiles
            || m_fileMode == ManyExistingFilesAndDirectories)
        {
            m_detailTree->setSelectionMode(
                QAbstractItemView::ExtendedSelection);
        }
        else
        {
            m_detailTree->setSelectionMode(QAbstractItemView::SingleSelection);
        }

        //
        //  We can't get columnView to behave properly with
        //  Multi/Extended selection, so we limit it to single
        //  selection for now.
        //
        m_columnView->setSelectionMode(QAbstractItemView::SingleSelection);
    }

    void RvFileDialog::setFilter(QDir::Filters filters) { m_filters = filters; }

    void RvFileDialog::setViewMode(ViewMode v)
    {
        m_ui.viewCombo->setCurrentIndex(v);
        m_viewMode = NoViewMode;
        viewComboChanged((int)v);
    }

    void RvFileDialog::setRole(Role r)
    {
        m_role = r;
        //  XXX this is hidden for now
        //  m_ui.newFolderButton->setDisabled(m_role == OpenFileRole);
        m_ui.buttonBox->setStandardButtons(
            m_role == OpenFileRole
                ? (QDialogButtonBox::Open | QDialogButtonBox::Cancel)
                : (QDialogButtonBox::Save | QDialogButtonBox::Cancel));
    }

    void RvFileDialog::setTitleLabel(const QString& text)
    {
        m_ui.titleLabel->setText(text);
    }

    QFileInfoList RvFileDialog::findMountPoints()
    {
        QFileInfoList list;

        list.push_back(QDir::home().path());
#if defined(PLATFORM_LINUX) || defined(PLATFORM_DARWIN)
        list.push_back(QString("/"));
        m_drives.insert("/");
#endif

#if defined(PLATFORM_LINUX)
        QStringList dirs;
        dirs << "/Volumes";
        dirs << "/Network";
        dirs << "/media";
        dirs << "/mnt";

        for (size_t i = 0; i < dirs.size(); i++)
        {
            QDir dir(dirs[i]);

            if (dir.exists())
            {
                //
                //  On linux, check and see if there's actually anything in
                //  there
                //

                QFileInfoList mpoints = dir.entryInfoList();

                // skip the . and .. entries
                for (int q = 2; q < mpoints.size(); q++)
                {
                    if (dirs[i] == "/media")
                    {
                        if (mpoints[q].isDir())
                        {
                            QString aPath = mpoints[q].absoluteFilePath();
                            QDir d(aPath);
                            if (d.count() <= 2)
                                continue;
                            else
                                m_drives.insert(aPath);
                        }
                        else
                        {
                            continue;
                        }
                    }

                    list.push_back(mpoints[q]);
                }
            }
        }

#if 0
    //
    //  Never got this to work
    //
    QFileInfo sbin_mount("/sbin/mount");
    QFileInfo bin_mount("/bin/mount");
    QFileInfo mount;

    if (sbin_mount.exists()) mount = sbin_mount;
    else if (bin_mount.exists()) mount = bin_mount;

    if (mount.exists() && mount.isExecutable())
    {
        m_process.execute(mount.absolutePath());
        m_process.waitForStarted();
        DB ("started");

        QByteArray array;

        m_process.waitForReadyRead();

        array.append(m_process.readAllStandardOutput());

        m_process.waitForFinished();

        for (int i=0; i < array.size(); i++)
        {
            //DB ("[i]");
        }
    }
#endif

#elif defined(PLATFORM_WINDOWS)

        //
        //  Windows, get the drives
        //

        QFileInfoList drives = QDir::drives();

        for (int i = 0; i < drives.size(); i++)
        {
            QString aPath = drives[i].absoluteFilePath();
            DB("DRIVES[" << i << "]: '" << aPath.toUtf8().data() << "'");
            m_drives.insert(aPath);
            list.push_back(drives[i]);
        }
#endif

        return list;
    }

    void RvFileDialog::loadSidePanel()
    {
        DB("loadSidePanel()");
        RV_QSETTINGS;
        QListWidget* l = m_ui.sidePanelList;
        l->clear();

        settings.beginGroup("RvFileDialog");
        QStringList paths =
            settings.value("sidePanelPaths", QStringList()).toStringList();
        DB("    settings value, " << paths.size() << "items");
        settings.endGroup();

        //  Add current working directory to head of list
        //
        QListWidgetItem* item =
            newItemForFile(QFileInfo(QDir::current().path()));
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        item->setText("Current Directory");
        l->addItem(item);

        QFileInfoList mounts = findMountPoints();

        for (int i = 0; i < mounts.size(); i++)
        {
            QListWidgetItem* item = newItemForFile(mounts[i]);
            item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            l->addItem(item);
        }

        for (int i = 0; i < paths.size(); i++)
        {
            QFileInfo info(paths[i]);
            l->addItem(newItemForFile(info));
        }
        DB("loadSidePanel() done");
    }

    void RvFileDialog::watchedDirChanged(const QString& dir)
    {
        if (!m_reloadTimer->isActive())
            m_reloadTimer->start(1000);
    }

    void RvFileDialog::setDirectory(const QString& inpath, bool force)
    {
        if (m_building)
            return;

        QString path = inpath;
        QString cpath = m_ui.currentPath->text();

        DB("setDirectory " << path.toUtf8().data() << " "
                           << m_ui.currentPath->text().toUtf8().data());

        QFileInfo pathInfo(path);

        if (path == "")
        {
            RV_QSETTINGS;
            settings.beginGroup(m_settingsGroup);
            path = settings.value("lastDir", inpath).toString();
            settings.endGroup();
            pathInfo = QFileInfo(path);
            if (!pathInfo.isDir())
            {
                path = QDir::currentPath();
                pathInfo = QFileInfo(path);
            }
            DB("    INITIALIZING with " << path.toUtf8().data());
        }

        /*
         *  Well, who knows.  For a while, I thought the below was
         *  really helping, preventing us from doing things twice.
         *  Now it seems to be necessary that we do those things twice
         *  (or more).  So I'm leaving it in case we need it later,
         *  but be aware that uncommenting the below triggers bad
         *  behavior in the columnview: mouse down on a file, and all
         *  files above it in the list will be highlighted untill you
         *  mouse up.
         *
        if (!m_dirPrev.empty())
        {
            QString prev = m_dirPrev.back();

            DB ("    m_dirPrev.back() " <<  prev.toUtf8().data());
            if (!force && !m_dirPrev.empty() && (path == prev ||
                        (prev.startsWith(path) &&
                        prev.endsWith("/") &&
                        prev.length() == path.length()+1)))
            {
                DB ("    returning from setDirectory");
                return;
            }
        }

        NOTE: (from jim) this is actually a Qt bug that I think the above
        code is just exacerbating
        */

        m_building = true;

        QDir dir(path, "", QDir::Name | QDir::IgnoreCase, m_filters);

        dir.makeAbsolute();
        QString absoluteDirPath = dir.absolutePath();

        if (m_allowAutoRefresh)
        {
            if (!m_watcher)
            {
                m_watcher = new QFileSystemWatcher(this);
                connect(m_watcher, SIGNAL(directoryChanged(const QString&)),
                        this, SLOT(watchedDirChanged(const QString&)));
            }

            if (!m_watcher->directories().empty())
                m_watcher->removePaths(m_watcher->directories());

            m_watcher->addPath(absoluteDirPath);
        }

        if (pathInfo.isDir())
        {
            if (!absoluteDirPath.endsWith("/"))
                absoluteDirPath += "/";
            if (m_dirPrev.empty() || absoluteDirPath != m_dirPrev.back())
            {
                m_dirPrev.push_back(absoluteDirPath);
            }
        }
        else
        {
            dir = QDir(pathInfo.absolutePath(), "",
                       QDir::Name | QDir::IgnoreCase, m_filters);
        }
        DB("    setting currentPath " << absoluteDirPath.toUtf8().data());
        m_ui.currentPath->setText(absoluteDirPath);
        m_ui.currentPath->end(false);
        m_currentFile = "";

        if (m_viewMode == DetailedFileView || m_viewMode == DetailedMediaView)
        {
            MediaDirModel* model =
                dynamic_cast<MediaDirModel*>(m_detailTree->model());
            model->setDirectory(dir, m_viewMode == DetailedFileView
                                         ? MediaDirModel::BasicFileDetails
                                         : MediaDirModel::MediaDetails);

            for (int i = 0; i < 8; i++)
                m_detailTree->resizeColumnToContents(i);
        }
        else if (m_viewMode == ColumnView)
        {
            QModelIndex i = m_columnModel->indexOfPath(absoluteDirPath);

            if (!i.isValid())
            {
                //
                //  On windows, we need to setdir to the root of the
                //  current file system. For example, C:/ or Y:/ not just
                //  "/" or some directory.
                //

                QDir fd = dir;
                while (fd.cdUp())
                    ;
                m_columnModel->setDirectory(fd, MediaDirModel::NoDetails);
            }

            //
            //    This is a hack to get around a Qt bug in the column view (or
            //    maybe its the scrolled area). The column view will get
            //    scrolled too far by setCurrentIndex() if its called before
            //    the dialog box is fully formed.
            //
            DB("    m_columnViewTimer active "
               << m_columnViewTimer->isActive());

            if (i != m_columnView->currentIndex())
            {
                if (m_columnViewTimer->isActive())
                {
                    m_columnViewFile = absoluteDirPath;
                }
                else if (i.isValid())
                {
                    DB("    setCurrentIndex on "
                       << absoluteDirPath.toUtf8().data());
                    m_columnView->setCurrentIndex(i);
                }
            }
        }

        DB("    rebuilding path combo");
        m_ui.pathCombo->clear();

        for (bool done = false; !done;)
        {
            QString aPath = dir.absolutePath();
            QString dName = dir.dirName();
            DB("    calling cdUp()");
            if (!dir.cdUp())
            //
            //  it's a root path
            //
            {
                QString aPathNoSlash = aPath;
                DB("    up to root: '" << aPath.toUtf8().data() << "'");
#ifdef WIN32
                if (aPathNoSlash.endsWith("/"))
                    aPathNoSlash.chop(1);
#endif
                m_ui.pathCombo->addItem(iconForFile(aPath), aPathNoSlash,
                                        aPath);

                done = true;
            }
            else
            {
                DB("    not at root: '" << aPath.toUtf8().data() << "'");
                m_ui.pathCombo->addItem(iconForFile(aPath), dName, aPath);
            }
        }

        DB("    adding Recent items");

        m_ui.pathCombo->insertSeparator(m_ui.pathCombo->count());
        m_ui.pathCombo->addItem("     Recent");
        m_ui.pathCombo->insertSeparator(m_ui.pathCombo->count());

        if (m_visited.contains(absoluteDirPath))
        {
            m_visited.move(m_visited.indexOf(absoluteDirPath), 0);
        }
        else
        {
            DB("adding to visited list: " << absoluteDirPath.toUtf8().data());
            m_visited.push_front(absoluteDirPath);
            if (m_visited.size() > 20)
                m_visited.pop_back();
        }

        QStringList newVisited;

        for (QStringList::const_iterator i = m_visited.begin();
             i != m_visited.end(); ++i)
        {
            QString name = *i;
            QFileInfo info(*i);
            QString aPath = info.absoluteFilePath();
            //
            //  Deciding if "drives" exist can take a while.  (We know
            //  they did when we added them to the list, Let's just assume
            //  they still do.
            //
            //  Actually this can take a long time whether it's a drive
            //  or not, cause it could be a network path pointing to a
            //  wedged mount, etc.  So don't check here and put it in
            //  the list, we'll find out if it doesn't exist when the
            //  user clicks on it. (see pathComboChanged())
            //
            //  if (m_drives.contains (aPath) || info.exists())
            {
                m_ui.pathCombo->addItem(iconForFile(info), name, aPath);
                newVisited.append(name);
            }
        }

        //
        //  This new list has only "drives" and existing directories.
        //
        m_visited = newVisited;

        m_ui.pathCombo->setCurrentIndex(0);
        DB("    rebuilding path combo done");

        m_ui.previousButton->defaultAction()->setEnabled(m_dirPrev.size() > 1);
        m_ui.nextButton->defaultAction()->setEnabled(m_dirNext.size() > 0);

        m_building = false;
        DB("setDirectory done");
    }

    void RvFileDialog::setCurrentFile(const QString& infileRef)
    {
        QString infile = infileRef;
        infile.replace("\\", "/");
        DB("setCurrentFile() infile " << infile.toUtf8().data());

        QFileInfo info(infile);

        if (info.isDir())
            setDirectory(info.absoluteFilePath());
        else
            setDirectory(info.absolutePath());

        //
        //  Just return if we're not changing anything.
        //
        if (m_currentFile == infile)
        {
            DB("    returning from setCurrentFile");
            return;
        }

        m_building = true;
        m_currentFile = infile;
        m_ui.currentPath->setText(infile);
        m_ui.currentPath->end(false);

        //
        //  Get appropriate Data Model and View
        //
        MediaDirModel* mdm(0);
        QAbstractItemView* view(0);
        if (m_viewMode == ColumnView)
        {
            mdm = m_columnModel;
            view = m_columnView;
        }
        else if (m_viewMode == DetailedFileView)
        {
            mdm = m_detailFileModel;
            view = m_detailTree;
        }
        else if (m_viewMode == DetailedMediaView)
        {
            mdm = m_detailMediaModel;
            view = m_detailTree;
        }

        //
        //  Ensure that this item is selected and current and visible, if it's
        //  in the list.
        //
        if (!mdm || !view)
        {
            DB("ERROR: RvFileDialog::" << __FUNCTION__
                                       << " Can't find Data Model or View");
        }
        else
        {
            DB("    calling mdm->indexOfPath()");
            QModelIndex i = mdm->indexOfPath(info);
            if (i.isValid() && view->currentIndex() != i)
            {
                DB("    selecting ");
                view->setCurrentIndex(i);
                //  view->selectionModel()->select (i,
                //  QItemSelectionModel::ClearAndSelect);
                //  view->selectionModel()->setCurrentIndex (i,
                //  QItemSelectionModel::ClearAndSelect);
                DB("    selecting complete");
                if (QTreeView* tv = dynamic_cast<QTreeView*>(view))
                {
                    tv->scrollTo(i, QAbstractItemView::PositionAtCenter);
                }
            }
            /*
             * At some point I thought we needed the below to deselect a
             * file when we've switch to selecting the directory that
             * contains the file.  But it seems to work without if now,
             * so I'm taking it out in the interests of simplicity, but
             * leaving it in the file, in case we think we need it in
             * the future.
             *
            else
            {
                if (QTreeView *tv = dynamic_cast <QTreeView *> (view))
                {
                    view->setCurrentIndex (QModelIndex());
                    //  view->selectionModel()->select (QModelIndex(),
            QItemSelectionModel::Clear);
                }
            }
            */
        }

        m_building = false;
        DB("setCurrentFile() done ");
    }

    void RvFileDialog::accept()
    {
        switch (m_fileMode)
        {
        case OneExistingFile:
        case OneDirectory:
        {
            if (TwkUtil::pathIsURL(m_currentFile.toStdString()))
            {
                QDialog::accept();
            }
            else
            {
                QFileInfo info(m_currentFile);

                if (info.exists())
                {
                    if ((m_fileMode == OneExistingFile && !info.isDir())
                        || (m_fileMode == OneDirectory && info.isDir()))
                    {
                        QDialog::accept();
                    }
                    break;
                }
            }
        }

        case ManyExistingFilesAndDirectories:
        case ManyExistingFiles:
        {
            QStringList files = selectedFiles();
            if (files.empty())
                break;
            bool ok = true;
            bool hasdir = false;

            for (int i = 0; i < files.size(); i++)
            {
                if (TwkUtil::pathIsURL(files[i].toStdString()))
                    continue;

                QFileInfo info(files[i]);

                if (!info.exists())
                {
                    QString test =
                        firstFileInPattern(files[i].toUtf8().data()).c_str();
                    QFileInfo testinfo(test);
                    if (!testinfo.exists())
                        ok = false;
                }

                if (info.isDir())
                    hasdir = true;
            }

            if (ok)
            {
                if (m_fileMode == ManyExistingFiles)
                {
                    if (!hasdir)
                        QDialog::accept();
                    else if (1 == files.size())
                        setDirectory(files[0]);
                }
                else
                {
                    QDialog::accept();
                }
            }

            break;
        }

        case OneFileName:
        case OneDirectoryName:
        {
            if (TwkUtil::pathIsURL(m_currentFile.toStdString()))
            {
                QDialog::accept();
            }
            else
            {
                QFileInfo info(m_currentFile);

                if (!info.exists()
                    || (info.exists()
                        && ((m_fileMode == OneFileName && !info.isDir())
                            || (m_fileMode == OneDirectoryName
                                && info.isDir()))))
                {
                    QDialog::accept();
                }
            }

            break;
        }
        }

        if (m_watcher)
        {
            delete m_watcher;
            m_watcher = 0;
        }
    }

    void RvFileDialog::reject()
    {
        if (m_watcher)
        {
            delete m_watcher;
            m_watcher = 0;
        }

        QDialog::reject();
    }

    void RvFileDialog::reloadTimeout()
    {
        if (!m_building)
            reload(true);
    }

    void RvFileDialog::reload(bool)
    {
        if (m_viewMode == ColumnView)
            m_columnModel->reload();
        QString current = m_ui.currentPath->text();
        setDirectory(current);
    }

    void RvFileDialog::autoRefreshChanged(int state)
    {
        m_allowAutoRefresh = (state != 0);

        RV_QSETTINGS;
        settings.beginGroup(m_settingsGroup);
        settings.setValue("allowAutoRefresh", m_allowAutoRefresh);
        settings.endGroup();

        if (m_allowAutoRefresh)
            reload(true);
    }

    void RvFileDialog::columnSelectionChanged(const QItemSelection& selected,
                                              const QItemSelection& deselected)
    {
        DB("columnSelectionChanged() m_building "
           << m_building << " sel " << selected.indexes().size() << " desel "
           << deselected.indexes().size());

        if (selected.empty())
            return;

        if (m_building)
        {
            DB("    returning from columnSelectionChanged");
            return;
        }

        const QModelIndex& i = m_columnView->selectionModel()->currentIndex();
        if (i.isValid())
        {
            QString path = m_columnModel->absoluteFilePath(i);
            DB("    path: '" << path.toUtf8().data() << "'");
            setCurrentFile(path);
        }
    }

    void RvFileDialog::treeSelectionChanged(const QItemSelection& selected,
                                            const QItemSelection& deselected)
    {
        DB("treeSelectionChanged() m_building "
           << m_building << " sel " << selected.indexes().size() << " desel "
           << deselected.indexes().size());

        if (m_building)
        {
            DB("    returning from treeSelectionChanged");
            return;
        }

        if (selected.empty())
            return;

        QModelIndexList selection =
            m_detailTree->selectionModel()->selectedIndexes();
        DB("    selection size " << selection.size());
        int reallyHowManySelected = 0;
        for (int i = 0; i < selection.size(); i++)
        {
            const QModelIndex& index = selection[i];

            if (index.isValid() && index.column() == 0)
                ++reallyHowManySelected;
        }
        DB("    real selection size " << reallyHowManySelected);

        if (reallyHowManySelected == 1)
        {
            MediaDirModel* model = 0;

            if (m_viewMode == DetailedFileView)
                model = m_detailFileModel;
            if (m_viewMode == DetailedMediaView)
                model = m_detailMediaModel;

            const QModelIndex& i =
                m_detailTree->selectionModel()->currentIndex();
            const QModelIndex& s =
                i.sibling(i.row(), 0); // Read path from first col
            QString path = "";
            if (s.isValid())
            {
                path = model->absoluteFilePath(s);
            }
            else if (!m_dirPrev.empty())
            {
                path = m_dirPrev.back();
            }

            m_ui.currentPath->setText(path);
            m_ui.currentPath->end(false);
            m_currentFile = "";
        }
        else
        {
            if (!m_dirPrev.empty())
            {
                m_ui.currentPath->setText(m_dirPrev.back());
                m_ui.currentPath->end(false);
                m_currentFile = "";
            }
        }
    }

    void RvFileDialog::pathComboChanged(int index)
    {
        DB("pathComboChanged() m_building " << m_building);
        if (!m_building)
        {
            QString t = m_ui.pathCombo->itemData(index).toString();

            QFileInfo pathInfo(t);
            if (!pathInfo.exists())
            {
                if (m_visited.removeOne(t))
                {
                    cerr << "WARNING: " << t.toUtf8().data()
                         << " does not exist, removing from list." << endl;
                }
                m_ui.pathCombo->setCurrentIndex(0);
            }
            else
                setDirectory(t);
        }
    }

    void RvFileDialog::inputPathEntry()
    {
        DB("inputPathEntry() m_building " << m_building);
        if (!m_building)
        {
            string p = varTildExp(m_ui.currentPath->text().toUtf8().data());
            setCurrentFile(p.c_str());
        }
    }

    void RvFileDialog::done(int r)
    {
        {
            RV_QSETTINGS;
            settings.beginGroup(m_settingsGroup);
            if (m_visited.empty())
                settings.remove("visited");
            else
                settings.setValue("visited", m_visited);
            settings.setValue("viewModeNew", m_ui.viewCombo->currentIndex());
            QFileInfo info(m_ui.currentPath->text());
            DB("lastDir = " << info.absolutePath().toUtf8().data());
            settings.setValue("lastDir", info.absolutePath());
            int iconMode = 0;
            if (m_cpGeneric->isChecked())
                iconMode = 1;
            else if (m_cpNone->isChecked())
                iconMode = 2;
            settings.setValue("iconMode", iconMode);
            settings.setValue("showHiddenFiles", m_cpHidden->isChecked());
            settings.setValue("dialogGeometry", geometry());
            settings.endGroup();
        }

        saveSidePanel();
        QDialog::done(r);
    }

    void RvFileDialog::columnViewTimeout()
    {
        DB("columnViewTimeout file " << m_columnViewFile.toUtf8().data());
        QModelIndex i = m_columnModel->indexOfPath(m_columnViewFile);
        if (i.isValid())
            m_columnView->setCurrentIndex(i);
    }

    void RvFileDialog::sidePanelTimeout()
    {
        //
        //  Delete any older duplicates
        //

        QListWidget* l = m_ui.sidePanelList;
        QStringList dropped;

        for (int i = m_sideInsertStart; i <= m_sideInsertEnd; i++)
        {
            QListWidgetItem* item = l->item(i);
            dropped.push_back(item->data(Qt::UserRole).toString());
        }

        QList<QListWidgetItem*> tbd;

        for (int i = 0; i < l->count(); i++)
        {
            if (i < m_sideInsertStart || i > m_sideInsertEnd)
            {
                QListWidgetItem* item = l->item(i);

                if (dropped.contains(item->data(Qt::UserRole).toString()))
                {
                    tbd.push_back(item);
                }
            }
        }

        for (int i = 0; i < tbd.size(); i++)
            delete tbd[i];

        saveSidePanel();
    }

    void RvFileDialog::saveSidePanel()
    {
        DB("saveSidePanel()");
        QListWidget* l = m_ui.sidePanelList;
        RV_QSETTINGS;
        QStringList paths;

        for (int i = 0; i < l->count(); i++)
        {
            QListWidgetItem* item = l->item(i);

            if (item->flags() & Qt::ItemIsDragEnabled)
            {
                QVariant v = item->data(Qt::UserRole);

                if (v.canConvert(QVariant::String))
                //
                //  Sometimes we end up here with QVariant::Invalid
                //  data, so check and skip any such.
                //
                {
                    QString p = v.toString();

                    if (!p.trimmed().isEmpty())
                    //
                    //  Don't know how, but sometimes we get empty strings in
                    //  the list.  We don't want to save them.
                    //
                    {
                        DB("    " << p.toUtf8().data());
                        paths.push_back(p);
                    }
                }
            }
        }
        settings.beginGroup("RvFileDialog");
        //
        //  This is super annoying.  If paths is empty, Qt writes
        //  garbage into the settings file ("@Invalid()").  So we check
        //  and remove this key from the settings if paths is empty.
        //
        DB("    settings setValue, " << paths.size() << "items");
        if (paths.empty())
            settings.remove("sidePanelPaths");
        else
            settings.setValue("sidePanelPaths", paths);
        settings.endGroup();
    }

    void RvFileDialog::sidePanelInserted(const QModelIndex& parent, int first,
                                         int last)
    {
        //
        //  For some reason, we can't get ahold of the side panel's new
        //  items in this function (eventhough it supposedly already
        //  finished inserting them). Perhaps Qt is trying to prevent us
        //  from accidently getting into an infinite loop. Anyway, its
        //  safer to edit the list in a new stack frame.
        //

        m_sideInsertStart = first;
        m_sideInsertEnd = last;
        m_sidePanelTimer->start();
    }

    void RvFileDialog::sidePanelPopup(const QPoint& pos)
    {
        m_sidePanelPopup->popup(m_ui.sidePanelList->mapToGlobal(pos));
    }

    void RvFileDialog::sidePanelClick(QListWidgetItem* item)
    {
        QString path = item->data(Qt::UserRole).toString();
        DB("RvFileDialog::sidePanelClick path " << path.toUtf8().data());
        sidePanelGo(true);
    }

    void RvFileDialog::sidePanelDoubleClick(QListWidgetItem* item)
    {
        sidePanelGo(true);
        m_sidePanelAccept = true;
        accept();
    }

    void RvFileDialog::sidePanelGo(bool)
    {
        if (QListWidgetItem* item = m_ui.sidePanelList->currentItem())
        {
            QString path = item->data(Qt::UserRole).toString();
            QFileInfo info(path);

            setCurrentFile(path);
        }
    }

    void RvFileDialog::sidePanelAccept(bool)
    {
        m_sidePanelAccept = true;
        accept();
    }

    void RvFileDialog::sidePanelRemove(bool)
    {
        QList<QListWidgetItem*> selection = m_ui.sidePanelList->selectedItems();

        for (int i = 0; i < selection.size(); i++)
        {
            delete selection[i];
        }

        saveSidePanel();
    }

    void RvFileDialog::hiddenViewChanged(bool b)
    {
        DB("hiddenViewChanged() b " << b << " m_building " << m_building);
        m_columnModel->setShowHiddenFiles(b);
        m_detailFileModel->setShowHiddenFiles(b);
        m_detailMediaModel->setShowHiddenFiles(b);

        ViewMode v = m_viewMode;
        setViewMode(NoViewMode);
        setViewMode(v);
        QFileInfo currentInfo(m_ui.currentPath->text());
        setDirectory(currentInfo.absoluteDir().absolutePath(), true);
    }

    void RvFileDialog::iconViewChanged(QAction* action)
    {
        FileTypeTraits::IconMode m;

        if (action == m_cpSystem)
        {
            m = FileTypeTraits::SystemIcons;
        }
        else if (action == m_cpGeneric)
        {
            m = FileTypeTraits::GenericIcons;
        }
        else if (action == m_cpNone)
        {
            m = FileTypeTraits::NoIcons;
        }

        m_fileTraits->setIconMode(m);
        m_columnModel->fileTraits()->setIconMode(m);
        m_detailMediaModel->fileTraits()->setIconMode(m);
        m_detailFileModel->fileTraits()->setIconMode(m);
        m_iconMap.clear();

        m_ui.sidePanelList->clear();
        loadSidePanel();

        ViewMode v = m_viewMode;
        setViewMode(NoViewMode);
        setViewMode(v);
        QFileInfo currentInfo(m_ui.currentPath->text());
        setDirectory(currentInfo.absoluteDir().absolutePath(), true);
    }

    bool RvFileDialog::event(QEvent* event)
    {
        if (event->type() == QEvent::KeyPress)
        {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

            if (keyEvent->modifiers() == Qt::ControlModifier
                && keyEvent->key() == Qt::Key_D)
            {
                //
                //  Go to ~/Desktop
                //

                setDirectory(QDir::home().absoluteFilePath("Desktop"));
                return true;
            }
            else if (keyEvent->modifiers()
                         == (Qt::ControlModifier | Qt::ShiftModifier)
                     && keyEvent->key() == Qt::Key_H)
            {
                //
                //  Go to ~
                //

                setDirectory(QDir::home().path());
                return true;
            }
        }

        return QDialog::event(event);
    }

    bool RvFileDialog::eventFilter(QObject* object, QEvent* event)
    {
        if (object == m_ui.sidePanelList)
        {
            if (event->type() == QEvent::DragEnter)
            {
                QDropEvent* devent = static_cast<QDropEvent*>(event);
            }

            return false;
        }
        else if (object == m_ui.currentPath)
        {
            if (event->type() == QEvent::KeyPress)
            {
                QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
                string utf8string = keyEvent->text().toUtf8().data();
                bool tab = keyEvent->key() == Qt::Key_Tab;

                if ((tab || utf8string == "/" || utf8string == "\\")
                    && m_ui.currentPath->selectionStart() != 0)
                {
                    m_ui.currentPath->end(true);

#ifdef WIN32
                    return false;
#else
                    string p =
                        varTildExp(m_ui.currentPath->text().toUtf8().data());

                    if (utf8string == "/")
                    {
                        if (p.empty() || p[p.size() - 1] != '/')
                            p += "/";
                    }

                    DB("eventFilter setting currentPath " << p);
                    m_ui.currentPath->setText(p.c_str());
                    return true;
#endif
                }
                else if (utf8string.size() == 1)
                {
                    switch (utf8string[0])
                    {
                    case 1 + ('a' - 'a'): // control-a
                        m_ui.currentPath->home(false);
                        return true;
                    case 1 + ('e' - 'a'): // control-e
                        m_ui.currentPath->end(false);
                        return true;
                    case 1 + ('k' - 'a'): // control-k
                        m_ui.currentPath->setText("");
                        return true;
                    case 1 + ('w' - 'a'): // control-w
                        m_ui.currentPath->cut();
                        return true;
                    case 1 + ('y' - 'a'): // control-y
                        m_ui.currentPath->paste();
                        return true;
                    }
                }
            }
            return m_ui.currentPath->eventFilter(object, event);
        }

        return false;
    }

    QStringList RvFileDialog::selectedFiles() const
    {
        MediaDirModel* model = 0;
        QAbstractItemView* view = 0;

        switch (m_viewMode)
        {
        case DetailedFileView:
            model = m_detailFileModel;
            view = m_detailTree;
            break;
        case DetailedMediaView:
            model = m_detailMediaModel;
            view = m_detailTree;
            break;
        case ColumnView:
            model = m_columnModel;
            view = m_columnView;
            break;
        default:
            break;
        }

        QItemSelectionModel* selectionModel = view->selectionModel();
        QModelIndexList selection = selectionModel->selectedIndexes();

        int reallyHowManySelected = 0;
        for (int i = 0; i < selection.size(); i++)
        {
            const QModelIndex& index = selection[i];

            if (index.isValid() && index.column() == 0)
                ++reallyHowManySelected;
        }
        DB("RvFileDialog::selectedFiles() selection size "
           << selection.size() << " really " << reallyHowManySelected);

        QStringList files;
        //
        //  We want to allow a "partial selection specification" like
        //  "blah.1-3#.exr" that does not appear in the list, and hence
        //  cannot be selected.  In this case the full selection spec
        //  will be in the list but may or may not be selected.
        //
        //  In all other cases when there is only one element in the
        //  selection, it should be the same as the text in currentPath.
        //

        if (reallyHowManySelected < 2)
        {
            // windows weirdness fix
            if (m_ui.currentPath)
            {
                QString text = m_ui.currentPath->text();
                files.push_back(text);
            }
        }
        else
        {
            for (int i = 0; i < selection.size(); i++)
            {
                const QModelIndex& index = selection[i];

                if (index.isValid() && index.column() == 0)
                {
                    QString file = model->absoluteFilePath(index);
                    files.push_back(file);
                    DB("    file " << file.toUtf8().data());
                }
            }
        }
        return files;
    }

    QListWidgetItem* RvFileDialog::newItemForFile(const QFileInfo& info)
    {
        DB("newItemForFile()");
        QString n = info.fileName();
        QString aPath;
        DB("    fileName '" << n.toUtf8().data() << "'");

        aPath = info.absoluteFilePath();
        if (n.isEmpty())
        {
            n = aPath;
#ifdef PLATFORM_WINDOWS
            if (n.endsWith("/"))
                n.chop(1);
#endif
        }
        DB("    aPath " << aPath.toUtf8().data());

        DB("    finding icon (icon size" << sizeof(QIcon) << ")");
        QIcon icon = iconForFile(info);
        DB("    making QListWidgetItem");
        QListWidgetItem* item = new QListWidgetItem(icon, n);
        DB("    calling item->setData()");
        item->setData(Qt::UserRole, QVariant(aPath));
        DB("newItemForFile() done");
        return item;
    }

    QIcon RvFileDialog::iconForFile(const QFileInfo& ininfo)
    {
        DBL(DB_ICON, "iconForFile()");
        QFileInfo info = ininfo;
        QString file = info.absoluteFilePath();
        DBL(DB_ICON, "    abs name " << file.toUtf8().data());

        if (m_iconMap.contains(file))
        {
            DBL(DB_ICON, "    found icon in map");
            return m_iconMap[file];
        }
        else
        {
            QIcon icon;

            //
            //  First check to see if we already know it's a "Drive".
            //
            if (m_drives.contains(file))
            {
                icon =
                    m_fileTraits->fileInfoIcon(info, QFileIconProvider::Drive);
            }
#ifdef PLATFORM_WINDOWS
            //
            //  Windows special case.  We don't want to stat network
            //  drives unless we really have to, so if it starts with
            //  "//" let's just say it's a network path and move on.
            //
            //  Maybe we could check for "/Network" on mac ...
            //
            else if (file.startsWith("//"))
            {
                DBL(DB_ICON, "    calling m_fileTraits->fileInfoIcon() 1");
                icon = m_fileTraits->fileInfoIcon(info,
                                                  QFileIconProvider::Network);
            }
#endif
            else
            {
                DBL(DB_ICON, "    calling exists()");
                if (!info.exists())
                {
                    DBL(DB_ICON, "    checking non-existant file");
                    QString name =
                        firstFileInPattern(file.toUtf8().data()).c_str();
                    info = QFileInfo(info.absoluteDir().absoluteFilePath(name));
                }
                DBL(DB_ICON, "    calling m_fileTraits->fileInfoIcon() 2");
                icon = m_fileTraits->fileInfoIcon(info);
            }
            m_iconMap[file] = icon;
            return icon;
        }
    }

    void RvFileDialog::resizeColumns(const QModelIndex&)
    {
        for (int i = 0; i < 8; i++)
            m_detailTree->resizeColumnToContents(i);
    }

    void RvFileDialog::doubleClickAccept(const QModelIndex& index)
    {
        if (!index.isValid())
            return;

        if (const MediaDirModel* model =
                dynamic_cast<const MediaDirModel*>(index.model()))
        {
            if (model->flags(index) == Qt::NoItemFlags)
                return;

            MediaDirModel* m = const_cast<MediaDirModel*>(model);
            QString path = model->absoluteFilePath(index);
            QFileInfo info(path);

            if (info.isDir())
            {
                if (model == m_detailMediaModel || model == m_detailFileModel)
                {
                    setDirectory(path);
                }
                else
                {
                    // nothing
                }
            }
            else
            {
                accept();
            }
        }
    }

    void RvFileDialog::prevButtonTrigger(QAction*)
    {
        if (m_dirPrev.size() > 1)
        {
            m_dirNext.push_back(m_dirPrev.back());
            m_dirPrev.pop_back();
            QString d = m_dirPrev.back();
            m_dirPrev.pop_back();
            setDirectory(d); // m_dirPrev will have d pushed onto it
        }
    }

    void RvFileDialog::nextButtonTrigger(QAction*)
    {
        if (m_dirNext.size() > 0)
        {
            QString d = m_dirNext.back();
            m_dirNext.pop_back();
            setDirectory(d);
        }
    }

    void RvFileDialog::sortComboChanged(int index)
    {
        QDir::SortFlags f = 0;

        switch (index)
        {
        case 0:
            f = QDir::Name;
            break;
        case 1:
            f = QDir::Time;
            break;
        case 2:
            f = QDir::Size;
            break;
        case 3:
            f = QDir::Type;
            break;
        }

        if (m_viewMode == ColumnView)
        {
            m_columnModel->setSortFlags(f);
            QFileInfo info(m_ui.currentPath->text());
            QModelIndex i = m_columnModel->indexOfPath(info);
            if (i.isValid() && !m_columnViewTimer->isActive())
                m_columnView->setCurrentIndex(i);
        }
        else if (m_viewMode == DetailedMediaView)
        {
            m_detailMediaModel->setSortFlags(f);
        }
        else if (m_viewMode == DetailedFileView)
        {
            m_detailFileModel->setSortFlags(f);
        }

        RV_QSETTINGS;
        settings.beginGroup(m_settingsGroup);
        settings.setValue("sortMethod", index);
        settings.endGroup();
    }

    void RvFileDialog::viewComboChanged(int index)
    {
        DB("viewComboChanged() index " << index);
        switch (index)
        {
        case 0:
            if (m_viewMode != ColumnView)
            {
                DB("    switching to ColumnView");
                m_viewMode = ColumnView;
                m_ui.viewStack->setCurrentIndex(0);
                m_columnModel->setDirectory(QDir::root(),
                                            MediaDirModel::NoDetails);

                if (!m_dirPrev.empty())
                    setDirectory(m_dirPrev.back(), true);
                else
                    setDirectory("", true);

                /*
                QFileInfo info(m_ui.currentPath->text());
                QModelIndex i =
                m_columnModel->indexOfPath(info.absoluteFilePath());
                //
                //    This is a hack to get around a Qt bug in the column view
                (or
                //    maybe its the scrolled area). The column view will get
                //    scrolled too far by setCurrentIndex() if its called before
                //    the dialog box is fully formed.
                //
                if (i != m_columnView->currentIndex())
                {
                    if (m_columnViewTimer->isActive())
                    {
                        m_columnViewFile = m_ui.currentPath->text();
                    }
                    else if (i.isValid())
                    {
                        m_columnView->setCurrentIndex(i);
                    }
                }
                */
            }
            break;

        case 1:
            if (m_viewMode != DetailedFileView)
            {
                m_viewMode = DetailedFileView;
                m_ui.viewStack->setCurrentIndex(1);
                m_detailTree->setModel(m_detailFileModel);
                connect(m_detailTree->selectionModel(),
                        SIGNAL(selectionChanged(const QItemSelection&,
                                                const QItemSelection&)),
                        this,
                        SLOT(treeSelectionChanged(const QItemSelection&,
                                                  const QItemSelection&)));
                if (!m_dirPrev.empty())
                    setDirectory(m_dirPrev.back(), true);
                else
                    setDirectory("", true);
                sortComboChanged(m_ui.sortCombo->currentIndex());
            }
            break;

        case 2:
            if (m_viewMode != DetailedMediaView)
            {
                m_viewMode = DetailedMediaView;
                m_ui.viewStack->setCurrentIndex(1);
                m_detailTree->setModel(m_detailMediaModel);
                connect(m_detailTree->selectionModel(),
                        SIGNAL(selectionChanged(const QItemSelection&,
                                                const QItemSelection&)),
                        this,
                        SLOT(treeSelectionChanged(const QItemSelection&,
                                                  const QItemSelection&)));
                if (!m_dirPrev.empty())
                    setDirectory(m_dirPrev.back(), true);
                else
                    setDirectory("", true);
                sortComboChanged(m_ui.sortCombo->currentIndex());
            }
            break;
        }
        DB("viewComboChanged() done");
    }

    void RvFileDialog::columnUpdatePreview(const QModelIndex& index)
    {
        DB("columnUpdatePreview()");
        QString file = m_columnModel->absoluteFilePath(index);
        m_columnPreview->clear();
        QStringList attrs = m_fileTraits->fileAttributes(file);

        DB("    traits has image: " << m_fileTraits->hasImage(file));
        if (m_fileTraits->hasImage(file))
        {
            QImage image = m_fileTraits->fileImage(file);
            m_columnPreview->document()->addResource(
                QTextDocument::ImageResource, QUrl("icon://file.raw"), image);
        }
        else
        {
            QPixmap pixmap =
                m_fileTraits->fileIcon(file).pixmap(QSize(128, 128));
            m_columnPreview->document()->addResource(
                QTextDocument::ImageResource, QUrl("icon://file.raw"), pixmap);
        }

        ostringstream str;
        str << "<html><body>"
            << "<center><img src=\"icon://file.raw\"><small>"
            << "<br><b><u>" << QFileInfo(file).fileName().toUtf8().data()
            << "</u></b><br>"
            << "<table width=95% border=0 cellspacing=1 cellpadding=0>";

        DB("    attrs size: " << attrs.size());
        for (size_t i = 0; i < attrs.size(); i += 2)
        {
            str << "<tr>"
                << "<td align=right><b>" << attrs[i].toUtf8().data()
                << "</b></td>"
                << "<td width=5></td>"
                << "<td align=left>" << attrs[i + 1].toUtf8().data() << "</td>"
                << "</tr>";
        }

        str << "</table>"
            << "</small></center>"
            << "</body></html>";

        m_columnPreview->setHtml(str.str().c_str());
    }

    void RvFileDialog::lockViewMode(bool lock)
    {
        if (lock)
            m_ui.viewCombo->hide();
        else
            m_ui.viewCombo->show();
    }

    void RvFileDialog::fileTypeChanged(int index)
    {
        m_detailFileModel->setFileTraitsIndex(index);
        m_detailMediaModel->setFileTraitsIndex(index);
        m_columnModel->setFileTraitsIndex(index);
    }

    void RvFileDialog::centerOverApp()
    {
        if (QWidget* app = QApplication::activeWindow())
        {
            QRect appGeom = app->geometry();
            int appX = appGeom.left() + int(appGeom.width() / 2);
            int appY = appGeom.top() + int(appGeom.height() / 2);

            int x = appX - int(width() / 2);
            int y = appY - int(height() / 2);
            DB("appX: " << appX << " appY: " << appY << " x: " << x
                        << " y: " << y);
            move(x, y);
        }
    }
} // namespace Rv
