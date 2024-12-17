//
//  Copyright (c) 2010 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __RvPackage__PackageManager__h__
#define __RvPackage__PackageManager__h__
#include <iostream>
#include <QtCore/QtCore>

namespace Rv
{

#define RV_QSETTINGS \
    Rv::RvSettings& settings = Rv::RvSettings::globalSettings();

    class RvSettings
    {
    public:
        typedef QMap<QString, QVariant> SettingsMap;

        //
        //  QSettings API
        //
        void beginGroup(const QString& prefix);
        void endGroup();
        QVariant value(const QString& key,
                       const QVariant& defaultValue = QVariant()) const;
        void setValue(const QString& key, const QVariant& value);
        void remove(const QString& key);
        bool contains(const QString& key) const;
        void sync();

        QString fileName() const { return m_userSettings->fileName(); }

        bool isWritable() const { return m_userSettings->isWritable(); }

        //
        //  Get (possibly create) the single global settings object
        //
        static RvSettings& globalSettings();
        static void cleanupGlobalSettings();

    private:
        RvSettings();
        ~RvSettings();

        QSettings* getQSettings();

        QSettings* m_userSettings;

        // The QSettings class does not reset its error status: once an error
        // status has been flagged, it is never cleared. For reference :
        // https://bugreports.qt.io/browse/QTBUG-23857 With the aim of
        // preventing the occurence of numerous duplicated error messages, this
        // class will only report the first time an error is detected.
        bool m_userSettingsErrorAlredyReported{false};

        SettingsMap m_overridingSettings;
        SettingsMap m_clobberingSettings;

        static RvSettings* m_globalSettingsP;
    };

    class PackageManager
    {
    public:
        struct Mode
        {
            QString file;
            QString menu;
            QString shortcut;
            QString event;
            QString load;
            QString icon;
            QStringList
                requires;
        };

        struct AuxFile
        {
            QString file;
            QString location;
        };

        struct AuxFolder
        {
            QString folder;
            QString location;
        };

        struct Package
        {
            Package()
                : installed(false)
                , loadable(false)
                , zipFile(false)
                , installing(false)
                , hidden(false)
                , system(false)
                , optional(false)
                , compatible(false)
                , fileWritable(false)
                , dirWritable(false)
                , row(-1)
            {
            }

            QString name;
            QString baseName;
            QString file;
            QString dir;
            QString author;
            QString organization;
            QString contact;
            QString version;
            QString url;
            QString icon;
            QString
                requires;
            QStringList imageio;
            QStringList movieio;
            QString rvversion;
            QString openrvversion;
            QString description;
            QList<Mode> modes;
            QStringList files;

            QList<AuxFile> auxFiles;
            QList<AuxFolder> auxFolders;

            bool installed;
            bool loadable;
            bool zipFile;
            bool installing;
            bool hidden;
            bool system;
            bool optional;
            bool compatible;
            bool fileWritable;
            bool dirWritable;
            int row; // row in tree widget

            QList<int> uses;
            QList<int> usedBy;
        };

        struct ModeEntry
        {
            ModeEntry()
                : loaded(false)
                , active(false)
            {
            }

            QString name;
            QString package;
            QString menu;
            QString shortcut;
            QString event;
            QString rvversion;
            QString openrvversion;
            bool loaded;
            bool active;
            bool optional;
            QStringList
                requires;
        };

        typedef QList<Package> PackageList;
        typedef QList<ModeEntry> ModeEntryList;
        typedef QMap<QString, int> PackageMap;

        PackageManager();
        virtual ~PackageManager();

        virtual void loadPackages();
        virtual void loadPackageInfo(const QString&);
        virtual void loadInstalltionFile(const QDir&, const QString&);
        virtual void writeInstallationFile(const QString&);
        virtual bool addPackages(const QStringList& files, const QString& path);
        virtual void removePackages(const QStringList& files);

        virtual bool installPackage(Package&);
        virtual bool uninstallPackage(Package&);

        virtual bool isBundle(const QString&);
        virtual std::vector<QString> handleBundle(const QString&,
                                                  const QString&);

        virtual ModeEntryList loadModeFile(const QString&);
        virtual void writeModeFile(const QString&, const ModeEntryList&,
                                   int version = 0);

        virtual int findPackageIndexByZip(const QString&);
        virtual void findPackageDependencies();

        virtual bool allowLoading(Package&, bool, int d = 0);
        virtual bool makeSupportDirTree(QDir& root);

        //
        //  Override these for UI versions. The defaults use cin/cout to
        //  collect responses from the user.
        //

        virtual bool fixLoadability(const QString& msg);
        virtual bool fixUnloadability(const QString& msg);
        virtual bool overwriteExistingFiles(const QString& msg);
        virtual bool installDependantPackages(const QString& msg);
        virtual void errorMissingPackageDependancies(const QString& msg);
        virtual bool uninstallDependantPackages(const QString& msg);
        virtual void informCannotRemoveSomeFiles(const QString& msg);
        virtual void errorModeFileWriteFailed(const QString& file);
        virtual void informPackageFailedToCopy(const QString& msg);
        virtual void declarePackage(Package&, size_t);
        virtual bool uninstallForRemoval(const QString& msg);

        int auxFileIndex(Package&, const QString&);
        QString expandVarsInPath(Package&, const QString&);

        PackageList& packageList() { return m_packages; }

        void setNoConfirmation(bool force = true) { m_force = force; }

        static void setIgnorePrefs(bool b) { m_ignorePrefs = b; }

        static bool ignoringPrefs() { return m_ignorePrefs; }

        //
        //  Swap symbolic application dir name in/out for the actual directory.
        //
        static QStringList swapAppDir(const QStringList& packages, bool swapIn);

    private:
        bool yesOrNo(const char*, const char*, const QString&, const char*);

    protected:
        PackageList m_packages;
        PackageMap m_packageMap;
        QStringList m_doNotLoadPackages;
        QStringList m_optLoadPackages;
        bool m_force;

        static bool m_ignorePrefs;
    };

} // namespace Rv

#endif // __RvPackage__PackageManager__h__
