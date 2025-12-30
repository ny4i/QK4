# Changelog

All notable changes to K4Controller will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Added
- Sub RX main panadapter with full spectrum/waterfall display
- Three panadapter display modes: Main Only, Dual (A+B side-by-side), Sub Only
- Independent span control buttons (C/+/-) for both panadapters
- Click-to-tune and scroll-wheel tuning for VFO B panadapter
- SUB RX button cycles through panadapter display modes
- Mini-Pan for VFO B (Sub RX) with green spectrum line
- Mode-dependent Mini-Pan bandwidth for VFO B (CW=3kHz, Voice/Data=10kHz)

### Changed
- VFO A passband indicator now uses blue color (matching physical K4 display)
- VFO B passband indicator uses green color for visual distinction
- Frequency markers use darker shades (blue for A, green for B) for clarity
- Mini-Pan now sends CAT commands (#MP / #MP$) to enable/disable streaming
- Mini-Pan spectrum line thickness reduced for cleaner display
- Mini-Pan colors match main panadapter scheme (blue for A, green for B)
- Passband display simplified: removed edge lines, shows only fill and center frequency marker
- Mini-Pan now shows center frequency marker line (darker shade of passband color)

### Fixed
- Mini-Pan VFO B now displays correct Sub RX spectrum (discovered undocumented RX byte at position 4)
- Dual panadapter mode now shows correct frequency alignment (spectrum was only showing partial range)

---

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
