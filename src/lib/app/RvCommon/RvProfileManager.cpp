//
//  Copyright (c) 2013 Tweak Software. 
//  All rights reserved.
//  
//  SPDX-License-Identifier: Apache-2.0
//  
//




#include <RvCommon/RvProfileManager.h>
#include <IPCore/Profile.h>
#include <TwkApp/Bundle.h>
#include <RvApp/RvSession.h>
#include <RvApp/RvGraph.h>
#include <IPCore/DisplayGroupIPNode.h>
#include <IPCore/PipelineGroupIPNode.h>
#include <QtCore/QtCore>
#include <TwkUtil/File.h>
#include <TwkQtCoreUtil/QtConvert.h>

#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>

namespace Rv {
using namespace std;
using namespace TwkApp;
using namespace IPCore;
using namespace TwkQtCoreUtil;

RvProfileManager::RvProfileManager(QObject* obj) 
    : QDialog(),
      m_model(0),
      m_createDialog(0)
{
    m_ui.setupUi(this);
    loadModel();

    m_createDialogUI.setupUi(m_createDialog = new QDialog(this, Qt::Sheet));

    connect(m_ui.profileTreeView->selectionModel(),
            SIGNAL(selectionChanged(const QItemSelection&,const QItemSelection&)),
            this, SLOT(selectionChanged(const QItemSelection&,const QItemSelection&)));
    connect(m_ui.addButton, SIGNAL(released()), this, SLOT(addProfile()));
    connect(m_ui.deleteButton, SIGNAL(released()), this, SLOT(deleteProfile()));
    connect(m_ui.applyButton, SIGNAL(pressed()),this, SLOT(applyProfile()));
}

RvProfileManager::~RvProfileManager()
{
}

void
RvProfileManager::loadModel()
{
    if (!m_model) m_model = new QStandardItemModel();
    m_model->clear();

    QStringList headers;
    headers << "Name" << "Locked" << "Assigned" << "File";
    m_model->setHorizontalHeaderLabels(headers);

    RvSession* session = Rv::RvSession::currentRvSession();
    const IPCore::IPGraph::DisplayGroups& dgroups = session->graph().displayGroups();

    //
    //  At the moment we only manage "display" profiles.
    //
    profilesInPath(m_profiles, "display", &session->graph());

    for (size_t i = 0; i < m_profiles.size(); i++)
    {
        Profile* p = m_profiles[i];
        QString filename = p->fileName().c_str();
        QString name = p->name().c_str();
        
        string assignName = "";

        for (size_t q = 0; q < dgroups.size(); q++)
        {
            if (dgroups[q]->displayPipelineNode()->profile() == p->name())
            {
                if (assignName.size()) assignName += ", ";
                assignName += dgroups[q]->physicalDevice()->name();
            }
        }

        QIcon icon = TwkUtil::isWritable(p->fileName().c_str()) ? QIcon() : QIcon(":images/lock_out.png");

        QList<QStandardItem*> row;

        QStandardItem* nameItem   = new QStandardItem(name); 
        QStandardItem* lockedItem = new QStandardItem(icon, "");
        QStandardItem* assignItem = new QStandardItem(assignName.c_str());
        QStandardItem* fileItem   = new QStandardItem(filename); 

        nameItem->setData(QVariant((int)i));

        row << nameItem
            << lockedItem
            << assignItem
            << fileItem;

        for (size_t q = 0; q < row.size(); q++)
        {
            row[q]->setEditable(false);
        }

        m_model->appendRow(row);
    }

    m_ui.profileTreeView->setModel(m_model);
    m_ui.profileTreeView->setAlternatingRowColors(true);
    for (int i=0; i < 4; i++) m_ui.profileTreeView->resizeColumnToContents(i);
}

void 
RvProfileManager::addProfile()
{

    //
    //  At the moment, new profiles are created in the user support area, but
    //  this may not be in the RV_SUPPORT_PATH, in which case newly created
    //  profiles will be invisible, hence confusion.  Disallow creation in this
    //  case.  Also check if the directory exists or can be created if not.

    TwkApp::Bundle* bundle = TwkApp::Bundle::mainBundle();

    QDir profilesDir(bundle->userPluginPath("Profiles").c_str());

    //
    //  Check that user dir is in support Path
    //

    QDir userDir(profilesDir);
    userDir.cdUp();

    bool foundIt = false;
    vector<string> supportPath = bundle->supportPath();

    for (int i = 0; i < supportPath.size(); ++i)
    {
        if (userDir.canonicalPath() == QDir(supportPath[i].c_str()).canonicalPath())
	{
	    foundIt = true;
	}
    }

    if (!foundIt)
    {
	cerr <<  "ERROR: New Display Profiles will be created in the user support area: " << endl
	     <<  "ERROR: '" << UTF8::qconvert(userDir.canonicalPath()) << "'" << endl
	     <<  "ERROR: but that directory has been removed from the RV_SUPPORT_PATH." << endl
	     <<  "ERROR: Please consult your sysadmin." << endl;
	return;
    }

    //
    //  Check that Profiles subdir exists or can be created.
    //

    profilesDir.mkpath(profilesDir.absolutePath());

    if (!profilesDir.exists()) 
    {
	cerr <<  "ERROR: target Profiles directory does not exist: "
             << UTF8::qconvert(profilesDir.absolutePath())
             << endl;
	return;
    }


    RvSession* session = Rv::RvSession::currentRvSession();
    const IPCore::IPGraph::DisplayGroups& dgroups = session->graph().displayGroups();

    m_createDialogUI.deviceComboBox->clear();

    for (size_t i = 0; i < dgroups.size(); i++)
    {
        IPCore::DisplayGroupIPNode* dgroup = dgroups[i];
        m_createDialogUI.deviceComboBox->addItem( dgroup->physicalDevice()->name().c_str() );
    }

    m_createDialog->show();
    m_createDialog->exec();
    if (m_createDialog->result() == QDialog::Rejected) return;

    int index = m_createDialogUI.deviceComboBox->currentIndex();
    IPCore::DisplayGroupIPNode* node = dgroups[index];

    Session::WriteRequest request;
    request.setOption("comments", string(m_createDialogUI.commentsEdit->toPlainText().toUtf8().constData()));
    request.setOption("connections", true);
    request.setOption("membership", true);
    request.setOption("recursive", true);
    request.setOption("sparse", false);
    QString initName = m_createDialogUI.nameEdit->text().trimmed();
    if (initName.isEmpty())
    {
        cerr <<  "ERROR: Profile name must contain non-white-space characters." << endl;
    }
    else
    {
	string name = initName.toUtf8().constData();
	//
	//  At the moment we only manage "display" profiles.
	//
	name += ".display.profile";

        QFileInfo fileInfo(profilesDir, QString(name.c_str()));
        QFileInfo dirInfo(profilesDir.canonicalPath());

        const bool isDirWritable = TwkUtil::isWritable(UTF8::qconvert(profilesDir.canonicalPath()).c_str()); 
        const bool isFileWritable = TwkUtil::isWritable(UTF8::qconvert(fileInfo.absoluteFilePath()).c_str()); 

        if (!isDirWritable || (fileInfo.exists() && !isFileWritable))
        {
            cerr << "ERROR: User Profile directory \""
                 << UTF8::qconvert(profilesDir.canonicalPath())
                 << "\" is unwriteable! Profile not saved!"
                 << endl;
        }
        else
	{
            QString file = profilesDir.absoluteFilePath(name.c_str());
            session->writeProfile(file.toUtf8().constData(), node->displayPipelineNode(), request);
	}
    }
    loadModel();
}

void
RvProfileManager::clear()
{
    for (size_t i = 0; i < m_profiles.size(); i++) delete m_profiles[i];
}

Profile*
RvProfileManager::currentProfile()
{
    QModelIndex index = m_ui.profileTreeView->selectionModel()->currentIndex();
    if (index.isValid()) return m_profiles[index.row()];
    return 0;
}

void 
RvProfileManager::deleteProfile()
{
    Profile* p = currentProfile();
    if (!p) return;
    QString filename = p->fileName().c_str();

    if (TwkUtil::isWritable(p->fileName().c_str()))
    {
        QMessageBox box(this);
        box.setWindowTitle(tr("Delete Profile"));
        box.setText(tr("Are you sure you want to delete the profile?"));

        box.setWindowModality(Qt::WindowModal);
        QPushButton* b1 = box.addButton(tr("Cancel"), QMessageBox::RejectRole);
        QPushButton* b2 = box.addButton(tr("Delete"), QMessageBox::AcceptRole);
        box.setIcon(QMessageBox::Critical);
        box.exec();

        if (box.clickedButton() != b1)
        {
            QFile::remove(filename);
            clear();
            loadModel();
        }
    }
}

void 
RvProfileManager::selectionChanged(const QItemSelection&, const QItemSelection&)
{
    Profile* profile = currentProfile();
    if (!profile) return;
    bool ok = true;

    try
    {
        profile->load();
    }
    catch (...)
    {
        ok = false;
        m_ui.profileDescriptionBrowser->setText("ERROR: reading profile");
    }

    if (ok)
    {
        string comment = profile->comment();
        m_ui.profileDescriptionBrowser->setText(comment.c_str());
        m_ui.profileDescriptionBrowser->append("");
        m_ui.profileDescriptionBrowser->append("Non-default Values:");
        
        string desc = profile->structureDescription();
        m_ui.profileDescriptionBrowser->append(desc.c_str());
    }
}

void
RvProfileManager::applyProfile()
{
    Profile* profile = currentProfile();
    if (!profile) return;
    RvSession* session = Rv::RvSession::currentRvSession();

    profile->apply( session->graph().primaryDisplayGroup()->displayPipelineNode() );

    loadModel();
    session->askForRedraw();
}

} // Rv

