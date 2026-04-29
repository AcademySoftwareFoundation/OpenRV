#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

#
# WebP official Web page:  -- https://www.webmproject.org
#
# WebP official sources:   -- https://chromium.googlesource.com/webm/libwebp
#
# WebP build from sources: -- https://github.com/webmproject/libwebp/blob/main/doc/building.md
#

FIND_PACKAGE(WebP QUIET)
IF(WebP_FOUND)
  MESSAGE(STATUS "Using system WebP")
  IF(DEFINED WebP_VERSION)
    SET(RV_DEPS_WEBP_VERSION
        "${WebP_VERSION}"
    )
    SET(RV_DEPS_WEBP_VERSION
        "${WebP_VERSION}"
        CACHE STRING "" FORCE
    )
  ELSEIF(DEFINED WEBP_VERSION)
    SET(RV_DEPS_WEBP_VERSION
        "${WEBP_VERSION}"
    )
    SET(RV_DEPS_WEBP_VERSION
        "${WEBP_VERSION}"
        CACHE STRING "" FORCE
    )
  ENDIF()
  # If the system package doesn't provide a WebP::webp target, we might need to create an alias, but FindWebP usually provides WebP::webp.
ENDIF()
