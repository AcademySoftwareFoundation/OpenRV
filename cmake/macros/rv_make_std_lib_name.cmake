
MACRO(RV_MAKE_STANDARD_LIB_NAME base_name version lib_type debug_postfix)

  IF(CMAKE_BUILD_TYPE MATCHES "^Debug$")
    SET(_debug_postfix "${debug_postfix}")
    MESSAGE(DEBUG "Using debug postfix: '${_debug_postfix}'")
  ELSE()
    # Voiding the specified postfix
    SET(_debug_postfix "")
  ENDIF()

  IF(RV_TARGET_DARWIN)
    SET(_libname
        ${CMAKE_${lib_type}_LIBRARY_PREFIX}${base_name}${_debug_postfix}.${version}${CMAKE_${lib_type}_LIBRARY_SUFFIX}
    )
  ELSEIF(RV_TARGET_LINUX)
    SET(_libname
        ${CMAKE_${lib_type}_LIBRARY_PREFIX}${base_name}${_debug_postfix}${CMAKE_${lib_type}_LIBRARY_SUFFIX}.${version}
    )
  ELSEIF(RV_TARGET_WINDOWS)
    SET(_libname
        ${CMAKE_${lib_type}_LIBRARY_PREFIX}${base_name}${_debug_postfix}${CMAKE_${lib_type}_LIBRARY_SUFFIX}
    )
  ENDIF()

  SET(_libpath
      ${_lib_dir}/${_libname}
  )

  SET( _byproducts ${_libpath})

  IF(RV_TARGET_WINDOWS)
    SET(_implibname
        ${CMAKE_IMPORT_LIBRARY_PREFIX}${_libname}${CMAKE_IMPORT_LIBRARY_SUFFIX}
    )
    SET(_implibpath
        ${_lib_dir}/${_implibname}
    )
    LIST(APPEND _byproducts ${_implibpath})
  ENDIF()

  MESSAGE(DEBUG "RV_MAKE_STANDARD_LIB_NAME:")
  MESSAGE(DEBUG "  _libname      ='${_libname}'")
  MESSAGE(DEBUG "  _libpath      ='${_libpath}'")
  MESSAGE(DEBUG "  _implibname   ='${_implibname}'")
  MESSAGE(DEBUG "  _implibpath   ='${_implibpath}'")
  MESSAGE(DEBUG "  _byproducts   ='${_byproducts}'")
  
ENDMACRO()