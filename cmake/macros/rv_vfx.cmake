#
# Copyright (C) 2024  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

#
# RV_VFX_SET_DOWNLOAD_HASH output the right hash based on the VFX platform.
# The value is choosen based on the RV_VFX_CY20XX variable.
# Output variable: _download_hash
macro(RV_VFX_SET_DOWNLOAD_HASH)
  cmake_parse_arguments(
      _rv_dep_hash
      # options
      ""
      # one value keywords
      "CY2023;CY2024"
      # multi value keywords
      ""
      # args
      ${ARGN}
  )

  IF(RV_VFX_CY2023)
    SET(_download_hash "${_rv_dep_hash_CY2023}")
  ELSEIF(RV_VFX_CY2024)
    SET(_download_hash "${_rv_dep_hash_CY2024}")
  ENDIF()

  # Clean up
  UNSET(_rv_dep_hash_CY2023)
  UNSET(_rv_dep_hash_CY2024)
endmacro()

#
# RV_VFX_SET_VERSION output the right version based on the VFX platform.
# The value is choosen based on the RV_VFX_CY20XX variable.
# Output variable: _ext_dep_version
macro(RV_VFX_SET_VERSION)
  cmake_parse_arguments(
      _rv_dep_version
      # options
      ""
      # one value keywords
      "CY2023;CY2024"
      # multi value keywords
      ""
      # args
      ${ARGN}
  )

  IF(RV_VFX_CY2023)
    SET(_ext_dep_version "${_rv_dep_version_CY2023}")
  ELSEIF(RV_VFX_CY2024)
    SET(_ext_dep_version "${_rv_dep_version_CY2024}")
  ENDIF()

  # Clean up
  UNSET(_rv_dep_version_CY2023)
  UNSET(_rv_dep_version_CY2024)
endmacro()