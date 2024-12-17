//
//  Copyright (c) 2010 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <RvPackage/PackageManager.h>
#include <TwkDeploy/Deploy.h>
#include <TwkUtil/File.h>
#include <unzip.h>
#include <yaml.h>
#include <TwkApp/Bundle.h>
#include <TwkUtil/File.h>
#include <TwkQtCoreUtil/QtConvert.h>
#include <stl_ext/string_algo.h>
#include <fstream>
#include <string_view>
#include <iostream>

namespace Rv
{
    using namespace std;

    QString extractFile(unzFile uf, const string& outputPath)
    {
        char filename[256];
        unz_file_info fileInfo;

        if (unzGetCurrentFileInfo(uf, &fileInfo, filename, sizeof(filename),
                                  nullptr, 0, nullptr, 0)
            != UNZ_OK)
        {
            cerr << "ERROR: Unable to get file info" << endl;
            return "";
        }

        if (unzOpenCurrentFile(uf) != UNZ_OK)
        {
            cerr << "ERROR: Unable to open file in zip" << endl;
            return "";
        }

        string fullPath = outputPath + "/" + filename;
        FILE* outFile = std::fopen(fullPath.c_str(), "wb");
        if (outFile == nullptr)
        {
            cerr << "ERROR: Unable to open output file" << endl;
            unzCloseCurrentFile(uf);
            return "";
        }

        char buffer[8192];
        int bytesRead = 0;
        while ((bytesRead = unzReadCurrentFile(uf, buffer, sizeof(buffer))) > 0)
        {
            fwrite(buffer, 1, bytesRead, outFile);
        }

        fclose(outFile);
        unzCloseCurrentFile(uf);

        return QString::fromStdString(fullPath);
    }

    vector<QString> unzipBundle(const string& zipFilePath,
                                const string& outputPath)
    {
        vector<QString> includedPackages;

        // Opening zip file
        unzFile uf = unzOpen(zipFilePath.c_str());
        if (uf == nullptr)
        {
            cerr << "ERROR: Unable to open zip file" << endl;
            return {};
        }

        // Checking for the presence of files
        if (unzGoToFirstFile(uf) != UNZ_OK)
        {
            cerr << "ERROR: Unable to find first file in zip" << endl;
            unzClose(uf);
            return {};
        }

        // Extracting each file in the zip
        do
        {
            QString extractedFilePath = extractFile(uf, outputPath);
            if (extractedFilePath == "")
            {
                cerr << "ERROR: Unable to extract zip file" << endl;
            }
            else
            {
                includedPackages.push_back(extractedFilePath);
            }
        } while (unzGoToNextFile(uf) == UNZ_OK);

        unzClose(uf);

        return includedPackages;
    }

    bool PackageManager::isBundle(const QString& infileNonCanonical)
    {

        // Getting package file extension
        string filePath = infileNonCanonical.toStdString();
        size_t dot = filePath.rfind('.');
        if (dot == string::npos)
        {
            cerr << "ERROR: Unsupported file type. Ensure each package has the "
                    "extension 'rvpkg', 'zip' or 'rvpkgs'"
                 << endl;
            return false;
        }

        // Edge case, last character is dot
        if (dot == filePath.size() - 1)
        {
            cerr << "ERROR: File paths cannot end with a dot/period. Extension "
                    "cannot be determined."
                 << endl;
            return false;
        }

        // File extension
        string ext = filePath.substr(dot + 1);
        if (ext == "rvpkgs")
        {
            return true;

            // May or may not be a bundle
        }
        else if (ext == "zip")
        {

            unzFile uf = unzOpen(filePath.c_str());
            if (!uf)
            {
                cerr << "ERROR: Failed to open zip file" << endl;
                return false;
            }

            // Checking for presence of PACKAGE file
            if (unzGoToFirstFile(uf) == UNZ_OK)
            {
                do
                {
                    unz_file_info fileInfo;
                    char fileName[256];
                    if (unzGetCurrentFileInfo(uf, &fileInfo, fileName,
                                              sizeof(fileName), nullptr, 0,
                                              nullptr, 0)
                            == UNZ_OK
                        && strcmp(fileName, "PACKAGE") == 0)
                    {
                        return false;
                    }
                } while (unzGoToNextFile(uf) == UNZ_OK);
            }

            unzClose(uf);
            return true; // Zip does not contain a PACKAGE file, therefore
                         // assume it is a bundle
        }

        return false; // Extension is not zip or rvpkgs
    }

    vector<QString> PackageManager::handleBundle(const QString& bundlePath,
                                                 const QString& outputPath)
    {

        // Attempting to unzip bundle into the Packages directory
        vector<QString> includedPackages =
            unzipBundle(bundlePath.toStdString(), outputPath.toStdString());
        if (includedPackages.size() == 0)
        {
            cerr << "ERROR: Unable to unzip bundle." << endl;
            return {};
        }

        return includedPackages;
    }

    bool PackageManager::m_ignorePrefs = false;

    PackageManager::PackageManager()
        : m_force(false)
    {
    }

    PackageManager::~PackageManager() {}

    int PackageManager::findPackageIndexByZip(const QString& zipfile)
    {
        QString zipfileCanonical = QFileInfo(zipfile).canonicalFilePath();
        QString pattern = zipfile + "-.*\\.rvpkg";
        QRegExp rx(pattern);

        for (size_t i = 0; i < m_packages.size(); i++)
        {
            QFileInfo info(m_packages[i].file);

            if (info.fileName() == zipfile || info.absoluteFilePath() == zipfile
                || info.absoluteFilePath() == zipfileCanonical
                || rx.exactMatch(info.fileName())
                || rx.exactMatch(info.absoluteFilePath()))
            {
                return i;
            }
        }

        return -1;
    }

    bool PackageManager::allowLoading(Package& package, bool allow, int depth)
    {
        if (package.loadable == allow)
            return true;

        if (allow)
        {
            QList<int> notloaded;

            for (int i = 0; i < package.uses.size(); i++)
            {
                int index = package.uses[i];
                if (!m_packages[index].loadable)
                    notloaded.push_back(index);
            }

            if (depth == 0 && notloaded.size())
            {
                QString s("Package requires these unloadable packages:\n");

                for (size_t q = 0; q < notloaded.size(); q++)
                {
                    s += m_packages[notloaded[q]].name;
                    s += "\n";
                }

                if (!fixLoadability(s))
                    return false;
            }

            package.loadable = true;

            for (size_t q = 0; q < notloaded.size(); q++)
            {
                if (!allowLoading(m_packages[notloaded[q]], allow, depth + 1))
                {
                    return false;
                }
            }

            m_doNotLoadPackages.removeOne(package.file);

            if (package.optional && !m_optLoadPackages.contains(package.file))
            {
                m_optLoadPackages.push_back(package.file);
            }
        }
        else
        {
            QList<int> loaded;

            for (int i = 0; i < package.usedBy.size(); i++)
            {
                int index = package.usedBy[i];
                if (m_packages[index].loadable)
                    loaded.push_back(index);
            }

            if (depth == 0 && loaded.size())
            {
                QString s("Packages that would be unloaded:\n");

                for (size_t q = 0; q < loaded.size(); q++)
                {
                    s += m_packages[loaded[q]].name;
                    s += "\n";
                }

                if (!fixUnloadability(s))
                    return false;
            }

            package.loadable = false;

            for (size_t q = 0; q < loaded.size(); q++)
            {
                if (!allowLoading(m_packages[loaded[q]], allow, depth + 1))
                {
                    return false;
                }
            }

            m_doNotLoadPackages.push_back(package.file);
            m_doNotLoadPackages.removeDuplicates();
            m_optLoadPackages.removeDuplicates();
            m_optLoadPackages.removeOne(package.file);
        }

        //
        //  Don't read/write settings unless this is the RV app (as opposed to
        //  rvpkg).
        //
        if (QCoreApplication::applicationName() == INTERNAL_APPLICATION_NAME)
        {
            RV_QSETTINGS;
            settings.beginGroup("ModeManager");
            settings.setValue("doNotLoadPackages",
                              swapAppDir(m_doNotLoadPackages, true));
            settings.setValue("optionalPackages",
                              swapAppDir(m_optLoadPackages, true));
            settings.endGroup();
        }

        return true;
    }

