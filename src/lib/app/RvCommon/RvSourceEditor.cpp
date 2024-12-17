//
//  Copyright (c) 2013 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//

#include <RvCommon/RvSourceEditor.h>
#include <RvCommon/GLSLSyntaxHighlighter.h>
#include <IPCore/Session.h>
#include <IPCore/NodeDefinition.h>
#include <IPCore/ShaderFunction.h>
#include <IPCore/IPGraph.h>
#include <IPCore/DynamicIPNode.h>
#include <IPCore/PropertyEditor.h>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <QtCore/QDateTime>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QFileDialog>

namespace Rv
{
    using namespace std;
    using namespace TwkApp;
    using namespace boost;
    using namespace IPCore;

    RvSourceEditor::RvSourceEditor(Session* session, QWidget* parent)
        : QMainWindow(parent)
        , EventNode("sourceEditor")
        , m_session(session)
        , m_highlighter(0)
        , m_currentPage(0)
        , m_changeLock(false)
        , m_updateLock(false)
    {
        m_ui.setupUi(this);
        m_publishDialog = new QDialog(this, Qt::Sheet);
        m_publishUI.setupUi(m_publishDialog);

        m_typeComboBox = m_ui.typeComboBox;

        listenTo(&(session->graph()));

        QFont font;
#ifdef PLATFORM_DARWIN
        font.setFamily("Monaco");
#else
        font.setFamily("Courier");
#endif
        font.setFixedPitch(true);
        font.setPointSize(14);

        m_ui.textEdit->setFont(font);
        m_ui.marginTextEdit->setFont(font);
        m_ui.marginTextEdit->setHorizontalScrollBarPolicy(
            Qt::ScrollBarAlwaysOff);
        m_ui.marginTextEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        m_highlighter = new GLSLSyntaxHighlighter(m_ui.textEdit->document());

        // m_typeComboBox->addItem("Source", QVariant("source"));
        m_typeComboBox->addItem("Filter", QVariant("filter"));
        m_typeComboBox->addItem("Color", QVariant("color"));
        m_typeComboBox->addItem("Merge", QVariant("merge"));
        m_typeComboBox->addItem("Transition", QVariant("transition"));
        m_typeComboBox->addItem("Combine", QVariant("combine"));

        connect(m_ui.actionCompile, SIGNAL(triggered()), this, SLOT(compile()));
        connect(m_ui.actionRevert, SIGNAL(triggered()), this, SLOT(revert()));
        connect(m_ui.actionPublish, SIGNAL(triggered()), this, SLOT(publish()));
        connect(m_typeComboBox, SIGNAL(activated(int)), this,
                SLOT(typeChanged(int)));
        connect(m_ui.actionUndo, SIGNAL(triggered()), m_ui.textEdit,
                SLOT(undo()));
        connect(m_ui.actionRedo, SIGNAL(triggered()), m_ui.textEdit,
                SLOT(redo()));
        connect(m_ui.actionCut, SIGNAL(triggered()), m_ui.textEdit,
                SLOT(cut()));
        connect(m_ui.actionCopy, SIGNAL(triggered()), m_ui.textEdit,
                SLOT(copy()));
        connect(m_ui.actionPaste, SIGNAL(triggered()), m_ui.textEdit,
                SLOT(paste()));
        connect(m_ui.actionSelectAll, SIGNAL(triggered()), m_ui.textEdit,
                SLOT(selectAll()));
        connect(m_ui.actionDeselectAll, SIGNAL(triggered()), this,
                SLOT(deselectAll()));
        connect(m_ui.actionBigger_Text, SIGNAL(triggered()), m_ui.textEdit,
                SLOT(zoomIn()));
        connect(m_ui.actionSmaller_Text, SIGNAL(triggered()), m_ui.textEdit,
                SLOT(zoomOut()));
        connect(m_ui.actionBigger_Text, SIGNAL(triggered()),
                m_ui.marginTextEdit, SLOT(zoomIn()));
        connect(m_ui.actionSmaller_Text, SIGNAL(triggered()),
                m_ui.marginTextEdit, SLOT(zoomOut()));
        connect(m_ui.actionBigger_Text, SIGNAL(triggered()),
                m_ui.outputTextEdit, SLOT(zoomIn()));
        connect(m_ui.actionSmaller_Text, SIGNAL(triggered()),
                m_ui.outputTextEdit, SLOT(zoomOut()));
        connect(m_ui.actionNext_Line, SIGNAL(triggered()), this,
                SLOT(downLine()));
        connect(m_ui.actionPrevious_Line, SIGNAL(triggered()), this,
                SLOT(upLine()));
        connect(m_ui.actionNext_Word, SIGNAL(triggered()), this,
                SLOT(forwardWord()));
        connect(m_ui.actionPrevious_Word, SIGNAL(triggered()), this,
                SLOT(backWord()));

        connect(m_ui.textEdit->verticalScrollBar(), SIGNAL(valueChanged(int)),
                m_ui.marginTextEdit->verticalScrollBar(), SLOT(setValue(int)));

        connect(m_ui.textEdit, SIGNAL(textChanged()), this,
                SLOT(sourceChanged()));

        connect(m_ui.tabWidget, SIGNAL(currentChanged(int)), this,
                SLOT(tabChanged(int)));
        connect(m_ui.tabWidget, SIGNAL(tabCloseRequested(int)), this,
                SLOT(tabClosed(int)));

        connect(m_publishDialog, SIGNAL(accepted()), this,
                SLOT(publishAccepted()));
    }

