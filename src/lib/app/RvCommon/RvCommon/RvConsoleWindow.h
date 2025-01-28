//******************************************************************************
// Copyright (c) 2008 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __RvCommon__RvConsoleWindow__h__
#define __RvCommon__RvConsoleWindow__h__

#include <RvCommon/generated/ui_RvConsoleWindow.h>
#include <QtCore/QtCore>
#include <RvApp/Options.h>
#include <TwkUtil/FileLogger.h>
#include <sstream>

namespace Rv
{

    class RvConsoleWindow : public QDialog
    {
        Q_OBJECT

    public:
        class ConsoleBuf : public std::streambuf
        {
        public:
            ConsoleBuf(RvConsoleWindow* w)
                : m_console(w)
            {
            }

            int sync();
            int overflow(int ch);
            std::streamsize xsputn(char* text, std::streamsize n);

            void sendLine();

            RvConsoleWindow* m_console;
            std::string m_lineBuffer;
            QMutex m_mutex;
        };

        RvConsoleWindow(QWidget* parent = 0);
        virtual ~RvConsoleWindow();

        int append(const char* text, size_t n);
        void append(const std::string& text);

        QTextEdit* textEdit() const { return m_ui.textEdit; }

        void appendLine(const std::string&);
        bool processLine(std::string&, QString& html);
        bool processAndDisplayLine(std::string&);

        void processTimer();

        // To be called in place of processTextBuffer (see below) prior to
        // exiting the process exit(-1). This will prevent some funny business
        // in the exit() function such as trying to start QtTimers in
        // ConsoleBuf::sync() for example
        void processLastTextBuffer();

    public slots:
        void processTextBuffer();

    private:
        void closeEvent(QCloseEvent*);

    private:
        QMutex m_lock;
        Ui::RvConsoleDialog m_ui;
        std::ostream* m_cout;
        std::ostream* m_cerr;
        std::streambuf* m_stdoutBuf;
        std::streambuf* m_stderrBuf;
        ConsoleBuf* m_consoleBuf;
        std::stringstream m_textBuffer;
        QTimer* m_processTimer;
        bool m_processTimerRunning;
        TwkUtil::FileLogger m_fileLogger;
    };

} // namespace Rv

#endif // __RvCommon__RvConsoleWindow__h__
