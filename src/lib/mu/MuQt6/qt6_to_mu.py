#!/usr/bin/env python
#
# Copyright (C) 2024  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
from sgmllib import SGMLParser
import urllib
import sys
import traceback
import pprint
import string
import re
import pickle
import os

#
#   Qt 5 splits the docs into subdirs which complicates things slightly.
#
#   We need to at least use objectHTML and qtHTML as the roots for
#   recursiveParse() below. Other base classes may be generated as well.
#

forceParse = False
# forceParse = True

pp = pprint.PrettyPrinter()

htmlDir = "/home/cfuoco/Qt6.5.3/Docs/Qt-6.5.3"
objectHTML = "%s/qtcore/qobject.html" % htmlDir
qtHTML = "%s/qtcore/qt.html" % htmlDir
qtapifile = "qt6api.p"
templateObjectH = "templates/TemplateObjectType.h"
templateObjectCPP = "templates/TemplateObjectType.cpp"
templateLayoutItemH = "templates/TemplateLayoutItemType.h"
templateLayoutItemCPP = "templates/TemplateLayoutItemType.cpp"
templatePaintDeviceH = "templates/TemplatePaintDeviceType.h"
templatePaintDeviceCPP = "templates/TemplatePaintDeviceType.cpp"
templateIODeviceBaseH = "templates/TemplateIODeviceBaseType.h"
templateIODeviceBaseCPP = "templates/TemplateIODeviceBaseType.cpp"
templateTypeH = "templates/TemplateType.h"
templateTypeCPP = "templates/TemplateType.cpp"
templatePointerTypeH = "templates/TemplatePointerType.h"
templatePointerTypeCPP = "templates/TemplatePointerType.cpp"
templateInterfaceTypeH = "templates/TemplateInterfaceType.h"
templateInterfaceTypeCPP = "templates/TemplateInterfaceType.cpp"
templateGlobalH = "templates/TemplateGlobal.h"
templateGlobalCPP = "templates/TemplateGlobal.cpp"

#
#   Setting this to true causes the library to be generated with
#   unique subclasses for every Qt class. This allows inheritance from
#   Mu.
#

allowInheritance = True

#
# base class html files: these are recursively scanned
#

xbaseHTML = ["qobject"]

#
#   In 5.6 they no longer have a list of all the classes that inherit
#   from a base class so we have include the whole shebang here instead
#   of only a few root files
#
baseHTML = [
    "qt",
    "qobject",
    "qwindow",
    "qsurface",
    "qwidget",
    "qbitmap",
    "qimage",
    "qactionevent",
    "qcloseevent",
    "qcontextmenuevent",
    "qgradient",
    "qconicalgradient",
    "qlineargradient",
    "qradialgradient",
    "qdragmoveevent",
    "qdragleaveevent",
    "qdropevent",
    "qlayout",
    "qenterevent",
    "qexposeevent",
    "qfileopenevent",
    "qactiongroup",
    "qfocusevent",
    "qhelpevent",
    "qhideevent",
    "qhoverevent",
    "qicondragevent",
    "qinputevent",
    "qinputmethodevent",
    "qinputmethodqueryevent",
    "qkeyevent",
    "qmoveevent",
    "qpaintevent",
    "qresizeevent",
    "qscrollevent",
    "qscrollprepareevent",
    "qshortcutevent",
    "qshowevent",
    "qstatustipevent",
    "qtabletevent",
    "qtouchevent",
    "qfont",
    "qsize",
    "qpoint",
    "qrect",
    "qicon",
    "qvariant",
    "qstandarditem",
    "qmodelindex",
    "qpixmap",
    "qdatetime",
    "qtime",
    "qdate",
    "qkeysequence",
    "qevent",
    "qregularexpression",
    "qregion",
    "qcolor",
    "qlistwidgetitem",
    "qtreewidgetitem",
    "qtablewidgetitem",
    "qurl",
    "qurlquery",
    "qbytearray",
    "qlayoutitem",
    "qpaintdevice",
    "qhostaddress",
    "qhostinfo",
    "qtimer",
    "qdesktopservices",
    "qdir",
    "qfileinfo",
    "qdatastream",
    "qitemselectionrange",
    "qitemselection",
    "qmargins",
    "qbrush",
    "qmatrix4x4",
    "qtransform",
    "qpointf",
    "qpalette",
    "qnetworkcookie",
    "qtextstream",
    "qstringconverter",
    "qpainter",
    "qpainterpath",
    "qrectf",
    "qtextcursor",
    "qtextoption",
    "qtextblock",
    "qclipboard",
    "qprocessenvironment",
    "qcursor",
    "qaction",
    "qbuttongroup",
    "qcompleter",
    "qstandarditemmodel",
    "qsvgwidget",
    "qapplication",
    "qguiapplication",
    "qtextdocument",
    "qwidgetaction",
    "qnetworkreply",
    "qabstractsocket",
    "qtcpsocket",
    "qlocalsocket",
    "qgestureevent",
    "qtcpserver",
    "qfiledevice",
    "qwindowstatechangeevent",
    "qnetworkcookiejar",
    "qnetworkaccessmanager",
    "qquickitem",
    "qqmlengine",
    "qjsengine",
    "qqmlapplicationengine",
    "qqmlcontext",
    "qquickwidget",
    "qwebenginecookiestore",
    "qwebengineprofile",
    "qwebengineview",
    "qwebenginepage",
    "qwebenginesettings",
    "qwebchannel",
    "qwebenginehistory",
    "qwebenginehistoryitem",
    "qboxlayout",
    "qformlayout",
    "qgridlayout",
    "qstackedlayout",
    "qhboxlayout",
    "qvboxlayout",
    "qdockwidget",
    "qtoolbar",
    "qmainwindow",
    "qslider",
    "qdial",
    "qlineedit",
    "qframe",
    "qabstractslider",
    "qabstractbutton",
    "qcheckbox",
    "qpushbutton",
    "qradiobutton",
    "qtoolbutton",
    "qmenu",
    "qcombobox",
    "qfiledialog",
    "qabstractitemmodel",
    "qabstractlistmodel",
    "qabstracttablemodel",
    "qabstractitemview",
    "qcolordialog",
    "qdialog",
    "qcolumnview",
    "qheaderview",
    "qlistview",
    "qtableview",
    "qtreeview",
    "qlistwidget",
    "qtreewidget",
    "qtablewidget",
    "qcoreapplication",
    "qlabel",
    "qmenubar",
    "qplaintextedit",
    "qprogressbar",
    "qscrollarea",
    "qspinbox",
    "qsplitter",
    "qstackedwidget",
    "qstatusbar",
    "qtabbar",
    "qtabwidget",
    "qtextedit",
    "qtoolbox",
    "qabstractspinbox",
    "qabstractscrollarea",
    "qinputdialog",
    "qeventloop",
    "qgroupbox",
    "qiodevice",
    "qiodevicebase",
    "qprocess",
    "qfile",
    "qudpsocket",
    "qscreen",
    "qitemselectionmodel",
    "qspaceritem",
    "qwidgetitem",
    "qmimedata",
    "qtimerevent",
    "qdragenterevent",
    "qmouseevent",
    "qwheelevent",
    "qtextbrowser",
    "qsinglepointevent",
    "qpointerevent",
    "qtimezone",
    "qcalendar",
]

#
#   These classes use the standard Mu ClassInstance to hold their data
#   and have the indicated data members. The primitiveTypes actually
#   embed the data type in the Instance object. The pointerTypes use a
#   named field to hold the native object.
#

primitiveTypes = set(
    [
        "QSize",
        "QRect",
        "QRectF",
        "QPoint",
        "QPointF",
        "QRegularExpression",
        "QIcon",
        "QPixmap",
        "QBitmap",
        "QImage",
        "QKeySequence",
        "QColor",
        "QVariant",
        "QRegion",
        "QDateTime",
        "QTime",
        "QDate",
        "QModelIndex",
        "QFont",
        "QUrl",
        "QUrlQuery",
        "QByteArray",
        "QDataStream",
        "QHostAddress",
        "QHostInfo",
        "QDir",
        "QFileInfo",
        "QImageWriter",
        "QItemSelectionRange",
        "QItemSelection",
        "QBrush",
        "QGradient",
        "QMatrix4x4",
        "QTransform",
        "QConicalGradient",
        "QLinearGradient",
        "QRadialGradient",
        "QPalette",
        "QPainter",
        "QPainterPath",
        "QMargins",
        "QNetworkCookie",
        "QProcessEnvironment",
        "QTextCursor",
        "QTextOption",
        "QTextBlock",
        "QCursor",
        "QTimeZone",
        "QCalendar",
    ]
)

copyOnWriteTypes = [
    "QBitArray",
    "QBitmap",
    "QBrush",
    "QByteArray",
    "QCache",
    "QContiguousCache",
    "QCursor",
    "QDir",
    "QFileInfo",
    "QFont",
    "QFontInfo",
    "QFontMetrics",
    "QFontMetricsF",
    "QGLColormap",
    "QGradient",
    "QHash",
    "QIcon",
    "QImage",
    "QKeySequence",
    "QLinkedList",
    "QList",
    "QLocale",
    "QMap",
    "QMultiHash",
    "QMultiMap",
    "QPainterPath",
    "QPalette",
    "QPen",
    "QPicture",
    "QPixmap",
    "QPolygon",
    "QPolygonF",
    "QQueue",
    # Qt6: Removed for QRegularExpression
    # "QRegExp",
    "QRegularExpression",
    "QRegion",
    "QSet",
    "QSqlField",
    "QSqlQuery",
    "QSqlRecord",
    "QStack",
    "QString",
    "QStringList",
    "QTextBoundaryFinder",
    "QTextCursor",
    "QTextDocumentFragment",
    "QTextFormat",
    "QUrl",
    "QUrlQuery",
    "QVariant",
    "QVector",
    "QX11Info",
    "QNetworkCookie",
    "QProcessEnvironment",
]

# interfaceTypes = set(["QLayoutItem", "QPaintDevice"])
interfaceTypes = set([])

pointerTypes = set(
    [
        "QStandardItem",
        "QListWidgetItem",
        "QTreeWidgetItem",
        "QTableWidgetItem",
        "QEvent",
        "QResizeEvent",
        "QTextStream",
        "QDragLeaveEvent",
        "QDragMoveEvent",
        "QDropEvent",
        "QCloseEvent",
        "QDragEnterEvent",
        "QInputEvent",
        "QMoveEvent",
        "QPaintEvent",
        "QShortcutEvent",
        "QShowEvent",
        "QTimerEvent",
        "QFocusEvent",
        "QContextMenuEvent",
        "QKeyEvent",
        "QMouseEvent",
        "QTabletEvent",
        "QWheelEvent",
        "QHelpEvent",
        "QFileOpenEvent",
        "QHideEvent",
        "QHoverEvent",
        "QGestureEvent",
        "QStringConverter",
        "QWebEngineSettings",
        # "QWebEngineHistory",
        "QWindowStateChangeEvent",
        "QSinglePointEvent",
        "QPointerEvent",
    ]
)
# "QTouchEvent",

notInheritableTypes = set(["QNetworkReply", "QIODevice", "QIODeviceBase", "QClipboard"])


def isAPrimitiveType(name):
    return name in primitiveTypes


def isAPointerType(name):
    return name in pointerTypes


def isCopyOnWrite(name):
    return isAPrimitiveType(name) and (name in copyOnWriteTypes)


#
#   C++ things which modify the meat of class name
#

typeElaborations = ["const", "virtual", "*", "&"]


def sansElaborations(x):
    if "*" in x:
        x = string.replace(x, "*", "")
    for i in x.split(" &*"):
        if i not in typeElaborations:
            return i


#
#   Map from C++ types to Mu types. Additional elements are added
#   during parsing.
#

translationMap = {
    "void": "void",
    "int": "int",
    "uint": "int",
    "bool": "bool",
    "double": "double",
    "float": "float",
    "qreal": "double",
    "qint64": "int64",
    "quint32": "int",
    "QString": "string",
    "QStringList": "string[]",
    "QModelIndexList": "qt.QModelIndex[]",
    "QFileInfoList": "qt.QFileInfo[]",
    "QRgb": "int",
    "QList<QTreeWidgetItem * >": "qt.QTreeWidgetItem[]",
    # "QList<QQuickItem * >"      : "qt.QQuickItem[]",
    "const QList<QTreeWidgetItem * > &": "qt.QTreeWidgetItem[]",
    "QList<QListWidgetItem * >": "qt.QListWidgetItem[]",
    "const QList<QListWidgetItem * > &": "qt.QListWidgetItem[]",
    "QList<QTableWidgetItem * >": "qt.QTableWidgetItem[]",
    "const QList<QTableWidgetItem * > &": "qt.QTableWidgetItem[]",
    "QList<QStandardItem * >": "qt.QStandardItem[]",
    "const QList<QStandardItem * > &": "qt.QStandardItem[]",
    "QList<QUrl>": "qt.QUrl[]",
    "QList<QNetworkCookie>": "qt.QNetworkCookie[]",
    # "const char *"              : "string",
    # "const QSizeF &"            : "qt.QSizeF",
    # "const QPointF &"           : "qt.QPointF",
    # "const QLineF &"            : "qt.QLineF",
    # "const QRectF &"            : "qt.QRectF",
    # "const QRegularExpression &"           : "qt.QRegularExpression",
    # "const QLine &"             : "qt.QLine",
    # "const QDate &"             : "qt.QDate",
    # "const QTime &"             : "qt.QTime",
}

#
#   Map from Mu type (in regex form) to Mu machine representation. The
#   tuple value contains (rep * more specific type) in case it matters.
#
#   NOTE: the order here matters! They are test in order
#

repMap = [
    (re.compile("bool"), ("bool", "bool")),
    (re.compile("void"), ("void", "void")),
    (re.compile("int64"), ("int64", "int64")),
    (re.compile("int"), ("int", "int")),
    (re.compile("float"), ("float", "float")),
    (re.compile("double"), ("double", "double")),
    (re.compile("short"), ("short", "short")),
    (re.compile("byte"), ("byte", "byte")),
    (re.compile("char"), ("char", "char")),
    (re.compile("flags .*"), ("int", "int")),
    (re.compile("string"), ("Pointer", "StringType::String")),
    (re.compile("string\\[\\]"), ("Pointer", "DynamicArray")),
    (re.compile("qt.QTreeWidgetItem\\[\\]"), ("Pointer", "DynamicArray")),
    (re.compile("qt.QListWidgetItem\\[\\]"), ("Pointer", "DynamicArray")),
    (re.compile("qt.QTableWidgetItem\\[\\]"), ("Pointer", "DynamicArray")),
    (re.compile("qt.QStandardItem\\[\\]"), ("Pointer", "DynamicArray")),
    (re.compile("qt.QUrl\\[\\]"), ("Pointer", "DynamicArray")),
    (re.compile(".*"), ("Pointer", "ClassInstance")),
]


def repMapFind(x):
    for rexp, val in repMap:
        if rexp.match(x):
            return val
    return None


convertFromMap = {
    "string": ("qstring($E)", "QString"),
    "string[]": ("qstringlist($E)", "QStringList"),
    "qt.QModelIndex[]": ("qmodelindexlist($E)", "QModelIndexList"),
    "qt.QFileInfo[]": ("qfileinfolist($E)", "QFileInfoList"),
    # "string"     : ("$E->c_str()", "const char *"),
    "qt.QObject": ("object<$T>($E)", "$T*"),
    "qt.QLayoutItem": ("layoutitem<$T>($E)", "$T*"),
    "qt.QPaintDevice": ("paintdevice<$T>($E)", "$T*"),
    "qt.QSpacerItem": ("layoutitem<$T>($E)", "$T*"),
    "qt.QWidgetItem": ("layoutitem<$T>($E)", "$T*"),
    # "qt.QTreeWidgetItem[]" : ("qpointerlist<$T>($E)", "$T*"),
    "qt.QIODeviceBase": ("iodevicebase<$T>($E)", "$T*"),
}

