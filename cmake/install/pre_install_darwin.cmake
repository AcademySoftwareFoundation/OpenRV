#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

FUNCTION(before_copy_platform FILE_PATH RET_VAL)
  IF(FILE_PATH MATCHES "\\.dSYM")
    IF(CMAKE_INSTALL_CONFIG_NAME MATCHES "^Release$")
      SET(${RET_VAL}
          "NO"
          PARENT_SCOPE
      )
      RETURN()
    ENDIF()
  ENDIF()

  IF(FILE_PATH MATCHES "\\.DS_Store")
    SET(${RET_VAL}
        "NO"
        PARENT_SCOPE
    )
    RETURN()
  ENDIF()

  SET(${RET_VAL}
      "YES"
      PARENT_SCOPE
  )
  RETURN()
ENDFUNCTION()

# It's really slow to try to fix the rpaths on all the files. This list contains extensions that are known to not be mach-o
SET(KNOWN_EXTENSIONS_TO_SKIP
    ".h"
    ".c"
    ".C"
    ".js"
    ".mu"
    ".rvpkg"
    ".glsl"
    ".hpp"
    ".cpp"
    ".json"
    ".xml"
    ".py"
    ".pyc"
    ".pyi"
    ".pxd"
    ".pyproject"
    ".txt"
    ".pak"
    ".qm"
    ".qmltypes"
    ".plist"
    ".png"
    ".jpg"
    ".jpeg"
    ".aifc"
    ".aiff"
    ".au"
    ".wav"
    ".decTest"
    ".qml"
    ".qmlc"
    ".metainfo"
    ".mesh"
    ".init"
    ".zip"
    ".md"
)

# Using MACRO because it need to able to set a variable outside of its own scope.
MACRO(after_copy_platform FILE_PATH FILES_TO_FIX_RPATH)
  IF(NOT IS_SYMLINK ${FILE_PATH})
    GET_FILENAME_COMPONENT(FILE_EXT ${FILE_PATH} LAST_EXT)
    IF(NOT FILE_EXT IN_LIST KNOWN_EXTENSIONS_TO_SKIP)
      LIST(APPEND FILES_TO_FIX_RPATH "${FILE_PATH}\n")
    ENDIF()
  ELSE()
    MESSAGE(STATUS "\tSkipping RPATH cleanup on symlinks ${FILE_PATH}")
  ENDIF()
ENDMACRO()

MACRO(post_install_platform)
  SET(_output_file_to_fix_
      "${RV_DEPS_BASE_DIR}/files_to_fix.txt"
  )
  FILE(
    WRITE ${_output_file_to_fix_}
    ${FILES_TO_FIX_RPATH}
  )

  MESSAGE(STATUS "Fixing rpaths: python3 ${CMAKE_CURRENT_LIST_DIR}/../../src/build/remove_absolute_rpath.py --files-list ${_output_file_to_fix_} --root ${CMAKE_SOURCE_DIR}")
  EXECUTE_PROCESS(
    COMMAND python3 ${CMAKE_CURRENT_LIST_DIR}/../../src/build/remove_absolute_rpath.py --files-list ${_output_file_to_fix_} --root ${CMAKE_SOURCE_DIR}
            COMMAND_ERROR_IS_FATAL ANY
  )

  # Ad-hoc codesign all Mach-O binaries. install_name_tool modifications above invalidate any existing signatures.
  MESSAGE(STATUS "Ad-hoc codesigning installed binaries...")
  FILE(STRINGS ${_output_file_to_fix_} _files_to_sign)
  FOREACH(_file_to_sign ${_files_to_sign})
    STRING(STRIP "${_file_to_sign}" _file_to_sign)
    IF(_file_to_sign AND EXISTS "${_file_to_sign}")
      EXECUTE_PROCESS(
        COMMAND file -bh "${_file_to_sign}"
        OUTPUT_VARIABLE _file_type
        OUTPUT_STRIP_TRAILING_WHITESPACE
      )
      IF(_file_type MATCHES "^Mach-O")
        EXECUTE_PROCESS(
          COMMAND codesign --force --sign - "${_file_to_sign}"
          RESULT_VARIABLE _codesign_result
        )
        IF(NOT _codesign_result EQUAL 0)
          MESSAGE(WARNING "Failed to codesign ${_file_to_sign}")
        ENDIF()
      ENDIF()
    ENDIF()
  ENDFOREACH()
  MESSAGE(STATUS "Ad-hoc codesigning complete.")
ENDMACRO()
