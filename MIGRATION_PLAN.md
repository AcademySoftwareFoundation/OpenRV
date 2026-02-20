# Gradual Conan Integration Plan

## Context

The `conan-poc` branch at `/Users/fuococ/rv/conan-poc/OpenRV_sync` adds Conan 2.x package manager support as an **alternative** to the traditional ExternalProject_Add build. Controlled by `RV_USE_PACKAGE_MANAGER` (default OFF), both modes coexist. The goal is to incorporate these changes into the main branch gradually so that:

1. Each PR is independently reviewable (~200-500 lines)
2. The traditional build path **never breaks**
3. Each PR compiles and works on its own

The `staging-refactor` worktree already unified dependency staging with `RV_STAGE_DEPENDENCY_LIBS()`. This plan builds on that foundation — the package manager path will **also** use `RV_STAGE_DEPENDENCY_LIBS()`, eliminating the ~200 lines of duplicated staging code present in the original conan-poc.

---

## Key Design Improvements Over conan-poc

### Improvement 1: `RV_FIND_DEPENDENCY()` Macro

Every package manager block in conan-poc repeats the same 8-line boilerplate. We collapse this into one macro that works with **any** package manager (Conan, vcpkg, Homebrew, system packages) — anything that provides `find_package()`-compatible CMake configs.

**Why MACRO (not FUNCTION):** `RV_SETUP_PACKAGE_STAGING` is a MACRO that sets `_lib_dir`, `_bin_dir`, `_include_dir`, `_install_dir` in the caller's scope. If `RV_FIND_DEPENDENCY` were a FUNCTION, those variables would be trapped in the FUNCTION scope and never reach the dispatcher file. Using MACRO ensures all variables propagate correctly.

**Full implementation** (added to `cmake/macros/rv_create_std_deps_vars.cmake`):

```cmake
#
# RV_FIND_DEPENDENCY - Find a pre-built package and set up staging variables
#
# Works with any package manager that provides find_package()-compatible
# CMake configs (Conan, vcpkg, Homebrew, system packages, etc.).
#
# Collapses the repetitive package-manager boilerplate into a single call:
#   FIND_PACKAGE + RV_MAKE_TARGETS_GLOBAL + RV_PRINT_PACKAGE_INFO
#   + RV_SETUP_PACKAGE_STAGING + RV_DEPS_LIST append + version caching
#
# After calling, the following variables are set in the caller's scope:
#   _lib_dir      - Package library directory
#   _bin_dir      - Package binary directory
#   _include_dir  - Package include directory
#   _install_dir  - Package root directory
#   ${TARGET}_ROOT_DIR - Same as _install_dir (e.g., RV_DEPS_IMATH_ROOT_DIR)
#
# Usage:
#   RV_FIND_DEPENDENCY(
#     TARGET RV_DEPS_IMATH                # REQUIRED: Internal dep target name
#     PACKAGE Imath                        # REQUIRED: find_package() package name
#     [VERSION ${RV_DEPS_IMATH_VERSION}]   # Optional: version constraint
#     GLOBAL_TARGETS Imath::Imath          # Targets to make GLOBAL (for subdirectories)
#     DEPS_LIST_TARGETS Imath::Imath       # Targets to add to RV_DEPS_LIST
#   )
#
# IMPORTANT: Must be a MACRO (not FUNCTION) because RV_SETUP_PACKAGE_STAGING
# sets variables (_lib_dir, _bin_dir, etc.) in the caller's scope via macro
# text substitution. A FUNCTION would trap those variables.
#
MACRO(RV_FIND_DEPENDENCY)
  CMAKE_PARSE_ARGUMENTS(
    _RFD                                           # prefix (RV_Find_Dependency)
    ""                                             # boolean options
    "TARGET;PACKAGE;VERSION"                       # single-value args
    "GLOBAL_TARGETS;DEPS_LIST_TARGETS"             # multi-value args
    ${ARGN}
  )

  IF(NOT _RFD_TARGET)
    MESSAGE(FATAL_ERROR "RV_FIND_DEPENDENCY: TARGET is required")
  ENDIF()
  IF(NOT _RFD_PACKAGE)
    MESSAGE(FATAL_ERROR "RV_FIND_DEPENDENCY: PACKAGE is required")
  ENDIF()

  MESSAGE(STATUS "Finding ${_RFD_TARGET} via find_package(${_RFD_PACKAGE})")

  # Find the package (with optional version constraint)
  IF(_RFD_VERSION)
    FIND_PACKAGE(${_RFD_PACKAGE} ${_RFD_VERSION} CONFIG REQUIRED)
  ELSE()
    FIND_PACKAGE(${_RFD_PACKAGE} CONFIG REQUIRED)
  ENDIF()

  # Make imported targets GLOBAL for use in subdirectories
  IF(_RFD_GLOBAL_TARGETS)
    RV_MAKE_TARGETS_GLOBAL(${_RFD_GLOBAL_TARGETS})
  ENDIF()

  # Print diagnostic info
  RV_PRINT_PACKAGE_INFO("${_RFD_PACKAGE}")

  # Set up staging variables: _lib_dir, _bin_dir, _include_dir, _install_dir
  # Also creates the ${TARGET} custom target for dependency tracking
  RV_SETUP_PACKAGE_STAGING(${_RFD_TARGET} ${_RFD_PACKAGE})

  # Append to global deps list
  FOREACH(_rfd_dep_target ${_RFD_DEPS_LIST_TARGETS})
    LIST(APPEND RV_DEPS_LIST ${_rfd_dep_target})
  ENDFOREACH()

  # Cache the found version (e.g., RV_DEPS_IMATH_VERSION)
  STRING(TOUPPER ${_RFD_PACKAGE} _RFD_PACKAGE_UPPER)
  SET(RV_DEPS_${_RFD_PACKAGE_UPPER}_VERSION
      ${${_RFD_PACKAGE}_VERSION}
      CACHE INTERNAL "" FORCE
  )
ENDMACRO()
```

