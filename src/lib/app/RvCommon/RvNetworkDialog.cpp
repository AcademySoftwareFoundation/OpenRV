//
//  Copyright (c) 2008 Tweak Software. 
//  All rights reserved.
//  
//  SPDX-License-Identifier: Apache-2.0
//  
//




#include <RvCommon/RvApplication.h>
#include <RvCommon/RvNetworkDialog.h>
#include <RvCommon/RvPreferences.h>
#include <RvCommon/PermDelegate.h>
#include <RvCommon/StreamConnection.h>
#include <QtNetwork/QtNetwork>
#include <QtWidgets/QMessageBox>
#include <TwkQtChat/Client.h>
#include <TwkQtChat/Connection.h>
#include <IPCore/Session.h>
#include <RvApp/Options.h>
#include <TwkQtCoreUtil/QtConvert.h>
#include <stl_ext/string_algo.h>
#ifdef PLATFORM_WINDOWS
    #include <process.h>
#endif

#if 0
#define DB_ICON         0x01
#define DB_ALL          0xff

//  #define DB_LEVEL        (DB_ALL & (~ DB_ICON))
#define DB_LEVEL        DB_ALL

#ifdef PLATFORM_WINDOWS
    #define GPID _getpid
#else
    #define GPID getpid
#endif
#define DB(x)           cerr << "NetworkDialog " << GPID() << ": " << x << endl
#define DBL(level, x)   if (level & DB_LEVEL) cerr << "NetworkDialog " GPID() << ": " << x << endl
#else
#define DB(x)
#define DBL(level, x)
#endif