    static bool versionCompatible(QString version, int maj, int min, int rev)
    {
        if (version == "")
            return true;

        int cmajor = -1;
        int cminor = -1;
        int crev = -1;

        QStringList parts = version.split(".");

        if (parts.size() > 0)
            cmajor = parts[0].toUInt();
        if (parts.size() > 1)
            cminor = parts[1].toUInt();
        if (parts.size() > 2)
            crev = parts[2].toUInt();

        bool compatible = false;

        if (cmajor > -1)
        {
            compatible = cmajor <= maj;

            if (cmajor == maj && cminor > -1)
            {
                compatible = cminor <= min;

                if (cminor == min && crev > -1)
                {
                    compatible = crev <= rev;
                }
            }
        }

        return compatible;
    }

    bool PackageManager::installPackage(Package& package)
    {
        //
        //  Checked the dependancies first
        //

        if (package.installing)
            return true;

        QStringList deps = package.
                               requires
            .split(" ", QString::SkipEmptyParts);
        QStringList missing;
        QStringList notinstalled;
        QFileInfo info(package.file);
        QDir rdir = info.absoluteDir();
        rdir.cdUp();

        if (!makeSupportDirTree(rdir))
            return false;

        QDir mudir = rdir;
        QDir pydir = rdir;
        QDir supportdir = rdir;
        QDir imgdir = rdir;
        QDir movdir = rdir;
        QDir libdir = rdir;
        QDir nodedir = rdir;
        QDir profdir = rdir;

        m_doNotLoadPackages.removeOne(package.file);
        m_optLoadPackages.removeOne(package.file);

        mudir.cd("Mu");
        pydir.cd("Python");
        supportdir.cd("SupportFiles");
        imgdir.cd("ImageFormats");
        movdir.cd("MovieFormats");
        libdir.cd("lib");
        nodedir.cd("Nodes");
        profdir.cd("Profiles");

        QFileInfo pfile(package.file);
        QString pname = package.baseName;

        if (pname.isEmpty())
        {
            cerr << "ERROR: Illegal package file name: "
                 << package.file.toUtf8().data() << endl
                 << "       should be either <name>.zip or "
                    "<name>-<version>.rvpkg"
                 << endl;
            return false;
        }

        if (!supportdir.exists(pname))
            supportdir.mkdir(pname);
        supportdir.cd(pname);

        //
        //  Test to see if any of the package files are already there
        //

        QStringList existingFiles;

        for (size_t i = 0; i < package.files.size(); i++)
        {
            QString filename = package.files[i];
            string outfilename;
            int auxIndex = auxFileIndex(package, filename);

            // Once it is not an auxfile, the installer expects a flat list
            // structure Therefore, need to check for basename and not the file
            // name which can include directories
            QFileInfo fInfo(filename);
            QString baseName = fInfo.fileName();

            if (auxIndex != -1)
            {
                const AuxFile& a = package.auxFiles[auxIndex];
                QDir outdir(rdir.absoluteFilePath(
                    expandVarsInPath(package, a.location)));
                QFileInfo auxfileInfo(a.file);
                QString auxfileName = auxfileInfo.fileName();
                outfilename =
                    outdir.absoluteFilePath(auxfileName).toUtf8().constData();
            }
            else if (filename.endsWith(".mu") || filename.endsWith(".mud")
                     || filename.endsWith(".muc"))
            {
                outfilename =
                    mudir.absoluteFilePath(baseName).toUtf8().constData();
            }
            else if (filename.endsWith(".py") || filename.endsWith(".pyc")
                     || filename.endsWith(".pyo") || filename.endsWith(".pyd"))
            {
                outfilename =
                    pydir.absoluteFilePath(baseName).toUtf8().constData();
            }
            else if (filename.endsWith(".glsl") || filename.endsWith(".gto"))
            //
            //  Assume this is a NodeDefinition file (gto) or associated shader
            //  code.
            //
            {
                outfilename =
                    nodedir.absoluteFilePath(baseName).toUtf8().constData();
            }
            else if (filename.endsWith(".profile"))
            //
            //  Assume this is a Profile
            //
            {
                outfilename =
                    profdir.absoluteFilePath(baseName).toUtf8().constData();
            }

            QString n =
                QString::fromUtf8(outfilename.c_str(), outfilename.size());
            QFile file(n);

            if (file.exists())
                existingFiles.push_back(n);
        }

        if (existingFiles.size())
        {
            QString s("Package conflicts with these existing files:\n");

            for (size_t q = 0; q < existingFiles.size(); q++)
            {
                s += existingFiles[q];
                s += "\n";
            }

            if (!overwriteExistingFiles(s))
                return false;
        }

        //
        //  Check dependencies
        //

        for (size_t i = 0; i < deps.size(); i++)
        {
            QString dep = deps[i];
            int di = findPackageIndexByZip(dep);

            if (di == -1)
            {
                missing << dep;
            }
            else if (!m_packages[di].installed)
            {
                notinstalled << dep;
            }
        }

        if (missing.size())
        {
            //
            //  We don't have some packages
            //

            QString s("Package requires these missing packages:\n");

            for (size_t q = 0; q < missing.size(); q++)
            {
                s += missing[q];
                s += "\n";
            }

            errorMissingPackageDependancies(s);
            return false;
        }

        if (notinstalled.size())
        {
            //
            //  Required packages not installed but available
            //

            QString s("Package requires these uninstalled packages:\n");

            for (size_t q = 0; q < notinstalled.size(); q++)
            {
                s += notinstalled[q];
                s += "\n";
            }

            if (!installDependantPackages(s))
                return false;

            //
            //  Try and install them recursively
            //

            package.installing = true;

            for (size_t q = 0; q < notinstalled.size(); q++)
            {
                int di = findPackageIndexByZip(notinstalled[q]);

                if (!installPackage(m_packages[di]))
                {
                    break;
                }
            }

            package.installing = false;
        }

        //
        //  Loop over the files in the zip file
        //

        bool fbio = false;

        if (unzFile file = unzOpen(package.file.toUtf8().data()))
        {
            for (size_t i = 0; i < package.files.size(); i++)
            {
                QString filename = package.files[i];
                QFileInfo fInfo(filename);
                QString baseName = fInfo.fileName();
                vector<char> buffer(4096);

                if (unzLocateFile(file, filename.toUtf8().data(), 1) != UNZ_OK)
                {
                    cerr << "ERROR: reading zip file "
                         << package.file.toUtf8().data() << endl;
                    break;
                }

                unzOpenCurrentFile(file);

                string outfilename;
                int auxIndex = auxFileIndex(package, filename);

                if (auxIndex != -1)
                {
                    const AuxFile& a = package.auxFiles[auxIndex];
                    QDir outdir(rdir.absoluteFilePath(
                        expandVarsInPath(package, a.location)));
                    if (!outdir.exists())
                    {
                        bool success = outdir.mkpath(".");
                        if (!success)
                        {
                            cerr << "ERROR: Failed to create needed auxiliary "
                                    "directory: "
                                        + outdir.absolutePath().toStdString()
                                 << endl;
                        }
                    }
                    QFileInfo auxfileInfo(a.file);
                    QString auxfileName = auxfileInfo.fileName();
                    outfilename = outdir.absoluteFilePath(auxfileName)
                                      .toUtf8()
                                      .constData();
                }
                else if (filename.endsWith(".mu") || filename.endsWith(".mud")
                         || filename.endsWith(".muc"))
                {
                    outfilename =
                        mudir.absoluteFilePath(baseName).toUtf8().data();
                }
                else if (filename.endsWith(".py") || filename.endsWith(".pyc")
                         || filename.endsWith(".pyo")
                         || filename.endsWith(".pyd"))
                {
                    outfilename =
                        pydir.absoluteFilePath(baseName).toUtf8().data();
                }
                else if (filename.endsWith(".so") || filename.endsWith(".dll")
                         || filename.endsWith(".dylib"))
                {
                    if (package.imageio.contains(filename))
                    {
                        outfilename =
                            imgdir.absoluteFilePath(baseName).toUtf8().data();
                        fbio = true;
                    }
                    else if (package.movieio.contains(filename))
                    {
                        outfilename =
                            movdir.absoluteFilePath(baseName).toUtf8().data();
                    }
                    else
                    {
                        for (size_t q = 0; q < package.modes.size(); q++)
                        {
                            if (package.modes[q].file == filename)
                            {
                                outfilename = mudir.absoluteFilePath(baseName)
                                                  .toUtf8()
                                                  .data();
                            }
                        }
                    }

                    if (outfilename == "")
                    {
                        outfilename =
                            libdir.absoluteFilePath(baseName).toUtf8().data();
                    }
                }
                else if (filename.endsWith(".glsl")
                         || filename.endsWith(".gto"))
                //
                //  Assume this is a NodeDefinition file (gto) or associated
                //  shader code.
                //
                {
                    outfilename =
                        nodedir.absoluteFilePath(baseName).toUtf8().constData();
                }
                else if (filename.endsWith(".profile"))
                {
                    outfilename =
                        profdir.absoluteFilePath(baseName).toUtf8().constData();
                }
                else
                {
                    outfilename =
                        supportdir.absoluteFilePath(baseName).toUtf8().data();
                }

                ofstream outfile(UNICODE_C_STR(outfilename.c_str()),
                                 ios::binary);

                while (1)
                {
                    int read = unzReadCurrentFile(file, &buffer.front(),
                                                  buffer.size());
                    if (read == 0)
                        break;
                    else
                        outfile.write(&buffer.front(), read);
                }

                if (fbio)
                {
                    string makeFBIO =
                        TwkApp::Bundle::mainBundle()->executableFile(
                            "makeFBIOformats");
                    QFileInfo info(outfilename.c_str());
                    QString dir = info.dir().absolutePath();
                    string cmd = makeFBIO;
                    cmd += " ";
                    cmd += dir.toUtf8().constData();

                    if (system(cmd.c_str()) == -1)
                    {
                        cerr << "ERROR: executing command " << cmd << endl;
                    }

                    // QProcess process;
                    // QStringList args;
                    // args.push_back(dir);
                    // args.push_back(dir);
                    // process.start(makeFBIO.c_str(), args);
                    // process.waitForFinished();
                }
            }

            unzClose(file);
        }

        //
        //  Load the mode file (rvload) which needs to be updated
        //

        QString rvloadV1 = mudir.absoluteFilePath("rvload");
        ModeEntryList mlistV1 = loadModeFile(rvloadV1);
        QString rvloadV2 = mudir.absoluteFilePath("rvload2");
        ModeEntryList mlistV2 = loadModeFile(rvloadV2);

        for (size_t i = 0; i < package.modes.size(); i++)
        {
            const Mode& mode = package.modes[i];
            QString name = mode.file;
            if (name.endsWith(".mu") || name.endsWith(".py"))
                name.remove(name.size() - 3, 3);

            //
            //  Just as a sanity check -- if there's already an entry for this
            //  mode for some reason just delete the old one
            //

            for (int q = mlistV1.size() - 1; q >= 0; q--)
            {
                if (mlistV1[q].name == name)
                {
                    cerr << "WARNING: removing duplicate mode entry in "
                         << rvloadV1.toUtf8().data() << " for mode "
                         << name.toUtf8().data() << endl;

                    mlistV1.erase(mlistV1.begin() + q);
                }
            }
            for (int q = mlistV2.size() - 1; q >= 0; q--)
            {
                if (mlistV2[q].name == name)
                {
                    cerr << "WARNING: removing duplicate mode entry in "
                         << rvloadV2.toUtf8().data() << " for mode "
                         << name.toUtf8().data() << endl;

                    mlistV2.erase(mlistV2.begin() + q);
                }
            }

            QFileInfo pfile(package.file);

            ModeEntry entry;
            entry.name = name;
            entry.package = pfile.fileName();
            entry.menu = mode.menu;
            entry.shortcut = mode.shortcut;
            entry.event = mode.event;
            entry.
                requires
            = mode.
                  requires;
            entry.rvversion = package.rvversion;
            entry.openrvversion = package.openrvversion;
            entry.optional = package.optional;

            if (mode.load == "delay")
            {
                entry.loaded = false;
                entry.active = false;
            }
            else if (mode.load == "immediate")
            {
                entry.loaded = true;
                entry.active = true;
            }

            if (versionCompatible(entry.rvversion, 3, 7, 99))
                mlistV1.push_back(entry);
            else
                mlistV2.push_back(entry);
        }

        //
        //  Write the mode file back out
        //

        writeModeFile(rvloadV1, mlistV1, 1);
        writeModeFile(rvloadV2, mlistV2);
        package.installed = true;

        //
        //  Write the installation record
        //

        QFileInfo rinfo(package.file);
        writeInstallationFile(
            rinfo.absoluteDir().absoluteFilePath("rvinstall"));

        return true;
    }

