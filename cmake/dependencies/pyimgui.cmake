INCLUDE(ProcessorCount) # require CMake 3.15+

RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_PYIMGUI" "" "" "")
RV_SHOW_STANDARD_DEPS_VARIABLES()

# Get the Python executable from the Python::Python target
GET_TARGET_PROPERTY(_python_executable Python::Python IMPORTED_LOCATION)
MESSAGE(STATUS "Python executable 123: ${_python_executable}")

SET(_requirements_install_command
    "${RV_DEPS_BASE_DIR}/RV_DEPS_PYTHON3/install/bin/python${RV_DEPS_PYTHON_VERSION_SHORT}" -m pip install nanobind
)

# Create a stamp file to track nanobind installation
SET(_nanobind_stamp ${CMAKE_CURRENT_BINARY_DIR}/${_target}-nanobind-stamp)
ADD_CUSTOM_COMMAND(
  OUTPUT ${_nanobind_stamp}
  COMMAND ${_requirements_install_command}
  COMMAND ${CMAKE_COMMAND} -E touch ${_nanobind_stamp}
  COMMENT "Installing nanobind and creating stamp file"
  DEPENDS Python::Python
)

# Add a custom target that depends on the stamp file
ADD_CUSTOM_TARGET(
  ${_target}-nanobind-install
  DEPENDS ${_nanobind_stamp}
  COMMENT "Ensuring nanobind is installed for ${_target}"
)


GET_TARGET_PROPERTY(_imgui_include_dirs imgui::imgui INTERFACE_INCLUDE_DIRECTORIES)
GET_TARGET_PROPERTY(_imgui_library_file imgui::imgui LOCATION)

SET(_configure_options
    ""
)

LIST(APPEND _configure_options "-DCMAKE_INSTALL_PREFIX=${_install_dir}")
LIST(APPEND _configure_options "-DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}")
LIST(APPEND _configure_options "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
LIST(APPEND _configure_options "-S ${CMAKE_BINARY_DIR}/RV_DEPS_IMGUI/deps/imgui-bundle/external/imgui")
LIST(APPEND _configure_options "-B ${_build_dir}")


SET(_libname
  "pyimgui${CMAKE_SHARED_MODULE_SUFFIX}"
)

IF(RV_TARGET_LINUX)
  # Override the library name for Linux for now because our CMakelists.txt install it in lib for all platform.
  SET(_lib_dir
    ${_install_dir}/lib
  )
  SET(_libpath
    ${_lib_dir}/${_libname}
  )
ENDIF()

SET(_PYTHON_LIB_DIR
    "python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}"
)

EXTERNALPROJECT_ADD(
  ${_target}
  GIT_REPOSITORY "https://github.com/pthom/imgui_bundle.git"
  GIT_TAG "76cbc64953858ff2a8699aa1f7e8c7cb77e3b17a"
  DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  SOURCE_DIR ${CMAKE_BINARY_DIR}/RV_DEPS_IMGUI/deps/imgui-bundle
  GIT_SUBMODULES "" GIT_SUBMODULES_RECURSE 0
  PATCH_COMMAND
    ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/imgui/CMakeLists_PyImGUI.cmake
    ${CMAKE_BINARY_DIR}/RV_DEPS_IMGUI/deps/imgui-bundle/external/imgui/CMakeLists.txt && ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/pybind_imgui_module.cpp ${CMAKE_BINARY_DIR}/RV_DEPS_IMGUI/deps/imgui-bundle/external/imgui/bindings
  UPDATE_COMMAND ""
  CONFIGURE_COMMAND ${CMAKE_COMMAND} ${_configure_options} -DCMAKE_PREFIX_PATH=$ENV{QT_HOME}/lib/cmake -DPython_ROOT=${RV_DEPS_BASE_DIR}/RV_DEPS_PYTHON3/install
                    -Dimgui_INCLUDE_DIRS=${_imgui_include_dirs} -Dimgui_LIBRARY=${_imgui_library_file}
  BUILD_COMMAND ${_cmake_build_command}
  INSTALL_COMMAND ${_cmake_install_command}
  BUILD_BYPRODUCTS ${_libpath}
  BUILD_ALWAYS FALSE
  BUILD_IN_SOURCE TRUE
  USES_TERMINAL_DOWNLOAD TRUE
  DEPENDS RV_DEPS_IMGUI Python::Python imgui::imgui
)

# Not using RV_COPY_LIB_BIN_FOLDERS() because we need to copy the library to a specific location.
ADD_CUSTOM_COMMAND(
  COMMENT "Installing ${_target}'s libs into ${_PYTHON_LIB_DIR}"
  OUTPUT ${RV_STAGE_LIB_DIR}/${_libname}
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${_lib_dir} ${RV_STAGE_LIB_DIR}/${_PYTHON_LIB_DIR}
  DEPENDS ${_target}
)
ADD_CUSTOM_TARGET(
  ${_target}-stage-target ALL
  DEPENDS ${RV_STAGE_LIB_DIR}/${_libname}
)

ADD_DEPENDENCIES(dependencies ${_target}-stage-target)
