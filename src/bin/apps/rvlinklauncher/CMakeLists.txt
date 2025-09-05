INCLUDE(cxx_defaults)

SET(_target "RVLinkLauncher")

set(_sources
    RVLinkLauncher.mm
)

add_executable(${_target} MACOSX_BUNDLE ${_sources})

set_target_properties(${_target} PROPERTIES
    MACOSX_BUNDLE TRUE
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

target_link_libraries(${_target}
    "-framework AppKit"
    "-framework Foundation"
)

FILE(GLOB _icns_files *.icns)
FOREACH(
  _icns_file
  ${_icns_files}
)
  CONFIGURE_FILE(${_icns_file} ${RVLINKLAUNCHER_RESOURCES_DIR} COPYONLY)
ENDFOREACH()

RV_STAGE(TYPE "EXECUTABLE_WITH_PLUGINS" TARGET ${_target})