    bool PackageManager::makeSupportDirTree(QDir& root)
    {
        const char* dirs[] = {"Mu",          "Python",       "SupportFiles",
                              "ConfigFiles", "ImageFormats", "MovieFormats",
                              "Packages",    "lib",          "libquicktime",
                              "Nodes",       "Profiles",     NULL};

        for (const char** d = dirs; *d; d++)
        {
            if (!root.exists(*d))
            {
                if (!root.mkdir(*d))
                {
                    // QMessageBox::critical(this,
                    //                       QString("Support Directory Creation
                    //                       Failed"), QString("Unable to create
                    //                       %1 in directory
                    //                       %2\n").arg(QString(*d)).arg(root.absolutePath()));
                    return false;
                }
            }
        }

        return true;
    }

    bool PackageManager::uninstallPackage(Package& package)
    {
        if (package.installing)
            return true;
        QFileInfo info(package.file);
        QDir rdir = info.absoluteDir();
        rdir.cdUp();
        QDir mudir = rdir;
        QDir pydir = rdir;
        QDir supportdir = rdir;
        QDir imgdir = rdir;
        QDir movdir = rdir;
        QDir libdir = rdir;
        QDir nodedir = rdir;
        QDir profdir = rdir;

        mudir.cd("Mu");
        pydir.cd("Python");
        supportdir.cd("SupportFiles");
        imgdir.cd("ImageFormats");
        movdir.cd("MovieFormats");
        libdir.cd("lib");
        nodedir.cd("Nodes");
        profdir.cd("Nodes");

        QFileInfo pfile(package.file);
        QRegExp rvpkgRE("(.*)-[0-9]+\\.[0-9]+\\.rvpkg");
        QRegExp zipRE("(.*)\\.zip");
        QRegExp rvpkgsRE("(.*)-[0-9]+\\.[0-9]+\\.rvpkgs");
        QString pname = package.baseName;

        if (!supportdir.exists(pname))
            supportdir.mkdir(pname);
        supportdir.cd(pname);

        QList<int> others;

        for (size_t i = 0; i < package.usedBy.size(); i++)
        {
            int index = package.usedBy[i];
            const Package& other = m_packages[index];
            if (other.installed)
                others.push_back(index);
        }

        if (others.size())
        {
            //
            //  Packages that depend on this one are installed
            //

            QString s("Package is used by these installed packages:\n");

            for (size_t q = 0; q < others.size(); q++)
            {
                s += m_packages[others[q]].file;
                s += "(";
                s += m_packages[others[q]].name;
                s += ")\n";
            }

            if (!uninstallDependantPackages(s))
                return false;
        }

        for (size_t i = 0; i < others.size(); i++)
        {
            uninstallPackage(m_packages[others[i]]);
        }

        //
        //  Loop over the files and remove them (if possible)
        //

        QStringList notremoved;

        for (size_t i = 0; i < package.files.size(); i++)
        {
            QString outfilename;
            QString filename = package.files[i];
            QFileInfo fInfo(filename);
            QString baseName = fInfo.fileName();

            QStringList auxfiles;
            int auxIndex = auxFileIndex(package, filename);

            if (auxIndex != -1)
            {
                const AuxFile& a = package.auxFiles[auxIndex];
                QDir outdir(rdir.absoluteFilePath(
                    expandVarsInPath(package, a.location)));
                QFileInfo auxfileInfo(a.file);
                QString auxfileName = auxfileInfo.fileName();
                outfilename =
                    outdir.absoluteFilePath(auxfileName).toUtf8().constData();
            }
            else if (filename.endsWith(".mu") || filename.endsWith(".mud")
                     || filename.endsWith(".muc"))
            {
                outfilename = mudir.absoluteFilePath(baseName);

                if (filename.endsWith(".mu"))
                {
                    baseName.chop(3);
                    auxfiles.push_back(
                        mudir.absoluteFilePath(baseName + QString(".muc")));
                    auxfiles.push_back(
                        mudir.absoluteFilePath(baseName + QString(".mud")));
                    auxfiles.push_back(
                        mudir.absoluteFilePath(baseName + QString(".so")));
                }
            }
            else if (filename.endsWith(".py") || filename.endsWith(".pyc")
                     || filename.endsWith(".pyo") || filename.endsWith(".pyd"))
            {
                outfilename = pydir.absoluteFilePath(baseName);

                if (filename.endsWith(".py"))
                {
                    baseName.chop(3);
                    auxfiles.push_back(
                        pydir.absoluteFilePath(baseName + QString(".pyc")));
                    auxfiles.push_back(
                        pydir.absoluteFilePath(baseName + QString(".pyd")));
                    auxfiles.push_back(
                        pydir.absoluteFilePath(baseName + QString(".pyo")));
                }
            }
            else if (filename.endsWith(".so") || filename.endsWith(".dll")
                     || filename.endsWith(".dylib"))
            {
                if (package.imageio.contains(filename))
                {
                    outfilename = imgdir.absoluteFilePath(baseName);
                }
                else if (package.movieio.contains(filename))
                {
                    outfilename = movdir.absoluteFilePath(baseName);
                }
                else
                {
                    for (size_t q = 0; q < package.modes.size(); q++)
                    {
                        if (package.modes[q].file == filename)
                        {
                            outfilename = mudir.absoluteFilePath(baseName);
                        }
                    }
                }

                if (outfilename == "")
                {
                    outfilename = libdir.absoluteFilePath(baseName);
                }
            }
            else if (filename.endsWith(".glsl") || filename.endsWith(".gto"))
            //
            //  Assume this is a NodeDefinition file (gto) or associated shader
            //  code.
            //
            {
                outfilename = nodedir.absoluteFilePath(baseName);
            }
            else if (filename.endsWith(".profile"))
            {
                outfilename = profdir.absoluteFilePath(baseName);
            }
            else
            {
                outfilename = supportdir.absoluteFilePath(baseName);
            }

            QFile file(outfilename);
            if (!file.remove())
                notremoved.push_back(outfilename);

            //
            //  Check for and remove any generated files (e.g. .pyc files
            //  created by python).
            //

            for (size_t i = 0; i < auxfiles.size(); i++)
            {
                QFile file(auxfiles[i]);
                if (file.exists())
                    file.remove();
            }
        }

        //
        // Remove the dangling supportdir
        //

        if (!supportdir.removeRecursively())
            notremoved.push_back(supportdir.absolutePath());

        //
        // Report what was unremovable
        //

        if (notremoved.size())
        {
            QString s("Files which could not be removed:\n");

            for (size_t q = 0; q < notremoved.size(); q++)
            {
                s += notremoved[q];
                s += "\n";
            }

            informCannotRemoveSomeFiles(s);
        }

        //
        //  Purge the mode entries of this package
        //

        QString rvloadV1 = mudir.absoluteFilePath("rvload");
        ModeEntryList mlistV1 = loadModeFile(rvloadV1);
        QString rvloadV2 = mudir.absoluteFilePath("rvload2");
        ModeEntryList mlistV2 = loadModeFile(rvloadV2);

        QFileInfo finfo(package.file);
        QString packageZip = finfo.fileName();

        for (int q = mlistV1.size() - 1; q >= 0; q--)
        {
            if (mlistV1[q].package == packageZip)
            {
                mlistV1.erase(mlistV1.begin() + q);
            }
        }
        for (int q = mlistV2.size() - 1; q >= 0; q--)
        {
            if (mlistV2[q].package == packageZip)
            {
                mlistV2.erase(mlistV2.begin() + q);
            }
        }

        writeModeFile(rvloadV1, mlistV1, 1);
        writeModeFile(rvloadV2, mlistV2);

        //
        //  Update the install file
        //

        package.installed = false;
        QFileInfo rinfo(package.file);
        writeInstallationFile(
            rinfo.absoluteDir().absoluteFilePath("rvinstall"));

        return true;
    }

