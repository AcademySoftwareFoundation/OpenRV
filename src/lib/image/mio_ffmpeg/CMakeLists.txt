#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "mio_ffmpeg"
)

LIST(APPEND _sources init.cpp)

ADD_LIBRARY(
  ${_target} SHARED
  ${_sources}
)
# mio_ffmpeg and the external build plus the CTest for the external build depend on FFMPEG (and boost) However, cmake/dependencies is built before but we
# emphasize here the dependency.
ADD_DEPENDENCIES(${_target} ffmpeg::avcodec)

TARGET_LINK_LIBRARIES(
  ${_target}
  PRIVATE MovieFFMpeg Boost::program_options TwkFB
)

IF(RV_TARGET_WINDOWS)
  TARGET_COMPILE_OPTIONS(
    ${_target}
    PRIVATE "-D__STDC_CONSTANT_MACROS"
  )
ENDIF()

SET(IMAGE_MOVIE_FFMPEG_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/../MovieFFMpeg/MovieFFMpeg"
)

SET(BASE_STL_EXT_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/../../base/stl_ext/stl_ext"
)

SET(IMAGE_TWEAK_TWKFB_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/../TwkFB/TwkFB"
)

SET(AUDIO_TWEAK_TWKAUDIO_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/../../audio/TwkAudio/TwkAudio"
)

SET(BASE_TWEAK_TWKEXC_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/../../base/TwkExc/TwkExc"
)

SET(BASE_TWEAK_TWKMATH_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/../../base/TwkMath/TwkMath"
)

SET(IMAGE_TWEAK_TWKMOVIE_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/../TwkMovie/TwkMovie"
)

# Includes for Ext. Deps

SET(EXT_PUB_NEDMALLOC_INCLUDE_ROOT
    ${PROJECT_SOURCE_DIR}/src/pub/nedmalloc
)
SET(EXT_PUB_NEDMALLOC_HEADER_FILE
    ${EXT_PUB_NEDMALLOC_INCLUDE_ROOT}/nedmalloc.h
)

