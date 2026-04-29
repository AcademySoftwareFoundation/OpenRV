# Changelog

All notable changes to this project will be documented in this file.

## [3.2.0](https://github.com/AcademySoftwareFoundation/OpenRV/compare/v3.1.0...v3.2.0) - 2026-04-29

### Features

- SG-42387 - Implement the infrastructure to allow OpenRV to find dependencies ([#1230](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1230)) ([b379c71](https://github.com/AcademySoftwareFoundation/OpenRV/commit/b379c71c91932df6171f9e384b3147dd72180c41))
- 998: SG-42419: Add support for Apple ProRes decode SDK on Linux and Windows ([#1186](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1186)) ([24d2a67](https://github.com/AcademySoftwareFoundation/OpenRV/commit/24d2a67d894506a7207067e0891194273dbf0d4e))
- SG-42241: RV: Generate thumbnail before filmstrips ([#1235](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1235)) ([98a33ee](https://github.com/AcademySoftwareFoundation/OpenRV/commit/98a33ee3398bdd1da0634c5c503bd2d26d9bb00b))
- SG-42241: RV: Implement the ability to fetch filmstrips and thumbnails for sources in coming into RV's Session Manager ([#1200](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1200)) ([0eeaa79](https://github.com/AcademySoftwareFoundation/OpenRV/commit/0eeaa791152101dbd25ef6d780b601b94643e1a6))
- SG-27382: Add 12G SDI single-link support for the AJA Kona 5 card ([#1198](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1198)) ([d1ca02c](https://github.com/AcademySoftwareFoundation/OpenRV/commit/d1ca02c6a17b982195de8114ff3facb16e433619))
- SG-41316: Add Dissolve blend composite mode to stackIPNode with UI controls ([#940](https://github.com/AcademySoftwareFoundation/OpenRV/pull/940)) ([6d1c478](https://github.com/AcademySoftwareFoundation/OpenRV/commit/6d1c478a20380ac1163fa9cbb7ef3da4aa449617))
- SG-42131 - Migrate to OpenImageIO 3.x and build it with OpenColorIO ([#1087](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1087)) ([defa0a9](https://github.com/AcademySoftwareFoundation/OpenRV/commit/defa0a93be994cc9c09186c5282e154c5bc099ff))
- SG-41979: Add hotkey and env var for no sequence formation during drag and drop ([#1139](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1139)) ([45417a5](https://github.com/AcademySoftwareFoundation/OpenRV/commit/45417a5ff17c73ed469ae82506bf8414a0f13e85))

### Bug Fixes

- SG-42241: Address comment in PR 1200 ([#1236](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1236)) ([0971176](https://github.com/AcademySoftwareFoundation/OpenRV/commit/097117679b9fdc0afa3c39c47449b814c86bb3a2))
- GLSL version directive never being applied ([#1221](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1221)) ([9fcf7a2](https://github.com/AcademySoftwareFoundation/OpenRV/commit/9fcf7a26a49072321299820a80574aceff2adef0))
- SG-42902: Fix seg fault on Linux when displays are stacked in the system settings ([#1226](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1226)) ([f37cc3f](https://github.com/AcademySoftwareFoundation/OpenRV/commit/f37cc3f17f1366bd0b687699c912c9812dd55996))
- SG-35913: Fix importing files with long path names on Windows ([#1215](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1215)) ([5b8c0f8](https://github.com/AcademySoftwareFoundation/OpenRV/commit/5b8c0f874c370f8af13f3ebc55e00d62f86d6982))
- SG-42386: Replace linux commands with CMake ([#1195](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1195)) ([db74d22](https://github.com/AcademySoftwareFoundation/OpenRV/commit/db74d2263322d2e8a66911086450e51d810108d9))
- Improve scrubbing annotated frames ([#1197](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1197)) ([3e5c402](https://github.com/AcademySoftwareFoundation/OpenRV/commit/3e5c402eccd2fc811e9e41def6d087ee8e8c5019))
- SG-40350: Persist console "Show on" setting on all close methods ([#1078](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1078)) ([287a2a9](https://github.com/AcademySoftwareFoundation/OpenRV/commit/287a2a9745048c4e97eb0eb01352390b6e118855))
- SG-41698: Add Alt modifier check for mouse wheel events ([#1060](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1060)) ([0d61a84](https://github.com/AcademySoftwareFoundation/OpenRV/commit/0d61a841a9e0d7c28c4ffdb11d75c94b5f4bd172))
- SG-42334: Fix random crashes involving SoundTrackIPNode ([#1152](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1152)) ([75bfcc7](https://github.com/AcademySoftwareFoundation/OpenRV/commit/75bfcc7ddb86657d7444e18fc9b673b073efd79b))
- Revert fix: Fix crashes when calling sourcesAtFrame when clearing ([#1185](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1185)) ([b950e1d](https://github.com/AcademySoftwareFoundation/OpenRV/commit/b950e1d66b30c726732d65a1bae93b3423edad88))
- Update markdownlint to latest version ([#1148](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1148)) ([887c3d6](https://github.com/AcademySoftwareFoundation/OpenRV/commit/887c3d6d109463435a3b4fe7a6fd4f5202f709eb))
- SG-42144: Fix off-by-one frame count error for audio-only files ([#1068](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1068)) ([c4e0e15](https://github.com/AcademySoftwareFoundation/OpenRV/commit/c4e0e15cf57b3aa56c402344268a565f4c3e2242))
- Fix crashes when calling sourcesAtFrame when clearing ([#1122](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1122)) ([02a537b](https://github.com/AcademySoftwareFoundation/OpenRV/commit/02a537b494ba4862f92750a6cea3a69fcb4f10c8))

### Build System

- Fix namespace for OIIO 2.5 and VFX2023 ([#1228](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1228)) ([ef5b210](https://github.com/AcademySoftwareFoundation/OpenRV/commit/ef5b210121b100f70ede3d0020df55b98b038392))
- Small build improvement for speed and fix CI for CY2023 ([#1223](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1223)) ([adb66ba](https://github.com/AcademySoftwareFoundation/OpenRV/commit/adb66ba60e68037482c31ee52c86e1adf94f6b7a))
- Help OpenImageIO find OpenColorIO ([#1220](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1220)) ([4c487b1](https://github.com/AcademySoftwareFoundation/OpenRV/commit/4c487b1e25ebfe8582bb9ae0fc96d8f046646713))
- Fix various with the debug build on Windows ([#1222](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1222)) ([2889e3c](https://github.com/AcademySoftwareFoundation/OpenRV/commit/2889e3c0409855b42b5bce39a4144dd5a5c6a04b))
- Improve resolution of Python for OCIO ([#1219](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1219)) ([7377bcc](https://github.com/AcademySoftwareFoundation/OpenRV/commit/7377bcc2950357e8d655ec542f893ce0b3225ac6))
- SG-42407: Replace autotools with CMake for PCRE2 and atomic_ops ([#1196](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1196)) ([8504b90](https://github.com/AcademySoftwareFoundation/OpenRV/commit/8504b90a7916cb8f5c119e770c1d0ebcbc9b3bdc))
- SG-42175: Update AJA SDK version to 17.6.0 ([#1192](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1192)) ([2d24281](https://github.com/AcademySoftwareFoundation/OpenRV/commit/2d24281d3d2e39862923eaebf5d948897e8d9937))
- add setuptools-rust to RV_PYTHON_WHEEL_SAFE ([#1213](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1213)) ([1e00148](https://github.com/AcademySoftwareFoundation/OpenRV/commit/1e00148fac50adc151933036efe05072f58550c8))
- SG-42137: disable x11 features on macos ([#1117](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1117)) ([2079e0b](https://github.com/AcademySoftwareFoundation/OpenRV/commit/2079e0b7e56de17b4d36355ed12118b75ab9df40))
- SG-42121: Update libPNG to 1.6.55 to address CVE-2026-25646 ([#1149](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1149)) ([848fe73](https://github.com/AcademySoftwareFoundation/OpenRV/commit/848fe7348453c48a58a43a3e2fe4ca2d558116ae))
- SG-42130: XCode26 support ([#1091](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1091)) ([946e73c](https://github.com/AcademySoftwareFoundation/OpenRV/commit/946e73c5fd33a6a55e10e3828e97dc3d24c7030d))
- Speed up builds with ccahe/scache ([#1123](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1123)) ([0645877](https://github.com/AcademySoftwareFoundation/OpenRV/commit/06458773dba51107130c00d84f99c72a50ae4af8))

### GitHub Actions

- Add cmake to wheel-safe packages for pip install ([#1212](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1212)) ([ba88389](https://github.com/AcademySoftwareFoundation/OpenRV/commit/ba88389b24bd411cdd7b5469db7c4d0632acc793))
- Staging refactor ([#1159](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1159)) ([67b59a7](https://github.com/AcademySoftwareFoundation/OpenRV/commit/67b59a75459ee22a81c2857b2664a2cedb43bd4b))
- Fix detect change job ([#1158](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1158)) ([ef2181f](https://github.com/AcademySoftwareFoundation/OpenRV/commit/ef2181fe2a4435033b4fb696d6425bd0cc4df0e8))
- SG-42352: Optimize CI strategy ([#1155](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1155)) ([0e7b3e6](https://github.com/AcademySoftwareFoundation/OpenRV/commit/0e7b3e603a0551a8a5f33984554c89d7c047c4f4))
- Split ci.yml into platform specifics file ([#1154](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1154)) ([f3c38ee](https://github.com/AcademySoftwareFoundation/OpenRV/commit/f3c38ee4553e9c9b7b061eb5fed44b8a23072deb))
- Use gitcliff instead of release-please ([#1150](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1150)) ([1a414f3](https://github.com/AcademySoftwareFoundation/OpenRV/commit/1a414f34b6a066a1659297149d2cccdd6c899220))
- Add semantic versioning for OpenRV to automate change logs and release ([#1143](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1143)) ([42b3cd4](https://github.com/AcademySoftwareFoundation/OpenRV/commit/42b3cd4bd46f413899b478bb200c355dbf62702b))
## [1.0.0](https://github.com/AcademySoftwareFoundation/OpenRV/compare/...v1.0.0) - 2023-01-18