convertToMap = {
    "QString": ("makestring(c,$E)", "string"),
    "string": ("makestring(c,$E)", "string"),
    "QStringList": ("makestringlist(c,$E)", "string[]"),
    "QModelIndexList": ("makeqmodelindexlist(c,$E)", "qt.QModelIndex[]"),
    "QFileInfoList": ("makeqfileinfolist(c,$E)", "qt.QFileInfo[]"),
    # "const char *" : ("$E->c_str()", "string"),
}

setMap = {
    # "const char *" : "$O->string() = $E",
}

includeClasses = set(
    [
        "QAbstractButton",
        "QComboBox",
        "QMenu",
        "QToolBar",
        "QAbstractItemModel",
        "QCompleter",
        "QObject",
        "QToolButton",
        "QAbstractListModel",
        "QDial",
        "QPixmap",
        "QAbstractSlider",
        "QDialog",
        "QPushButton",
        "QAbstractTableModel",
        "QDockWidget",
        "QRadioButton",
        "QActionGroup",
        "QFrame",
        "QWidget",
        "QFileDialog",
        "QAction",
        "QIcon",
        "QSlider",
        "QStandardItem",
        "QCheckBox",
        "QLineEdit",
        "QStandardItemModel",
        "QBitmap",
        "QColorDialog",
        "QMainWindow",
        "QSvgWidget",
        "QModelIndex",
        "QCoreApplication",
        "QApplication",
        "QUrl",
        "QByteArray",
        "QMenuBar",
        "QLabel",
        "QPlainTextEdit",
        "QProgressBar",
        "QScrollArea",
        "QSpinBox",
        "QSplitter",
        "QStackedWidget",
        "QStatusBar",
        "QTabBar",
        "QTabWidget",
        "QTextDocument",
        "QTextEdit",
        "QToolBox",
        "QWidgetAction",
        "QAbstractSpinBox",
        "QAbstractScrollArea",
        "QButtonGroup",
        "QRect",
        "QRegion",
        "QSize",
        "QPoint",
        "QFont",
        "QVariant",
        "QStandardItem",
        "QColor",
        "QTreeWidgetItem",
        "QListWidgetItem",
        "QTableWidgetItem",
        "QListWidget",
        "QTableWidget",
        "QTreeWidget",
        "QTableView",
        "QListView",
        "QTreeView",
        "QAbstractItemView",
        "QColumnView",
        "QHeaderView",
        "QLayout",
        "QBoxLayout",
        "QHBoxLayout",
        "QVBoxLayout",
        "QFormLayout",
        "QGridLayout",
        "QLayoutItem",
        "QPaintDevice",
        "QImage",
        "QInputDialog",
        "QEventLoop",
        "QGroupBox",
        "QIODevice",
        "QIODeviceBase",
        "QDateTime",
        "QProcess",
        "QFile",
        "QAbstractSocket",
        "QDir",
        "QTcpSocket",
        "QUdpSocket",
        "QTcpServer",
        "QHostAddress",
        "QHostInfo",
        "QLocalSocket",
        "QTimer",
        "QTimeZone",
        "QCalendar",
        "QScreen",
        "QFileInfo",
        "QItemSelection",
        "QItemSelectionRange",
        "QItemSelectionModel",
        "QGradient",
        "QBrush",
        "QMatrix4x4",
        "QTransform",
        "QConicalGradient",
        "QLinearGradient",
        "QRadialGradient",
        "QTime",
        "QPointF",
        "QPalette",
        "QRectF",
        "QDate",
        "QNetworkAccessManager",
        "QNetworkReply",
        "QKeySequence",
        "QEvent",
        "QResizeEvent",
        "QDragLeaveEvent",
        "QDragMoveEvent",
        "QDropEvent",
        "QDragEnterEvent",
        "QInputEvent",
        "QMoveEvent",
        "QPaintEvent",
        "QShortcutEvent",
        "QShowEvent",
        "QTimerEvent",
        "QFocusEvent",
        "QGestureEvent",
        "QWheelEvent",
        "QTabletEvent",
        "QMimeData",
        "QContextMenuEvent",
        "QKeyEvent",
        "QMouseEvent",
        "QSinglePointEvent",
        "QPointerEvent",
        "QWindowStateChangeEvent",
        "QHideEvent",
        "QHoverEvent",
        "QHelpEvent",
        "QFileOpenEvent",
        "QRegularExpression",
        "QTextCursor",
        "QTextOption",
        "QTextBlock",
        "QMargins",
        "QCloseEvent",
        "QClipboard",
        "QNetworkCookie",
        # Qt6: Removed for QStringConverter
        # "QTextCodec",
        "QStringConverter",
        "QTextStream",
        "QTextBrowser",
        "QNetworkCookieJar",
        "QProcessEnvironment",
        "QCursor",
        "QFileDevice",
        "QGuiApplication",
        "QWebEngineCookieStore",
        "QWebEngineProfile",
        "QWebEngineView",
        "QWebEnginePage",
        "QWebEngineSettings",
        "QWebChannel",
        "QWebEngineHistory",
        "QUrlQuery",
        # "QStyleOption",           "QStyle",
        # "QPainter", "QPainterPath",
        # "QTouchEvent",
        # "QDesktopServices",
        # "QDataStream",
        "QQuickItem",
        "QQmlContext",
        "QJSEngine",
        "QQmlEngine",
        "QQmlApplicationEngine",
        "QQuickWidget",
        "QString",  # has to be here
        "QStringList",
        "QModelIndexList",
        "QFileInfoList",
        "QList<QTreeWidgetItem * >",
        "QList<QListWidgetItem * >",
        "QList<QTableWidgetItem * >",
        "QList<QStandardItem * >",
        "QList<QUrl>",
        "QList<QNetworkCookie>",
    ]
)

abstractClasses = [
    re.compile("QAbstract.*"),
    re.compile("QLayout"),
    re.compile("QIODevice"),
    re.compile("QIODeviceBase"),
]


def isAbstract(name):
    for reg in abstractClasses:
        if reg.match(name):
            return True
    return False


def isConstReference(t):
    parts = t.split(" ")
    if len(parts) == 3 and parts[0] == "const" and parts[2] == "&":
        return True
    else:
        return False


def isPointerToSomething(t):
    parts = t.split(" ")
    if len(parts) >= 2 and parts[-1] == "*":
        return True
    else:
        return False


def pointedToType(t):
    parts = t.split(" ")
    if len(parts) == 2:
        return parts[0]
    elif parts[0] == "const":
        return parts[1]
    else:
        return parts[0]


def constReferenceType(t):
    return t.split(" ")[1]


def indexOf(element, sequence):
    for i in range(0, len(sequence)):
        if element == sequence[i]:
            return i
    return -1


#
#   Most enums should be handled by the metaObject system, however
#   there are some that are not declared like that. They should be
#   listed here.
#

forceEnumOutput = {
    "QIODeviceBase": ["OpenModeFlag"],
    "QProcess": [
        "ExitStatus",
        "ProcessChannel",
        "ProcessChannelMode",
        "ProcessError",
        "ProcessState",
    ],
    "QAbstractSocket": [
        "NetworkLayerProtocol",
        "SocketError",
        "SocketState",
        "SocketType",
    ],
    "QFileDevice": ["FileError", "FileHandleFlag", "MemoryMapFlags", "Permission"],
    "QLocalSocket": ["LocalSocketError", "LocalSocketState"],
}

#
# These are mis labeled in Qt's header files. QDOC_PROPERTY is used in
# insteadof Q_PROPERTY. For these classes, translate the props as
# functions.
#
doProps = ["QInputDialog"]

doPropsIfFuncToo = ["primaryScreen", "availableGeometry"]

noHFileOutput = []  # ["QObject"]

#
#   run the script with -rawclass to get the fully qualified pattern
#   for a function in exclusionMap
#

