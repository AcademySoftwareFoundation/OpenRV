#!/usr/bin/env python3
#
# Copyright (C) 2025  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# Generate about_rv.cpp with build and dependency information

import sys
import subprocess
from datetime import datetime
from pathlib import Path


def get_git_info(git_hash_from_cmake=""):
    """Get git commit hash - prefer the one from CMake for consistency"""
    if git_hash_from_cmake:
        return git_hash_from_cmake

    # Fallback: try to get from git if not provided
    try:
        result = subprocess.run(["git", "log", "-1", "--pretty=format:%h"], capture_output=True, text=True, check=True)
        return result.stdout.strip()
    except (subprocess.CalledProcessError, FileNotFoundError, OSError):
        return "unknown"


def parse_versions(versions_str):
    """Parse the versions string into a dictionary"""
    versions = {}
    if versions_str:
        for item in versions_str.split(","):
            if ":" in item:
                key, value = item.split(":", 1)
                versions[key] = value
    return versions


def get_dependencies_info(vfx_platform, versions, app_name, platform=""):
    """Generate dependency information using detected versions"""

    # Helper to get version or default
    def get_ver(key, default="Unknown"):
        version = versions.get(key, default)
        # Strip FFmpeg's 'n' prefix from version tags (e.g., n6.1.2 -> 6.1.2)
        if key == "FFmpeg" and version.startswith("n"):
            version = version[1:]
        return version

    # Determine Qt license based on application type
    # OpenRV uses LGPL, commercial RV uses Qt Commercial
    is_commercial_rv = app_name == "RV"
    qt_license = "Qt Commercial" if is_commercial_rv else "LGPL v3"
    pyside_license = "Qt Commercial" if is_commercial_rv else "LGPL v3"

    # Detect if we're on macOS
    is_macos = "darwin" in platform.lower() or "macos" in platform.lower()

    # VFX Platform components (listed in order)
    # Note: PyQt is NOT used by RV (only PySide), so it's excluded from the list
    vfx_deps = [
        ("Qt Framework", get_ver("Qt"), qt_license),
        ("CMake", get_ver("CMake"), "BSD 3-Clause"),
        ("Python", get_ver("Python"), "PSF License"),
        ("PySide", get_ver("PySide"), pyside_license),
        ("NumPy", get_ver("numpy", "1.24+"), "BSD 3-Clause"),
        ("Imath", get_ver("Imath"), "BSD 3-Clause"),
        ("OpenEXR", get_ver("OpenEXR"), "BSD 3-Clause"),
        ("Boost", get_ver("Boost"), "Boost Software License"),
        ("OpenImageIO", get_ver("OpenImageIO"), "Apache 2.0"),
        ("OpenColorIO", get_ver("OpenColorIO"), "BSD 3-Clause"),
    ]

    # RV-specific proprietary components (only for commercial RV)
    rv_specific_deps = []
    if is_commercial_rv:
        # Apple ProRes is only available on macOS (and not on ARM64 currently)
        if is_macos:
            prores_ver = get_ver("prores", "Not Used")
            rv_specific_deps.append(("Apple ProRes", prores_ver, "Apple Proprietary"))

        # Always add these for commercial RV (version will show "Not Used" if not available)
        rv_specific_deps.extend(
            [
                ("RED R3D SDK", get_ver("r3dsdk", "Not Used"), "RED Proprietary"),
                ("ARRI SDK", get_ver("arriraw", "Not Used"), "ARRI Proprietary"),
                ("x264", get_ver("x264", "Not Used"), "x264 Commercial License"),
            ]
        )

    # Other third-party components
    other_deps = [
        ("FFmpeg", get_ver("FFmpeg"), "LGPL v2.1+"),
        ("libjpeg-turbo", get_ver("jpegturbo"), "BSD-style"),
        ("libpng", get_ver("png"), "libpng License"),
        ("libtiff", get_ver("tiff"), "libtiff License"),
        ("OpenJPEG", get_ver("openjpeg"), "BSD 2-Clause"),
        ("OpenJPH", get_ver("openjph"), "BSD 2-Clause"),
        ("libwebp", get_ver("webp"), "BSD 3-Clause"),
        ("LibRaw", get_ver("raw"), "LGPL v2.1 / CDDL"),
        ("dav1d", get_ver("dav1d"), "BSD 2-Clause"),
        ("zlib", get_ver("zlib"), "zlib License"),
        ("OpenSSL", get_ver("OpenSSL"), "Apache License 2.0"),
        ("GLEW", get_ver("GLEW"), "Modified BSD / MIT"),
        ("Dear ImGui", get_ver("imgui"), "MIT License"),
        ("spdlog", get_ver("spdlog"), "MIT License"),
        ("yaml-cpp", get_ver("yaml-cpp"), "MIT License"),
        ("Boehm GC", get_ver("gc"), "MIT-style"),
        ("AJA NTV2 SDK", get_ver("aja"), "MIT License"),
        ("Blackmagic DeckLink SDK", get_ver("bmd"), "Proprietary"),
    ]

    return vfx_deps, other_deps, rv_specific_deps


