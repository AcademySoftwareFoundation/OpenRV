//
//  Copyright (c) 2008 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __playbackVis__VisMainWindow__h__
#define __playbackVis__VisMainWindow__h__
#include <iostream>
#include <QtGui/QtGui>
#include <QtOpenGL/QtOpenGL>
#include <QtWidgets/QtWidgets>

#include <TwkMath/Mat44.h>
#include <TwkMath/Vec3.h>
#include <TwkMath/Vec4.h>
#include "ui_VisMainWindow.h"
#include "ui_DockWidget.h"
#include "ui_FileViewDialog.h"

struct DataElement
{
    double render0;
    double render1;
    double swap0;
    double swap1;
    double eval0;
    double eval1;
    double userRender0;
    double userRender1;
    double frameChange0;
    double frameChange1;
    double internalRender0;
    double internalRender1;
    double prefetch0;
    double prefetch1;

    double expectedSyncTime;
    double deviceClockOffset;
    int gccount;
    int frame;

    double cacheTest0;
    double cacheTest1;
    double evalGraph0;
    double evalGraph1;
    double evalID0;
    double evalID1;
    double cacheQuery0;
    double cacheQuery1;
    double cacheEval0;
    double cacheEval1;
    double io0;
    double io1;

    double restartA0;
    double restartA1;
    double restartB0;
    double restartB1;
    double cacheTestLock0;
    double cacheTestLock1;
    double setDisplayFrame0;
    double setDisplayFrame1;
    double frameCachedTest0;
    double frameCachedTest1;
    double awaken0;
    double awaken1;

    double prefetchRender0;
    double prefetchRender1;
    double prefetchUploadPlane;
    double renderUploadPlane;
    double renderFenceWait;

    bool repeatframe;
    bool collectionOccured;
    bool singleFrame;

    double start;
    double end;
};

struct NameOffsetPair
{
    const char* name;
    long offset;
};

typedef std::vector<DataElement> DataVector;

class VisMainWindow;

class GLView : public QGLWidget
{
    Q_OBJECT

public:
    typedef TwkMath::Mat44f Matrix;
    typedef TwkMath::Vec3f Vec;

    GLView(QWidget* parent, QTextEdit* readout, VisMainWindow* visWin);

    void setData(const DataVector&);

    DataVector& data() { return m_data; }

    void setRange(int start, int end);
    void setTimeRange(float start, float end);

    size_t startIndex() const { return m_rangeStartIndex; }

    size_t endIndex() const { return m_rangeEndIndex; }

    double rangeStart() const { return m_rangeStart; }

    double rangeEnd() const { return m_rangeEnd; }

    float actualFPS() const { return m_actualFPS; }

    void setActualFPS(float fps);
    void setShowIdealFrames(bool b);
    void setShowEvalTiming(bool b);
    void setComputedRefresh(float);
    void setComputedFPS(float);

protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int, int);
    void rebuildIdealFrames();
    int timeToSample(float t);
    int sampleFromMousePosition(float x, float y);
    void generateHtml();
    void autoRangeProcessing();

    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);
    virtual void wheelEvent(QWheelEvent* event);

private:
    DataVector m_data;
    double m_startTime;
    double m_endTime;
    float m_scale;
    float m_xtran;
    float m_ytran;

    float m_actualFPS;
    bool m_showIdealFrames;
    bool m_showEvalTiming;
    float m_computedRefresh;
    float m_computedFPS;

    float m_rangeStart;
    float m_rangeEnd;
    size_t m_rangeStartIndex;
    size_t m_rangeEndIndex;

    Matrix m_matrix;

    QPoint m_mouseDown;

    QString m_readoutHtml;
    QTextEdit* m_readout;
    int m_readoutSample;
    VisMainWindow* m_visWin;
};

class VisMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    VisMainWindow(QWidget* parent = 0);

    void readFile(const QString&);
    bool eventFilter(QObject* obj, QEvent* event);
    void updateTimeRangeFromGLView();
    void updateActualFPSFromGLView();
    void setShowIdealFrames();

public slots:
    void rangeChanged();
    void showIdealFrames(int);

private slots:
    void openFile();
    void showFile();
    void quit();
    void fpsChanged();
    void showEvalTiming(int);

private:
    GLView* m_glWidget;
    Ui::VisMainWindow m_ui;
    Ui::DockWidget m_dockUI;
    Ui::FileViewDialog m_fileViewUI;
    QDockWidget* m_dockWidget;
    QDialog* m_fileViewDialog;
    double m_mouseTime;
};

#endif // __playbackVis__VisMainWindow__h__
