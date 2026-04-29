# Changelog

All notable changes to this project will be documented in this file.

## [3.2.0](https://github.com/AcademySoftwareFoundation/OpenRV/compare/v3.1.0...v3.2.0) - 2026-04-29

### Features

- SG-41979: Add hotkey and env var for no sequence formation during drag and drop ([#1139](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1139)) ([45417a5](https://github.com/AcademySoftwareFoundation/OpenRV/commit/45417a5ff17c73ed469ae82506bf8414a0f13e85))

### Bug Fixes

- SG-40350: Persist console "Show on" setting on all close methods ([#1078](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1078)) ([287a2a9](https://github.com/AcademySoftwareFoundation/OpenRV/commit/287a2a9745048c4e97eb0eb01352390b6e118855))
- SG-41698: Add Alt modifier check for mouse wheel events ([#1060](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1060)) ([0d61a84](https://github.com/AcademySoftwareFoundation/OpenRV/commit/0d61a841a9e0d7c28c4ffdb11d75c94b5f4bd172))
- SG-42334: Fix random crashes involving SoundTrackIPNode ([#1152](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1152)) ([75bfcc7](https://github.com/AcademySoftwareFoundation/OpenRV/commit/75bfcc7ddb86657d7444e18fc9b673b073efd79b))
- Revert fix: Fix crashes when calling sourcesAtFrame when clearing ([#1185](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1185)) ([b950e1d](https://github.com/AcademySoftwareFoundation/OpenRV/commit/b950e1d66b30c726732d65a1bae93b3423edad88))
- Update markdownlint to latest version ([#1148](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1148)) ([887c3d6](https://github.com/AcademySoftwareFoundation/OpenRV/commit/887c3d6d109463435a3b4fe7a6fd4f5202f709eb))
- SG-42144: Fix off-by-one frame count error for audio-only files ([#1068](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1068)) ([c4e0e15](https://github.com/AcademySoftwareFoundation/OpenRV/commit/c4e0e15cf57b3aa56c402344268a565f4c3e2242))
- Fix crashes when calling sourcesAtFrame when clearing ([#1122](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1122)) ([02a537b](https://github.com/AcademySoftwareFoundation/OpenRV/commit/02a537b494ba4862f92750a6cea3a69fcb4f10c8))

### Build System

- SG-42137: disable x11 features on macos ([#1117](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1117)) ([2079e0b](https://github.com/AcademySoftwareFoundation/OpenRV/commit/2079e0b7e56de17b4d36355ed12118b75ab9df40))
- SG-42121: Update libPNG to 1.6.55 to address CVE-2026-25646 ([#1149](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1149)) ([848fe73](https://github.com/AcademySoftwareFoundation/OpenRV/commit/848fe7348453c48a58a43a3e2fe4ca2d558116ae))
- SG-42130: XCode26 support ([#1091](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1091)) ([946e73c](https://github.com/AcademySoftwareFoundation/OpenRV/commit/946e73c5fd33a6a55e10e3828e97dc3d24c7030d))
- Speed up builds with ccahe/scache ([#1123](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1123)) ([0645877](https://github.com/AcademySoftwareFoundation/OpenRV/commit/06458773dba51107130c00d84f99c72a50ae4af8))

### GitHub Actions

- retarget release-pr workflow to RB-3.X.X-VFX2024 ([ee1d86d](https://github.com/AcademySoftwareFoundation/OpenRV/commit/ee1d86da3918d22b00db1865fe6b362283eebb68))
- Staging refactor ([#1159](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1159)) ([67b59a7](https://github.com/AcademySoftwareFoundation/OpenRV/commit/67b59a75459ee22a81c2857b2664a2cedb43bd4b))
- Fix detect change job ([#1158](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1158)) ([ef2181f](https://github.com/AcademySoftwareFoundation/OpenRV/commit/ef2181fe2a4435033b4fb696d6425bd0cc4df0e8))
- SG-42352: Optimize CI strategy ([#1155](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1155)) ([0e7b3e6](https://github.com/AcademySoftwareFoundation/OpenRV/commit/0e7b3e603a0551a8a5f33984554c89d7c047c4f4))
- Split ci.yml into platform specifics file ([#1154](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1154)) ([f3c38ee](https://github.com/AcademySoftwareFoundation/OpenRV/commit/f3c38ee4553e9c9b7b061eb5fed44b8a23072deb))
- Use gitcliff instead of release-please ([#1150](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1150)) ([1a414f3](https://github.com/AcademySoftwareFoundation/OpenRV/commit/1a414f34b6a066a1659297149d2cccdd6c899220))
- Add semantic versioning for OpenRV to automate change logs and release ([#1143](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1143)) ([42b3cd4](https://github.com/AcademySoftwareFoundation/OpenRV/commit/42b3cd4bd46f413899b478bb200c355dbf62702b))
## [1.0.0](https://github.com/AcademySoftwareFoundation/OpenRV/compare/...v1.0.0) - 2023-01-18

