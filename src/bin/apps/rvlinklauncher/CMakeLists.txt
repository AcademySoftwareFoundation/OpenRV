INCLUDE(cxx_defaults)

SET(_target "RVLinkLauncher")

set(_sources
    RVLinkLauncher.mm
)

add_executable(${_target} MACOSX_BUNDLE ${_sources})

set_target_properties(${_target} PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
    MACOSX_BUNDLE_BUNDLE_NAME "RVLinkLauncher"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.autodesk.RVLinkLauncher"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
    MACOSX_BUNDLE_BUNDLE_VERSION "1.0"
    MACOSX_BUNDLE_COPYRIGHT "Copyright Autodesk 2025"
)

target_link_libraries(${_target}
    "-framework AppKit"
    "-framework Foundation"
)

RV_STAGE(TYPE "EXECUTABLE_WITH_PLUGINS" TARGET ${_target})
