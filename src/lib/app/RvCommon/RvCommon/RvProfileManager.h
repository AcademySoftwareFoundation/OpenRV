//
//  Copyright (c) 2013 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __RvCommon__RvProfileManager__h__
#define __RvCommon__RvProfileManager__h__
#include <RvCommon/generated/ui_RvProfileManager.h>
#include <RvCommon/generated/ui_CreateProfileDialog.h>
#include <QtGui/QStandardItemModel>
#include <QtCore/QItemSelection>
#include <iostream>

namespace IPCore
{
    class Profile;
}

namespace Rv
{

    class RvProfileManager : public QDialog
    {
        Q_OBJECT

    public:
        typedef QPair<QString, QString> QStringPair;
        typedef std::vector<IPCore::Profile*> ProfileVector;

        RvProfileManager(QObject*);
        virtual ~RvProfileManager();

        void loadModel();

    public slots:
        void addProfile();
        void deleteProfile();
        void selectionChanged(const QItemSelection&, const QItemSelection&);
        void applyProfile();

    private:
        void clear();
        IPCore::Profile* currentProfile();

    private:
        Ui::RvProfileManager m_ui;
        Ui::CreateProfileDialog m_createDialogUI;
        QDialog* m_createDialog;
        QStandardItemModel* m_model;
        ProfileVector m_profiles;
    };

} // namespace Rv

#endif // __RvCommon__RvProfileManager__h__
