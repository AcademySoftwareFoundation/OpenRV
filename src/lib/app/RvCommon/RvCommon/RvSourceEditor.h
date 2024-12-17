//
//  Copyright (c) 2013 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __RvCommon__RvSourceEditor__h__
#define __RvCommon__RvSourceEditor__h__
#include <RvCommon/generated/ui_RvSourceEditor.h>
#include <RvCommon/generated/ui_PublishDialog.h>
#include <TwkApp/EventNode.h>
#include <QtWidgets/QMainWindow>
#include <iostream>

class QTextEdit;
class QSyntaxHighlighter;
class QComboBox;

namespace IPCore
{
    class Session;
    class DynamicIPNode;
} // namespace IPCore

namespace Rv
{

    class RvSourceEditor
        : public QMainWindow
        , public TwkApp::EventNode
    {
        Q_OBJECT

    public:
        struct NodeEditState
        {
            NodeEditState(IPCore::DynamicIPNode* n = 0,
                          const std::string& s = "")
                : node(n)
                , source(s)
            {
            }

            IPCore::DynamicIPNode* node;
            std::string source;
        };

        typedef std::map<QWidget*, NodeEditState> NodeEditStateMap;

        RvSourceEditor(IPCore::Session*, QWidget* parent = 0);
        virtual ~RvSourceEditor();

        virtual Result receiveEvent(const TwkApp::Event&);

        void editNode(const std::string& node);
        void initFromNode(IPCore::DynamicIPNode*);

    private:
        void updateSourceMargin(bool);

    public slots:
        void compile();
        void typeChanged(int);
        void sourceChanged();
        void importFile();
        void publish();
        void publishAccepted();
        void revert();
        void deselectAll();
        void downLine();
        void upLine();
        void forwardWord();
        void backWord();
        void tabChanged(int);
        void tabClosed(int);

    private:
        Ui::RvSourceEditor m_ui;
        Ui::PublishDialog m_publishUI;
        QDialog* m_publishDialog;
        QTextEdit* m_textEdit;
        IPCore::Session* m_session;
        QSyntaxHighlighter* m_highlighter;
        QComboBox* m_typeComboBox;
        NodeEditStateMap m_stateMap;
        QWidget* m_currentPage;
        IPCore::DynamicIPNode* m_node;
        bool m_changeLock;
        bool m_updateLock;
    };

} // namespace Rv

#endif // __RvCommon__RvSourceEditor__h__