**Concrete examples of before/after:**

```cmake
# === BEFORE (conan-poc imath.cmake, package manager block: ~30 lines) ===
MESSAGE(STATUS "Finding ${_target} from Conan via find_package()")
SET(_find_target Imath)
FIND_PACKAGE(${_find_target} ${RV_DEPS_IMATH_VERSION} CONFIG REQUIRED)
RV_MAKE_TARGETS_GLOBAL(Imath::Imath)
RV_PRINT_PACKAGE_INFO("${_find_target}")
LIST(APPEND RV_DEPS_LIST Imath::Imath)
# ... 15 lines of library naming ...
RV_SETUP_PACKAGE_STAGING(${_target} ${_find_target})
# ... 25 lines of inline staging ...
STRING(TOUPPER ${_find_target} _find_target_uppercase)
SET(RV_DEPS_${_find_target_uppercase}_VERSION ${${_find_target}_VERSION} CACHE INTERNAL "" FORCE)

# === AFTER (our version: ~7 lines in package manager block) ===
RV_FIND_DEPENDENCY(
  TARGET ${_target}
  PACKAGE Imath
  VERSION ${RV_DEPS_IMATH_VERSION}
  GLOBAL_TARGETS Imath::Imath
  DEPS_LIST_TARGETS Imath::Imath
)
# Library naming is SHARED (before IF/ELSE), staging is SHARED (after ENDIF)
```

### Improvement 2: Shared Staging After IF/ELSE

In conan-poc, staging is duplicated — the package manager path has inline `ADD_CUSTOM_COMMAND` and the traditional path has its own. Since our staging refactor created `RV_STAGE_DEPENDENCY_LIBS()`, we share a single staging call after `ENDIF()`.

### Improvement 3: Shared Library Naming (Where Possible)

For deps where Conan and traditional produce the **same** library filenames (imath, openexr, openssl, dav1d, pcre2, ffmpeg), the `_libname` computation can go BEFORE the `IF/ELSE`. For deps where names differ (boost, zlib), naming stays per-path.

### Combined Pattern

