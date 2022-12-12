#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "mu-interp"
)

LIST(APPEND _sources main.cpp)

ADD_EXECUTABLE(
  ${_target}
  ${_sources}
)

TARGET_INCLUDE_DIRECTORIES(
  ${_target}
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
)

TARGET_LINK_LIBRARIES(
  ${_target}
  PRIVATE arg Mu MuLang stl_ext
)

IF(RV_TARGET_DARWIN)
  TARGET_COMPILE_OPTIONS(
    ${_target}
    PRIVATE "-DMU_USE_READLINE"
  )

  TARGET_LINK_LIBRARIES(
    ${_target}
    PRIVATE readline BDWGC::Gc
  )
ELSEIF(RV_TARGET_LINUX)
  TARGET_COMPILE_OPTIONS(
    ${_target}
    PRIVATE "-DMU_USE_READLINE"
  )

  TARGET_LINK_LIBRARIES(
    ${_target}
    PRIVATE readline BDWGC::Gc
  )
ELSEIF(RV_TARGET_WINDOWS)
  TARGET_COMPILE_OPTIONS(
    ${_target}
    PRIVATE "-DLINKED_MODULES"
  )

  TARGET_LINK_LIBRARIES(
    ${_target}
    PRIVATE MuAutoDoc
            MuSystem
            MuIO
            MuImage
            MuEncoding
            MuQt5
            gc
  )
ENDIF()

RV_STAGE(TYPE "EXECUTABLE_WITH_PLUGINS" TARGET ${_target})

ADD_TEST(
  NAME "mu-interp"
  COMMAND ${CMAKE_COMMAND} -E env QT_QPA_PLATFORM=minimal "$<TARGET_FILE:${_target}>" -help
)

FILE(GLOB mu_test_files test/*.mu)

# Drop any mu script in that
FOREACH(
  _mu_test_file
  ${mu_test_files}
)
  GET_FILENAME_COMPONENT(mu_test_file ${_mu_test_file} ABSOLUTE)
  GET_FILENAME_COMPONENT(mu_test_file_name ${_mu_test_file} NAME)

  SET(_test_name
      "mu-interp ${mu_test_file_name}"
  )

  SET(SKIP_THIS_MU_TEST
      FALSE
  )

  # TODO: This test currently fails on Windows+Debug
  IF(RV_TARGET_WINDOWS
     AND ${CMAKE_BUILD_TYPE} STREQUAL "Debug"
  )
    IF(mu_test_file_name STREQUAL "case.mu")
      MESSAGE(STATUS "Skipping ${mu_test_file_name} on Windows")
      SET(SKIP_THIS_MU_TEST
          TRUE
      )
    ENDIF()
  ENDIF()

  IF(NOT SKIP_THIS_MU_TEST)
    IF(RV_TARGET_WINDOWS)
      # The Windows version of Mu environment loader uses ';' to split entries. Most likely to cope with 'C:\' like syntax. See
      # 'src\lib\mu\Mu\Environment.cpp:init' static method. Then, on top of that, we can't easily use the ';' character with CMake command as it gets
      # interpreted to split statements. We then resolve to a plain 'bash -c' invocation.
      SET(_mu_module_path
          "\"${CMAKE_CURRENT_SOURCE_DIR}/test;${RV_STAGE_PLUGINS_MU_DIR}\""
      )
      ADD_TEST(
        NAME ${_test_name}
        COMMAND bash -c "export MU_MODULE_PATH=\"${_mu_module_path}\";QT_QPA_PLATFORM=minimal; \"$<TARGET_FILE:${_target}>\" -main ${mu_test_file}"
      )
    ELSE()
      # macOS & Linux
      ADD_TEST(
        NAME ${_test_name}
        COMMAND ${CMAKE_COMMAND} -E env MU_MODULE_PATH=${CMAKE_CURRENT_SOURCE_DIR}/test:${RV_STAGE_PLUGINS_MU_DIR} QT_QPA_PLATFORM=minimal
                "$<TARGET_FILE:${_target}>" -main ${mu_test_file}
      )
    ENDIF()
  ENDIF()

ENDFOREACH()