    // Called on every package on Preference menu load and everytime a new
    // package or bundle is added
    void PackageManager::loadPackageInfo(const QString& infileNonCanonical)
    {

        if (findPackageIndexByZip(infileNonCanonical) == -1)
        {
            //
            // Under Windows, using canonical file pathing
            // for unzOpen can sometimes fail if the path
            // contains a symlink to a UNC mapped dir.
            // e.g. 'rvpkgs -> \\server\plugins\rvpkgs'.
            // So if unzOpen is unsuccessful using the canonical
            // file path we try again with the original
            // non-canonical absolute file path.

            // NB: The file path we use for m_packages
            // i.e. 'infile' should be the file path that was
            // successful for unzOpen().
            //
            QString infile = QFileInfo(infileNonCanonical).canonicalFilePath();
            unzFile file = unzOpen(infile.toUtf8().data());
            if (!file)
            {
                infile = QFileInfo(infileNonCanonical).absoluteFilePath();
                file = unzOpen(infile.toUtf8().data());
            }

            if (file)
            {
                vector<char> buffer;
                m_packages.push_back(Package());
                m_packages.back().file = infile;
                m_packageMap.insert(infile, m_packages.size() - 1);

                QFileInfo finfo(infile);

                m_packages.back().fileWritable = TwkUtil::isWritable(
                    TwkQtCoreUtil::UTF8::qconvert(infile).c_str());
                m_packages.back().dirWritable = TwkUtil::isWritable(
                    TwkQtCoreUtil::UTF8::qconvert(finfo.absolutePath())
                        .c_str());

                unz_global_info info;
                unzGetGlobalInfo(file, &info);

                unzLocateFile(file, "PACKAGE", 1);
                unzOpenCurrentFile(file);

                unzGoToFirstFile(file);

                QStringList files;

                do
                {
                    // unz_file_info info;
                    char temp[256];
                    unzGetCurrentFileInfo(file, NULL /*&info*/, temp, 256, NULL,
                                          0, NULL, 0);
                    temp[255] = 0;
                    files.push_back(temp);
                    QString f = temp;

                    // Check if it's a directory
                    QFileInfo fInfo(f);
                    if (!finfo.isDir())
                    {
                        if (f != "PACKAGE")
                            m_packages.back().files.push_back(f);
                    }
                } while (unzGoToNextFile(file) == UNZ_OK);

                while (!unzeof(file))
                {
                    buffer.resize(buffer.size() + 256);
                    int read =
                        unzReadCurrentFile(file, &buffer.back() - 255, 256);
                    if (read != 256)
                        buffer.resize(buffer.size() - 256 + read);
                }

                unzClose(file);

                if (!buffer.empty())
                {
                    yaml_parser_t parser;
                    yaml_event_t input_event;
                    memset(&parser, 0, sizeof(parser));
                    memset(&input_event, 0, sizeof(input_event));

                    if (!yaml_parser_initialize(&parser))
                    {
                        cerr << "ERROR: Could not initialize the YAML parser "
                                "object"
                             << endl;
                        return;
                    }

                    yaml_parser_set_input_string(
                        &parser, (const unsigned char*)&buffer.front(),
                        buffer.size());

                    Package& package = m_packages.back();
                    package.installed = false;
                    package.hidden = false;
                    package.system = false;
                    package.optional = false;
                    package.openrvversion = "1.0.0";
                    bool modeState = false;
                    bool auxFileState = false;
                    bool auxFolderState = false;
                    bool valueState = false;
                    QString pname;
                    QString auxFile;
                    QString auxFolder;

                    for (bool done = false; !done;)
                    {
                        if (!yaml_parser_parse(&parser, &input_event))
                        {
                            cerr << "ERROR: YAML parser failed on PACKAGE file "
                                    "in "
                                 << infile.toStdString() << endl;
                            break;
                        }

                        if (input_event.type == YAML_STREAM_END_EVENT)
                            done = true;

                        switch (input_event.type)
                        {
                        case YAML_STREAM_START_EVENT:
                        case YAML_STREAM_END_EVENT:
                        case YAML_DOCUMENT_START_EVENT:
                        case YAML_DOCUMENT_END_EVENT:
                        case YAML_ALIAS_EVENT:
                            break;
                        case YAML_SEQUENCE_START_EVENT:
                            // cout << "seq start" << endl;
                            break;
                        case YAML_SEQUENCE_END_EVENT:
                            // cout << "seq end" << endl;
                            if (modeState)
                                modeState = false;
                            if (auxFileState)
                                auxFileState = false;
                            if (auxFolderState)
                                auxFolderState = false;
                            break;
                        case YAML_MAPPING_START_EVENT:
                            // cout << "map start" << endl;
                            if (modeState)
                                package.modes.push_back(Mode());
                            else if (auxFileState)
                            {
                                package.auxFiles.push_back(AuxFile());
                                package.auxFiles.back().file = auxFile;
                            }
                            else if (auxFolderState)
                            {
                                package.auxFolders.push_back(AuxFolder());
                                package.auxFolders.back().folder = auxFolder;
                            }
                            break;
                        case YAML_MAPPING_END_EVENT:
                            // cout << "map end" << endl;
                            break;
                        case YAML_SCALAR_EVENT:
                        {
                            QString v = QString::fromUtf8(
                                (const char*)input_event.data.scalar.value);
                            // cout << "scalar " << v.toUtf8().data() << endl;

                            if (!valueState)
                            {
                                pname = v;

                                if (v == "modes")
                                    modeState = true;
                                else if (v == "files")
                                    auxFileState = true;
                                else if (v == "folders")
                                    auxFolderState = true;
                                else
                                    valueState = true;
                            }
                            else if (modeState)
                            {
                                if (package.modes.size())
                                {
                                    Mode& m = package.modes.back();

                                    if (pname == "file")
                                        m.file = v;
                                    else if (pname == "menu")
                                        m.menu = v;
                                    else if (pname == "shortcut")
                                        m.shortcut = v;
                                    else if (pname == "event")
                                        m.event = v;
                                    else if (pname == "load")
                                        m.load = v;
                                    else if (pname == "icon")
                                        m.icon = v;
                                    else if (pname == "requires")
                                    m.
                                        requires
                                    = v.split(" ");
                                }

                                valueState = false;
                            }
                            else if (auxFileState)
                            {
                                if (package.auxFiles.size())
                                {
                                    AuxFile& a = package.auxFiles.back();
                                    if (pname == "file")
                                        a.file = v;
                                    else if (pname == "location")
                                        a.location = v;
                                }

                                valueState = false;
                            }
                            else if (auxFolderState)
                            {
                                if (package.auxFolders.size())
                                {
                                    AuxFolder& a = package.auxFolders.back();
                                    if (pname == "folder")
                                        a.folder = v;
                                    else if (pname == "location")
                                        a.location = v;
                                }

                                valueState = false;
                            }
                            else
                            {
                                if (pname == "package")
                                    package.name = v;
                                else if (pname == "url")
                                    package.url = v;
                                else if (pname == "icon")
                                    package.icon = v;
                                else if (pname == "description")
                                    package.description = v;
                                else if (pname == "author")
                                    package.author = v;
                                else if (pname == "organization")
                                    package.organization = v;
                                else if (pname == "contact")
                                    package.contact = v;
                                else if (pname == "version")
                                    package.version = v;
                                else if (pname == "requires")
                                package.
                                    requires
                                = v;
                                else if (pname == "rv") package.rvversion = v;
                                else if (pname == "openrv")
                                    package.openrvversion = v;
                                else if (pname == "imageio") package.imageio =
                                    v.split(" ");
                                else if (pname == "movieio") package.movieio =
                                    v.split(" ");
                                else if (pname == "hidden") package.hidden =
                                    v == "true";
                                else if (pname == "system") package.system =
                                    v == "true";
                                else if (pname == "optional") package.optional =
                                    v == "true";
                                valueState = false;
                            }
                        }
                        break;
                        default:
                            break;
                        }
                    }

                    static const bool isOpenRV =
                        std::string_view(INTERNAL_APPLICATION_NAME) == "OpenRV";

                    package.compatible = versionCompatible(
                        isOpenRV ? package.openrvversion : package.rvversion,
                        TWK_DEPLOY_MAJOR_VERSION(), TWK_DEPLOY_MINOR_VERSION(),
                        TWK_DEPLOY_PATCH_LEVEL());

                    package.zipFile = true;
                    package.installing = false;
                    yaml_event_delete(&input_event);
                    yaml_parser_delete(&parser);

                    for (auto& auxFolder : package.auxFolders)
                    {
                        // Iterate through files and compare paths
                        std::for_each(
                            package.files.begin(), package.files.end(),
                            [&](const QString& fullPath)
                            {
                                // Check if the file is in the current folder or
                                // any of its subdirectories
                                if (fullPath.startsWith(auxFolder.folder + '/'))
                                {
                                    // Separate the filename and directory
                                    // structure information
                                    QFileInfo fileInfo(fullPath);
                                    QString fileName = fileInfo.fileName();
                                    QString directory = fileInfo.dir().path();

                                    AuxFile newAuxFile;
                                    newAuxFile.file = fullPath;
                                    newAuxFile.location =
                                        auxFolder.location + "/" + directory;
                                    package.auxFiles.push_back(newAuxFile);
                                }
                            });
                    }

                    QRegExp rvpkgRE("(.*)-[0-9]+\\.[0-9]+\\.rvpkg");
                    QRegExp zipRE("(.*)\\.zip");

                    if (rvpkgRE.exactMatch(finfo.fileName()))
                    {
                        pname = rvpkgRE.capturedTexts()[1];
                    }
                    else if (zipRE.exactMatch(finfo.fileName()))
                    {
                        pname = zipRE.capturedTexts()[1];
                    }

                    package.baseName = pname;
                }
            }
        }
    }