exclusionMap = {
    "::qVariantCanConvert": None,
    "::bin": None,
    "::bom": None,
    "::center": None,
    "::dec": None,
    "::endl": None,
    "::fixed": None,
    "::flush": None,
    "::forcepoint": None,
    "::forcesign": None,
    "::hex": None,
    "::left": None,
    "::right": None,
    "::lowercasebase": None,
    "::lowercasedigits": None,
    "::uppercasebase": None,
    "::noforcepoint": None,
    "::noforcesign": None,
    "::oct": None,
    "::qSetFieldWidth": None,
    "::qSetPadChar": None,
    "::qSetRealNumberPrecision": None,
    "::reset": None,
    "::scientific": None,
    "::uppercasedigits": None,
    "::ws": None,
    "::showbase": None,
    "::noshowbase": None,
    "QSize::scaled": None,
    "QSize::transposed": None,
    "QPoint::dotProduct": None,
    "QPointF::dotProduct": None,
    "QPoint::hiResGlobalPos": None,
    "QPointf::hiResGlobalPos": None,
    "QTabletEvent::hiResGlobalPos": None,
    "QObject::disconnect": None,
    "QWidget::hasEditFocus": None,
    "QWidget::setEditFocus": None,
    "QWidget::macQDHandle": None,
    "QWidget::macCGHandle": None,
    "QWidget::x11Info": None,
    "QWidget::x11PictureHandle": None,
    "QWidget::getContentsMargins": None,
    "QWidget::setContentsMargins": None,
    "QWidget::setupUi": None,
    "QWidget::render": None,  # QPainter*
    "QWidget::redirected": None,
    "QWidget::WindowFlags": None,
    "QProcess::setNativeArguments": None,
    "QProcess::nativeArguments": None,
    "QProcess::createProcessArgumentsModifier": None,
    "QProcess::setCreateProcessArgumentsModifier": None,
    "QProcess::setChildProcessModifier": None,
    "QProcess::startDetached": None,  # handrolled
    "QMenuBar::defaultAction": None,
    "QMenuBar::setDefaultAction": None,
    "QPlainTextEdit::extraSelections": None,
    "QPlainTextEdit::setExtraSelections": None,
    "QPlainTextEdit::getPaintContext": None,  # something wrong w/enum convert
    "QTextBlock::textFormats": None,
    "QPainterPath::elementAt": None,
    "QPainter::QPainter": None,  # handrolled
    "QKeySequence::QVariant": None,
    "QPdfWriter::QPdfWriter": None,
    "QPixmap::handle": None,
    "QPixmap::x11PictureHandle": None,
    "QPixmap::HBitmapFormat": None,
    "QPixmap::ShareMode": None,
    "QPixmap::scroll": None,  # Uses QRegion* as default arg with val == 0
    "QPixmap::fromX11Pixmap": None,
    "QPixmap::NoAlpha": None,
    "QPixmap::PremultipliedAlpha": None,
    "QPixmap::Alpha": None,
    "QPixmap::ImplicitlyShared": None,
    "QPixmap::ExplicitlyShared": None,
    "QPixmap::fromImage": [
        (
            "fromImage",
            "",
            [
                ("image", "QImage & &", None),
                ("flags", "Qt::ImageConversionFlags", "Qt::AutoColor"),
            ],
            "QPixmap",
            False,
        )
    ],
    "QPixmap::alphaChannel": None,
    "QPixmap::setAlphaChannel": None,
    "QImage::QImage": [
        (
            "QImage",
            "",
            [("fileName", "const char &", None), ("format", "const char *", "nullptr")],
            "",
            False,
        )
    ],
    "QImage::smoothScaled": None,
    "QIcon::paint": None,
    "QColor::allowX11ColorNames": None,
    "QColor::setAllowX11ColorNames": None,
    # shadows the int version
    "QByteArray::number": [
        (
            "number",
            "",
            [("n", "uint", None), ("base", "int", "10")],
            "QByteArray",
            False,
        )
    ],
    # "QByteArray::toStdString" : [ ('toStdString', 'const', [], 'std::string', False) ],
    # "QByteArray::fromStdString" : [ ('fromStdString', '', [('str', 'const std::string &', None)], 'QByteArray', False) ],
    "QByteArray::toStdString": None,
    "QByteArray::fromStdString": None,
    # cannot convert from 'QByteArray::FromBase64Result' to 'int'
    "QByteArray::fromBase64Encoding": None,
    "QByteArray::toEcmaUint8Array": None,
    "QByteArray::fromEcmaUint8Array": None,
    "QRegion::setRects": None,
    # these use QString* to pass default args of 0: they need to be implemented by hand
    "QFileDialog::getOpenFileName": None,
    "QFileDialog::getOpenFileUrl": None,
    "QFileDialog::getOpenFileNames": None,
    "QFileDialog::getSaveFileName": None,
    "QFileDialog::getExistingDirectoryUrl": None,
    "QFileDialog::getOpenFileUrls": None,
    "QFileDialog::getSaveFileUrl": None,
    # Issue with std::function
    "QFileDialog::getOpenFileContent": None,
    # these are shadowing get prop funcs of the same names"
    "QCoreApplication::applicationName": None,
    "QCoreApplication::setApplicationName": None,
    "QCoreApplication::applicationVersion": None,
    "QCoreApplication::setApplicationVersion": None,
    "QCoreApplication::organizationName": None,
    "QCoreApplication::setOrganizationName": None,
    "QCoreApplication::organizationDomain": None,
    "QCoreApplication::setOrganizationDomain": None,
    "QCoreApplication::isQuitLockEnabled": None,
    "QCoreApplication::setQuitLockEnabled": None,
    # Issue with protected enum
    "QDial::sliderChange": None,
    # deprecated (its gone?)
    "QApplication::keypadNavigationEnabled": None,
    # shadows props
    "QApplication::navigationMode": None,
    "QApplication::cursorFlashTime": None,
    "QApplication::doubleClickInterval": None,
    "QApplication::globalStrut": None,
    "QApplication::keyboardInputInterval": None,
    "QApplication::quitOnLastWindowClosed": None,
    "QApplication::layoutDirection": None,
    "QApplication::startDragDistance": None,
    "QApplication::startDragTime": None,
    "QApplication::windowIcon": None,
    "QApplication::wheelScrollLines": None,
    "QApplication::setNavigationMode": None,
    "QApplication::setCursorFlashTime": None,
    "QApplication::setDoubleClickInterval": None,
    "QApplication::setGlobalStrut": None,
    "QApplication::setKeyboardInputInterval": None,
    "QApplication::setQuitOnLastWindowClosed": None,
    "QApplication::setLayoutDirection": None,
    "QApplication::setStartDragDistance": None,
    "QApplication::setWindowIcon": None,
    "QApplication::setWheelScrollLines": None,
    "QApplication::setStartDragTime": None,
    "QApplication::overrideCursor": None,
    "QGuiApplication::keypadNavigationEnabled": None,
    # shadows props
    "QGuiApplication::navigationMode": None,
    "QGuiApplication::cursorFlashTime": None,
    "QGuiApplication::doubleClickInterval": None,
    "QGuiApplication::globalStrut": None,
    "QGuiApplication::keyboardInputInterval": None,
    "QGuiApplication::quitOnLastWindowClosed": None,
    "QGuiApplication::layoutDirection": None,
    "QGuiApplication::startDragDistance": None,
    "QGuiApplication::startDragTime": None,
    "QGuiApplication::windowIcon": None,
    "QGuiApplication::wheelScrollLines": None,
    "QGuiApplication::setNavigationMode": None,
    "QGuiApplication::setCursorFlashTime": None,
    "QGuiApplication::setDoubleClickInterval": None,
    "QGuiApplication::setGlobalStrut": None,
    "QGuiApplication::setKeyboardInputInterval": None,
    "QGuiApplication::setQuitOnLastWindowClosed": None,
    "QGuiApplication::setLayoutDirection": None,
    "QGuiApplication::setStartDragDistance": None,
    "QGuiApplication::setWindowIcon": None,
    "QGuiApplication::setWheelScrollLines": None,
    "QGuiApplication::setStartDragTime": None,
    "QGuiApplication::overrideCursor": None,
    "QGuiApplication::applicationDisplayName": None,
    "QGuiApplication::setApplicationDisplayName": None,
    "QGuiApplication::platformName": None,
    # Same as above properties, issue generating a Mu::NodeFunc for it
    "QGuiApplication::desktopFileName": None,
    # Same as above properties, issue generating a Mu::NodeFunc for it
    "QGuiApplication::setDesktopFileName": None,
    # Don't seem to exist in the doc, but it gets generated
    "QGuiApplication::Cursor": None,
    "QAbstractSpinBox::stepEnabled": None,  # needs to be done by hand
    "QAbstractListModel::columnCount": None,  # private
    "QAbstractListModel::parent": None,  # private
    "QAbstractListModel::hasChildren": None,  # private
    "QAbstractTableModel::parent": None,  # private
    "QAbstractTableModel::hasChildren": None,  # private
    # Issue with CheckIndexOption type.
    "QAbstractItemModel::checkIndex": None,
    "QHeaderView::indexAt": None,  # protected -- FIX ME?
    "QHeaderView::scrollTo": None,  # protected -- FIX ME?
    "QHeaderView::visualRect": None,  # protected -- FIX ME?
    "QHeaderView::moveCursor": None,  # protected -- FIX ME?
    "QListView::moveCursor": None,  # protected enum
    "QSpinBox::fixup": None,  # protected
    # funky "return"
    "QItemSelection::split": None,
    "QVariant::canConvert": None,
    "QVariant::PointArray": None,
    "QVariant::canView": None,
    "QBrush::gradient": None,
    "QTextDocument::drawContents": None,  # QPainter*
    # these two are "shadowing" the constructor that takes an int
    "QVariant::QVariant": [
        ("QVariant", "", [("color", "Qt::GlobalColor", None)], "", False),
        ("QVariant", "", [("type", "QVariant::Type", None)], "", False),
    ],
    # Issue with std::variant and Types: "const std::variant<Types...>"
    "QVariant::fromStdVariant": None,
    "QFile::QFile": [
        ("QFile", "", [("name", "const std::filesystem::path &", None)], "", False),
        (
            "QFile",
            "",
            [
                ("name", "const std::filesystem::path &", None),
                ("parent", "QObject *", None),
            ],
            "",
            False,
        ),
    ],
    "QFile::filesystemFileName": None,
    "QFile::filesystemSymLinkTarget": None,
    "QFile::link": [
        (
            "link",
            "",
            [("newName", "const std::filesystem::path &", None)],
            "bool",
            False,
        )
    ],
    "QFile::rename": [
        (
            "rename",
            "",
            [("newName", "const std::filesystem::path &", None)],
            "bool",
            False,
        )
    ],
    "QFile::setFileName": [
        (
            "setFileName",
            "",
            [("name", "const std::filesystem::path &", None)],
            "void",
            False,
        )
    ],
    "QFile::permissions": [
        (
            "permissions",
            "",
            [("filename", "const std::filesystem::path &", None)],
            "QFileDevice::Permissions",
            False,
        )
    ],
    "QFile::copy": [
        (
            "copy",
            "",
            [("newName", "const std::filesystem::path &", None)],
            "bool",
            False,
        )
    ],
    "QFile::setPermissions": [
        (
            "setPermissions",
            "",
            [("permissions", "QFileDevice::Permissions", None)],
            "virtual bool",
            False,
        ),
        (
            "setPermissions",
            "",
            [
                ("fileName", "const QString &", None),
                ("permissions", "QFileDevice::Permissions", None),
            ],
            "bool",
            False,
        ),
        (
            "setPermissions",
            "",
            [
                ("filename", "const std::filesystem::path &", None),
                ("permissionSpec", "QFileDevice::Permissions", None),
            ],
            "bool",
            False,
        ),
    ],
    # Issue with conversion QString to QString *
    "QFile::moveToTrash": [
        (
            "moveToTrash",
            "",
            [
                ("fileName", "const QString &", None),
                ("pathInTrash", "QString *", "nullptr"),
            ],
            "bool",
            False,
        )
    ],
    # QFile has funky semantics which screw things up in QFileInfo
    # so these have to be done manually
    "QFileInfo::QFileInfo": [
        ("QFileInfo", "", [("file", "const QFileDevice &", None)], "", False),
        (
            "QFileInfo",
            "",
            [
                ("dir", "const QDir &", None),
                ("file", "const std::filesystem::path &", None),
            ],
            "",
            False,
        ),
        ("QFileInfo", "", [("file", "const std::filesystem::path &", None)], "", False),
    ],
    "QFileInfo::setFile": [
        (
            "setFile",
            "",
            [("file", "const std::filesystem::path &", None)],
            "void",
            False,
        ),
        ("setFile", "", [("file", "const QFileDevice &", None)], "void", False),
    ],
    "QFileInfo::operator!=": [
        (
            "operator!=",
            "const",
            [("fileinfo", "const QFileInfo &", None)],
            "bool",
            False,
        )
    ],
    "QFileInfo::operator==": [
        (
            "operator==",
            "const",
            [("fileinfo", "const QFileInfo &", None)],
            "bool",
            False,
        )
    ],
    "QFileOpenEvent::openFile": None,  # 4.8 added this: can't deal with QFile* <-> QFile& conversion yet
    # cannot convert from 'QFormLayout::TakeRowResult' to 'int'
    "QFormLayout::takeRow": None,
    "QStandardItem::operator<": None,
    "QListWidgetItem::operator<": None,
    "QTableWidgetItem::operator<": None,
    "QTreeWidgetItem::operator<": None,
    "QTreeWidget::setModel": None,  # private
    "QListWidget::setModel": None,  # private
    "QTableWidget::setModel": None,  # private
    "QLabel::picture": None,
    "QLabel::resourceProvider": None,
    "QLabel::setResourceProvider": None,
    # Issue with protected enum
    "QListWidget::moveCursor": None,
    # Issue with protected enum
    "QTableWidget::moveCursor": None,
    # Issue with protected enum
    "QColumnView::moveCursor": None,
    # Issue with protected enum
    "QTableView::moveCursor": None,
    # Issue with protected enum
    "QTreeView::moveCursor": None,
    # Issue with protected enum
    "QTreeWidget::moveCursor": None,
    "QFont::QFont": [
        (
            "QFont",
            "",
            [("font", "const QFont &", None), ("pd", "QPaintDevice *", None)],
            "",
            False,
        )
    ],
    "QFont::macFontID": None,
    # Trouble with QNetworkReply::RawHeaderPair (QPair<QByteArray, QByteArray>)
    "QNetworkReply::rawHeaderPairs": None,
    # "QWebFrame::render" : None, # QPainter*
    # "QWebPage::javaScriptPrompt" : None, # QString*
    "QWebEngineHistory::backItem": None,
    "QWebEngineHistory::forwardItem": None,
    "QWebEngineHistory::currentItem": None,
    "QWebEngineHistory::itemAt": None,
    # Issue with std::function
    "QWebEngineCookieStore::setCookieFilter": None,
    "QEvent::ChildInserted": None,
    "QEvent::EnterEditFocus": None,  # ????
    "QEvent::LeaveEditFocus": None,  # ????
    "QAbstractSpinBox::fixup": None,  # return val is arg
    "QTouchEvent::QTouchEvent": None,
    "QStyle::polish": [
        ("polish", "", [("palette", "QPalette &", None)], "virtual void", False)
    ],
    "QTextDocument::undo": None,  # Takes a TextCursor* needs manual imp
    "QTextDocument::redo": None,  # same
    "QSlider::sliderChange": None,  # protected
    # Issue with the "feature" argument.
    "QTextDocument::setMarkdown": None,
    # Issue with the "feature" argument.
    "QTextDocument::toMarkdown": None,
    "QTextDocument::setDefaultResourceProvider": None,
    "QTextDocument::resourceProvider": None,
    "QTextDocument::setResourceProvider": None,
    "QTextDocument::defaultResourceProvider": None,
    "QTextEdit::extraSelections": None,
    # Issue with const QList<QTextOption::Tab>  arg1 = (&)(param_tabStops);
    "QTextOption::setTabs": None,
    # Issue with the generated "QTextOption::TType". Does not exist.
    "QTextOption::tabs": None,
    # Syntax error with "const QList<QTextEdit::ExtraSelection>  arg1 = (&)(param_selections);"
    "QTextEdit::setExtraSelections": None,
    "QTextStream::QTextStream": None,  # handrolled
    "QTextStream::setString": None,
    "QTextStream::string": None,
    "QTextStream::readLineInto": None,
    "QTextStream::operator<<": None,
    "QTextStream::operator>>": None,
    # cannot convert from 'std::chrono::milliseconds' to 'int'
    "QTimer::intervalAsDuration": None,
    # cannot convert from 'std::chrono::milliseconds' to 'int'
    "QTimer::remainingTimeAsDuration": None,
    "QCursor::bitmap": None,
    "QCursor::mask": None,
    "QMargins::operator+=": None,
    "QMargins::operator-=": None,
    "QMargins::operator*=": None,
    "QMargins::operator/=": None,
    "QWebEnginePage::findText": None,
    "QWebEnginePage::javaScriptPrompt": None,
    "QWebEnginePage::runJavaScript": None,
    "QWebEnginePage::toHtml": None,
    "QWebEnginePage::toPlainText": None,
    "QWebEnginePage::history": None,
    "QWebEngineView::history": None,
    "QWebEngineProfile::setNotificationPresenter": None,
    "QWebEngineProfile::requestIconForIconURL": None,
    "QWebEngineProfile::requestIconForPageURL": None,
    "QWebEngineView::findText": None,
    "QMenu::setAsDockMenu": None,
    # Trouble with std::initializer_list
    # syntax error: std::initializer_list<QPair<QString, QString> > arg1 = (>)(param_list);
    "QUrlQuery::QUrlQuery": [
        (
            "QUrlQuery",
            "",
            [
                ("list", "std::initializer_list<QPair<QString, QString>>", None),
            ],
            "",
            False,
        )
    ],
    "QQuickItem::itemChange": [
        (
            "itemChange",
            "",
            [
                ("change", "QQuickItem::ItemChange", None),
                ("value", "const QQuickItem::ItemChangeData &", None),
            ],
            "virtual void",
            False,
        )
    ],
    # Syntax issue with 'const QVector<QQmlContext::PropertyPair>  arg1 = (&)(param_properties);'
    "QQmlContext::setContextProperties": None,
    "QRectF::toDOMRect": None,
    "QRectF::fromDOMRect": None,
    # Requires C++20 and new method in Qt 6.4.
    "QDateTime::toStdSysSeconds": None,
    # Requires C++20 and new method in Qt 6.4.
    "QDateTime::toStdSysMilliseconds": None,
    # Requires C++20 and new method in Qt 6.4.
    "QDateTime::fromStdTimePoint": None,
    # Requires C++20 and new method in Qt 6.4.
    "QDate::toStdSysDays": None,
    # Requires C++20 and new method in Qt 6.4.
    "QDate::addDuration": None,
    # Requires C++20 and new method in Qt 6.4.
    "QDate::QDate": [
        (
            "QDate",
            "",
            [
                ("ymd", "int", None),
            ],
            "",
            False,
        ),
        (
            "QDate",
            "",
            [
                ("ymd", "std::chrono::year_month_day_last ymd", None),
            ],
            "",
            False,
        ),
        (
            "QDate",
            "",
            [
                ("ymd", "std::chrono::year_month_weekday ymd", None),
            ],
            "",
            False,
        ),
        (
            "QDate",
            "",
            [
                ("ymd", "std::chrono::year_month_weekday_last ymd", None),
            ],
            "",
            False,
        ),
    ],
    "QDir::QDir": [
        (
            "QDir",
            "",
            [
                ("path", "const std::filesystem::path &", None),
                ("nameFilter", "const QString &", None),
                ("sort", "QDir::SortFlags", "SortFlags(Name | IgnoreCase)"),
                ("filters", "QDir::Filters", "AllEntries"),
            ],
            "",
            False,
        ),
        ("QDir", "", [("path", "const std::filesystem::path &", None)], "", False),
    ],
    "QDir::filesystemAbsolutePath": None,
    "QDir::filesystemCanonicalPath": None,
    "QDir::filesystemPath": None,
    "QDir::setPath": [
        (
            "setPath",
            "",
            [("path", "const std::filesystem::path &", None)],
            "void",
            False,
        )
    ],
    "QDir::addSearchPath": [
        (
            "addSearchPath",
            "",
            [
                ("prefix", "const QString &", None),
                ("path", "const std::filesystem::path &", None),
            ],
            "void",
            False,
        )
    ],
    "QFileInfo::filesystemAbsoluteFilePath": None,
    "QFileInfo::filesystemAbsolutePath": None,
    "QFileInfo::filesystemCanonicalFilePath": None,
    "QFileInfo::filesystemCanonicalPath": None,
    "QFileInfo::filesystemFilePath": None,
    "QFileInfo::filesystemJunctionTarget": None,
    "QFileInfo::filesystemPath": None,
    "QFileInfo::filesystemReadSymLink": None,
    "QFileInfo::filesystemSymLinkTarget": None,
    "QUrl::fromAce": None,
    "QUrl::toAce": None,
    "QTimeZone::nextTransition": None,
    "QTimeZone::offsetData": None,
    "QTimeZone::previousTransition": None,
    "QTimeZone::transitions": None,
    "QCalendar::dateFromParts": None,
    "QCalendar::partsFromDate": None,
    "QCalendar::daysInMonth": None,
}

customNativeFuncsHeader = {
    "QAbstractItemModel": "QModelIndex createIndex0_pub(int row, int column, Pointer p) const { return createIndex(row, column, p); }"
}

excludedDefaultValues = [
    # "QWebFrame::addToJavaScriptWindowObject",
    "QDir::entryInfoList",
    "QDir::entryList",
]


def isFunctionExcluded(qtnamespace, qtfunc):
    if qtnamespace is not None:
        cppname = qtnamespace.name + "::" + qtfunc[0]
    else:
        cppname = qtfunc[0]
    if cppname in exclusionMap:
        e = exclusionMap[cppname]
        if e is None:
            return True
        inname = str(qtfunc)
        for f in e:
            if inname == str(f):
                return True
        return False


def doesFunctionAllowDefaultValues(qtnamespace, funcname):
    if qtnamespace is not None:
        cppname = qtnamespace.name + "::" + funcname
    else:
        cppname = qtfunc[0]
    if cppname in excludedDefaultValues:
        return False
    else:
        return True


#
#   For name mangling. This is taken directly from the Context.cpp
#   file in Mu. The names generated by this map/func are used by the
#   muc front end when compiling to C++
#

mangleMap = {
    " ": "_",
    ".": "__",
    "~": "Tilde_",
    "`": "Tick_",
    "+": "Plus_",
    "-": "Minus_",
    "!": "Bang_",
    "%": "PCent_",
    "@": "At_",
    "#": "Pound_",
    "$": "Dollar_",
    "^": "Caret_",
    "&": "Amp_",
    "*": "Star_",
    "(": "BParen_",
    ")": "EParen_",
    "=": "EQ_",
    "{": "BCB_",
    "}": "ECB_",
    "[": "BSB_",
    "]": "ESB_",
    "|": "Pipe_",
    "\\": "BSlash_",
    ":": "Colon_",
    ";": "SColon_",
    '"': "Quote_",
    "'": "SQuote_",
    "<": "LT_",
    ">": "GT_",
    ",": "Comma_",
    "?": "QMark_",
    "/": "Slash_",
}

qListRE = re.compile("(const)? *QList<([a-zA-Z][a-zA-Z0-9_]+)[ *]*>")
makeRE = re.compile("<([A-Za-z]+,)?(Q[A-Za-z]+Type)")
intRE = re.compile("-?[0-9]+")


def mangleName(name):
    out = ""
    for c in name:
        k = str(c)
        if mangleMap.has_key(k):
            out += mangleMap[k]
        else:
            out += k
    return out


