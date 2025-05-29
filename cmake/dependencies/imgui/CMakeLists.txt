# imgui/CMakeLists.txt
CMAKE_MINIMUM_REQUIRED(VERSION 3.27)
PROJECT(imgui)

SET(IMGUI_SOURCES
    imgui.cpp
    imgui_draw.cpp
    imgui_tables.cpp
    imgui_widgets.cpp
    imgui_demo.cpp # for testing
    misc/cpp/imgui_stdlib.cpp
    backends/imgui_impl_opengl2.cpp
    backends/imgui_impl_qt.cpp
    # Add/remove files as needed
)

SET(IMGUI_HEADERS
    imgui.h
    backends/imgui_impl_opengl2.h
    backends/imgui_impl_qt.hpp
    misc/cpp/imgui_stdlib.h
    imconfig.h
    imstb_rectpack.h
    imstb_textedit.h
    imstb_truetype.h
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

FIND_PACKAGE(OpenGL REQUIRED)

FIND_PACKAGE(
  Qt6
  COMPONENTS Core Gui OpenGL Widgets
  REQUIRED
)

ADD_LIBRARY(
  imgui SHARED
  ${IMGUI_SOURCES} ${IMPLOT_SOURCES}
)

TARGET_COMPILE_FEATURES(
  imgui
  PUBLIC cxx_std_11
)
TARGET_LINK_LIBRARIES(
  imgui
  PRIVATE OpenGL::GL OpenGL::GLU Qt::Core Qt::Gui Qt::Widgets Qt::OpenGL
)
TARGET_INCLUDE_DIRECTORIES(
  imgui
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

INSTALL(
  TARGETS imgui
  LIBRARY DESTINATION lib
)
INSTALL(
  FILES ${IMGUI_HEADERS} ${IMPLOT_HEADERS}
  DESTINATION include
)
