# CY2025 VFX Reference Platform versions of dependencies
# see https://vfxplatform.com/

IF(RV_VFX_PLATFORM STREQUAL "CY2025")
    # Year
    SET(RV_VFX_CY_YEAR "2025")
    SET(RV_VFX_CY2025 ON )
    ADD_COMPILE_DEFINITIONS(QT65ON)

    # Boost
    SET(RV_DEPS_BOOST_VERSION "1.85.0")
    SET(RV_DEPS_BOOST_MAJOR_MINOR_VERSION "1_85")
    SET(RV_DEPS_BOOST_DOWNLOAD_HASH "53aeccc3167909ee770e34469f8dd592")

    # Imath
    # Can find the build version in OpenRV/_build/RV_DEPS_IMATH/install/lib/
    SET(RV_DEPS_IMATH_VERSION "3.1.12")
    SET(RV_DEPS_IMATH_DOWNLOAD_HASH "d4059140972da68a2b5a1287ebe5a653")
    SET(RV_DEPS_IMATH_LIB_VER "29.11.0")
    SET(RV_DEPS_IMATH_LIB_MAJOR "3_1")

    # NumPy
    # https://numpy.org/doc/stable/release.html
    SET(ENV{RV_DEPS_NUMPY_VERSION} "1.26.4")

    # OCIO
    # https://github.com/AcademySoftwareFoundation/OpenColorIO
    SET(RV_DEPS_OCIO_VERSION "2.4.2")
    SET(RV_DEPS_OCIO_VERSION_SHORT "2_4")
    SET(RV_DEPS_OCIO_DOWNLOAD_HASH "1bc8f31a1479ce6518644cdd7df26631")

    # OIIO
    # https://github.com/AcademySoftwareFoundation/OpenImageIO
    SET(RV_DEPS_OIIO_VERSION "2.5.19.1")
    SET(RV_DEPS_OIIO_DOWNLOAD_HASH "5af6de5a73c6d234eed8e2874a5aed62")

    # OpenEXR
    # https://github.com/AcademySoftwareFoundation/openexr/releases
    SET(RV_DEPS_OPENEXR_VERSION "3.3.6")
    SET(RV_DEPS_OPENEXR_DOWNLOAD_HASH "a91a522132384ace0d8ffeb6e60a0732")
    SET(RV_DEPS_OPENEXR_LIBNAME_SUFFIX "3_3")
    SET(RV_DEPS_OPENEXR_LIB_VERSION_SUFFIX "32.${RV_DEPS_OPENEXR_VERSION}")
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
    SET(RV_DEPS_PYTHON_VERSION "3.11.9")
    SET(RV_DEPS_PYTHON_DOWNLOAD_HASH "392eccd4386936ffcc46ed08057db3e7")
    # SET(RV_DEPS_PYTHON_VERSION "3.11.14")
    # SET(RV_DEPS_PYTHON_DOWNLOAD_HASH "5f43ab9d5a74b9ac0dd2e20f58740f9e")

    # Qt
    SET(RV_DEPS_QT_VERSION "6.5.3")
    SET(RV_DEPS_QT_MAJOR "6")
ENDIF()