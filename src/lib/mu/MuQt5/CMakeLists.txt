#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)
INCLUDE(rv_sed)

FIND_PACKAGE(
  Qt5
  COMPONENTS Core
             Gui
             Network
             OpenGL
             Qml
             Quick
             QuickWidgets
             Svg
             Test
             UiTools
             WebChannel
             WebEngine
             WebEngineCore
             WebEngineWidgets
             Widgets
             Xml
             XmlPatterns
  REQUIRED
)

SET(CMAKE_AUTOUIC
    ON
)
SET(CMAKE_AUTOMOC
    ON
)
SET(CMAKE_AUTORCC
    ON
)

SET(_target
    "MuQt5"
)

SET(_sources
    qtModule.cpp
    Bridge.cpp
    qtUtils.cpp
    qtGlobals.cpp
    SignalSpy.cpp
    RvNetworkAccessManager.cpp
    QtColorTriangleType.cpp
)

SET(_autotypes
    Color
    Size
    Point
    PointF
    Rect
    RectF
    Font
    KeySequence
    Icon
    Pixmap
    Bitmap
    Region
    Variant
    Url
    UrlQuery
    DateTime
    Time
    Date
    ByteArray
    ModelIndex
    Object
    LayoutItem
    PaintDevice
    Image
    RegExp
    TextCursor
    TextOption
    TextBlock
    Widget
    Layout
    Margins
    Clipboard
    BoxLayout
    FormLayout
    GridLayout
    StackedLayout
    HBoxLayout
    VBoxLayout
    DockWidget
    ToolBar
    MainWindow
    Action
    ActionGroup
    Slider
    Dial
    LineEdit
    Frame
    AbstractSlider
    AbstractButton
    CheckBox
    PushButton
    RadioButton
    ToolButton
    ButtonGroup
    Menu
    ComboBox
    Completer
    SvgWidget
    FileDialog
    AbstractItemModel
    AbstractListModel
    AbstractTableModel
    StandardItemModel
    AbstractItemView
    ColorDialog
    Dialog
    StandardItem
    ListWidgetItem
    TreeWidgetItem
    TableWidgetItem
    ColumnView
    HeaderView
    ListView
    TableView
    TreeView
    ListWidget
    TreeWidget
    TableWidget
    GuiApplication
    CoreApplication
    Application
    Label
    MenuBar
    PlainTextEdit
    ProgressBar
    ScrollArea
    SpinBox
    Splitter
    StackedWidget
    StatusBar
    TabBar
    TabWidget
    TextDocument
    TextEdit
    ToolBox
    WidgetAction
    AbstractSpinBox
    AbstractScrollArea
    InputDialog
    EventLoop
    GroupBox
    NetworkReply
    Timer
    IODevice
    FileDevice
    Process
    ProcessEnvironment
    File
    FileInfo
    Dir
    AbstractSocket
    TcpSocket
    UdpSocket
    TcpServer
    HostAddress
    HostInfo
    LocalSocket
    DesktopWidget
    ItemSelectionRange
    ItemSelection
    ItemSelectionModel
    Gradient
    Brush
    Matrix
    Transform
    ConicalGradient
    LinearGradient
    RadialGradient
    Palette
    SpacerItem
    WidgetItem
    MimeData
    Event
    ResizeEvent
    DragLeaveEvent
    DragMoveEvent
    DropEvent
    InputEvent
    MoveEvent
    PaintEvent
    ShortcutEvent
    ShowEvent
    TimerEvent
    FocusEvent
    GestureEvent
    FileOpenEvent
    CloseEvent
    DragEnterEvent
    HideEvent
    HoverEvent
    HelpEvent
    ContextMenuEvent
    KeyEvent
    MouseEvent
    TabletEvent
    WheelEvent
    WindowStateChangeEvent
    TextStream
    TextCodec
    TextBrowser
    NetworkCookieJar
    NetworkCookie
    NetworkAccessManager
    Cursor
    JSEngine
    QmlEngine
    QmlApplicationEngine
    QmlContext
    QuickWidget
    QuickItem
    WebEngineCookieStore
    WebEngineProfile
    WebEngineView
    WebEngineSettings
    WebEnginePage
    WebEngineHistory
    WebChannel
    #
    # NetworkAccessManager has been modified to us an RVNetworkAccessManager instead of a QNetworkAccessManager, so if the generation script is run again the
    # files QNetworkAccessManagerType.* must be modified accordingly.
    #
    # Painter PainterPath
)
FOREACH(
  _entry
  ${_autotypes}
)
  LIST(APPEND _sources "Q${_entry}Type.cpp")
ENDFOREACH()

