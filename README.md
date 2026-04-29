# Visto

---

[![Visto](docs/images/Visto_icon.png)](https://github.com/AcademySoftwareFoundation/Visto.git)
---

![Supported Versions](https://img.shields.io/badge/python-3.11-blue)
[![Supported VFX Platform Versions](https://img.shields.io/badge/vfx%20platform-2024-lightgrey.svg)](http://www.vfxplatform.com/)
[![docs](https://readthedocs.org/projects/aswf-openrv/badge/?version=latest)](https://aswf-openrv.readthedocs.io/en/latest)

## Overview

Visto is an image and sequence viewer for VFX and animation artists.
Visto is high-performant, hardware accelerated, and pipeline-friendly.

[Visto Documentation on Read the Docs](https://aswf-openrv.readthedocs.io/en/latest/)

## Cloning the repository

The quickest way to download Visto's source code is to clone this repository.

```bash
# Don't forget the --recursive flag! 
git clone --recursive https://github.com/AcademySoftwareFoundation/Visto.git
```

However, if you'd like to actually build Visto or contribute to the project, we recommend you follow the instructions below *before* cloning the repository, as it involves quite a lot more setup than just cloning. If that's your case, skip this step for now, we'll get to cloning in a bit.

## Contributing to Visto

We welcome community contributions. In general, to maximize your chances of successfully building and contributing to Visto, you should:

- Be very familiar with git/github and its usage.
- Have your own github account, where you will fork this repository
- Clone your forked repository instead of the main Visto repository
- Create a branch from your fork's 'main' branch before modifying code
- Fix conflicts prior to creating a pull request
- Update your branch (git pull, git rebase) before creating your pull request

If you're comfortable and familiar with the above, we are looking forward to your contributions. Just continue reading, we'll help you along with your setup.

## Building the workstation

Visto is currently supported on macOS Ventura 13.x and later, Windows 10 and later, and Rocky Linux 8.x/9.x.

You should start by setting up the platform-specific packages and dependencies for your operating system:

- [macOS Ventura and later](https://aswf-openrv.readthedocs.io/en/latest/build_system/config_macos.html)
- [Windows 10 and later](https://aswf-openrv.readthedocs.io/en/latest/build_system/config_windows.html)
- [Rocky Linux 8/9](https://aswf-openrv.readthedocs.io/en/latest/build_system/config_linux_rocky89.html)

Once this platform-specific setup is complete, move to the common build instructions.

- [Common build instructions](https://aswf-openrv.readthedocs.io/en/latest/build_system/config_common_build.html)

Note that as of 2025, CentOS 7 is no longer supported as a platform for Visto since CentOS was end-of-life'd as of summer of 2024.

Also note that support for other *current* operating systems (typically, other Linux variants) is on a "if-we-can-find-time" basis because our primary mandate is to support immediate customer needs on official VFX reference platforms. We would however welcome community-driven contributions to support other platforms.

## About third-party licenses

See [THIRD-PARTY.md](THIRD-PARTY.md) for license information about portions of Visto that have been imported from other projects.
