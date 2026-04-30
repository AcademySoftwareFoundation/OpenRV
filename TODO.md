# Visto TODOs

- Migrate `src/lib/app/RvCommon/DisplayLink.cpp` to use modern `NSView`/`NSScreen` display links (`CVDisplayLink` is deprecated in macOS 15.0+). This will likely involve converting it to an Objective-C++ (`.mm`) file.
