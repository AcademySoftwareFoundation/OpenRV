#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# *****************************************************************************
# Copyright 2025 Autodesk, Inc. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# *****************************************************************************

"""
Check dependencies of Python extension (.pyd) files on Windows.
Uses PowerShell to call dumpbin if available, otherwise provides manual instructions.
"""

import glob
import os
import subprocess
import sys


def find_dumpbin():
    """Find dumpbin.exe in Visual Studio installation."""
    vs_paths = [
        r"C:\Program Files\Microsoft Visual Studio",
        r"C:\Program Files (x86)\Microsoft Visual Studio",
    ]

    for vs_base in vs_paths:
        if not os.path.exists(vs_base):
            continue

        try:
            for vs_year in os.listdir(vs_base):
                vs_year_path = os.path.join(vs_base, vs_year)
                if not os.path.isdir(vs_year_path):
                    continue

                for edition in ["Enterprise", "Professional", "Community", "BuildTools"]:
                    edition_path = os.path.join(vs_year_path, edition)
                    if not os.path.exists(edition_path):
                        continue

                    # Look for dumpbin in VC\Tools
                    vc_tools = os.path.join(edition_path, "VC", "Tools", "MSVC")
                    if os.path.exists(vc_tools):
                        try:
                            msvc_versions = sorted(os.listdir(vc_tools), reverse=True)
                            for msvc_ver in msvc_versions[:3]:
                                dumpbin_path = os.path.join(vc_tools, msvc_ver, "bin", "Hostx64", "x64", "dumpbin.exe")
                                if os.path.exists(dumpbin_path):
                                    return dumpbin_path
                        except (OSError, PermissionError):
                            pass
        except (OSError, PermissionError):
            pass

    return None


def check_pyd_dependencies(pyd_path, dumpbin_path=None):
    """Check dependencies of a .pyd file.

    Args:
        pyd_path: Path to the .pyd file
        dumpbin_path: Optional path to dumpbin.exe

    Returns:
        List of dependency DLL names, or None if check failed
    """
    if not os.path.exists(pyd_path):
        print(f"ERROR: File not found: {pyd_path}")
        return None

    if dumpbin_path is None:
        dumpbin_path = find_dumpbin()

    if not dumpbin_path or not os.path.exists(dumpbin_path):
        print("dumpbin.exe not found. Manual check required:")
        print("  1. Open Developer Command Prompt for VS")
        print(f'  2. Run: dumpbin /dependents "{pyd_path}"')
        return None

    try:
        result = subprocess.run([dumpbin_path, "/dependents", pyd_path], capture_output=True, text=True, timeout=30)

        if result.returncode != 0:
            print(f"dumpbin failed with return code {result.returncode}")
            print(result.stderr)
            return None

        # Parse output to extract dependencies
        dependencies = []
        in_dependencies_section = False

        for line in result.stdout.split("\n"):
            line = line.strip()

            if "Image has the following dependencies:" in line:
                in_dependencies_section = True
                continue

            if in_dependencies_section:
                if line == "Summary" or line == "":
                    break

                if line.endswith(".dll"):
                    dependencies.append(line)

        return dependencies

    except subprocess.TimeoutExpired:
        print("dumpbin timed out")
        return None
    except Exception as e:
        print(f"Error running dumpbin: {e}")
        return None


def analyze_otio_dependencies(site_packages_dir):
    """Analyze OpenTimelineIO extension dependencies.

    Args:
        site_packages_dir: Path to site-packages directory
    """
    otio_path = os.path.join(site_packages_dir, "opentimelineio")

    if not os.path.exists(otio_path):
        print(f"OpenTimelineIO not found in: {site_packages_dir}")
        return

    print("=" * 80)
    print("Analyzing OpenTimelineIO Extension Dependencies")
    print("=" * 80)
    print(f"Location: {otio_path}")
    print()

    # Find all .pyd files
    pyd_files = glob.glob(os.path.join(otio_path, "*.pyd"))

    if not pyd_files:
        print("No .pyd files found!")
        return

    print(f"Found {len(pyd_files)} extension(s):")
    for pyd in pyd_files:
        print(f"  - {os.path.basename(pyd)}")
    print()

    # Find dumpbin
    dumpbin_path = find_dumpbin()
    if dumpbin_path:
        print(f"Using dumpbin: {dumpbin_path}")
    else:
        print("WARNING: dumpbin.exe not found - cannot check dependencies automatically")
    print()

    # Check each .pyd file
    for pyd_file in pyd_files:
        basename = os.path.basename(pyd_file)
        print("-" * 80)
        print(f"Extension: {basename}")
        print("-" * 80)

        deps = check_pyd_dependencies(pyd_file, dumpbin_path)

        if deps:
            print(f"Dependencies ({len(deps)}):")

            # Categorize dependencies
            debug_runtime = []
            release_runtime = []
            python_dlls = []
            system_dlls = []
            other_dlls = []

            for dep in deps:
                dep_lower = dep.lower()
                if "d.dll" in dep_lower and ("msvcp" in dep_lower or "vcruntime" in dep_lower):
                    debug_runtime.append(dep)
                elif "msvcp" in dep_lower or "vcruntime" in dep_lower:
                    release_runtime.append(dep)
                elif "python" in dep_lower:
                    python_dlls.append(dep)
                elif "api-ms-win-crt" in dep_lower or "kernel32" in dep_lower:
                    system_dlls.append(dep)
                else:
                    other_dlls.append(dep)

            if python_dlls:
                print("  Python Runtime:")
                for dll in python_dlls:
                    print(f"    - {dll}")

            if debug_runtime:
                print("  MSVC Debug Runtime:")
                for dll in debug_runtime:
                    print(f"    - {dll}")

            if release_runtime:
                print("  MSVC Release Runtime:")
                for dll in release_runtime:
                    print(f"    - {dll}")

            if other_dlls:
                print("  Other:")
                for dll in other_dlls:
                    print(f"    - {dll}")

            if system_dlls:
                print("  System (Universal CRT + Windows):")
                for dll in system_dlls[:5]:
                    print(f"    - {dll}")
                if len(system_dlls) > 5:
                    print(f"    ... and {len(system_dlls) - 5} more")

            # Detect Debug/Release mismatch
            print()
            if "_d.cp" in basename or basename.endswith("_d.pyd"):
                print("  This is a Debug extension (name ends with _d)")
                if release_runtime and not debug_runtime:
                    print("  ⚠️  WARNING: Debug extension linking against RELEASE runtime!")
                    print("      This will cause 'DLL initialization routine failed' errors")
                    print("      OTIO should link against Debug runtime (msvcp140d.dll, vcruntime140d.dll)")
                elif debug_runtime:
                    print("  ✓ Correctly links against Debug runtime")
            else:
                print("  This is a Release extension")
                if debug_runtime and not release_runtime:
                    print("  ⚠️  WARNING: Release extension linking against DEBUG runtime!")
                elif release_runtime:
                    print("  ✓ Correctly links against Release runtime")
        else:
            print("  Could not determine dependencies")

        print()


def main():
    """Main function."""
    if len(sys.argv) > 1:
        # Check specific directory
        site_packages = sys.argv[1]
    else:
        # Use current Python's site-packages
        import site

        site_packages_list = site.getsitepackages()
        if site_packages_list:
            site_packages = site_packages_list[0]
        else:
            print("ERROR: Could not find site-packages directory")
            return 1

    analyze_otio_dependencies(site_packages)
    return 0


if __name__ == "__main__":
    sys.exit(main())