def convertFrom(expr, ktype, atype):
    if convertFromMap.has_key(ktype):
        (s, o) = convertFromMap[ktype]
        return (
            string.replace(string.replace(s, "$E", expr), "$T", atype),
            string.replace(o, "$T", atype),
        )
    elif ktype.startswith("flags "):
        ntype = atype.split()[-1]
        return ("(%s)(%s)" % (ntype, expr), ntype)
    elif ktype.split(".")[-1] in primitiveTypes:
        ptype = ktype.split(".")[-1]
        ntype = atype.split()[-1]
        return ("getqtype<%sType>(%s)" % (ptype, expr), ntype)
    elif ktype.split(".")[-1] in pointerTypes:
        ptype = ktype.split(".")[-1]
        ntype = atype
        return ("getqpointer<%sType>(%s)" % (ptype, expr), ntype)
    elif "[]" in ktype:
        # QList
        ptype = ktype.split(".")[-1][0:-2]
        ntype = atype
        if ptype in primitiveTypes:
            return ("qtypelist<%s,%sType>(%s)" % (ptype, ptype, expr), ntype)
        else:
            return ("qpointerlist<%sType>(%s)" % (ptype, expr), ntype)
    else:
        return ("(%s)(%s)" % (atype, expr), atype)


def convertTo(expr, ktype):
    ktype = sstrip(string.replace(ktype, "virtual", ""))
    constRef = isConstReference(ktype)
    pointer = isPointerToSomething(ktype)
    if constRef:
        ktype = constReferenceType(ktype)
    elif pointer:
        ktype = pointedToType(ktype)
    if convertToMap.has_key(ktype):
        (s, o) = convertToMap[ktype]
        return string.replace(string.replace(s, "$E", expr), "$T", ktype)
    elif ktype in primitiveTypes:
        return 'makeqtype<%sType>(c,%s,"qt.%s")' % (ktype, expr, ktype)
    elif ktype in pointerTypes and not constRef:
        return 'makeqpointer<%sType>(c,%s,"qt.%s")' % (ktype, expr, ktype)
    elif muapi.classes.has_key(ktype):
        muclass = muapi.classes[ktype]
        if muclass.isAByName("QObject"):
            return 'makeinstance<%sType>(c,%s,"qt.%s")' % (ktype, expr, ktype)
        elif muclass.isAByName("QLayoutItem"):
            return 'makelayoutitem<%sType>(c,%s,"qt.%s")' % (ktype, expr, ktype)
        elif muclass.isAByName("QPaintDevice"):
            return 'makepaintdevice<%sType>(c,%s,"qt.%s")' % (ktype, expr, ktype)
        elif muclass.isAByName("QIODeviceBase"):
            return 'makeiodevicebase<%sType>(c,%s,"qt.%s")' % (ktype, expr, ktype)
        else:
            print("ERROR: can't convertTo", expr, ktype)
            return "NOT_CONVERTED(%s,%s)" % (expr, ktype)
    elif "QList<" in ktype:
        # QList
        mtype = ktype[6:-3]
        m = qListRE.match(ktype)
        if m:
            mtype = m.group(2)
        if mtype in primitiveTypes:
            return 'makeqtypelist<%s, %sType>(c,%s,"qt.%s")' % (
                mtype,
                mtype,
                expr,
                mtype,
            )
        else:
            return 'makeqpointerlist<%sType>(c,%s,"qt.%s")' % (mtype, expr, mtype)
    elif "::" in ktype:
        return "int(%s)" % expr
    else:
        # print "convertTo fall through", str(expr), str(ktype)
        return expr


def setExpr(targetexpr, expr, ktype):
    ktype = sstrip(string.replace(ktype, "virtual", ""))
    t = ""
    if setMap.has_key(ktype):
        t = setMap[ktype]
    elif ktype in primitiveTypes:
        return "setqtype<%sType>(%s,%s)" % (ktype, targetexpr, expr)
    elif ktype in pointerTypes:
        return "setqpointer<%sType>(%s,%s)" % (ktype, targetexpr, expr)
    else:
        t = "$O = $E"
    return string.replace(string.replace(t, "$E", expr), "$O", targetexpr)


def conditionType(t):
    if t.startswith("flags "):
        return "int"
    else:
        return t


#
#   Qt converter map
#


#
#   During parsing this will get populated with all of the types that have
#   been discovered
#


class API:
    def __init__(self):
        self.types = set()
        self.classes = {}
        self.tmap = translationMap
        self.excludeProps = []

    def finish(self):
        # adjust all the inheritedby and inherits fields in qt 5 the
        # docs no longer show cross module inheritance so we have to
        # hook it up after the fact
        for k in self.classes.keys():
            c = self.classes[k]
            for parent in c.inherits:
                if parent in self.classes:
                    other = self.classes[parent]
                    if k not in other.inheritedby:
                        other.inheritedby.append(k)
        for k in self.classes.keys():
            # for the time being only do classes we've got working so far
            if k in includeClasses:
                self.tmap[k] = k
                c = self.classes[k]
            self.tmap[k] = k

    def showClass(self, name):
        c = self.classes[name]
        c.output()

    def translate(self, cpptype, inclass):
        F = lambda x: x not in typeElaborations
        parts = cpptype.split(" ")

        if ("int" in parts or "qreal" in parts) and ("*" in parts):
            return None

        # rvalue not supported
        if cpptype.find("&&") != -1 or cpptype.find("& &") != -1:
            return None

        # iterators not supported
        if cpptype.find("iterator") != -1:
            return None

        key = " ".join(filter(F, parts))
        # e.g. "const QString &" -> "QString"
        if self.tmap.has_key(key) and (key in includeClasses):
            return self.tmap[key]
        # e.g. "const char *"
        if self.tmap.has_key(cpptype):
            return self.tmap[cpptype]
        # an enum at the top level?
        # if cpptype.startswith("Qt::"):
        #    return "flags %s" % cpptype
        if cpptype.find("::") != -1:
            return "flags %s" % cpptype

        if inclass.enumInHierarchy(cpptype):
            return "flags %s" % inclass.fullEnumName(cpptype)
        # an enum in the class not qualified
        # test whether the type name needs to be fully qualified
        # e.g. "InsertPolicy" -> "QComboBox::InsertPolicy"
        # if inclass is not None:
        # for e in inclass.enums:
        #    if e.name == cpptype or e.flags == cpptype:
        #        return "flags %s::%s" % (inclass.name, cpptype)
        #    return self.translate(inclass.name + "::" + cpptype, None)
        return None


api = API()
verbose = True


def message(s):
    global verbose
    if verbose:
        print(s)


def sstrip(s):
    return s.lstrip().rstrip()


def addType(t):
    api.types.add(t)


def parseProperty(prop):
    prop = prop[0:-1]
    parts = prop.split(" : ")
    if len(parts) == 2:
        name = sstrip(parts[0])
        type = sstrip(parts[1])
        addType(type)
        return (name, type)
    else:
        return None


def parseType(t):
    global api
    t = sstrip(reduce(lambda x, y: str(x) + " " + str(y), t, ""))
    addType(t)
    return t


def indexInList(el, list):
    i = 0
    for list in list:
        if list == el:
            return i
        i += 1
    return i


def parseParameter(param, n):
    param = sstrip(param)  # .replace("*", "* ").replace("&", "& ")
    if param == "const" or param == "":
        return param
    parts = param.split()
    default = None
    name = "_p%d" % n
    if parts == []:
        return ("", param, None)
    if "=" in parts:
        index = indexInList("=", parts)
        default = sstrip(
            reduce(lambda x, y: str(x) + " " + str(y), parts[index + 1 :], "")
        )
        name = parts[index - 1]
        del parts[index - 1 :]
    elif "*" in parts or "&" in parts:
        if parts[-1] == "*" or parts[-1] == "&":
            # name = None
            pass
        else:
            name = parts[-1]
            del parts[-1]
    elif len(parts) == 1:
        return (name, parts[0], default)
    else:
        name = parts[-1]
        del parts[-1]
    if parts == [] and name is None and default is None:
        return None
    else:
        type = parseType(parts[:])
        return (name, type, default)


def parse_cpp_function(function_signature):
    # Updated regular expression to handle operator overloading
    pattern = r"^((?:template\s+)?(?:virtual\s+)?(?:explicit\s+)?(?:[\w:]+(?:<.*?>)?(?:\s*[\*&])?\s+)+)?\s*(\~?\w+(?:::\w+)*|\w+\s*(?:<.*?>)?::(?:~?\w+|operator\s*\S+)|operator\s*\S+)\s*(\(.*?\))(?:\s*((?:const)?\s*(?:noexcept)?\s*(?:override)?\s*(?:final)?\s*(?:&)?\s*(?:->.*?)?(?:=\s*\w+)?))?$"

    match = re.match(pattern, function_signature.strip())

    if not match:
        return None, None, None, None

    return_type, function_name, parameters, after_parameters = match.groups()

    # Clean up the matches
    if return_type:
        return_type = return_type.strip()
    else:
        return_type = ""

    function_name = function_name.strip()

    # Fix this as this remove () for thing like Qt::WindowsFlag()
    parameters = parameters[1:-1]

    if after_parameters:
        after_parameters = after_parameters.strip()
    else:
        after_parameters = ""

    return return_type, function_name, parameters, after_parameters


def parseFunction(func, qtnamespace):
    orig_func = func
    # parts = func.split("(")
    # in 5.6 they added some crap and * and & are no longer separated by spaces
    # 5.9 has override in the const portion
    if "(deprecated)" in func:
        message("WARNING: " + func)
    if "(shadows)" in func:
        message("WARNING: " + func)
    # Qt6: Do not parse obsolete function.
    if "(obsolete)" in func:
        message("WARNING: Obsolete: " + func)
        return None
    func = func.replace("(deprecated) ", "")
    func = func.replace("(shadows) ", "")
    func = func.replace("*", "* ")
    func = func.replace("&", "& ")
    func = func.replace("override", "")
    prop = False
    demoted = False
    if len(func) == 0:
        return None
    message("Processing function " + func)
    if func[-1] == "@":
        # demote virtual props to not props otherwise we can't create
        # the _func array because they won't exist in time
        demoted = "virtual" in func
        prop = not demoted
        func = func[0:-1]

    # try:
    #     groups = parse_cpp_function(func)
    #     parts = [groups[0] + " " + groups[1], groups[2]]
    #     thistype = groups[3]
    #     message("CEDRIK Parts: " + str(parts))
    #     message("CEDRIK thistype: " + groups[3])
    # except:
    #     # Fall back to previous method
    #     sindex = func.find("(")
    #     eindex = func.rfind(")")
    #     thistype = sstrip(func[eindex + 1 :])
    #     func = func[0:eindex]
    #     if sindex == -1:
    #         return None
    #     parts = [func[0:sindex], func[sindex + 1 :]]
    #     message("Parts: " + str(parts))
    #     message("thistype = " + thistype)

    try:
        sindex = func.find("(")
        eindex = func.rfind(")")
        thistype = sstrip(func[eindex + 1 :])
        func = func[0:eindex]
        if sindex == -1:
            return None
        parts = [func[0:sindex], func[sindex + 1 :]]
        message("Parts: " + str(parts))
        message("thistype = " + thistype)

        if len(parts) == 2:
            nameproto = parts[0].split()
            allparams = parts[1]

            bracketlevel = 0
            current = ""
            params = []
            count = 0

            for c in allparams:
                if c == "," or c == ")":
                    if bracketlevel == 0:
                        params.append(parseParameter(current, count))
                        current = ""
                    else:
                        if c == ")":
                            bracketlevel -= 1
                        current += c
                elif c == "<" or c == "(":
                    bracketlevel += 1
                    current += c
                elif c == ">" or c == ")":
                    bracketlevel -= 1
                    current += c
                else:
                    current += c

                count = count + 1

            if len(allparams) != 0:
                params.append(parseParameter(current, count))

            message("Params: " + str(params))

            # make new name for params with same name as func
            for i in range(0, len(params)):
                p = params[i]
                # Compare the first element of p and the last element of nameproto.
                message("Before comparison: " + str(p) + " with " + str(nameproto))
                # if nameproto:
                # message("Comparing: " + p[0] + " with " + nameproto[-1])
                if p[0] == nameproto[-1]:
                    # Append underscore
                    params[i] = (p[0] + "_", p[1], p[2])

            if nameproto:
                v = (
                    nameproto[-1],
                    thistype,
                    params,
                    string.join(nameproto[0:-1]),
                    prop,
                )
                if demoted:
                    qtnamespace.demotedProps.append(v)
                    # message("PROP demoted " + str(func))
                return v
            else:
                message("WARNING: Nameproto empty for " + func)
    except Exception:
        # Skip any function that has issue with the simple parsing above.
        print("Error parsing function.. skipping: {0}".format(orig_func))
        return None
    return None


##
##  NamespaceInfo holds the final parsed info about a Qt Class The
##  other results are Enumeration and Enum which are the name of the
##  enumeration and each individual name*value pair.
##


class NamespaceInfo:
    def __init__(self, name=None):
        self.name = name
        self.isclass = False
        self.slots = []
        self.signals = []
        self.functions = []
        self.publicfuncs = []
        self.protectedfuncs = []
        self.enums = []
        self.parents = []
        self.properties = []
        self.globalfuncs = []
        self.staticfuncs = []
        self.isclass = False
        self.includes = []
        self.inheritedby = []
        self.inherits = []
        self.module = ""
        self.demotedProps = []

    def output(self):
        if self.isclass:
            print("class", self.name)
        else:
            print("namespace", self.name)

        print("--module--")
        print(self.module)

        print("--inherits--")
        print(self.inherits)

        print("--inheritedby--")
        print(self.inheritedby)

        if self.includes:
            print("--includes--")
            for i in self.includes:
                print(i)

        if self.properties:
            print("--props--")
            for i in self.properties:
                if i is not None:
                    print(i)
        if self.enums:
            print("--enums--")
            for i in self.enums:
                print(i.name, i.flags)
                for q in i.enums:
                    print("   ", q.name, "=", q.value)

        if self.publicfuncs:
            print("--public member functions--")
            for i in self.publicfuncs:
                if i is not None:
                    print(i)

        if self.protectedfuncs:
            print("--protected member functions--")
            for i in self.protectedfuncs:
                if i is not None:
                    print(i)

        if self.signals:
            print("--signals functions--")
            for i in self.signals:
                if i is not None:
                    print(i)

        if self.slots:
            print("--slot functions--")
            for i in self.slots:
                if i is not None:
                    print(i)

        if self.staticfuncs:
            print("--static functions--")
            for i in self.staticfuncs:
                if i is not None:
                    print(i)

        if self.globalfuncs:
            print("--global functions--")
            for i in self.globalfuncs:
                if i is not None:
                    print(i)

    def enumInHierarchy(self, name):
        global api
        for i in self.enums:
            fullname = "%s::%s" % (self.name, name)
            if (
                i.name == name
                or i.name == fullname
                or i.flags == name
                or i.flags == fullname
            ):
                return True
            for p in self.inherits:
                if api.classes.has_key(p):
                    parent = api.classes[p]
                    if parent.enumInHierarchy(name):
                        return True
        return False

    def fullEnumName(self, name):
        global api
        for i in self.enums:
            fullname = "%s::%s" % (self.name, name)
            if (
                i.name == name
                or i.name == fullname
                or i.flags == name
                or i.flags == fullname
            ):
                return i.name
            for p in self.inherits:
                if p not in api.classes.keys():
                    print(
                        "WARNING:",
                        p,
                        "not in classes - can't make fullEnumName in",
                        self.name,
                    )
                else:
                    parent = api.classes[p]
                    x = parent.fullEnumName(name)
                    if x:
                        return x
        return None

    def finish(self):
        for q in range(0, len(self.functions)):
            f = self.functions[q]
            (name, fconst, params, rtype, prop) = f
            ctype = self.name + "::" + rtype
            fixed = False
            for e in self.enums:
                if e.name == ctype or e.flags == ctype:
                    self.functions[q] = (name, fconst, params, ctype, prop)
                    if f in self.protectedfuncs:
                        # patch protectedfuncs so we can do f in protectedfuncs
                        n = indexOf(f, self.protectedfuncs)
                        self.protectedfuncs[n] = self.functions[q]
                    fixed = True

            (name, fconst, params, rtype, prop) = self.functions[q]

            for i in range(0, len(params)):
                p = params[i]
                (pname, ptype, pval) = p
                ctype = self.name + "::" + ptype
                for e in self.enums:
                    if e.name == ctype or e.flags == ctype:
                        # print "fixed:", params[i][1], "to", ctype
                        params[i] = (pname, ctype, pval)


