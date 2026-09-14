//******************************************************************************
// Copyright (c) 2001-2005 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifdef _MSC_VER
//
//  We are targetting at least XP, (IE no windows95, etc).
//
#define WINVER 0x0501
#define _WIN32_WINNT 0x0501
#include <winsock2.h>
#include <windows.h>
#include <winnt.h>
#include <wincon.h>
#include <pthread.h>
//
//  NOTE: win_pthreads, which supplies implement.h, seems
//  targetted at an earlier version of windows (pre-XP).  If you
//  include implement.h here, it won't compile.  But as far as I
//  can tell, it's not needed, so just leave it out.
//
//  #include <implement.h>
#endif

#include <TwkApp/Document.h>
#include <TwkDeploy/Deploy.h>
#include <TwkExc/TwkExcException.h>
#include <TwkUtil/FrameUtils.h>
#include <TwkUtil/SystemInfo.h>
#include <stl_ext/string_algo.h>
#include <TwkUtil/File.h>
#include <TwkUtil/PathConform.h>
#include <RvPackage/PackageManager.h>
#include <arg.h>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sched.h>
#include <sstream>
#include <stdlib.h>
#include <stl_ext/thread_group.h>
#include <string.h>
#include <vector>
#include <set>

#ifdef PLATFORM_LINUX
#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>
#endif

#ifdef PLATFORM_DARWIN
#include <DarwinBundle/DarwinBundle.h>
#else
#include <QTBundle/QTBundle.h>
#endif

#include <QtCore/QtCore>
#include <QFileInfo>
#include <QDateTime>

//
//  Check that a version actually exists. If we're compiling opt error
//  out in cpp. Otherwise, just note the lack of version info.
//

#if MAJOR_VERSION == 99
#if NDEBUG
// #error ********* NO VERSION INFORMATION ***********
#else
#ifndef _MSC_VER
#warning ********* NO VERSION INFORMATION ***********
#endif
#endif
#endif

#ifdef WIN32
#define FILE_SEP ";"
#else
#define FILE_SEP ":"
#endif

using namespace std;
using namespace Rv;
using namespace TwkUtil;

void setEnvVar(const string& var, const string& val)
{
#ifdef WIN32
    ostringstream str;
    str << var << "=" << val;
    putenv(str.str().c_str());
#else
    setenv(var.c_str(), val.c_str(), 1);
#endif
}

void setEnvVar(const string& var, const QFileInfo& val) { setEnvVar(var, val.absoluteFilePath().toUtf8().data()); }

void addPathToEnv(const string& var, const string& path)
{
    string newValue;

    if (const char* v = getenv(var.c_str()))
    {
        string newValue = pathConform(v);
        newValue += FILE_SEP;
    }

    newValue += pathConform(path);
    setEnvVar(var, newValue);
}

bool hasPathInEnv(const string& var, const string& path0)
{
    string path = pathConform(path0);

    if (const char* v = getenv(var.c_str()))
    {
        vector<string> tokens;
        stl_ext::tokenize(tokens, pathConform(v), FILE_SEP);

        for (size_t i = 0; i < tokens.size(); i++)
        {
            if (tokens[i] == path)
                return true;
        }
    }

    return false;
}

void includeIfNotThere(const string& path0)
{
    string path = pathConform(path0);

    if (!hasPathInEnv("RV_SUPPORT_PATH", path))
    {
        addPathToEnv("RV_SUPPORT_PATH", path);
    }
}

vector<string> inputArgs;

int parseInFiles(int argc, char* argv[])
{
    for (int i = 0; i < argc; i++)
    {
        inputArgs.push_back(pathConform(argv[i]));
    }

    return 0;
}

//
//  rvpkg pretents to be rv
//

#ifdef PLATFORM_DARWIN
#define BundleType TwkApp::DarwinBundle
#define APPNAME "RV"
#else
#define BundleType TwkApp::QTBundle
#define APPNAME "rv"
#endif

ostream& operator<<(ostream& o, const QString& str)
{
    o << str.toUtf8().constData();
    return o;
}

bool matchesNameOrFile(const PackageManager::Package& p, const QString& str)
{
    QFileInfo pFI(p.file);
    QFileInfo strFI(str);

    if (strFI.exists())
    {
        if (pFI.canonicalFilePath() == strFI.canonicalFilePath())
            return true;
        return false;
    }

    if (p.name == str)
        return true;
    if (pFI.fileName() == str)
        return true;

    return false;
}

