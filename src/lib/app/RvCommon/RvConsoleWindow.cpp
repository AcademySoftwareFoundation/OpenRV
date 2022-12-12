//******************************************************************************
// Copyright (c) 2008 Tweak Inc. 
// All rights reserved.
// 
// SPDX-License-Identifier: Apache-2.0
// 
//******************************************************************************



#include <RvCommon/QTUtils.h>
#include <RvCommon/RvConsoleWindow.h>
#include <RvPackage/PackageManager.h>
#include <iostream>
#include <stl_ext/string_algo.h>

namespace
{
    //--------------------------------------------------------------------------
    // 
    bool stringPotentiallyContainsHtml( const std::string& s )
    {
      return !s.empty()                          &&
              s.find( '<' ) != std::string::npos &&
              s.find( '>' ) != std::string::npos;
    } 
} // namespace


namespace Rv {
using namespace std;

int 
RvConsoleWindow::ConsoleBuf::sync()
{
    m_console->processTimer();
    streamsize n = pptr() - pbase();
    return (n && m_console->append(pbase(), n) != n) ? EOF : 0;
}

void
RvConsoleWindow::ConsoleBuf::sendLine()
{
    m_console->processTimer();
    m_console->appendLine(m_lineBuffer);
    m_lineBuffer.clear();
}

int 
RvConsoleWindow::ConsoleBuf::overflow(int ch)
{
    QMutexLocker l(&m_mutex);
    streamsize n = pptr() - pbase();

    if (n && sync()) return EOF;

    if (ch != EOF)
    {
        m_lineBuffer.append(1, (char)ch);
        if (ch == '\n') sendLine();
    }

    pbump(-n);  // Reset pptr().
    return 0;
}

streamsize 
RvConsoleWindow::ConsoleBuf::xsputn(char *text, streamsize n)
{
    QMutexLocker l(&m_mutex);
    return sync() == EOF ? 0 : m_console->append(text, n);
}

//----------------------------------------------------------------------

static QThread *mainThread = 0; 

RvConsoleWindow::RvConsoleWindow(QWidget* parent) 
    : QDialog(parent, Qt::Window),
      m_consoleBuf(0),
      m_processTimerRunning(false),
      m_cout(0),
      m_cerr(0),
      m_stdoutBuf(0),
      m_stderrBuf(0)
{
    mainThread = QThread::currentThread();
    m_processTimer = new QTimer(this);
    m_processTimer->setSingleShot(true);
    m_processTimer->setInterval(500);
    connect(m_processTimer, SIGNAL(timeout()), this, SLOT(processTextBuffer()));

    m_ui.setupUi(this);
    m_ui.biggerButton->setIcon(colorAdjustedIcon(":images/zoomi_32x32.png"));
    m_ui.smallerButton->setIcon(colorAdjustedIcon(":images/zoomo_32x32.png"));
    m_ui.clearButton->setIcon(colorAdjustedIcon(":images/del_32x32.png"));
    m_ui.textEdit->setReadOnly(true);
    //
    //  Set focus to combo box, otherwise focus defaults to the "clear"
    //  push button, and if you happen to hit "space" (on linux) it 
    //  triggers the button.
    //
    m_ui.showComboBox->setFocus();

    setWindowTitle("Open RV Console");
    setSizeGripEnabled(true);
    bool doRedirect = (getenv("RV_NO_CONSOLE_REDIRECT") == 0);
    //setAttribute(Qt::WA_MacBrushedMetal);

#if defined(NDEBUG) || defined(PLATFORM_WINDOWS)
    if (doRedirect)
    {
	m_stdoutBuf = cout.rdbuf();
	m_stderrBuf = cerr.rdbuf();
	
	m_consoleBuf = new ConsoleBuf(this);
	
	cout.rdbuf(m_consoleBuf);
	cerr.rdbuf(m_consoleBuf);
	
	m_cout = new ostream(m_stdoutBuf);
	m_cerr = new ostream(m_stderrBuf);
    }
#endif

    RV_QSETTINGS;

    settings.beginGroup("Console");
    m_ui.showComboBox->setCurrentIndex(settings.value("showOn", 0).toInt());
    settings.endGroup();
    int showIndex = m_ui.showComboBox->currentIndex();
}

RvConsoleWindow::~RvConsoleWindow()
{
    if (m_consoleBuf) m_consoleBuf->sync();
    processTextBuffer();

#if defined(NDEBUG) || !defined(PLATFORM_WINDOWS)
    if (m_stdoutBuf) cout.rdbuf(m_stdoutBuf);
    if (m_stderrBuf) cerr.rdbuf(m_stderrBuf);
#endif
}


void 
RvConsoleWindow::processTimer()
{
    if (!m_processTimerRunning)
    {
        //
        //  Only allow the thread that created the timer (the main
        //  display thread) to start the timer (other threads don't
        //  have event loops).
        //
        if (QThread::currentThread() == mainThread)
        {
            m_processTimer->start();
            m_processTimerRunning = true;
        }
    }
}

void 
RvConsoleWindow::closeEvent(QCloseEvent* event)
{
    int showIndex = m_ui.showComboBox->currentIndex();

    RV_QSETTINGS;

    settings.beginGroup("Console");
    settings.setValue("showOn", showIndex);
    settings.endGroup();
    settings.sync();

    QWidget::closeEvent(event);
}

int
RvConsoleWindow::append(const char* text, size_t n)
{
    QMutexLocker l(&m_lock);
    for (int i=0; i < n; i++) m_textBuffer << text[i];
    return n;
}

void
RvConsoleWindow::append(const string& text)
{
    QMutexLocker l(&m_lock);
    m_textBuffer << text;
}

void
RvConsoleWindow::appendLine(const std::string& lineBuffer)
{
    if (m_cout)
    {
        QMutexLocker l(&m_lock);
        m_textBuffer << lineBuffer;
    }
}

void
RvConsoleWindow::processTextBuffer()
{
    if (!m_textBuffer.str().empty())
    {
        vector<string> lines;
        stl_ext::tokenize(lines, m_textBuffer.str(), "\n\r");

        bool shouldShow = false;

        textEdit()->moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);

        // QTextEdit::insertHtml() is slow so we use it only when necessary
        if ( stringPotentiallyContainsHtml( m_textBuffer.str() ) )
        {
            QString html;

            for (int i=0; i < lines.size(); i++) 
            {
                lines[i] += "\n";
                shouldShow |= processLine(lines[i], html);
            }
            html += "<br>";
            
            textEdit()->insertHtml( html );
        } 
        else
        {
            for (int i=0; i < lines.size(); i++)
            {
                lines[i] += "\n";
                shouldShow |= processAndDisplayLine(lines[i]);
            }
        }

        if (shouldShow)
        {
            show();
            raise();
        }

        m_textBuffer.str("");
    }