namespace Rv {
using namespace IPCore;
using namespace std;
using namespace TwkQtChat;
using namespace TwkQtCoreUtil;

RvNetworkDialog::RvNetworkDialog(QWidget* parent) 
    : QWidget(parent),
      m_client(0),
      m_contactsModel(0),
      m_connectDialog(this)
{
    DB("");
    setWindowFlags(Qt::Window);
    setWindowModality(Qt::NonModal);
    setWindowIcon(QIcon(qApp->applicationDirPath() + QString(RV_ICON_PATH_SUFFIX)));
    m_ui.setupUi(this);

    m_connectUI.setupUi(&m_connectDialog);
    m_connectUI.buttonBox->button(QDialogButtonBox::Ok)->setText("Connect");

    m_contactPopup = new QMenu("Selection", 0);
    QAction* mtitle      = m_contactPopup->addAction("");
    QAction* msep        = m_contactPopup->addSeparator();
    QAction* mconnect    = m_contactPopup->addAction("Connect...");
    QAction* mdisconnect = m_contactPopup->addAction("Disconnect...");
    QAction* mdelete     = m_contactPopup->addAction("Delete");

    m_ui.statusLabel->setText("<b>Not Running</b>");
    m_contactsModel = new QStandardItemModel();
    m_ui.contactTreeView->setModel(m_contactsModel);
    m_ui.contactTreeView->setItemDelegate(new PermDelegate());

    connect(m_ui.startButton, SIGNAL(clicked()), this, SLOT(toggleServer()));
    connect(m_ui.connectButton, SIGNAL(clicked()), this, SLOT(startConnect()));
    connect(mconnect, SIGNAL(triggered(bool)), this, SLOT(popupConnect(bool)));
    connect(mdisconnect, SIGNAL(triggered(bool)), this, SLOT(popupDisconnect(bool)));
    connect(mdelete, SIGNAL(triggered(bool)), this, SLOT(popupDelete(bool)));
    connect(m_ui.resetConfigButton, SIGNAL(clicked()), this, SLOT(resetConfig()));

    connect(m_ui.contactTreeView,
            SIGNAL(customContextMenuRequested(const QPoint&)),
            this, SLOT(popup(const QPoint&)));

    connect(m_ui.contactTreeView, SIGNAL(doubleClicked(const QModelIndex&)), 
            this, SLOT(doubleClickContact(const QModelIndex&)));


    loadSettings();
}

RvNetworkDialog::~RvNetworkDialog()
{
    DB ("RvNetworkDialog::~RvNetworkDialog");
    if (m_client) toggleServer();
}

void
RvNetworkDialog::loadSettings()
{
    RV_QSETTINGS;
    settings.beginGroup("Network");
    
    const Rv::Options& opts = Rv::Options::sharedOptions();

    int permPrefs[3] = {opts.networkPerm, settings.value("defaultPermission").toInt(), AskConnect};
    for (int permIndex = 0; permIndex < 3; permIndex++)
    {
        int permCheck = permPrefs[permIndex];
        if (permCheck == AskConnect || permCheck == AllowConnect || permCheck == DenyConnect)
        {
            m_ui.permissionCombo->setCurrentIndex(permCheck);
            break;
        }
    }
            
    //
    //  Create default contact information for this end. This will
    //  probably be replaced by the client app later.
    //

    QStringList envVariables;
    envVariables << "USERNAME.*" << "USER.*" << "USERDOMAIN.*"
                 << "HOSTNAME.*" << "DOMAINNAME.*";

    QStringList environment = QProcess::systemEnvironment();

    QString defaultName;

    foreach (QString string, envVariables) 
    {
        int index = environment.indexOf(QRegExp(string));

        if (index != -1) 
        {
            QStringList stringList = environment.at(index).split("=");

            if (stringList.size() == 2) 
            {
                defaultName = stringList.at(1).toUtf8();
                break;
            }
        }
    }

    QString name;
    if (opts.networkUser) 
    {
        name = opts.networkUser;
    }
    else
    {
        name = settings.value("name", defaultName).toString();
    }
    m_ui.nameLineEdit->setText(name);

    QString port = QString("%1").arg(settings.value("primaryPort", 45124).toInt());
    if (opts.networkPort) port = QString(QString("%1").arg(opts.networkPort));
    m_ui.portLineEdit->setText(port);

    ContactMap contacts = settings.value("contacts").toMap();

    QList<QString> keys = contacts.keys();
    m_contactsModel->clear();
    QStringList headers;
    headers.push_back("Connected");
    headers.push_back("Name   ");
    headers.push_back("Machine");
    headers.push_back("Permission");
    headers.push_back("App    ");
    headers.push_back("Session");
    m_contactsModel->setHorizontalHeaderLabels(headers);

    for (int i=0; i < keys.size(); i++)
    {
        QStringList parts = keys[i].split("@");
        unsigned int p = contacts[keys[i]].toUInt();
        addContact(parts[0], parts[1], 0, p, "", false);
    }

    for (int i=0; i < 4; i++) m_ui.contactTreeView->resizeColumnToContents(i);

    settings.endGroup();
}

bool
RvNetworkDialog::confirmDisconnect(const QString& contact)
{
    QMessageBox box(QMessageBox::Warning, 
                    "Disconnect",
                    QString("Disconnect from %1?").arg(contact),
                    QMessageBox::NoButton, this, Qt::Sheet);

    QPushButton* q1 = box.addButton("Disconnect", QMessageBox::AcceptRole);
    QPushButton* q2 = box.addButton("Cancel", QMessageBox::RejectRole);
    box.setIcon(QMessageBox::Question);
    box.exec();
    return box.clickedButton() == q1;
}

void
RvNetworkDialog::resetConfig()
{
    QMessageBox box(QMessageBox::Warning, "Reset Network Configuration",
                    "Reset Network Configuration Settings?",
                    QMessageBox::NoButton, this, Qt::Sheet);

    QPushButton* q1 = box.addButton("Reset", QMessageBox::AcceptRole);
    QPushButton* q2 = box.addButton("Cancel", QMessageBox::RejectRole);
    box.setIcon(QMessageBox::Warning);
    box.exec();

    if (box.clickedButton() == q1)
    {
        m_ui.portLineEdit->setText("45124");
    }
}

int 
RvNetworkDialog::findContact(const QString& cname,
                             const QString& chost)
{
    int n = m_contactsModel->rowCount();

    for (int i=0; i < n; i++)
    {
        if (m_contactsModel->item(i, 1)->text() == cname &&
            m_contactsModel->item(i, 2)->text() == chost)
        {
            return i;
        }
    }

    return -1;
}

void 
RvNetworkDialog::addContact(const QString& cname,
                            const QString& chost,
                            TwkQtChat::Connection* connection,
                            unsigned int cperms,
                            const QString& capp,
                            bool checkForExisting)
{
    unsigned int tperms = cperms;
    if (tperms == CurrentConnect) tperms = m_ui.permissionCombo->currentIndex();

    QString permText = "";
    if (tperms == AskConnect) permText = "Ask";
    else if (tperms == AllowConnect) permText = "Allow";
    else if (tperms == DenyConnect) permText = "Deny";
    else
    {
        cerr << "ERROR: Unable to determine default contact permission: " << tperms;
        return;
    }

    string sender = string(cname.toUtf8().constData()) + "@" + chost.toUtf8().constData();
    QString csess(m_sessionMap[sender].c_str());
    DB ("RvNetworkDialog::addContact cname " << cname.toUtf8().data() << 
        " chost " << chost.toUtf8().data() << 
        " connection " << connection <<
        " capp " << capp.toUtf8().data() << 
        " session " << csess.toUtf8().data());

    if (checkForExisting)
    {
        int i = findContact(cname, chost);

        if (i != -1)
        {
            if (cperms != CurrentConnect)
                m_contactsModel->item(i, 3)->setText(permText);
            m_contactsModel->item(i, 0)->setCheckState(connection ? Qt::Checked : Qt::Unchecked);
            m_contactsModel->item(i, 4)->setText(capp);
            m_contactsModel->item(i, 5)->setText(csess);

            return;
        }
    }

    QStandardItem* conn = new QStandardItem();
    QStandardItem* name = new QStandardItem(cname);
    QStandardItem* host = new QStandardItem(chost);
    QStandardItem* perm = new QStandardItem();
    QStandardItem* appl = new QStandardItem(capp);
    QStandardItem* sess = new QStandardItem(csess);

    conn->setCheckState(connection ? Qt::Checked : Qt::Unchecked);
    conn->setEditable(false);
    name->setEditable(false);
    host->setEditable(false);
    perm->setEditable(true);
    perm->setText(permText);
    appl->setEditable(false);
    sess->setEditable(false);

    QList<QStandardItem*> row;
    row.push_back(conn);
    row.push_back(name);
    row.push_back(host);
    row.push_back(perm);
    row.push_back(appl);
    row.push_back(sess);

    m_contactsModel->appendRow(row);

    //
    //  If this is a local connection, hide this row in the contact list, since
    //  we always allow them anyway, and display drivers, etc can clutter the
    //  contact list with lots of connections.
    //
    if (connection && connection->isLocal()) 
    {
        m_ui.contactTreeView->setRowHidden(m_contactsModel->rowCount() - 1, QModelIndex(), true);
    }
}

void
RvNetworkDialog::saveSettings()
{
    RV_QSETTINGS;
    settings.beginGroup("Network");
    settings.setValue("name", m_ui.nameLineEdit->text());
    settings.setValue("primaryPort", m_ui.portLineEdit->text().toInt());

    ContactMap contacts;
    int n = m_contactsModel->rowCount();

    for (int i=0; i < n; i++)
    {
        //
        // Only store in prefs if not hidden.  Hidden rows hold contact info
        // for local connections, which we don't need to store (since we always
        // allow them).
        //
        if (! m_ui.contactTreeView->isRowHidden(i, QModelIndex()))
        {
            QString name = m_contactsModel->item(i, 1)->text();
            QString mach = m_contactsModel->item(i, 2)->text();
            QString pname = m_contactsModel->item(i, 3)->text();

            unsigned int p = 0;
            if (pname == "Allow") p = 1;
            else if (pname == "Deny") p = 2;

            contacts[name + "@" + mach] = p;
        }
    }

    settings.setValue("contacts", contacts);
    settings.setValue("defaultPermission", m_ui.permissionCombo->currentIndex());
    settings.endGroup();
}

void
RvNetworkDialog::shutdownServer()
{
    DB ("RvNetworkDialog::shutdownServer m_client " << m_client);
    if (m_client)
    {
        std::vector<string> cs = connections();
        for (int i=0; i < cs.size(); i++) 
        {
            DB ("    signOff " << cs[i]);
            m_client->signOff(cs[i].c_str());
        }
        saveSettings();
        delete m_client;
        m_client = 0;
        m_ui.participantList->clear();
        m_sessionMap.clear();
        sendSessionEvent("remote-network-stop", "", "");

        deletePortNumberFile();
    }
}

void
RvNetworkDialog::savePortNumber()
{
    DB ("savePortNumber");

    //  System temp dir
    QDir tmp = QDir::temp();

    if (! tmp.exists ("tweak_rv_proc"))
    {
        DB ("    dir doesn't exist");
        tmp.mkdir ("tweak_rv_proc");
        tmp.cd ("tweak_rv_proc");
        QFile::setPermissions (tmp.absolutePath(), 
                QFile::ReadOwner  | QFile::ReadUser  | QFile::ReadGroup  | QFile::ReadOther |
                QFile::WriteOwner | QFile::WriteUser | QFile::WriteGroup | QFile::WriteOther |
                QFile::ExeOwner   | QFile::ExeUser   | QFile::ExeGroup   | QFile::ExeOther);
    }
    else tmp.cd("tweak_rv_proc");

    QString pid;
    #ifdef PLATFORM_WINDOWS
        pid.setNum (_getpid());
    #else
        pid.setNum (getpid());
    #endif

    //
    //  Write port number to file named: <tmp>/tweak_rv_proc/<processID>_<tag>
    //

    QString tagString("_");
    const Rv::Options& opts = Rv::Options::sharedOptions();
    if (opts.networkTag) tagString += opts.networkTag;

    QFile portFile(tmp.absolutePath() + "/" + pid + tagString);

    DB ("    opening portfile '" << portFile.fileName().toUtf8().constData() << "'");
    portFile.open (QIODevice::WriteOnly | QIODevice::Truncate);
    portFile.setPermissions (
                QFile::ReadOwner  | QFile::ReadUser  | QFile::ReadGroup  | QFile::ReadOther |
                QFile::WriteOwner | QFile::WriteUser | QFile::WriteGroup | QFile::WriteOther);

    QString portNum;
    portNum.setNum(myPort());
    portNum += "\n";
    portFile.write (portNum.toUtf8());
    portFile.close();

    m_portFile = portFile.fileName();
}

void
RvNetworkDialog::deletePortNumberFile()
{
    if (!m_portFile.isEmpty()) QFile::remove(m_portFile);
}

void
RvNetworkDialog::closeEvent(QCloseEvent *event)
{
    saveSettings();
    QWidget::closeEvent(event);
}

void
RvNetworkDialog::toggleServer()
{
    DB ("RvNetworkDialog::toggleServer m_client " << m_client);
    if (m_client)
    {
        int n = m_ui.participantList->count();
        const TwkApp::Application::Documents& docs = IPCore::App()->documents();

        if (n > 0 && docs.size())
        {
            QMessageBox box(QMessageBox::Warning, "Stop Network",
                            QString("%1 connection%2 %3 still active")
                            .arg(n).arg(n==1 ? "" : "s").arg(n==1 ? "is" : "are"),
                            QMessageBox::NoButton, this, Qt::Sheet);

            QPushButton* q1 = box.addButton("Stop Network", QMessageBox::AcceptRole);
            QPushButton* q2 = box.addButton("Cancel", QMessageBox::RejectRole);
            box.setIcon(QMessageBox::Warning);
            box.exec();
            if (box.clickedButton() == q2) return;
        }

        saveSettings();

        std::vector<string> cs = connections();
        for (int i=0; i < cs.size(); i++) 
        {
            DB ("    signOff " << cs[i]);
            m_client->signOff(cs[i].c_str());
        }

        delete m_client;
        m_client = 0;
        m_ui.participantList->clear();
        m_sessionMap.clear();
        
        sendSessionEvent("remote-network-stop", "", "");

        deletePortNumberFile();
    }
    else
    {
        int port = m_ui.portLineEdit->text().toInt();
        m_client = new Client(m_ui.nameLineEdit->text(), "rv", port, true, StreamConnection::connectionFactory);

        if (m_client->online())
        {
	    fprintf (stderr, "INFO: listening on port %d\n", m_client->serverPort());
            QString portStr = QString(QString("%1").arg(m_client->serverPort()));
            m_ui.portLineEdit->setText(portStr);

            connect(m_client, SIGNAL(newData(const QString&,
                                            const QString&,
                                            const QByteArray&)),
                    this, SLOT(newData(const QString&,
                                    const QString&,
                                    const QByteArray&)));

            connect(m_client, SIGNAL(newContact(const QString &)),
                    this, SLOT(newRemoteContact(const QString &)));

            connect(m_client, SIGNAL(newMessage(const QString &, const QString &)),
                    this, SLOT(newMessage(const QString &, const QString &)));

            connect(m_client, SIGNAL(contactLeft(const QString &)),
                    this, SLOT(remoteContactLeft(const QString &)));

            connect(m_client, SIGNAL(requestConnection(TwkQtChat::Connection*)),
                    this, SLOT(requestConnection(TwkQtChat::Connection*)));

            connect(m_client, SIGNAL(contactError(const QString&, const QString&, const QString&)),
                    this, SLOT(contactError(const QString&, const QString&, const QString&)));

            sendSessionEvent("remote-network-start", "", "");

            savePortNumber();
        }
        else
        {
            delete m_client;
            m_client = 0;
        }

    }

    updateStatus();
}

IPCore::Session*
RvNetworkDialog::targetSession (QString sender, bool doDisconnect)
{
    DB ("targetSession sender " << sender.toUtf8().constData() << 
            " doDis " << doDisconnect);
    Session* s = RvApp()->session(m_sessionMap[sender.toUtf8().constData()]);
    if (!s && doDisconnect) 
    {
        cerr << "ERROR: Session associated with sender '"
                << sender.toUtf8().constData()
                << "' does not exist, disconnecting"
                << endl;

        QList<QListWidgetItem*> items = 
            m_ui.participantList->findItems(sender, Qt::MatchExactly);
        for (int i=0; i < items.size(); i++) delete items[i];

        m_sessionMap.erase(sender.toUtf8().constData());
        m_client->signOff(sender);
        m_client->disconnectFrom(sender);
    }
    return s;
}

void 
RvNetworkDialog::newRemoteContact(const QString& name)
{
    DB ("newRemoteContact '" << UTF8::qconvert(name) << "'");
    Session* s = targetSession(name);
    if (!s) return;

    m_ui.participantList->addItem(name);
    updateStatus();

    s->userGenericEvent("remote-connection-start", name.toUtf8().constData(), "");
}

RvNetworkDialog::ConnectPermission
RvNetworkDialog::contactPermission(const QStringList& parts)
{
    int n = m_contactsModel->rowCount();

    for (int i=0; i < n; i++)
    {
        QStandardItem* item1 = m_contactsModel->item(i, 1);

        if (item1->text() == parts[0])
        {
            QStandardItem* item2 = m_contactsModel->item(i, 2);

            if (item2->text() == parts[1])
            {
                QStandardItem* item3 = m_contactsModel->item(i, 3);
                QString perms = item3->text();
                if (perms == "Ask") return AskConnect;
                if (perms == "Allow") return AllowConnect;
                if (perms == "Deny") return DenyConnect;
            }
        }
    }

    int index = m_ui.permissionCombo->currentIndex();
    if (index == 1) return AllowConnect;
    if (index == 2) return DenyConnect;
    return AskConnect;
}


void
RvNetworkDialog::requestConnection(TwkQtChat::Connection* c)
{
    DB ("RvNetworkDialog::requestConnection " <<
            c->remoteContactName().toUtf8().constData());
    //
    //  This may add the contact
    //

    DB ("    checking for StreamConnection");
    const char* streamDir = getenv ("RV_NETWORK_STREAM_STORAGE_DIR");
    if (streamDir)
    {
        if (StreamConnection *strCon = dynamic_cast<StreamConnection *> (c))
        {
            if (strCon->streamState() == StreamConnection::NormalState)
            {
                DB ("    calling storeStream() dir '" << streamDir << "'");
                strCon->storeStream(streamDir);
            }
        }
    }

    QString name = c->remoteContactName();
    QStringList parts = name.split("@");

    //
    //  XXX  Prob we neeed to pop a dialog here to ask the user
    //  which session they'd like to connect to.
    //
    Session* session = Session::activeSession();
    QString csess;
    if (!session)
    {
        cerr << "ERROR: RvNetwork: no session for incoming connection" << endl;
        m_client->rejectConnection(c);
        return;
    }
    string sender = name.toUtf8().constData();
    if (m_sessionMap[sender].size())
    {
        cerr << "ERROR: RvNetwork: '" << sender << "' is already connected." << endl;
        c->setDuplicate(true);
        m_client->rejectConnection(c);
        return;
    }
    m_sessionMap[sender] = session->name();
    DB ("    new sessionMap val '" << m_sessionMap[sender] << "'");
    csess = session->name().c_str();

    if (c->isLocal()) 
    {
        addContact(parts[0], parts[1], c, CurrentConnect, c->remoteApp());
        return;
    }

    //
    //  Find out the existing policy regarding the contact that's
    //  trying to make the connection.
    //

    ConnectPermission perm = contactPermission(parts);

    if (perm == AskConnect)
    {
        QMessageBox box(QMessageBox::Warning, "Connection Request",
                        QString("%1 wants to connect\n ").arg(name),
                        QMessageBox::NoButton, this, Qt::Dialog);

        QPushButton* q3 = box.addButton("Always Allow", QMessageBox::AcceptRole);
        QPushButton* q1 = box.addButton("Allow Once", QMessageBox::ApplyRole);
        QPushButton* q2 = box.addButton("Deny", QMessageBox::RejectRole);
        box.exec();

        if (box.clickedButton() == q2)
        {
            //  Deny
            m_client->rejectConnection(c);
            addContact(parts[0], parts[1], 0, CurrentConnect, "");
        }
        else if (box.clickedButton() == q1)
        {
            // allow
            addContact(parts[0], parts[1], c, CurrentConnect, c->remoteApp());
        }
        else if (box.clickedButton() == q3) 
        {
            // allow and record it for future

            addContact(parts[0], parts[1], c, AllowConnect, c->remoteApp());
        }
    }
    else if (perm == DenyConnect)
    {
        m_client->rejectConnection(c);
        addContact(parts[0], parts[1], 0, CurrentConnect, c->remoteApp());
    }
    else
    {
        // just allow connect
        addContact(parts[0], parts[1], c, CurrentConnect, c->remoteApp());
    }
}

void 
RvNetworkDialog::remoteContactLeft(const QString& name)
{
    DB ("RvNetworkDialog::remoteContactLeft name " << name.toUtf8().constData());
    const TwkApp::Application::Documents& docs = App()->documents();
    if (!docs.size())
    {
        DB ("exiting ?");
        // we're probably exiting
        return;
    }

    QList<QListWidgetItem*> items = 
        m_ui.participantList->findItems(name, Qt::MatchExactly);

    for (int i=0; i < items.size(); i++) delete items[i];
    QStringList parts = name.split("@");

    if (parts.size() == 2)
    {
        int row = findContact(parts[0], parts[1]);

        if (row != -1)
        {
            m_contactsModel->item(row, 0)->setCheckState(Qt::Unchecked);
            m_contactsModel->item(row, 5)->setText("");
        }
    }

    updateStatus();
    Session* s = targetSession(name, false);
    m_sessionMap.erase(name.toUtf8().constData());

    if (!s) return;

    s->userGenericEvent("remote-connection-stop", name.toUtf8().constData(), "");
}

void
RvNetworkDialog::updateStatus()
{
    if (m_client)
    {
        int n = m_ui.participantList->count();
        QString t = QString("<big><b>Running</b></big><br>%1 Connection%2").arg(n).arg(n==1?"":"s");
        m_ui.statusLabel->setText(t);
        m_ui.startButton->setText("Stop Network");
        m_ui.nameLineEdit->setDisabled(true);
        m_ui.portLineEdit->setDisabled(true);
        m_ui.resetConfigButton->setDisabled(true);
    }
    else
    {
        m_ui.statusLabel->setText("Not Running");
        m_ui.startButton->setText("Start Network");
        m_ui.nameLineEdit->setDisabled(false);
        m_ui.portLineEdit->setDisabled(false);
        m_ui.resetConfigButton->setDisabled(false);

        int n = m_contactsModel->rowCount();
        for (int i=0; i < n; i++)
        {
            m_contactsModel->item(i, 0)->setCheckState(Qt::Unchecked);
        }
    }
}

void 
RvNetworkDialog::newData(const QString& sender, 
                         const QString& interp, 
                         const QByteArray& data)
{
    DB ("newData from " << UTF8::qconvert(sender) << " interp '" << UTF8::qconvert(interp) << "' " << data.size() << " bytes");
    Session* s = targetSession(sender);
    if (!s) return;

    if (interp.startsWith("PIXELTILE"))
    {
        s->pixelBlockEvent("pixel-block",
                           interp.toUtf8().constData(),
                           data.constData(),
                           data.size());
    }
    else 
    if (interp.startsWith("DATAEVENT"))
    {
        vector<string> tokens;
        stl_ext::tokenize(tokens, interp.toUtf8().constData(), "(),");

        if (tokens.size() < 4)
        {
            cerr << "ERROR: mal-formed DATAEVENT: "
                    << interp.toUtf8().constData()
                    << endl;
        }
        string ename   = tokens[1];
        string etarget = tokens[2];
        string dinterp = tokens[3];
        string senderName = sender.toUtf8().data();

        const TwkApp::Application::Documents& docs = App()->documents();
        if (docs.size())
        {
            string r = s->userRawDataEvent (ename, dinterp, 
                    data.constData(), data.size(), 0, senderName);
            //  XXX no replies from data events.
        }
    }
    else
    {
        cerr << "ERROR: Unknown Data message: " << 
                interp.toUtf8().constData() << endl;
    }
}

void
RvNetworkDialog::newMessage(const QString& sender, 
                            const QString& inmessage)
{
    DB ("newMessage from " << UTF8::qconvert(sender) << " '" << UTF8::qconvert(inmessage) << "'");
    Session* s = targetSession(sender);
    if (!s) return;

    string senderName = sender.toUtf8().data();
    bool isReturnEvent = (inmessage.startsWith("RETURNEVENT "));

    if (isReturnEvent || inmessage.startsWith("EVENT "))
    {
        QString message = inmessage;
        const TwkApp::Application::Documents& docs = App()->documents();
        QString eventName = message.section(" ", 1, 1);
        QString eventTarget = message.section(" ", 2, 2);
        int typeSize = (isReturnEvent) ? 13 : 7;
        message.remove(0, typeSize + eventName.size() + 1 + eventTarget.size());

        string ename   = eventName.toUtf8().data();
        string contents = message.toUtf8().data();

        string r = s->userGenericEvent(ename, contents, senderName);
        if (isReturnEvent) 
        {
            m_client->sendMessage(sender, QString("RETURN %1").arg(r.c_str()));
        }
    }
    else if (inmessage.startsWith("DISCONNECT"))
    {
        DB ("DISCONNECT from " << senderName);
        m_client->disconnectFrom(sender);
        s->userGenericEvent("remote-connection-stop", senderName, "");
    }
}

void
RvNetworkDialog::startConnect()
{
    startConnectDialog("");
}

void
RvNetworkDialog::connectByName(string host, int port)
{
    QString qhost(host.c_str());
    QHostInfo info = QHostInfo::fromName(qhost);
    if (!m_client) toggleServer();

    if (info.error() == QHostInfo::NoError && info.addresses().size())
    {
        m_client->connectTo("unknown", info.addresses()[0], port);
    }
    else
    {
        QHostAddress addr(qhost);
        if (addr.protocol() != QAbstractSocket::UnknownNetworkLayerProtocol)
        {
            m_client->connectTo("unknown", addr, port);
        }
        else
        {
            QMessageBox::critical(this, "Unknown Host Name",
                QString("Could not resolve host name %1").arg(qhost));
        }
    }
}

void
RvNetworkDialog::startConnectDialog(const QString& name)
{
    //
    //  If name is empty, just hide the fields for it
    //

    if (name != "")
    {
        m_connectUI.nameLineEdit->show();
        m_connectUI.nameLabel->show();
        m_connectUI.nameLineEdit->setText(name);
    }
    else
    {
        m_connectUI.nameLineEdit->hide();
        m_connectUI.nameLabel->hide();
    }

    m_connectDialog.setWindowFlags(Qt::Sheet);
    m_connectDialog.setModal(true);
    m_connectDialog.setWindowModality(Qt::WindowModal);

    if (m_connectDialog.exec() == QDialog::Accepted)
    {
        if (!m_client) toggleServer();
        QString hname = m_connectUI.hostnameLineEdit->text();
        QHostInfo info = QHostInfo::fromName(hname);
        int port = m_connectUI.portLineEdit->text().toInt();
        
        if (info.error() == QHostInfo::NoError && info.addresses().size())
        {
            m_client->connectTo(name, info.addresses()[0], port);
        }
        else
        {
            QHostAddress addr(hname);

            if (addr.protocol() != QAbstractSocket::UnknownNetworkLayerProtocol)
            {
                m_client->connectTo(name, addr, port);
            }
            else
            {
                QMessageBox::critical(this, "Unknown Host Name",
                                      QString("Could not resolve host name %1").arg(hname));
            }
        }
    }
}

set<int>
RvNetworkDialog::selectedContacts()
{
    QModelIndexList selection = 
        m_ui.contactTreeView->selectionModel()->selectedIndexes();

    set<int> rows;

    for (int i=0; i < selection.size(); i++)
    {
        const QModelIndex& index = selection[i];
        rows.insert(index.row());
    }

    return rows;
}

void 
RvNetworkDialog::popup(const QPoint& pos)
{
    set<int> rows = selectedContacts();

    if (!rows.empty())
    {
        int row = *rows.begin();

        QStandardItem*  check     = m_contactsModel->item(row, 0);
        QStandardItem*  name      = m_contactsModel->item(row, 1);
        QStandardItem*  host      = m_contactsModel->item(row, 2);
        bool            connected = check->checkState() == Qt::Checked;
        QString         contact   = QString("%1@%2").arg(name->text()).arg(host->text());
        QList<QAction*> actions   = m_contactPopup->actions();

        m_contactPopup->setTitle(contact);
        actions[0]->setText(contact);       // popup "title"
        actions[0]->setEnabled(false);

        actions[2]->setEnabled(!connected); // connect
        actions[3]->setEnabled(connected);  // disconnect
    }

    QWidget* w = (QWidget*)sender();
    m_contactPopup->popup(w->mapToGlobal(pos));
}

void
RvNetworkDialog::popupDisconnect(bool)
{
    set<int> rows = selectedContacts();

    if (!rows.empty())
    {
        int row = *rows.begin();

        QStandardItem* name = m_contactsModel->item(row, 1);
        QStandardItem* host = m_contactsModel->item(row, 2);
        m_connectUI.hostnameLineEdit->setText(host->text());

        QString contact = QString("%1@%2").arg(name->text()).arg(host->text());

        if (confirmDisconnect(contact))
        {
            m_client->signOff(contact);
            m_client->disconnectFrom(contact);
        }
    }
}

void 
RvNetworkDialog::popupConnect(bool)
{
    set<int> rows = selectedContacts();

    if (!rows.empty())
    {
        int row = *rows.begin();

        QStandardItem* name = m_contactsModel->item(row, 1);
        QStandardItem* host = m_contactsModel->item(row, 2);
        m_connectUI.hostnameLineEdit->setText(host->text());

        startConnectDialog(name->text());
    }
}

void
RvNetworkDialog::popupDelete(bool)
{
    //
    //  Delete all the selected rows
    //

    QModelIndexList selection = 
        m_ui.contactTreeView->selectionModel()->selectedIndexes();

    set<int> rows;

    for (int i=0; i < selection.size(); i++)
    {
        const QModelIndex& index = selection[i];
        rows.insert(index.row());
    }

    //
    //  Delete the rows in reverse order (since the other selected row
    //  numbers will not change).
    //

    for (set<int>::reverse_iterator i = rows.rbegin();
         i != rows.rend();
         ++i)
    {
        m_contactsModel->removeRow(*i);
    }
                
    saveSettings();
}

void
RvNetworkDialog::doubleClickContact(const QModelIndex& index)
{
    //
    //  Don't respond if its double click on permissions (that causes
    //  editing).
    //

    if (index.column() == 3 || !index.isValid()) return;

    //
    //  Update the connect dialogs fields from the double clicked on
    //  item and start it up.
    //

    QStandardItem* check = m_contactsModel->item(index.row(), 0);
    QStandardItem* name  = m_contactsModel->item(index.row(), 1);
    QStandardItem* host  = m_contactsModel->item(index.row(), 2);

    //
    //  If we're connected ask about disconnecting. Otherwise, set up
    //  the connection dialog
    //

    if (check->checkState() == Qt::Checked)
    {
        QString contact = QString("%1@%2").arg(name->text()).arg(host->text());

        if (confirmDisconnect(contact))
        {
            m_client->disconnectFrom(contact);
        }
    }
    else
    {
        m_connectUI.hostnameLineEdit->setText(host->text());
        startConnectDialog(name->text());
    }
}

void 
RvNetworkDialog::contactError(const QString& contact, 
                              const QString& machine,
                              const QString& msg)
{
    Session* s = targetSession(contact, false /*doDisconnect*/);

    bool showContactError = true;
    if (s)
    {
        const string r = s->userGenericEvent("remote-contact-error-show", 
                                             contact.toUtf8().constData(), "");
        showContactError = r != "no";
    }
      
    if (showContactError)
    {
        if (contact == "" || contact[0] == '@')
        {
            QMessageBox::critical(this, "Network Error",
                                QString("Connection to %1:\n%2")
                                .arg(machine)
                                .arg(msg));
        }
        else
        {
            QMessageBox::critical(this, "Network Error",
                                QString("Connection to %1:\n%3")
                                .arg(contact)
                                .arg(msg));
        }
    }

    if (!s) return;

    s->userGenericEvent("remote-contact-error", contact.toUtf8().constData(), "");
}

void
RvNetworkDialog::sendSessionEvent(const QString& event,
                                  const QString& contents,
                                  const QString& sender)
{
    RvApp()->userGenericEventOnAll(event.toUtf8().data(),
                                   contents.toUtf8().data(),
                                   sender.toUtf8().data());
}

vector<string>
RvNetworkDialog::contacts() const
{
    vector<string> c;
    
    int n = m_contactsModel->rowCount();

    for (int i=0; i < n; i++)
    {
        c.push_back(m_contactsModel->item(i, 1)->text().toUtf8().data());
    }

    return c;
}

vector<string>
RvNetworkDialog::connections() const
{
    vector<string> c;
    
    int n = m_contactsModel->rowCount();

    for (int i=0; i < n; i++)
    {
        if (m_contactsModel->item(i, 0)->checkState() == Qt::Checked)
        {
            QString cname = m_contactsModel->item(i, 1)->text();
            cname += "@";
            cname += m_contactsModel->item(i, 2)->text();
            c.push_back(cname.toUtf8().data());
        }
    }

    return c;
}

vector<string>
RvNetworkDialog::sessions()
{
    vector<string> c;
    
    int n = m_contactsModel->rowCount();

    for (int i=0; i < n; i++)
    {
        if (m_contactsModel->item(i, 0)->checkState() == Qt::Checked)
        {
            QString cname = m_contactsModel->item(i, 1)->text();
            cname += "@";
            cname += m_contactsModel->item(i, 2)->text();
            c.push_back(m_sessionMap[cname.toUtf8().data()]);
        }
    }

    return c;
}

vector<string>
RvNetworkDialog::applications() const
{
    vector<string> c;
    
    int n = m_contactsModel->rowCount();

    for (int i=0; i < n; i++)
    {
        if (m_contactsModel->item(i, 0)->checkState() == Qt::Checked)
        {
            QString capp = m_contactsModel->item(i, 4)->text();
            c.push_back(capp.toUtf8().data());
        }
    }

    return c;
}

int
RvNetworkDialog::myPort()
{
    return m_ui.portLineEdit->text().toInt();
}

void
RvNetworkDialog::sessionDeleted(const char *sessionName)
{
    DB ("sessionDeleted " << sessionName);
    string sstr(sessionName);
    if (!sstr.size()) return;
    vector<string> contacts;

    for (SessionMap::iterator i = m_sessionMap.begin(); i != m_sessionMap.end(); ++i)
    {
        if ((*i).second == sstr) contacts.push_back ((*i).first);
    }

    for (int i = 0; i < contacts.size(); ++i)
    {
        DB ("disconnecting from " << contacts[i]);
        QList<QListWidgetItem*> items = 
            m_ui.participantList->findItems(contacts[i].c_str(), Qt::MatchExactly);
        for (int j=0; j < items.size(); j++) delete items[i];

        m_sessionMap.erase(contacts[i]);
	if (m_client)
	{
	    m_client->signOff(contacts[i].c_str());
	    m_client->disconnectFrom(contacts[i].c_str());
	}
    }
}

void
RvNetworkDialog::spoofConnectionStream(const QString& fileName, float timeScale, bool verbose)
{
    // XXX mem leak
    StreamConnection* stream = new StreamConnection(this, true);

    if (! stream->spoofStream(UTF8::qconvert(fileName), verbose)) return;

    requestConnection(stream);

    newRemoteContact(stream->remoteContactName());

    connect(stream, SIGNAL(newData(const QString&,
                                            const QString&,
                                            const QByteArray&)),
                    this, SLOT(newData(const QString&,
                                    const QString&,
                                    const QByteArray&)));

    connect(stream, SIGNAL(newMessage(const QString &, const QString &)),
                    this, SLOT(newMessage(const QString &, const QString &)));

    stream->startSpoofing(timeScale);
}


} // Rv
