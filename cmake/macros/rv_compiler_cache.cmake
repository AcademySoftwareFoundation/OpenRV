#
# Copyright (C) 2025  Academy Software Foundation. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# rv_compiler_cache.cmake
#
# Automatically enables compiler caching with ccache or sccache when available. This significantly speeds up rebuilds by caching compiled object files.
#
# The module will automatically: - Search for ccache or sccache in your PATH - Configure CMake to use the found compiler cache i - Set appropriate cache
# directories and options
#
# Environment Variables: - CCACHE_DIR: Override ccache cache directory (default: ~/.ccache) - SCCACHE_DIR: Override sccache cache directory (default: varies by
# OS) - RV_DISABLE_COMPILER_CACHE - Set to any value to disable compiler caching - RV_CCACHE_IGNORE_SIZE_CHECK - Set to any value to bypass cache size check -
# RV_CCACHE_MAX_SIZE - Override maximum cache size (e.g., "10G", "20G")
#

IF(DEFINED ENV{RV_DISABLE_COMPILER_CACHE})
  MESSAGE(STATUS "Compiler caching disabled via RV_DISABLE_COMPILER_CACHE environment variable")
  RETURN()
ENDIF()

# Check if user manually specified compiler launchers
IF(CMAKE_C_COMPILER_LAUNCHER
   OR CMAKE_CXX_COMPILER_LAUNCHER
)
  MESSAGE(STATUS "Compiler launcher already set, skipping automatic compiler cache detection")
  RETURN()
ENDIF()

FIND_PROGRAM(CCACHE_PROGRAM ccache)
FIND_PROGRAM(SCCACHE_PROGRAM sccache)

# Prefer sccache on Windows, ccache elsewhere
IF(RV_TARGET_WINDOWS
   AND SCCACHE_PROGRAM
)
  SET(COMPILER_CACHE
      ${SCCACHE_PROGRAM}
  )
  SET(COMPILER_CACHE_NAME
      "sccache"
  )
ELSEIF(CCACHE_PROGRAM)
  SET(COMPILER_CACHE
      ${CCACHE_PROGRAM}
  )
  SET(COMPILER_CACHE_NAME
      "ccache"
  )
ELSEIF(SCCACHE_PROGRAM)
  SET(COMPILER_CACHE
      ${SCCACHE_PROGRAM}
  )
  SET(COMPILER_CACHE_NAME
      "sccache"
  )
ENDIF()

