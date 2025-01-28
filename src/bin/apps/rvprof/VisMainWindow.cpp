//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "VisMainWindow.h"
#include <iostream>
#include <TwkGLText/TwkGLText.h>
#include <sstream>
#ifdef PLATFORM_DARWIN
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif

using namespace TwkGLText;

inline void glVertex(const TwkMath::Vec3f& v) { glVertex3fv((GLfloat*)(&v)); }

inline void glVertex(const TwkMath::Vec4f& v) { glVertex4fv((GLfloat*)(&v)); }

inline TwkMath::Mat44f getMatrix(GLenum matrix)
{
    GLfloat m[16];
    glGetFloatv(matrix, m);

    // Unfortunately, open GL stores matrices in COLUMN-major
    // format, whereas we store them in the much more sane ROW-major
    // format.
    return TwkMath::Mat44f(m[0], m[4], m[8], m[12], m[1], m[5], m[9], m[13],
                           m[2], m[6], m[10], m[14], m[3], m[7], m[11], m[15]);
}

inline void glLoadMatrix(const TwkMath::Mat44f& m)
{
    GLfloat mv[16];

    mv[0] = m[0][0];
    mv[1] = m[1][0];
    mv[2] = m[2][0];
    mv[3] = m[3][0];

    mv[4] = m[0][1];
    mv[5] = m[1][1];
    mv[6] = m[2][1];
    mv[7] = m[3][1];

    mv[8] = m[0][2];
    mv[9] = m[1][2];
    mv[10] = m[2][2];
    mv[11] = m[3][2];

    mv[12] = m[0][3];
    mv[13] = m[1][3];
    mv[14] = m[2][3];
    mv[15] = m[3][3];

    glLoadMatrixf(mv);
}

inline void glMultMatrix(const TwkMath::Mat44f& m)
{
    GLfloat mv[16];

    mv[0] = m[0][0];
    mv[1] = m[1][0];
    mv[2] = m[2][0];
    mv[3] = m[3][0];

    mv[4] = m[0][1];
    mv[5] = m[1][1];
    mv[6] = m[2][1];
    mv[7] = m[3][1];

    mv[8] = m[0][2];
    mv[9] = m[1][2];
    mv[10] = m[2][2];
    mv[11] = m[3][2];

    mv[12] = m[0][3];
    mv[13] = m[1][3];
    mv[14] = m[2][3];
    mv[15] = m[3][3];

    glMultMatrixf(mv);
}

using namespace std;
using namespace TwkMath;

//----------------------------------------------------------------------

VisMainWindow::VisMainWindow(QWidget* parent)
    : m_mouseTime(0.0)
    , QMainWindow(parent)
{
    m_ui.setupUi(this);

    m_dockWidget = new QDockWidget(this);
    m_dockUI.setupUi(m_dockWidget);
    addDockWidget(Qt::BottomDockWidgetArea, m_dockWidget);
    m_dockWidget->show();

    m_fileViewDialog = new QDialog(NULL);
    m_fileViewUI.setupUi(m_fileViewDialog);

    connect(m_ui.actionOpen, SIGNAL(triggered()), this, SLOT(openFile()));
    connect(m_ui.actionQuit, SIGNAL(triggered()), this, SLOT(quit()));
    connect(m_ui.actionShow_Raw_Profile_Data, SIGNAL(triggered()), this,
            SLOT(showFile()));
    connect(m_dockUI.startEdit, SIGNAL(editingFinished()), this,
            SLOT(rangeChanged()));
    connect(m_dockUI.endEdit, SIGNAL(editingFinished()), this,
            SLOT(rangeChanged()));

    connect(m_dockUI.startTimeEdit, SIGNAL(editingFinished()), this,
            SLOT(rangeChanged()));
    connect(m_dockUI.endTimeEdit, SIGNAL(editingFinished()), this,
            SLOT(rangeChanged()));

    connect(m_dockUI.showIdealFramesButton, SIGNAL(stateChanged(int)), this,
            SLOT(showIdealFrames(int)));
    connect(m_dockUI.showEvalTimingButton, SIGNAL(stateChanged(int)), this,
            SLOT(showEvalTiming(int)));
    connect(m_dockUI.actualFPSEdit, SIGNAL(editingFinished()), this,
            SLOT(fpsChanged()));

    m_glWidget = new GLView(this, m_dockUI.readoutEdit, this);
    setCentralWidget(m_glWidget);
    m_glWidget->show();

    qApp->installEventFilter(this);
}

bool VisMainWindow::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::FileOpen)
    {
        QFileOpenEvent* fileEvent = static_cast<QFileOpenEvent*>(event);
        QString filename = fileEvent->file().toUtf8().data();

        if (filename == "")
        {
            filename = fileEvent->url().path();
        }

        readFile(filename);
        return true;
    }

    return QMainWindow::eventFilter(obj, event);
}

void VisMainWindow::openFile()
{
    QString filename = QFileDialog::getOpenFileName(
        this, "Select File", ".",
        UI_APPLICATION_NAME " Profile Data (*.rvprof *.timedata)");

    readFile(filename);
}