rootnamespace = NamespaceInfo("")


class Enum:
    def __init__(self, name, value):
        self.name = name
        self.value = value


class Enumeration:
    def __init__(self, name):
        self.name = name
        self.enums = []
        self.flags = None
        self.protected = False


#
#   These are Mu versions of above. By handing MuClass a Namespace
#   from above it will generate MuFunction and MuEnum objects which
#   are then used to produce output
#


class MuEnum:
    def __init__(self, enumeration, muclass):
        self.muclass = muclass
        self.name = enumeration.name.split(":")[-1]
        self.flags = None
        self.output = False
        self.protected = enumeration.protected
        self.value = None

        if forceEnumOutput.has_key(muclass.name):
            enums = forceEnumOutput[muclass.name]
            for e in enums:
                if e == self.name:
                    # print "INFO: will output %s.%s\n" % (muclass.name, e)
                    self.output = True

        if self.protected:
            self.output = True

        xmap = exclusionMap

        if enumeration.flags:
            self.flags = enumeration.flags.split(":")[-1]
        self.symbols = []
        for e in enumeration.enums:
            if e.name != "\n":
                # TODO: Might not be the right place to do it, but it works.
                # Remove anything parenthesis and spacing in the name.
                pattern = r"\([^)]*\)"
                e.name = re.sub(pattern, "", e.name).strip()
                name_without_class_name = e.name.split(muclass.name, 1)[1]
                # Could use e.name for val, but let's keep the logic similar.
                val = muclass.name + name_without_class_name
                # Name the symbolic constant with the enum name (no class or enum class).
                n = name_without_class_name.split(":")[-1]
                if self.protected:
                    # for protected enums use actual value instead of symbolic
                    val = e.value
                if not xmap or (val not in xmap):
                    self.symbols.append((n, val))

    def aliasDeclation(self):
        s = 'new Alias(c, "%s", "int")' % self.name
        if self.protected:
            s += " /* PROTECTED ENUM */"
        return s

    def flagsAliasDeclaration(self):
        if self.flags:
            return 'new Alias(c, "%s", "int")' % self.flags
        else:
            return None

    def symbolDeclaration(self, symbolTuple):
        (name, val) = symbolTuple
        out = 'new SymbolicConstant(c, "%s", "int", Value(int(' % name
        out += "%s)))" % val
        return out


class MuFunction:
    def __init__(self, api, muclass, qtfunc, isMember, isProtected, isCastOp):
        #
        #   Iterate over the functions
        #

        c = muclass.qt
        (name, const, args, rtype, isprop) = qtfunc
        if c.name in doProps:
            isprop = False

        # Manual list that says that the function is a properties as well.
        if qtfunc[0] in doPropsIfFuncToo:
            isprop = False

        count = 0
        if muclass.funcCount.has_key(name):
            count = muclass.funcCount[name]
            count = count + 1
        muclass.funcCount[name] = count

        self.muclass = muclass
        self.abstract = isAbstract(name)
        self.name = name
        self.qtfunc = qtfunc
        self.iscastop = isCastOp
        self.isconstructor = self.name == muclass.name or isCastOp
        self.isprotected = isProtected
        self.isprop = isprop
        self.args = []
        self.rtype = None
        self.failed = False
        self.node = mangleName("_n_%s%d" % (name, count))
        self.virtual = rtype.find("virtual") != -1
        self.virtualSlot = -1
        self.operator = self.name.find("operator") != -1
        self.ismember = isMember
        self.isconst = "const" in const
        self.purevirtual = "= 0" in qtfunc[1]

        print("Processing function {0}".format(qtfunc))
        print(
            "failed: {0}, isprop: {1}, isprotected: {2}".format(
                str(self.failed), str(self.isprop), str(self.isprotected)
            )
        )

        if self.operator:
            self.name = name[8 : len(name)]

        if self.iscastop:
            self.node = "_co" + self.node

        # no overloading of op= yet
        if self.name == "=":
            self.failed = True
            # message("%s failed because name is operator=" % name)

        # if "operator" in self.name:
        #    self.failed = True
        # message("%s failed because of operator in name" % name)
        for omit in ["virtual", "Q_INVOKABLE"]:
            rtype = sstrip(string.replace(rtype, omit, ""))

        if isMember:
            if self.iscastop:
                self.args.append(("this", self.name, None))
            else:
                self.args.append(("this", muclass.name, None))

        for aname, atype, aval in args:
            message("Trying to translate (%s, %s) for %s" % (atype, c, name))
            mutype = api.translate(atype, c)
            if mutype is None:
                self.failed = True
                message(
                    "%s failed because api.translate(%s,%s) return None"
                    % (name, atype, c)
                )
                mutype = '"%s"' % atype
            self.args.append((aname, mutype, aval))

        self.iscopyconstructor = False

        if self.isconstructor:
            self.rtype = name
            if len(self.args) == 2:
                if self.args[1][1] == self.name:
                    if "*" not in qtfunc[2][0][1]:  # not a pointer
                        self.iscopyconstructor = True
        else:
            self.rtype = api.translate(rtype, c)

        if self.iscopyconstructor:
            self.failed = True
            message("%s failed because its a copy constructor" % name)

        if self.rtype is None:
            self.failed = True
            self.rtype = '"%s"' % rtype
            message("%s failed because rtype == %s" % (name, rtype))

        self.compiled = mangleName(
            "qt_%s_%s_%s" % (muclass.name, name, conditionType(self.rtype))
        )

        #
        #   Test for functions that were already successfully translated
        #   that have the same function signature: if one is found we can't
        #   translate this one because it will shadow the previous
        #   one.
        #
        overloads = []
        if count > 0:
            for f in muclass.functions:
                if f.name == self.name and not f.failed:
                    overloads.append(f)
            nargs = len(self.args)
            for f in overloads:
                fargs = len(f.args)
                if fargs == nargs and not f.failed:
                    samecount = 0
                    for i in range(0, nargs):
                        a = f.args[i][1]
                        b = self.args[i][1]
                        if a.startswith("flags"):
                            a = "int"
                        if b.startswith("flags"):
                            b = "int"
                        if a == b:
                            samecount = samecount + 1
                    if samecount == nargs:
                        if name == "QVariant":
                            message("FAILED: (shadows) in %s" % str(qtfunc))
                            message("OTHER = %s" % str(f.qtfunc))
                        message(
                            "WARNING: %s.%s with %s shadows existing version"
                            % (muclass.name, self.name, str(self.args))
                        )
                        self.failed = True

        for a in self.args:
            self.compiled += "_%s" % mangleName(conditionType(a[1]))

        print("self.compiled={0}, {1}".format(self.compiled, self.failed))

    def unpackReturnValue(self, expr):
        rtype = conditionType(self.rtype)
        rep = repMapFind(rtype)
        if rep is not None:
            (repType, instType) = rep
            return expr + "._" + repType
        else:
            return expr + "._Pointer /* guess */"

    def symbolDeclaration(self):
        # note: assumes the context is in a var called "c"
        rtype = conditionType(self.rtype)
        if self.muclass.muapi.classes.has_key(rtype):
            rtype = "qt." + rtype
        ftype = "Function"
        if self.virtual:
            ftype = "MemberFunction"
        flags = "None"
        if self.operator:
            flags = "Op"
        if self.iscastop:
            flags = "Cast"
        out = 'new %s(c, "%s", %s, %s, Compiled, %s, Return, "%s", ' % (
            ftype,
            self.name,
            self.node,
            flags,
            self.compiled,
            rtype,
        )
        if self.args:
            out += "Parameters, "
            for aname, atype, aval in self.args:
                atype = conditionType(atype)
                if self.muclass.muapi.classes.has_key(atype):
                    atype = "qt." + atype
                out += 'new Param(c, "%s", "%s"' % (aname, atype)
                # output default values: note that its only easy to do
                # this for some value types. Generally Qt rarely used
                # default parameter values before Qt 5
                # right now only ints, flags, and enums are supported
                if aval is not None and doesFunctionAllowDefaultValues(
                    self.muclass, self.name
                ):
                    if atype == "int":
                        out += ", Value((int)%s)" % self.muclass.qualifyValue(aval)
                out += "), "
        out += "End)"
        return out

    def nodeImplementation(self):
        print("nodeImplementation -> {0}".format(self.name))
        if self.failed:
            return "// MISSING NODE: %s" % self.muDeclaration()
        if self.abstract and self.isconstructor and not self.muclass.inheritable:
            return "// NO NODE: CLASS IS ABSTRACT: %s" % self.muDeclaration()
        if self.isprotected and self.isconstructor:
            return "// NO NODE: CONSTRUCTOR IS PROTECTED: %s" % self.muDeclaration()
        rep = repMapFind(self.rtype)
        if rep is None:
            return None
        (repType, instType) = rep
        out = "static NODE_IMPLEMENTATION(%s, %s)\n{\n    " % (self.node, repType)
        endParen = False
        if repType != "void":
            out += "NODE_RETURN("
            endParen = True
        out += "%s(NODE_THREAD" % self.compiled
        if self.args:
            count = 0
            for aname, atype, aval in self.args:
                rep = repMapFind(atype)
                (repType, instType) = rep
                if count == 0 and self.ismember:
                    out += ", NONNIL_NODE_ARG(%d, %s)" % (count, repType)
                else:
                    out += ", NODE_ARG(%d, %s)" % (count, repType)
                count = count + 1
        if endParen:
            out += "));\n"
        else:
            out += ");\n"
        out += "}\n"

        return out

    def derefExp(self, expr, atype):
        muclass = self.muclass
        muapi = muclass.muapi
        # protected enum
        if "::" in atype:
            atype = muclass.enumTypeRep(atype)
        ktype = atype
        if muapi.classes.has_key(atype):
            c = muapi.classes[atype]
            if c.isAByName("QObject"):
                return convertFrom(expr, "qt.QObject", atype)
            elif c.pointertype:
                return convertFrom(expr, "qt." + atype, atype + " *")
            else:
                return convertFrom(expr, "qt." + atype, atype)
        return convertFrom(expr, atype, atype)

    def derefArg(self, arg):
        (aname, atype, aval) = arg
        return self.derefExp("param_" + aname, atype)

    def compiledFunction(self):
        #
        #   This is the function that does the real business
        #
        muclass = self.muclass
        muapi = muclass.muapi

        if self.iscastop:
            if muapi.classes.has_key(self.rtype):
                muclass = muapi.classes[self.rtype]
            else:
                print("FAILED to find", self.rtype)

        print("Compiled function for {0}".format(muclass.name))

        isQObject = muclass.isAByName("QObject")
        isQPaintDevice = muclass.isAByName("QPaintDevice")
        isQLayoutItem = muclass.isAByName("QLayoutItem")
        isQIODeviceBase = muclass.isAByName("QIODeviceBase")
        inheritable = muclass.inheritable

        setter = None
        getter = None
        maker = None

        callname = self.name
        if self.operator:
            callname = "operator%s" % self.name

        if isQObject:
            setter = "setobject"
            getter = "object"
            maker = "makeinstance"
            isafunc = "isMuQtObject"
        elif isQLayoutItem:
            setter = "setlayoutitem"
            getter = "layoutitem"
            maker = "makelayoutitem"
            isafunc = "isMuQtLayoutItem"
        elif isQPaintDevice:
            setter = "setpaintdevice"
            getter = "paintdevice"
            maker = "makepaintdevice"
            isafunc = "isMuQtPaintDevice"
        elif isQIODeviceBase:
            setter = "setiodevicebase"
            getter = "iodevicebase"
            maker = "makeiodevicebase"
            isafunc = "isMuQtIODeviceBase"

        if self.failed:
            return "// MISSING FUNC: %s" % self.muDeclaration()
        if self.abstract and self.isconstructor and not inheritable:
            return "// NO FUNC: CLASS IS ABSTRACT: %s" % self.muDeclaration()
        if self.isprotected and self.isconstructor:
            return "// NO FUNC: CONSTRUCTOR IS PROTECTED: %s" % self.muDeclaration()
        rep = repMapFind(self.rtype)
        out = "%s %s(Mu::Thread& NODE_THREAD" % (rep[0], self.compiled)
        for aname, atype, aval in self.args:
            rep = repMapFind(atype)
            out += ", %s param_%s" % (rep[0], aname)
        out += ")\n{\n"
        body = ""

        qtrtype = self.qtfunc[3]

        expr = ""
        altexpr = None

        if self.isconstructor:
            if muclass.primitivetype:
                expr = "%s(" % self.name
            else:
                if inheritable:
                    expr = (
                        "new MuQt_%s(param_this, NODE_THREAD.process()->callEnv()"
                        % self.name
                    )
                    if len(self.args) > 1:
                        expr += ", "
                else:
                    expr = "new %s(" % self.name
        else:
            if self.ismember:
                pub = ""
                parentpub = ""
                if muclass.primitivetype:
                    if self.isprotected:
                        expr = "((MuQt_%s&)arg0).%s_pub(" % (muclass.name, self.name)
                    else:
                        expr = "arg0.%s(" % callname
                else:
                    if inheritable and self.virtual and not self.purevirtual:
                        # not self.isprotected \
                        # only if its public
                        if self.isprotected:
                            expr = "((MuQt_%s*)arg0)->%s_pub_parent(" % (
                                muclass.name,
                                self.name,
                            )
                            altexpr = "((MuQt_%s*)arg0)->%s_pub(" % (
                                muclass.name,
                                self.name,
                            )
                        else:
                            expr = "arg0->%s::%s(" % (muclass.name, callname)
                            altexpr = "arg0->%s(" % self.name
                    else:
                        if self.isprotected:
                            expr = "((MuQt_%s*)arg0)->%s_pub(" % (
                                muclass.name,
                                self.name,
                            )
                        else:
                            expr = "arg0->%s(" % callname
            else:
                # if self.iscastop:
                #    expr = "%s(" % self.name
                # else:
                expr = "%s::%s(" % (muclass.qt.name, callname)

        for i in range(0, len(self.args)):
            a = self.args[i]
            (aname, atype, aval) = a
            (dexpr, dtype) = self.derefArg(a)

            ntype = ""

            if self.ismember:
                if i > 0:
                    qarg = self.qtfunc[2][i - 1]
                    if dtype.find("::") != -1 and dtype.find(qarg[1]) != -1:
                        ntype = dtype
                    else:
                        ntype = qarg[1]
                else:
                    ntype = dtype
            else:
                qarg = self.qtfunc[2][i]
                # if dtype.find("::") != -1 and dtype.find(qarg[1]) != -1:
                if dtype.find("::") != -1:
                    ntype = dtype
                else:
                    ntype = qarg[1]

            # check for protected enum
            if "::" in ntype:
                ntype = muclass.enumTypeRep(ntype)

            if i == 0 and self.ismember and muclass.iscopyonwrite:
                if ntype[-1] != "&":
                    ntype += "&"
                if self.isconst and "const" not in ntype:
                    ntype = "const " + ntype
            elif ntype[-1] == "&":
                ntype = ntype[0:-1]

            if i > 0 or not self.isconstructor:
                lbody = "    %s arg%d = " % (ntype, i)
                lbody += dexpr
                lbody += ";\n"
                if i == 0 and self.isprotected and inheritable:
                    etype = sansElaborations(ntype)
                    # body += string.replace(lbody, etype, "MuQt_" + etype)
                    body += lbody
                else:
                    body += lbody

            if self.ismember:
                if i > 1:
                    expr += ", "
                    if altexpr:
                        altexpr += ", "
            else:
                if i > 0:
                    expr += ", "
                    if altexpr:
                        altexpr += ", "

            if i > 0 or not self.ismember:
                expr += "arg%d" % i
                if altexpr:
                    altexpr += "arg%d" % i

        expr += ")"
        if altexpr:
            altexpr += ")"

        rtypeObject = muapi.isAByName(self.rtype, "QObject")
        rtypeLayoutItem = muapi.isAByName(self.rtype, "QLayoutItem")
        rtypePaintDevice = (
            muapi.isAByName(self.rtype, "QPaintDevice")
            and self.rtype.split(".")[-1] not in pointerTypes
            and self.rtype.split(".")[-1] not in primitiveTypes
        )
        rtypeIODeviceBase = muapi.isAByName(self.rtype, "QIODeviceBase")
        rmaker = None

        if rtypeObject:
            rmaker = "makeinstance"
        elif rtypeLayoutItem:
            rmaker = "makelayoutitem"
        elif rtypePaintDevice:
            rmaker = "makepaintdevice"
        elif rtypeIODeviceBase:
            rmaker = "makeiodevicebase"

        if isQObject or isQLayoutItem or isQPaintDevice or isQIODeviceBase:
            if self.isconstructor:
                body += "    %s(param_this, %s);\n    return param_this;\n" % (
                    setter,
                    expr,
                )
            elif self.rtype == "void":
                if altexpr:
                    # body += "    if (dynamic_cast<const MuQt_%s*>(arg0)) %s;\n" % (muclass.name, expr)
                    body += "    if (%s(arg0)) %s;\n" % (isafunc, expr)
                    body += "    else %s;\n" % altexpr
                else:
                    body += "    %s;\n" % expr
            else:
                if altexpr:
                    needsDeref = muapi.classes.has_key(self.rtype)
                    body += "    return %s(arg0) ? " % isafunc
                    if needsDeref:
                        if rmaker:
                            body += '%s<%sType>(c, %s, "%s")' % (
                                rmaker,
                                self.rtype,
                                expr,
                                "qt." + self.rtype,
                            )
                            body += ' : %s<%sType>(c, %s, "%s");\n' % (
                                rmaker,
                                self.rtype,
                                altexpr,
                                "qt." + self.rtype,
                            )
                        else:
                            body += "%s : %s;\n" % (
                                convertTo(expr, self.rtype),
                                convertTo(altexpr, self.rtype),
                            )
                    else:
                        body += "%s : %s;\n" % (
                            convertTo(expr, qtrtype),
                            convertTo(altexpr, qtrtype),
                        )
                else:
                    needsDeref = muapi.classes.has_key(self.rtype)
                    if needsDeref:
                        if rmaker:
                            body += '    return %s<%sType>(c, %s, "%s");\n' % (
                                rmaker,
                                self.rtype,
                                expr,
                                "qt." + self.rtype,
                            )
                        else:
                            body += "    return %s;\n" % convertTo(expr, self.rtype)
                    else:
                        body += "    return %s;\n" % convertTo(expr, qtrtype)
        elif muclass.pointertype:
            if self.isconstructor:
                body += "    %s;\n    return param_this;\n" % setExpr(
                    "param_this", expr, muclass.name
                )
            elif self.rtype == "void":
                body += "    %s;\n" % expr
                if self.ismember:
                    body += "    %s;\n" % setExpr("param_this", "arg0", muclass.name)
            else:
                body += "    return %s;\n" % convertTo(expr, qtrtype)
        # elif muclass.isBaseClass():
        else:
            if muclass.primitivetype and self.isconstructor:
                n = muclass.name
                # n = muclass.baseClassNames()[0]
                body += "    %s;\n" % setExpr("param_this", expr, n)
                # body += "    setqtype<%sType>(param_this,%s);\n" % (n, expr)
                body += "    return param_this;\n"
            elif self.rtype == "void":
                body += "    %s;\n" % expr
                if muclass.primitivetype and self.ismember:
                    body += "    %s;\n" % setExpr("param_this", "arg0", muclass.name)
            else:
                # body += "    return %s;\n" % convertTo(expr, qtrtype)
                needsDeref = muapi.classes.has_key(self.rtype)
                if needsDeref and rtypeObject:
                    body += '    return makeinstance<%sType>(c, %s, "%s");\n' % (
                        self.rtype,
                        expr,
                        "qt." + self.rtype,
                    )
                elif needsDeref:
                    body += "    return %s;\n" % convertTo(expr, self.rtype)
                else:
                    body += "    return %s;\n" % convertTo(expr, qtrtype)

        out += "    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());\n"
        out += body
        out += "}\n"
        return out

    def muDeclaration(self):
        out = "%s (%s; " % (self.name, self.rtype)
        comma = False
        for aname, mutype, aval in self.args:
            if comma:
                out += ", "
            out += "%s %s" % (mutype, aname)
            comma = True
        out += ")"
        if self.isprotected:
            out += " // protected"
        return out


