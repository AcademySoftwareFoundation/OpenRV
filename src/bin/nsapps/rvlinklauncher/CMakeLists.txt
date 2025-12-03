INCLUDE(cxx_defaults)

SET(_target
    "RVLinkLauncher"
)

SET(_sources
    RVLinkLauncher.mm
)

ADD_EXECUTABLE(
  ${_target} MACOSX_BUNDLE
  ${_sources}
)

# Set version suffix based on release type
IF(NOT RV_RELEASE_DESCRIPTION STREQUAL "RELEASE")
  SET(RV_VERSION_SUFFIX "-${ORV_GIT_COMMIT_SHORT_HASH_RES}")
ELSE()
  SET(RV_VERSION_SUFFIX "")
ENDIF()

SET_TARGET_PROPERTIES(
  ${_target}
  PROPERTIES MACOSX_BUNDLE TRUE
             OUTPUT_NAME ${_target}
             MACOSX_BUNDLE_BUNDLE_NAME "${_target}"
             MACOSX_BUNDLE_EXECUTABLE_NAME ${_target}
             MACOSX_BUNDLE_GUI_IDENTIFIER "com.autodesk.${_target}.${RV_MAJOR_VERSION}.${RV_MINOR_VERSION}.${RV_REVISION_NUMBER}"
             MACOSX_BUNDLE_SHORT_VERSION_STRING "${RV_MAJOR_VERSION}.${RV_MINOR_VERSION}.${RV_REVISION_NUMBER}"
             MACOSX_BUNDLE_BUNDLE_VERSION "${RV_MAJOR_VERSION}.${RV_MINOR_VERSION}.${RV_REVISION_NUMBER}"
             MACOSX_BUNDLE_COPYRIGHT "Copyright Autodesk ${RV_VERSION_YEAR}"
             MACOSX_BUNDLE_ICON_FILE "${_target}.icns"
             MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
)

TARGET_LINK_LIBRARIES(${_target} "-framework AppKit" "-framework Foundation")

FILE(GLOB _icns_files *.icns)
FOREACH(
  _icns_file
  ${_icns_files}
)
  CONFIGURE_FILE(${_icns_file} ${RVLINKLAUNCHER_RESOURCES_DIR} COPYONLY)
ENDFOREACH()

RV_STAGE(TYPE "EXECUTABLE_WITH_PLUGINS" TARGET ${_target})
