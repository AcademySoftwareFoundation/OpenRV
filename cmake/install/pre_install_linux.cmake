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

# Resolved at file scope (not inside the function) so it does not depend on which listfile is executing when after_copy_platform() is called.
SET(RV_STRIP_DEBUG_SAFE_SCRIPT
    "${CMAKE_CURRENT_LIST_DIR}/../scripts/strip_debug_safe.sh"
)

FUNCTION(after_copy_platform FILE_PATH FILES_TO_FIX_RPATH)
  IF(CMAKE_INSTALL_CONFIG_NAME MATCHES "^Release$")
    EXECUTE_PROCESS(
      COMMAND file --mime-type ${FILE_PATH}
      OUTPUT_VARIABLE FILE_CMD_OUT
    )
    IF(${FILE_CMD_OUT} MATCHES ": application\/(.+)\n")
      IF(NOT "${CMAKE_MATCH_1}" IN_LIST STRIP_IGNORE_LIST)
        # Saved before any later MATCHES below, which would overwrite CMAKE_MATCH_1 and put the wrong mime type in the warning message.
        SET(_rv_mime_subtype
            "${CMAKE_MATCH_1}"
        )
        # strip_debug_safe.sh (not a bare `strip -S`) guards against a GNU strip bug that corrupts binaries whose layout makes strip emit "allocated section
        # `.dynstr' not in segment": strip relocates .dynstr to the end of the file while leaving its virtual address inside a PT_LOAD segment that no longer
        # covers it, so at runtime the whole dynamic string table reads as zeros. The binary then fails to load with exit 127. strip exits 0 while doing this,
        # so the previous RESULT_VARIABLE check could not catch it.
        #
        # This mattered in practice: `strip -S` here silently destroyed bin/crashpad_handler in the packaged Linux build, which is why RV reported "Failed to
        # start Crashpad handler" / "Failed to initialize crash handler" from installed packages while the same build ran fine out of the stage tree (the stage
        # tree never goes through install). PySide6's lupdate is affected the same way. Both keep their debug info now; the size cost is negligible because such
        # binaries carry little or no DWARF. See also cmake/macros/rv_stage.cmake, which uses the same guard for RV's own targets.
        EXECUTE_PROCESS(
          COMMAND bash ${RV_STRIP_DEBUG_SAFE_SCRIPT} ${FILE_PATH}
          RESULT_VARIABLE STRIP_EXIT_CODE
          ERROR_VARIABLE STRIP_ERROR_OUT
        )
        # The guard always exits 0, so the file-was-left-alone signal is its stderr, not the exit code. Report the known-benign ".dynstr not in segment" skip at
        # STATUS (expected, by design, nothing to action), and keep everything else at WARNING so a genuine strip failure stays as visible as it was before.
        IF(STRIP_EXIT_CODE EQUAL 0
           AND STRIP_ERROR_OUT MATCHES "not in segment"
        )
          MESSAGE(STATUS "${STRIP_ERROR_OUT}")
        ELSEIF(
          NOT STRIP_EXIT_CODE EQUAL 0
          OR STRIP_ERROR_OUT
        )
          MESSAGE(WARNING "Unable to strip ${FILE_PATH} with mime type application/${_rv_mime_subtype}. Consider adding it to the ignore list.")
        ENDIF()
      ENDIF()
    ENDIF()
  ENDIF()
ENDFUNCTION()

MACRO(post_install_platform)

ENDMACRO()