```cmake
SET(_target "RV_DEPS_IMATH")

# SHARED library naming — same in both paths (uses CYCOMMON variables)
IF(RV_TARGET_DARWIN)
  SET(_libname ${CMAKE_SHARED_LIBRARY_PREFIX}Imath-${RV_DEPS_IMATH_LIB_MAJOR}...)
ELSEIF(...)
  ...
ENDIF()

IF(RV_USE_PACKAGE_MANAGER)
  # Works with Conan, vcpkg, Homebrew, or any find_package()-compatible source
  RV_FIND_DEPENDENCY(
    TARGET ${_target}
    PACKAGE Imath
    VERSION ${RV_DEPS_IMATH_VERSION}
    GLOBAL_TARGETS Imath::Imath
    DEPS_LIST_TARGETS Imath::Imath
  )
  # _lib_dir, _bin_dir set by RV_SETUP_PACKAGE_STAGING (called internally)
ELSE()
  INCLUDE(${CMAKE_CURRENT_LIST_DIR}/build/imath.cmake)
  # _lib_dir, _bin_dir set by RV_CREATE_STANDARD_DEPS_VARIABLES
ENDIF()

# SHARED staging — works for both paths since _lib_dir and _libname are set
IF(RV_TARGET_WINDOWS)
  RV_STAGE_DEPENDENCY_LIBS(
    TARGET ${_target} LIB_DIR ${_lib_dir} BIN_DIR ${_bin_dir}
    OUTPUTS ${RV_STAGE_BIN_DIR}/${_libname}
  )
ELSE()
  RV_STAGE_DEPENDENCY_LIBS(
    TARGET ${_target} LIB_DIR ${_lib_dir}
    OUTPUTS ${RV_STAGE_LIB_DIR}/${_libname}
  )
ENDIF()
```

**Result**: For a simple dep like imath, the entire package manager block is ~7 lines (vs ~40 in conan-poc). Library naming and staging are each written once.

---

## Scope: What Changes Come From conan-poc

### Deps with dual-mode (10 deps — have `IF(RV_USE_PACKAGE_MANAGER)` + `build/*.cmake`)

boost, dav1d, ffmpeg, imath, jpegturbo, oiio, openssl, pcre2, python (renamed from python3), zlib

### New CMake infrastructure

- `cmake/defaults/rv_options.cmake` — `RV_USE_PACKAGE_MANAGER` option
- `cmake/macros/rv_create_std_deps_vars.cmake` — `RV_MAKE_TARGETS_GLOBAL`, `RV_SETUP_PACKAGE_STAGING`, `RV_PRINT_PACKAGE_INFO`

### New Conan infrastructure

- `conanfile.py`, `openrvcore-conanfile.py` (root recipes)
- `conan/profiles/` (8 platform profiles)
- `conan/recipes/python/all/` (custom Python recipe, ~1400 lines)
- `conan/recipes/pyside6/all/` (custom PySide6 recipe, ~1200 lines)
- `conan/recipes/ffmpeg/all/` (custom FFmpeg recipe)
- `conan/extensions/hooks/hook_atomic_ops_source.py`
- `.github/workflows/conan.yml` (CI)
- `CMakeUserPresets.json`

### Other changes

- `jpeg-turbo::jpeg` renamed to `libjpeg-turbo::jpeg` (both paths + IOjpeg consumer)
- `python3.cmake` renamed to `python.cmake` (+ CMakeLists.txt include change)

### NOT changed in conan-poc (stay traditional-only)

expat, qt5, qt6, openexr, openjpeg, openjph, png, raw, imgui, atomic_ops, gc, glew, spdlog, aja, bmd, doctest, yaml-cpp, nanobind, ocio, pyimgui, pyimplot, tiff, webp

---

## PR Sequence

### PR 1: Staging Refactor *(already done, needs commit)*

**Status**: All code changes complete in `worktree-staging-refactor`, uncommitted.

- New `cmake/macros/rv_stage_dependency_libs.cmake`
- 22+ dependency files migrated to `RV_STAGE_DEPENDENCY_LIBS()`
- `rv_copy_lib_bin_folders.cmake` deprecated (kept for ocio.cmake)
- ~26 files, net -23 lines

**Verification**: `cmake-format --check` passes on all modified files.

---

### PR 2: CMake Infrastructure for Package Manager

**Files to create/modify:**

- `cmake/defaults/rv_options.cmake` — add `RV_USE_PACKAGE_MANAGER` option (default OFF)
- `cmake/macros/rv_create_std_deps_vars.cmake` — add 4 macros:
  - `RV_PRINT_PACKAGE_INFO()` (~35 lines) — diagnostic/debug printing of found package info
  - `RV_MAKE_TARGETS_GLOBAL()` (~12 lines) — makes imported targets GLOBAL for subdirectory use
  - `RV_SETUP_PACKAGE_STAGING()` (~65 lines) — derives `_lib_dir`/`_bin_dir`/`_include_dir` from found package; creates dummy `${RV_DEPS_TARGET}` target
  - **`RV_FIND_DEPENDENCY()`** (~50 lines, NEW) — collapses the repetitive package-manager boilerplate into one call: `FIND_PACKAGE` + `RV_MAKE_TARGETS_GLOBAL` + `RV_PRINT_PACKAGE_INFO` + `RV_SETUP_PACKAGE_STAGING` + `RV_DEPS_LIST` append + version caching. Works with any `find_package()`-compatible source (Conan, vcpkg, Homebrew, etc.)