    RvSourceEditor::~RvSourceEditor() {}

    EventNode::Result RvSourceEditor::receiveEvent(const Event& event)
    {
        if (const GenericStringEvent* gevent =
                dynamic_cast<const GenericStringEvent*>(&event))
        {
            if (event.name() == "graph-state-change")
            {
                vector<string> parts;
                algorithm::split(parts, gevent->stringContent(),
                                 is_any_of(string(".")));

                IPNode* node = m_session->graph().findNode(parts.front());

                if (node == m_node)
                {
                    if (parts[1] == "node")
                    {
                        if (parts[2] == "evaluationType")
                        {
                            TwkContainer::StringProperty* sp =
                                node->property<TwkContainer::StringProperty>(
                                    "node.evaluationType");
                            QString etype = sp->front().c_str();

                            int n = m_typeComboBox->count();

                            for (size_t i = 0; i < n; i++)
                            {
                                if (m_typeComboBox->itemData(i).toString()
                                    == etype)
                                {
                                    m_typeComboBox->setCurrentIndex(i);
                                    break;
                                }
                            }
                        }
                    }
                    else if (parts[1] == "function")
                    {
                        if (parts[2] == "glsl")
                        {
                            initFromNode(m_node);
                        }
                    }
                }

                for (NodeEditStateMap::const_iterator i = m_stateMap.begin();
                     i != m_stateMap.end(); ++i)
                {
                    if ((*i).second.node == node)
                    {
                        if (parts[1] == "ui")
                        {
                            if (parts[2] == "name")
                            {
                                int index = m_ui.tabWidget->indexOf((*i).first);
                                m_ui.tabWidget->setTabText(
                                    index, node->uiName().c_str());
                            }
                        }
                    }
                }
            }
        }

        return EventAcceptAndContinue;
    }

