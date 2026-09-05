## [Unreleased]

### Features

- SG-43509: Add rebind and rebindRegex commands ([#1284](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1284)) ([3762724](https://github.com/AcademySoftwareFoundation/OpenRV/commit/37627242a789b8337d4d6566e147ba044e5f95b5))
- SG-44604: RVLinkLauncher - Save RV selection that was made in previous session ([#1385](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1385)) ([c79b7be](https://github.com/AcademySoftwareFoundation/OpenRV/commit/c79b7be3e2b7f6ccc929caf9e762ab4237025603))
- SG-43305: Add new annotation UI to expose new features ([#1359](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1359)) ([00867cf](https://github.com/AcademySoftwareFoundation/OpenRV/commit/00867cf4336ef3d05e910b3cd701a474c78d83dc))

### Bug Fixes

- SG-44508: Fix snapping shapes with stylus on hotkey ([#1400](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1400)) ([00a67aa](https://github.com/AcademySoftwareFoundation/OpenRV/commit/00a67aa4be502cff2c20b70937d76137705f8e17))
- SG-44910: defer HDPI resize workaround until after window is shown ([#1392](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1392)) ([09930b8](https://github.com/AcademySoftwareFoundation/OpenRV/commit/09930b81f8e905751f4b603b9ee8aefe6232b1b3))
- SG-44546: Fix annotations always enabled ([#1373](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1373)) ([31d17a7](https://github.com/AcademySoftwareFoundation/OpenRV/commit/31d17a7419ca1d4c39b61d2965f9f20b20e33759))
- SG-44428: Fix text cursor ([#1360](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1360)) ([794fd78](https://github.com/AcademySoftwareFoundation/OpenRV/commit/794fd783a2237a2e9a96f201bd0139cc496a253c))
- SG-43605: OTIO export failing when OCIO is active, and support reversed-order stack blending ([#1291](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1291)) ([f00641e](https://github.com/AcademySoftwareFoundation/OpenRV/commit/f00641e7e84f1bde1e04850bb95770bfe499cd4c))

### Performance Improvements

- SG-43585: Performance improvement of pyevaluate and pyexec ([#1375](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1375)) ([a8498fd](https://github.com/AcademySoftwareFoundation/OpenRV/commit/a8498fdd0c52912e367b0d23e0c67a4a9e7a657c))

### Build System

- SG-44926: add PySide6 fallback mappings for clang 18 through 22 ([#1383](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1383)) ([577eaf3](https://github.com/AcademySoftwareFoundation/OpenRV/commit/577eaf35221d86ad49fcd013b7ff253a69ac0e6d))
- Apple ProRes SDK dependency edge missing on Linux/macOS causing header race ([#1398](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1398)) ([92d8b07](https://github.com/AcademySoftwareFoundation/OpenRV/commit/92d8b0762e34c45f6e809c2b937d06fda11948c4))
- Fix Windows Debug OTIO pip build (cmake 4.4.1+ FindPython, python311_d.lib) ([#1376](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1376)) ([515073f](https://github.com/AcademySoftwareFoundation/OpenRV/commit/515073fae3833928e95c887760011fbde832831d))
- Fix stale FFmpeg libs not restaged after decoder config change ([#1388](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1388)) ([300c357](https://github.com/AcademySoftwareFoundation/OpenRV/commit/300c35762d54a341a837fa30274f7d743d16e502))

### GitHub Actions

- Uncomment if conditions in Conan workflow ([#1382](https://github.com/AcademySoftwareFoundation/OpenRV/pull/1382)) ([8dc11a8](https://github.com/AcademySoftwareFoundation/OpenRV/commit/8dc11a8a00bf77d9f2f94c4f5acb21f633e3dee3))