**Source**: `RV_PRINT_PACKAGE_INFO`, `RV_MAKE_TARGETS_GLOBAL`, `RV_SETUP_PACKAGE_STAGING` from conan-poc `cmake/macros/rv_create_std_deps_vars.cmake` lines 230-361. `RV_USE_PACKAGE_MANAGER` from conan-poc `cmake/defaults/rv_options.cmake` lines 120-143. `RV_FIND_DEPENDENCY` is new (not in conan-poc).

**Risk**: Zero — option defaults to OFF, macros unused until PR 4+.

**Verification**: Traditional build unaffected. `cmake-format --check` passes.

---

### PR 3: Target Rename `jpeg-turbo` → `libjpeg-turbo`

Rename the imported target to match Conan's naming convention. This is a pure rename that doesn't depend on PR 2.

**Files to modify (2 files):**

- `cmake/dependencies/jpegturbo.cmake` — rename all `jpeg-turbo::jpeg` → `libjpeg-turbo::jpeg` and `jpeg-turbo::turbojpeg` → `libjpeg-turbo::turbojpeg`
- `src/lib/image/IOjpeg/CMakeLists.txt` — update link target name

**Verification**: Traditional build succeeds. `grep -r "jpeg-turbo::" cmake/ src/` shows no remaining old names.

---

### PR 4: Dual-Mode — Simple Deps (dav1d, imath, zlib, pcre2)

For each dep, the pattern is:

1. **Extract**: Move ExternalProject logic to `cmake/dependencies/build/<dep>.cmake`
2. **Shared naming**: Move `_libname` computation before `IF/ELSE` (where names match between paths)
3. **Dispatch**: `IF(RV_USE_PACKAGE_MANAGER)` calls `RV_FIND_DEPENDENCY()`, `ELSE()` includes build file
4. **Shared staging**: `RV_STAGE_DEPENDENCY_LIBS()` after `ENDIF()`

**New files (4):**

- `cmake/dependencies/build/dav1d.cmake`
- `cmake/dependencies/build/imath.cmake`
- `cmake/dependencies/build/zlib.cmake`
- `cmake/dependencies/build/pcre2.cmake`

**Modified files (4):**

- `cmake/dependencies/dav1d.cmake`
- `cmake/dependencies/imath.cmake`
- `cmake/dependencies/zlib.cmake`
- `cmake/dependencies/pcre2.cmake`

**Shared naming applies to**: dav1d, imath, pcre2 (same CYCOMMON-based names in both paths).
**Per-path naming**: zlib (Conan: `libz.dylib`, Traditional: `libz.1.3.1.dylib`).

**Example — imath dispatcher after refactor:**

```cmake
SET(_target "RV_DEPS_IMATH")

# Shared library naming (CYCOMMON vars, same in both paths)
IF(RV_TARGET_DARWIN)
  SET(_libname ${CMAKE_SHARED_LIBRARY_PREFIX}Imath-${RV_DEPS_IMATH_LIB_MAJOR}${RV_DEBUG_POSTFIX}.${RV_DEPS_IMATH_LIB_VER}${CMAKE_SHARED_LIBRARY_SUFFIX})
ELSEIF(RV_TARGET_LINUX)
  SET(_libname ...)
ELSEIF(RV_TARGET_WINDOWS)
  SET(_libname ...)
ENDIF()

IF(RV_USE_PACKAGE_MANAGER)
  RV_FIND_DEPENDENCY(
    TARGET ${_target} PACKAGE Imath VERSION ${RV_DEPS_IMATH_VERSION}
    GLOBAL_TARGETS Imath::Imath DEPS_LIST_TARGETS Imath::Imath
  )
ELSE()
  INCLUDE(${CMAKE_CURRENT_LIST_DIR}/build/imath.cmake)
ENDIF()

# Shared staging
IF(RV_TARGET_WINDOWS)
  RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} LIB_DIR ${_lib_dir} BIN_DIR ${_bin_dir} OUTPUTS ${RV_STAGE_BIN_DIR}/${_libname})
ELSE()
  RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} LIB_DIR ${_lib_dir} OUTPUTS ${RV_STAGE_LIB_DIR}/${_libname})
ENDIF()
```

**Special cases:**