    void RvSourceEditor::updateSourceMargin(bool force)
    {
        size_t linesInEditor = m_ui.textEdit->document()->lineCount();
        size_t linesInMargin = m_ui.marginTextEdit->document()->lineCount();

        if (linesInEditor != linesInMargin || force)
        {
            set<int> errorLines;
            set<int> warningLines;

            // Mac/AMD style
            regex errorRE("\\bERROR\\b:?\\s*([0-9]+):([0-9]+):");
            regex warningRE("\\bWARNING\\b:?\\s*([0-9]+):([0-9]+):");
            // NVIDIA style
            regex errorRE2("([0-9]+)\\(([0-9]+)\\)\\s*:\\s*error\\s+");
            regex warningRE2("([0-9]+)\\(([0-9]+)\\)\\s*:\\s*warning\\s+");

            smatch m;
            string messages = m_node->outputMessage();
            vector<string> lines;
            algorithm::split(lines, messages, is_any_of(string("\n")));

            for (size_t i = 0; i < lines.size(); i++)
            {
                if (regex_search(lines[i], m, errorRE))
                {
                    istringstream istr(m[2]);
                    int line;
                    istr >> line;
                    errorLines.insert(line);
                }

                if (regex_search(lines[i], m, errorRE2))
                {
                    istringstream istr(m[2]);
                    int line;
                    istr >> line;
                    errorLines.insert(line);
                }

                if (regex_search(lines[i], m, warningRE))
                {
                    istringstream istr(m[2]);
                    int line;
                    istr >> line;
                    warningLines.insert(line);
                }

                if (regex_search(lines[i], m, warningRE2))
                {
                    istringstream istr(m[2]);
                    int line;
                    istr >> line;
                    warningLines.insert(line);
                }
            }

            m_ui.marginTextEdit->clear();
            QString lineText;

            for (size_t i = 0; i < linesInEditor; i++)
            {
                if (errorLines.count(i + 1))
                {
                    lineText +=
                        QString("<font color=red>%1</font><br>").arg(i + 1);
                }
                else if (warningLines.count(i + 1))
                {
                    lineText +=
                        QString("<font color=orange>%1</font><br>").arg(i + 1);
                }
                else
                {
                    lineText +=
                        QString("<font color=#505050>%1</font><br>").arg(i + 1);
                }
            }

            m_ui.marginTextEdit->setHtml(lineText);
            m_ui.marginTextEdit->verticalScrollBar()->setValue(
                m_ui.textEdit->verticalScrollBar()->value());
        }
    }

    void RvSourceEditor::sourceChanged() { updateSourceMargin(false); }

    void RvSourceEditor::initFromNode(IPCore::DynamicIPNode* node)
    {
        if (m_updateLock)
        {
            if (m_currentPage && m_stateMap.count(m_currentPage) > 0)
            {
                if (m_stateMap[m_currentPage].node == node)
                    return;
            }
        }

        m_changeLock = true;

        if (m_stateMap.empty() && !m_currentPage)
        {
            m_ui.tabWidget->clear();
        }

        if (m_currentPage)
        {
            if (m_stateMap.count(m_currentPage))
            {
                m_stateMap[m_currentPage].source =
                    m_ui.textEdit->toPlainText().toUtf8().constData();
                m_stateMap[m_currentPage].node = m_node;
            }
        }

        m_currentPage = 0;

        for (NodeEditStateMap::const_iterator i = m_stateMap.begin();
             i != m_stateMap.end(); ++i)
        {
            if ((*i).second.node == node)
            {
                m_currentPage = (*i).first;
            }
        }

        if (!m_currentPage)
        {
            m_currentPage = new QWidget(m_ui.tabWidget);
            int index =
                m_ui.tabWidget->addTab(m_currentPage, node->uiName().c_str());
            m_stateMap[m_currentPage] = NodeEditState(node, "");
        }

        m_ui.tabWidget->setCurrentWidget(m_currentPage);

        m_node = node;
        m_ui.textEdit->setPlainText(
            node->property<IPNode::StringProperty>("function.glsl")
                ->front()
                .c_str());
        m_ui.tabWidget->setTabText(m_ui.tabWidget->indexOf(m_currentPage),
                                   m_node->uiName().c_str());
        m_ui.outputTextEdit->clear();
        string etype = m_node->stringValue("node.evaluationType");
        QString es = etype.c_str();

        for (size_t i = 0; i < m_typeComboBox->count(); i++)
        {
            if (m_typeComboBox->itemData(i).toString() == es)
            {
                m_typeComboBox->setCurrentIndex(i);
                break;
            }
        }

        m_changeLock = false;
    }

    void RvSourceEditor::editNode(const string& nodeName)
    {
        if (IPNode* node = m_session->graph().findNode(nodeName))
        {
            if (DynamicIPNode* dnode = dynamic_cast<DynamicIPNode*>(node))
            {
                initFromNode(dnode);
            }
        }
    }

    void RvSourceEditor::typeChanged(int index) {}

