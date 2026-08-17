# CY2025 VFX Reference Platform versions of dependencies see https://vfxplatform.com/

IF(RV_VFX_PLATFORM STREQUAL "CY2025")
  # Year
  SET(RV_VFX_CY_YEAR
      "2025"
  )
  SET(RV_VFX_CY2025
      ON
  )
  ADD_COMPILE_DEFINITIONS(QT65ON)

  # Boost
  SET(RV_DEPS_BOOST_VERSION
      "1.85.0"
  )
  SET(RV_DEPS_BOOST_MAJOR_MINOR_VERSION
      "1_85"
  )
  SET(RV_DEPS_BOOST_DOWNLOAD_HASH
      "53aeccc3167909ee770e34469f8dd592"
  )

  # Imath Can find the build version in OpenRV/_build/RV_DEPS_IMATH/install/lib/
  SET(RV_DEPS_IMATH_VERSION
      "3.1.12"
  )
  SET(RV_DEPS_IMATH_DOWNLOAD_HASH
      "d4059140972da68a2b5a1287ebe5a653"
  )
  SET(RV_DEPS_IMATH_LIB_VER
      "29.11.0"
  )
  SET(RV_DEPS_IMATH_LIB_MAJOR
      "3_1"
  )

  # NumPy https://numpy.org/doc/stable/release.html
  SET(RV_DEPS_NUMPY_VERSION
      "1.26.4"
  )

  # OCIO https://github.com/AcademySoftwareFoundation/OpenColorIO
  SET(RV_DEPS_OCIO_VERSION
      "2.4.2"
  )
  SET(RV_DEPS_OCIO_VERSION_SHORT
      "2_4"
  )
  SET(RV_DEPS_OCIO_DOWNLOAD_HASH
      "1bc8f31a1479ce6518644cdd7df26631"
  )

  # OpenEXR https://github.com/AcademySoftwareFoundation/openexr/releases
  SET(RV_DEPS_OPENEXR_VERSION
      "3.3.6"
  )
  SET(RV_DEPS_OPENEXR_DOWNLOAD_HASH
      "a91a522132384ace0d8ffeb6e60a0732"
  )
  SET(RV_DEPS_OPENEXR_LIBNAME_SUFFIX
      "3_3"
  )
  SET(RV_DEPS_OPENEXR_LIB_VERSION_SUFFIX
      "32.${RV_DEPS_OPENEXR_VERSION}"
  )
  SET(RV_DEPS_OPENEXR_PATCH_NAME
      "openexr_${RV_DEPS_OPENEXR_VERSION}_invalid_to_black"
  )

  # OpenSSL https://github.com/openssl/openssl
  SET(RV_DEPS_OPENSSL_VERSION
      "3.6.2"
  )
  SET(RV_DEPS_OPENSSL_HASH
      "f27e8f53ac612bb0e3e781a45799fb90"
  )
  SET(RV_DEPS_OPENSSL_VERSION_DOT
      ".3"
  )
  SET(RV_DEPS_OPENSSL_VERSION_UNDERSCORE
      "3"
  )

  # PySide
  SET(RV_DEPS_PYSIDE_VERSION
      "6.5.3"
  )
  SET(RV_DEPS_PYSIDE_DOWNLOAD_HASH
      "515d3249c6e743219ff0d7dd25b8c8d8"
  )
  SET(RV_DEPS_PYSIDE_TARGET
      "RV_DEPS_PYSIDE6"
  )
  SET(RV_DEPS_PYSIDE_ARCHIVE_URL
      "https://mirrors.ocf.berkeley.edu/qt/official_releases/QtForPython/pyside6/PySide6-${RV_DEPS_PYSIDE_VERSION}-src/pyside-setup-everywhere-src-${RV_DEPS_PYSIDE_VERSION}.zip"
  )

  # Python https://www.python.org/downloads/source/
  SET(RV_DEPS_PYTHON_VERSION
      "3.11.9"
  )
  SET(RV_DEPS_PYTHON_DOWNLOAD_HASH
      "392eccd4386936ffcc46ed08057db3e7"
  )
  # SET(RV_DEPS_PYTHON_VERSION "3.11.14") SET(RV_DEPS_PYTHON_DOWNLOAD_HASH "5f43ab9d5a74b9ac0dd2e20f58740f9e")

  # python-build-standalone (PBS) pins, used only when RV_DEPS_PYTHON_USE_PBS=ON (Release). PBS release 20240814 is the latest that ships CPython 3.11.9 for all
  # target triples. Hashes are the official .sha256 sidecars published next to each install_only asset. Keep the tag in sync with RV_DEPS_PYTHON_VERSION above.
  SET(RV_DEPS_PYTHON_PBS_RELEASE_TAG
      "20240814"
  )
  SET(RV_DEPS_PYTHON_PBS_HASH_AARCH64_APPLE_DARWIN
      "8760e908f25fdc8a01f4d1b101854ac047b4eacb723fb2593a168fb989c86eef"
  )
  SET(RV_DEPS_PYTHON_PBS_HASH_X86_64_APPLE_DARWIN
      "76073305812c093ce840df9c4c17068aa69da8d951e7376ef48f43376986a13e"
  )
  SET(RV_DEPS_PYTHON_PBS_HASH_X86_64_LINUX_GNU
      "9a332ba354f3b4e8a96a15db6b2805a7a31dcc1b6b9c1b7b93e5246949fbb50f"
  )
  SET(RV_DEPS_PYTHON_PBS_HASH_X86_64_WINDOWS_MSVC
      "4c71d25731214b8a960d1d87510f24179d819249c5b434aaf7135818421b6215"
  )

  # Qt
  SET(RV_DEPS_QT_VERSION
      "6.5.3"
  )
  SET(RV_DEPS_QT_MAJOR
      "6"
  )
ENDIF()
