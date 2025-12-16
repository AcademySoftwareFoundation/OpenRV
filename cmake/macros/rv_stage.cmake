#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

# Create & populate a list of all the shared libraries for later testing.
MACRO(ADD_SHARED_LIBRARY_LIST new_entry)
  LIST(APPEND _shared_libraries "${new_entry}")
  LIST(REMOVE_DUPLICATES _shared_libraries)
  SET(_shared_libraries
      ${_shared_libraries}
      CACHE INTERNAL ""
  )
ENDMACRO()

ADD_CUSTOM_TARGET(main_executable)
ADD_CUSTOM_TARGET(executables)
ADD_CUSTOM_TARGET(executables_with_plugins)
ADD_CUSTOM_TARGET(shared_libraries)
ADD_CUSTOM_TARGET(libraries)
ADD_CUSTOM_TARGET(mu_source_modules)
ADD_CUSTOM_TARGET(python_source_modules)
ADD_CUSTOM_TARGET(compiled_python_source_modules)
ADD_CUSTOM_TARGET(mu_plugins)
ADD_CUSTOM_TARGET(python_plugins)
ADD_CUSTOM_TARGET(packages)
ADD_CUSTOM_TARGET(installed_packages)
ADD_CUSTOM_TARGET(movie_formats)
ADD_CUSTOM_TARGET(installed_movie_formats)
ADD_CUSTOM_TARGET(image_formats)
ADD_CUSTOM_TARGET(installed_image_formats)
ADD_CUSTOM_TARGET(oiio_plugins)
ADD_CUSTOM_TARGET(output_plugins)

