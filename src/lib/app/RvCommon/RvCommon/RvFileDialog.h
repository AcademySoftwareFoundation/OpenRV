//
//  Copyright (c) 2008 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __RvCommon__RvFileDialog__h__
#define __RvCommon__RvFileDialog__h__
#include <RvCommon/generated/ui_RvFileDialog.h>
#include <iostream>
#include <QtCore/QtCore>
#include <QtGui/QtGui>
#include <RvApp/Options.h>
#include <QtWidgets/QTreeView>
#include <QtWidgets/QColumnView>
#include <QtWidgets/QTextEdit>
#include <sstream>

#include <QDialog>

namespace Rv
{
    class MediaDirModel;
    class FileTypeTraits;

    /// Replacement Qt File Dialog with Preview

    ///
    /// This class replaces the default Qt file dialog. It does three
    /// things that are majorly different: NeXT-like multiple-list
    /// scrolling navigation (list mode), preview area, and handles
    /// sequences as a single item. Otherwise, it tries to be mostly
    /// compatible with the Qt API.
    ///

    class RvFileDialog : public QDialog
    {
        Q_OBJECT

    public:
        enum FileMode
        {
            OneExistingFile,
            ManyExistingFiles,
            ManyExistingFilesAndDirectories,
            OneFileName,
            OneDirectory,
            OneDirectoryName
        };

        enum ViewMode
        {
            ColumnView,
            DetailedFileView,
            DetailedMediaView,
            NoViewMode
        };

        enum Role
        {
            OpenFileRole,
            SaveFileRole
        };

        RvFileDialog(QWidget* parent, FileTypeTraits*, Role role,
                     Qt::WindowFlags flags = Qt::Sheet,
                     QString settingsGroup = "FileDialog");

        virtual ~RvFileDialog();

        void setDirectory(const QString& dir, bool force = false);
        void setCurrentFile(const QString& file);

        void setFileTypeTraits(FileTypeTraits*);
        void setFileTypeIndex(size_t);

        void setFileMode(FileMode m);
        void setFilter(QDir::Filters filters);
        void setViewMode(ViewMode v);
        void lockViewMode(bool lock);
        void setRole(Role r);
        void setTitleLabel(const QString&);

        QString existingFileChoice() const { return m_currentFile; }

        QStringList selectedFiles() const;
        void centerOverApp();

    public slots:
        void reloadTimeout();
        void sidePanelTimeout();
        void columnViewTimeout();
        void pathComboChanged(int);
        void inputPathEntry();
        void reload(bool);
        void autoRefreshChanged(int);
        void watchedDirChanged(const QString&);

        void sidePanelInserted(const QModelIndex&, int, int);
        void sidePanelPopup(const QPoint&);
        void sidePanelGo(bool);
        void sidePanelAccept(bool);
        void sidePanelRemove(bool);
        void sidePanelClick(QListWidgetItem*);
        void sidePanelDoubleClick(QListWidgetItem*);

        void viewComboChanged(int);
        void sortComboChanged(int);

        void columnSelectionChanged(const QItemSelection&,
                                    const QItemSelection&);
        void columnUpdatePreview(const QModelIndex&);

        void treeSelectionChanged(const QItemSelection&, const QItemSelection&);

        void iconViewChanged(QAction*);

        void doubleClickAccept(const QModelIndex&);

        void accept();
        void reject();

        void fileTypeChanged(int);

        void hiddenViewChanged(bool);

        void prevButtonTrigger(QAction*);
        void nextButtonTrigger(QAction*);

        void resizeColumns(const QModelIndex&);

    public:
        void done(int);

    protected:
        virtual bool eventFilter(QObject* object, QEvent* event);
        virtual bool event(QEvent*);
        void saveSidePanel();
        void loadSidePanel();

        QFileInfoList findMountPoints();

        QListWidgetItem* newItemForFile(const QFileInfo&);
        QIcon iconForFile(const QFileInfo&);

    private:
        FileMode m_fileMode;
        ViewMode m_viewMode;
        bool m_viewModeLocked;
        Role m_role;
        QDir::Filters m_filters;
        QString m_currentFile;
        QString m_settingsGroup;
        QStringList m_dirPrev;
        QStringList m_dirNext;
        Ui::RvFileDialog m_ui;
        bool m_building;
        QTimer* m_reloadTimer;
        QTimer* m_sidePanelTimer;
        QTimer* m_columnViewTimer;
        QString m_columnViewFile;
        QListWidgetItem* m_newCurrent;
        int m_sideInsertStart;
        int m_sideInsertEnd;
        QMenu* m_sidePanelPopup;
        bool m_sidePanelAccept;
        QTreeView* m_detailTree;
        QColumnView* m_columnView;
        QTextEdit* m_columnPreview;
        MediaDirModel* m_detailFileModel;
        MediaDirModel* m_detailMediaModel;
        MediaDirModel* m_columnModel;
        FileTypeTraits* m_fileTraits;
        QProcess m_process;
        QMenu* m_configPopup;
        QAction* m_cpSystem;
        QAction* m_cpGeneric;
        QAction* m_cpNone;
        QAction* m_cpHidden;
        QSet<QString> m_drives;
        QFileSystemWatcher* m_watcher;
        static QMap<QString, QIcon> m_iconMap;
        static QStringList m_visited;
        bool m_allowAutoRefresh;
    };

} // namespace Rv

#endif // __RvCommon__RvFileDialog__h__