void VisMainWindow::readFile(const QString& filename)
{
    QFile infile(filename);

    m_fileViewUI.plainTextEdit->clear();

    if (!infile.exists())
    {
        cerr << "ERROR: file doesn't exist: " << filename.toUtf8().constData()
             << endl;
        return;
    }

    if (!infile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        cerr << "ERROR: can't open: " << filename.toUtf8().constData() << endl;
        return;
    }

    QFileInfo info(filename);
    setWindowTitle(QString(UI_APPLICATION_NAME " Profile Viewer -- %1")
                       .arg(info.baseName()));

    DataVector data;

    QString fileContents;

    while (!infile.atEnd())
    {
        QByteArray array = infile.readLine();
        QString line(array);
        fileContents += line;
        if (line.startsWith("#") || line == "")
            continue;
        QStringList parts = line.split(",");

        if (line.contains("="))
        {
            //
            //  NEW FORMAT
            //

            DataElement e;
#define OFFSET_PAIR(NAME, FIELD)                     \
    {#NAME "0", ((char*)&e.FIELD##0) - ((char*)&e)}, \
        {#NAME "1", ((char*)&e.FIELD##1) - ((char*)&e)}

#define OFFSET_SINGLE(NAME, FIELD) {#NAME, ((char*)&e.FIELD) - ((char*)&e)}

            NameOffsetPair offsets[] = {OFFSET_PAIR(R, render),
                                        OFFSET_PAIR(S, swap),
                                        OFFSET_PAIR(E, eval),
                                        OFFSET_PAIR(U, userRender),
                                        OFFSET_PAIR(FC, frameChange),
                                        OFFSET_PAIR(IR, internalRender),
                                        OFFSET_PAIR(PR, prefetch),
                                        OFFSET_PAIR(CT, cacheTest),
                                        OFFSET_PAIR(EI, evalGraph),
                                        OFFSET_PAIR(ID, evalID),
                                        OFFSET_PAIR(CQ, cacheQuery),
                                        OFFSET_PAIR(CE, cacheEval),
                                        OFFSET_PAIR(IO, io),
                                        OFFSET_PAIR(THA, restartA),
                                        OFFSET_PAIR(THB, restartB),
                                        OFFSET_PAIR(CTL, cacheTestLock),
                                        OFFSET_PAIR(DSDP, setDisplayFrame),
                                        OFFSET_PAIR(FCT, frameCachedTest),
                                        OFFSET_PAIR(WAK, awaken),
                                        OFFSET_PAIR(PRR, prefetchRender),
                                        {0, 0}};

            memset(&e, 0, sizeof(DataElement));

            for (size_t i = 0; i < parts.size(); i++)
            {
                QStringList nameValue = parts[i].split("=");

                if (nameValue.size() != 2)
                {
                    cout << "ERROR: reading file "
                         << filename.toUtf8().constData() << endl;
                    cout << "ERROR: reading this field: \""
                         << parts[i].toUtf8().constData() << "\"" << endl;
                    continue;
                }

                for (const NameOffsetPair* p = offsets; p->name; p++)
                {
                    if (nameValue[0] == p->name)
                    {
                        *(double*)((char*)&e + p->offset) =
                            nameValue[1].toDouble();
                        break;
                    }
                }

                if (nameValue[0] == "GC")
                    e.gccount = nameValue[1].toInt();
                else if (nameValue[0] == "F")
                    e.frame = nameValue[1].toInt();
                else if (nameValue[0] == "EST")
                    e.expectedSyncTime = nameValue[1].toFloat();
                else if (nameValue[0] == "DCO")
                    e.deviceClockOffset = nameValue[1].toFloat();
                else if (nameValue[0] == "PRUP")
                    e.prefetchUploadPlane = nameValue[1].toFloat();
                else if (nameValue[0] == "RRUP")
                    e.renderUploadPlane = nameValue[1].toFloat();
                else if (nameValue[0] == "RFW")
                    e.renderFenceWait = nameValue[1].toFloat();
            }

            data.push_back(e);
        }
        else
        {
            //
            //  OLD FORMAT(S)
            //

            if (parts.size() > 12)
            {
                DataElement e;
                size_t index = 0;

                e.render0 = parts[index++].toDouble();
                e.render1 = parts[index++].toDouble();
                e.swap0 = parts[index++].toDouble();
                e.swap1 = parts[index++].toDouble();
                e.eval0 = parts[index++].toDouble();
                e.eval1 = parts[index++].toDouble();
                e.userRender0 = parts[index++].toDouble();
                e.userRender1 = parts[index++].toDouble();
                e.frameChange0 = parts[index++].toDouble();
                e.frameChange1 = parts[index++].toDouble();
                e.internalRender0 = parts[index++].toDouble();
                e.internalRender1 = parts[index++].toDouble();
                e.prefetch0 = parts[index++].toDouble();
                e.prefetch1 = parts[index++].toDouble();

                if (parts.size() > 22)
                {
                    e.cacheTest0 = parts[index++].toDouble();
                    e.cacheTest1 = parts[index++].toDouble();
                    e.evalGraph0 = parts[index++].toDouble();
                    e.evalGraph1 = parts[index++].toDouble();
                }
                else
                {
                    e.cacheTest0 = 0;
                    e.cacheTest1 = 0;
                    e.evalGraph0 = 0;
                    e.evalGraph1 = 0;
                }

                if (parts.size() > 16)
                {
                    e.evalID0 = parts[index++].toDouble();
                    e.evalID1 = parts[index++].toDouble();
                    e.cacheQuery0 = parts[index++].toDouble();
                    e.cacheQuery1 = parts[index++].toDouble();
                    e.cacheEval0 = parts[index++].toDouble();
                    e.cacheEval1 = parts[index++].toDouble();

                    if (parts.size() >= 28)
                    {
                        e.io0 = parts[index++].toDouble();
                        e.io1 = parts[index++].toDouble();
                    }
                }
                else
                {
                    e.evalID0 = 0;
                    e.evalID1 = 0;
                    e.cacheQuery0 = 0;
                    e.cacheQuery1 = 0;
                    e.cacheEval0 = 0;
                    e.cacheEval1 = 0;
                }

                e.gccount = parts[index++].toInt();
                e.frame = parts[index++].toInt();

                data.push_back(e);
            }
        }
    }

    cout << "INFO: found " << data.size() << " elements" << endl;
    m_glWidget->setData(data);
    m_fileViewUI.plainTextEdit->setPlainText(fileContents);
}

void VisMainWindow::showFile()
{
    if (m_fileViewDialog->isVisible())
        m_fileViewDialog->hide();
    else
        m_fileViewDialog->show();
}

void VisMainWindow::quit() { qApp->quit(); }

void VisMainWindow::showIdealFrames(int b)
{
    m_glWidget->setShowIdealFrames(b == Qt::Checked);
}

void VisMainWindow::showEvalTiming(int b)
{
    m_glWidget->setShowEvalTiming(b == Qt::Checked);
}

void VisMainWindow::fpsChanged()
{
    m_glWidget->setActualFPS(m_dockUI.actualFPSEdit->text().toFloat());
}

void VisMainWindow::updateTimeRangeFromGLView()
{
    m_dockUI.startTimeEdit->setText(
        QString("%1").arg(m_glWidget->rangeStart()));
    m_dockUI.endTimeEdit->setText(QString("%1").arg(m_glWidget->rangeEnd()));
}

void VisMainWindow::updateActualFPSFromGLView()
{
    m_dockUI.actualFPSEdit->setText(QString("%1").arg(m_glWidget->actualFPS()));
}

void VisMainWindow::setShowIdealFrames()
{
    m_dockUI.showIdealFramesButton->setCheckState(Qt::Checked);
}

void VisMainWindow::rangeChanged()
{
    float startTime = m_dockUI.startTimeEdit->text().toFloat();
    float endTime = m_dockUI.endTimeEdit->text().toFloat();

    if (startTime < endTime)
    {
        m_glWidget->setTimeRange(startTime, endTime);
    }
    else
    {
        int start = m_dockUI.startEdit->text().toInt();
        int end = m_dockUI.endEdit->text().toInt();

        if (end < start)
        {
            end = start;
            m_dockUI.endEdit->setText(QString("%1").arg(end));
        }

        m_glWidget->setRange(start, end);
    }

    const DataVector& data = m_glWidget->data();

    size_t i0 = m_glWidget->startIndex();
    size_t i1 = m_glWidget->endIndex();

    float minRR = 1000000;
    float maxRR = 0;
    double rr = 0;
    size_t count = 0;

    if (i0 < data.size() && i1 < data.size())
    {
        double startTime = data[i0].swap1;
        double endTime = data[i1 - 1].swap1;
        int frames = data[i1].frame - data[i0].frame + 1;

        float fps = float(frames) / (endTime - startTime);
        m_glWidget->setComputedFPS(fps);
        m_dockUI.fpsLabel->setText(QString("%1").arg(fps));

        m_dockUI.totalTimeLabel->setText(
            QString("%1 sec").arg(endTime - startTime));

        for (size_t i = i0; i <= i1; i++)
        {
            if (i > 0)
            {
                float d = data[i].swap1 - data[i - 1].swap1;
                rr += d * d;
                count++;
                minRR = std::min(d, minRR);
                maxRR = std::max(d, maxRR);
            }
        }

        if (rr > 0)
        {
            rr = 1.0 / sqrt(rr / double(count));
            m_dockUI.refreshRateLabel->setText(QString("%1 Hz").arg(rr));
            m_glWidget->setComputedRefresh(rr);
        }
        else
        {
            m_glWidget->setComputedRefresh(0);
        }

        // yes, this is right, its inverted so min is max
        if (minRR > 0)
            m_dockUI.maxVariationLabel->setText(
                QString("%1 Hz").arg(1.0 / minRR));
        if (maxRR > 0)
            m_dockUI.minVariationLabel->setText(
                QString("%1 Hz").arg(1.0 / maxRR));
    }
}

//----------------------------------------------------------------------

GLView::GLView(QWidget* parent, QTextEdit* readout, VisMainWindow* visWin)
    : QGLWidget(QGLFormat(QGL::DoubleBuffer | QGL::SampleBuffers | QGL::Rgba),
                parent, NULL, Qt::Widget)
    , m_scale(1.0)
    , m_xtran(0)
    , m_ytran(0)
    , m_rangeStart(0)
    , m_rangeEnd(0)
    , m_rangeStartIndex(size_t(-1))
    , m_rangeEndIndex(size_t(-1))
    , m_actualFPS(0.0)
    , m_showIdealFrames(false)
    , m_showEvalTiming(true)
    , m_readoutSample(-1)
    , m_readout(readout)
    , m_computedRefresh(0.0)
    , m_computedFPS(0.0)
    , m_visWin(visWin)
{
    m_matrix.makeIdentity();
    setMouseTracking(true);
}

void GLView::setRange(int start, int end)
{
    bool foundStart = false;
    bool foundEnd = false;

    for (size_t i = 0; i < m_data.size(); i++)
    {
        if (m_data[i].frame == start && !foundStart)
        {
            m_rangeStart = m_data[i].render0;
            m_rangeStartIndex = i;
            foundStart = true;
        }

        if (m_data[i].frame == end + 1 && !foundEnd)
        {
            if (i > 0)
            {
                m_rangeEnd = m_data[i].swap1;
                m_rangeEndIndex = i;
                foundEnd = true;
            }
        }
    }

    updateGL();
}

void GLView::setTimeRange(float start, float end)
{
    bool foundStart = false;
    bool foundEnd = false;

    for (size_t i = 0; i < m_data.size(); i++)
    {
        if (m_data[i].render0 >= start && !foundStart)
        {
            m_rangeStart = m_data[i].render0;
            m_rangeStartIndex = i;
            foundStart = true;
        }

        if (m_data[i].render0 >= end && !foundEnd)
        {
            if (i > 0)
            {
                m_rangeEnd = m_data[i].swap1;
                m_rangeEndIndex = i;
                foundEnd = true;
            }
        }
    }
    //  cerr << "rangeEnd " << m_rangeEnd << " index " << m_rangeEndIndex <<
    //  endl;
    updateGL();
}

void GLView::setData(const DataVector& data)
{
    m_data = data;
    m_startTime = m_data.front().render0;
    m_endTime = m_data.back().swap1;

    for (size_t i = 0; i < m_data.size(); i++)
    {
        DataElement& e = m_data[i];

        if (i == 0)
        {
            e.repeatframe = false;
            e.collectionOccured = false;
        }
        else
        {
            DataElement& e0 = m_data[i - 1];
            e.repeatframe = e.frame == e0.frame;
            e.collectionOccured = e.gccount != e0.gccount;
            e0.singleFrame = !e0.repeatframe && !e.repeatframe;
        }
    }

    /*
    float d =  (m_endTime - m_startTime);
    float y = d / float(width());
    m_matrix = scaleMatrix<float>(Vec(d, d, 1))
                * translationMatrix<float>(Vec(-m_startTime + 0.02, y * 0.25,
    0));
    */
    float d = 1.0;
    float y = 4.0 * (m_endTime - m_startTime) / height();
    m_matrix = scaleMatrix<float>(Vec(d, d, 1))
               * translationMatrix<float>(Vec(-m_startTime, y, 0));

    updateGL();
}

void GLView::rebuildIdealFrames()
{
    if (m_actualFPS < 0)
        return;
}

void GLView::setActualFPS(float fps)
{
    m_actualFPS = fps;
    updateGL();
}

void GLView::setShowEvalTiming(bool b)
{
    m_showEvalTiming = b;
    updateGL();
}

void GLView::setShowIdealFrames(bool b)
{
    m_showIdealFrames = b;
    updateGL();
}

void GLView::setComputedRefresh(float rate)
{
    m_computedRefresh = 1.0 / rate;
    updateGL();
}

void GLView::setComputedFPS(float fps)
{
    m_computedFPS = fps;
    updateGL();
}

int GLView::timeToSample(float t)
{
    for (size_t i = 0; i < m_data.size(); i++)
    {
        const DataElement& e = m_data[i];

        if ((e.prefetch1 != 0.0 && e.prefetch1 >= t)
            || (e.prefetch1 == 0.0 && e.swap1 >= t))
        {
            return i;
        }
    }
    return -1;
}

#define APP0(str) m_readoutHtml.append(QString(str))
#define APP1(str, a1) m_readoutHtml.append(QString(str).arg(a1))
#define APP2(str, a1, a2) m_readoutHtml.append(QString(str).arg(a1).arg(a2))
#define APP3(str, a1, a2, a3) \
    m_readoutHtml.append(QString(str).arg(a1).arg(a2).arg(a3))
#define APPBR() m_readoutHtml.append(QString("\n<br>\n"))
#define APPCOLSTART(r, g, b)                                       \
    m_readoutHtml.append(QString("%1<font color=\"#%2%3%4\">")     \
                             .arg(indent)                          \
                             .arg(int(r * 255), 2, 16, QChar('0')) \
                             .arg(int(g * 255), 2, 16, QChar('0')) \
                             .arg(int(b * 255), 2, 16, QChar('0')))
#define APPCOLEND() m_readoutHtml.append(QString("</font>"))
#define APPCOLTXT(r, g, b, s)                                             \
    m_readoutHtml.append(                                                 \
        QString("%1<font color=\"#%2%3%4\">&diams;&diams;&diams;&diams; " \
                "</font><b>%5</b>")                                       \
            .arg(indent)                                                  \
            .arg(int(r * 255), 2, 16, QChar('0'))                         \
            .arg(int(g * 255), 2, 16, QChar('0'))                         \
            .arg(int(b * 255), 2, 16, QChar('0'))                         \
            .arg(s))

#define APPITEM(str, a1)                                                      \
    APP3(QString(str) + QString(", start %1, end %2, total %3<br>\n"), a1##0, \
         a1##1, a1##1 - a1##0)

#define APPDELTAITEM(str, a1) \
    APP1(QString(str) + QString(", total %1<br>\n"), a1)

#define APPITEMTEST(bold, str, a1, r, g, b) \
    if (a1##1 - a1##0 > 0.0000)             \
    {                                       \
        APPCOLTXT(r, g, b, bold ": ");      \
        APPITEM(str, a1);                   \
    }

#define APPDELTAITEMTEST(bold, str, a1, r, g, b) \
    if (a1 > 0.0000)                             \
    {                                            \
        APPCOLTXT(r, g, b, bold ": ");           \
        APP1(str ", total %1<br>\n", a1);        \
    }

#define APPITEMTESTBOLD(bold, str, a1)                                       \
    if (a1##1 - a1##0 > 0.0000)                                              \
    {                                                                        \
        m_readoutHtml.append(QString("%1<b>%2</b> ").arg(indent).arg(bold)); \
        APPITEM(str, a1);                                                    \
    }

void GLView::generateHtml()
{
    m_readoutHtml = "";
    QString indent;
    QString spaces = QString("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;");

    if (m_readoutSample < 0 || m_readoutSample >= m_data.size())
        return;

    const DataElement& e = m_data[m_readoutSample];
    const DataElement& elast =
        m_data[(m_readoutSample == 0) ? 0 : m_readoutSample - 1];

    APP0("<body bgcolor=\"#909090\">");
    APP1("Start with frame %1, ", elast.frame);
    APP1("swap to frame %1.", e.frame);
    APPBR();
    APPBR();

    APPCOLTXT((e.repeatframe ? 0.6 : 1), .3, .3, "render start: ");
    APPITEM("from begin of GLView::paint, until we wait for swap", e.render);

    indent = spaces;
    APPCOLTXT(.3, 1, .3, "evaluate (render) start: ");
    APPITEM("Evaluation during Session::render (Session::evaluateForDisplay)",
            e.eval);

    if (e.cacheTest0 < e.eval1)
    {
        QString saveIndent = indent;
        indent = saveIndent + spaces;
        APPITEMTEST(
            "cacheTest",
            "test to see if image is in cache, IPGraph::evaluateAtFrame()",
            e.cacheTest, 1, 1, 0);

        indent = saveIndent + spaces + spaces;
        APPITEMTESTBOLD("cacheTestLock", "acquire lock on frame cache",
                        e.cacheTestLock);
        APPITEMTESTBOLD("setDisplayFrame", "set display frame on frame cache",
                        e.setDisplayFrame);
        APPITEMTESTBOLD("frameCachedTest",
                        "check that all FBs for this frame are in cache",
                        e.frameCachedTest);

        indent = saveIndent + spaces;
        APPITEMTESTBOLD("evalGraph", "get the FBs for this frame from cache.",
                        e.evalGraph);
    }

    indent = spaces;
    APPCOLTXT(.3, 1, .3, "evaluate (render) end");
    APPBR();

    APPCOLTXT(1, .6, .6, "internalRender: ");
    APPITEM("Internal render time, total time in CompositeRenderer::render",
            e.internalRender);

    indent = spaces + spaces;
    APPITEMTEST("userRender",
                "userRender code (handling \"render\" event).  only time delta "
                "is valid.",
                e.userRender, .6, .6, 1);
    APPITEMTEST("frameChange",
                "internal frameChangedMessage processing plug "
                "\"frame-changed\" user event.",
                e.frameChange, 1, 1, .6);

    APPDELTAITEMTEST(
        "renderFenceWait",
        "total fence wait time.  should only be non-zero in presentation mode.",
        e.renderFenceWait, .6, .2, .2);
    APPDELTAITEMTEST("renderUploadPlane",
                     "total time in uploadPlane().  should only be non-zero if "
                     "upload did not complete in prefetch.",
                     e.renderUploadPlane, .6, .2, .6);

    indent = spaces;
    APPCOLTXT(1, .6, .6, "internalRender end");
    APPBR();

    indent = "";
    APPCOLTXT((e.repeatframe ? 0.6 : 1), .3, .3, "render end");
    APPBR();

    float swapDiff = e.swap1 - e.swap0;
    float swapRate = (swapDiff == 0.0) ? 0.0 : 1.0 / swapDiff;

    QString swapText;
    if (swapRate != 0.0 && m_computedRefresh != 0.0
        && swapRate < 1.0 / m_computedRefresh)
    {
        swapText = QString("rendering complete, waiting for buffer swap, "
                           "<b>equiv rate = %1</b>")
                       .arg(swapRate);
    }
    else
    {
        swapText =
            QString(
                "rendering complete, waiting for buffer swap, equiv rate = %1")
                .arg(swapRate);
    }
    APPITEMTEST("buffer swap / vsync", swapText, e.swap, .3, .3, .75);

    APPITEMTEST("awaken", "awaken caching threads post-render", e.awaken, .2,
                .6, .6);

    APPCOLTXT(.7, 1, 1, "prefetch: ");
    APPITEM("time spent in Session::postRender, including eval time if "
            "prefetching.",
            e.prefetch);

    if (e.cacheTest1 > e.prefetch0)
    {
        QString saveIndent = indent;
        indent = saveIndent + spaces;
        APPITEMTEST(
            "cacheTest",
            "test to see if image is in cache, IPGraph::evaluateAtFrame()",
            e.cacheTest, 1, 1, 0);

        indent = saveIndent + spaces + spaces;

        APPITEMTESTBOLD("cacheTestLock", "acquire lock on frame cache",
                        e.cacheTestLock);
        APPITEMTESTBOLD("setDisplayFrame", "set display frame on frame cache",
                        e.setDisplayFrame);
        APPITEMTESTBOLD("frameCachedTest",
                        "check that all FBs for this frame are in cache",
                        e.frameCachedTest);

        indent = saveIndent + spaces;
        APPITEMTESTBOLD("evalGraph", "get the FBs for this frame from cache.",
                        e.evalGraph);
    }

    indent = spaces;
    APPCOLTXT(.6, .2, .2, "prefetchRender: ");
    APPITEM("total prefetch rendertime, CompositeRenderer::prefetch.",
            e.prefetchRender);

    indent = spaces + spaces;
    APPDELTAITEMTEST(
        "prefetchUploadPlanes",
        "time spent in uploadPlanes, may be zero of upload is async",
        e.prefetchUploadPlane, .6, .2, .6);

    indent = spaces;
    APPCOLTXT(.6, .2, .2, "prefetchRender end ");
    APPBR();

    indent = "";
    APPCOLTXT(.7, 1, 1, "prefetch end");
    APPBR();

    APPBR();
    APP0("<b>Cache Node Evaluation Items</b>  (CacheIPNode::evaluate) These "
         "may be garbled by timing issues if caching is happening during "
         "playback.<br>");

    indent = spaces;
    APPITEMTEST(
        "evalID",
        "evaluate identifiers preparatory to looking them up in the cache",
        e.evalID, .75, 1, .75);
    APPITEMTEST(
        "cacheQuery",
        "evaluate identifiers preparatory to looking them up in the cache.",
        e.cacheQuery, 0, .75, 1);
    APPITEMTEST("cacheEval", "actually evaluate the FBs for this frame.",
                e.cacheEval, 1, .6, 1);
    APPITEMTESTBOLD("IO", "FileSource calling imagesAtFrame on it's movies.",
                    e.io);

    indent = "";

    APPBR();
    APP1("%1 Garbage collections so far.", e.gccount);
    APPBR();
    APP3("Expected time of next vsync: %1, which is in the %2 (%3 sec from end "
         "of render).",
         e.expectedSyncTime,
         ((e.expectedSyncTime < e.render1) ? "past" : "future"),
         (e.expectedSyncTime - e.render1));
    if (e.deviceClockOffset != 0.0)
    {
        APPBR();
        APP1("Elapsed play time - Device clock = %1 seconds.",
             e.deviceClockOffset);
    }

    APP0("</body>");
    //  cerr << m_readoutHtml.toStdString() << endl;
}

int GLView::sampleFromMousePosition(float x, float y)
{
    float tx = m_startTime + (x / width()) * (m_endTime - m_startTime);
    float ty =
        m_startTime
        + (-y / height()) * (m_endTime - m_startTime) * (height() / width());

    Matrix m = m_matrix;
    m.invert();
    Vec t = m * Vec(tx, ty, 0);

    return timeToSample(t.x);
}

void GLView::mouseMoveEvent(QMouseEvent* event)
{
    QPoint dp = event->pos() - m_mouseDown;
    float w = width();
    float h = height();

    if (event->buttons() == Qt::NoButton)
    {
        int sample =
            sampleFromMousePosition(event->pos().x(), event->pos().y());

        if (sample != m_readoutSample)
        {
            m_readoutSample = sample;
            generateHtml();
            m_readout->setHtml(m_readoutHtml);
        }
        return;
    }

    if (event->buttons() & Qt::RightButton)
    {
        Vec t(.5 * (m_endTime - m_startTime),
              .5 * (m_endTime - m_startTime) * (h / w), 0);

        float a = dp.x() * .8;
        a *= 0.001;
        a += 1.0;
        m_matrix = translationMatrix<float>(t)
                   * scaleMatrix<float>(Vec(a, a, 1))
                   * translationMatrix<float>(-t) * m_matrix;
    }
    else if (event->buttons() & Qt::LeftButton)
    {
        Vec t = Vec(dp.x() / w * (m_endTime - m_startTime),
                    -dp.y() / h * (m_endTime - m_startTime) * (h / w), 0);
        m_matrix = translationMatrix<float>(t) * m_matrix;
    }
    m_mouseDown = event->pos();
    updateGL();
}

void GLView::autoRangeProcessing()
{
    if (m_rangeEnd > m_rangeStart)
    {
        m_visWin->updateTimeRangeFromGLView();
        m_visWin->rangeChanged();
        m_actualFPS = float(int(m_computedFPS + 0.5));
        m_visWin->updateActualFPSFromGLView();
        m_showIdealFrames = true;
        m_visWin->setShowIdealFrames();
    }
}

void GLView::mousePressEvent(QMouseEvent* event)
{
    m_mouseDown = event->pos();

    if (event->buttons() & Qt::LeftButton)
    {
        if (event->modifiers() & Qt::ShiftModifier)
        {
            int sample =
                sampleFromMousePosition(event->pos().x(), event->pos().y());

            m_rangeStartIndex = sample;
            while (m_rangeStartIndex > 1)
            {
                const DataElement& one = m_data[m_rangeStartIndex - 1];
                const DataElement& two = m_data[m_rangeStartIndex];
                if (one.expectedSyncTime == 0.0 || two.expectedSyncTime == 0.0)
                    break;
                if (one.frame > two.frame)
                    break;
                --m_rangeStartIndex;
            }
            ++m_rangeStartIndex;
            int frame = m_data[m_rangeStartIndex].frame;
            while (frame == m_data[m_rangeStartIndex].frame)
                ++m_rangeStartIndex;

            m_rangeStart = m_data[m_rangeStartIndex].render0;

            m_rangeEndIndex = sample;
            while (m_rangeEndIndex < m_data.size() - 1)
            {
                const DataElement& one = m_data[m_rangeEndIndex - 1];
                const DataElement& two = m_data[m_rangeEndIndex];
                if (one.expectedSyncTime == 0.0 || two.expectedSyncTime == 0.0)
                    break;
                if (one.frame > two.frame)
                    break;
                ++m_rangeEndIndex;
            }
            --m_rangeEndIndex;
            frame = m_data[m_rangeEndIndex].frame;
            while (frame == m_data[m_rangeEndIndex].frame)
                --m_rangeEndIndex;

            m_rangeEnd = m_data[m_rangeEndIndex].swap1;

            autoRangeProcessing();
        }
        else if (event->modifiers() & Qt::ControlModifier)
        {
            int sample =
                sampleFromMousePosition(event->pos().x(), event->pos().y());
            m_rangeStart = m_data[sample].render0;
            m_rangeStartIndex = sample;
            autoRangeProcessing();
        }
        else if (event->modifiers() & Qt::AltModifier)
        {
            int sample =
                sampleFromMousePosition(event->pos().x(), event->pos().y());
            m_rangeEnd = m_data[sample].swap1;
            m_rangeEndIndex = sample;
            autoRangeProcessing();
        }
    }
    updateGL();
}

void GLView::mouseReleaseEvent(QMouseEvent* event) {}

void GLView::wheelEvent(QWheelEvent* event)
{
    float w = width();
    float h = height();

    Vec t(.5 * (m_endTime - m_startTime),
          .5 * (m_endTime - m_startTime) * (h / w), 0);

    float a = event->delta() * .8;
    a *= 0.001;
    a += 1.0;
    m_matrix = translationMatrix<float>(t) * scaleMatrix<float>(Vec(a, a, 1))
               * translationMatrix<float>(-t) * m_matrix;

    updateGL();
}

void GLView::initializeGL()
{
    Context c = GLtext::newContext();
    GLtext::setContext(c);
    GLtext::init();
}

void GLView::resizeGL(int, int)
{
    glViewport(0, 0, width(), height());
    updateGL();
}

static void drawBox(float r, float g, float b, float a, float llx, float lly,
                    float urx, float ury)
{
    glColor4f(r, g, b, a);
    glBegin(GL_POLYGON);
    glVertex2f(llx, lly);
    glVertex2f(llx, ury);
    glVertex2f(urx, ury);
    glVertex2f(urx, lly);
    glEnd();
}

static void drawBox(float r, float g, float b, float a, float x0, float y0,
                    float x1, float y1, float x2, float y2, float x3, float y3)
{
    glColor4f(r, g, b, a);
    glBegin(GL_POLYGON);
    glVertex2f(x0, y0);
    glVertex2f(x1, y1);
    glVertex2f(x2, y2);
    glVertex2f(x3, y3);
    glEnd();
}

void GLView::paintGL()
{
    glViewport(0, 0, width(), height());
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    if (m_data.empty())
    {
        glFinish();
        return;
    }
    int idealStartIndex = -1;
    int lastIdealFrame = -1;

#ifndef PLATFORM_WINDOWS
    glEnable(GL_MULTISAMPLE);
#endif
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    double l = (m_endTime - m_startTime) / double(width());
    gluOrtho2D(0, m_endTime - m_startTime, 0, l * double(height()));
    glEnable(GL_POLYGON_SMOOTH);

    glMatrixMode(GL_MODELVIEW);
    glLoadMatrix(m_matrix);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (m_rangeStart != 0 && m_rangeEnd != 0 && m_rangeEnd != m_rangeStart)
    {
        drawBox(1, 1, .8, .1, m_rangeStart, -0.05, m_rangeEnd, 0.08);
    }

    float h0 = 0;
    float h = .05;
    double x0, x1, y0, y1;

    for (size_t i = 0; i < m_data.size(); i++)
    {
        const DataElement& e = m_data[i];

        //
        //  Red box: all GLview paint time; many other times are
        //  fractions of this time, so drawn over this box.  brighter for
        //  the first time we've rendered this frame.  This includs all of
        //  Sesssion::render().
        //

        y0 = h0;
        y1 = h / 3.0;

        drawBox(e.repeatframe ? 0.6 : 1, .3, .3, 1, e.render0, y0, e.render1,
                y1);

        //
        //  Green box, drawn above render box.  corresponds to Session's
        //  evaluateForDisplay().
        //

        y0 = y1;
        y1 = 2 * h / 3.0;

        drawBox(.3, 1, .3, 1, e.eval0, y0, e.eval1, y1);

        if (m_showEvalTiming)
        {
            //
            //  Eval time components may come from evalFrameForDisplay (part of
            //  render), or later (in same frame) as part of prefetch
            //  (postRender()).
            //
            //  So these may be part of the R0-R1 (render) interval, or of the
            //  PR0-P01 (prefetch) interval; draw them so they can work either
            //  way.  Horizontally in upper third.
            //

            y0 = y1;
            y1 = h;

            //
            //  Yellow: Testing to see if image is already in cache, first part
            //  of IPGraph::evaluateAtFrame()
            //

            drawBox(1, 1, 0, 1, e.cacheTest0, y0, e.cacheTest1, y1);

            //
            //  Below three happen in CacheNode::evaluate, so if caching is
            //  happening during playback, timing here is likely to be messed
            //  up (IE numbers here may have nothing to do with this frame).
            //

            //
            //  Light green: In CacheNode::evaluate, evaluate the IDs
            //  prepratory to testing for presence in cache.
            //

            drawBox(.75, 1, .75, 1, e.evalID0, y0, e.evalID1, y1);

            //
            //  Sky blue: Now we have the IDs, query the cache to see if they
            //  are there.
            //

            drawBox(0, .75, 1, 1, e.cacheQuery0, y0, e.cacheQuery1, y1);

            //
            //  Light magenta: If we didn't find our frame buffers in the
            //  cache, evaluate cache node input to get them.  This color only
            //  appears if the display thread does the eval (caching is off).
            //

            drawBox(1, .6, 1, 1, e.cacheEval0, y0, e.cacheEval1, y1);
        }

        //
        //  Light red.  Internal render time (total time in
        //  CompositeRenderer::render() Disjoint from eval time in total paint
        //  time, so draw in 2nd vertical third.
        //

        y0 = h / 3.0;
        y1 = 2.0 * h / 3.0;

        drawBox(1, .6, .6, 1, e.internalRender0, y0, e.internalRender1, y1);

        //
        //   Next four are subsets of Internal Render, so draw above in top
        //   vertical third.
        //

        y0 = y1;
        y1 = h;

        //
        //  Light blue.  Time spent in user render (this is now subset of
        //  internal rendertime), but exact timing can be mixed with other
        //  stuff, so only use time delta.
        //

        x0 = e.internalRender0;
        x1 = x0 + e.userRender1 - e.userRender0;

        drawBox(.6, .6, 1, 1, x0, y0, x1, y1);

        //
        //  Light yellow.  Time spent in "frame change", both internal
        //  frameChangedMessage() processing, and user event "frame-changed".
        //

        x0 = x1;
        x1 = x0 + e.frameChange1 - e.frameChange0;

        drawBox(1, 1, .6, 1, x0, y0, x1, y1);

        //
        //  dark green.  Total time spent waiting on glfences in render, should
        //  only be non-zero in presentation mode.
        //

        x0 = x1;
        x1 = x0 + e.renderFenceWait;

        drawBox(.6, .2, .2, 1, x0, y0, x1, y1);

        //
        //  dark magenta.  Total time spent in uploadPlane() in render, should
        //  only be non-zero if texture uploads did not complete during
        //  prefetch.
        //

        x0 = x1;
        x1 = x0 + e.renderUploadPlane;

        drawBox(.6, .2, .6, 1, x0, y0, x1, y1);

        //
        //  Dark blue.  Swap wait.  Full height
        //

        y0 = 0;
        y1 = h;
        drawBox(.3, .3, .75, 1, e.swap0, y0, e.swap1, y1);

        //
        //  Dark Cyan.  Awakening caching threads (part of
        //  Session::postRender).  Full height
        //

        y0 = 0;
        y1 = h;
        drawBox(.2, .6, .6, 1, e.awaken0, y0, e.awaken1, y1);

        //
        //  Light cyan.  Prefetch (time spend in Session::postRender().  If
        //  prefetch is on, this includes eval time (for next frame).
        //

        y0 = h / 3.0;
        y1 = 2.0 * h / 3.0;

        if (e.prefetch0 != 0.0 && e.prefetch1 != 0.0)
        {
            drawBox(.7, 1, 1, 1, e.prefetch0, y0, e.prefetch1, y1);
        }

        //
        //  Eval timing drawn above may appear in upper third above general
        //  prefetch time.  After eval, we do some parts of the next "render",
        //  basically starting texture uploads in prep for shader execution.
        //  This is asynchronous, so should not take much time.  Draw details
        //  in upper third.
        //

        y0 = 2.0 * h / 3.0;
        y1 = h;

        //
        //  dark magenta.  Total time spent in uploadPlane() in prefetch, should
        //  really never be non-zero (because it should be asynchronous).
        //

        if (e.prefetchRender0 != 0.0 && e.prefetchRender1 != 0.0)
        {
            x0 = e.prefetchRender0;
            x1 = x0 + e.prefetchUploadPlane;

            drawBox(.6, .2, .6, 1, x0, y0, x1, y1);
        }

        //
        //  Dark red.  remainder of prefetch render time (not spent in
        //  uploadPlane).
        //

        if (e.prefetchRender0 != 0.0 && e.prefetchRender1 != 0.0)
        {
            x0 = x1;
            x1 = e.prefetchRender1;

            drawBox(0.6, .2, .2, 1, x0, y0, x1, y1);
        }
    }

    //
    //  Draw text last
    //

    for (size_t i = 0; i < m_data.size(); i++)
    {
        const DataElement& e = m_data[i];

        //
        //  Swap tick
        //
        glLineWidth(2.0);
        glColor4f(0, 0, 0, .5);
        glBegin(GL_LINES);
        glVertex2f(e.swap1, h);
        glVertex2f(e.swap1, h + h * .1);
        glEnd();

        //
        //  Target swap time for this frame.
        //
        if (e.expectedSyncTime != 0.0)
        {
            glColor4f(.7, .2, .2, 1);
            glBegin(GL_LINES);
            glVertex2f(e.render0, 0);
            glVertex2f(e.expectedSyncTime, h);
            glVertex2f(e.expectedSyncTime, h + h * .05);
            glEnd();

            GLtext::size(20.0);
            GLtext::color(.7, .2, .2, 1);
            float s = 1.0
                      / (GLtext::globalAscenderHeight()
                         - GLtext::globalDescenderHeight());
            ostringstream str;
            str << e.frame;
            glPushMatrix();
            glTranslatef(e.expectedSyncTime, h + h * .05, 0);
            glScalef(s * .0035, s * .0035, 1.0);
            GLtext::writeAt(0, 0, str.str());
            glPopMatrix();
        }

        //
        //  Frame flags
        //
        if (!e.repeatframe)
        {
            GLtext::size(20.0);
            GLtext::color(0, 0, 0, .7);
            float s = 1.0
                      / (GLtext::globalAscenderHeight()
                         - GLtext::globalDescenderHeight());
            ostringstream str;
            str << e.frame;

            Box2f b = GLtext::bounds(str.str());
            float tw = (b.max.x - b.min.x) * s * 0.0065;
            float th = (b.max.y - b.min.y) * s * 0.0065;
            float slop = .0008;

            glColor4f(1, 1, 1, .5);
            glBegin(GL_POLYGON);
            glVertex2f(e.swap1 - slop, h + h * .11 - slop);
            glVertex2f(e.swap1 + tw + slop, h + h * .11 - slop);
            glVertex2f(e.swap1 + tw + slop * 6, h + h * .11 + th / 2);
            glVertex2f(e.swap1 + tw + slop, h + h * .11 + th + slop);
            glVertex2f(e.swap1 - slop, h + h * .11 + th + slop);
            glEnd();

            glPushMatrix();
            glTranslatef(e.swap1, h + h * .11, 0);
            glScalef(s * .0065, s * .0065, 1.0);
            GLtext::writeAt(0, 0, str.str());
            glPopMatrix();
        }

        //
        //  Mark frames only drawn once.
        //
        if (e.singleFrame)
        {
            glColor4f(0, 0, 0, .7);
            glLineWidth(2.0);
            glBegin(GL_LINES);
            glVertex2f(e.swap1, h + h * .25);
            glVertex2f(e.swap1, h + h * .4);
            glEnd();

            GLtext::size(50.0);
            GLtext::color(1, 1, 1, 1);
            float s = 1.0
                      / (GLtext::globalAscenderHeight()
                         - GLtext::globalDescenderHeight());
            ostringstream str;
            str << "*1 ";

            Box2f b = GLtext::bounds(str.str());
            float tw = (b.max.x - b.min.x) * s * 0.0065;
            float th = (b.max.y - b.min.y) * s * 0.0065;
            float slop = .0008;

            glColor4f(1, .5, 0, .8);
            glBegin(GL_POLYGON);
            glVertex2f(e.swap1 - slop, h + h * .4 - slop);
            glVertex2f(e.swap1 + tw + slop, h + h * .4 - slop);
            glVertex2f(e.swap1 + tw + slop, h + h * .4 + th + slop);
            glVertex2f(e.swap1 - slop, h + h * .4 + th + slop);
            glEnd();

            glPushMatrix();
            glTranslatef(e.swap1, h + h * .4, 0);
            glScalef(s * .0065, s * .0065, 1.0);
            GLtext::writeAt(0, 0, str.str());
            glPopMatrix();
        }

        if (e.collectionOccured)
        {
            glColor4f(0, 0, 0, .7);
            glLineWidth(2.0);
            glBegin(GL_LINES);
            glVertex2f(e.swap1, h + h * .25);
            glVertex2f(e.swap1, h + h * .7);
            glEnd();

            GLtext::size(20.0);
            GLtext::color(0, 0, 0, .85);
            float s = 1.0
                      / (GLtext::globalAscenderHeight()
                         - GLtext::globalDescenderHeight());
            ostringstream str;
            str << "GC# " << e.gccount << " ";

            Box2f b = GLtext::bounds(str.str());
            float tw = (b.max.x - b.min.x) * s * 0.0100;
            float th = (b.max.y - b.min.y) * s * 0.0100;
            float slop = .0008;

            glColor4f(1, 0, 1, .9);
            glBegin(GL_POLYGON);
            glVertex2f(e.swap1 - slop, h + h * .7 - slop);
            glVertex2f(e.swap1 + tw + slop, h + h * .7 - slop);
            glVertex2f(e.swap1 + tw + slop, h + h * .7 + th + slop);
            glVertex2f(e.swap1 - slop, h + h * .7 + th + slop);
            glEnd();

            glPushMatrix();
            glTranslatef(e.swap1, h + h * .7, 0);
            glScalef(s * .0100, s * .0100, 1.0);
            GLtext::writeAt(0, 0, str.str());
            glPopMatrix();
        }

        if (false && m_showIdealFrames && i >= m_rangeStartIndex
            && i <= m_rangeEndIndex)
        {
            if (idealStartIndex == -1)
            {
                if (!e.repeatframe)
                {
                    idealStartIndex = i;
                    lastIdealFrame = m_data[idealStartIndex].frame;
                }
            }
            else
            {
                int startFrame = m_data[idealStartIndex].frame;
                int frame = startFrame
                            + int(0.5
                                  + ((e.swap1 - m_data[idealStartIndex].swap1)
                                     * m_actualFPS));

                /*
                int nsyncs = i - m_rangeStartIndex;
                double t = nsyncs * m_computedRefresh;
                double t0 = (nsyncs - 1) * m_computedRefresh;
                int frame = m_actualFPS * t + m_data[m_rangeStartIndex].frame;
                int previous = m_actualFPS * t0 +
                m_data[m_rangeStartIndex].frame;

                if (frame != previous)
                {
                */
                if (frame != lastIdealFrame)
                {
                    lastIdealFrame = frame;
                    GLtext::size(20.0);
                    GLtext::color(0, 0, 0, .7);
                    float s = 1.0
                              / (GLtext::globalAscenderHeight()
                                 - GLtext::globalDescenderHeight());
                    ostringstream str;
                    str << frame;

                    Box2f b = GLtext::bounds(str.str());
                    float tw = (b.max.x - b.min.x) * s * 0.0065;
                    float th = (b.max.y - b.min.y) * s * 0.0065;
                    float slop = .0008;

                    glColor4f(.5, 1, 1, .5);
                    glBegin(GL_POLYGON);
                    glVertex2f(e.swap1 - slop, h + h * .21 - slop);
                    glVertex2f(e.swap1 + tw + slop, h + h * .21 - slop);
                    glVertex2f(e.swap1 + tw + slop * 6, h + h * .21 + th / 2);
                    glVertex2f(e.swap1 + tw + slop, h + h * .21 + th + slop);
                    glVertex2f(e.swap1 - slop, h + h * .21 + th + slop);
                    glEnd();

                    glPushMatrix();
                    glTranslatef(e.swap1, h + h * .21, 0);
                    glScalef(s * .0065, s * .0065, 1.0);
                    GLtext::writeAt(0, 0, str.str());
                    glPopMatrix();
                }
            }
        }
    }

    if (m_showIdealFrames)
    {
        idealStartIndex = m_rangeStartIndex;

        const DataElement& e = m_data[0];
        int i;
        for (i = m_rangeStartIndex; m_data[i].repeatframe; ++i)
            ;

        int startFrame = m_data[i].frame;
        double time = m_data[i].swap1;
        double endTime = m_data[m_rangeEndIndex].swap1;

        for (int frame = startFrame + 1; time < endTime; ++frame)
        {
            time = time + 1.0 / m_actualFPS;

            GLtext::size(20.0);
            GLtext::color(0, 0, 0, .7);
            float s = 1.0
                      / (GLtext::globalAscenderHeight()
                         - GLtext::globalDescenderHeight());
            ostringstream str;
            str << frame;

            Box2f b = GLtext::bounds(str.str());
            float tw = (b.max.x - b.min.x) * s * 0.0065;
            float th = (b.max.y - b.min.y) * s * 0.0065;
            float slop = .0008;

            glColor4f(.5, 1, 1, .5);
            glBegin(GL_POLYGON);
            glVertex2f(time - slop, h + h * .21 - slop);
            glVertex2f(time + tw + slop, h + h * .21 - slop);
            glVertex2f(time + tw + slop * 6, h + h * .21 + th / 2);
            glVertex2f(time + tw + slop, h + h * .21 + th + slop);
            glVertex2f(time - slop, h + h * .21 + th + slop);
            glEnd();

            glPushMatrix();
            glTranslatef(time, h + h * .21, 0);
            glScalef(s * .0065, s * .0065, 1.0);
            GLtext::writeAt(0, 0, str.str());
            glPopMatrix();
        }
    }

    glColor4f(0, 0, 0, .5);

    glLineWidth(2.0);
    glBegin(GL_LINES);
    glVertex2f(m_startTime, 0);
    glVertex2f(m_endTime, 0);
    glVertex2f(m_startTime, 0);
    glVertex2f(m_startTime, h * 2.0);
    glVertex2f(m_endTime, 0);
    glVertex2f(m_endTime, h * 2.0);
    glEnd();

    GLtext::size(100.0);
    GLtext::color(0, 0, 0, .5);
    float s =
        1.0
        / (GLtext::globalAscenderHeight() - GLtext::globalDescenderHeight());

    for (int i = int(m_startTime); i < int(m_endTime - m_startTime + 1.0); i++)
    {
        glBegin(GL_LINES);
        glVertex2f(float(i), 0);
        glVertex2f(float(i), -0.01);
        glVertex2f(float(i) + 0.5, 0);
        glVertex2f(float(i) + 0.5, -0.008);
        glEnd();

        {
            ostringstream str;
            str << i << " sec";
            glPushMatrix();
            glTranslatef(float(i), float(-0.03), 0);
            glScalef(s * .02, s * .02, 1.0);
            GLtext::writeAt(0, 0, str.str());
            glPopMatrix();
        }

        {
            ostringstream str;
            str << i << ".5 sec";
            glPushMatrix();
            glTranslatef(float(i) + 0.5, float(-0.03), 0);
            glScalef(s * .02, s * .02, 1.0);
            GLtext::writeAt(0, 0, str.str());
            glPopMatrix();
        }

        // renderText(float(i), -0.01, 0, QString("%1").arg(i));
    }

    glFinish();
}