def generate_about_cpp(
    output_file,
    compiler,
    build_type,
    cxx_flags,
    vfx_platform,
    build_root,
    platform,
    arch,
    app_name,
    versions_str="",
    git_hash="",
):
    """Generate the about_rv.cpp file"""

    git_commit = get_git_info(git_hash)
    versions = parse_versions(versions_str)
    vfx_deps, other_deps, rv_specific_deps = get_dependencies_info(vfx_platform, versions, app_name, platform)
    build_date = datetime.now().strftime("%B %d, %Y")

    # Build the HTML content
    html_content = []
    html_content.append("<p>")
    html_content.append(f"<b>{platform} {arch}</b>")
    html_content.append("</p>")
    html_content.append("<p>")
    html_content.append(f"Compiled using <b>{compiler}</b>")
    html_content.append("</p>")
    html_content.append("<p>")
    html_content.append(f"Build identifier: {app_name}, HEAD={git_commit}")
    html_content.append("</p>")
    html_content.append("<p>")
    html_content.append(f"Built on: {build_date}")
    html_content.append("</p>")

    # Single table with both sections
    html_content.append("<p><hr></p>")
    html_content.append("<p>")
    html_content.append('<table border="0" cellpadding="4" cellspacing="0" width="100%">')

    # VFX Platform header row
    if vfx_platform:
        # Extract year from platform (e.g., "CY2024" -> "2024")
        vfx_year = vfx_platform.replace("CY", "") if vfx_platform.startswith("CY") else vfx_platform
        html_content.append("<tr>")
        html_content.append(f'<td colspan="3"><b>VFX Reference Platform {vfx_year}</b></td>')
        html_content.append("</tr>")

        # Column headers
        html_content.append("<tr>")
        html_content.append("<td><b>Description</b></td>")
        html_content.append("<td><b>Version</b></td>")
        html_content.append("<td><b>License</b></td>")
        html_content.append("</tr>")

        # VFX Platform dependencies
        for desc, version, license in vfx_deps:
            html_content.append("<tr>")
            html_content.append(f"<td>{desc}</td>")
            html_content.append(f"<td>{version}</td>")
            html_content.append(f"<td>{license}</td>")
            html_content.append("</tr>")

        # Empty row separator
        html_content.append('<tr><td colspan="3">&nbsp;</td></tr>')

    # Other Dependencies header row
    html_content.append("<tr>")
    html_content.append('<td colspan="3"><b>Other Dependencies</b></td>')
    html_content.append("</tr>")

    # Column headers for other dependencies
    html_content.append("<tr>")
    html_content.append("<td><b>Description</b></td>")
    html_content.append("<td><b>Version</b></td>")
    html_content.append("<td><b>License</b></td>")
    html_content.append("</tr>")

    # Other dependencies
    for desc, version, license in other_deps:
        html_content.append("<tr>")
        html_content.append(f"<td>{desc}</td>")
        html_content.append(f"<td>{version}</td>")
        html_content.append(f"<td>{license}</td>")
        html_content.append("</tr>")

    # RV-Specific Components section (only for commercial RV)
    if rv_specific_deps:
        # Empty row separator
        html_content.append('<tr><td colspan="3">&nbsp;</td></tr>')

        # RV Components header row
        html_content.append("<tr>")
        html_content.append('<td colspan="3"><b>RV-Specific Components</b></td>')
        html_content.append("</tr>")

        # Column headers
        html_content.append("<tr>")
        html_content.append("<td><b>Description</b></td>")
        html_content.append("<td><b>Version</b></td>")
        html_content.append("<td><b>License</b></td>")
        html_content.append("</tr>")

        # RV-specific components
        for desc, version, license in rv_specific_deps:
            html_content.append("<tr>")
            html_content.append(f"<td>{desc}</td>")
            html_content.append(f"<td>{version}</td>")
            html_content.append(f"<td>{license}</td>")
            html_content.append("</tr>")

    html_content.append("</table>")
    html_content.append("</p>")
    html_content.append("<p><hr></p>")
    html_content.append("<p>")
    html_content.append("For detailed license information, please see the THIRD-PARTY.md file ")
    html_content.append("included with this distribution or visit the Open RV GitHub repository.")
    html_content.append("</p>")

    # Join without newlines - HTML doesn't need them
    html_str = "".join(html_content)

    # Escape quotes for C++
    html_str = html_str.replace('"', '\\"')

    # Generate the C++ file
    cpp_content = f'''//
// Copyright (C) 2025  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is auto-generated by generate_about_rv.py
// DO NOT EDIT MANUALLY

const char* about_RV = "{html_str}";
'''

    # Ensure the output directory exists
    try:
        output_path = Path(output_file)
        print(f"Creating directory: {output_path.parent}")
        output_path.parent.mkdir(parents=True, exist_ok=True)

        print(f"Writing to: {output_path}")
        with open(output_file, "w", encoding="utf-8", newline="\n") as f:
            f.write(cpp_content)

        # Verify the file was written
        if not output_path.exists():
            print(f"ERROR: File was not created: {output_file}", file=sys.stderr)
            sys.exit(1)

        file_size = output_path.stat().st_size
        print(f"Generated {output_file} ({file_size} bytes)")

    except Exception as e:
        print(f"ERROR generating {output_file}: {e}", file=sys.stderr)
        import traceback

        traceback.print_exc()
        sys.exit(1)


if __name__ == "__main__":
    if len(sys.argv) < 9:
        print(
            "Usage: generate_about_rv.py <output_file> <compiler> <build_type> "
            + "<cxx_flags> <vfx_platform> <build_root> <platform> <arch> <app_name> [versions] [git_hash]"
        )
        sys.exit(1)

    output_file = sys.argv[1]
    compiler = sys.argv[2]
    build_type = sys.argv[3]
    cxx_flags = sys.argv[4]
    vfx_platform = sys.argv[5]
    build_root = sys.argv[6]
    platform = sys.argv[7]
    arch = sys.argv[8]
    app_name = sys.argv[9] if len(sys.argv) > 9 else "Open RV"
    versions_str = sys.argv[10] if len(sys.argv) > 10 else ""
    git_hash = sys.argv[11] if len(sys.argv) > 11 else ""

    generate_about_cpp(
        output_file,
        compiler,
        build_type,
        cxx_flags,
        vfx_platform,
        build_root,
        platform,
        arch,
        app_name,
        versions_str,
        git_hash,
    )
