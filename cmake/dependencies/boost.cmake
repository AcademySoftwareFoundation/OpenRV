#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# Modified for the Visto project. Copyright (C) 2026  Makai Systems. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

FIND_PACKAGE(
  Boost REQUIRED
  COMPONENTS atomic
             chrono
             date_time
             filesystem
             graph
             iostreams
             locale
             program_options
             random
             regex
             serialization
             thread
             timer
)

IF(TARGET Boost::headers)
  SET_PROPERTY(
    TARGET Boost::headers
    PROPERTY IMPORTED_GLOBAL TRUE
  )
ELSEIF(NOT TARGET Boost::headers)
  ADD_LIBRARY(Boost::headers INTERFACE IMPORTED GLOBAL)
  TARGET_INCLUDE_DIRECTORIES(
    Boost::headers
    INTERFACE ${Boost_INCLUDE_DIRS}
  )
ENDIF()

# Promote component targets to global
FOREACH(
  _lib
  atomic
  chrono
  date_time
  filesystem
  graph
  iostreams
  locale
  program_options
  random
  regex
  serialization
  thread
  timer
)
  IF(TARGET Boost::${_lib})
    SET_PROPERTY(
      TARGET Boost::${_lib}
      PROPERTY IMPORTED_GLOBAL TRUE
    )
  ENDIF()
ENDFOREACH()

# Provide a dummy Boost::system target since modern Boost has dropped it, but Visto still references it
IF(NOT TARGET Boost::system)
  ADD_LIBRARY(Boost::system INTERFACE IMPORTED GLOBAL)
ENDIF()

IF(DEFINED Boost_VERSION)
  SET(RV_DEPS_BOOST_VERSION
      "${Boost_VERSION}"
  )
  SET(RV_DEPS_BOOST_VERSION
      "${Boost_VERSION}"
      CACHE STRING "" FORCE
  )
ELSEIF(DEFINED BOOST_VERSION)
  SET(RV_DEPS_BOOST_VERSION
      "${BOOST_VERSION}"
  )
  SET(RV_DEPS_BOOST_VERSION
      "${BOOST_VERSION}"
      CACHE STRING "" FORCE
  )
ELSEIF(DEFINED Boost_VERSION_STRING)
  SET(RV_DEPS_BOOST_VERSION
      "${Boost_VERSION_STRING}"
  )
  SET(RV_DEPS_BOOST_VERSION
      "${Boost_VERSION_STRING}"
      CACHE STRING "" FORCE
  )
ELSEIF(DEFINED BOOST_VERSION_STRING)
  SET(RV_DEPS_BOOST_VERSION
      "${BOOST_VERSION_STRING}"
  )
  SET(RV_DEPS_BOOST_VERSION
      "${BOOST_VERSION_STRING}"
      CACHE STRING "" FORCE
  )
ENDIF()