class MuClass:
    def __init__(self, api, qtnamespace, muapi):
        self.qt = qtnamespace
        self.name = qtnamespace.name
        self.isinterface = self.name in interfaceTypes
        self.functions = []
        self.virtuals = []
        self.statics = []
        self.globalfuncs = []
        self.castoperators = []
        self.enums = []
        self.F_trans = (
            lambda f: not f.failed
            and not f.isprop
            and (self.inheritable or not f.isprotected)
        )
        self.funcCount = {}
        self.inherits = []
        self.inheritedby = []
        self.muapi = muapi
        self.primitivetype = isAPrimitiveType(self.name)
        self.pointertype = isAPointerType(self.name)
        self.demotedProps = qtnamespace.demotedProps
        self.hasdestructor = False
        self.iscopyonwrite = isCopyOnWrite(self.name)
        self.inheritedVirtuals = []

        for f in qtnamespace.functions:
            fname = f[0]
            rtype = f[3]
            cppname = qtnamespace.name + "::" + fname
            message(
                "functions fname = %s, rtype = %s, cppname = %s"
                % (fname, rtype, cppname)
            )
            exclude = isFunctionExcluded(qtnamespace, f)
            if exclude and verbose:
                print("EXCLUDED:", str(f))
            if fname.startswith("~"):
                self.hasdestructor = True
            if rtype == "operator":
                # cast operator
                exclude = isFunctionExcluded(qtnamespace, f)
                if not exclude:
                    newqtfunc = (fname, "", [("from", self.name, None)], "", False)
                    mufunc = MuFunction(api, self, newqtfunc, True, False, True)
                    self.castoperators.append(mufunc)
            elif not fname.startswith("~") and not exclude:
                mufunc = MuFunction(
                    api, self, f, True, f in qtnamespace.protectedfuncs, False
                )
                if mufunc.virtual and not mufunc.failed:
                    mufunc.virtualSlot = len(self.virtuals)
                    self.virtuals.append(mufunc)
                # can decide not to be a member
                if mufunc.ismember and not mufunc.operator:
                    self.functions.append(mufunc)
                else:
                    self.statics.append(mufunc)

        if not self.primitivetype and not self.pointertype and not self.isinterface:
            for p in qtnamespace.inherits:
                self.collectVirtuals(p)

            # if len(self.inheritedVirtuals) > 0:
            #    print "----", self.name
            for ft in self.inheritedVirtuals:
                (f, protected, origin) = ft
                fhere = self.name + "::" + f[0]
                if not self.hasFunction(f[0]) and not isFunctionExcluded(
                    qtnamespace, f
                ):
                    mufunc = MuFunction(api, self, f, True, protected, False)
                    if mufunc.virtual and not mufunc.failed:
                        mufunc.virtualSlot = len(self.virtuals)
                        self.virtuals.append(mufunc)
                    # can decide not to be a member
                    if mufunc.ismember and not mufunc.operator:
                        self.functions.append(mufunc)

        for f in qtnamespace.staticfuncs:
            fname = f[0]
            cppname = qtnamespace.name + "::" + fname
            exclude = isFunctionExcluded(qtnamespace, f)
            print(qtnamespace)
            message(
                "staticfuncs fname = %s, cppname = %s, exclude = %s"
                % (fname, cppname, str(exclude))
            )
            if not exclude:
                self.statics.append(
                    MuFunction(
                        api, self, f, False, f in qtnamespace.protectedfuncs, False
                    )
                )

        for e in qtnamespace.enums:
            cppname = qtnamespace.name + "::" + e.name.split(":")[-1]
            exclude = cppname in exclusionMap
            if not exclude:
                self.enums.append(MuEnum(e, self))

    def hasFunction(self, name):
        for f in self.functions:
            if f.name == name:
                return True
        return False

    def collectVirtuals(self, qtname):
        if api.classes.has_key(qtname):
            qtnamespace = api.classes[qtname]
            for f in qtnamespace.functions:
                if "virtual" in f[3]:
                    if not self.hasFunction(f[0]) and "~" not in f[0]:
                        # if self.name == "QLayout":
                        #    print " -> ",str(f)
                        self.inheritedVirtuals.append(
                            (f, f in qtnamespace.protectedfuncs, qtnamespace)
                        )
            for p in qtnamespace.parents:
                self.collectVirtuals(p)

    def enumByName(self, fullname):
        nameparts = fullname.split("::")
        localname = nameparts[-1]
        classname = nameparts[0]

        if classname != localname and classname != self.name:
            muclass = self.muapi.classForName(classname)
            if muclass:
                return muclass.enumByName(fullname)

        for e in self.enums:
            if e.name == localname:
                return e
        return None

    def enumTypeRep(self, fullname):
        e = self.enumByName(fullname)
        if e and e.protected:
            return "MuQtPublicEnum"
        else:
            return fullname

    def outputMuDeclarations(self):
        F_prop = lambda f: not f.failed and f.isprop
        F_failed = lambda f: f.failed

        print("---enums translated---")
        for e in self.enums:
            if e.output:
                print(e.name, ":= int")
                if e.flags:
                    print(e.flags, ":= int")
                for enum in e.symbols:
                    (name, val) = enum
                    print(name, ":=", val)

        print("---functions translated---")
        count = 1
        for f in filter(self.F_trans, self.functions):
            print("% 4d  %s" % (count, f.muDeclaration()))
            count = count + 1

        print("---functions that are props---")
        count = 1
        for f in filter(F_prop, self.functions):
            print("% 4d  %s" % (count, f.muDeclaration()))
            count = count + 1

        print("---static functions translated---")
        count = 1
        for f in filter(self.F_trans, self.statics):
            print("% 4d  %s" % (count, f.muDeclaration()))
            count = count + 1

        print("---cast operators translated---")
        count = 1
        for f in filter(self.F_trans, self.castoperators):
            print("% 4d  %s" % (count, f.muDeclaration()))
            count = count + 1

        print("---functions that failed---")
        count = 1
        for f in filter(F_failed, self.functions):
            print("% 4d  %s" % (count, f.muDeclaration()))
            count = count + 1

    def outputCompiledNodes(self):
        out = ""
        for f in filter(
            self.F_trans, self.functions + self.statics + self.castoperators
        ):
            out += f.compiledFunction()
            out += "\n"
        return out

    def outputNodeImplementations(self):
        out = ""
        print("self.statics for {0}".format(self.name))

        for f in filter(
            self.F_trans, self.functions + self.statics + self.castoperators
        ):
            out += f.nodeImplementation()
            out += "\n"
        return out

    def isA(self, muclass):
        if self == muclass:
            return True
        else:
            for c in self.inherits:
                if c.isA(muclass):
                    return True
        return False

    def isAByName(self, muclassName):
        if self.muapi.classes.has_key(muclassName):
            return self.isA(self.muapi.classes[muclassName])
        else:
            return False

    def baseClassNames(self):
        all = []
        if len(self.inherits) == 0:
            all.append(self.name)
            return all
        for c in self.inherits:
            all.extend(c.baseClassNames())
        return all

    def isBaseClass(self):
        return not self.inherits or len(self.inherits) == 0

    def assembleIncludes(self, cpplines):
        global makeRE
        itypes = set([])
        endinclude = 0

        for i in range(0, len(cpplines)):
            m = makeRE.search(cpplines[i])
            if "#include" in cpplines[i]:
                endinclude = i
            if m:
                itype = m.group(2)
                if itype != (self.name + "Type"):
                    itypes.add(itype)

        for t in itypes:
            cpplines.insert(endinclude + 1, "#include <MuQt6/%s.h>\n" % t)

    def outputEnumDeclarations(self):
        out = "addSymbols(\n"
        for e in self.enums:
            out += "    %s,\n" % e.aliasDeclation()
            flags = e.flagsAliasDeclaration()
            if flags:
                out += "    %s,\n" % flags
            for enum in e.symbols:
                out += "      %s,\n" % e.symbolDeclaration(enum)

        out += "    EndArguments);\n"
        return out

    def outputSymbolDeclarations(self):
        out = "addSymbols(\n"
        out += "    // enums\n"
        for e in self.enums:
            if e.output:
                out += "    %s,\n" % e.aliasDeclation()
                if e.flags:
                    out += "    %s,\n" % e.flagsAliasDeclaration()
                for enum in e.symbols:
                    out += "      %s,\n" % e.symbolDeclaration(enum)
        out += "    // member functions\n"

        for f in self.functions:
            if f.failed:
                out += "    // MISSING: %s\n" % f.muDeclaration()
            elif f.isprop:
                out += "    // PROP: %s\n" % f.muDeclaration()
            elif f.isprotected and not self.inheritable:
                out += "    // NOT INHERITABLE PROTECTED: %s\n" % f.muDeclaration()
            elif f.isconstructor and f.abstract and not self.inheritable:
                out += "    // ABSTRACT CONSTRUCTOR: %s\n" % f.muDeclaration()
            elif f.isconstructor and f.isprotected:
                out += "    // CONSTRUCTOR IS PROTECTED: %s\n" % f.muDeclaration()
            elif f.virtualSlot != -1 and self.inheritable:
                out += "    _func[%d] = %s,\n" % (f.virtualSlot, f.symbolDeclaration())
            else:
                out += "    %s,\n" % f.symbolDeclaration()

        out += "    // static functions\n"

        for f in self.statics:
            if not f.operator:
                if f.failed:
                    out += "    // MISSING: %s\n" % f.muDeclaration()
                else:
                    out += "    %s,\n" % f.symbolDeclaration()

        out += "    EndArguments);\n"
        out += "globalScope()->addSymbols(\n"

        for f in self.statics:
            if f.operator:
                if f.failed:
                    out += "    // MISSING: %s\n" % f.muDeclaration()
                else:
                    out += "    %s,\n" % f.symbolDeclaration()

        out += "    EndArguments);\n"
        out += "scope()->addSymbols(\n"

        for f in self.castoperators:
            if f.failed:
                out += "    // MISSING: %s\n" % f.muDeclaration()
            else:
                out += "    %s,\n" % f.symbolDeclaration()

        out += "    EndArguments);\n"
        return out

    def hasFlagType(self, flagName):
        for e in self.enums:
            if e.flags is not None and flagName == e.flags:
                return True
        return False

    def hasEnumType(self, enumName):
        for e in self.enums:
            if enumName == e.name:
                return True
            for s in e.symbols:
                (name, val) = s
                if name == enumName:
                    return True
        return False

    def needsLocalQualification(self, name):
        if name is None or intRE.match(name):
            return False
        return self.hasEnumType(name) or self.hasFlagType(name)

    def qualifyValue(self, value):
        if "(" in value or "|" in value:
            newvalue = value
            parts = (
                value.replace("(", " ").replace(")", " ").replace("|", " ").split(" ")
            )
            for p in parts:
                if p != "" and p is not None:
                    if self.needsLocalQualification(p):
                        newvalue = newvalue.replace(p, "%s::%s" % (self.name, p))
            return newvalue
        if self.needsLocalQualification(value):
            return self.name + "::" + value

        for parent in self.inherits:
            v = parent.qualifyValue(value)
            if v != value:
                return v

        qt = self.muapi.qtClass
        if qt == self:
            return value
        else:
            return qt.qualifyValue(value)

    def outputMuQtNativeImplementation(self):
        if not allowInheritance or not self.inheritable:
            return ""

        out = "// Inheritable object implemenation\n\n"
        # constructors
        qt = self.qt
        out = ""

        if self.hasdestructor:
            out += "// destructor\n"
            out += "MuQt_" + self.name + "::~MuQt_" + self.name + "()\n"
            out += "{\n"
            out += "    if (_obj)\n"
            out += "    {\n"
            out += "        *_obj->data<Pointer>() = Pointer(0);\n"
            out += "        _obj->releaseExternal();\n"
            out += "    }\n"
            out += "    _obj = 0;\n"
            out += "    _env = 0;\n"
            out += "    _baseType = 0;\n"
            out += "}\n\n"

        for f in self.functions:
            if not f.failed:
                (name, fconst, params, rtype, prop) = f.qtfunc
                purevirtual = "= 0" in fconst
                fconst = string.replace(fconst, "= 0", "")
                if f.isconstructor or f.virtual:
                    rtype_clean = string.strip(string.replace(rtype, "virtual", ""))
                    if self.needsLocalQualification(rtype_clean):
                        rtype_clean = self.name + "::" + rtype_clean
                    # rtype_clean = rtype
                    # output the function rtype name and args
                    if f.isconstructor:
                        out += (
                            "MuQt_"
                            + name
                            + "::MuQt_"
                            + name
                            + "(Pointer muobj, const CallEnvironment* ce"
                        )
                    else:
                        out += rtype_clean + " MuQt_" + self.name + "::" + name + "("

                    # args
                    for i in range(0, len(params)):
                        p = params[i]
                        (pname, ptype, pval) = p
                        if i > 0 or f.isconstructor:
                            out += ", "
                        out += ptype + " " + pname

                    out += ") " + fconst

                    if f.isconstructor:
                        # call the base class constr
                        out += "\n : " + name + "("
                        for i in range(0, len(params)):
                            p = params[i]
                            (pname, ptype, pval) = p
                            if i > 0:
                                out += ", "
                            out += pname
                        out += ")"

                    if purevirtual:
                        # pure virtual func
                        # what to do here?
                        out += " // pure virtual"

                    out += "\n{\n"

                    # output the rest of the body

                    if f.isconstructor:
                        out += "    _env = ce;\n"
                        out += "    _obj = reinterpret_cast<ClassInstance*>(muobj);\n"
                        out += "    _obj->retainExternal();\n"
                        out += (
                            "    MuLangContext* c = (MuLangContext*)_env->context();\n"
                        )
                        out += (
                            '    _baseType = c->findSymbolOfTypeByQualifiedName<%sType>(c->internName("qt.%s"));\n'
                            % (self.name, self.name)
                        )
                    else:
                        out += "    if (!_env) "
                        if purevirtual:
                            if rtype_clean == "void":
                                out += "return;\n"
                            else:
                                out += "return defaultValue<%s>();\n" % rtype_clean
                        else:
                            if rtype_clean == "void":
                                out += "{ "
                            else:
                                out += "return "
                            out += self.name + "::" + f.name + "("
                            for i in range(0, len(params)):
                                p = params[i]
                                (pname, ptype, pval) = p
                                if i > 0 or f.isconstructor:
                                    out += ", "
                                out += pname
                            out += ");"
                            if rtype_clean == "void":
                                out += " return; }"
                            out += "\n"
                        out += (
                            "    MuLangContext* c = (MuLangContext*)_env->context();\n"
                        )
                        out += (
                            "    const MemberFunction* F0 = _baseType->_func[%d];\n"
                            % f.virtualSlot
                        )
                        out += "    const MemberFunction* F = _obj->classType()->dynamicLookup(F0);\n"
                        out += "    if (F != F0) \n    {\n"
                        out += "        Function::ArgumentVector args(%d);\n" % (
                            len(params) + 1
                        )
                        # if f.name == "splitPath":
                        #   print str(parms)
                        out += "        args[0] = Value(Pointer(_obj));\n"
                        for i in range(0, len(params)):
                            p = params[i]
                            # print str(f.args[i+1])
                            out += "        args[%d] = Value(%s);\n" % (
                                i + 1,
                                convertTo(p[0], p[1]),
                            )
                        out += "        Value rval = _env->call(F, args);\n"
                        if f.rtype != "void":
                            out += (
                                "        return %s;\n"
                                % f.derefExp(f.unpackReturnValue("rval"), f.rtype)[0]
                            )
                        out += "    }\n"
                        out += "    else\n"
                        out += "    {\n        "
                        if rtype_clean != "void":
                            out += "return "
                        if purevirtual:
                            out += "defaultValue<%s>();\n" % rtype_clean
                        else:
                            out += self.name + "::" + f.name + "("
                            for i in range(0, len(params)):
                                p = params[i]
                                (pname, ptype, pval) = p
                                if i > 0 or f.isconstructor:
                                    out += ", "
                                out += pname
                            out += ");\n"
                        out += "    }\n"

                    # finish
                    out += "}\n\n"

        return out

    def outputMuQtNativeDeclaration(self):
        if not allowInheritance or not self.inheritable:
            return ""

        isQWebEngineHistory = self.isAByName("QWebEngineHistory")

        out = "// Inheritable object\n\n"
        out += "class MuQt_%s : public %s\n{\n  public:\n" % (self.name, self.name)
        if self.hasdestructor or isQWebEngineHistory:
            out += "    virtual ~MuQt_" + self.name + "();\n"

        # constructors
        qt = self.qt
        protected = False
        hasprotected = False
        for f in self.functions:
            if not f.failed:
                if f.isprotected:
                    hasprotected = True
                (name, fconst, params, rtype, prop) = f.qtfunc
                fconst = string.replace(fconst, "= 0", "")
                if f.isconstructor or f.virtual:
                    if f.isprotected and not protected:
                        protected = True
                        out += "  protected:\n"
                    elif not f.isprotected and protected:
                        protected = False
                        out += "  public:\n"
                    if f.isconstructor:
                        out += (
                            "    "
                            + "MuQt_"
                            + name
                            + "(Pointer muobj, const CallEnvironment*"
                        )
                    else:
                        out += "    " + rtype + " " + name + "("
                    for i in range(0, len(params)):
                        p = params[i]
                        (pname, ptype, pval) = p
                        if i > 0 or f.isconstructor:
                            out += ", "
                        out += ptype + " " + pname
                    out += ") " + fconst + ";\n"
        # make public versions of the protected funcs
        if hasprotected:
            out += "  public:\n"
            for f in self.functions:
                if not f.failed:
                    (name, fconst, params, rtype, prop) = f.qtfunc
                    fconst = string.replace(fconst, "= 0", "")
                    nvrtype = string.replace(rtype, "virtual ", "")
                    if f.isprotected and not f.isconstructor:
                        for parent in [False, True]:
                            nameSuffix = ""
                            if parent:
                                nameSuffix = "_parent"
                            out += (
                                "    " + nvrtype + " " + name + "_pub%s(" % nameSuffix
                            )
                            for i in range(0, len(params)):
                                p = params[i]
                                (pname, ptype, pval) = p
                                if "::" in ptype:
                                    ptype = self.enumTypeRep(ptype)
                                if i > 0:
                                    out += ", "
                                out += ptype + " " + pname
                            out += ") " + fconst
                            out += " { "
                            if "void" not in rtype:
                                out += "return "
                            if parent:
                                out += self.name + "::"
                            out += name + "("
                            for i in range(0, len(params)):
                                p = params[i]
                                (pname, ptype, pval) = p
                                if i > 0:
                                    out += ", "
                                if "::" in ptype:
                                    # need to cast it to the protected enum
                                    if self.enumTypeRep(ptype) != ptype:
                                        out += "(%s)" % ptype
                                out += pname
                            out += "); }\n"
        if customNativeFuncsHeader.has_key(self.name):
            out += "  public:\n"
            out += "    "
            out += customNativeFuncsHeader[self.name]
            out += "\n"
        out += "  public:\n"
        out += "    const %sType* _baseType;\n" % self.name
        out += "    ClassInstance* _obj;\n"
        out += "    const CallEnvironment* _env;\n"
        out += "};\n"
        return out

    def outputSourceFiles(self):

        name = self.name
        if name == "":
            name = "Global"

        templateCPP = None
        templateH = None

        if name == "Global":
            templateCPP = templateGlobalCPP
            templateH = templateGlobalH
        elif self.primitivetype:
            templateCPP = templateTypeCPP
            templateH = templateTypeH
        elif self.isinterface:
            templateCPP = templateInterfaceTypeCPP
            templateH = templateInterfaceTypeH
        elif self.pointertype:
            templateCPP = templatePointerTypeCPP
            templateH = templatePointerTypeH
        elif self.isAByName("QLayoutItem") and not self.isAByName("QObject"):
            templateCPP = templateLayoutItemCPP
            templateH = templateLayoutItemH
        elif self.isAByName("QPaintDevice") and not self.isAByName("QObject"):
            templateCPP = templatePaintDeviceCPP
            templateH = templatePaintDeviceH
        elif self.isAByName("QIODeviceBase") and not self.isAByName("QObject"):
            templateCPP = templateIODeviceBaseCPP
            templateH = templateIODeviceBaseH
        else:
            templateCPP = templateObjectCPP
            templateH = templateObjectH

        handrolledSymbols = "handrolled/%sSymbols.cpp" % name
        handrolledDefs = "handrolled/%sDefinitions.cpp" % name
        handrolledInclude = "handrolled/%sIncludes.cpp" % name

        hsyms = None
        hdefs = None
        hincs = None

        if os.path.exists(handrolledSymbols):
            xfile = open(handrolledSymbols, "r")
            hsyms = xfile.readlines()
            xfile.close()

        if os.path.exists(handrolledDefs):
            xfile = open(handrolledDefs, "r")
            hdefs = xfile.readlines()
            xfile.close()

        if os.path.exists(handrolledInclude):
            xfile = open(handrolledInclude, "r")
            hincs = xfile.readlines()
            xfile.close()

        cppfile = open(templateCPP, "r")

        cpplines = []

        while True:
            line = cppfile.readline()
            if line == "":
                break
            if line.find("{%%definitions%%}") != -1:
                cpplines.extend(self.outputCompiledNodes().split("\n"))
                cpplines.extend(self.outputNodeImplementations().split("\n"))
            elif line.find("{%%addSymbols%%}") != -1:
                cpplines.extend(self.outputSymbolDeclarations().split("\n"))
            elif line.find("{%%addSymbolsEnums%%}") != -1:
                cpplines.extend(self.outputEnumDeclarations().split("\n"))
            elif line.find("{%%nativeMuQtClassImplemenation%%}") != -1:
                cpplines.extend(self.outputMuQtNativeImplementation().split("\n"))
            elif line.find("{%%propExclusions%%}") != -1:
                if len(self.demotedProps):
                    s = "    const char* propExclusions[] = {"
                    for i in range(0, len(self.demotedProps)):
                        if i != 0:
                            s += ", "
                        s += '"%s"' % self.demotedProps[i][0]
                    s += ", 0"
                    s += "};"
                    cpplines.append(s)
                else:
                    cpplines.append("    const char** propExclusions = 0;")
            elif line.find("{%%nativeObject%%}") != -1:
                if self.name == "QObject":
                    cpplines.append(
                        '   new MemberVariable(c, "native", "qt.NativeObject"),'
                    )
            elif line.find("{%%addHandRolledSymbols%%}") != -1:
                if hsyms:
                    cpplines.extend(hsyms)
            elif line.find("{%%handRolledDefinitions%%}") != -1:
                if hdefs:
                    cpplines.extend(hdefs)
            elif line.find("{%%handRolledInclude%%}") != -1:
                if hincs:
                    cpplines.extend(hincs)
            else:
                cpplines.append(string.replace(line, "$T", self.name))
        cppfile.close()

        for i in range(0, len(cpplines)):
            line = cpplines[i]
            if len(line) >= 1 and line[-1] != "\n":
                cpplines[i] = line + "\n"
            elif len(line) == 0:
                cpplines[i] = "\n"

        self.assembleIncludes(cpplines)

        cppout = None

        if name == "Global":
            print("OPENING qtGlobals.cpp for WRITE")
            cppout = open("qtGlobals.cpp", "w")
        else:
            cppout = open(name + "Type.cpp", "w")
        cppout.writelines(cpplines)
        cppout.close()

        if self.name not in noHFileOutput:
            hfile = open(templateH, "r")
            hlines = []
            while True:
                line = hfile.readline()
                if line == "":
                    break
                elif line.find("{%%nativeMuQtClass%%}") != -1:
                    hlines.append(self.outputMuQtNativeDeclaration())
                    hlines.append("\n")
                elif line.find("{%%isInheritableFunc%%}") != -1:
                    if self.inheritable:
                        hlines.append(
                            "    static bool isInheritable() { return true; }\n"
                        )
                    else:
                        hlines.append(
                            "    static bool isInheritable() { return false; }\n"
                        )
                elif line.find("{%%virtualArray%%}") != -1:
                    hlines.append(
                        "    MemberFunction* _func[%d];\n" % len(self.virtuals)
                    )
                elif line.find("{%%cachedInstanceFunc%%}") != -1:
                    if self.inheritable:
                        hlines.append(
                            "inline ClassInstance* %sType::cachedInstance(const %sType::MuQtType* obj) { return obj->_obj; }\n"
                            % (name, name)
                        )
                    else:
                        hlines.append(
                            "inline ClassInstance* %sType::cachedInstance(const %sType::MuQtType* obj) { return 0; }\n"
                            % (name, name)
                        )
                elif line.find("{%%muqtForwardDeclaration%%}") != -1:
                    if self.inheritable:
                        hlines.append("class MuQt_" + name + ";\n")
                elif line.find("{%%typeDeclarations%%}") != -1:
                    if self.name == "QObject":
                        hlines.append("    struct Struct { QObject* object; };\n")
                    if self.name == "QLayoutItem":
                        hlines.append("    struct Struct { QLayoutItem* object; };\n")
                    if self.name == "QPaintDevice":
                        hlines.append("    struct Struct { QPaintDevice* object; };\n")
                    if self.name == "QIODeviceBase":
                        hlines.append("    struct Struct { QIODeviceBase* object; };\n")
                    if not self.inheritable:
                        hlines.append("    typedef " + name + " MuQt_" + name + ";\n")
                else:
                    hlines.append(string.replace(line, "$T", name))
            hfile.close()

            hout = None
            if name == "Global":
                hout = open("qtGlobals.h", "w")
            else:
                hout = open(name + "Type.h", "w")
            hout.writelines(hlines)
            hout.close()

    def outputModuleDefinition(self, finishedMap):
        out = ""
        inherits = filter(lambda x: not x.isinterface, self.inherits)
        inherits.sort(inheritCMP)
        if self not in finishedMap:
            print("Processing {0}...".format(self.name))
            if self.name in includeClasses:
                print("... is in includeClasses")
                if len(inherits) > 0:
                    out += '    %sType* t_%s = new %sType(c, "%s"' % (
                        self.name,
                        self.name,
                        self.name,
                        self.name,
                    )
                    for i in range(0, len(inherits)):
                        out += ", t_%s" % inherits[i].name
                    out += ");"
                else:
                    print("... no inherits")
                    out += '    %sType* t_%s = new %sType(c, "%s");' % (
                        self.name,
                        self.name,
                        self.name,
                        self.name,
                    )
                out += " qt->addSymbol(t_%s);\n" % self.name
                finishedMap[self] = self
        # for c in self.inheritedby:
        #    out += c.outputModuleDefinition(finishedMap)
        return out