bool matchesNameOrFile(const PackageManager::Package& p, const std::string& str) { return matchesNameOrFile(p, QString(str.c_str())); }

void matchingPackages(const PackageManager::PackageList& inlist, vector<int>& outindexlist)
{
    set<int> indexSet;

    for (size_t i = 0; i < inputArgs.size(); i++)
    {
        for (size_t q = 0; q < inlist.size(); q++)
        {
            if (matchesNameOrFile(inlist[q], inputArgs[i]))
            {
                indexSet.insert(q);
            }
        }
    }

    for (set<int>::iterator i = indexSet.begin(); i != indexSet.end(); ++i)
    {
        outindexlist.push_back(*i);
    }
}

int utf8Main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    //
    //  Parse cmd line args
    //

    int install = 0;
    int uninstall = 0;
    int update = 0;
    int list = 0;
    int info = 0;
    int add = 0;
    char* addDir = 0;
    int remove = 0;
    int env = 0;
    int force = 0;
    int optin = 0;
    char* filename = 0;
    char* withDir = 0;
    char* onlyDir = 0;

    //
    // We handle the '--help' flag by changing it to '-help' for
    // argparse to work.
    //
    for (int i = 0; i < argc; ++i)
    {
        if (!strcmp("--help", argv[i]))
        {
            strcpy(argv[i], "-help");
            break;
        }
    }

    if (arg_parse(
            argc, argv, "", "Usage: rvpkg command line .rvpkg management", "", "  In examples below 'area' means a support area", "",
            "  Many commands can use one of: package name, rvpkg file name,", "",
            "  or full path to rvpkg file. the commands -remove, -info,", "",
            "  -install, -uninstall, operating on already added packages.", "", "  They will not work on packages not visible to -list", "",
            "", "", "  List all in RV_SUPPORT_PATH:             rvpkg -list", "",
            "  List in specific area:                   rvpkg -list -only "
            "/path/to/support/area",
            "",
            "  List including an area:                  rvpkg -list -include "
            "/path/to/additional/support/area",
            "",
            "  Info about a package:                    rvpkg -info "
            "/path/to/file.rvpkg",
            "",
            "  Add package(s) to an area:               rvpkg -add "
            "/path/to/area /path/to/file1.rvpkg /path/to/file2.rvpkg ...",
            "",
            "  Remove package(s):                       rvpkg -remove "
            "/path/to/file1.rvpkg ...",
            "",
            "  Install package(s):                      rvpkg -install "
            "/path/to/file1.rvpkg ...",
            "",
            "  Uninstall package(s):                    rvpkg -uninstall "
            "/path/to/file1.rvpkg ...",
            "",
            "  Update package(s):                       rvpkg -update "
            "/path/to/file1.rvpkg ...",
            "",
            "  Add and install package(s):              rvpkg -install -add "
            "/path/to/area /path/to/file1.rvpkg ...",
            "",
            "  Opt-in all users to optional package(s): rvpkg -optin "
            "/path/to/file1.rvpkg ...",
            "", "", "", ARG_SUBR(parseInFiles), "Input packages names or rvpkg files", "-include %S", &withDir,
            "include directory as if part of RV_SUPPORT_PATH", "-env", ARG_FLAG(&env), "show RV_SUPPORT_PATH include app areas", "-only %S",
            &onlyDir, "use directory as sole content of RV_SUPPORT_PATH", "-add %S", &addDir, "add packages to specified support directory",
            "-remove", ARG_FLAG(&remove), "remove packages (by name, rvpkg name, or full path to rvpkg)", "-install", ARG_FLAG(&install),
            "install packages (by name, rvpkg name, or full path to rvpkg)", "-uninstall", ARG_FLAG(&uninstall),
            "uninstall packages (by name, rvpkg name, or full path to rvpkg)", "-update", ARG_FLAG(&update),
            "update packages to newer version (by name, rvpkg name, or full path to rvpkg)", "-optin", ARG_FLAG(&optin),
            "make installed optional packages opt-in by default for all users", "-list", ARG_FLAG(&list), "list installed packages",
            "-info", ARG_FLAG(&info),
            "detailed info about packages (by name, rvpkg name, or full path "
            "to rvpkg)",
            "-force", ARG_FLAG(&force), "Assume answer is 'y' to any confirmations -- don't be interactive", NULL)
        < 0)
    {
        exit(-1);
    }

    add = addDir ? 1 : 0;

    if (inputArgs.empty() && !list && !info && !env)
    {
        cerr << "ERROR: use -help for usage" << endl;
    }

    if (add && remove)
    {
        cerr << "ERROR: only one of -add or -remove allowed" << endl;
        exit(-1);
    }

    if (onlyDir && (withDir || addDir))
    {
        cerr << "ERROR: -only cannot be used with -include or -add" << endl;
        exit(-1);
    }

    if (addDir && withDir)
    {
        cout << "WARNING: -include ignored with -add" << endl;
    }

    if ((remove && install) || (add && uninstall) || (update && (add || remove)))
    {
        cerr << "ERROR: conflicting arguments" << endl;
        exit(-1);
    }

    if ((remove || add || install || uninstall || update) && (list || info))
    {
        cerr << "ERROR: -list and -info do not work with other arguments" << endl;
        exit(-1);
    }

    if (list && info)
    {
        cerr << "ERROR: only one of -list or -info allowed" << endl;
        exit(-1);
    }

    if ((info || install || uninstall || remove || add || update) && inputArgs.empty())
    {
        cerr << "ERROR: need more arguments" << endl;
        exit(-1);
    }

