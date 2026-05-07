#
# rv_create_soname_symlink.cmake
#
# Build-time script to create SONAME symlinks
#
# Usage: cmake -P rv_create_soname_symlink.cmake -DLIBRARY=<path-to-staged-lib>
#
# Reads the library's SONAME via otool -D (macOS) or objdump -p (Linux). If the SONAME differs from the actual filename, creates a symlink so the dynamic linker
# can resolve it at runtime.
#

IF(NOT LIBRARY)
  MESSAGE(FATAL_ERROR "rv_create_soname_symlink: LIBRARY argument required")
ENDIF()

IF(NOT EXISTS "${LIBRARY}")
  MESSAGE(FATAL_ERROR "rv_create_soname_symlink: ${LIBRARY} does not exist")
ENDIF()

GET_FILENAME_COMPONENT(_actual_name "${LIBRARY}" NAME)
GET_FILENAME_COMPONENT(_lib_dir "${LIBRARY}" DIRECTORY)

SET(_soname
    ""
)

IF(APPLE)
  # macOS: read LC_ID_DYLIB via otool -D
  EXECUTE_PROCESS(
    COMMAND otool -D "${LIBRARY}"
    OUTPUT_VARIABLE _otool_out
    OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET
  )
  # otool -D output: first line is the file path, second line is the install name
  STRING(REGEX MATCH "[^\n]+$" _install_name "${_otool_out}")
  IF(_install_name)
    GET_FILENAME_COMPONENT(_soname "${_install_name}" NAME)
  ENDIF()
ELSEIF(LINUX)
  # Linux: read SONAME via objdump -p
  EXECUTE_PROCESS(
    COMMAND objdump -p "${LIBRARY}"
    OUTPUT_VARIABLE _objdump_out
    OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET
  )
  # objdump -p output contains: "  SONAME               libFoo.so.2.3"
  STRING(REGEX MATCH "SONAME[ \t]+([^\n]+)" _soname_match "${_objdump_out}")
  SET(_soname
      "${CMAKE_MATCH_1}"
  )
  # Strip trailing whitespace
  IF(_soname)
    STRING(STRIP "${_soname}" _soname)
  ENDIF()
ENDIF()

IF(_soname
   AND NOT "${_soname}" STREQUAL "${_actual_name}"
)
  SET(_symlink_path
      "${_lib_dir}/${_soname}"
  )
  IF(NOT EXISTS "${_symlink_path}")
    MESSAGE(STATUS "  Creating SONAME symlink: ${_soname} -> ${_actual_name}")
    EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} -E create_symlink "${_actual_name}" "${_symlink_path}")
  ENDIF()
ENDIF()