FILE(
  GLOB _files_to_moc
  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  MuQt5/*.h
)

LIST(REMOVE_ITEM _files_to_moc SignalSpy.h)
FOREACH(
  file_to_moc
  ${_files_to_moc}
)
  GET_FILENAME_COMPONENT(outfile ${file_to_moc} NAME_WE)
  GET_FILENAME_COMPONENT(infile ${file_to_moc} ABSOLUTE)
  FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${_target}/generated)
  SET(outfile
      ${CMAKE_CURRENT_SOURCE_DIR}/${_target}/generated/moc_${outfile}.cpp
  )
  ADD_CUSTOM_COMMAND(
    OUTPUT ${outfile}
    COMMAND ${Qt5Core_MOC_EXECUTABLE} -I ${CMAKE_CURRENT_SOURCE_DIR} -o ${outfile} ${infile}
    MAIN_DEPENDENCY ${infile}
    VERBATIM
  )
  SET_SOURCE_FILES_PROPERTIES(
    ${infile}
    PROPERTIES SKIP_AUTOMOC ON
               SKIP_AUTOUIC ON
               SKIP_AUTORCC ON
  )
  SET_SOURCE_FILES_PROPERTIES(
    ${outfile}
    PROPERTIES SKIP_AUTOMOC ON
               SKIP_AUTOUIC ON
               SKIP_AUTORCC ON
  )

  LIST(APPEND _sources ${outfile})
ENDFOREACH()

ADD_LIBRARY(
  ${_target} STATIC
  ${_sources} ${CMAKE_CURRENT_SOURCE_DIR}/${_target}/generated/moc_SignalSpy_filtered.hpp
)

#
# Note: Defining WIN32_LEAN_AND_MEAN to prevent a clash between winsock.h and winsock2.h: windows.h, which is included indirectly by <QtCore/QtCore>, includes
# winsock.h by default (if WIN32_LEAN_AND_MEAN is not defined). And then winsock2.h is later included by SignalSpy.h. Any time winsock.h gets included before
# winsock2.h, there will be compiler errors. The reason is because the two files DO NOT co-exist very well. winsock2.h was designed to replace winsock.h, not
# extend it. Everything that is defined in winsock.h is also defined in winsock2.h.
IF(RV_TARGET_WINDOWS)
  TARGET_COMPILE_OPTIONS(
    ${_target}
    PRIVATE "-DWIN32_LEAN_AND_MEAN"
  )
ENDIF()

#
# We have a few tweaks to the generated SignalSpy moc file and therfore we'll be commiting the 'tweaked version' since we basically want to override the
# 'SignalSpy::qt_metacall' method with the one that connects Qt & the Mu language in our SignalSpy.cpp source file.
SET_PROPERTY(
  SOURCE SignalSpy.h
  PROPERTY SKIP_AUTOGEN ON
)

ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/${_target}/generated/moc_SignalSpy.cpp
  COMMAND ${RV_DEPS_QT5_LOCATION}/bin/moc -I ${CMAKE_CURRENT_SOURCE_DIR} -o ${CMAKE_CURRENT_SOURCE_DIR}/${_target}/generated/moc_SignalSpy.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/${_target}/SignalSpy.h
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/SignalSpy.cpp
  COMMENT "Generating SignalSpy moc file ..."
  VERBATIM
)

ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/${_target}/generated/moc_SignalSpy_filtered.hpp
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${_target}/generated/moc_SignalSpy.cpp
  COMMAND
    bash -c
    "sed -f ${CMAKE_CURRENT_SOURCE_DIR}/signalspy.sed ${CMAKE_CURRENT_SOURCE_DIR}/${_target}/generated/moc_SignalSpy.cpp >${CMAKE_CURRENT_SOURCE_DIR}/${_target}/generated/moc_SignalSpy_filtered.hpp"
  COMMENT "Patching the moc_SignalSpy.cpp file ..."
)

FIND_PACKAGE(
  Qt5
  COMPONENTS Core
             Gui
             Network
             OpenGL
             Qml
             Quick
             QuickWidgets
             Svg
             Test
             UiTools
             WebChannel
             WebEngine
             WebEngineCore
             WebEngineWidgets
             Widgets
             Xml
             XmlPatterns
  REQUIRED
)
TARGET_INCLUDE_DIRECTORIES(
  ${_target}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
)

TARGET_LINK_LIBRARIES(
  ${_target}
  PUBLIC Qt5::Core
         Qt5::Gui
         Qt5::Network
         Qt5::OpenGL
         Qt5::Qml
         Qt5::Quick
         Qt5::QuickWidgets
         Qt5::Svg
         Qt5::Test
         Qt5::WebChannel
         Qt5::WebEngine
         Qt5::WebEngineCore
         Qt5::WebEngineWidgets
         Qt5::Widgets
         Qt5::Xml
         Qt5::XmlPatterns
         qtcolortriangle
  PRIVATE Mu MuLang Qt5::UiTools TwkQtCoreUtil TwkUtil
)

IF(RV_TARGET_WINDOWS)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PUBLIC gc
  )
ELSE()
  TARGET_LINK_LIBRARIES(
    ${_target}
    PUBLIC BDWGC::Gc
  )
ENDIF()

RV_STAGE(TYPE "LIBRARY" TARGET ${_target})