FUNCTION(rv_stage)
  SET(flags
      ""
  )
  SET(args
      ""
  )
  SET(listArgs
      TYPE TARGET FILES PATH
  )

  CMAKE_PARSE_ARGUMENTS(arg "${flags}" "${args}" "${listArgs}" ${ARGN})

  MESSAGE(DEBUG "******************************************************")
  MESSAGE(DEBUG "rv_stage params:")
  MESSAGE(DEBUG "--- TYPE                    : '${arg_TYPE}'")
  MESSAGE(DEBUG "--- TARGET                  : '${arg_TARGET}'")
  MESSAGE(DEBUG "--- FILES                   : '${arg_FILES}'")
  MESSAGE(DEBUG "--- PATH                    : '${arg_PATH}'")
  MESSAGE(DEBUG "--- UNPARSED_ARGUMENTS      : '${arg_UNPARSED_ARGUMENTS}'")
  MESSAGE(DEBUG "--- KEYWORDS_MISSING_VALUES : '${arg_KEYWORDS_MISSING_VALUES}'")

  IF(NOT arg_TARGET)
    MESSAGE(FATAL_ERROR "The 'TARGET' parameter was not specified.")
  ENDIF()

  IF(NOT arg_TYPE)
    MESSAGE(FATAL_ERROR "The 'TYPE' parameter was not specified.")
  ENDIF()

  IF(RV_TARGET_LINUX)
    IF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${arg_TARGET}.wrapper)

      ADD_CUSTOM_COMMAND(
        TARGET ${arg_TARGET}
        POST_BUILD
        COMMENT "Building ${RV_STAGE_BIN_DIR}/${arg_TARGET}.wrapper"
        COMMAND ${CMAKE_COMMAND} -E rename "$<TARGET_FILE:${arg_TARGET}>" "$<TARGET_FILE:${arg_TARGET}>.bin"
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/${arg_TARGET}.wrapper "$<TARGET_FILE:${arg_TARGET}>"
        COMMAND chmod +x "$<TARGET_FILE:${arg_TARGET}>"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      )
    ENDIF()
  ENDIF()

  IF(RV_TARGET_DARWIN)
    IF(TARGET ${arg_TARGET})
      GET_TARGET_PROPERTY(_native_target_type ${arg_TARGET} TYPE)
      IF(_native_target_type STREQUAL "EXECUTABLE")
        ADD_CUSTOM_COMMAND(
          COMMENT "Fixing ${arg_TARGET}'s RPATHs" TARGET ${arg_TARGET} POST_BUILD
          COMMAND ${CMAKE_INSTALL_NAME_TOOL} -add_rpath "@executable_path/../Frameworks" "$<TARGET_FILE:${arg_TARGET}>"
          COMMAND ${CMAKE_INSTALL_NAME_TOOL} -add_rpath "@executable_path/../lib" "$<TARGET_FILE:${arg_TARGET}>"
        )
      ENDIF()

      IF(_native_target_type STREQUAL "EXECUTABLE"
         OR _native_target_type STREQUAL "SHARED_LIBRARY"
      )
        FOREACH(
          dep
          ${RV_DEPS_LIST}
        )
          IF(TARGET ${dep})
            GET_PROPERTY(
              dep_file_path
              TARGET ${dep}
              PROPERTY LOCATION
            )
            GET_FILENAME_COMPONENT(dep_file_name ${dep_file_path} NAME)
            ADD_CUSTOM_COMMAND(
              COMMENT "Fixing ${dep_file_name}'s rpath in ${arg_TARGET}" TARGET ${arg_TARGET} POST_BUILD
              COMMAND ${CMAKE_INSTALL_NAME_TOOL} -change "${dep_file_path}" "@rpath/${dep_file_name}" "$<TARGET_FILE:${arg_TARGET}>"
            )
          ENDIF()
        ENDFOREACH()
      ENDIF()
    ENDIF()
  ENDIF()

  IF(${arg_TYPE} STREQUAL "SHARED_LIBRARY")
    GET_TARGET_PROPERTY(_native_target_type ${arg_TARGET} TYPE)
    IF(NOT _native_target_type STREQUAL "SHARED_LIBRARY")
      MESSAGE(FATAL_ERROR "\"${arg_TARGET}\" ${arg_TYPE} should be a SHARED_LIBRARY, not a ${_native_target_type}")
    ENDIF()
    SET_TARGET_PROPERTIES(
      ${arg_TARGET}
      PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${RV_STAGE_LIB_DIR}"
                 C_VISIBILITY_PRESET default
                 CXX_VISIBILITY_PRESET default
    )
    IF(RV_TARGET_WINDOWS)
      FOREACH(
        OUTPUTCONFIG
        ${CMAKE_CONFIGURATION_TYPES}
      )
        STRING(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
        SET_TARGET_PROPERTIES(
          ${arg_TARGET}
          PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${RV_STAGE_BIN_DIR}"
        )
      ENDFOREACH()
    ENDIF()

    ADD_SHARED_LIBRARY_LIST(${arg_TARGET})

    ADD_DEPENDENCIES(shared_libraries ${arg_TARGET})

  ELSEIF(${arg_TYPE} STREQUAL "LIBRARY")
    GET_TARGET_PROPERTY(_native_target_type ${arg_TARGET} TYPE)
    IF(NOT _native_target_type STREQUAL "STATIC_LIBRARY")
      MESSAGE(FATAL_ERROR "\"${arg_TARGET}\" ${arg_TYPE} should be a STATIC_LIBRARY, not a ${_native_target_type}")
    ENDIF()

    SET_TARGET_PROPERTIES(
      ${arg_TARGET}
      PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${RV_STAGE_LIB_DIR}"
    )
    IF(RV_TARGET_WINDOWS)
      FOREACH(
        OUTPUTCONFIG
        ${CMAKE_CONFIGURATION_TYPES}
      )
        STRING(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
        SET_TARGET_PROPERTIES(
          ${arg_TARGET}
          PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${RV_STAGE_LIB_DIR}"
        )
      ENDFOREACH()
    ENDIF()

    ADD_DEPENDENCIES(libraries ${arg_TARGET})

  ELSEIF(${arg_TYPE} STREQUAL "MU_SOURCE_MODULE")
    FOREACH(
      _file
      ${arg_FILES}
    )
      SET(input_file
          ${CMAKE_CURRENT_SOURCE_DIR}/${_file}
      )
      SET(output_file
          ${RV_STAGE_PLUGINS_MU_DIR}/${_file}
      )

      IF(IS_DIRECTORY ${input_file})
        LIST(APPEND ${arg_TARGET}_mu_source_module_directory_input ${input_file})

        FILE(
          GLOB_RECURSE output_dir_files LIST_DIRECTORY FALSE
          RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
          "${input_file}/*"
        )
        FOREACH(
          output_dir_file
          ${output_dir_files}
        )
          LIST(APPEND ${arg_TARGET}_mu_source_module_directory_output ${RV_STAGE_PLUGINS_MU_DIR}/${output_dir_file})
        ENDFOREACH()

      ELSE()
        LIST(APPEND ${arg_TARGET}_mu_source_module_file_input ${input_file})
        LIST(APPEND ${arg_TARGET}_mu_source_module_file_output ${output_file})
      ENDIF()
    ENDFOREACH()

    FOREACH(
      directory
      ${${arg_TARGET}_mu_source_module_directory_input}
    )
      GET_FILENAME_COMPONENT(directory_name ${directory} NAME)
      FILE(GLOB_RECURSE directory_files LIST_DIRECTORY FALSE "${directory}/*")
      FOREACH(
        file
        ${directory_files}
      )

        FILE(RELATIVE_PATH relative_file ${directory} ${file})

        LIST(APPEND ${directory}_depends ${file})
        LIST(APPEND ${directory}_output ${RV_STAGE_PLUGINS_MU_DIR}/${directory_name}/${relative_file})

        LIST(APPEND ${arg_TARGET}_mu_source_module_directory_output ${RV_STAGE_PLUGINS_MU_DIR}/${output_dir_file})
      ENDFOREACH()

      ADD_CUSTOM_COMMAND(
        COMMENT "Copying ${arg_TARGET} Python Source Module Directory '${directory_name}'"
        OUTPUT ${${directory}_output}
        DEPENDS ${${directory}_depends}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${directory} ${RV_STAGE_PLUGINS_MU_DIR}/${directory_name}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      )
    ENDFOREACH()

    IF(${arg_TARGET}_mu_source_module_file_input)
      ADD_CUSTOM_COMMAND(
        COMMENT "Copying ${arg_TARGET} Python Source Module Files"
        OUTPUT ${${arg_TARGET}_mu_source_module_file_output}
        DEPENDS ${${arg_TARGET}_mu_source_module_file_input}
        COMMAND ${CMAKE_COMMAND} -E copy ${${arg_TARGET}_mu_source_module_file_input} ${RV_STAGE_PLUGINS_MU_DIR}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      )
    ENDIF()

    ADD_CUSTOM_TARGET(
      mu-source-${arg_TARGET}
      DEPENDS ${${arg_TARGET}_mu_source_module_file_output} ${${arg_TARGET}_mu_source_module_directory_output}
    )

    ADD_DEPENDENCIES(mu_source_modules mu-source-${arg_TARGET})
  ELSEIF(${arg_TYPE} STREQUAL "PYTHON_SOURCE_MODULE")
    FOREACH(
      _file
      ${arg_FILES}
    )
      SET(input_file
          ${CMAKE_CURRENT_SOURCE_DIR}/${_file}
      )
      SET(output_file
          ${RV_STAGE_PLUGINS_PYTHON_DIR}/${_file}
      )

      IF(IS_DIRECTORY ${input_file})
        LIST(APPEND ${arg_TARGET}_python_source_module_directory_input ${input_file})

        FILE(
          GLOB_RECURSE output_dir_files LIST_DIRECTORY FALSE
          RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
          "${input_file}/*"
        )
        FOREACH(
          output_dir_file
          ${output_dir_files}
        )
          LIST(APPEND ${arg_TARGET}_python_source_module_directory_output ${RV_STAGE_PLUGINS_PYTHON_DIR}/${output_dir_file})
        ENDFOREACH()

      ELSE()
        LIST(APPEND ${arg_TARGET}_python_source_module_file_input ${input_file})
        LIST(APPEND ${arg_TARGET}_python_source_module_file_output ${output_file})
      ENDIF()
    ENDFOREACH()

    FOREACH(
      directory
      ${${arg_TARGET}_python_source_module_directory_input}
    )
      GET_FILENAME_COMPONENT(directory_name ${directory} NAME)
      FILE(GLOB_RECURSE directory_files LIST_DIRECTORY FALSE "${directory}/*")
      FOREACH(
        file
        ${directory_files}
      )

        FILE(RELATIVE_PATH relative_file ${directory} ${file})

        LIST(APPEND ${directory}_depends ${file})
        LIST(APPEND ${directory}_output ${RV_STAGE_PLUGINS_PYTHON_DIR}/${directory_name}/${relative_file})

        LIST(APPEND ${arg_TARGET}_python_source_module_directory_output ${RV_STAGE_PLUGINS_PYTHON_DIR}/${output_dir_file})
      ENDFOREACH()

      ADD_CUSTOM_COMMAND(
        COMMENT "Copying ${arg_TARGET} Python Source Module Directory '${directory_name}'"
        OUTPUT ${${directory}_output}
        DEPENDS ${${directory}_depends}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${directory} ${RV_STAGE_PLUGINS_PYTHON_DIR}/${directory_name}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      )
    ENDFOREACH()

    IF(${arg_TARGET}_python_source_module_file_input)
      ADD_CUSTOM_COMMAND(
        COMMENT "Copying ${arg_TARGET} Python Source Module Files"
        OUTPUT ${${arg_TARGET}_python_source_module_file_output}
        DEPENDS ${${arg_TARGET}_python_source_module_file_input}
        COMMAND ${CMAKE_COMMAND} -E copy ${${arg_TARGET}_python_source_module_file_input} ${RV_STAGE_PLUGINS_PYTHON_DIR}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      )
    ENDIF()

    ADD_CUSTOM_TARGET(
      python-source-${arg_TARGET}
      DEPENDS ${${arg_TARGET}_python_source_module_file_output} ${${arg_TARGET}_python_source_module_directory_output}
    )

    ADD_DEPENDENCIES(python_source_modules python-source-${arg_TARGET})

  ELSEIF(${arg_TYPE} STREQUAL "MU_PLUGIN")
    GET_TARGET_PROPERTY(_native_target_type ${arg_TARGET} TYPE)
    IF(NOT _native_target_type STREQUAL "SHARED_LIBRARY")
      MESSAGE(FATAL_ERROR "\"${arg_TARGET}\" ${arg_TYPE} should be a SHARED_LIBRARY, not a ${_native_target_type}")
    ENDIF()

    SET_TARGET_PROPERTIES(
      ${arg_TARGET}
      PROPERTIES PREFIX ""
                 SUFFIX ".so"
                 LIBRARY_OUTPUT_DIRECTORY "${RV_STAGE_PLUGINS_MU_DIR}"
                 C_VISIBILITY_PRESET default
                 CXX_VISIBILITY_PRESET default
    )
    IF(RV_TARGET_WINDOWS)
      FOREACH(
        OUTPUTCONFIG
        ${CMAKE_CONFIGURATION_TYPES}
      )
        STRING(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
        SET_TARGET_PROPERTIES(
          ${arg_TARGET}
          PROPERTIES PREFIX ""
                     SUFFIX ".so"
                     RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${RV_STAGE_PLUGINS_MU_DIR}"
        )
      ENDFOREACH()
    ENDIF()

    ADD_DEPENDENCIES(mu_plugins ${arg_TARGET})

    ADD_SHARED_LIBRARY_LIST(${arg_TARGET})

  ELSEIF(${arg_TYPE} STREQUAL "PYTHON_PLUGIN")
    GET_TARGET_PROPERTY(_native_target_type ${arg_TARGET} TYPE)
    IF(NOT _native_target_type STREQUAL "SHARED_LIBRARY")
      MESSAGE(FATAL_ERROR "\"${arg_TARGET}\" ${arg_TYPE} should be a SHARED_LIBRARY, not a ${_native_target_type}")
    ENDIF()
    SET_TARGET_PROPERTIES(
      ${arg_TARGET}
      PROPERTIES PREFIX ""
                 SUFFIX ".so"
                 LIBRARY_OUTPUT_DIRECTORY "${RV_STAGE_PLUGINS_PYTHON_DIR}"
                 C_VISIBILITY_PRESET default
                 CXX_VISIBILITY_PRESET default
    )
    IF(RV_TARGET_WINDOWS)
      FOREACH(
        OUTPUTCONFIG
        ${CMAKE_CONFIGURATION_TYPES}
      )
        # Note: Python expects modules to have the _d.pyd suffix in Debug on Windows
        STRING(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
        SET_TARGET_PROPERTIES(
          ${arg_TARGET}
          PROPERTIES PREFIX ""
                     SUFFIX "${RV_DEBUG_POSTFIX}.pyd"
                     RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${RV_STAGE_PLUGINS_PYTHON_DIR}"
                     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}./
        )
      ENDFOREACH()
    ENDIF()

    ADD_DEPENDENCIES(python_plugins ${arg_TARGET})

    ADD_SHARED_LIBRARY_LIST(${arg_TARGET})

  ELSEIF(${arg_TYPE} STREQUAL "RVPKG")
    IF(${arg_TARGET} IN_LIST RVPKG_DO_NOT_INSTALL_LIST)
      MESSAGE(STATUS "Skipping RV package '${arg_TARGET}' as requested")
    ELSE()
      SET(_package_file
          PACKAGE
      )

      IF(NOT arg_FILES)
        FILE(
          GLOB_RECURSE _files
          RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
          CONFIGURE_DEPENDS *
        )
      ELSE()
        SET(_files
            ${arg_FILES}
        )
      ENDIF()

      LIST(REMOVE_ITEM _files Makefile CMakeLists.txt ${_package_file})

      IF(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${_package_file})
        MESSAGE(FATAL_ERROR "Couldn't find expected package descriptor file: '${_package_file}'")
      ENDIF()

      SET_DIRECTORY_PROPERTIES(PROPERTIES CMAKE_CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${_package_file})

      EXECUTE_PROCESS(
        COMMAND bash -c "cat ${_package_file} | grep version: | grep --only-matching -e '[0-9.]*'"
        RESULT_VARIABLE _result
        OUTPUT_VARIABLE _pkg_version
        OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      )
      IF(_result
         AND NOT _result EQUAL 0
      )
        MESSAGE(FATAL_ERROR "Error retrieving version field from '${_package_file}'")
      ELSE()
        MESSAGE(DEBUG "Found version for '${arg_TARGET}.rvpkg' package version ${_pkg_version} ...")
      ENDIF()

      SET(_package_filename
          "${RV_PACKAGES_DIR}/${arg_TARGET}-${_pkg_version}.rvpkg"
      )

      LIST(APPEND RV_PACKAGE_LIST "${RV_PACKAGES_DIR}/${arg_TARGET}-${_pkg_version}.rvpkg")
      LIST(REMOVE_DUPLICATES RV_PACKAGE_LIST)
      SET(RV_PACKAGE_LIST
          ${RV_PACKAGE_LIST}
          CACHE INTERNAL ""
      )

      # Generate a file to store the list of package files
      SET(_temp_file
          "${CMAKE_CURRENT_BINARY_DIR}/pkgfilelist.txt"
      )

      # Remove the file if it exists
      FILE(REMOVE ${_temp_file})

      # For each package file in the list, append it to the file
      FOREACH(
        file IN
        LISTS _files _package_file
      )
        FILE(
          APPEND ${_temp_file}
          "${file}\n"
        )
      ENDFOREACH()

      # Create the package zip file
      ADD_CUSTOM_COMMAND(
        COMMENT "Creating ${_package_filename} ..."
        OUTPUT ${_package_filename}
        DEPENDS ${_temp_file} ${_files} ${_package_file}
        COMMAND ${CMAKE_COMMAND} -E tar "cfv" ${_package_filename} --format=zip --files-from=${_temp_file}
        COMMAND ${CMAKE_COMMAND} -E rm -f ${RV_STAGE_PLUGINS_PACKAGES_DIR}/rvinstall
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      )

      ADD_CUSTOM_TARGET(
        ${arg_TARGET}-${_pkg_version}.rvpkg ALL
        DEPENDS ${_package_filename}
      )

      ADD_DEPENDENCIES(packages ${arg_TARGET}-${_pkg_version}.rvpkg)
    ENDIF()

  ELSEIF(${arg_TYPE} STREQUAL "IMAGE_FORMAT")
    GET_TARGET_PROPERTY(_native_target_type ${arg_TARGET} TYPE)
    IF(NOT _native_target_type STREQUAL "SHARED_LIBRARY")
      MESSAGE(FATAL_ERROR "\"${arg_TARGET}\" ${arg_TYPE} should be a SHARED_LIBRARY, not a ${_native_target_type}")
    ENDIF()
    SET_TARGET_PROPERTIES(
      ${arg_TARGET}
      PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${RV_STAGE_PLUGINS_IMAGEFORMATS_DIR}"
                 PREFIX ""
                 C_VISIBILITY_PRESET default
                 CXX_VISIBILITY_PRESET default
    )
    IF(RV_TARGET_WINDOWS)
      FOREACH(
        OUTPUTCONFIG
        ${CMAKE_CONFIGURATION_TYPES}
      )
        STRING(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
        SET_TARGET_PROPERTIES(
          ${arg_TARGET}
          PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${RV_STAGE_PLUGINS_IMAGEFORMATS_DIR}"
        )
      ENDFOREACH()
    ENDIF()

    ADD_DEPENDENCIES(image_formats ${arg_TARGET})

    ADD_SHARED_LIBRARY_LIST(${arg_TARGET})

  ELSEIF(${arg_TYPE} STREQUAL "MOVIE_FORMAT")
    GET_TARGET_PROPERTY(_native_target_type ${arg_TARGET} TYPE)
    IF(NOT _native_target_type STREQUAL "SHARED_LIBRARY")
      MESSAGE(FATAL_ERROR "\"${arg_TARGET}\" ${arg_TYPE} should be a SHARED_LIBRARY, not a ${_native_target_type}")
    ENDIF()
    SET_TARGET_PROPERTIES(
      ${arg_TARGET}
      PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${RV_STAGE_PLUGINS_MOVIEFORMATS_DIR}"
                 PREFIX ""
                 C_VISIBILITY_PRESET default
                 CXX_VISIBILITY_PRESET default
    )
    IF(RV_TARGET_WINDOWS)
      FOREACH(
        OUTPUTCONFIG
        ${CMAKE_CONFIGURATION_TYPES}
      )
        STRING(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
        SET_TARGET_PROPERTIES(
          ${arg_TARGET}
          PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${RV_STAGE_PLUGINS_MOVIEFORMATS_DIR}"
        )
      ENDFOREACH()
    ENDIF()

    ADD_DEPENDENCIES(movie_formats ${arg_TARGET})

    ADD_SHARED_LIBRARY_LIST(${arg_TARGET})

  ELSEIF(${arg_TYPE} STREQUAL "OIIO_PLUGIN")
    GET_TARGET_PROPERTY(_native_target_type ${arg_TARGET} TYPE)
    IF(NOT _native_target_type STREQUAL "SHARED_LIBRARY")
      MESSAGE(FATAL_ERROR "\"${arg_TARGET}\" ${arg_TYPE} should be a SHARED_LIBRARY, not a ${_native_target_type}")
    ENDIF()
    SET_TARGET_PROPERTIES(
      ${arg_TARGET}
      PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${RV_STAGE_PLUGINS_OIIO_DIR}"
                 PREFIX ""
                 C_VISIBILITY_PRESET default
                 CXX_VISIBILITY_PRESET default
    )
    IF(RV_TARGET_WINDOWS)
      FOREACH(
        OUTPUTCONFIG
        ${CMAKE_CONFIGURATION_TYPES}
      )
        STRING(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
        SET_TARGET_PROPERTIES(
          ${arg_TARGET}
          PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${RV_STAGE_PLUGINS_OIIO_DIR}"
                     PREFIX ""
        )
      ENDFOREACH()
    ENDIF()

    ADD_DEPENDENCIES(oiio_plugins ${arg_TARGET})

    ADD_SHARED_LIBRARY_LIST(${arg_TARGET})

  ELSEIF(${arg_TYPE} STREQUAL "OUTPUT_PLUGIN")
    GET_TARGET_PROPERTY(_native_target_type ${arg_TARGET} TYPE)
    IF(NOT _native_target_type STREQUAL "SHARED_LIBRARY")
      MESSAGE(FATAL_ERROR "\"${arg_TARGET}\" ${arg_TYPE} should be a SHARED_LIBRARY, not a ${_native_target_type}")
    ENDIF()
    SET_TARGET_PROPERTIES(
      ${arg_TARGET}
      PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${RV_STAGE_PLUGINS_OUTPUT_DIR}"
                 PREFIX ""
                 C_VISIBILITY_PRESET default
                 CXX_VISIBILITY_PRESET default
    )
    IF(RV_TARGET_WINDOWS)
      FOREACH(
        OUTPUTCONFIG
        ${CMAKE_CONFIGURATION_TYPES}
      )
        STRING(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
        SET_TARGET_PROPERTIES(
          ${arg_TARGET}
          PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${RV_STAGE_PLUGINS_OUTPUT_DIR}"
        )
      ENDFOREACH()
    ENDIF()

    ADD_DEPENDENCIES(output_plugins ${arg_TARGET})

    ADD_SHARED_LIBRARY_LIST(${arg_TARGET})

  ELSEIF(${arg_TYPE} STREQUAL "EXECUTABLE")
    GET_TARGET_PROPERTY(_native_target_type ${arg_TARGET} TYPE)
    IF(NOT _native_target_type STREQUAL "EXECUTABLE")
      MESSAGE(FATAL_ERROR "\"${arg_TARGET}\" ${arg_TYPE} should be a EXECUTABLE, not a ${_native_target_type}")
    ENDIF()

    ADD_DEPENDENCIES(executables ${arg_TARGET})

    SET_TARGET_PROPERTIES(
      ${arg_TARGET}
      PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${RV_STAGE_BIN_DIR}"
    )
    IF(RV_TARGET_WINDOWS)
      FOREACH(
        OUTPUTCONFIG
        ${CMAKE_CONFIGURATION_TYPES}
      )
        STRING(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
        SET_TARGET_PROPERTIES(
          ${arg_TARGET}
          PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${RV_STAGE_BIN_DIR}"
        )
      ENDFOREACH()
    ENDIF()

    ADD_DEPENDENCIES(${arg_TARGET} dependencies)

  ELSEIF(${arg_TYPE} STREQUAL "EXECUTABLE_WITH_PLUGINS")
    GET_TARGET_PROPERTY(_native_target_type ${arg_TARGET} TYPE)
    IF(NOT _native_target_type STREQUAL "EXECUTABLE")
      MESSAGE(FATAL_ERROR "\"${arg_TARGET}\" ${arg_TYPE} should be a EXECUTABLE, not a ${_native_target_type}")
    ENDIF()

    ADD_DEPENDENCIES(executables_with_plugins ${arg_TARGET})

    SET_TARGET_PROPERTIES(
      ${arg_TARGET}
      PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${RV_STAGE_BIN_DIR}"
    )
    IF(RV_TARGET_WINDOWS)
      FOREACH(
        OUTPUTCONFIG
        ${CMAKE_CONFIGURATION_TYPES}
      )
        STRING(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
        SET_TARGET_PROPERTIES(
          ${arg_TARGET}
          PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${RV_STAGE_BIN_DIR}"
        )
      ENDFOREACH()
    ENDIF()

    IF(RV_TARGET_LINUX)
      # Allow plugins that call dlopen to read app symbols
      TARGET_LINK_OPTIONS(${arg_TARGET} PRIVATE "-export-dynamic")
    ENDIF()

    ADD_DEPENDENCIES(
      ${arg_TARGET}
      mu_source_modules
      python_source_modules
      mu_plugins
      python_plugins
      packages
      installed_packages
      movie_formats
      image_formats
      oiio_plugins
      output_plugins
      dependencies
    )

  ELSEIF(${arg_TYPE} STREQUAL "MAIN_EXECUTABLE")
    GET_TARGET_PROPERTY(_native_target_type ${arg_TARGET} TYPE)
    IF(NOT _native_target_type STREQUAL "EXECUTABLE")
      MESSAGE(FATAL_ERROR "\"${arg_TARGET}\" ${arg_TYPE} should be a EXECUTABLE, not a ${_native_target_type}")
    ENDIF()

    ADD_DEPENDENCIES(main_executable ${arg_TARGET})

    IF(RV_TARGET_DARWIN)
      SET_TARGET_PROPERTIES(
        ${arg_TARGET}
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${RV_APP_ROOT}"
                   MACOSX_BUNDLE TRUE
                   OUTPUT_NAME ${_target}
                   MACOSX_BUNDLE_BUNDLE_NAME "${RV_UI_APPLICATION_NAME}"
                   MACOSX_BUNDLE_EXECUTABLE_NAME ${_target}
                   RESOURCE "${arg_FILES}"
                   MACOSX_BUNDLE_GUI_IDENTIFIER "com.autodesk.${_target}"
                   MACOSX_BUNDLE_SHORT_VERSION_STRING "${RV_MAJOR_VERSION}.${RV_MINOR_VERSION}.${RV_REVISION_NUMBER}"
                   MACOSX_BUNDLE_BUNDLE_VERSION "${RV_MAJOR_VERSION}.${RV_MINOR_VERSION}.${RV_REVISION_NUMBER}"
                   MACOSX_BUNDLE_ICON_FILE "${_target}.icns"
                   MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist"
      )
    ELSEIF(RV_TARGET_LINUX)
      # Allow plugins that call dlopen to read app symbols
      TARGET_LINK_OPTIONS(${arg_TARGET} PRIVATE "-export-dynamic")
    ENDIF()

    ADD_DEPENDENCIES(
      ${arg_TARGET}
      mu_source_modules
      python_source_modules
      compiled_python_source_modules
      mu_plugins
      python_plugins
      packages
      installed_packages
      movie_formats
      installed_movie_formats
      image_formats
      installed_image_formats
      oiio_plugins
      output_plugins
      shared_libraries
      executables
      executables_with_plugins
      dependencies
    )
  ELSE()
    MESSAGE(FATAL_ERROR "Unsupported installation type: '${arg_TYPE}' in '${CMAKE_CURRENT_SOURCE_DIR}'")
  ENDIF()

ENDFUNCTION()