    if (m_processTimerRunning)
    {
        m_processTimer->stop();
        m_processTimerRunning = false;
    }
}

void
RvConsoleWindow::processLastTextBuffer()
{
    processTextBuffer();

    if ( m_consoleBuf )
    {
        if (m_stdoutBuf) cout.rdbuf(m_stdoutBuf);
        m_stdoutBuf = nullptr;
        if (m_stderrBuf) cerr.rdbuf(m_stderrBuf);
        m_stderrBuf = nullptr;

        delete m_consoleBuf;
        m_consoleBuf = nullptr;
    }
}

bool 
RvConsoleWindow::processLine(string& line, QString& html)
{
    bool error = line.find("ERROR:") == 0; 
    bool warning = line.find("WARNING:") == 0; 
    bool info = line.find("INFO:") == 0; 
    bool debug = line.find("DEBUG:") == 0; 

    bool qtimerwarning = line.find("Application asked to unregister timer") != string::npos; 
    bool qfilesystemwatcher = line.find("QFileSystemWatcher:") != string::npos;

    // filter this
    if (qtimerwarning || qfilesystemwatcher) return false;

    ostream* out = m_cout;

    if (error)
    {
        html += "<font color=red>ERROR:</font> ";
        line.erase(0, 6);
        out = m_cerr;
        *out << "ERROR:";
    }
    else if (warning)
    {
        html += "<font color=orange>WARNING:</font> ";
        line.erase(0, 8);
        out = m_cerr;
        *out << "WARNING:";
    }
    else if (info)
    {
        html += "<font color=cyan>INFO:</font> ";
        line.erase(0, 5);
        *out << "INFO:";
    }
    else if (debug)
    {
        html += "<font color=green>DEBUG:</font> ";
        line.erase(0, 6);
        *out << "DEBUG:";
    }

    html += line.c_str();

    if (line.size() && line[0] != '<') 
    {
        *out << line;
        html += "<br>";
    }

    int showIndex = m_ui.showComboBox->currentIndex();

    //
    //  Should we show the Console as a result of this line ?
    //
    return (showIndex != 4 &&
        ((error && showIndex <= 3) ||
         (warning && showIndex <= 3 && showIndex >= 1) ||
         (info && showIndex <= 3 && showIndex >= 2) ||
         showIndex == 3));
}

bool 
RvConsoleWindow::processAndDisplayLine(string& line)
{
    bool error = line.find("ERROR:") == 0; 
    bool warning = line.find("WARNING:") == 0; 
    bool info = line.find("INFO:") == 0; 
    bool debug = line.find("DEBUG:") == 0; 

    bool qtimerwarning = line.find("Application asked to unregister timer") != string::npos; 
    bool qfilesystemwatcher = line.find("QFileSystemWatcher:") != string::npos;

    // filter this
    if (qtimerwarning || qfilesystemwatcher) return false;

    ostream* out = m_cout;

    if (error)
    {
        textEdit()->setTextColor( Qt::red ); 
        textEdit()->insertPlainText( "ERROR:" );
        line.erase(0, 6);
        out = m_cerr;
        *out << "ERROR:";
    }
    else if (warning)
    {
        static const QColor orangeColor(255, 175, 0);
        textEdit()->setTextColor( orangeColor ); 
        textEdit()->insertPlainText( "WARNING:" );
        line.erase(0, 8);
        out = m_cerr;
        *out << "WARNING:";
    }
    else if (info)
    {
        textEdit()->setTextColor( Qt::cyan );
        textEdit()->insertPlainText( "INFO:" );
        line.erase(0, 5);
        *out << "INFO:";
    }
    else if (debug)
    {
        textEdit()->setTextColor( Qt::green );
        textEdit()->insertPlainText( "DEBUG:" );
        line.erase(0, 6);
        *out << "DEBUG:";
    }

    textEdit()->setTextColor( Qt::white );
    textEdit()->insertPlainText( line.c_str() );
    *out << line;

    int showIndex = m_ui.showComboBox->currentIndex();

    //
    //  Should we show the Console as a result of this line ?
    //
    return (showIndex != 4 &&
        ((error && showIndex <= 3) ||
         (warning && showIndex <= 3 && showIndex >= 1) ||
         (info && showIndex <= 3 && showIndex >= 2) ||
         showIndex == 3));
}

} // Rv