IF(COMPILER_CACHE)
  MESSAGE(STATUS "Compiler cache enabled: ${COMPILER_CACHE_NAME} (${COMPILER_CACHE})")

  # Set compiler launchers for all languages
  SET(CMAKE_C_COMPILER_LAUNCHER
      ${COMPILER_CACHE}
  )
  SET(CMAKE_CXX_COMPILER_LAUNCHER
      ${COMPILER_CACHE}
  )

  # Configure ccache-specific settings
  IF(COMPILER_CACHE_NAME STREQUAL "ccache")
    # Set ccache options for better performance These are set as environment variables that ccache reads

    # Enable compiler check to detect compiler upgrades
    SET(ENV{CCACHE_COMPILERCHECK}
        "content"
    )

    # Store statistics
    SET(ENV{CCACHE_STATS}
        "1"
    )

    # Configure cache directory
    IF(DEFINED ENV{CCACHE_DIR})
      MESSAGE(STATUS "  Using ccache directory: $ENV{CCACHE_DIR}")
    ELSE()
      MESSAGE(STATUS "  Using default ccache directory (~/.ccache)")
    ENDIF()

    # Check cache size and usage Get current cache size and max size
    EXECUTE_PROCESS(
      COMMAND ${COMPILER_CACHE} --show-stats
      OUTPUT_VARIABLE _ccache_stats
      ERROR_QUIET
    )

    EXECUTE_PROCESS(
      COMMAND ${COMPILER_CACHE} --show-config
      OUTPUT_VARIABLE _ccache_config
      ERROR_QUIET
    )

    # Parse cache size from stats (format: "cache size           X.X GB" or similar)
    STRING(REGEX MATCH "cache size[ \t]+([0-9.]+) ([kMG]?B)" _cache_size_match "${_ccache_stats}")
    IF(_cache_size_match)
      SET(_cache_size_value
          ${CMAKE_MATCH_1}
      )
      SET(_cache_size_unit
          ${CMAKE_MATCH_2}
      )

      # Convert to GB for easier comparison
      IF(_cache_size_unit STREQUAL "kB")
        MATH(EXPR _cache_size_gb "${_cache_size_value} / 1024 / 1024")
      ELSEIF(_cache_size_unit STREQUAL "MB")
        MATH(EXPR _cache_size_gb "${_cache_size_value} / 1024")
      ELSEIF(_cache_size_unit STREQUAL "GB")
        SET(_cache_size_gb
            ${_cache_size_value}
        )
      ELSE()
        SET(_cache_size_gb
            0
        )
      ENDIF()
    ELSE()
      SET(_cache_size_gb
          0
      )
    ENDIF()

    # Parse max size from config (format: "max_size = X.X G" or "max_size = X G")
    STRING(REGEX MATCH "max_size = ([0-9.]+) ?([kMG]?)" _max_size_match "${_ccache_config}")
    IF(_max_size_match)
      SET(_max_size_value
          ${CMAKE_MATCH_1}
      )
      SET(_max_size_unit
          ${CMAKE_MATCH_2}
      )

      # Convert to GB
      IF(_max_size_unit STREQUAL "k")
        MATH(EXPR _max_size_gb "${_max_size_value} / 1024 / 1024")
      ELSEIF(_max_size_unit STREQUAL "M")
        MATH(EXPR _max_size_gb "${_max_size_value} / 1024")
      ELSEIF(
        _max_size_unit STREQUAL "G"
        OR _max_size_unit STREQUAL ""
      )
        SET(_max_size_gb
            ${_max_size_value}
        )
      ELSE()
        SET(_max_size_gb
            5
        ) # Default fallback
      ENDIF()
    ELSE()
      SET(_max_size_gb
          5
      ) # Default fallback
    ENDIF()

    # Calculate percentage if we have valid values
    IF(_max_size_gb GREATER 0)
      MATH(EXPR _cache_percent "(${_cache_size_gb} * 100) / ${_max_size_gb}")
      MESSAGE(STATUS "  Cache usage: ${_cache_size_gb}GB / ${_max_size_gb}GB (${_cache_percent}%)")

      # Check if cache is >90% full
      IF(_cache_percent GREATER 90
         AND NOT DEFINED ENV{RV_CCACHE_IGNORE_SIZE_CHECK}
      )
        MESSAGE(
          FATAL_ERROR
            "\nccache is ${_cache_percent}% full (${_cache_size_gb}GB / ${_max_size_gb}GB)!\n"
            "\n"
            "Your ccache is running out of space. Please take one of these actions:\n"
            "  1. Clear the cache:         ccache --clear\n"
            "  2. Increase max size:       ccache --max-size=20G  (or set RV_CCACHE_MAX_SIZE=20G)\n"
            "  3. Clean old entries:       ccache --cleanup\n"
            "  4. Override this check:     export RV_CCACHE_IGNORE_SIZE_CHECK=1\n"
            "\n"
            "A full cache can cause build failures or reduced performance.\n"
        )
      ENDIF()
    ENDIF()

    # Allow user to override max size via environment variable
    IF(DEFINED ENV{RV_CCACHE_MAX_SIZE})
      MESSAGE(STATUS "  Setting cache max size to $ENV{RV_CCACHE_MAX_SIZE}")
      EXECUTE_PROCESS(
        COMMAND ${COMPILER_CACHE} --max-size=$ENV{RV_CCACHE_MAX_SIZE}
        OUTPUT_QUIET ERROR_QUIET
      )
    ENDIF()
  ENDIF()

  # Configure sccache-specific settings
  IF(COMPILER_CACHE_NAME STREQUAL "sccache")
    # sccache uses different environment variables
    IF(DEFINED ENV{SCCACHE_DIR})
      MESSAGE(STATUS "  Using sccache directory: $ENV{SCCACHE_DIR}")
    ELSE()
      MESSAGE(STATUS "  Using default sccache directory")
    ENDIF()

    # Check cache size if possible (sccache --show-stats provides limited info)
    EXECUTE_PROCESS(
      COMMAND ${COMPILER_CACHE} --show-stats
      OUTPUT_VARIABLE _sccache_stats
      ERROR_QUIET
    )

    # Parse cache size from stats (format varies by version)
    STRING(REGEX MATCH "Cache size[ \t]+([0-9.]+) ([kMG]?B)" _cache_size_match "${_sccache_stats}")
    IF(_cache_size_match)
      SET(_cache_size_value
          ${CMAKE_MATCH_1}
      )
      SET(_cache_size_unit
          ${CMAKE_MATCH_2}
      )
      MESSAGE(STATUS "  Cache size: ${_cache_size_value} ${_cache_size_unit}")

      # Convert to GB for warning
      IF(_cache_size_unit STREQUAL "GB"
         AND _cache_size_value GREATER 9
      )
        MESSAGE(WARNING "  sccache is using ${_cache_size_value}GB. Consider cleaning: sccache --stop-server && rm -rf ~/.cache/sccache")
      ENDIF()
    ENDIF()

    # Allow user to override cache size via environment variable
    IF(DEFINED ENV{RV_CCACHE_MAX_SIZE})
      MESSAGE(STATUS "  Setting cache max size to $ENV{RV_CCACHE_MAX_SIZE}")
      SET(ENV{SCCACHE_CACHE_SIZE}
          "$ENV{RV_CCACHE_MAX_SIZE}"
      )
    ELSEIF(NOT DEFINED ENV{SCCACHE_CACHE_SIZE})
      # Set default to 10GB if not already set
      SET(ENV{SCCACHE_CACHE_SIZE}
          "10G"
      )
      MESSAGE(STATUS "  Setting default cache size to 10G (override with RV_CCACHE_MAX_SIZE)")
    ENDIF()
  ENDIF()

  # Print statistics on reconfigure (useful for CI)
  IF(COMPILER_CACHE_NAME STREQUAL "ccache")
    EXECUTE_PROCESS(
      COMMAND ${COMPILER_CACHE} --show-stats
      OUTPUT_VARIABLE CCACHE_STATS
      ERROR_QUIET
    )
    IF(CCACHE_STATS)
      MESSAGE(STATUS "  Current cache statistics:")
      MESSAGE(STATUS "${CCACHE_STATS}")
    ENDIF()
  ELSEIF(COMPILER_CACHE_NAME STREQUAL "sccache")
    EXECUTE_PROCESS(
      COMMAND ${COMPILER_CACHE} --show-stats
      OUTPUT_VARIABLE SCCACHE_STATS
      ERROR_QUIET
    )
    IF(SCCACHE_STATS)
      MESSAGE(STATUS "  Current cache statistics:")
      MESSAGE(STATUS "${SCCACHE_STATS}")
    ENDIF()
  ENDIF()
ELSE()
  MESSAGE(STATUS "No compiler cache found (ccache/sccache). Install one for faster rebuilds:")
  MESSAGE(STATUS "  macOS:  brew install ccache")
  MESSAGE(STATUS "  Linux:  sudo apt install ccache  OR  sudo dnf install ccache")
  MESSAGE(STATUS "  Windows: choco install sccache  OR download from https://github.com/mozilla/sccache")
ENDIF()
