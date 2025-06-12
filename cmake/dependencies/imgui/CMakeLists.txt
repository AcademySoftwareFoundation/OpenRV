# imgui/CMakeLists.txt
CMAKE_MINIMUM_REQUIRED(VERSION 3.27)
PROJECT(imgui)

SET(IMGUI_SOURCES
    imgui.cpp
    imgui_draw.cpp
    imgui_tables.cpp
    imgui_widgets.cpp
    imgui_stacklayout.cpp
    imgui_demo.cpp # for testing
    backends/imgui_impl_opengl2.cpp
    backends/imgui_impl_qt.cpp
    # Add/remove files as needed
)

SET(IMGUI_MISC_SOURCES
    misc/cpp/imgui_stdlib.cpp
    # Add/remove files as needed
)

SET(IMGUI_HEADERS
    imgui.h
    backends/imgui_impl_opengl2.h
    backends/imgui_impl_qt.hpp
    imgui_internal.h
    imgui_stacklayout_internal.h
    imgui_stacklayout.h
    imconfig.h
    imstb_rectpack.h
    imstb_textedit.h
    imstb_truetype.h
    # Add/remove headers as needed
)

SET(IMGUI_MISC_HEADERS
    misc/cpp/imgui_stdlib.h
    # Add/remove headers as needed
)

SET(IMPLOT_SOURCES
    implot/implot.cpp implot/implot_items.cpp implot/implot_demo.cpp
    # Add/remove files as needed
)

SET(IMPLOT_HEADERS
    implot/implot.h implot/implot_internal.h
    # Add/remove headers as needed
)

FILE(GLOB IMGUI_NODE_EDITOR_SOURCES imgui-node-editor/*.cpp)
FILE(GLOB IMGUI_NODE_EDITOR_HEADERS imgui-node-editor/*.h)

FIND_PACKAGE(OpenGL REQUIRED)

FIND_PACKAGE(
  ${FIND_QT_VERSION}
  COMPONENTS Core Gui OpenGL Widgets
  REQUIRED
)

ADD_LIBRARY(
  imgui SHARED
  ${IMGUI_SOURCES} ${IMGUI_MISC_SOURCES} ${IMPLOT_SOURCES} ${IMGUI_NODE_EDITOR_SOURCES}
)

TARGET_COMPILE_FEATURES(
  imgui
  PUBLIC cxx_std_11
)
# Add this line to define IMGUI_BUNDLE_PYTHON_API for the imgui target
TARGET_COMPILE_DEFINITIONS(
  imgui
  PUBLIC IMGUI_BUNDLE_PYTHON_API
)

TARGET_COMPILE_DEFINITIONS(
  imgui
  PUBLIC
    "IMPLOT_CUSTOM_NUMERIC_TYPES=(signed char)(unsigned char)(signed short)(unsigned short)(signed int)(unsigned int)(signed long)(unsigned long)(signed long long)(unsigned long long)(float)(double)(long double)"
)

TARGET_LINK_LIBRARIES(
  imgui
  PRIVATE OpenGL::GL OpenGL::GLU Qt::Core Qt::Gui Qt::Widgets Qt::OpenGL
)
TARGET_INCLUDE_DIRECTORIES(
  imgui
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/implot ${CMAKE_CURRENT_SOURCE_DIR}/imgui-node-editor ${CMAKE_CURRENT_SOURCE_DIR}/backends
)

INSTALL(
  TARGETS imgui
  LIBRARY DESTINATION lib
)
INSTALL(
  FILES ${IMGUI_HEADERS} ${IMGUI_NODE_EDITOR_HEADERS}
  DESTINATION include
)
INSTALL(
  FILES ${IMGUI_MISC_HEADERS}
  DESTINATION include/misc/cpp
)
INSTALL(
  FILES ${IMPLOT_HEADERS}
  DESTINATION include/implot
)
