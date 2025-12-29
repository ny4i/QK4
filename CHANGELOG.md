# Changelog

All notable changes to K4Controller will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [0.1.0-alpha.108] - 2025-12-29

### Added
- Cross-platform CI/CD workflows (macOS 14 Apple Silicon, Windows 11 x64)
- Automated release workflow triggered by git tags (v*)
- Code formatting with clang-format (LLVM style, 4-space indent)
- Lint workflow for code style checking
- GitHub Actions badges in README

### Changed
- CMakeLists.txt now supports Windows builds via vcpkg

### Fixed
- macOS release now bundles all required Qt frameworks and dependencies
- Fixed QtDBus.framework missing from release bundle
- Fixed brotli libraries (libbrotlicommon, libbrotlidec, libbrotlienc) missing
- Fixed libdbus-1.3.dylib missing from release bundle
- Fixed library path rewrites for standalone app execution

### Removed
- Legacy spectrum.cpp/waterfall.cpp files (replaced by panadapter.cpp)
- CodeQL workflow (not available on free GitHub accounts)

---

## [0.1.0] - 2025-12-29

### Added
- Initial release
- Dual VFO display (A/B) with frequency, mode, S-meter
- Real-time spectrum analyzer with waterfall display
- Mini-pan widget in VFO area (toggleable with S-meter)
- Opus audio decoding and playback
- Full control panel: WPM, power, bandwidth, RF/SQL, TX functions
- Band selection popup menu
- Options dialog with radio info and KPA1500 settings
- KPOD USB device detection (HID communication pending)

### Technical
- Qt 6 / C++17
- TCP connection with K4 authentication (port 9205)
- CAT command parsing via RadioState model
- Signal/slot architecture for UI updates
