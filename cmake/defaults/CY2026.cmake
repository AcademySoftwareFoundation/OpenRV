# CY2026 VFX Reference Platform versions of dependencies
# see https://vfxplatform.com/

IF(RV_VFX_PLATFORM STREQUAL "CY2026")
    # Year
    SET(RV_VFX_CY_YEAR "2026")
    SET(RV_VFX_CY2026 ON )
    ADD_COMPILE_DEFINITIONS(QT65ON)

    # Boost
    SET(RV_DEPS_BOOST_VERSION "1.88.0")
    SET(RV_DEPS_BOOST_MAJOR_MINOR_VERSION "1_88")
    SET(RV_DEPS_BOOST_DOWNLOAD_HASH "6cd58b3cc890e4fbbc036c7629129e18")

    # Imath
    # Can find the build version in OpenRV/_build/RV_DEPS_IMATH/install/lib/
    SET(RV_DEPS_IMATH_VERSION "3.2.2")
    SET(RV_DEPS_IMATH_DOWNLOAD_HASH "d9c3aadc25a7d47a893b649787e59a44")
    SET(RV_DEPS_IMATH_LIB_VER "30.${RV_DEPS_IMATH_VERSION}")
    SET(RV_DEPS_IMATH_LIB_MAJOR "3_2")

    # NumPy
    # https://numpy.org/doc/stable/release.html
    SET(ENV{RV_DEPS_NUMPY_VERSION} "2.3.0")

    # OCIO
    # https://github.com/AcademySoftwareFoundation/OpenColorIO
    SET(RV_DEPS_OCIO_VERSION "2.5.0")
    SET(RV_DEPS_OCIO_VERSION_SHORT "2_5")
    SET(RV_DEPS_OCIO_DOWNLOAD_HASH "fd402ea99fd2c4e5b43ea31b4a3387df")

    # OpenEXR
    # https://github.com/AcademySoftwareFoundation/openexr/releases
    SET(RV_DEPS_OPENEXR_VERSION "3.4.3")
    SET(RV_DEPS_OPENEXR_DOWNLOAD_HASH "c11676598aa27a01a1cd21ad75b72e44")
    SET(RV_DEPS_OPENEXR_LIBNAME_SUFFIX "3_4")
    SET(RV_DEPS_OPENEXR_LIB_VERSION_SUFFIX "33.${RV_DEPS_OPENEXR_VERSION}")
    SET(RV_DEPS_OPENEXR_PATCH_NAME "openexr_${RV_DEPS_OPENEXR_VERSION}_invalid_to_black")

    # OpenSSL
    # https://github.com/openssl/openssl
    SET(RV_DEPS_OPENSSL_VERSION "3.6.0")
    SET(RV_DEPS_OPENSSL_HASH "77ab78417082f22a2ce809898bd44da0")
    SET(RV_DEPS_OPENSSL_VERSION_DOT ".3")
    SET(RV_DEPS_OPENSSL_VERSION_UNDERSCORE "3")

    # OTIO
    # https://github.com/AcademySoftwareFoundation/OpenTimelineIO
    SET(RV_DEPS_OTIO_VERSION "0.18.1")

    # PySide
    SET(RV_DEPS_PYSIDE_VERSION "6.8.3")
    SET(RV_DEPS_PYSIDE_DOWNLOAD_HASH "2a81028f5896edeb9c2a80adac3a8e68")
    SET(RV_DEPS_PYSIDE_TARGET "RV_DEPS_PYSIDE6")
    SET(RV_DEPS_PYSIDE_ARCHIVE_URL "https://mirrors.ocf.berkeley.edu/qt/official_releases/QtForPython/pyside6/PySide6-${RV_DEPS_PYSIDE_VERSION}-src/pyside-setup-everywhere-src-${RV_DEPS_PYSIDE_VERSION}.zip")

    # Python
    # https://www.python.org/downloads/source/
    SET(RV_DEPS_PYTHON_VERSION "3.13.9")
    SET(RV_DEPS_PYTHON_DOWNLOAD_HASH "922596355aaa82f1f431fd88e114310d")

    # Qt
    SET(RV_DEPS_QT_VERSION "6.8.3")
    SET(RV_DEPS_QT_MAJOR "6")
ENDIF()