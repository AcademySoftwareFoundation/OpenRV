//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
QWidgetType* addAllQTSymbols(MuLangContext* c, Module* qt)
{
    QColorType* t_QColor = new QColorType(c, "QColor");
    qt->addSymbol(t_QColor);
    QSizeType* t_QSize = new QSizeType(c, "QSize");
    qt->addSymbol(t_QSize);
    QPointType* t_QPoint = new QPointType(c, "QPoint");
    qt->addSymbol(t_QPoint);
    QPointFType* t_QPointF = new QPointFType(c, "QPointF");
    qt->addSymbol(t_QPointF);
    QRectType* t_QRect = new QRectType(c, "QRect");
    qt->addSymbol(t_QRect);
    QRectFType* t_QRectF = new QRectFType(c, "QRectF");
    qt->addSymbol(t_QRectF);
    QFontType* t_QFont = new QFontType(c, "QFont");
    qt->addSymbol(t_QFont);
    QKeySequenceType* t_QKeySequence = new QKeySequenceType(c, "QKeySequence");
    qt->addSymbol(t_QKeySequence);
    QIconType* t_QIcon = new QIconType(c, "QIcon");
    qt->addSymbol(t_QIcon);
    QRegionType* t_QRegion = new QRegionType(c, "QRegion");
    qt->addSymbol(t_QRegion);
    QVariantType* t_QVariant = new QVariantType(c, "QVariant");
    qt->addSymbol(t_QVariant);
    QUrlType* t_QUrl = new QUrlType(c, "QUrl");
    qt->addSymbol(t_QUrl);
    QUrlQueryType* t_QUrlQuery = new QUrlQueryType(c, "QUrlQuery");
    qt->addSymbol(t_QUrlQuery);
    QDateTimeType* t_QDateTime = new QDateTimeType(c, "QDateTime");
    qt->addSymbol(t_QDateTime);
    QTimeType* t_QTime = new QTimeType(c, "QTime");
    qt->addSymbol(t_QTime);
    QDateType* t_QDate = new QDateType(c, "QDate");
    qt->addSymbol(t_QDate);
    QByteArrayType* t_QByteArray = new QByteArrayType(c, "QByteArray");
    qt->addSymbol(t_QByteArray);
    QModelIndexType* t_QModelIndex = new QModelIndexType(c, "QModelIndex");
    qt->addSymbol(t_QModelIndex);
    QObjectType* t_QObject = new QObjectType(c, "QObject");
    qt->addSymbol(t_QObject);
    QLayoutItemType* t_QLayoutItem = new QLayoutItemType(c, "QLayoutItem");
    qt->addSymbol(t_QLayoutItem);
    QPaintDeviceType* t_QPaintDevice = new QPaintDeviceType(c, "QPaintDevice");
    qt->addSymbol(t_QPaintDevice);
    QRegExpType* t_QRegExp = new QRegExpType(c, "QRegExp");
    qt->addSymbol(t_QRegExp);
    QTextCursorType* t_QTextCursor = new QTextCursorType(c, "QTextCursor");
    qt->addSymbol(t_QTextCursor);
    QTextOptionType* t_QTextOption = new QTextOptionType(c, "QTextOption");
    qt->addSymbol(t_QTextOption);
    QTextBlockType* t_QTextBlock = new QTextBlockType(c, "QTextBlock");
    qt->addSymbol(t_QTextBlock);
    QMarginsType* t_QMargins = new QMarginsType(c, "QMargins");
    qt->addSymbol(t_QMargins);
    QStandardItemType* t_QStandardItem =
        new QStandardItemType(c, "QStandardItem");
    qt->addSymbol(t_QStandardItem);
    QListWidgetItemType* t_QListWidgetItem =
        new QListWidgetItemType(c, "QListWidgetItem");
    qt->addSymbol(t_QListWidgetItem);
    QTreeWidgetItemType* t_QTreeWidgetItem =
        new QTreeWidgetItemType(c, "QTreeWidgetItem");
    qt->addSymbol(t_QTreeWidgetItem);
    QTableWidgetItemType* t_QTableWidgetItem =
        new QTableWidgetItemType(c, "QTableWidgetItem");
    qt->addSymbol(t_QTableWidgetItem);
    QProcessEnvironmentType* t_QProcessEnvironment =
        new QProcessEnvironmentType(c, "QProcessEnvironment");
    qt->addSymbol(t_QProcessEnvironment);
    QFileInfoType* t_QFileInfo = new QFileInfoType(c, "QFileInfo");
    qt->addSymbol(t_QFileInfo);
    QDirType* t_QDir = new QDirType(c, "QDir");
    qt->addSymbol(t_QDir);
    QHostAddressType* t_QHostAddress = new QHostAddressType(c, "QHostAddress");
    qt->addSymbol(t_QHostAddress);
    QHostInfoType* t_QHostInfo = new QHostInfoType(c, "QHostInfo");
    qt->addSymbol(t_QHostInfo);
    QItemSelectionRangeType* t_QItemSelectionRange =
        new QItemSelectionRangeType(c, "QItemSelectionRange");
    qt->addSymbol(t_QItemSelectionRange);
    QItemSelectionType* t_QItemSelection =
        new QItemSelectionType(c, "QItemSelection");
    qt->addSymbol(t_QItemSelection);
    QGradientType* t_QGradient = new QGradientType(c, "QGradient");
    qt->addSymbol(t_QGradient);
    QBrushType* t_QBrush = new QBrushType(c, "QBrush");
    qt->addSymbol(t_QBrush);
    QMatrixType* t_QMatrix = new QMatrixType(c, "QMatrix");
    qt->addSymbol(t_QMatrix);
    QTransformType* t_QTransform = new QTransformType(c, "QTransform");
    qt->addSymbol(t_QTransform);
    QPaletteType* t_QPalette = new QPaletteType(c, "QPalette");
    qt->addSymbol(t_QPalette);
    QEventType* t_QEvent = new QEventType(c, "QEvent");
    qt->addSymbol(t_QEvent);
    QTextStreamType* t_QTextStream = new QTextStreamType(c, "QTextStream");
    qt->addSymbol(t_QTextStream);
    QTextCodecType* t_QTextCodec = new QTextCodecType(c, "QTextCodec");
    qt->addSymbol(t_QTextCodec);
    QNetworkCookieType* t_QNetworkCookie =
        new QNetworkCookieType(c, "QNetworkCookie");
    qt->addSymbol(t_QNetworkCookie);
    QCursorType* t_QCursor = new QCursorType(c, "QCursor");
    qt->addSymbol(t_QCursor);
    QWebEngineSettingsType* t_QWebEngineSettings =
        new QWebEngineSettingsType(c, "QWebEngineSettings");
    qt->addSymbol(t_QWebEngineSettings);
    QWebEngineHistoryType* t_QWebEngineHistory =
        new QWebEngineHistoryType(c, "QWebEngineHistory");
    qt->addSymbol(t_QWebEngineHistory);
    QFocusEventType* t_QFocusEvent =
        new QFocusEventType(c, "QFocusEvent", t_QEvent);
    qt->addSymbol(t_QFocusEvent);
    QTimerType* t_QTimer = new QTimerType(c, "QTimer", t_QObject);
    qt->addSymbol(t_QTimer);
    QWebChannelType* t_QWebChannel =
        new QWebChannelType(c, "QWebChannel", t_QObject);
    qt->addSymbol(t_QWebChannel);
    QPaintEventType* t_QPaintEvent =
        new QPaintEventType(c, "QPaintEvent", t_QEvent);
    qt->addSymbol(t_QPaintEvent);
    QIODeviceType* t_QIODevice = new QIODeviceType(c, "QIODevice", t_QObject);
    qt->addSymbol(t_QIODevice);
    QCompleterType* t_QCompleter =
        new QCompleterType(c, "QCompleter", t_QObject);
    qt->addSymbol(t_QCompleter);
    QHelpEventType* t_QHelpEvent =
        new QHelpEventType(c, "QHelpEvent", t_QEvent);
    qt->addSymbol(t_QHelpEvent);
    QQuickItemType* t_QQuickItem =
        new QQuickItemType(c, "QQuickItem", t_QObject);
    qt->addSymbol(t_QQuickItem);
    QItemSelectionModelType* t_QItemSelectionModel =
        new QItemSelectionModelType(c, "QItemSelectionModel", t_QObject);
    qt->addSymbol(t_QItemSelectionModel);
    QClipboardType* t_QClipboard =
        new QClipboardType(c, "QClipboard", t_QObject);
    qt->addSymbol(t_QClipboard);
    QTimerEventType* t_QTimerEvent =
        new QTimerEventType(c, "QTimerEvent", t_QEvent);
    qt->addSymbol(t_QTimerEvent);
    QConicalGradientType* t_QConicalGradient =
        new QConicalGradientType(c, "QConicalGradient", t_QGradient);
    qt->addSymbol(t_QConicalGradient);
    QJSEngineType* t_QJSEngine = new QJSEngineType(c, "QJSEngine", t_QObject);
    qt->addSymbol(t_QJSEngine);
    QPixmapType* t_QPixmap = new QPixmapType(c, "QPixmap", t_QPaintDevice);
    qt->addSymbol(t_QPixmap);
    QTextDocumentType* t_QTextDocument =
        new QTextDocumentType(c, "QTextDocument", t_QObject);
    qt->addSymbol(t_QTextDocument);
    QTcpServerType* t_QTcpServer =
        new QTcpServerType(c, "QTcpServer", t_QObject);
    qt->addSymbol(t_QTcpServer);
    QRadialGradientType* t_QRadialGradient =
        new QRadialGradientType(c, "QRadialGradient", t_QGradient);
    qt->addSymbol(t_QRadialGradient);
    QEventLoopType* t_QEventLoop =
        new QEventLoopType(c, "QEventLoop", t_QObject);
    qt->addSymbol(t_QEventLoop);
    QWebEnginePageType* t_QWebEnginePage =
        new QWebEnginePageType(c, "QWebEnginePage", t_QObject);
    qt->addSymbol(t_QWebEnginePage);
    QWebEngineCookieStoreType* t_QWebEngineCookieStore =
        new QWebEngineCookieStoreType(c, "QWebEngineCookieStore", t_QObject);
    qt->addSymbol(t_QWebEngineCookieStore);
    QWebEngineProfileType* t_QWebEngineProfile =
        new QWebEngineProfileType(c, "QWebEngineProfile", t_QObject);
    qt->addSymbol(t_QWebEngineProfile);
    QGestureEventType* t_QGestureEvent =
        new QGestureEventType(c, "QGestureEvent", t_QEvent);
    qt->addSymbol(t_QGestureEvent);
    QQmlContextType* t_QQmlContext =
        new QQmlContextType(c, "QQmlContext", t_QObject);
    qt->addSymbol(t_QQmlContext);
    QMimeDataType* t_QMimeData = new QMimeDataType(c, "QMimeData", t_QObject);
    qt->addSymbol(t_QMimeData);
    QButtonGroupType* t_QButtonGroup =
        new QButtonGroupType(c, "QButtonGroup", t_QObject);
    qt->addSymbol(t_QButtonGroup);
    QActionGroupType* t_QActionGroup =
        new QActionGroupType(c, "QActionGroup", t_QObject);
    qt->addSymbol(t_QActionGroup);
    QWindowStateChangeEventType* t_QWindowStateChangeEvent =
        new QWindowStateChangeEventType(c, "QWindowStateChangeEvent", t_QEvent);
    qt->addSymbol(t_QWindowStateChangeEvent);
    QLinearGradientType* t_QLinearGradient =
        new QLinearGradientType(c, "QLinearGradient", t_QGradient);
    qt->addSymbol(t_QLinearGradient);
    QDropEventType* t_QDropEvent =
        new QDropEventType(c, "QDropEvent", t_QEvent);
    qt->addSymbol(t_QDropEvent);
    QHideEventType* t_QHideEvent =
        new QHideEventType(c, "QHideEvent", t_QEvent);
    qt->addSymbol(t_QHideEvent);
    QResizeEventType* t_QResizeEvent =
        new QResizeEventType(c, "QResizeEvent", t_QEvent);
    qt->addSymbol(t_QResizeEvent);
    QAbstractItemModelType* t_QAbstractItemModel =
        new QAbstractItemModelType(c, "QAbstractItemModel", t_QObject);
    qt->addSymbol(t_QAbstractItemModel);
    QMoveEventType* t_QMoveEvent =
        new QMoveEventType(c, "QMoveEvent", t_QEvent);
    qt->addSymbol(t_QMoveEvent);
    QNetworkAccessManagerType* t_QNetworkAccessManager =
        new QNetworkAccessManagerType(c, "QNetworkAccessManager", t_QObject);
    qt->addSymbol(t_QNetworkAccessManager);
    QImageType* t_QImage = new QImageType(c, "QImage", t_QPaintDevice);
    qt->addSymbol(t_QImage);
    QDragLeaveEventType* t_QDragLeaveEvent =
        new QDragLeaveEventType(c, "QDragLeaveEvent", t_QEvent);
    qt->addSymbol(t_QDragLeaveEvent);
    QLayoutType* t_QLayout =
        new QLayoutType(c, "QLayout", t_QObject, t_QLayoutItem);
    qt->addSymbol(t_QLayout);
    QShortcutEventType* t_QShortcutEvent =
        new QShortcutEventType(c, "QShortcutEvent", t_QEvent);
    qt->addSymbol(t_QShortcutEvent);
    QCloseEventType* t_QCloseEvent =
        new QCloseEventType(c, "QCloseEvent", t_QEvent);
    qt->addSymbol(t_QCloseEvent);
    QNetworkCookieJarType* t_QNetworkCookieJar =
        new QNetworkCookieJarType(c, "QNetworkCookieJar", t_QObject);
    qt->addSymbol(t_QNetworkCookieJar);
    QCoreApplicationType* t_QCoreApplication =
        new QCoreApplicationType(c, "QCoreApplication", t_QObject);
    qt->addSymbol(t_QCoreApplication);
    QFileOpenEventType* t_QFileOpenEvent =
        new QFileOpenEventType(c, "QFileOpenEvent", t_QEvent);
    qt->addSymbol(t_QFileOpenEvent);
    QWidgetType* t_QWidget =
        new QWidgetType(c, "QWidget", t_QObject, t_QPaintDevice);
    qt->addSymbol(t_QWidget);
    QShowEventType* t_QShowEvent =
        new QShowEventType(c, "QShowEvent", t_QEvent);
    qt->addSymbol(t_QShowEvent);
    QActionType* t_QAction = new QActionType(c, "QAction", t_QObject);
    qt->addSymbol(t_QAction);
    QInputEventType* t_QInputEvent =
        new QInputEventType(c, "QInputEvent", t_QEvent);
    qt->addSymbol(t_QInputEvent);
    QStandardItemModelType* t_QStandardItemModel = new QStandardItemModelType(
        c, "QStandardItemModel", t_QAbstractItemModel);
    qt->addSymbol(t_QStandardItemModel);
    QToolBarType* t_QToolBar = new QToolBarType(c, "QToolBar", t_QWidget);
    qt->addSymbol(t_QToolBar);
    QGridLayoutType* t_QGridLayout =
        new QGridLayoutType(c, "QGridLayout", t_QLayout);
    qt->addSymbol(t_QGridLayout);
    QContextMenuEventType* t_QContextMenuEvent =
        new QContextMenuEventType(c, "QContextMenuEvent", t_QInputEvent);
    qt->addSymbol(t_QContextMenuEvent);
    QProcessType* t_QProcess = new QProcessType(c, "QProcess", t_QIODevice);
    qt->addSymbol(t_QProcess);
    QQmlEngineType* t_QQmlEngine =
        new QQmlEngineType(c, "QQmlEngine", t_QJSEngine);
    qt->addSymbol(t_QQmlEngine);
    QWheelEventType* t_QWheelEvent =
        new QWheelEventType(c, "QWheelEvent", t_QInputEvent);
    qt->addSymbol(t_QWheelEvent);
    QBoxLayoutType* t_QBoxLayout =
        new QBoxLayoutType(c, "QBoxLayout", t_QLayout);
    qt->addSymbol(t_QBoxLayout);
    QHoverEventType* t_QHoverEvent =
        new QHoverEventType(c, "QHoverEvent", t_QInputEvent);
    qt->addSymbol(t_QHoverEvent);
    QSvgWidgetType* t_QSvgWidget =
        new QSvgWidgetType(c, "QSvgWidget", t_QWidget);
    qt->addSymbol(t_QSvgWidget);
    QAbstractButtonType* t_QAbstractButton =
        new QAbstractButtonType(c, "QAbstractButton", t_QWidget);
    qt->addSymbol(t_QAbstractButton);
    QTabBarType* t_QTabBar = new QTabBarType(c, "QTabBar", t_QWidget);
    qt->addSymbol(t_QTabBar);
    QStatusBarType* t_QStatusBar =
        new QStatusBarType(c, "QStatusBar", t_QWidget);
    qt->addSymbol(t_QStatusBar);
    QQuickWidgetType* t_QQuickWidget =
        new QQuickWidgetType(c, "QQuickWidget", t_QWidget);
    qt->addSymbol(t_QQuickWidget);
    QLocalSocketType* t_QLocalSocket =
        new QLocalSocketType(c, "QLocalSocket", t_QIODevice);
    qt->addSymbol(t_QLocalSocket);
    QTabletEventType* t_QTabletEvent =
        new QTabletEventType(c, "QTabletEvent", t_QInputEvent);
    qt->addSymbol(t_QTabletEvent);
    QKeyEventType* t_QKeyEvent =
        new QKeyEventType(c, "QKeyEvent", t_QInputEvent);
    qt->addSymbol(t_QKeyEvent);
    QFormLayoutType* t_QFormLayout =
        new QFormLayoutType(c, "QFormLayout", t_QLayout);
    qt->addSymbol(t_QFormLayout);
    QTabWidgetType* t_QTabWidget =
        new QTabWidgetType(c, "QTabWidget", t_QWidget);
    qt->addSymbol(t_QTabWidget);
    QMouseEventType* t_QMouseEvent =
        new QMouseEventType(c, "QMouseEvent", t_QInputEvent);
    qt->addSymbol(t_QMouseEvent);
    QLineEditType* t_QLineEdit = new QLineEditType(c, "QLineEdit", t_QWidget);
    qt->addSymbol(t_QLineEdit);
    QMainWindowType* t_QMainWindow =
        new QMainWindowType(c, "QMainWindow", t_QWidget);
    qt->addSymbol(t_QMainWindow);
    QDragMoveEventType* t_QDragMoveEvent =
        new QDragMoveEventType(c, "QDragMoveEvent", t_QDropEvent);
    qt->addSymbol(t_QDragMoveEvent);
    QAbstractListModelType* t_QAbstractListModel = new QAbstractListModelType(
        c, "QAbstractListModel", t_QAbstractItemModel);
    qt->addSymbol(t_QAbstractListModel);
    QDesktopWidgetType* t_QDesktopWidget =
        new QDesktopWidgetType(c, "QDesktopWidget", t_QWidget);
    qt->addSymbol(t_QDesktopWidget);
    QAbstractSocketType* t_QAbstractSocket =
        new QAbstractSocketType(c, "QAbstractSocket", t_QIODevice);
    qt->addSymbol(t_QAbstractSocket);
    QBitmapType* t_QBitmap = new QBitmapType(c, "QBitmap", t_QPixmap);
    qt->addSymbol(t_QBitmap);
    QMenuType* t_QMenu = new QMenuType(c, "QMenu", t_QWidget);
    qt->addSymbol(t_QMenu);
    QMenuBarType* t_QMenuBar = new QMenuBarType(c, "QMenuBar", t_QWidget);
    qt->addSymbol(t_QMenuBar);
    QComboBoxType* t_QComboBox = new QComboBoxType(c, "QComboBox", t_QWidget);
    qt->addSymbol(t_QComboBox);
    QFrameType* t_QFrame = new QFrameType(c, "QFrame", t_QWidget);
    qt->addSymbol(t_QFrame);
    QWebEngineViewType* t_QWebEngineView =
        new QWebEngineViewType(c, "QWebEngineView", t_QWidget);
    qt->addSymbol(t_QWebEngineView);
    QAbstractSliderType* t_QAbstractSlider =
        new QAbstractSliderType(c, "QAbstractSlider", t_QWidget);
    qt->addSymbol(t_QAbstractSlider);
    QGroupBoxType* t_QGroupBox = new QGroupBoxType(c, "QGroupBox", t_QWidget);
    qt->addSymbol(t_QGroupBox);
    QGuiApplicationType* t_QGuiApplication =
        new QGuiApplicationType(c, "QGuiApplication", t_QCoreApplication);
    qt->addSymbol(t_QGuiApplication);
    QAbstractSpinBoxType* t_QAbstractSpinBox =
        new QAbstractSpinBoxType(c, "QAbstractSpinBox", t_QWidget);
    qt->addSymbol(t_QAbstractSpinBox);
    QProgressBarType* t_QProgressBar =
        new QProgressBarType(c, "QProgressBar", t_QWidget);
    qt->addSymbol(t_QProgressBar);
    QAbstractTableModelType* t_QAbstractTableModel =
        new QAbstractTableModelType(c, "QAbstractTableModel",
                                    t_QAbstractItemModel);
    qt->addSymbol(t_QAbstractTableModel);
    QDockWidgetType* t_QDockWidget =
        new QDockWidgetType(c, "QDockWidget", t_QWidget);
    qt->addSymbol(t_QDockWidget);
    QNetworkReplyType* t_QNetworkReply =
        new QNetworkReplyType(c, "QNetworkReply", t_QIODevice);
    qt->addSymbol(t_QNetworkReply);
    QDialogType* t_QDialog = new QDialogType(c, "QDialog", t_QWidget);
    qt->addSymbol(t_QDialog);
    QFileDeviceType* t_QFileDevice =
        new QFileDeviceType(c, "QFileDevice", t_QIODevice);
    qt->addSymbol(t_QFileDevice);
    QWidgetActionType* t_QWidgetAction =
        new QWidgetActionType(c, "QWidgetAction", t_QAction);
    qt->addSymbol(t_QWidgetAction);
    QHBoxLayoutType* t_QHBoxLayout =
        new QHBoxLayoutType(c, "QHBoxLayout", t_QBoxLayout);
    qt->addSymbol(t_QHBoxLayout);
    QColorDialogType* t_QColorDialog =
        new QColorDialogType(c, "QColorDialog", t_QDialog);
    qt->addSymbol(t_QColorDialog);
    QCheckBoxType* t_QCheckBox =
        new QCheckBoxType(c, "QCheckBox", t_QAbstractButton);
    qt->addSymbol(t_QCheckBox);
    QApplicationType* t_QApplication =
        new QApplicationType(c, "QApplication", t_QGuiApplication);
    qt->addSymbol(t_QApplication);
    QTcpSocketType* t_QTcpSocket =
        new QTcpSocketType(c, "QTcpSocket", t_QAbstractSocket);
    qt->addSymbol(t_QTcpSocket);
    QSpinBoxType* t_QSpinBox =
        new QSpinBoxType(c, "QSpinBox", t_QAbstractSpinBox);
    qt->addSymbol(t_QSpinBox);
    QToolButtonType* t_QToolButton =
        new QToolButtonType(c, "QToolButton", t_QAbstractButton);
    qt->addSymbol(t_QToolButton);
    QPushButtonType* t_QPushButton =
        new QPushButtonType(c, "QPushButton", t_QAbstractButton);
    qt->addSymbol(t_QPushButton);
    QQmlApplicationEngineType* t_QQmlApplicationEngine =
        new QQmlApplicationEngineType(c, "QQmlApplicationEngine", t_QQmlEngine);
    qt->addSymbol(t_QQmlApplicationEngine);
    QUdpSocketType* t_QUdpSocket =
        new QUdpSocketType(c, "QUdpSocket", t_QAbstractSocket);
    qt->addSymbol(t_QUdpSocket);
    QDialType* t_QDial = new QDialType(c, "QDial", t_QAbstractSlider);
    qt->addSymbol(t_QDial);
    QFileDialogType* t_QFileDialog =
        new QFileDialogType(c, "QFileDialog", t_QDialog);
    qt->addSymbol(t_QFileDialog);
    QSliderType* t_QSlider = new QSliderType(c, "QSlider", t_QAbstractSlider);
    qt->addSymbol(t_QSlider);
    QSplitterType* t_QSplitter = new QSplitterType(c, "QSplitter", t_QFrame);
    qt->addSymbol(t_QSplitter);
    QStackedWidgetType* t_QStackedWidget =
        new QStackedWidgetType(c, "QStackedWidget", t_QFrame);
    qt->addSymbol(t_QStackedWidget);
    QVBoxLayoutType* t_QVBoxLayout =
        new QVBoxLayoutType(c, "QVBoxLayout", t_QBoxLayout);
    qt->addSymbol(t_QVBoxLayout);
    QRadioButtonType* t_QRadioButton =
        new QRadioButtonType(c, "QRadioButton", t_QAbstractButton);
    qt->addSymbol(t_QRadioButton);
    QLabelType* t_QLabel = new QLabelType(c, "QLabel", t_QFrame);
    qt->addSymbol(t_QLabel);
    QToolBoxType* t_QToolBox = new QToolBoxType(c, "QToolBox", t_QFrame);
    qt->addSymbol(t_QToolBox);
    QAbstractScrollAreaType* t_QAbstractScrollArea =
        new QAbstractScrollAreaType(c, "QAbstractScrollArea", t_QFrame);
    qt->addSymbol(t_QAbstractScrollArea);
    QFileType* t_QFile = new QFileType(c, "QFile", t_QFileDevice);
    qt->addSymbol(t_QFile);
    QDragEnterEventType* t_QDragEnterEvent =
        new QDragEnterEventType(c, "QDragEnterEvent", t_QDragMoveEvent);
    qt->addSymbol(t_QDragEnterEvent);
    QInputDialogType* t_QInputDialog =
        new QInputDialogType(c, "QInputDialog", t_QDialog);
    qt->addSymbol(t_QInputDialog);
    QScrollAreaType* t_QScrollArea =
        new QScrollAreaType(c, "QScrollArea", t_QAbstractScrollArea);
    qt->addSymbol(t_QScrollArea);
    QTextEditType* t_QTextEdit =
        new QTextEditType(c, "QTextEdit", t_QAbstractScrollArea);
    qt->addSymbol(t_QTextEdit);
    QPlainTextEditType* t_QPlainTextEdit =
        new QPlainTextEditType(c, "QPlainTextEdit", t_QAbstractScrollArea);
    qt->addSymbol(t_QPlainTextEdit);
    QAbstractItemViewType* t_QAbstractItemView = new QAbstractItemViewType(
        c, "QAbstractItemView", t_QAbstractScrollArea);
    qt->addSymbol(t_QAbstractItemView);
    QTableViewType* t_QTableView =
        new QTableViewType(c, "QTableView", t_QAbstractItemView);
    qt->addSymbol(t_QTableView);
    QTextBrowserType* t_QTextBrowser =
        new QTextBrowserType(c, "QTextBrowser", t_QTextEdit);
    qt->addSymbol(t_QTextBrowser);
    QHeaderViewType* t_QHeaderView =
        new QHeaderViewType(c, "QHeaderView", t_QAbstractItemView);
    qt->addSymbol(t_QHeaderView);
    QListViewType* t_QListView =
        new QListViewType(c, "QListView", t_QAbstractItemView);
    qt->addSymbol(t_QListView);
    QTreeViewType* t_QTreeView =
        new QTreeViewType(c, "QTreeView", t_QAbstractItemView);
    qt->addSymbol(t_QTreeView);
    QColumnViewType* t_QColumnView =
        new QColumnViewType(c, "QColumnView", t_QAbstractItemView);
    qt->addSymbol(t_QColumnView);
    QListWidgetType* t_QListWidget =
        new QListWidgetType(c, "QListWidget", t_QListView);
    qt->addSymbol(t_QListWidget);
    QTreeWidgetType* t_QTreeWidget =
        new QTreeWidgetType(c, "QTreeWidget", t_QTreeView);
    qt->addSymbol(t_QTreeWidget);
    QTableWidgetType* t_QTableWidget =
        new QTableWidgetType(c, "QTableWidget", t_QTableView);
    qt->addSymbol(t_QTableWidget);
    return t_QWidget;
}