    PackageManager::ModeEntryList
    PackageManager::loadModeFile(const QString& filename)
    {
        QFile file(filename);
        bool exists = file.exists();
        bool ok = file.open(QIODevice::ReadOnly);
        bool first = true;

        ModeEntryList list;
        int rvloadVersion = 0;

        while (ok && !file.atEnd())
        {
            QByteArray barray = file.readLine().trimmed();
            barray.push_back(char(0));
            QString line = QString::fromUtf8(barray.constData());

            if (first)
            {
                rvloadVersion = line.toInt();
                // if (rvloadVersion >= 4) break;
                first = false;
            }
            else
            {
                QStringList parts = line.split(",", QString::KeepEmptyParts);

                if (parts.size() >= 7 && parts[0] != "#")
                {
                    int index = 0;

                    ModeEntry entry;
                    entry.name = parts[index++];
                    entry.package = parts[index++];
                    entry.menu = parts[index++];
                    entry.shortcut = parts[index++];
                    entry.event = parts[index++];
                    entry.loaded = parts[index++] == "true";
                    entry.active = parts[index++] == "true";
                    entry.rvversion =
                        (rvloadVersion == 1) ? "" : parts[index++];
                    entry.optional = (rvloadVersion >= 3 && parts.size() >= 9
                                          ? parts[index++] == "true"
                                          : false);
                    entry.openrvversion =
                        (rvloadVersion >= 4 && parts.size() >= 10
                             ? parts[index++]
                             : "");

                    int requiresIndex = index;
                    for (int i = requiresIndex; i < parts.size(); i++)
                    entry.
                        requires
                        .push_back(parts[i]);

                    list.push_back(entry);
                }
            }
        }

        return list;
    }

    void PackageManager::writeModeFile(const QString& filename,
                                       const ModeEntryList& list, int version)
    {
        QFile file(filename);

        if (file.open(QIODevice::WriteOnly))
        {
            file.write((version == 1) ? "1\n" : "4\n", 2);

            for (size_t i = 0; i < list.size(); i++)
            {
                const ModeEntry& e = list[i];

                QString line =
                    QString("%1,%2,%3,%4,%5,%6,%7")
                        .arg(e.name)
                        .arg(e.package)
                        .arg(e.menu == "" ? QString("nil") : e.menu)
                        .arg(e.shortcut == "" ? QString("nil") : e.shortcut)
                        .arg(e.event == "" ? QString("nil") : e.event)
                        .arg(e.loaded ? QString("true") : QString("false"))
                        .arg(e.active ? QString("true") : QString("false"));

                if (version > 1 || version == 0)
                {
                    line += QString(",") + e.rvversion;
                }

                if (version >= 3 || version == 0)
                {
                    line += QString(e.optional ? ",true" : ",false");
                }

                if (version >= 4 || version == 0)
                {
                    line += QString(",") + e.openrvversion;
                }

                if (!e.requires.empty())
                {
                    for (int q = 0; q < e.requires.size(); q++)
                    {
                        line += QString(",%1").arg(e.requires[q]);
                    }
                }

                line += "\n";

                QByteArray a = line.toUtf8();
                file.write(a.data(), a.size());
            }
        }
        else
        {
            errorModeFileWriteFailed(filename);
        }
    }