- **zlib/Darwin**: `install_name_tool` PRE_COMMANDS — applies in both paths, keep in shared staging
- **zlib/Windows debug**: FFmpeg rename (`zlibd.lib` → `zlib.lib`) stays as POST_BUILD in build/zlib.cmake
- **zlib naming**: Per-path because Conan uses simple names (e.g., `libz.dylib`) vs traditional versioned names
- **pcre2**: Windows-only dep, copies specific files (not directory)

**Verification**: Traditional build succeeds (`RV_USE_PACKAGE_MANAGER=OFF`). `cmake-format --check` passes. No residual inline staging in modified dispatcher files.

---

### PR 5: Dual-Mode — Medium Deps (boost, openssl, jpegturbo)

Same extract+dispatch pattern, but these deps have more complex staging/naming.

**New files (3):**

- `cmake/dependencies/build/boost.cmake`
- `cmake/dependencies/build/openssl.cmake`
- `cmake/dependencies/build/jpegturbo.cmake`

**Modified files (3):**

- `cmake/dependencies/boost.cmake`
- `cmake/dependencies/openssl.cmake`
- `cmake/dependencies/jpegturbo.cmake`

**Shared naming**: openssl (same CYCOMMON naming in both paths). **Per-path naming**: boost (Conan: simple `boost_filesystem.dll`, Traditional: versioned `boost_filesystem-vc143-mt-x64-1_82.dll`), jpegturbo (Conan and traditional may differ).

**Special cases:**

- **boost**: Per-path `_stage_outputs` list. Both paths call `RV_FIND_DEPENDENCY()` (Conan) or `build/boost.cmake` (traditional), then build the output list with per-path naming, then shared `RV_STAGE_DEPENDENCY_LIBS()`.
- **boost/Windows**: Both .lib and .dll in `lib/` dir — use `EXTRA_LIB_DIRS ${RV_STAGE_BIN_DIR}`
- **openssl/Linux**: Stages to `RV_STAGE_LIB_DIR/OpenSSL/` subdir — use `STAGE_LIB_DIR` override
- **openssl/Windows**: Copies specific DLLs from bin/ — kept as platform-specific inline block, non-Windows uses shared macro
- **jpegturbo/Windows**: Copies specific DLLs — same platform-specific approach

**Verification**: Traditional build succeeds. `cmake-format --check` passes.

---

### PR 6: Dual-Mode — Complex Deps (ffmpeg, oiio, python)

These have the most complex build/staging logic. `RV_FIND_DEPENDENCY()` still handles the boilerplate, but each has unique requirements.

**New files (3):**

- `cmake/dependencies/build/ffmpeg.cmake`
- `cmake/dependencies/build/oiio.cmake`
- `cmake/dependencies/build/python.cmake`

**Modified/renamed files (4):**

- `cmake/dependencies/ffmpeg.cmake`
- `cmake/dependencies/oiio.cmake`
- `cmake/dependencies/python3.cmake` → **renamed** to `cmake/dependencies/python.cmake`
- `cmake/dependencies/CMakeLists.txt` — change `INCLUDE(python3)` → `INCLUDE(python)`

**Special cases:**

- **ffmpeg**: Uses `USE_FLAG_FILE` for staging (output filenames unknown). Windows stores libs in `bin/`. Shared naming works (CYCOMMON version vars).
- **oiio**: Depends on many other deps (openexr, ocio, boost, etc.). package manager path still uses ExternalProject_Add for OIIO itself but with `-DCMAKE_PREFIX_PATH` pointing to Conan packages. Python hints needed.
- **python**: Most complex. 3 build phases (pip install, requirements, tests) are path-specific and stay inside IF/ELSE. Only final lib/bin staging is shared. package manager path uses `RV_FIND_DEPENDENCY()` for the find+setup, then has its own pip phase.
- **python rename**: `python3.cmake` → `python.cmake` to align with Conan naming.

**Verification**: Traditional build succeeds. `cmake-format --check` passes. Python pip packages install correctly.

---

### PR 7: Conan Root Files + Profiles

**New files:**

- `conanfile.py` (main OpenRV recipe)
- `openrvcore-conanfile.py` (python_requires base class)
- `conan/profiles/common`
- `conan/profiles/arm64_apple_release`
- `conan/profiles/arm64_apple_debug`
- `conan/profiles/x86_64_apple_release`
- `conan/profiles/x86_64_apple_debug`
- `conan/profiles/x86_64_rocky8`
- `conan/profiles/x86_64_rocky8_debug`
- `conan/profiles/x86_64_windows`
- `conan/profiles/x86_64_windows_debug`
- `conan/extensions/hooks/hook_atomic_ops_source.py`
- `conan/README.md`

