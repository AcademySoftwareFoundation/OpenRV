# Changelog

All notable changes to this project will be documented in this file.

## [4.0.1](https://github.com/AcademySoftwareFoundation/OpenRV/compare/v4.0.0...v4.0.1) - 2026-07-22

### Bug Fixes

- strip DWARF from staged Release Linux binaries ([#1354](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1354)) ([98f9d7b](https://github.com/AcademySoftwareFoundation/OpenRV/commit/98f9d7b68c0b59f3f0c33ee802c556e09f2f0fbf))
- remove automatic Mu debugging on crash handler init ([#1353](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1353)) ([679987a](https://github.com/AcademySoftwareFoundation/OpenRV/commit/679987aade256167c11c9e53243c4ad7301471d8))

### Documentation

- Set VFX platform badge to 2025 in README.md ([#1357](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1357)) ([f2ae7a7](https://github.com/AcademySoftwareFoundation/OpenRV/commit/f2ae7a70e23f69099403a5d09ff6ea691d8b72db))
- SG-43650: Update Rocky Linux build instructions ([#1349](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1349)) ([e881bf0](https://github.com/AcademySoftwareFoundation/OpenRV/commit/e881bf099c2534d7374db427753effbd0790fbe3))
## [4.0.0](https://github.com/AcademySoftwareFoundation/OpenRV/compare/v3.2.0...v4.0.0) - 2026-07-11

### Features

- SG-43604: Add thread safe consoleWrite command for Python console output ([#1292](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1292)) ([fc0eeb4](https://github.com/AcademySoftwareFoundation/OpenRV/commit/fc0eeb46e5b693ba7d550f07fd38898aa7e2a069))
- SG-42630: Add crash-dump reporting capability to RV ([#1312](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1312)) ([af258f9](https://github.com/AcademySoftwareFoundation/OpenRV/commit/af258f90779a274fa050b1481cd3cd7d4c037a31))
- Add shapes and text support to OTIO annotation reader ([#1331](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1331)) ([3dad8d0](https://github.com/AcademySoftwareFoundation/OpenRV/commit/3dad8d05a3cd8b02c6ef86983cc9291cc01fbd8b))
- SG-35309: Set and save OCIO environment variable if missing and config is chosen ([#1263](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1263)) ([d43f3c3](https://github.com/AcademySoftwareFoundation/OpenRV/commit/d43f3c341d4330ac745d5e9569ab93a9787cb91d))
- Add annotation shapes, text, input smoothing, and stamp rendering ([#1301](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1301)) ([7f598d6](https://github.com/AcademySoftwareFoundation/OpenRV/commit/7f598d6a02c3d71e483ab3fb658dddc2de738359))
- SG-42880: Add fps options 60 5994 to Control FPS menu ([#1184](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1184)) ([19ac4db](https://github.com/AcademySoftwareFoundation/OpenRV/commit/19ac4db19c332a86289ddf8b0f6dee5d4b82d23f))
- Moving to major version 4 ([#1250](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1250)) ([bc20b43](https://github.com/AcademySoftwareFoundation/OpenRV/commit/bc20b43b62dc8c0c44a81c68db3d08fdcc700afb))
- SG-43132: Move main branch to CY2025 ([#1247](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1247)) ([fcafc94](https://github.com/AcademySoftwareFoundation/OpenRV/commit/fcafc94b18578a48654f3bb9c2d641cfd3e240f8))
- SG-42387 - Implement the infrastructure to allow OpenRV to find dependencies ([#1230](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1230)) ([b379c71](https://github.com/AcademySoftwareFoundation/OpenRV/commit/b379c71c91932df6171f9e384b3147dd72180c41))

### Bug Fixes

- SG-44076: Fix session file thumbnail generation ([#1342](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1342)) ([27c479c](https://github.com/AcademySoftwareFoundation/OpenRV/commit/27c479cbae09920ba7258e12dabcb688bf91f71b))
- SG-43880: Optimize session manager inputs reordering with previews ([#1340](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1340)) ([f310145](https://github.com/AcademySoftwareFoundation/OpenRV/commit/f3101450b121f32e99e893c62a41073622f781b3))
- SG-44089 add soft_deleted to shape schemas ([#1343](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1343)) ([a003e79](https://github.com/AcademySoftwareFoundation/OpenRV/commit/a003e7953ac1c84be0807c2b020ae42f87d41c70))
- SG-43974 Make font decorators case-insensitive ([#1339](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1339)) ([51a2eb2](https://github.com/AcademySoftwareFoundation/OpenRV/commit/51a2eb2168acaaf1745499d2eb59d69bf0d31ccc))
- SG-43948 more stale paint cache fixes ([#1336](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1336)) ([6e1a55c](https://github.com/AcademySoftwareFoundation/OpenRV/commit/6e1a55c5494736cfc55a4d586ecf279753a2906d))
- SG-43970 loading pre-qt font sizes and line wrapping ([#1338](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1338)) ([21da6c4](https://github.com/AcademySoftwareFoundation/OpenRV/commit/21da6c449da52bbae158f53bf6619cc2625ed07f))
- handle unknown schema in otio import more robustly ([#1341](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1341)) ([e842f0b](https://github.com/AcademySoftwareFoundation/OpenRV/commit/e842f0bd7ebed953e6988ed31c0fff6143c7983d))
- SG-43881: Fix scrollbar on reordering with previews enabled ([#1333](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1333)) ([07cd4c2](https://github.com/AcademySoftwareFoundation/OpenRV/commit/07cd4c26085df4e1250e9cee7bfd69b97dfe8175))
- SG-43808: Fix killing rvio processes for the session manager when shutting down ([#1332](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1332)) ([584084f](https://github.com/AcademySoftwareFoundation/OpenRV/commit/584084fbdde0b8717f2fe82e5abcd18e627ca300))
- SG-43961 text size in otio_reader ([#1337](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1337)) ([71e3bf2](https://github.com/AcademySoftwareFoundation/OpenRV/commit/71e3bf2d5f6ccca1bf8f6f7112f608702a31a769))
- SG-43924 Fix text size slider in the UI ([#1334](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1334)) ([b0c430b](https://github.com/AcademySoftwareFoundation/OpenRV/commit/b0c430b0695d1fab29c7d8225fdbecf9d5150e6a))
- SG-43607: Bugfix/aces2 support ([#1288](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1288)) ([f702cd9](https://github.com/AcademySoftwareFoundation/OpenRV/commit/f702cd926b58a86da4616e29980c8ef8c378aba4))
- SG-43908: stale frame-invariant paint cache ([#1329](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1329)) ([9831b1a](https://github.com/AcademySoftwareFoundation/OpenRV/commit/9831b1ac171459ef2af9a1040dab88ac0d4618bc))
- stage macOS brush assets under Contents/Resources for codesign ([#1317](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1317)) ([0c92273](https://github.com/AcademySoftwareFoundation/OpenRV/commit/0c922732db5aa7ae59ff7f00e12e12f161811c79))
- SG-43603: Bounds-check start/num in get*Property commands ([#1293](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1293)) ([4714496](https://github.com/AcademySoftwareFoundation/OpenRV/commit/47144967c4dcaccfac281f19fe916cbca58f2850))
- SG-43602: Fix crash when adding a session file alongside media files ([#1294](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1294)) ([1f435df](https://github.com/AcademySoftwareFoundation/OpenRV/commit/1f435dffe29e44597da5a43399c989b5e3a23598))
- SG-43565: Source preview continues being generated after RV session is cleared ([#1295](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1295)) ([e8147ef](https://github.com/AcademySoftwareFoundation/OpenRV/commit/e8147ef66bf6b26dab7c7a9a7d8491f4d0f3ffc8))
- SG-43465: Source preview generation slows down with progressive loading ([#1289](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1289)) ([a5f07e7](https://github.com/AcademySoftwareFoundation/OpenRV/commit/a5f07e711064f48b0e4a8e5a1c79dbec7b97d70d))
- SG-43097: Thumbnail gen psutil refactor ([#1281](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1281)) ([5055423](https://github.com/AcademySoftwareFoundation/OpenRV/commit/50554231d46d93bd0ea662b19c6f1b36b2f79dc5))
- SG-43494: Disable thumbnail worker threads when preview is turned off ([#1283](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1283)) ([195a654](https://github.com/AcademySoftwareFoundation/OpenRV/commit/195a654769ed97d30ac55cbe968866f82e5f245c))
- Pin Rocky 9 CI to 9.7 to keep mesa-libOSMesa available ([#1285](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1285)) ([3e22a8f](https://github.com/AcademySoftwareFoundation/OpenRV/commit/3e22a8f51004ec696e3268381a6e5218498f8b96))
- SG-42890: Fix SEGV error when running RVIO commands on Rocky Linux 9 (part 2) ([#1279](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1279)) ([6219ece](https://github.com/AcademySoftwareFoundation/OpenRV/commit/6219ece6f3efc1e8f86fc6b2521c39e3fba6b3d8))
- SG-42890: Fix BADACCESS X error when running RVIO commands on Rocky Linux 9 ([#1249](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1249)) ([9fb070b](https://github.com/AcademySoftwareFoundation/OpenRV/commit/9fb070bc4ac64d8e570aa3170fe299130ae9dabb))
- SG-43392: Fix ACES 2 displaying black ([#1275](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1275)) ([88d5875](https://github.com/AcademySoftwareFoundation/OpenRV/commit/88d5875d24d5646d83cd9ab3b1a42363ca1026fc))
- small fix for the Python Import test that breaks Conan CI ([#1276](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1276)) ([baef0b1](https://github.com/AcademySoftwareFoundation/OpenRV/commit/baef0b1458f7451b7aacf5384118fe33291c925b))
- SG-43255: Fix latent issue with shaders (AMD) ([#1262](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1262)) ([163cd04](https://github.com/AcademySoftwareFoundation/OpenRV/commit/163cd0450397471c5129d54884ad5870156e9eb8))
- Set objectName on Diagnostics QDockWidget ([#1261](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1261)) ([f6f9d70](https://github.com/AcademySoftwareFoundation/OpenRV/commit/f6f9d7008d507c401f47fba874ce0e4e0b6fe17e))
- SG-42272: Fix subsample422_10bit tail pixel handling for non-multiple-of-6 widths ([#1141](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1141)) ([c5cbcc7](https://github.com/AcademySoftwareFoundation/OpenRV/commit/c5cbcc761fad1ec37ff1ab4c59f04566dc6c9046))

### Build System

- SG-42382 - Set FFMpeg 8 as default ([#1296](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1296)) ([4517984](https://github.com/AcademySoftwareFoundation/OpenRV/commit/4517984b09a0760e5bf200b3004f09252253e200))
- Add a python script to test imports during the build ([#1057](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1057)) ([4565a24](https://github.com/AcademySoftwareFoundation/OpenRV/commit/4565a246b2eae2a8d0433ccb930bc944b9dcf59d))
- Specify full path to the grep utility in rvcmd.sh ([#1093](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1093)) ([3bebfb6](https://github.com/AcademySoftwareFoundation/OpenRV/commit/3bebfb60af3231357a9a1ce7739294e10d1df937))
- Fix oiio build clashing with Homebrew on macOS ([#1264](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1264)) ([89457ca](https://github.com/AcademySoftwareFoundation/OpenRV/commit/89457ca728300b5091f04b8fa9b29accab87fc64))

### GitHub Actions

- Free up more space on the Ubuntu runner ([#1335](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1335)) ([d29a83b](https://github.com/AcademySoftwareFoundation/OpenRV/commit/d29a83bb389e4838234e01006e5e8c300127733d))
- point pinned Rocky 9.7 repos at the vault archive (conan.yml) ([#1309](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1309)) ([f0257e6](https://github.com/AcademySoftwareFoundation/OpenRV/commit/f0257e63820f7a3906c357289422fea2ea8c8a5b))
- point pinned Rocky 9.7 repos at the vault archive ([#1307](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1307)) ([2265094](https://github.com/AcademySoftwareFoundation/OpenRV/commit/22650944200e0ec00e41be5f8229f8a878c7af67))
- Add workflow to upload dependencies to ASWF Conan registry and use them in CI ([#1234](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1234)) ([31f9764](https://github.com/AcademySoftwareFoundation/OpenRV/commit/31f9764428a8cc4394f684a559ffa2fce1d6ee00))
## [3.2.0](https://github.com/AcademySoftwareFoundation/OpenRV/compare/v3.1.0...v3.2.0) - 2026-04-24

### Features

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

