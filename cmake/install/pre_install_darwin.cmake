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

FUNCTION(after_copy_platform FILE_PATH)

  IF(IS_SYMLINK ${FILE_PATH})
    MESSAGE(STATUS "\tSkipping RPATH cleanup on symlinks")
    RETURN()
  ENDIF()

  GET_FILENAME_COMPONENT(FILE_EXT ${FILE_PATH} LAST_EXT)
  IF(FILE_EXT IN_LIST KNOWN_EXTENSIONS_TO_SKIP)
    MESSAGE(STATUS "\tSkipping RPATH cleanup due to file extension")
    RETURN()
  ENDIF()

  EXECUTE_PROCESS(COMMAND python3 ${PROJECT_SOURCE_DIR}/src/build/remove_absolute_rpath.py --target ${FILE_PATH} COMMAND_ERROR_IS_FATAL ANY)

ENDFUNCTION()