class MuAPI:
    def __init__(self, api):
        self.module = None
        self.classes = {}
        self.api = api
        self.qtClass = None

        for k in api.classes.keys():
            qtclass = api.classes[k]
            muclass = MuClass(api, qtclass, self)
            if muclass.name == "Qt":
                self.qtClass = muclass
            self.classes[muclass.name] = muclass

        for k in self.classes.keys():
            c = self.classes[k]
            for i in c.qt.inheritedby:
                if self.classes.has_key(i):
                    child = self.classes[i]
                    child.inherits.append(c)
                    c.inheritedby.append(child)
                else:
                    pass
                    # print "missing class", i

        global allowInheritance
        for k in self.classes.keys():
            c = self.classes[k]
            isQObject = c.isAByName("QObject")
            isQPaintDevice = c.isAByName("QPaintDevice")
            isQLayoutItem = c.isAByName("QLayoutItem")
            isQIODeviceBase = c.isAByName("QIODeviceBase")
            c.inheritable = (
                (isQObject or isQLayoutItem or isQPaintDevice or isQIODeviceBase)
                and (c.name not in notInheritableTypes)
                and allowInheritance
            )

    def classForName(self, name):
        if self.classes.has_key(name):
            return self.classes[name]
        else:
            return None

    def isAByName(self, name, classname):
        c = self.classForName(name)
        if c:
            return c.isAByName(classname)
        else:
            return False