    void PackageManager::loadInstalltionFile(const QDir& dir,
                                             const QString& filename)
    {
        QFile file(filename);
        bool exists = file.exists();
        bool ok = file.open(QIODevice::ReadOnly);

        while (ok && !file.atEnd())
        {
            QString line(file.readLine().trimmed());
            bool installed = line.size() > 1 && line[0] == '*';
            if (installed)
                line.remove(0, 1);
            QString path =
                QFileInfo(dir.absoluteFilePath(line)).canonicalFilePath();
            QMap<QString, int>::iterator i = m_packageMap.find(path);

            if (i == m_packageMap.end())
            {
                // Now try with the nonCanonicalPath for dir; This is is
                // really to handle Windows symlinks with UNC pathing.
                path = dir.absoluteFilePath(line);
                i = m_packageMap.find(path);
                if (i == m_packageMap.end())
                {
                    m_packages.push_back(Package());
                    m_packages.back().file = line;
                    m_packages.back().installed = installed;
                    m_packages.back().zipFile = false;
                    m_packages.back().name = "Missing Package";
                    m_packages.back().description =
                        "<p><i>The original zip file for this package is "
                        "missing</i></p>";
                    m_packageMap.insert(line, m_packages.size() - 1);
                }
                else
                {
                    m_packages[i.value()].installed = installed;
                }
            }
            else
            {
                m_packages[i.value()].installed = installed;
            }
        }
    }

    void PackageManager::writeInstallationFile(const QString& filename)
    {
        QFileInfo fileinfo(filename);
        QDir filedir = fileinfo.absoluteDir();
        QFile file(filename);

        if (file.open(QIODevice::WriteOnly))
        {
            for (size_t i = 0; i < m_packages.size(); i++)
            {
                const Package& package = m_packages[i];

                QFileInfo info(package.file);

                if (info.absoluteDir() == filedir)
                {
                    if (package.installed)
                        file.write("*", 1);
                    QByteArray n = info.fileName().toUtf8();
                    file.write(n.data(), n.size());
                    file.write("\n", 1);
                }
            }
        }
    }

    void PackageManager::findPackageDependencies()
    {
        for (size_t i = 0; i < m_packages.size(); i++)
        {
            Package& package = m_packages[i];
            QStringList deps = package.
                                   requires
                .split(" ", QString::SkipEmptyParts);

            for (size_t q = 0; q < deps.size(); q++)
            {
                int di = findPackageIndexByZip(deps[q]);

                if (di != -1)
                {
                    m_packages[di].usedBy.push_back(i);
                    package.uses.push_back(di);
                    if (!m_packages[di].compatible)
                        package.compatible = false;
                }
            }
        }
    }

    //
    //  Swap $APPLICATION_DIR in for the actual (canonical) directory if it
    //  appears at the head of the package file path.  This is to enable a
    //  optional package that was switched "on" to be "remembered" across
    //  use of different versions of RV (stored in different directories).
    //

    QStringList PackageManager::swapAppDir(const QStringList& packages,
                                           bool swapIn)
    {
        QString appDirSymbol("$APPLICATION_DIR");
        QFileInfo topInfo(TwkApp::Bundle::mainBundle()->top().c_str());
        QString top = topInfo.canonicalFilePath();
        QStringList ret;

        for (int i = 0; i < packages.size(); ++i)
        {
            QString path = packages[i];

            if (swapIn)
            {
                QFileInfo info(path);

                if (info.exists())
                    path = info.canonicalFilePath();
                if (path.startsWith(top))
                    path.replace(top, appDirSymbol);
            }
            else
            {
                if (path.startsWith(appDirSymbol))
                    path.replace(appDirSymbol, top);
            }

            ret.push_back(path);
        }

        return ret;
    }

    void PackageManager::loadPackages()
    {
        typedef TwkApp::Bundle Bundle;
        Bundle* bundle = Bundle::mainBundle();
        Bundle::PathVector paths = bundle->pluginPath("Packages");

        m_packages.clear();

        //
        //  Don't read/write settings unless this is the RV app (as opposed to
        //  rvpkg).
        //
        if (QCoreApplication::applicationName() == INTERNAL_APPLICATION_NAME)
        {
            RV_QSETTINGS;
            settings.beginGroup("ModeManager");
            m_doNotLoadPackages =
                swapAppDir(settings.value("doNotLoadPackages", QStringList())
                               .toStringList(),
                           false);
            m_optLoadPackages =
                swapAppDir(settings.value("optionalPackages", QStringList())
                               .toStringList(),
                           false);
            settings.endGroup();
        }

        m_doNotLoadPackages.removeDuplicates();
        m_optLoadPackages.removeDuplicates();

        for (size_t i = 0; i < paths.size(); i++)
        {
            QDir dir(paths[i].c_str());

            if (dir.exists())
            {
                QFileInfoList entries = dir.entryInfoList(QDir::Files);

                for (size_t q = 0; q < entries.size(); q++)
                {
                    if (entries[q].fileName().endsWith(".zip")
                        || entries[q].fileName().endsWith("rvpkg")
                        || entries[q].fileName().endsWith("rvpkgs"))
                    {
                        loadPackageInfo(entries[q].filePath());
                    }
                }

                //
                //  Load the installation file
                //

                if (dir.exists("rvinstall"))
                {
                    loadInstalltionFile(dir, dir.absoluteFilePath("rvinstall"));
                }
            }
        }

        //
        //  Check for optional packages that have been opted into
        //

        paths = bundle->pluginPath("Mu");

        for (size_t i = 0; i < paths.size(); i++)
        {
            QDir dir(paths[i].c_str());

            if (dir.exists())
            {
                if (dir.exists("rvload2"))
                {
                    PackageManager::ModeEntryList entries =
                        loadModeFile(dir.absoluteFilePath("rvload2"));

                    for (size_t i = 0; i < entries.size(); i++)
                    {
                        PackageManager::ModeEntry& entry = entries[i];

                        if (entry.optional == false)
                        {
                            for (size_t i = 0; i < m_packages.size(); i++)
                            {
                                if (QFileInfo(m_packages[i].file).fileName()
                                        == entry.package
                                    && m_packages[i].optional)
                                //
                                //  Then this package has been "opted in" so
                                //  reset optional flag
                                //
                                {
                                    m_packages[i].optional = false;
                                }
                            }
                        }
                    }
                }
            }
        }

        findPackageDependencies();

        for (size_t i = 0; i < m_packages.size(); i++)
        {
            m_packages[i].loadable =
                !m_doNotLoadPackages.contains(m_packages[i].file);

            if (m_packages[i].optional)
            {
                m_packages[i].loadable =
                    m_optLoadPackages.contains(m_packages[i].file);
            }

            declarePackage(m_packages[i], i);
        }
    }

