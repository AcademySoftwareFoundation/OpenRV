//
//  Copyright (c) 2009 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __RvCommon__RvJavaScriptObject__h__
#define __RvCommon__RvJavaScriptObject__h__
#include <iostream>
#include <QtCore/QtCore>
#include <TwkApp/Event.h>
#include <TwkApp/EventNode.h>

class QWebEnginePage;
class QWebChannel;

namespace Rv
{
    class RvDocument;

    class RvJavaScriptObject
        : public QObject
        , public TwkApp::EventNode
    {
        Q_OBJECT

    public:
        RvJavaScriptObject(RvDocument*, QWebEnginePage* frame);
        virtual ~RvJavaScriptObject();

        virtual Result receiveEvent(const TwkApp::Event&);

    public slots:
        QString evaluate(const QString& mu);
        QString pyevaluate(const QString& py);
        void pyexec(const QString& py);
        QString sendInternalEvent(const QString& eventName,
                                  const QString& contents,
                                  const QString& sender);
        void bindToRegex(const QString& nameRegex);
        void unbindRegex(const QString& nameRegex);
        bool hasBinding(const QString& nameRegex);
        void emitReady();

    signals:
        //
        //  These are the TwkApp::Event types:
        //
        //      ActivityChangeEvent        DragDropEvent
        //      Event                      GenericStringEvent
        //      KeyEvent                   KeyPressEvent
        //      KeyReleaseEvent            ModifierEvent
        //      PointerButtonPressEvent
        //      PointerButtonReleaseEvent  PointerEvent
        //      RawDataEvent               RenderContextChangeEvent
        //      RenderEvent                TabletEvent
        //
        //  Each of these should become a signal with the relevant info as
        //  args. Some of them can be merged (e.g. PointerButtonPressEvent
        //  and PointerButtonReleaseEvent) by just supplying the name of
        //  the event as well. So in the pointer case we only need a
        //  pointerEvent() in javascript which indicates which of the
        //  pointer events it actually was along with the data.
        //

        void eventString(const QString& eventName, const QString& contents,
                         const QString& senderName);

        void eventKey(const QString& eventName, unsigned int key,
                      unsigned int modifiers);

        void eventPointer(const QString& eventName, int x, int y, int w, int h,
                          int startX, int startY, int buttonStates,
                          float activationTime);

        void eventDragDrop(const QString& eventName, int x, int y, int w, int h,
                           int startX, int startY, int buttonStates,
                           const QString& dragDropType,
                           const QString& contentType, const QString& contents);

    private:
        RvDocument* m_doc;

        QWebChannel* m_channel;

        QWebEnginePage* m_frame;

        QList<QRegExp> m_eventNames;
    };

} // namespace Rv

#endif // __RvCommon__RvJavaScriptObject__h__
