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

# It's really slow to try to fix the rpaths on all the files. 
# This list contains extensions that are known to not be mach-o
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
  set(_output_file_to_fix_ "${RV_DEPS_BASE_DIR}/files_to_fix.txt")
  FILE(WRITE ${_output_file_to_fix_} ${FILES_TO_FIX_RPATH})

  MESSAGE(STATUS "python3 ${CMAKE_CURRENT_LIST_DIR}/../../src/build/remove_absolute_rpath.py --files-list ${_output_file_to_fix_} --root ${CMAKE_SOURCE_DIR}")
  EXECUTE_PROCESS(
    COMMAND python3 ${CMAKE_CURRENT_LIST_DIR}/../../src/build/remove_absolute_rpath.py 
            --files-list ${_output_file_to_fix_} 
            --root ${CMAKE_SOURCE_DIR} 
            COMMAND_ERROR_IS_FATAL ANY
  )
ENDMACRO()