    bool PackageManager::addPackages(const QStringList& files,
                                     const QString& path)
    {
        //
        //  First check that package files exist and have legal names
        //

        QRegExp rvpkgRE("(.*)-[0-9]+\\.[0-9]+\\.rvpkg");
        QRegExp rvpkgsRE("(.*)-[0-9]+\\.[0-9]+\\.rvpkgs"); // Bundle
        QRegExp zipRE("(.*)\\.zip");

        for (size_t i = 0; i < files.size(); i++)
        {

            // Unpacking any bundles
            if (isBundle(files[i]))
            {
                cout << "INFO: Bundle detected, adding subpackages now."
                     << endl;

                // Installing bundle
                vector<QString> includedPackages = handleBundle(files[i], path);
                if (includedPackages.size() > 0)
                {
                    cout << "INFO: Bundle subpackages added." << endl;
                }
                else
                {
                    cerr << "ERROR: Unable to add subpackages." << endl;
                }

                continue; // Bundle itself does not need to be copied
            };

            QFileInfo info(files[i]);

            if (!info.exists())
            {
                QString t("This package file does not exist: ");
                t += files[i];
                informPackageFailedToCopy(t);
                return false;
            }

            if (!rvpkgRE.exactMatch(info.fileName())
                && !zipRE.exactMatch(info.fileName())
                && !rvpkgsRE.exactMatch(info.fileName()))
            {
                QString t("ERROR: Illegal package file name: ");
                t += info.fileName();
                t += ", should be either <name>.zip, \n<name>-<version>.rvpkg "
                     "or <name>-<version>.rvpkgs";
                informPackageFailedToCopy(t);
                return false;
            }
        }

        QDir dir(path);
        if (dir.dirName() == "Packages")
            dir.cdUp();

        if (!dir.exists())
        {
            cerr << "ERROR: target support directory "
                 << dir.absolutePath().toUtf8().constData()
                 << " does not exist: please create it first" << endl;
            return false;
        }

        makeSupportDirTree(dir);
        dir.cd("Packages"); // User should not include the Packages directory in
                            // their add directory argument
        QStringList nocopy;

        for (size_t i = 0; i < files.size(); i++)
        {

            // Bundles do not need to be copied
            if (isBundle(files[i]))
                continue;

            QFileInfo info(files[i]);
            QString fromFile(files[i]);
            QString toFile(dir.absoluteFilePath(info.fileName()));

            QFileInfo toinfo(toFile);

            if (toinfo.exists() && m_force)
            {
                QStringList files;
                files.push_back(toFile);
                removePackages(files);
            }

            if (!QFile::copy(fromFile, toFile))
            {
                nocopy.push_back(fromFile);
            }
        }

        if (nocopy.size())
        {
            QString t("The following files failed to copy:\n");
            for (size_t i = 0; i < nocopy.size(); i++)
            {
                t += nocopy[i];
                t += "\n";
            }

            informPackageFailedToCopy(t);
            return false;
        }

        loadPackages();
        return true;
    }

    void PackageManager::removePackages(const QStringList& files)
    {
        QStringList toremove;

        for (size_t i = 0; i < files.size(); i++)
        {
            for (size_t q = 0; q < m_packages.size(); q++)
            {
                //  Must canonicalize these to guard against things like varying
                //  capitalization of driver letters on windows.
                //
                QFileInfo packageFI(m_packages[q].file);
                QFileInfo incomingFI(files[i]);

                if (packageFI.canonicalFilePath()
                    == incomingFI.canonicalFilePath())
                {
                    if (m_packages[q].installed)
                    {
                        if (!uninstallForRemoval(m_packages[q].file))
                        {
                            cerr << "SKIPPING: "
                                 << m_packages[q].file.toUtf8().constData()
                                 << endl;
                            continue;
                        }

                        uninstallPackage(m_packages[q]);
                    }

                    m_packages.erase(m_packages.begin() + q);

                    if (!QFile::remove(files[i]))
                    {
                        cerr << "ERROR: " << files[i].toUtf8().constData()
                             << " not removed" << endl;
                    }
                    else
                    {
                        toremove.push_back(files[i]);
                    }

                    break;
                }
            }
        }

        for (size_t q = 0; q < m_packages.size(); q++)
        {
            m_packages[q].usedBy.clear();
            m_packages[q].uses.clear();
        }

        findPackageDependencies();

        for (size_t i = 0; i < toremove.size(); i++)
        {
            QFileInfo info(toremove[i]);
            writeInstallationFile(
                info.absoluteDir().absoluteFilePath("rvinstall"));
        }

        loadPackages();
    }

    //----------------------------------------------------------------------
    //
    //  Default implementation uses cin/cout
    //

    bool PackageManager::yesOrNo(const char* m1, const char* m2,
                                 const QString& msg, const char* q)
    {
        char yorn = 0;

        cout << m1 << endl << m2 << endl << msg.toUtf8().constData();

        while (yorn != 'y' && yorn != 'n')
        {
            cout << endl << q << " (y or n): " << flush;
            if (m_force)
            {
                cout << "y" << endl;
                yorn = 'y';
            }
            else
                cin >> yorn;
        }

        return yorn == 'y';
    }

    bool PackageManager::fixLoadability(const QString& msg)
    {
        return yesOrNo("Unloadable Package Dependencies",
                       "Can't make package loadable because some of its "
                       "dependencies are not loadable.",
                       msg, "Load other packages first?");
    }

    bool PackageManager::fixUnloadability(const QString& msg)
    {
        return yesOrNo("Loadable Package Dependencies",
                       "Can't make package unloadable because some loaded "
                       "packages depend on it.",
                       msg, "Unload other packages too?");
    }

    bool PackageManager::installDependantPackages(const QString& msg)
    {
        return yesOrNo("Some Packages Depend on This One",
                       "Can't install package because some other packages "
                       "dependend on this one.",
                       msg, "Try and install others first?");
    }

    bool PackageManager::overwriteExistingFiles(const QString& msg)
    {
        return yesOrNo("Existing Package Files",
                       "Package files conflict with existing files.", msg,
                       "Overwrite existing files?");
    }

    void PackageManager::errorMissingPackageDependancies(const QString& msg)
    {
        cerr << "ERROR: Some package dependancies are missing" << endl
             << msg.toUtf8().constData() << endl;
    }

    bool PackageManager::uninstallDependantPackages(const QString& msg)
    {
        return yesOrNo("Some Packages Depend on This One",
                       "Can't uninstall package because some other packages "
                       "dependend on this one.",
                       msg, "Try and uninstall them first?");
    }

    void PackageManager::informCannotRemoveSomeFiles(const QString& msg)
    {
        cout << "INFO: Some Files Cannot Be Removed" << endl
             << msg.toUtf8().constData() << endl;
    }

    void PackageManager::errorModeFileWriteFailed(const QString& file)
    {
        cerr << "ERROR: File write failed: " << file.toUtf8().constData()
             << endl;
    }

    void PackageManager::informPackageFailedToCopy(const QString& msg)
    {
        cout << "INFO: package failed to copy: " << msg.toUtf8().constData()
             << endl;
    }

    void PackageManager::declarePackage(Package&, size_t)
    {
        // for UI
    }

    bool PackageManager::uninstallForRemoval(const QString& msg)
    {
        return yesOrNo("Package is installed",
                       "In order to remove the package it must be uninstalled",
                       msg, "Uninstall?");
    }

    int PackageManager::auxFileIndex(Package& p, const QString& file)
    {
        for (int i = 0; i < p.auxFiles.size(); i++)
        {
            if (p.auxFiles[i].file == file)
                return i;
        }

        return -1;
    }

    QString PackageManager::expandVarsInPath(Package& p, const QString& path)
    {
        QString s = path;
        s.replace("$PACKAGE", p.baseName);
        return s;
    }

#ifdef PLATFORM_WINDOWS
#define SEP ";"
#else
#define SEP ":"
#endif

    RvSettings* RvSettings::m_globalSettingsP = 0;

    RvSettings& RvSettings::globalSettings()
    {
        if (m_globalSettingsP == 0)
            m_globalSettingsP = new RvSettings();

        return *m_globalSettingsP;
    }

    void RvSettings::cleanupGlobalSettings()
    {
        if (m_globalSettingsP)
        {
            m_globalSettingsP->sync();
            delete m_globalSettingsP;
        }
    }

