#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

MACRO(RV_MAKE_STANDARD_LIB_NAME base_name version lib_type debug_postfix)

  IF(CMAKE_BUILD_TYPE MATCHES "^Debug$")
    SET(_debug_postfix
        "${debug_postfix}"
    )
    MESSAGE(DEBUG "Using debug postfix: '${_debug_postfix}'")
  ELSE()
    # Voiding the specified postfix
    SET(_debug_postfix
        ""
    )
  ENDIF()

  SET(_version_string
      ""
  )
  # Using quotes to avoid passing an empty string so 0 length is now 2 length.
  STRING(LENGTH '${version}' _version_length)
  IF(_version_length GREATER 2)
    SET(_version_string
        ".${version}"
    )
  ENDIF()
  IF(RV_TARGET_DARWIN)
    SET(_libname
        ${CMAKE_${lib_type}_LIBRARY_PREFIX}${base_name}${_debug_postfix}${_version_string}${CMAKE_${lib_type}_LIBRARY_SUFFIX}
    )
  ELSEIF(RV_TARGET_LINUX)
    SET(_libname
        ${CMAKE_${lib_type}_LIBRARY_PREFIX}${base_name}${_debug_postfix}${CMAKE_${lib_type}_LIBRARY_SUFFIX}${_version_string}
    )
  ELSEIF(RV_TARGET_WINDOWS)
    SET(_libname
        ${CMAKE_${lib_type}_LIBRARY_PREFIX}${base_name}${_debug_postfix}${CMAKE_${lib_type}_LIBRARY_SUFFIX}
    )
  ENDIF()

  IF(RV_TARGET_WINDOWS)
    SET(_libpath
        ${_bin_dir}/${_libname}
    )

    GET_FILENAME_COMPONENT(_binlibnamenoext ${_libname} NAME_WE)
    SET(_implibname
        ${CMAKE_IMPORT_LIBRARY_PREFIX}${_binlibnamenoext}${CMAKE_IMPORT_LIBRARY_SUFFIX}
    )
    SET(_implibpath
        ${_lib_dir}/${_implibname}
    )
    LIST(APPEND _byproducts ${_implibpath})
  ELSE()
    SET(_libpath
        ${_lib_dir}/${_libname}
    )
  ENDIF()

  SET(_byproducts
      ${_libpath}
  )

  MESSAGE(DEBUG "RV_MAKE_STANDARD_LIB_NAME:")
  MESSAGE(DEBUG "  _libname      ='${_libname}'")
  MESSAGE(DEBUG "  _libpath      ='${_libpath}'")
  MESSAGE(DEBUG "  _implibname   ='${_implibname}'")
  MESSAGE(DEBUG "  _implibpath   ='${_implibpath}'")
  MESSAGE(DEBUG "  _byproducts   ='${_byproducts}'")

ENDMACRO()
