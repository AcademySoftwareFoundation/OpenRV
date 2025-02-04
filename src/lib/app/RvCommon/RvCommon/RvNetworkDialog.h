//
//  Copyright (c) 2008 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __RvCommon__RvNetworkDialog__h__
#define __RvCommon__RvNetworkDialog__h__
#include <RvCommon/generated/ui_RvNetworkDialog.h>
#include <RvCommon/generated/ui_RvNetworkConnect.h>
#include <iostream>
#include <set>
#include <map>
#include <QtWidgets/QDialog>
#include <QtGui/QStandardItemModel>

#include <IPCore/Session.h>

namespace TwkQtChat
{
    class Client;
    class Connection;
} // namespace TwkQtChat

namespace Rv
{

    class RvNetworkDialog : public QWidget
    {
        Q_OBJECT

    public:
        typedef TwkQtChat::Client Client;
        typedef QMap<QString, QVariant> ContactMap;
        typedef std::map<std::string, std::string> SessionMap;

        enum ConnectPermission
        {
            AskConnect = 0,
            AllowConnect,
            DenyConnect,
            CurrentConnect
        };

        RvNetworkDialog(QWidget*);
        ~RvNetworkDialog();

        TwkQtChat::Client* client() { return m_client; }

        std::vector<std::string> contacts() const;
        std::vector<std::string> connections() const;
        std::vector<std::string> applications() const;
        std::vector<std::string> sessions();

        void spoofConnectionStream(const QString& fileName,
                                   float timeScale = 1.0, bool verbose = false);

        std::string localContactName()
        {
            return std::string(m_ui.nameLineEdit->text().toUtf8().constData());
        }

        void setLocalContactName(std::string n)
        {
            m_ui.nameLineEdit->setText(n.c_str());
        }

        int defaultPermission() { return m_ui.permissionCombo->currentIndex(); }

        void setDefaultPermission(int i)
        {
            m_ui.permissionCombo->setCurrentIndex(i);
        }

    public slots:
        void newRemoteContact(const QString&);
        void newData(const QString&, const QString&, const QByteArray&);
        void newMessage(const QString&, const QString&);
        void remoteContactLeft(const QString&);
        void toggleServer();

        bool serverRunning() { return (m_client != 0); };

        int myPort();
        void connectByName(std::string host, int port);
        void requestConnection(TwkQtChat::Connection*);
        void startConnect();
        void popupConnect(bool);
        void popupDelete(bool);
        void popupDisconnect(bool);
        void popup(const QPoint&);
        void doubleClickContact(const QModelIndex&);
        void resetConfig();
        void contactError(const QString&, const QString&, const QString&);

        void shutdownServer();
        void sessionDeleted(const char* name);

    private:
        void startConnectDialog(const QString&);
        void updateStatus();
        void loadSettings();
        void saveSettings();
        void closeEvent(QCloseEvent* event);

        int findContact(const QString& name, const QString& host);

        void addContact(const QString& name, const QString& host,
                        TwkQtChat::Connection* connection, unsigned int perms,
                        const QString& capp, bool checkForExisting = true);

        ConnectPermission contactPermission(const QStringList& parts);

        void sendSessionEvent(const QString&, const QString&, const QString&);

        bool confirmDisconnect(const QString&);
        std::set<int> selectedContacts();

        IPCore::Session* targetSession(QString sender,
                                       bool doDisconnect = true);

        void savePortNumber();
        void deletePortNumberFile();

    private:
        Ui::RvNetworkDialog m_ui;
        QDialog m_connectDialog;
        Ui::RvNetworkConnect m_connectUI;
        Client* m_client;
        QStandardItemModel* m_contactsModel;
        QMenu* m_contactPopup;
        SessionMap m_sessionMap;
        QString m_portFile;
    };

} // namespace Rv

#endif // __RvCommon__RvNetworkDialog__h__