    static void assembleSettings(RvSettings::SettingsMap& map,
                                 const char* envVar, QString prefFileName)
    {
        const char* p = getenv(envVar);
        if (!p)
            return;

        //
        //  Get list of dirs from path env var
        //

        vector<string> tokens;
        stl_ext::tokenize(tokens, p, SEP);

        QList<QDir> prefDirs;

        for (size_t i = 0; i < tokens.size(); i++)
        {
            prefDirs.push_back(QDir(tokens[i].c_str()));
        }

        for (int i = prefDirs.size() - 1; i >= 0; --i)
        {
            if (!prefDirs[i].exists(prefFileName))
                continue;

            QString overrideFileName =
                prefDirs[i].absoluteFilePath(prefFileName);
            QFileInfo overrideFileInfo(overrideFileName);

#ifdef PLATFORM_WINDOWS
            QSettings::Format format(QSettings::IniFormat);
#else
            QSettings::Format format(QSettings::NativeFormat);
#endif
            QSettings overrideSettings(overrideFileName, format);
            overrideSettings.setFallbacksEnabled(false);
            if (overrideSettings.status() != QSettings::NoError)
            {
                cerr << "ERROR: RvSettings was unable to read settings for: '"
                     << overrideSettings.fileName().toStdString()
                     << "' err: " << overrideSettings.status() << endl;
                continue;
            }

            QStringList overrideKeys = overrideSettings.allKeys();

            for (int j = 0; j < overrideKeys.size(); ++j)
            {
                map[overrideKeys[j]] = overrideSettings.value(overrideKeys[j]);
                // cerr << "    '" << overrideKeys[j].toStdString() << "'" <<
                // endl;
            }
        }

        return;
    }

    static QString defaultSettingsFileName()
    {
        QString name;
#if defined(PLATFORM_WINDOWS)
        QString path;
        const char* p = getenv("APPDATA");
        if (p)
            path = p;
        else
            path = QDir::homePath();
        name = QFileInfo(path).canonicalFilePath() + "/"
               + INTERNAL_ORGANIZATION_NAME + "/" + INTERNAL_APPLICATION_NAME
               + ".ini";
#elif defined(PLATFORM_DARWIN)
        name = QDir::homePath() + "/Library/Preferences/com."
               + INTERNAL_ORGANIZATION_NAME + "." + INTERNAL_APPLICATION_NAME
               + ".plist";
#else
        name = QDir::homePath() + "/.config/" + INTERNAL_ORGANIZATION_NAME + "/"
               + INTERNAL_APPLICATION_NAME + ".conf";
#endif

        return name;
    }

    RvSettings::RvSettings()
    {
        //
        //  This function should only ever be called once per run.
        //

        if (m_globalSettingsP)
            cerr << "ERROR: RvSettings instantiated multiple times!" << endl;

#ifdef PLATFORM_WINDOWS
        QSettings::setDefaultFormat(QSettings::IniFormat);
#endif

        QCoreApplication::setOrganizationName(INTERNAL_ORGANIZATION_NAME);
        QCoreApplication::setOrganizationDomain(INTERNAL_ORGANIZATION_DOMAIN);

        //
        //  If -noPrefs command line flag was used, use empty alternate
        //  prefs file, so that default values are used, and any changes
        //  the users makes do not affect stored prefs.
        //
        if (PackageManager::ignoringPrefs())
        {
            m_userSettings = getQSettings();
            //
            //  Empty the prefs file.
            //
            m_userSettings->clear();
            //
            //  Reload from empty file.
            //
            delete m_userSettings;
            m_userSettings = getQSettings();
            //
            //  Note that in the noPrefs mode, overriding and clobbering
            //  settings are also ignored.
            //
            return;
        }

        QString prefsFileName = QFileInfo(defaultSettingsFileName()).fileName();

        assembleSettings(m_overridingSettings, "RV_PREFS_OVERRIDE_PATH",
                         prefsFileName);
        assembleSettings(m_clobberingSettings, "RV_PREFS_CLOBBER_PATH",
                         prefsFileName);

        QCoreApplication::setApplicationName(INTERNAL_APPLICATION_NAME);

        m_userSettings = getQSettings();
    }

    RvSettings::~RvSettings()
    {
        if (!m_userSettingsErrorAlredyReported
            && (m_userSettings->status() != QSettings::NoError))
        {
            cerr << "ERROR: RvSettings encountered error with: '"
                 << m_userSettings->fileName().toStdString()
                 << "' err: " << m_userSettings->status() << endl;

            m_userSettingsErrorAlredyReported = true;
        }
        delete m_userSettings;
    }

    QSettings* RvSettings::getQSettings()
    {
#ifdef PLATFORM_WINDOWS
        QSettings::Format format(QSettings::IniFormat);
#else
        QSettings::Format format(QSettings::NativeFormat);
#endif
        QSettings* qs = new QSettings(
            format, QSettings::UserScope, INTERNAL_ORGANIZATION_NAME,
            PackageManager::ignoringPrefs() ? "RVALT"
                                            : INTERNAL_APPLICATION_NAME);
        qs->setFallbacksEnabled(false);

        if (qs->status() != QSettings::NoError)
        {
            cerr << "ERROR: RvSettings was unable to read settings for: '"
                 << qs->fileName().toStdString() << "' err: " << qs->status()
                 << endl;
        }

        return qs;
    }

    static QString rebuildGroup(QStringList& stack)
    {
        QString g;
        for (int i = 0; i < stack.size(); ++i)
        {
            if (i)
                g += "/";
            g += stack[i];
        }

        return g;
    }

    void RvSettings::beginGroup(const QString& prefix)
    {
        //
        // We only use two levels of settings, so safer to
        // ensure that beginGroup() always starts from the top level.
        // XXX note that this _forbids_ the use of more than one level
        //     of preferences.
        //
        while (!m_userSettings->group().isEmpty())
            m_userSettings->endGroup();

        m_userSettings->beginGroup(prefix);
    }

    void RvSettings::endGroup() { m_userSettings->endGroup(); }

    QVariant RvSettings::value(const QString& key,
                               const QVariant& defaultValue) const
    {
        SettingsMap::const_iterator i;

        QString fullKey = m_userSettings->group() + "/" + key;

        if ((i = m_clobberingSettings.find(fullKey))
            != m_clobberingSettings.end())
        //
        //  Clobbering value always wins
        //
        {
            return i.value();
        }
        else if (m_userSettings->contains(key))
        //
        //  User prefs have this key
        //
        {
            return m_userSettings->value(key, defaultValue);
        }
        else if ((i = m_overridingSettings.find(fullKey))
                 != m_overridingSettings.end())
        //
        //  No user key either, so use the overriding initializer
        //
        {
            return i.value();
        }

        return defaultValue;
    }

    void RvSettings::setValue(const QString& key, const QVariant& value)
    {
        SettingsMap::iterator i;

        QString fullKey = m_userSettings->group() + "/" + key;

        if ((i = m_clobberingSettings.find(fullKey))
            != m_clobberingSettings.end())
        //
        //  Value came from clobbering settings, so check that incoming value is
        //  different, then remove from clobbering settings (otherwise,
        //  value() will still return the clobbering setting.
        //
        {
            if (value == i.value())
                return;
            m_clobberingSettings.erase(i);
        }

        if ((i = m_overridingSettings.find(fullKey))
                != m_overridingSettings.end()
            && !m_userSettings->contains(key))
        //
        //  Value came from overriding settings, so check that incoming value is
        //  different, then remove from overriding settings (otherwise,
        //  value() will still return the overriding setting.
        //
        {
            if (value == i.value())
                return;
            m_overridingSettings.erase(i);
        }

        //
        //  Finally save setting in user settings, so that next time the value
        //  comes from user settings, unless it is clobbered.
        //
        m_userSettings->setValue(key, value);
        sync();
    }

    void RvSettings::sync()
    {
        m_userSettings->sync();
        if (!m_userSettingsErrorAlredyReported
            && (m_userSettings->status() != QSettings::NoError))
        {
            cerr << "ERROR: RvSettings was unable to write settings for: '"
                 << m_userSettings->fileName().toStdString()
                 << "' err: " << m_userSettings->status() << endl;

            m_userSettingsErrorAlredyReported = true;
        }
    }

    void RvSettings::remove(const QString& key)
    {
        QString fullKey = m_userSettings->group() + "/" + key;

        if (!m_clobberingSettings.contains(fullKey)
            && m_userSettings->contains(key))
        //
        //  Only remove key from user settings if it didn't come from the
        //  clobbering settings in the first place.
        //
        {
            m_userSettings->remove(key);
        }
    }

    bool RvSettings::contains(const QString& key) const
    {
        QString fullKey = m_userSettings->group() + "/" + key;

        if (m_clobberingSettings.contains(fullKey)
            || m_userSettings->contains(key)
            || m_overridingSettings.contains(fullKey))
        //
        //  The setting is provided by clobbering settings, or by user settings,
        //  or by overriding settings.
        //
        {
            return true;
        }

        return false;
    }

} // namespace Rv
