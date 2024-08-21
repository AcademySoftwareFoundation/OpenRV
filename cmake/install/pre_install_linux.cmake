#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

FUNCTION(before_copy_platform FILE_PATH RET_VAL)
  IF(FILE_PATH MATCHES "\\.debug")
    IF(CMAKE_INSTALL_CONFIG_NAME MATCHES "^Release$")
      SET(${RET_VAL}
          "NO"
          PARENT_SCOPE
      )
      RETURN()
    ENDIF()
  ENDIF()

  IF(FILE_PATH MATCHES "${RV_STAGE_LIB_DIR}/libcrypto"
     OR FILE_PATH MATCHES "${RV_STAGE_LIB_DIR}/libssl"
  )
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

SET(STRIP_IGNORE_LIST
  "csv"
  "gzip"
  "json"
  "octet-stream"
  "pdf"
  "x-bytecode.python"
  "x-bzip2"
  "x-dosexec"
  "x-font-ttf"
  "x-tar"
  "zip"
)

FUNCTION(after_copy_platform FILE_PATH FILES_TO_FIX_RPATH)
  IF(CMAKE_INSTALL_CONFIG_NAME MATCHES "^Release$")
    EXECUTE_PROCESS(COMMAND file --mime-type ${FILE_PATH} OUTPUT_VARIABLE FILE_CMD_OUT)
    IF(${FILE_CMD_OUT} MATCHES ": application\/(.+)\n")
      IF(NOT "${CMAKE_MATCH_1}" IN_LIST STRIP_IGNORE_LIST)
        EXECUTE_PROCESS(COMMAND strip -S ${FILE_PATH} RESULT_VARIABLE STRIP_EXIT_CODE)
        IF(NOT STRIP_EXIT_CODE EQUAL 0)
           MESSAGE(WARNING "Unable to strip ${FILE_PATH} with mime type application/${CMAKE_MATCH_1}. Consider adding it to the ignore list.")
        ENDIF()
      ENDIF()
    ENDIF()
  ENDIF()
ENDFUNCTION()

MACRO(post_install_platform)

ENDMACRO()
