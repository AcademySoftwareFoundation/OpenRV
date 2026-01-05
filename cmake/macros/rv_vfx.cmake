#
# Copyright (C) 2024  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

#
# RV_VFX_SET_VARIABLE output the right value based on the VFX platform.
# The value is choosen based on the RV_VFX_CY20XX variable.
# Output variable: ${_rv_vfx_variable_name}
macro(RV_VFX_SET_VARIABLE _rv_vfx_variable_name)
  cmake_parse_arguments(
      # Prefix
      _rv_dep_var
      # Options
      ""
      # One value keywords
      "${RV_VFX_SUPPORTED_OPTIONS}"
      # Multi value keywords
      ""
      # Args
      ${ARGN}
  )

  # The implemenation covers a scenario where CY2023 is active, but only CY2024 is passed to the macro. (and vice versa)
  # In the scenario above, the variable will not be set.
  # The variable is only set if the the same CYXXXX is set and passed to the macro.
  # The first CYXXXX that matches will win. It is based on the order in the list RV_VFX_SUPPORTED_OPTIONS.

  # Loop all defined VFX platform.
  FOREACH(_rv_vfx_platform_ ${RV_VFX_SUPPORTED_OPTIONS})
    # If the VFX platform is defined and it was pass as option to RV_VFX_SET_VARIABLE, set the variable.
    IF(RV_VFX_${_rv_vfx_platform_} AND _rv_dep_var_${_rv_vfx_platform_})
      set(${_rv_vfx_variable_name} "${_rv_dep_var_${_rv_vfx_platform_}}")
      # The order in RV_VFX_SUPPORTED_OPTIONS is IMPORTANT.
      # Break on the first match.
      break()
    ENDIF()
  ENDFOREACH()

  # Clean up
  unset(_rv_dep_var_${_rv_vfx_platform_})
  unset(_rv_vfx_platform_)
endmacro()