    void RvSourceEditor::compile()
    {
        if (m_node)
        {
            m_updateLock = true;

            StringPropertyEditor etype(m_node, "node.evaluationType");
            int index = m_typeComboBox->currentIndex();
            QVariant data = m_typeComboBox->itemData(index);
            etype.setValue(data.toString().toUtf8().constData());

            StringPropertyEditor code(m_node, "function.glsl");
            code.setValue(m_ui.textEdit->toPlainText().toUtf8().constData());

            int n = m_typeComboBox->count();
            string message = m_node->outputMessage();

            for (size_t i = 0; i < n; i++)
            {
                if (m_typeComboBox->itemData(i).toString()
                        == etype.value().c_str()
                    && m_typeComboBox->currentIndex() != i)
                {
                    if (message.size() && message[message.size() - 1] != '\n')
                        message += "\n";
                    message += "INFO: Compile succeeded but evaluation type "
                               "changed to ";
                    message += "\"";
                    message += etype.value();
                    message += "\"";
                    m_typeComboBox->setCurrentIndex(i);
                    break;
                }
            }

            m_session->askForRedraw();

            m_ui.outputTextEdit->clear();

            QDateTime dt = QDateTime::currentDateTime();
            QString dtString = QString("<font color=grey>%1</font>")
                                   .arg(dt.toString("ddd MMMM d h:mm:ss"));

            if (message == "")
            {
                m_ui.outputTextEdit->append(
                    QString("<i><font color=grey>Compilation "
                            "Succeeded</font></i> - %1")
                        .arg(dtString));
            }
            else
            {
                m_ui.outputTextEdit->append(QString("%1").arg(dtString));
            }

            QTextBlockFormat format;
            format.setAlignment(Qt::AlignHCenter);
            QTextCursor cursor = m_ui.outputTextEdit->textCursor();
            cursor.setBlockFormat(format);
            m_ui.outputTextEdit->setTextCursor(cursor);

            if (message != "")
            {
                //
                //  correct weird error messages
                //

                regex manySyntaxRE("syntax error syntax error");
                message = regex_replace(message, manySyntaxRE, "syntax error");

                //
                //  correct use of hashed function name
                //

                regex hashRE("\\b" + m_node->outputFunctionCallName() + "\\b");
                message = regex_replace(message, hashRE,
                                        m_node->outputFunctionName());

                //
                //  Mac/AMD style output
                //

                regex berrorRE("\\bERROR:\\s+(\\D)");
                regex bwarningRE("\\bWARNING:\\s+(\\D)");
                message =
                    regex_replace(message, berrorRE,
                                  "<font color=red><b>ERROR:</b></font> \\1");
                message = regex_replace(
                    message, bwarningRE,
                    "<font color=orange><b>WARNING:</b></font> \\1");

                //
                //  Mac/AMD style output
                //

                regex errorRE("\\bERROR\\b:?\\s*([0-9]+):([0-9]+):");
                regex warningRE("\\bWARNING\\b:?\\s*([0-9]+):([0-9]+):");
                message = regex_replace(
                    message, errorRE,
                    "<font color=red><b>ERROR:</b></font> <b>line \\2: </b>");
                message =
                    regex_replace(message, warningRE,
                                  "<font color=orange><b>WARNING:</b></font> "
                                  "<b>line \\2: </b>");

                //
                //  nvidia style output
                //

                regex errorRE2("[0-9]+\\(([0-9]+)\\)\\s*:\\s*error\\s+");
                regex warningRE2("[0-9]+\\(([0-9]+)\\)\\s*:\\s*warning\\s+");
                message = regex_replace(
                    message, errorRE2,
                    "<font color=red><b>ERROR:</b></font> <b>line \\1: </b>");
                message =
                    regex_replace(message, warningRE2,
                                  "<font color=orange><b>WARNING:</b></font> "
                                  "<b>line \\1: </b>");

                //
                //  Internal style info output
                //

                regex infoRE("\\bINFO\\b:?");
                message = regex_replace(message, infoRE,
                                        "<font color=cyan>INFO:</font>");

                vector<string> lines;
                algorithm::split(lines, message, is_any_of(string("\n")));

                for (size_t i = 0; i < lines.size(); i++)
                {
                    string line = lines[i];

                    if (line != "")
                    {
                        m_ui.outputTextEdit->append(line.c_str());

                        QTextBlockFormat format;
                        format.setAlignment(Qt::AlignLeft);
                        QTextCursor cursor = m_ui.outputTextEdit->textCursor();
                        cursor.setBlockFormat(format);
                        m_ui.outputTextEdit->setTextCursor(cursor);
                    }
                }
            }

            updateSourceMargin(true);

            m_updateLock = false;
        }
    }

    void RvSourceEditor::importFile() {}

