#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "rvpkg"
)

LIST(APPEND _sources main.cpp utf8Main.cpp)

ADD_EXECUTABLE(
  ${_target}
  ${_sources}
)

FIND_PACKAGE(
  ${RV_QT_PACKAGE_NAME}
  COMPONENTS Core
  REQUIRED
)

TARGET_INCLUDE_DIRECTORIES(
  ${_target}
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
)

TARGET_LINK_LIBRARIES(
  ${_target}
  PRIVATE TwkApp
          TwkDeploy
          TwkExc
          TwkUtil
          stl_ext
          RvPackage
          arg
          stl_ext
          Qt::Core
)

IF(RV_TARGET_DARWIN)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PRIVATE DarwinBundle
  )
ELSE()
  TARGET_LINK_LIBRARIES(
    ${_target}
    PRIVATE QTBundle
  )
ENDIF()

IF(RV_TARGET_WINDOWS)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PRIVATE win_posix win_pthreads
  )
ENDIF()

IF(RV_TARGET_WINDOWS)
  ADD_LINK_OPTIONS("-SUBSYSTEM:CONSOLE")
ENDIF()

TARGET_COMPILE_OPTIONS(
  ${_target}
  PRIVATE -DGIT_HEAD=\"${RV_GIT_COMMIT_SHORT_HASH}\" -DRELEASE_DESCRIPTION=\"${RV_RELEASE_DESCRIPTION}\"
)

RV_STAGE(TYPE "EXECUTABLE" TARGET ${_target})

ADD_CUSTOM_COMMAND(
  COMMENT "Installing ${RV_UI_APPLICATION_NAME} Packages"
  OUTPUT ${RV_STAGE_PLUGINS_PACKAGES_DIR}/rvinstall
  COMMAND ${RV_DEPS_PYTHON3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/src/build/install_all_rvpkg.py --rvpkg $<TARGET_FILE:${_target}> --destination
          ${RV_STAGE_PLUGINS_DIR} --source ${RV_PACKAGES_DIR}
  WORKING_DIRECTORY ${RV_STAGE_BIN_DIR}
  DEPENDS ${_target} packages ${RV_PACKAGE_LIST}
)

ADD_CUSTOM_TARGET(
  rvinstall ALL
  DEPENDS ${RV_STAGE_PLUGINS_PACKAGES_DIR}/rvinstall
)

ADD_DEPENDENCIES(installed_packages rvinstall)