SET(EXT_PUB_WIN_PTHREAD_INCLUDE_ROOT
    ${PROJECT_SOURCE_DIR}/src/pub/win_pthreads
)
FILE(GLOB EXT_PUB_WIN_PTHREAD_HEADER_FILES ${EXT_PUB_WIN_PTHREAD_INCLUDE_ROOT}/*.h)

SET(EXT_IMATH_INCLUDE_ROOT
    "${RV_DEPS_BASE_DIR}/RV_DEPS_IMATH/install/include"
)

IF(RV_TARGET_WINDOWS)
  SET(EXT_BOOST_INCLUDE_ROOT
      "${RV_DEPS_BASE_DIR}/RV_DEPS_BOOST/install/include/boost-${RV_DEPS_BOOST_MAJOR_MINOR_VERSION}/boost"
  )
ELSE()
  SET(EXT_BOOST_INCLUDE_ROOT
      "${RV_DEPS_BASE_DIR}/RV_DEPS_BOOST/install/include/boost"
  )
ENDIF()
SET(EXT_BOOST_PROGOPTIONS_HEADERS
    "${EXT_BOOST_INCLUDE_ROOT}/program_options"
)
SET(EXT_BOOST_PROGOPTIONS_MAIN_HEADER
    "${EXT_BOOST_INCLUDE_ROOT}/program_options.hpp"
)

# Stage the files in this folder so they can be rebuilt externally.
SET(STAGE_MIO_FFMPEG
    "${RV_STAGE_SRC_DIR}/${_target}"
)
SET(STAGE_INCLUDE_ROOT
    "${STAGE_MIO_FFMPEG}/includes"
)

SET(STAGE_MOVIE_FFMPEG_HEADERS
    "${STAGE_INCLUDE_ROOT}/MovieFFMpeg"
)

SET(STAGE_STL_EXT_HEADERS
    "${STAGE_INCLUDE_ROOT}/stl_ext"
)

SET(STAGE_TWEAK_TWKFB_HEADERS
    "${STAGE_INCLUDE_ROOT}/TwkFB"
)

SET(STAGE_TWEAK_TWKAudio_HEADERS
    "${STAGE_INCLUDE_ROOT}/TwkAudio"
)

SET(STAGE_TWEAK_TWKExc_HEADERS
    "${STAGE_INCLUDE_ROOT}/TwkExc"
)

SET(STAGE_TWEAK_TWKMath_HEADERS
    "${STAGE_INCLUDE_ROOT}/TwkMath"
)

SET(STAGE_TWEAK_TWKMovie_HEADERS
    "${STAGE_INCLUDE_ROOT}/TwkMovie"
)

# Staging Include loc for Ext Deps
SET(STAGE_BOOST_HEADERS
    "${STAGE_INCLUDE_ROOT}/boost"
)
SET(STAGE_BOOST_PROGOPTIONS_HEADERS
    "${STAGE_BOOST_HEADERS}/program_options/"
)
SET(STAGE_IMATH_HEADERS
    "${STAGE_INCLUDE_ROOT}"
)
SET(STAGE_WIN_PTHREADS_HEADERS
    "${STAGE_INCLUDE_ROOT}/win_pthreads"
)
SET(STAGE_FFMPEG_LIBS
    "${RV_STAGE_SRC_DIR}/${_target}/lib"
)

FILE(MAKE_DIRECTORY ${STAGE_FFMPEG_LIBS})
# Copy all files below in order to build mio_ffmpeg externally
ADD_CUSTOM_COMMAND(
  TARGET ${_target}
  POST_BUILD
  COMMENT "Copying all ${_target} (${CMAKE_CURRENT_LIST_DIR}) files and dependencies into '${RV_STAGE_PLUGINS_PYTHON_DIR}'."
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR} ${STAGE_MIO_FFMPEG}
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/ext_build/CMakeLists.txt ${STAGE_MIO_FFMPEG}/CMakeLists.txt
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/ext_build/README.md ${STAGE_MIO_FFMPEG}/README.md
  COMMAND ${CMAKE_COMMAND} -E rm -rf ${STAGE_MIO_FFMPEG}/ext_build
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${IMAGE_MOVIE_FFMPEG_HEADERS} ${STAGE_MOVIE_FFMPEG_HEADERS}
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${BASE_STL_EXT_HEADERS} ${STAGE_STL_EXT_HEADERS}
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${IMAGE_TWEAK_TWKFB_HEADERS} ${STAGE_TWEAK_TWKFB_HEADERS}
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${AUDIO_TWEAK_TWKAUDIO_HEADERS} ${STAGE_TWEAK_TWKAudio_HEADERS}
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${BASE_TWEAK_TWKEXC_HEADERS} ${STAGE_TWEAK_TWKExc_HEADERS}
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${BASE_TWEAK_TWKMATH_HEADERS} ${STAGE_TWEAK_TWKMath_HEADERS}
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${IMAGE_TWEAK_TWKMOVIE_HEADERS} ${STAGE_TWEAK_TWKMovie_HEADERS}
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${EXT_IMATH_INCLUDE_ROOT} ${STAGE_IMATH_HEADERS}
  COMMAND ${CMAKE_COMMAND} -E copy ${EXT_BOOST_PROGOPTIONS_MAIN_HEADER} "${STAGE_BOOST_HEADERS}/program_options.hpp"
  COMMAND ${CMAKE_COMMAND} -E copy ${EXT_BOOST_INCLUDE_ROOT}/config.hpp ${STAGE_BOOST_HEADERS}
  COMMAND ${CMAKE_COMMAND} -E copy ${EXT_BOOST_INCLUDE_ROOT}/version.hpp ${STAGE_BOOST_HEADERS}
  COMMAND ${CMAKE_COMMAND} -E copy ${EXT_BOOST_INCLUDE_ROOT}/any.hpp ${STAGE_BOOST_HEADERS}
  COMMAND ${CMAKE_COMMAND} -E copy ${EXT_BOOST_INCLUDE_ROOT}/array.hpp ${STAGE_BOOST_HEADERS}
  COMMAND ${CMAKE_COMMAND} -E copy ${EXT_BOOST_INCLUDE_ROOT}/type_index.hpp ${STAGE_BOOST_HEADERS}
  COMMAND ${CMAKE_COMMAND} -E copy ${EXT_BOOST_INCLUDE_ROOT}/type.hpp ${STAGE_BOOST_HEADERS}
  COMMAND ${CMAKE_COMMAND} -E copy ${EXT_BOOST_INCLUDE_ROOT}/static_assert.hpp ${STAGE_BOOST_HEADERS}
  COMMAND ${CMAKE_COMMAND} -E copy ${EXT_BOOST_INCLUDE_ROOT}/throw_exception.hpp ${STAGE_BOOST_HEADERS}
  COMMAND ${CMAKE_COMMAND} -E copy ${EXT_BOOST_INCLUDE_ROOT}/assert.hpp ${STAGE_BOOST_HEADERS}
  COMMAND ${CMAKE_COMMAND} -E copy ${EXT_BOOST_INCLUDE_ROOT}/current_function.hpp ${STAGE_BOOST_HEADERS}
  COMMAND ${CMAKE_COMMAND} -E copy ${EXT_BOOST_INCLUDE_ROOT}/cstdint.hpp ${STAGE_BOOST_HEADERS}
  COMMAND ${CMAKE_COMMAND} -E copy ${EXT_BOOST_INCLUDE_ROOT}/function.hpp ${STAGE_BOOST_HEADERS}
  COMMAND ${CMAKE_COMMAND} -E copy ${EXT_BOOST_INCLUDE_ROOT}/function_equal.hpp ${STAGE_BOOST_HEADERS}
  COMMAND ${CMAKE_COMMAND} -E copy ${EXT_BOOST_INCLUDE_ROOT}/limits.hpp ${STAGE_BOOST_HEADERS}
  COMMAND ${CMAKE_COMMAND} -E copy ${EXT_BOOST_INCLUDE_ROOT}/integer.hpp ${STAGE_BOOST_HEADERS}
  COMMAND ${CMAKE_COMMAND} -E copy ${EXT_BOOST_INCLUDE_ROOT}/integer_fwd.hpp ${STAGE_BOOST_HEADERS}
  COMMAND ${CMAKE_COMMAND} -E copy ${EXT_BOOST_INCLUDE_ROOT}/integer_traits.hpp ${STAGE_BOOST_HEADERS}
  COMMAND ${CMAKE_COMMAND} -E copy ${EXT_BOOST_INCLUDE_ROOT}/ref.hpp ${STAGE_BOOST_HEADERS}
  COMMAND ${CMAKE_COMMAND} -E copy ${EXT_BOOST_INCLUDE_ROOT}/lexical_cast.hpp ${STAGE_BOOST_HEADERS}
  COMMAND ${CMAKE_COMMAND} -E copy ${EXT_BOOST_INCLUDE_ROOT}/mem_fn.hpp ${STAGE_BOOST_HEADERS}
  COMMAND ${CMAKE_COMMAND} -E copy ${EXT_BOOST_INCLUDE_ROOT}/utility.hpp ${STAGE_BOOST_HEADERS}
  COMMAND ${CMAKE_COMMAND} -E copy ${EXT_BOOST_INCLUDE_ROOT}/next_prior.hpp ${STAGE_BOOST_HEADERS}
  COMMAND ${CMAKE_COMMAND} -E copy ${EXT_BOOST_INCLUDE_ROOT}/get_pointer.hpp ${STAGE_BOOST_HEADERS}
  COMMAND ${CMAKE_COMMAND} -E copy ${EXT_BOOST_INCLUDE_ROOT}/checked_delete.hpp ${STAGE_BOOST_HEADERS}
  COMMAND ${CMAKE_COMMAND} -E copy ${EXT_BOOST_INCLUDE_ROOT}/concept_check.hpp ${STAGE_BOOST_HEADERS}
  COMMAND ${CMAKE_COMMAND} -E copy ${EXT_BOOST_INCLUDE_ROOT}/swap.hpp ${STAGE_BOOST_HEADERS}
  COMMAND ${CMAKE_COMMAND} -E copy ${EXT_BOOST_INCLUDE_ROOT}/noncopyable.hpp ${STAGE_BOOST_HEADERS}
  COMMAND ${CMAKE_COMMAND} -E copy ${EXT_BOOST_INCLUDE_ROOT}/shared_ptr.hpp ${STAGE_BOOST_HEADERS}
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${EXT_BOOST_PROGOPTIONS_HEADERS} ${STAGE_BOOST_PROGOPTIONS_HEADERS}
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${EXT_BOOST_INCLUDE_ROOT}/config ${STAGE_BOOST_HEADERS}/config
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${EXT_BOOST_INCLUDE_ROOT}/type_index ${STAGE_BOOST_HEADERS}/type_index
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${EXT_BOOST_INCLUDE_ROOT}/container_hash ${STAGE_BOOST_HEADERS}/container_hash
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${EXT_BOOST_INCLUDE_ROOT}/exception ${STAGE_BOOST_HEADERS}/exception
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${EXT_BOOST_INCLUDE_ROOT}/assert ${STAGE_BOOST_HEADERS}/assert
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${EXT_BOOST_INCLUDE_ROOT}/bind ${STAGE_BOOST_HEADERS}/bind
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${EXT_BOOST_INCLUDE_ROOT}/core ${STAGE_BOOST_HEADERS}/core
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${EXT_BOOST_INCLUDE_ROOT}/detail ${STAGE_BOOST_HEADERS}/detail
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${EXT_BOOST_INCLUDE_ROOT}/type_traits ${STAGE_BOOST_HEADERS}/type_traits
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${EXT_BOOST_INCLUDE_ROOT}/integer ${STAGE_BOOST_HEADERS}/integer
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${EXT_BOOST_INCLUDE_ROOT}/utility ${STAGE_BOOST_HEADERS}/utility
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${EXT_BOOST_INCLUDE_ROOT}/function ${STAGE_BOOST_HEADERS}/function
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${EXT_BOOST_INCLUDE_ROOT}/preprocessor ${STAGE_BOOST_HEADERS}/preprocessor
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${EXT_BOOST_INCLUDE_ROOT}/lexical_cast ${STAGE_BOOST_HEADERS}/lexical_cast
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${EXT_BOOST_INCLUDE_ROOT}/move ${STAGE_BOOST_HEADERS}/move
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${EXT_BOOST_INCLUDE_ROOT}/range ${STAGE_BOOST_HEADERS}/range
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${EXT_BOOST_INCLUDE_ROOT}/iterator ${STAGE_BOOST_HEADERS}/iterator
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${EXT_BOOST_INCLUDE_ROOT}/mpl ${STAGE_BOOST_HEADERS}/mpl
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${EXT_BOOST_INCLUDE_ROOT}/concept ${STAGE_BOOST_HEADERS}/concept
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${EXT_BOOST_INCLUDE_ROOT}/numeric ${STAGE_BOOST_HEADERS}/numeric
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${EXT_BOOST_INCLUDE_ROOT}/container ${STAGE_BOOST_HEADERS}/container
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${EXT_BOOST_INCLUDE_ROOT}/smart_ptr ${STAGE_BOOST_HEADERS}/smart_ptr
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${EXT_BOOST_INCLUDE_ROOT}/algorithm ${STAGE_BOOST_HEADERS}/algorithm
  COMMAND ${CMAKE_COMMAND} -E copy ${RV_STAGE_LIB_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}MovieFFMpeg${CMAKE_STATIC_LIBRARY_SUFFIX} ${STAGE_FFMPEG_LIBS}
  COMMAND ${CMAKE_COMMAND} -E copy ${RV_STAGE_LIB_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}mp4v2Utils${CMAKE_STATIC_LIBRARY_SUFFIX} ${STAGE_FFMPEG_LIBS}
)

IF(RV_TARGET_WINDOWS
   OR RV_TARGET_LINUX
)
  ADD_CUSTOM_COMMAND(
    TARGET ${_target}
    POST_BUILD
    COMMENT "Copying Nedmalloc header..."
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EXT_PUB_NEDMALLOC_HEADER_FILE} ${STAGE_INCLUDE_ROOT}/nedmalloc.h
  )
  IF(RV_TARGET_WINDOWS)
    IF(NOT EXISTS ${STAGE_WIN_PTHREADS_HEADERS})
      FILE(MAKE_DIRECTORY ${STAGE_WIN_PTHREADS_HEADERS})
    ENDIF()
    ADD_CUSTOM_COMMAND(
      TARGET ${_target}
      POST_BUILD
      COMMENT "Copying Windows-only files (win_pthreads)..."
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EXT_PUB_WIN_PTHREAD_HEADER_FILES} ${STAGE_WIN_PTHREADS_HEADERS}
    )
  ENDIF()
ENDIF()

RV_STAGE(TYPE "MOVIE_FORMAT" TARGET ${_target})

SET(_test_base_location
    ${RV_DEPS_BASE_DIR}/src/lib/image/mio_ffmpeg
)

SET(_test_location
    ${_test_base_location}/test_workspace
)

IF(NOT EXISTS (${_test_location}))
  FILE(MAKE_DIRECTORY ${_test_location})
ENDIF()

SET(_test_build_dir_location
    ${_test_location}/test_build
)

# When CTest is called to run this test, the source code will be copied in this test directory (_test_location). Moreover, a build folder
# (_test_build_dir_location) will be created before CTest calls the test
IF(RV_TARGET_WINDOWS)
  ADD_TEST(
    MIO_FFMPEG_EXT_BUILD
    ${CMAKE_CTEST_COMMAND}
    --build-and-test
    ${_test_location}
    ${_test_build_dir_location}
    --build-generator
    ${CMAKE_GENERATOR}
    --build-noclean
    --build-config
    ${CMAKE_BUILD_TYPE}
  )
ELSE()
  ADD_TEST(
    MIO_FFMPEG_EXT_BUILD
    ${CMAKE_CTEST_COMMAND}
    --build-and-test
    ${_test_location}
    ${_test_build_dir_location}
    --build-generator
    "Ninja"
    --build-config
    ${CMAKE_BUILD_TYPE}
    --build-makeprogram
    ${CMAKE_MAKE_PROGRAM}
    VERBOSE=1
    --build-options
    "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
  )
ENDIF()

# Run the test in Post Build, use a EnvVar to set Lib/Bin search path to the Stage folder
ADD_CUSTOM_COMMAND(
  TARGET ${_target}
  POST_BUILD
  COMMENT "Running MIO_FFMPEG Test: Copying the external build files and start CTest"
  COMMAND ${CMAKE_COMMAND} -E rm -rf ${_test_build_dir_location}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${_test_build_dir_location}
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${RV_STAGE_SRC_DIR}/${_target} ${_test_location}
  COMMAND ${CMAKE_COMMAND} -E env MIO_DEP_ROOT_DIR=${RV_STAGE_ROOT_DIR} ${CMAKE_CTEST_COMMAND}
  WORKING_DIRECTORY ${_test_base_location}
)
