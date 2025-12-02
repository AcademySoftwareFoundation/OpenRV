#!/usr/bin/env python3
#
# Copyright (C) 2025  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# Generate about_rv.cpp with build and dependency information

import argparse
import sys
from datetime import datetime
from pathlib import Path


def get_git_info(git_hash_from_cmake=""):
    """Get git commit hash from CMake"""
    return git_hash_from_cmake if git_hash_from_cmake else "unknown"


def parse_versions(versions_str):
    """Parse the versions string into a dictionary"""
    versions = {}
    if versions_str:
        for item in versions_str.split(","):
            if ":" in item:
                key, value = item.split(":", 1)
                versions[key] = value
    return versions


def get_dependencies_info(versions, app_name, platform=""):
    """Generate dependency information using detected versions"""

    def get_version(key, default="Unknown"):
        version = versions.get(key, default)
        # Strip FFmpeg's 'n' prefix (e.g., n6.1.2 -> 6.1.2)
        if key == "FFmpeg" and version.startswith("n"):
            version = version[1:]
        return version

    # OpenRV uses LGPL, commercial RV uses Qt Commercial
    is_commercial_rv = app_name == "RV"
    qt_license = "Qt Commercial" if is_commercial_rv else "LGPL v3"
    pyside_license = "Qt Commercial" if is_commercial_rv else "LGPL v3"

    is_macos = "darwin" in platform.lower() or "macos" in platform.lower()

    vfx_deps = [
        ("Boost", get_version("Boost"), "Boost Software License"),
        ("CMake", get_version("CMake"), "BSD 3-Clause"),
        ("Imath", get_version("Imath"), "BSD 3-Clause"),
        ("NumPy", get_version("numpy", "1.24+"), "BSD 3-Clause"),
        ("OpenColorIO", get_version("OpenColorIO"), "BSD 3-Clause"),
        ("OpenEXR", get_version("OpenEXR"), "BSD 3-Clause"),
        ("OpenImageIO", get_version("OpenImageIO"), "Apache 2.0"),
        ("OpenTimelineIO", get_version("otio"), "Apache 2.0"),
        ("PySide", get_version("PySide"), pyside_license),
        ("Python", get_version("Python"), "PSF License"),
        ("Qt Framework", get_version("Qt"), qt_license),
    ]

    # RV-specific proprietary components (only for commercial RV, alphabetical order)
    rv_specific_deps = []
    if is_commercial_rv:
        # Apple ProRes is only available on macOS (and not on ARM64 currently)
        if is_macos:
            prores_ver = get_version("prores", "Not Used")
            rv_specific_deps.append(("Apple ProRes", prores_ver, "Apple Proprietary"))

        # Always add these for commercial RV (version will show "Not Used" if not available)
        rv_specific_deps.extend(
            [
                ("ARRI SDK", get_version("arriraw", "Not Used"), "ARRI Proprietary"),
                ("NDI SDK", get_version("ndi", "Not Used"), "NDI Proprietary"),
                ("RED R3D SDK", get_version("r3dsdk", "Not Used"), "RED Proprietary"),
                ("x264", get_version("x264", "Not Used"), "x264 Commercial License"),
            ]
        )

    # Other third-party components (alphabetical order)
    other_deps = [
        ("AJA NTV2 SDK", get_version("aja"), "MIT License"),
        ("Blackmagic DeckLink SDK", get_version("bmd"), "Proprietary"),
        ("Boehm GC", get_version("gc"), "MIT-style"),
        ("dav1d", get_version("dav1d"), "BSD 2-Clause"),
        ("Dear ImGui", get_version("imgui"), "MIT License"),
        ("Expat", get_version("expat"), "MIT License"),
        ("FFmpeg", get_version("FFmpeg"), "LGPL v2.1+"),
        ("GLEW", get_version("GLEW"), "Modified BSD / MIT"),
        ("libjpeg-turbo", get_version("jpegturbo"), "BSD-style"),
        ("libpng", get_version("png"), "libpng License"),
        ("LibRaw", get_version("raw"), "LGPL v2.1 / CDDL"),
        ("libtiff", get_version("tiff"), "libtiff License"),
        ("libwebp", get_version("webp"), "BSD 3-Clause"),
        ("nanobind", get_version("nanobind"), "BSD 3-Clause"),
        ("OpenJPEG", get_version("openjpeg"), "BSD 2-Clause"),
        ("OpenJPH", get_version("openjph"), "BSD 2-Clause"),
        ("OpenSSL", get_version("OpenSSL"), "Apache License 2.0"),
        ("PCRE2", get_version("pcre2"), "BSD License"),
        ("spdlog", get_version("spdlog"), "MIT License"),
        ("yaml-cpp", get_version("yaml-cpp"), "MIT License"),
        ("zlib", get_version("zlib"), "zlib License"),
    ]

    return vfx_deps, other_deps, rv_specific_deps


def generate_about_cpp(
    output_file,
    compiler,
    vfx_platform,
    platform,
    arch,
    app_name,
    versions_str="",
    git_hash="",
):
    """Generate the about_rv.cpp file"""

    git_commit = get_git_info(git_hash)
    versions = parse_versions(versions_str)
    vfx_deps, other_deps, rv_specific_deps = get_dependencies_info(versions, app_name, platform)
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
    parser = argparse.ArgumentParser(
        description="Generate about_rv.cpp with build and dependency information"
    )
    parser.add_argument("output_file", help="Path to the output C++ file")
    parser.add_argument("compiler", help="Compiler name and version")
    parser.add_argument("vfx_platform", help="VFX Platform version (e.g., CY2024)")
    parser.add_argument("platform", help="Target platform (e.g., macOS, Linux, Windows)")
    parser.add_argument("arch", help="Target architecture (e.g., x86_64, arm64)")
    parser.add_argument("app_name", nargs="?", default="Open RV", help="Application name")
    parser.add_argument("versions", nargs="?", default="", help="Comma-separated dependency versions")
    parser.add_argument("git_hash", nargs="?", default="", help="Git commit hash")

    args = parser.parse_args()

    print(f"=== generate_about_rv.py: generating {args.output_file} ===", flush=True)
    generate_about_cpp(
        args.output_file,
        args.compiler,
        args.vfx_platform,
        args.platform,
        args.arch,
        args.app_name,
        args.versions,
        args.git_hash,
    )