##
##  QtDocParser parses the html file
##


class QtDocParser(SGMLParser):
    def __init__(self, url):
        SGMLParser.__init__(self)
        self.buffer = ""
        self.inlist = False
        self.functable = False
        self.infunc = False
        self.intitle = False
        self.indetails = False
        self.funcIsProp = False
        self.h3 = False
        self.h2 = False
        self.acount = 0
        self.pcount = 0
        self.precount = 0
        self.public = []
        self.globaldefs = []
        self.staticPublic = []
        self.protected = []
        self.slots = []
        self.macros = []
        self.signals = []
        self.related = []
        self.properties = []
        self.enumaccess = False
        self.intypes = False
        self.inpubtypes = False
        self.protenums = []
        self.enums = []
        self.enumName = None
        self.enumValue = None
        self.defsover = False
        self.qtnamespace = None
        self.ininheritedby = False
        self.ininherits = False
        self.childurls = []
        self.bucket = self.globaldefs
        self.url = url
        self.defenum = False
        self.td = False
        self.includes = []
        self.classRE = re.compile("([A-Za-z0-9]+) Class")
        self.namespaceRE = re.compile("([A-Za-z0-9]+) Namespace")
        self.includeRE = re.compile("#include ")
        self.ininclude = False
        self.inheritedby = []
        self.inherits = []
        self.modulespan = False
        self.module = "--"

        ui = self.url.rfind("/")
        self.urlbase = url[: ui + 1]

        usock = urllib.urlopen(url)
        self.feed(usock.read())
        usock.close()

        self.qtnamespace.module = self.module
        self.qtnamespace.inheritedby = self.inheritedby
        self.qtnamespace.inherits = self.inherits
        self.convertFunctions()
        self.convertProperties()
        self.convertEnums()
        self.qtnamespace.includes = self.includes
        self.qtnamespace.finish()

    def result(self):
        return self.qtnamespace

    def addChildURL(self, url):
        if string.find(url, "q3") != -1:
            return
        p = url.split("/")
        if p[0] == url:
            self.childurls.append(os.path.join(self.urlbase, url))
        else:
            self.childurls.append(url)

    def start_title(self, attrs):
        self.intitle = True

    def end_title(self):
        self.intitle = False
        isclass = True
        g = self.classRE.search(self.title)
        if not g:
            isclass = False
            g = self.namespaceRE.search(self.title)
            if not g:
                print('ERROR: nothing is matching "%s"' % self.title)
                exit(0)
        self.qtnamespace = NamespaceInfo(g.group(1))
        self.qtnamespace.isclass = isclass

    def start_span(self, attrs):
        for attr, value in attrs:
            if value == "small-subtitle":
                self.modulespan = True

    def end_span(self):
        self.modulespan = False

    def start_pre(self, attrs):
        self.precount += 1

    def end_pre(self):
        self.precount -= 1
        self.ininclude = False

    def start_p(self, attrs):
        self.pcount += 1

    def end_p(self):
        self.ininheritedby = False
        self.ininherits = False
        self.pcount -= 1

    def start_a(self, attrs):
        if self.ininheritedby:
            for attr, value in attrs:
                if attr == "href":
                    self.addChildURL(value)
        elif self.infunc:
            for attr, value in attrs:
                if attr == "href":
                    self.funcIsProp = value.rfind("-prop") != -1
        self.acount += 1

    def end_a(self):
        self.acount -= 1

    def start_h2(self, attrs):
        self.h2 = True

    def end_h2(self):
        self.h2 = False

    def start_h3(self, attrs):
        self.h3 = True

    def end_h3(self):
        self.h3 = False

    def start_li(self, attrs):
        self.inlist = True

    def end_li(self):
        if not self.defsover:
            if self.infunc:
                self.bucket.append(self.buffer)
                if self.funcIsProp:
                    self.bucket[-1] += "@"
                self.buffer = ""
            self.inlist = False
            self.infunc = False
            self.funcIsProp = False

    def start_table(self, attrs):
        pass

    def end_table(self):
        self.functable = False
        self.defenum = False

    def start_tr(self, attrs):
        if self.functable:
            self.infunc = True
        self.td = 0

    def end_tr(self):
        self.ininheritedby = False
        self.ininherits = False
        if self.functable:
            if not self.defsover:
                if self.infunc:
                    self.bucket.append(self.buffer)
                    if self.funcIsProp:
                        self.bucket[-1] += "@"
                    self.buffer = ""
                self.infunc = False
                self.funcIsProp = False

        else:
            if self.defenum:
                if self.enumName and self.enumValue:
                    self.enums[-1].enums.append(Enum(self.enumName, self.enumValue))
            self.enumName = None
            self.enumValue = None

    def start_td(self, attrs):
        self.td = self.td + 1

    def start_div(self, attrs):
        self.infunc = ("class", "fn") in attrs

    def handle_data(self, data):
        if self.precount > 0:
            if self.includeRE.search(data):
                self.ininclude = True
        elif self.td > 0 and data.strip() == "Inherited By:":
            self.ininheritedby = True
            return
        elif self.td > 0 and data.strip() == "Inherits:":
            self.ininherits = True
            return
        elif data == "Obsolete flags:":  # note obsolete
            self.defenum = False
        elif data == " (preliminary)":  # shows up in qwidget.html, messes up everything
            return
        elif data == " (deprecated)":  # shows up in qprocess.html, messes up everything
            return
        elif self.modulespan and "Q" in data:
            self.module = data
        elif self.intypes and not self.inpubtypes:
            if data == "enum " or data == " enum ":
                self.enumaccess = True
            elif self.enumaccess:
                self.protenums.append("%s::%s" % (self.qtnamespace.name, data))
                self.enumaccess = False
        # more
        if self.h3:
            # if "flags" in data:
            #    print data
            if data[0:5] == "enum ":
                self.defenum = True
                self.enums.append(Enumeration(data.split()[-1]))
            elif data[0:6] == "flags " and self.defenum:
                self.enums[-1].flags = data.split()[-1]
            elif data[0:7] == ">flags " and self.defenum:
                # hack for malformed docs
                c = data.split()[1].replace("<", " ").split()[0]
                n = data.replace(">", " ").replace("<", " ").split()[-1]
                self.enums[-1].flags = c + n
            elif self.defenum:
                n = self.enums[-1].name
                if n is None:
                    self.enums[-1].name = data
                elif n[-2:] == "::":
                    self.enums[-1].name = n + data
        elif self.defenum:
            if self.td == 1:
                self.enumName = data
            elif self.td == 2:
                self.enumValue = data
            else:
                # docs
                pass
        elif not self.defsover:
            if self.intitle:
                self.title = data
            elif self.ininclude:
                if "#" in data:
                    self.includes.append(data)
                else:
                    self.includes[-1] = self.includes[-1] + data
            elif self.ininheritedby:
                if (
                    not data.startswith(",")
                    and not data.startswith(".")
                    and not data.startswith(" and")
                    and not data == " "
                    and not data.startswith("Inherited")
                ):
                    self.inheritedby.append(data)
            elif self.ininherits:
                if (
                    not data.startswith(",")
                    and not data.startswith(".")
                    and not data.startswith(" and")
                    and not data == " "
                    and not data.startswith("Inherits")
                ):
                    self.inherits.append(data)
            elif self.h2:
                self.intypes = False
                if (
                    data == "Public Functions"
                    or data == "Reimplemented Public Functions"
                ):
                    self.bucket = self.public
                    self.functable = True
                elif (
                    data == "Protected Functions"
                    or data == "Reimplemented Protected Functions"
                ):
                    self.bucket = self.protected
                    self.functable = True
                elif data == "Static Public Members":
                    self.bucket = self.staticPublic
                    self.functable = True
                elif data == "Related Non-Members":
                    self.bucket = self.globaldefs
                    self.functable = True
                elif data == "Signals":
                    self.bucket = self.signals
                    self.functable = True
                elif data == "Public Slots":
                    self.bucket = self.slots
                    self.functable = True
                elif data == "Properties":
                    self.bucket = self.properties
                elif data == "Macros":
                    self.bucket = self.macros
                    self.functable = True
                elif data == "Public Types":
                    self.intypes = True
                    self.inpubtypes = True
                elif data == "Protected Types":
                    self.intypes = True
                    self.inpubtypes = False
                elif data == "Detailed Description":
                    self.defsover = True
            # Don't add functions that are props: these will get added
            # programatically
            elif self.infunc:  # and not self.funcIsProp:
                self.buffer += data

    def convertFunctions(self):
        F = lambda x: x is not None
        P = lambda x: parseFunction(x, self.qtnamespace)
        self.qtnamespace.publicfuncs = filter(F, map(P, self.public))
        self.qtnamespace.slots = filter(F, map(P, self.slots))
        self.qtnamespace.staticfuncs = filter(F, map(P, self.staticPublic))
        self.qtnamespace.signals = filter(F, map(P, self.signals))
        self.qtnamespace.protectedfuncs = filter(F, map(P, self.protected))
        self.qtnamespace.functions = (
            self.qtnamespace.publicfuncs + self.qtnamespace.protectedfuncs
        )

        rootnamespace.staticfuncs += filter(F, map(P, self.globaldefs))

    def convertProperties(self):
        if self.properties:
            self.qtnamespace.properties = map(parseProperty, self.properties)

    def convertEnums(self):
        for e in self.enums:
            n = e.name
            if n in self.protenums:
                e.protected = True
        self.qtnamespace.enums = self.enums


def inheritCMP(a, b):
    if a.name == "QObject":
        return -1
    else:
        return 1


def outputCMP(a, b):
    ap = isAPrimitiveType(a)
    bp = isAPrimitiveType(b)
    if ap and not bp:
        return -1
    elif bp and not ap:
        return 1
    else:
        return 0


def sortHierarchically(array):
    bases = []
    allclasses = set([])
    for c in array:
        print("c = ", c, c.inherits)
        allclasses.add(c)
        if len(c.inherits) == 0:
            bases.append(c)
    sortedArray = []

    while len(bases) > 0:
        newBaseMap = set([])
        for c in bases:
            sortedArray.append(c)
            for ch in c.inheritedby:
                newBaseMap.add(ch)
        bases = []
        for c in newBaseMap:
            bases.append(c)

    found = set([])
    for c in sortedArray:
        print("c sortedd = ", c)
        found.add(c)

    diff = allclasses - found
    if len(diff) > 0:
        print("ERROR: missing", len(diff), "classes")
        for i in diff:
            print(i.name)

    return sortedArray


##
##


def parseURL(url):
    parser = QtDocParser(url)
    result = parser.result()
    message("INFO: parsed " + result.name)
    api.classes[result.name] = result
    return parser


def recursiveParse(url):
    try:
        parser = parseURL(url)
        for u in parser.childurls:
            # if "qwidget.html" in u:
            absPath = os.path.join(os.path.dirname(url), u)
            recursiveParse(absPath)
    except IOError:
        print("FAILED: (IOError)", url)
    except Exception:
        sys.stdout.flush()
        print("FAILED:", url)
        print(traceback.print_exc())


# ----------------------------------------------------------------------
# parse the args

outputFile = False
outputStdout = False
outputMu = False
outputOtherTypes = False
outputClasses = []
outputModuleParts = False
outputRawClass = False
outputDemoted = False


def setrawclass(x):
    global outputRawClass
    outputRawClass = True


def setout(x):
    global outputFile
    outputFile = True


def setstdout(x):
    global outputStdout
    outputStdout = True


def setmu(x):
    global outputMu
    outputMu = True


def setparts(x):
    global outputModuleParts
    outputModuleParts = True


def setother(x):
    global outputOtherTypes
    outputOtherTypes = True


def setverbose(x):
    global verbose
    verbose = True


def forcerebuild(x):
    global forceParse
    forceParse = True


def setdemoted(x):
    global outputDemoted
    outputDemoted = True


argMap = {
    "-o": setout,
    "-v": setverbose,
    "-f": forcerebuild,
    "-mu": setmu,
    "-module": setparts,
    "-stdout": setstdout,
    "-other": setother,
    "-rawclass": setrawclass,
    "-demoted": setdemoted,
}

del sys.argv[0]


for a in sys.argv:
    if argMap.has_key(a):
        argMap[a](a)
    elif a.startswith("-"):
        print("ERROR: bad arg", a)
        sys.exit(-1)
    else:
        outputClasses.append(a)


def findHTMLinDocTree(rootname):
    for d in [
        "qtcore",
        "qtgui",
        "qtwidgets",
        "qtnetwork",
        "qtopengl",
        "qtconcurrent",
        "qtsvg",
        "qtwebengine",
        "qtwebchannel",
        "qtqml",
        "qtquick",
    ]:
        testname = os.path.join(os.path.join(htmlDir, d), "%s.html" % rootname)
        if os.path.exists(testname):
            return testname
    print("ERROR: failed to find html file for", rootname)
    return None


if not forceParse and os.path.exists(qtapifile):
    message("INFO: loading Qt API")
    api = pickle.load(open(qtapifile))
    api.tmap = translationMap
    api.finish()
    rootnamespace = api.classes[""]
else:
    message("INFO: building Qt API")
    for base in baseHTML:
        htmlfile = findHTMLinDocTree(base)
        recursiveParse(htmlfile)
        api.classes[""] = rootnamespace
    api.finish()
    message("INFO: dumping Qt API")
    pickle.dump(api, open(qtapifile, "w"))

if outputRawClass:
    print("CED RAW CLASS", outputClasses)
    if len(outputClasses):
        for cname in outputClasses:
            api.showClass(cname)
    exit

message("INFO: building Mu API")
muapi = MuAPI(api)

if outputModuleParts:
    out = "QWidgetType* addAllQTSymbols(MuLangContext* c, Module* qt)\n{\n"
    finishedMap = {}
    muOutputClasses = []

    for cname in outputClasses:
        if muapi.classes.has_key(cname):
            c = muapi.classes[cname]
            muOutputClasses.append(c)
            # if c.isBaseClass():
            #    out += c.outputModuleDefinition(finishedMap)

    muOutputClasses = sortHierarchically(muOutputClasses)

    for c in muOutputClasses:
        print("output module definition for {0}".format(c.name))
        out += c.outputModuleDefinition(finishedMap)

    out += "return t_QWidget;\n"
    out += "}\n"
    f = open("qtTypeDefinitions.cpp", "w")
    f.write(out)
    f.close()

    includes = ""
    for cname in outputClasses:
        if muapi.classes.has_key(cname):
            if cname in includeClasses:
                includes += "#include <MuQt6/%sType.h>\n" % cname
    f = open("qtModuleIncludes.h", "w")
    f.write(includes)
    f.close()

if outputDemoted:
    for cname in outputClasses:
        if muapi.classes.has_key(cname):
            c = muapi.classes[cname]
            for i in c.demotedProps:
                print("%s::%s" % (c.name, i[0]))

if len(outputClasses):
    outputClasses.sort(outputCMP)
    for cname in outputClasses:
        if muapi.classes.has_key(cname):
            c = muapi.classes[cname]
            if outputMu:
                c.outputMuDeclarations()
            if outputStdout:
                print(c.outputCompiledNodes())
                print(c.outputNodeImplementations())
                print(c.outputSymbolDeclarations())
                print(c.outputEnumDeclarations())
            if outputFile:
                message("INFO: output files for " + cname)
                c.outputSourceFiles()
        else:
            print("WARNING: can't find class", cname)
else:
    if outputOtherTypes:
        typelist = []
        print("--other types--")
        for t in api.types:
            typelist.append(t)

        typelist.sort()
        pp.pprint(typelist)