    void RvSourceEditor::publish()
    {
        m_publishUI.typeNameEdit->setText(
            m_node->stringValue("node.name").c_str());
        m_publishUI.fileNameEdit->setText(
            m_node->stringValue("node.export").c_str());
        m_publishUI.authorEdit->setText(
            m_node->stringValue("node.author").c_str());
        m_publishUI.companyEdit->setText(
            m_node->stringValue("node.company").c_str());
        m_publishUI.commentEdit->setText(
            m_node->stringValue("node.comment").c_str());
        m_publishUI.fetchesSpinBox->setValue(
            m_node->intValue("function.fetches"));
        m_publishUI.versionSpinBox->setValue(m_node->intValue("node.version"));
        m_publishUI.userVisibleCheckBox->setChecked(
            m_node->intValue("node.userVisible") == 0);
        m_publishDialog->open();

        /*
            QString fileName = QFileDialog::getSaveFileName(this,
                                                            "Publish Node
           Definition",
                                                            ".",
                                                            "Node Definition
           File (*.gto)");
        */

        // m_node->publish(fileName.toUtf8().constData());
    }

    void RvSourceEditor::publishAccepted()
    {
        string typeName = m_publishUI.typeNameEdit->text().toUtf8().constData();
        string fileName = m_publishUI.fileNameEdit->text().toUtf8().constData();
        string author = m_publishUI.authorEdit->text().toUtf8().constData();
        string company = m_publishUI.companyEdit->text().toUtf8().constData();
        string comment = m_publishUI.commentEdit->text().toUtf8().constData();
        int version = m_publishUI.versionSpinBox->value();
        int fetches = m_publishUI.fetchesSpinBox->value();
        bool userVisible = m_publishUI.userVisibleCheckBox->isChecked();

        m_node->setStringValue("node.name", typeName);
        m_node->setStringValue("node.author", author);
        m_node->setStringValue("node.company", company);
        m_node->setStringValue("node.comment", comment);
        m_node->setIntValue("node.version", version);
        m_node->setIntValue("node.userVisible", userVisible ? 1 : 0);
        m_node->setIntValue("function.fetches", fetches);
        m_node->publish(fileName);
    }

    void RvSourceEditor::revert() {}

    void RvSourceEditor::deselectAll()
    {
        QTextCursor cursor = m_ui.textEdit->textCursor();
        cursor.clearSelection();
        m_ui.textEdit->setTextCursor(cursor);
    }

    void RvSourceEditor::downLine()
    {
        QTextCursor cursor = m_ui.textEdit->textCursor();
        cursor.movePosition(QTextCursor::Down);
        m_ui.textEdit->setTextCursor(cursor);
    }

    void RvSourceEditor::upLine()
    {
        QTextCursor cursor = m_ui.textEdit->textCursor();
        cursor.movePosition(QTextCursor::Up);
        m_ui.textEdit->setTextCursor(cursor);
    }

    void RvSourceEditor::forwardWord()
    {
        QTextCursor cursor = m_ui.textEdit->textCursor();
        cursor.movePosition(QTextCursor::NextWord);
        m_ui.textEdit->setTextCursor(cursor);
    }

    void RvSourceEditor::backWord()
    {
        QTextCursor cursor = m_ui.textEdit->textCursor();
        cursor.movePosition(QTextCursor::PreviousWord);
        m_ui.textEdit->setTextCursor(cursor);
    }

    void RvSourceEditor::tabChanged(int index)
    {
        if (m_changeLock)
            return;

        if (QWidget* page = m_ui.tabWidget->widget(index))
        {
            if (m_stateMap.count(page) > 0)
                initFromNode(m_stateMap[page].node);
        }
    }

    void RvSourceEditor::tabClosed(int index)
    {
        if (QWidget* page = m_ui.tabWidget->widget(index))
        {
            if (m_stateMap.count(page) > 0)
            {
                if (m_currentPage == page && m_ui.tabWidget->count() == 1)
                {
                    m_currentPage = 0;
                    m_stateMap.clear();
                    hide();
                    return;
                }
                else
                {
                    m_stateMap.erase(page);
                    m_ui.tabWidget->removeTab(index);
                    initFromNode(m_stateMap.begin()->second.node);
                }
            }
        }
    }

} // namespace Rv
