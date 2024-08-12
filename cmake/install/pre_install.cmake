#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

SET(QT_GPL_COMPONENTS
    charts
    datavisualization
    designer
    linguist
    designercomponent
    designerconfig
    networkauthorization
    networkauth
    virtualkeyboard
    quickwebgl
    webgl
    platforminputcontext
    design
)

# Note: These components break Apple codesign
SET(QT_BROKEN_COMPONENTS
    repparser uiplugin
)

LIST(APPEND QT_DO_NOT_INSTALL_COMPONENTS_LIST ${QT_GPL_COMPONENTS} ${QT_BROKEN_COMPONENTS})

FUNCTION(before_copy FILE_PATH RET_VAL)
  FOREACH(
    QT_DO_NOT_INSTALL_COMPONENT
    ${QT_DO_NOT_INSTALL_COMPONENTS_LIST}
  )
    STRING(TOLOWER "${FILE_PATH}" FILE_PATH_LOWER)

    IF(${FILE_PATH_LOWER} MATCHES "${QT_DO_NOT_INSTALL_COMPONENT}")
      SET(${RET_VAL}
          "NO"
          PARENT_SCOPE
      )
      RETURN()
    ENDIF()
  ENDFOREACH()

  IF(FILE_PATH MATCHES "\\.prl$")
    SET(${RET_VAL}
        "NO"
        PARENT_SCOPE
    )
    RETURN()
  ENDIF()

  IF(FILE_PATH MATCHES "\\.prl$"
     OR FILE_PATH MATCHES "\\.o$"
     OR FILE_PATH MATCHES "\\.obj$"
     OR FILE_PATH MATCHES "\\.a$"
     OR FILE_PATH MATCHES "\\.la$"
     OR FILE_PATH MATCHES "\\.c$"
  )
    SET(${RET_VAL}
        "NO"
        PARENT_SCOPE
    )
    RETURN()
  ENDIF()

  IF(FILE_PATH MATCHES "${RV_STAGE_LIB_DIR}/cmake"
     OR FILE_PATH MATCHES "${RV_STAGE_LIB_DIR}/engine"
     OR FILE_PATH MATCHES "${RV_STAGE_LIB_DIR}/metatype"
     OR FILE_PATH MATCHES "${RV_STAGE_LIB_DIR}/pkgconfig"
  )
    SET(${RET_VAL}
        "NO"
        PARENT_SCOPE
    )
    RETURN()
  ENDIF()

  IF(FILE_PATH MATCHES "${RV_STAGE_BIN_DIR}/.*Test.*"
     AND NOT FILE_PATH MATCHES "${RV_STAGE_BIN_DIR}/Qt5Test.*"
  )
    SET(${RET_VAL}
        "NO"
        PARENT_SCOPE
    )
    RETURN()
  ENDIF()

  IF(COMMAND before_copy_platform)
    BEFORE_COPY_PLATFORM(${FILE_PATH} RET_VAL_PLATFORM)
    SET(${RET_VAL}
        "${RET_VAL_PLATFORM}"
        PARENT_SCOPE
    )
    RETURN()
  ELSE()
    SET(${RET_VAL}
        "YES"
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

MACRO(after_copy FILE_PATH FILES_TO_FIX_RPATH)
  IF(COMMAND after_copy_platform)
    AFTER_COPY_PLATFORM(${FILE_PATH} ${FILES_TO_FIX_RPATH})
  ENDIF()
ENDMACRO()

MACRO(post_install)
  IF(COMMAND post_install_platform)
    POST_INSTALL_PLATFORM()
  ENDIF()
ENDMACRO()
