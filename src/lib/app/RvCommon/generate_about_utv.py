#!/usr/bin/env python3
#
# Copyright (C) 2025  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# Generate about_rv.cpp with build and dependency information

import argparse
import subprocess
import json
import sys
from datetime import datetime
from pathlib import Path


def get_git_info(git_hash_from_cmake=""):
    """Get git commit hash from CMake"""
    return git_hash_from_cmake if git_hash_from_cmake else "unknown"


def get_runtime_placeholder(name):
    return f"%RUNTIME_{name.upper()}%"


def get_pkg_version(pkg_name, brew_name=None):
    try:
        return subprocess.check_output(
            ["pkg-config", "--modversion", pkg_name], stderr=subprocess.DEVNULL, text=True
        ).strip()
    except Exception:
        if brew_name:
            try:
                out = subprocess.check_output(
                    ["brew", "info", "--json", brew_name], stderr=subprocess.DEVNULL, text=True
                )

                data = json.loads(out)
                return data[0]["versions"]["stable"]
            except Exception:
                pass
        return "Unknown"


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
    is_commercial_rv = app_name == "RV"
    qt_license = "Qt Commercial" if is_commercial_rv else "LGPL v3"
    pyside_license = "Qt Commercial" if is_commercial_rv else "LGPL v3"

    is_macos = "darwin" in platform.lower() or "macos" in platform.lower()

    vfx_deps = [
        ("Boost", get_pkg_version("boost"), get_runtime_placeholder("boost"), "Boost Software License"),
        ("Imath", get_pkg_version("Imath", "imath"), get_runtime_placeholder("imath"), "BSD 3-Clause"),
        ("NumPy", "Unknown", get_runtime_placeholder("numpy"), "BSD 3-Clause"),
        (
            "OpenColorIO",
            get_pkg_version("OpenColorIO", "opencolorio"),
            get_runtime_placeholder("opencolorio"),
            "BSD 3-Clause",
        ),
        ("OpenEXR", get_pkg_version("OpenEXR", "openexr"), get_runtime_placeholder("openexr"), "BSD 3-Clause"),
        (
            "OpenImageIO",
            get_pkg_version("OpenImageIO", "openimageio"),
            get_runtime_placeholder("openimageio"),
            "Apache 2.0",
        ),
        ("OpenTimelineIO", "Unknown", get_runtime_placeholder("opentimelineio"), "Apache 2.0"),
        ("PySide", get_pkg_version("PySide6"), get_runtime_placeholder("pyside"), pyside_license),
        ("Python", get_pkg_version("python3", "python"), get_runtime_placeholder("python"), "PSF License"),
        ("Qt Framework", get_pkg_version("Qt6Core", "qt"), get_runtime_placeholder("qt"), qt_license),
    ]

    rv_specific_deps = []
    if is_commercial_rv:
        if is_macos:
            rv_specific_deps.append(
                ("Apple ProRes", "Not Used", get_runtime_placeholder("prores"), "Apple Proprietary")
            )
        rv_specific_deps.extend(
            [
                ("ARRI SDK", "Not Used", get_runtime_placeholder("arri"), "ARRI Proprietary"),
                ("NDI SDK", "Not Used", get_runtime_placeholder("ndi"), "NDI Proprietary"),
                ("RED R3D SDK", "Not Used", get_runtime_placeholder("red"), "RED Proprietary"),
                ("x264", "Not Used", get_runtime_placeholder("x264"), "x264 Commercial License"),
            ]
        )

    other_deps = [
        ("AJA NTV2 SDK", "Unknown", get_runtime_placeholder("aja"), "MIT License"),
        ("Blackmagic DeckLink SDK", "Unknown", get_runtime_placeholder("bmd"), "Proprietary"),
        ("Boehm GC", get_pkg_version("bdw-gc", "bdw-gc"), get_runtime_placeholder("bdw-gc"), "MIT-style"),
        ("dav1d", get_pkg_version("dav1d", "dav1d"), get_runtime_placeholder("dav1d"), "BSD 2-Clause"),
        ("Dear ImGui", "Unknown", get_runtime_placeholder("imgui"), "MIT License"),
        ("Expat", get_pkg_version("expat", "expat"), get_runtime_placeholder("expat"), "MIT License"),
        ("FFmpeg", get_pkg_version("libavcodec", "ffmpeg"), get_runtime_placeholder("ffmpeg"), "LGPL v2.1+"),
        ("GLEW", get_pkg_version("glew", "glew"), get_runtime_placeholder("glew"), "Modified BSD / MIT"),
        (
            "libjpeg-turbo",
            get_pkg_version("libturbojpeg", "jpeg-turbo"),
            get_runtime_placeholder("jpeg-turbo"),
            "BSD-style",
        ),
        ("libpng", get_pkg_version("libpng", "libpng"), get_runtime_placeholder("libpng"), "libpng License"),
        ("LibRaw", get_pkg_version("libraw", "libraw"), get_runtime_placeholder("libraw"), "LGPL v2.1 / CDDL"),
        ("libtiff", get_pkg_version("libtiff-4", "libtiff"), get_runtime_placeholder("libtiff"), "libtiff License"),
        ("libwebp", get_pkg_version("libwebp", "webp"), get_runtime_placeholder("webp"), "BSD 3-Clause"),
        ("nanobind", "Unknown", get_runtime_placeholder("nanobind"), "BSD 3-Clause"),
        ("OpenJPEG", get_pkg_version("libopenjp2", "openjpeg"), get_runtime_placeholder("openjpeg"), "BSD 2-Clause"),
        ("OpenJPH", "Unknown", get_runtime_placeholder("openjph"), "BSD 2-Clause"),
        ("OpenSSL", get_pkg_version("openssl", "openssl"), get_runtime_placeholder("openssl"), "Apache License 2.0"),
        ("PCRE2", get_pkg_version("libpcre2-8", "pcre2"), get_runtime_placeholder("pcre2"), "BSD License"),
        ("spdlog", get_pkg_version("spdlog", "spdlog"), get_runtime_placeholder("spdlog"), "MIT License"),
        ("yaml-cpp", get_pkg_version("yaml-cpp", "yaml-cpp"), get_runtime_placeholder("yaml-cpp"), "MIT License"),
        ("zlib", get_pkg_version("zlib", "zlib"), get_runtime_placeholder("zlib"), "zlib License"),
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
        html_content.append(f'<td colspan="4"><b>VFX Reference Platform {vfx_year}</b></td>')
        html_content.append("</tr>")

        # Column headers
        html_content.append("<tr>")
        html_content.append("<td><b>Description</b></td>")
        html_content.append("<td><b>Build Version</b></td>")
        html_content.append("<td><b>Runtime Version</b></td>")
        html_content.append("<td><b>License</b></td>")
        html_content.append("</tr>")

        # VFX Platform dependencies
        for desc, build_version, runtime_version, license in vfx_deps:
            html_content.append("<tr>")
            html_content.append(f"<td>{desc}</td>")
            html_content.append(f"<td>{build_version}</td>")
            html_content.append(f"<td>{runtime_version}</td>")
            html_content.append(f"<td>{license}</td>")
            html_content.append("</tr>")

        # Empty row separator
        html_content.append('<tr><td colspan="4">&nbsp;</td></tr>')

    # Other Dependencies header row
    html_content.append("<tr>")
    html_content.append('<td colspan="4"><b>Other Dependencies</b></td>')
    html_content.append("</tr>")

    # Column headers for other dependencies
    html_content.append("<tr>")
    html_content.append("<td><b>Description</b></td>")
    html_content.append("<td><b>Version</b></td>")
    html_content.append("<td><b>License</b></td>")
    html_content.append("</tr>")

    # Other dependencies
    for desc, build_version, runtime_version, license in other_deps:
        html_content.append("<tr>")
        html_content.append(f"<td>{desc}</td>")
        html_content.append(f"<td>{build_version}</td>")
        html_content.append(f"<td>{runtime_version}</td>")
        html_content.append(f"<td>{license}</td>")
        html_content.append("</tr>")

    # RV-Specific Components section (only for commercial RV)
    if rv_specific_deps:
        # Empty row separator
        html_content.append('<tr><td colspan="4">&nbsp;</td></tr>')

        # RV Components header row
        html_content.append("<tr>")
        html_content.append('<td colspan="4"><b>RV-Specific Components</b></td>')
        html_content.append("</tr>")

        # Column headers
        html_content.append("<tr>")
        html_content.append("<td><b>Description</b></td>")
        html_content.append("<td><b>Build Version</b></td>")
        html_content.append("<td><b>Runtime Version</b></td>")
        html_content.append("<td><b>License</b></td>")
        html_content.append("</tr>")

        # RV-specific components
        for desc, build_version, runtime_version, license in rv_specific_deps:
            html_content.append("<tr>")
            html_content.append(f"<td>{desc}</td>")
            html_content.append(f"<td>{build_version}</td>")
            html_content.append(f"<td>{runtime_version}</td>")
            html_content.append(f"<td>{license}</td>")
            html_content.append("</tr>")

    html_content.append("</table>")
    html_content.append("</p>")
    html_content.append("<p><hr></p>")
    html_content.append("<p>")
    html_content.append("For detailed license information, please see the THIRD-PARTY.md file ")
    html_content.append("included with this distribution or visit the UTV GitHub repository.")
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

const char* about_UTV = "{html_str}";
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
    parser = argparse.ArgumentParser(description="Generate about_rv.cpp with build and dependency information")
    parser.add_argument("output_file", help="Path to the output C++ file")
    parser.add_argument("compiler", help="Compiler name and version")
    parser.add_argument("vfx_platform", help="VFX Platform version (e.g., CY2024)")
    parser.add_argument("platform", help="Target platform (e.g., macOS, Linux, Windows)")
    parser.add_argument("arch", help="Target architecture (e.g., x86_64, arm64)")
    parser.add_argument("app_name", nargs="?", default="UTV", help="Application name")
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