**Source**: Copy from conan-poc.

**Verification**: `conan export openrvcore-conanfile.py` succeeds. Traditional build unaffected.

---

### PR 8: Custom Conan Recipes

**New files:**

- `conan/recipes/python/all/` (conanfile.py, conandata.yml, patches/, test_package/, README.md)
- `conan/recipes/pyside6/all/` (conanfile.py, conandata.yml, README.md, test_package/)
- `conan/recipes/ffmpeg/all/` (conanfile.py, conandata.yml)

**Source**: Copy from conan-poc.

**Verification**: Each recipe exports successfully:

```bash
conan export conan/recipes/python/all/conanfile.py --version 3.11.9 --user openrv
conan export conan/recipes/pyside6/all/conanfile.py --version 6.5.3 --user openrv
conan export conan/recipes/ffmpeg/all/conanfile.py --version 6.1.1 --user openrv
```

---

### PR 9: CI + Misc

**New files:**

- `.github/workflows/conan.yml` (CI workflow for Conan builds)
- `CMakeUserPresets.json`

**Modified files:**

- `.gitignore` — add `.worktrees/`

**Verification**: CI workflow triggers on conan-poc branches. Traditional CI unaffected.

---

## Verification Strategy (per PR)

1. **cmake-format**: `cmake-format --check <modified-files>` — must pass
2. **Traditional build**: `cmake -B build -S . -DRV_USE_PACKAGE_MANAGER=OFF` — must configure without errors
3. **No staging duplication**: `grep -n "ADD_CUSTOM_COMMAND\|ADD_CUSTOM_TARGET\|ADD_DEPENDENCIES(dependencies" <modified-dispatchers>` — should only show `RV_STAGE_DEPENDENCY_LIBS` calls (no inline staging in dispatcher files)
4. **Full Conan build** (after all PRs): `conan install ... && cmake -DRV_USE_PACKAGE_MANAGER=ON -DCMAKE_TOOLCHAIN_FILE=...`

---

## File Reference

| File | Source | PR |
|------|--------|----|
| `cmake/macros/rv_stage_dependency_libs.cmake` | Already done (staging-refactor) | 1 |
| `cmake/defaults/rv_options.cmake` | conan-poc lines 120-143 | 2 |
| `cmake/macros/rv_create_std_deps_vars.cmake` | conan-poc lines 230-361 + new `RV_FIND_DEPENDENCY` | 2 |
| `cmake/dependencies/jpegturbo.cmake`, `src/lib/image/IOjpeg/CMakeLists.txt` | Target rename | 3 |
| `cmake/dependencies/build/{dav1d,imath,zlib,pcre2}.cmake` | Extract from staging-refactor dep files | 4 |
| `cmake/dependencies/{dav1d,imath,zlib,pcre2}.cmake` | Dispatch + shared staging | 4 |
| `cmake/dependencies/build/{boost,openssl,jpegturbo}.cmake` | Extract from staging-refactor dep files | 5 |
| `cmake/dependencies/{boost,openssl,jpegturbo}.cmake` | Dispatch + shared staging | 5 |
| `cmake/dependencies/build/{ffmpeg,oiio,python}.cmake` | Extract from staging-refactor dep files | 6 |
| `cmake/dependencies/{ffmpeg,oiio,python}.cmake` | Dispatch + shared staging + python rename | 6 |
| `conanfile.py`, `openrvcore-conanfile.py`, `conan/profiles/*` | Copy from conan-poc | 7 |
| `conan/recipes/*` | Copy from conan-poc | 8 |
| `.github/workflows/conan.yml`, `CMakeUserPresets.json` | Copy from conan-poc | 9 |

## Dep-Level Naming Strategy

| Dep | Shared naming? | Reason |
|-----|---------------|--------|
| dav1d | Yes | Same CYCOMMON vars |
| imath | Yes | Same CYCOMMON vars |
| pcre2 | Yes | Same naming |
| zlib | **No** — per-path | Conan: `libz.dylib`, Traditional: `libz.1.3.1.dylib` |
| boost | **No** — per-path | Conan: simple names, Traditional: versioned+toolset names |
| openssl | Yes | Same CYCOMMON version vars |
| jpegturbo | Verify | May differ between Conan and traditional |
| ffmpeg | Yes | Same version-based naming |
| oiio | Yes | Same naming |
| python | Verify | May differ between paths |
