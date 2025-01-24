//
//  Copyright (c) 2009 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <RvCommon/RvJavaScriptObject.h>
#include <RvCommon/RvDocument.h>
#include <TwkQtCoreUtil/QtConvert.h>
#include <QtWebEngineWidgets/QWebEnginePage>
#include <QtWebEngineWidgets/QWebEngineProfile>
#include <QtWebEngineWidgets/QWebEngineScript>
#include <QtWebEngineWidgets/QWebEngineScriptCollection>
#include <QtWebChannel/QWebChannel>

namespace Rv
{
    using namespace std;
    using namespace TwkApp;
    using namespace TwkQtCoreUtil;

    RvJavaScriptObject::RvJavaScriptObject(RvDocument* doc,
                                           QWebEnginePage* page)
        : QObject(doc)
        , EventNode("jsobject")
        , m_frame(page)
        , m_doc(doc)
    {
        QFile webChannelJsFile(":/qtwebchannel/qwebchannel.js");

        if (!webChannelJsFile.open(QIODevice::ReadOnly))
        {
            QString msg = QString("Couldn't open qwebchannel.js file: %1")
                              .arg(webChannelJsFile.errorString());
            string err = msg.toStdString();
            cout << "ERROR: " << err << endl;
            TWK_THROW_EXC_STREAM(err);
        }
        else
        {
            QByteArray webChannelJs = webChannelJsFile.readAll();
            webChannelJs.append(
                "\nnew QWebChannel(window.qt.webChannelTransport, "
                "function(channel) {window.rvsession = "
                "channel.objects.rvsession;});");
            QWebEngineScript script;
            script.setSourceCode(webChannelJs);
            script.setName("qwebchannel.js");
            script.setWorldId(QWebEngineScript::MainWorld);
            script.setInjectionPoint(QWebEngineScript::DocumentCreation);
            script.setRunsOnSubFrames(false);
            m_frame->scripts().insert(script);
        }

        if (m_frame->webChannel())
        {
            m_channel = m_frame->webChannel();
        }
        else
        {
            m_channel = new QWebChannel(m_frame);
        }

        m_channel->registerObject(QStringLiteral("rvsession"), this);
        m_frame->setWebChannel(m_channel);
        connect(m_frame, SIGNAL(loadFinished(bool)), this, SLOT(emitReady()));

        listenTo(m_doc->session());
    }

    RvJavaScriptObject::~RvJavaScriptObject() {}

    void RvJavaScriptObject::emitReady()
    {
        m_frame->runJavaScript(
            "var event = new CustomEvent(\"rvsession-ready\",{}); "
            "document.dispatchEvent(event);");
    }

    QString RvJavaScriptObject::evaluate(const QString& code)
    {
        string rval = m_doc->session()->userGenericEvent(
            "remote-eval", UTF8::qconvert(code),
            UTF8::qconvert(m_frame->url().toString()));
        return UTF8::qconvert(rval.c_str());
    }

    QString RvJavaScriptObject::pyevaluate(const QString& code)
    {
        string rval = m_doc->session()->userGenericEvent(
            "remote-pyeval", UTF8::qconvert(code),
            UTF8::qconvert(m_frame->url().toString()));
        return UTF8::qconvert(rval.c_str());
    }

    void RvJavaScriptObject::pyexec(const QString& code)
    {
        (void)m_doc->session()->userGenericEvent(
            "remote-pyexec", UTF8::qconvert(code),
            UTF8::qconvert(m_frame->url().toString()));
    }

    QString RvJavaScriptObject::sendInternalEvent(const QString& eventName,
                                                  const QString& contents,
                                                  const QString& sender)
    {
        QString s = (sender == "") ? m_frame->url().toString() : sender;

        string rval = m_doc->session()->userGenericEvent(
            UTF8::qconvert(eventName), UTF8::qconvert(contents),
            UTF8::qconvert(s));
        return UTF8::qconvert(rval.c_str());
    }

    void RvJavaScriptObject::bindToRegex(const QString& name)
    {
        QRegExp re(name);
        if (!m_eventNames.contains(re))
            m_eventNames.push_back(re);
    }

    void RvJavaScriptObject::unbindRegex(const QString& name)
    {
        m_eventNames.removeOne(QRegExp(name));
    }

    bool RvJavaScriptObject::hasBinding(const QString& name)
    {
        return m_eventNames.contains(QRegExp(name));
    }

    EventNode::Result
    RvJavaScriptObject::receiveEvent(const TwkApp::Event& event)
    {
        QString name = event.name().c_str();

        for (size_t i = 0; i < m_eventNames.size(); i++)
        {
            if (m_eventNames[i].indexIn(name) != -1)
            {
                if (const GenericStringEvent* ge =
                        dynamic_cast<const GenericStringEvent*>(&event))
                {
                    emit eventString(ge->name().c_str(),
                                     ge->stringContent().c_str(),
                                     ge->senderName().c_str());
                }
                else if (const KeyEvent* ke =
                             dynamic_cast<const KeyEvent*>(&event))
                {
                    emit eventKey(ke->name().c_str(), ke->key(),
                                  ke->modifiers());
                }
                else if (const PointerEvent* pe =
                             dynamic_cast<const PointerEvent*>(&event))
                {
                    float atime = 0.0;

                    if (const PointerButtonPressEvent* bp =
                            dynamic_cast<const PointerButtonPressEvent*>(pe))
                    {
                        atime = bp->activationTime();
                    }

                    if (const DragDropEvent* dd =
                            dynamic_cast<const DragDropEvent*>(pe))
                    {
                        QString t, ct;

                        switch (dd->type())
                        {
                        case DragDropEvent::Enter:
                            t = "enter";
                            break;
                        case DragDropEvent::Leave:
                            t = "leave";
                            break;
                        case DragDropEvent::Move:
                            t = "move";
                            break;
                        case DragDropEvent::Release:
                            t = "release";
                            break;
                        }

                        switch (dd->contentType())
                        {
                        case DragDropEvent::File:
                            ct = "file";
                            break;
                        case DragDropEvent::URL:
                            ct = "url";
                            break;
                        case DragDropEvent::Text:
                            ct = "text";
                            break;
                        }

                        emit eventDragDrop(pe->name().c_str(), pe->x(), pe->y(),
                                           pe->w(), pe->h(), pe->startX(),
                                           pe->startY(),
                                           (int)pe->buttonStates(), t, ct,
                                           dd->stringContent().c_str());
                    }
                    else
                    {
                        emit eventPointer(pe->name().c_str(), pe->x(), pe->y(),
                                          pe->w(), pe->h(), pe->startX(),
                                          pe->startY(), pe->buttonStates(),
                                          atime);
                    }
                }

                break;
            }
        }

        //
        //  Don't EventNode::Accept here -- if you do then other webkit
        //  panes won't get the event.
        //

        return EventNode::EventIgnored;
    }

} // namespace Rv
