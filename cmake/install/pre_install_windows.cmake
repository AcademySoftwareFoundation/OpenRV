#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

FUNCTION(before_copy_platform FILE_PATH RET_VAL)
  # PDB debug symbols are not shipped to customers; they are archived separately for offline crash symbolication via the 'symbols_archive' build target (see
  # docs/crash-reporting.md, section 7). Strip them from Release (customer) installs while keeping them for local Debug installs.
  IF(FILE_PATH MATCHES "\\.pdb$")
    IF(CMAKE_INSTALL_CONFIG_NAME MATCHES "^Release$")
      SET(${RET_VAL}
          "NO"
          PARENT_SCOPE
      )
      RETURN()
    ENDIF()
  ENDIF()

  # Exp files contains information about symbols. Not needed for runtime.
  IF(FILE_PATH MATCHES "\\.exp$")
    SET(${RET_VAL}
        "NO"
        PARENT_SCOPE
    )
    RETURN()
  ENDIF()

  # Only used by the compiler/linker. Not needed for runtime.
  IF(FILE_PATH MATCHES "\\.lib$")
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

FUNCTION(after_copy_platform FILE_PATH FILES_TO_FIX_RPATH)

ENDFUNCTION()

MACRO(post_install_platform)

ENDMACRO()
