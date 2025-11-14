# CY2023 VFX Reference Platform versions of dependencies
# see https://vfxplatform.com/

IF(RV_VFX_PLATFORM STREQUAL "CY2023")
    # Year
    SET(RV_VFX_CY_YEAR "2023")
    SET(RV_VFX_CY2023 ON )
    ADD_COMPILE_DEFINITIONS(RV_VFX_CY2023)

    # Boost
    SET(RV_DEPS_BOOST_DOWNLOAD_HASH "077f074743ea7b0cb49c6ed43953ae95")
    SET(RV_DEPS_BOOST_MAJOR_MINOR_VERSION "1_80")
    SET(RV_DEPS_BOOST_VERSION "1.80.0")

    # Special case for XCode 15 on macOS
    if(RV_TARGET_DARWIN)
        execute_process(
            COMMAND xcrun clang --version
            OUTPUT_VARIABLE CLANG_FULL_VERSION_STRING
        )
        string(
            REGEX
            REPLACE ".*clang version ([0-9]+\\.[0-9]+).*" "\\1" CLANG_VERSION_STRING ${CLANG_FULL_VERSION_STRING}
        )
        if(CLANG_VERSION_STRING VERSION_GREATER_EQUAL 15.0)
            message(STATUS "Clang version ${CLANG_VERSION_STRING} is not compatible with Boost 1.80, using Boost 1.81 instead. "
                            "Install XCode 14.3.1 if you absolutely want to use Boost version 1.80 as per VFX reference platform CY2023"
            )
            SET(RV_DEPS_BOOST_VERSION "1.81.0")
            SET(RV_DEPS_BOOST_MAJOR_MINOR_VERSION "1_81")
            SET(RV_DEPS_BOOST_DOWNLOAD_HASH "4bf02e84afb56dfdccd1e6aec9911f4b")
        endif()
    endif()

    # Imath
    # Can find the build version in OpenRV/_build/RV_DEPS_IMATH/install/lib/
    SET(RV_DEPS_IMATH_VERSION "3.1.12")
    SET(RV_DEPS_IMATH_DOWNLOAD_HASH "d4059140972da68a2b5a1287ebe5a653")
    SET(RV_DEPS_IMATH_LIB_VER "29.11.0")
    SET(RV_DEPS_IMATH_LIB_MAJOR "3_1")

    # NumPy
    # NumPY for CY2023 is 1.23.x series but Pyside2 requires < 1.23
    # So we comment this out and make_pyside.py hardcoded to use NumPy < 1.23
    #SET(ENV{RV_DEPS_NUMPY_VERSION} "1.23.5")

    # OCIO
    # https://github.com/AcademySoftwareFoundation/OpenColorIO
    SET(RV_DEPS_OCIO_VERSION "2.2.1")
    SET(RV_DEPS_OCIO_VERSION_SHORT "2_2")
    SET(RV_DEPS_OCIO_DOWNLOAD_HASH "d337d7cc890c6a04ad725556c2b7fb4c")

    # OpenEXR
    # https://github.com/AcademySoftwareFoundation/openexr/releases
    SET(RV_DEPS_OPENEXR_VERSION "3.1.13")
    SET(RV_DEPS_OPENEXR_DOWNLOAD_HASH "bbb385d52695502ea47303a2810a8bc1")
    SET(RV_DEPS_OPENEXR_LIBNAME_SUFFIX "3_1")
    SET(RV_DEPS_OPENEXR_LIB_VERSION_SUFFIX "30.13.1")
    SET(RV_DEPS_OPENEXR_PATCH_NAME "openexr_${RV_DEPS_OPENEXR_VERSION}_invalid_to_black")

    # OpenSSL
    # https://github.com/openssl/openssl
    SET(RV_DEPS_OPENSSL_HASH "72f7ba7395f0f0652783ba1089aa0dcc")
    SET(RV_DEPS_OPENSSL_VERSION "1.1.1u")
    SET(RV_DEPS_OPENSSL_VERSION_DOT ".1.1")
    SET(RV_DEPS_OPENSSL_VERSION_UNDERSCORE "1_1")

    # PySide
    SET(RV_DEPS_PYSIDE_TARGET "RV_DEPS_PYSIDE2")
    SET(RV_DEPS_PYSIDE_VERSION "5.15.10")
    SET(RV_DEPS_PYSIDE_DOWNLOAD_HASH "87841aaced763b6b52e9b549e31a493f")
    # SET(RV_DEPS_PYSIDE_VERSION "5.15.18")
    # SET(RV_DEPS_PYSIDE_DOWNLOAD_HASH "52aa32613f7a69ff46e38ed8a427eb38")
    SET(RV_DEPS_PYSIDE_ARCHIVE_URL "https://mirrors.ocf.berkeley.edu/qt/official_releases/QtForPython/pyside2/PySide2-${RV_DEPS_PYSIDE_VERSION}-src/pyside-setup-opensource-src-${RV_DEPS_PYSIDE_VERSION}.zip")

    # Python
    # https://www.python.org/downloads/source/
    SET(RV_DEPS_PYTHON_VERSION "3.10.13")
    SET(RV_DEPS_PYTHON_DOWNLOAD_HASH "21b32503f31386b37f0c42172dfe5637")
    # SET(RV_DEPS_PYTHON_VERSION "3.10.18")
    # SET(RV_DEPS_PYTHON_DOWNLOAD_HASH "e381359208e2bd0485169656b50ff24c")

    # Qt
    SET(RV_DEPS_QT_VERSION "5.15.10")
    SET(RV_DEPS_QT_MAJOR "5")
ENDIF()