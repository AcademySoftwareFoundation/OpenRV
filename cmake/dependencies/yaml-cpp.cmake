#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
##################################################################################
#
# Expose OCIO's own 'yaml_cpp' target replacing the legacy 'src/pub/yaml_cpp' folder.
#
##################################################################################
IF(RV_USE_OCIO_YAML_CPP)

    ADD_LIBRARY(yaml_cpp STATIC IMPORTED GLOBAL)
    ADD_DEPENDENCIES(yaml_cpp RV_DEPS_OCIO)
    IF(CMAKE_BUILD_TYPE MATCHES "^Debug$")
        # Here the postfix is "d" and not "_d": the postfix inside OCIO is: "d".
        SET(_debug_postfix "d")
        MESSAGE(DEBUG "Using debug postfix: '${_debug_postfix}'")
    ELSE()
        SET(_debug_postfix "")
    ENDIF()

    IF(RV_TARGET_LINUX)
        SET(_lib_dir
            ${RV_DEPS_OCIO_DIST_DIR}/lib64
        )
    ELSE()
        SET(_lib_dir
            ${RV_DEPS_OCIO_DIST_DIR}/lib
        )
    ENDIF()

    SET_PROPERTY(
        TARGET yaml_cpp
        # Why is it building yaml-cpp debug ?
        PROPERTY IMPORTED_LOCATION ${_lib_dir}/libyaml-cpp${_debug_postfix}.a
    )

    # It is required to force directory creation at configure time
    # otherwise CMake complains about importing a non-existing path
    SET(_yaml_cpp_include_dir ${RV_DEPS_OCIO_DIST_DIR}/include)
    FILE(MAKE_DIRECTORY ${_yaml_cpp_include_dir})
    TARGET_INCLUDE_DIRECTORIES(
        yaml_cpp
        INTERFACE ${_yaml_cpp_include_dir}
    )

    SET(RV_DEPS_YAML_CPP_VERSION
        "0.7.0"
        CACHE INTERNAL "" FORCE
    )    
ENDIF()