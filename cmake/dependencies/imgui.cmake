INCLUDE(ProcessorCount) # require CMake 3.15+

RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_IMGUI" "1.91.9" "" "")
RV_SHOW_STANDARD_DEPS_VARIABLES()

SET(_imgui_download_url
    "https://github.com/ocornut/imgui/archive/refs/tags/v${_version}.zip"
)

# Hashes for verification (replace with actual hash values)
SET(_imgui_download_hash
    "909ebf627ea5c7298c9f4d1f589c77a0"
)

# There is no version suffix for imgui library name.
RV_MAKE_STANDARD_LIB_NAME("imgui" "" "SHARED" "")

SET(_install_dir
    ${RV_DEPS_BASE_DIR}/${_target}/install
)

IF(RV_TARGET_WINDOWS)
  SET(_imgui_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}imgui${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
ELSE()
  SET(_imgui_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}imgui${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
ENDIF()

SET(_imgui_lib
    ${_lib_dir}/${_imgui_name}
)

SET(_lib_dir
    ${_install_dir}/lib
)

LIST(APPEND _imgui_byproducts ${_imgui_lib})

# Download implot into a separate directory
EXTERNALPROJECT_ADD(
  implot_download
  GIT_REPOSITORY "https://github.com/epezent/implot.git"
  GIT_TAG "master"
  DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  SOURCE_DIR ${CMAKE_BINARY_DIR}/${_target}/deps/implot
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  BUILD_IN_SOURCE TRUE
  USES_TERMINAL_DOWNLOAD TRUE
)

# Download implot into a separate directory
EXTERNALPROJECT_ADD(
  imgui_backend_qt_download
  GIT_REPOSITORY "https://github.com/dpaulat/imgui-backend-qt.git"
  GIT_TAG "main"
  DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  SOURCE_DIR ${CMAKE_BINARY_DIR}/${_target}/deps/imgui-backend-qt
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  BUILD_IN_SOURCE TRUE
  USES_TERMINAL_DOWNLOAD TRUE
)

MESSAGE(STATUS "Using Qt6 from: ${RV_DEPS_QT_LOCATION}")

EXTERNALPROJECT_ADD(
  ${_target}
  URL ${_imgui_download_url}
  URL_MD5 ${_imgui_download_hash}
  DOWNLOAD_NAME ${_target}_${_version}.zip
  DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  SOURCE_DIR ${CMAKE_BINARY_DIR}/${_target}/src
  # Copy the custom CMakeLists.txt for imgui and copy the source files from implot to imgui source directory.
  PATCH_COMMAND
    ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/imgui/CMakeLists.txt ${CMAKE_BINARY_DIR}/${_target}/src/CMakeLists.txt && ${CMAKE_COMMAND} -E
    copy_directory ${CMAKE_BINARY_DIR}/${_target}/deps/implot ${CMAKE_BINARY_DIR}/${_target}/src/implot && ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_BINARY_DIR}/${_target}/deps/imgui-backend-qt/backends ${CMAKE_BINARY_DIR}/${_target}/src/backends
  CONFIGURE_COMMAND ${CMAKE_COMMAND} ${_configure_options} -DCMAKE_PREFIX_PATH=$ENV{QT_HOME}/lib/cmake
  BUILD_COMMAND ${_cmake_build_command}
  INSTALL_COMMAND ${_cmake_install_command}
  BUILD_BYPRODUCTS ${_imgui_byproducts}
  BUILD_IN_SOURCE TRUE
  USES_TERMINAL_BUILD TRUE
  DEPENDS implot_download imgui_backend_qt_download
)

RV_COPY_LIB_BIN_FOLDERS()

ADD_LIBRARY(imgui::imgui SHARED IMPORTED GLOBAL)
ADD_DEPENDENCIES(dependencies ${_target}-stage-target)
ADD_DEPENDENCIES(imgui::imgui ${_target})

SET_PROPERTY(
  TARGET imgui::imgui
  PROPERTY IMPORTED_LOCATION ${_libpath}
)

SET_PROPERTY(
  TARGET imgui::imgui
  PROPERTY IMPORTED_SONAME ${_libname}
)

FILE(MAKE_DIRECTORY "${_include_dir}")
TARGET_INCLUDE_DIRECTORIES(
  imgui::imgui
  INTERFACE ${_include_dir}
)

LIST(APPEND RV_DEPS_LIST imgui::imgui)
