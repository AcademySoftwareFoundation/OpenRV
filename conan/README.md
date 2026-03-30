# Conan Integration for OpenRV

This directory contains Conan 2.x profiles for building OpenRV with pre-built dependencies instead of building everything from source.

## Overview

OpenRV's find-first dependency system (`RV_DEPS_PREFER_INSTALLED=ON`) uses `find_package()` to locate pre-installed libraries before falling back to building from source. Conan provides these libraries via its `CMakeDeps` generator, which creates CMake config files that `find_package()` discovers automatically through the Conan toolchain.

Dependencies **not** managed by Conan (built from source or found from the system):

- Python, PySide6, FFmpeg, OCIO, OIIO — built from source via ExternalProject
- libtiff — force-built from source (OpenRV uses private headers)
- GLEW — found from system/Homebrew, or built from source (not available on Conan Center at required version 2.3.1)
- LibRaw — managed by Conan with `with_jpeg=libjpeg-turbo` and `with_jasper=False`

## Directory Structure

```
conan/
├── profiles/
│   ├── common                    # Shared settings (openssl override, libjpeg replacement)
│   ├── arm64_apple_debug         # macOS ARM64 Debug
│   ├── arm64_apple_release       # macOS ARM64 Release
│   ├── x86_64_apple_debug        # macOS x86_64 Debug
│   ├── x86_64_apple_release      # macOS x86_64 Release
│   ├── x86_64_rocky8             # Linux Rocky 8 Release
│   ├── x86_64_rocky8_debug       # Linux Rocky 8 Debug
│   ├── x86_64_windows            # Windows x64 Release
│   └── x86_64_windows_debug      # Windows x64 Debug
└── README.md
```

## Quick Start

### Prerequisites

- Conan 2.x (`pip install conan>=2.0`)
- CMake 3.24+
- Qt 6.5.3 (set `QT_HOME` environment variable)

### Build

```bash
# 1. Export the openrvcore python-requires base class
conan export openrvcore-conanfile.py

# 2. Install dependencies (downloads/builds missing packages)
conan install conanfile.py --build=missing -pr:a conan/profiles/<your-profile> -of build/Release

# 3. Build OpenRV
conan build conanfile.py -of build/Release -pr:a conan/profiles/<your-profile>
```

### Platform Examples

#### macOS (Apple Silicon, Debug)

```bash
conan export openrvcore-conanfile.py
conan install conanfile.py --build=missing -pr:a conan/profiles/arm64_apple_debug -of build/Release
conan build conanfile.py -of build/Release -pr:a conan/profiles/arm64_apple_debug
```

#### Linux (Rocky 8)

```bash
conan export openrvcore-conanfile.py
conan install conanfile.py --build=missing -pr:a conan/profiles/x86_64_rocky8 -of build/Release
conan build conanfile.py -of build/Release -pr:a conan/profiles/x86_64_rocky8
```

#### Windows

```bash
conan export openrvcore-conanfile.py
conan install conanfile.py --build=missing -pr:a conan/profiles/x86_64_windows -of build/Release
conan build conanfile.py -of build/Release -pr:a conan/profiles/x86_64_windows
```

### Traditional Build (Without Conan)

The traditional build-from-source mode remains the default and is unaffected:

```bash
cmake -B _build_debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DRV_DEPS_QT_LOCATION=$QT_HOME -DRV_VFX_PLATFORM=CY2024
cmake --build _build_debug --target main_executable
```

## Conan-Managed Dependencies

| Package | Version | Shared | Notes |
| ------- | ------- | ------ | ----- |
| zlib | 1.3.1 | Yes | |
| boost | 1.82.0 | Yes | Pinned revision for reproducibility |
| imath | 3.2.1 | Yes | |
| openexr | 3.2.5 | Yes | CY2024 aligned |
| openssl | 3.5.0 | Yes | Overridden via `replace_requires` in profiles |
| dav1d | 1.5.3 | Yes | AV1 codec |
| libjpeg-turbo | 2.1.4 | Yes | Replaces libjpeg via `replace_requires` |
| libpng | 1.6.55 | Yes | |
| openjpeg | 2.5.4 | Yes | |
| openjph | 0.26.3 | Yes | `with_tiff=False` to avoid libjpeg conflict |
| libwebp | 1.2.1 | No | Static |
| libraw | 0.21.1 | Yes | `with_jpeg=libjpeg-turbo`, `with_jasper=False` |
| libatomic_ops | 7.10.0 | No | Static, GC dependency |
| pcre2 | 10.44 | Yes | Windows only |

## Profiles

The `common` profile is included by all platform profiles and contains:

- `openssl/*: openssl/3.5.0` — override transitive OpenSSL versions
- `libjpeg/*: libjpeg-turbo/2.1.4` — replace libjpeg with libjpeg-turbo globally

All profiles set `compiler.cppstd=17` and `compiler.libcxx=libc++` (macOS) or equivalent.

## Key Files

| File | Description |
| ---- | ----------- |
| `conanfile.py` | Consuming recipe — inherits from openrvcore, sets VFX platform and package options |
| `openrvcore-conanfile.py` | Python-requires base class — dependency declarations, CMake toolchain generation |
| `conan/profiles/common` | Shared profile settings (`replace_requires`) |
| `cmake/macros/rv_find_dependency.cmake` | Find-first dispatcher — handles Conan INTERFACE targets generically |
| `cmake/macros/rv_create_std_deps_vars.cmake` | Target resolution helpers (`RV_EXTRACT_LINK_TARGETS`, `RV_RESOLVE_IMPORTED_LOCATION`) |

## Troubleshooting

### Package not found

Ensure you've exported the python-requires and run `conan install` before building:

```bash
conan export openrvcore-conanfile.py
conan install conanfile.py --build=missing -pr:a conan/profiles/<profile> -of build/Release
```

### Stale CMake cache

If switching between Conan and traditional builds, delete the build directory to avoid cached `*_DIR` variables pointing to the wrong location:

```bash
rm -rf build/Release
```

### OIIO/OCIO sub-build finds wrong Boost

When Conan provides Boost, the OIIO ExternalProject sub-build may pick up Homebrew's Boost headers via transitive includes. This is handled automatically via `CMAKE_IGNORE_PREFIX_PATH` when `RV_CONAN_CMAKE_PREFIX_PATH` is set (macOS only).

### libjpeg vs libjpeg-turbo conflict

The `common` profile uses `replace_requires` to substitute `libjpeg` with `libjpeg-turbo` globally. If you see a "Provide Conflict" error, ensure you're using a profile that includes `common`.
