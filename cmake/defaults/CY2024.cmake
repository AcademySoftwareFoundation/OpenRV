# CY2024 VFX Reference Platform versions of dependencies
# see https://vfxplatform.com/

IF(RV_VFX_PLATFORM STREQUAL "CY2024")
    # Year
    SET(RV_VFX_CY_YEAR "2024")
    SET(RV_VFX_CY2024 ON )
    ADD_COMPILE_DEFINITIONS(RV_VFX_CY2024)

    # Boost
    SET(RV_DEPS_BOOST_VERSION "1.82.0")
    SET(RV_DEPS_BOOST_MAJOR_MINOR_VERSION "1_82")
    SET(RV_DEPS_BOOST_DOWNLOAD_HASH "f7050f554a65f6a42ece221eaeec1660")

    # Imath
    SET(RV_DEPS_IMATH_VERSION "3.1.12")
    SET(RV_DEPS_IMATH_DOWNLOAD_HASH "d4059140972da68a2b5a1287ebe5a653")

    # NumPy
    # https://numpy.org/doc/stable/release.html
    SET(ENV{RV_DEPS_NUMPY_VERSION} "1.24.4")

    # OCIO
    # https://github.com/AcademySoftwareFoundation/OpenColorIO
    SET(RV_DEPS_OCIO_VERSION "2.3.2")
    SET(RV_DEPS_OCIO_VERSION_SHORT "2_3")
    SET(RV_DEPS_OCIO_DOWNLOAD_HASH "9eb7834a7cc66b14f0251b7673be0d81")

    # OIIO
    # https://github.com/AcademySoftwareFoundation/OpenImageIO
    SET(RV_DEPS_OIIO_VERSION "2.4.12.0")
    SET(RV_DEPS_OIIO_DOWNLOAD_HASH "5af6de5a73c6d234eed8e2874a5aed62")

    # OpenEXR
    # https://github.com/AcademySoftwareFoundation/openexr/releases
    SET(RV_DEPS_OPENEXR_VERSION "3.2.5")
    SET(RV_DEPS_OPENEXR_DOWNLOAD_HASH "838dfec3bb2a60fee02cc4f0378b6a5c")
    SET(RV_DEPS_OPENEXR_LIBNAME_SUFFIX "3_2")
    SET(RV_DEPS_OPENEXR_LIB_VERSION_SUFFIX "31.${RV_DEPS_OPENEXR_VERSION}")
    SET(RV_DEPS_OPENEXR_PATCH_NAME "openexr_${RV_DEPS_OPENEXR_VERSION}_invalid_to_black")

    # OpenSSL
    # https://github.com/openssl/openssl
    SET(RV_DEPS_OPENSSL_VERSION "3.4.0")
    SET(RV_DEPS_OPENSSL_HASH "34733f7be2d60ecd8bd9ddb796e182af")
    SET(RV_DEPS_OPENSSL_VERSION_DOT ".3")
    SET(RV_DEPS_OPENSSL_VERSION_UNDERSCORE "3")

    # OTIO
    # https://github.com/AcademySoftwareFoundation/OpenTimelineIO
    SET(RV_DEPS_OTIO_VERSION "0.18.1")
    
    # PySide
    SET(RV_DEPS_PYSIDE_VERSION "6.5.3")
    SET(RV_DEPS_PYSIDE_DOWNLOAD_HASH "515d3249c6e743219ff0d7dd25b8c8d8")
    SET(RV_DEPS_PYSIDE_TARGET "RV_DEPS_PYSIDE6")
    SET(RV_DEPS_PYSIDE_ARCHIVE_URL "https://mirrors.ocf.berkeley.edu/qt/official_releases/QtForPython/pyside6/PySide6-${RV_DEPS_PYSIDE_VERSION}-src/pyside-setup-everywhere-src-${RV_DEPS_PYSIDE_VERSION}.zip")

    # Python
    # https://www.python.org/downloads/source/
    SET(RV_DEPS_PYTHON_VERSION "3.11.14")
    SET(RV_DEPS_PYTHON_DOWNLOAD_HASH "5f43ab9d5a74b9ac0dd2e20f58740f9e")

    # Qt
    SET(RV_DEPS_QT_VERSION "6.5.3")
ENDIF()