#ifdef PLATFORM_WINDOWS
    string addDirConformed;
    string withDirConformed;
    string onlyDirConformed;

    if (addDir)
    {
        addDirConformed = pathConform(addDir);
        addDir = (char*)addDirConformed.c_str();
    }

    if (withDir)
    {
        withDirConformed = pathConform(withDir);
        withDir = (char*)withDirConformed.c_str();
    }

    if (onlyDir)
    {
        onlyDirConformed = pathConform(onlyDir);
        onlyDir = (char*)onlyDirConformed.c_str();
    }
#endif

    if (addDir)
        includeIfNotThere(addDir);
    else if (withDir)
        includeIfNotThere(withDir);
    else if (onlyDir)
        setEnvVar("RV_SUPPORT_PATH", onlyDir);

    BundleType bundle(APPNAME, MAJOR_VERSION, MINOR_VERSION, REVISION_NUMBER);

    setEnvVar("LANG", "C");
    setEnvVar("LC_ALL", "C");

    if (env)
    {
        TwkApp::Bundle::PathVector paths = bundle.pluginPath("Packages");

        for (size_t i = 0; i < paths.size(); i++)
        {
            cout << paths[i] << endl;
        }

        exit(0);
    }

    TWK_DEPLOY_APP_OBJECT dobj(MAJOR_VERSION, MINOR_VERSION, REVISION_NUMBER, argc, argv, RELEASE_DESCRIPTION, "HEAD=" GIT_HEAD);

    Rv::PackageManager manager;
    manager.loadPackages();
    PackageManager::PackageList& packages = manager.packageList();

    if (force)
        manager.setNoConfirmation();

    if (list)
    {
        if (packages.empty())
        {
            cout << "No packages found" << endl;
        }

        for (size_t i = 0; i < packages.size(); i++)
        {
            PackageManager::Package& p = packages[i];
            cout << (p.installed ? "I" : "-");
            cout << " " << (p.loadable ? "L" : "-");
            cout << " " << (p.optional ? "O" : "-");

            cout << " " << p.version << " \"" << p.name << "\" " << p.file << endl;
        }
    }

    if (info)
    {
        vector<int> indices;
        matchingPackages(packages, indices);

        if (indices.empty())
        {
            cout << "No matching packages found" << endl;
        }

        for (size_t i = 0; i < indices.size(); i++)
        {
            PackageManager::Package& p = packages[indices[i]];

            cout << "Name: " << p.name << endl;
            cout << "Version: " << p.version << endl;
            cout << "Installed: " << (p.installed ? "YES" : "NO") << endl;
            cout << "Loadable: " << (p.loadable ? "YES" : "NO") << endl;
            cout << "Directory: " << p.dir << endl;
            cout << "Author: " << p.author << endl;
            cout << "Organization: " << p.organization << endl;
            cout << "Contact: " << p.contact << endl;
            cout << "URL: " << p.url << endl;
            cout << "Requires: "
                 << p.
                        requires
                 <<
                    endl;
            cout << "RV-Version: " << p.rvversion << endl;
            cout << "OpenRV-Version: " << p.openrvversion << endl;
            cout << "Hidden: " << (p.hidden ? "YES" : "NO") << endl;
            cout << "System: " << (p.system ? "YES" : "NO") << endl;
            cout << "Optional: " << (p.optional ? "YES" : "NO") << endl;
            cout << "Writable: " << (p.fileWritable ? "YES" : "NO") << endl;
            cout << "Dir-Writable: " << (p.dirWritable ? "YES" : "NO") << endl;

            cout << "Modes:";

            for (size_t j = 0; j < p.modes.size(); j++)
            {
                cout << " " << p.modes[j].file;
            }

            cout << endl;

            cout << "Files:";

            for (size_t j = 0; j < p.files.size(); j++)
            {
                cout << " " << p.files[j];
            }

            cout << endl;
        }
    }

    if (optin)
    {
        vector<int> indices;
        matchingPackages(packages, indices);

        if (indices.empty())
        {
            cout << "No matching packages found" << endl;
        }

        for (size_t i = 0; i < indices.size(); i++)
        {
            PackageManager::Package& p = packages[indices[i]];

            if (!p.optional)
            {
                cerr << "ERROR: " << p.file.toUtf8().constData() << " is not an optional package -- ignoring" << endl;
                continue;
            }

            QString file = p.file;
            QFileInfo info(file);

            if (!info.exists())
            {
                cerr << "ERROR: " << p.file.toUtf8().constData() << " doesn't exist -- ignoring" << endl;
                continue;
            }

            QDir dir = info.absoluteDir();
            dir.cdUp();
            dir.cd("Mu");
            QString rvload2 = dir.absoluteFilePath("rvload2");
            QFileInfo rvloadInfo(rvload2);

            if (!rvloadInfo.exists())
            {
                cerr << "ERROR: missing rvload2 file at " << rvload2.toUtf8().constData() << " -- ignoring" << endl;
                break;
            }

            PackageManager::ModeEntryList entries = manager.loadModeFile(rvload2);

            //
            //  Just force the package to become not optional
            //

            QString basename = info.fileName();
            cout << "INFO: for package " << basename.toUtf8().constData() << endl;

            for (size_t i = 0; i < entries.size(); i++)
            {
                PackageManager::ModeEntry& entry = entries[i];

                if (entry.package == basename)
                {
                    entry.loaded = true;
                    entry.optional = false;

                    cout << "INFO: opting-in all users for mode " << entry.name.toUtf8().constData() << endl;
                }
            }

            manager.writeModeFile(rvload2, entries);
        }
    }

    if (add)
    {
        QStringList files;

        // Will hold added packages from a bundle if an bundle is passes
        std::vector<QString> addedPackages = {};

        for (size_t i = 0; i < inputArgs.size(); i++)
        {
            string curFile = inputArgs[i];

            // Checking to see if the file is a bundle
            if (manager.isBundle(QString::fromStdString(curFile)))
            {
                cout << "INFO: Bundle detected, unpacking now." << endl;

                // Unpacking bundle
                QString toUnzip = QString::fromStdString(curFile);
                string addDirStr = addDir;
                QString outputDir = QString::fromStdString(addDirStr.append("/Packages"));
                addedPackages = manager.handleBundle(toUnzip, outputDir);

                // Ensuring that the bundle unpacked successfully
                if (addedPackages.size() > 0)
                {
                    cout << "INFO: Bundle unpacked successfully " << endl;
                    cout << "INFO: Added the following packages..." << endl;

                    for (QString s : addedPackages)
                    {
                        cout << s.toStdString() << endl;
                    }
                }
                else
                {
                    cerr << "ERROR: Unable to install bundle." << endl;
                }
            }
            else
            {
                files.push_back(inputArgs[i].c_str());
            }
        }

        // Adding bundle subpackages to input arguments to allow for chain calls
        // (e.g. -install -add ...)
        if (addedPackages.size() > 0)
        {

            for (QString s : addedPackages)
            {
                inputArgs.push_back(s.toStdString());
            }
        }

        if (!manager.addPackages(files, addDir))
        {
            cout << "exiting" << endl;
            exit(-1);
        }

        //
        //  Convert the input args to existing pkg file names so that
        //  -install can operate on them too
        //

        for (size_t i = 0; i < inputArgs.size(); i++)
        {
            QString name(inputArgs[i].c_str());
            QString base = name.split("/").back();

            // Bundles do not need to be modified and should instead be removed
            if (manager.isBundle(base))
                continue;

            QDir dir(addDir);
            dir.cd("Packages"); // Automatically navigating into the Packages
                                // directory
            if (!dir.exists())
                exit(-1);

            QString path = dir.absoluteFilePath(base);
            inputArgs[i] = path.toUtf8().constData();
        }
    }

    if (install)
    {
        cout << "INFO: Installing package..." << endl;

        vector<int> indices;
        matchingPackages(packages, indices);

        if (indices.empty())
        {
            cout << "No matching packages found" << endl;
        }

        for (size_t i = 0; i < indices.size(); i++)
        {
            PackageManager::Package& p = packages[indices[i]];

            // Ensuring package not installed already
            if (p.installed)
            {
                cout << "WARNING: " << p.name << " is already installed" << endl;
            }

            else
            {
                cout << "INFO: installing " << p.file << endl;

                if (!manager.installPackage(p))
                {
                    cerr << "ERROR: failed to install " << p.file << endl;
                }
            }
        }
    }

    if (uninstall)
    {
        vector<int> indices;
        matchingPackages(packages, indices);

        if (indices.empty())
        {
            cout << "No matching packages found" << endl;
        }

        for (size_t i = 0; i < indices.size(); i++)
        {
            PackageManager::Package& p = packages[indices[i]];

            if (p.installed)
            {
                manager.uninstallPackage(p);
            }
            else
            {
                cout << "INFO: " << p.name << " is not installed" << endl;
            }
        }
    }

    if (remove)
    {
        vector<int> indices;
        matchingPackages(packages, indices);
        QStringList files;

        if (indices.empty())
        {
            cout << "No matching packages found" << endl;
        }

        for (size_t i = 0; i < indices.size(); i++)
        {
            PackageManager::Package& p = packages[indices[i]];
            files.push_back(p.file);
        }

        manager.removePackages(files);
    }

    if (update)
    {
        cout << "INFO: Updating package(s)..." << endl;

        // Store the input files and handle bundles
        QStringList newPackageFiles;
        for (size_t i = 0; i < inputArgs.size(); i++)
        {
            QFileInfo fileInfo(inputArgs[i].c_str());

            // Validate that it's a file, not a directory
            if (fileInfo.isDir())
            {
                cerr << "ERROR: -update expects package files, not directories: " << inputArgs[i] << endl;
                cerr << "Usage: rvpkg -update /path/to/package.rvpkg" << endl;
                exit(-1);
            }

            if (!fileInfo.exists())
            {
                cerr << "ERROR: file does not exist: " << inputArgs[i] << endl;
                continue;
            }

            // Validate file extension
            QString suffix = fileInfo.suffix().toLower();
            if (suffix != "rvpkg" && suffix != "rvpkgs" && suffix != "zip")
            {
                cerr << "ERROR: invalid package file (must be .rvpkg, .rvpkgs, or .zip): " << inputArgs[i] << endl;
                continue;
            }

            QString absolutePath = fileInfo.absoluteFilePath();

            // Check if it's a bundle
            if (manager.isBundle(absolutePath))
            {
                cout << "INFO: Bundle detected, unpacking to temporary location..." << endl;

                // Create a temporary directory for unpacking
                QDir tempDir = QDir::temp();
                QString tempBasePath =
                    tempDir.absoluteFilePath("rvpkg_update_temp_" + QString::number(QDateTime::currentMSecsSinceEpoch()));
                QString outputDir = tempBasePath + "/Packages";

                // Create the full directory path
                if (!QDir().mkpath(outputDir))
                {
                    cerr << "ERROR: Failed to create temporary directory: " << outputDir.toUtf8().constData() << endl;
                    continue;
                }

                cout << "INFO: Extracting to: " << outputDir.toUtf8().constData() << endl;

                // Unpack the bundle (pass the full path including /Packages)
                std::vector<QString> unpackedPackages = manager.handleBundle(absolutePath, outputDir);

                if (unpackedPackages.size() > 0)
                {
                    cout << "INFO: Bundle unpacked successfully, found " << unpackedPackages.size() << " package(s)" << endl;
                    for (const QString& pkg : unpackedPackages)
                    {
                        cout << "INFO: - " << pkg.toStdString() << endl;
                        newPackageFiles.push_back(pkg);
                    }
                }
                else
                {
                    cerr << "ERROR: Failed to unpack bundle: " << absolutePath.toUtf8().constData() << endl;
                }
            }
            else
            {
                newPackageFiles.push_back(absolutePath);
            }
        }

        if (newPackageFiles.empty())
        {
            cerr << "ERROR: No valid package files provided" << endl;
            exit(-1);
        }

        // Create a temporary PackageManager to read new package info
        PackageManager tempManager;
        for (const QString& file : newPackageFiles)
        {
            tempManager.loadPackageInfo(file);
        }
        PackageManager::PackageList& newPackages = tempManager.packageList();

        if (newPackages.empty())
        {
            cerr << "ERROR: Failed to read package information from provided files" << endl;
            exit(-1);
        }

        // For each new package, find and handle the old version
        for (size_t i = 0; i < newPackages.size(); i++)
        {
            const PackageManager::Package& newPkg = newPackages[i];
            cout << endl << "INFO: ========================================" << endl;
            cout << "INFO: Processing update for package: " << newPkg.name << " (new version: " << newPkg.version << ")" << endl;

            // Find the existing installed package with the same name
            PackageManager::Package* oldPkg = nullptr;
            for (size_t j = 0; j < packages.size(); j++)
            {
                if (packages[j].name == newPkg.name)
                {
                    oldPkg = &packages[j];
                    break;
                }
            }

            QString targetDir;

            if (oldPkg)
            {
                cout << "INFO: Found existing package: " << oldPkg->name << " (version: " << oldPkg->version << ")" << endl;

                // Determine target directory from old package
                QFileInfo oldFileInfo(oldPkg->file);
                QDir oldDir = oldFileInfo.absoluteDir();
                targetDir = oldDir.absolutePath();

                // Skipping dependency check during uninstall/removal since the new package will be installed in the same directory
                manager.setSkipDependencyCheck();

                // Uninstall the old package if it's installed
                if (oldPkg->installed)
                {
                    cout << "INFO: Uninstalling old version..." << endl;
                    manager.uninstallPackage(*oldPkg);
                }

                // Remove the old package file
                cout << "INFO: Removing old package file..." << endl;
                QStringList filesToRemove;
                filesToRemove.push_back(oldPkg->file);
                manager.removePackages(filesToRemove);
            }
            else
            {
                cout << "INFO: No existing package found with name: " << newPkg.name << " (will perform fresh install)" << endl;

                // Use the first writable support path
                TwkApp::Bundle::PathVector paths =
                    BundleType(APPNAME, MAJOR_VERSION, MINOR_VERSION, REVISION_NUMBER).pluginPath("Packages");
                for (size_t j = 0; j < paths.size(); j++)
                {
                    QFileInfo pathInfo(paths[j].c_str());
                    if (pathInfo.isWritable())
                    {
                        targetDir = paths[j].c_str();
                        break;
                    }
                }
            }

            if (targetDir.isEmpty())
            {
                cerr << "ERROR: Could not find writable directory to install package" << endl;
                continue;
            }

            cout << "INFO: Installing new version to: " << targetDir.toUtf8().constData() << endl;

            // Add the new package
            QStringList filesToAdd;
            filesToAdd.push_back(newPackageFiles[i]);
            if (!manager.addPackages(filesToAdd, targetDir.toUtf8().constData()))
            {
                cerr << "ERROR: Failed to add new package" << endl;
                continue;
            }

            // Reload packages to get the newly added package
            manager.loadPackages();
            PackageManager::PackageList& updatedPackages = manager.packageList();

            // Find the newly added package and install it
            bool installed = false;
            for (size_t j = 0; j < updatedPackages.size(); j++)
            {
                PackageManager::Package& p = updatedPackages[j];
                if (p.name == newPkg.name && p.version == newPkg.version && !p.installed)
                {
                    cout << "INFO: Installing new version..." << endl;
                    if (!manager.installPackage(p))
                    {
                        cerr << "ERROR: Failed to install new package version" << endl;
                    }
                    else
                    {
                        cout << "INFO: Successfully updated " << p.name << " to version " << p.version << endl;
                        installed = true;
                    }
                    break;
                }
            }

            if (!installed)
            {
                // The package might already be installed if it's the same version
                for (size_t j = 0; j < updatedPackages.size(); j++)
                {
                    PackageManager::Package& p = updatedPackages[j];
                    if (p.name == newPkg.name && p.version == newPkg.version && p.installed)
                    {
                        cout << "INFO: Package " << p.name << " version " << p.version << " is already installed" << endl;
                        break;
                    }
                }
            }
        }

        cout << endl << "INFO: ========================================" << endl;
        cout << "INFO: Update process completed" << endl;
    }

    return 0;
}
