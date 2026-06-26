## [Unreleased]

### Features

- SG-35309: Set and save OCIO environment variable if missing and config is chosen ([#1263](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1263)) ([d43f3c3](https://github.com/AcademySoftwareFoundation/OpenRV/commit/d43f3c341d4330ac745d5e9569ab93a9787cb91d))
- Add annotation shapes, text, input smoothing, and stamp rendering ([#1301](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1301)) ([7f598d6](https://github.com/AcademySoftwareFoundation/OpenRV/commit/7f598d6a02c3d71e483ab3fb658dddc2de738359))
- SG-42880: Add fps options 60 5994 to Control FPS menu ([#1184](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1184)) ([19ac4db](https://github.com/AcademySoftwareFoundation/OpenRV/commit/19ac4db19c332a86289ddf8b0f6dee5d4b82d23f))
- Moving to major version 4 ([#1250](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1250)) ([bc20b43](https://github.com/AcademySoftwareFoundation/OpenRV/commit/bc20b43b62dc8c0c44a81c68db3d08fdcc700afb))
- SG-43132: Move main branch to CY2025 ([#1247](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1247)) ([fcafc94](https://github.com/AcademySoftwareFoundation/OpenRV/commit/fcafc94b18578a48654f3bb9c2d641cfd3e240f8))
- SG-42387 - Implement the infrastructure to allow OpenRV to find dependencies ([#1230](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1230)) ([b379c71](https://github.com/AcademySoftwareFoundation/OpenRV/commit/b379c71c91932df6171f9e384b3147dd72180c41))

### Bug Fixes

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

- point pinned Rocky 9.7 repos at the vault archive (conan.yml) ([#1309](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1309)) ([f0257e6](https://github.com/AcademySoftwareFoundation/OpenRV/commit/f0257e63820f7a3906c357289422fea2ea8c8a5b))
- point pinned Rocky 9.7 repos at the vault archive ([#1307](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1307)) ([2265094](https://github.com/AcademySoftwareFoundation/OpenRV/commit/22650944200e0ec00e41be5f8229f8a878c7af67))
- Add workflow to upload dependencies to ASWF Conan registry and use them in CI ([#1234](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1234)) ([31f9764](https://github.com/AcademySoftwareFoundation/OpenRV/commit/31f9764428a8cc4394f684a559ffa2fce1d6ee00))

