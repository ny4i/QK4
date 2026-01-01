# Changelog

All notable changes to K4Controller will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Added
- TX function buttons on left panel now send CAT commands (left-click: primary, right-click: secondary):
  - TUNE (SW16) / TUNE LP (SW131)
  - XMIT (SW30) / TEST (SW132)
  - ATU (SW158) / ATU TUNE (SW40)
  - VOX (SW50) / QSK (SW134)
  - ANT (SW60) / RX ANT (SW70) / SUB ANT (SW157)
- QSK indicator next to VOX (grey when off, white when enabled)
- TEST indicator in center column (red, visible only when test mode active)
- ATU indicator below RIT/XIT (orange, visible only when ATU in AUTO mode)
- RadioState: TEST mode (TS), QSK flag (SD), ATU mode (AT) parsing and signals
- Right side panel buttons now send CAT commands (left-click: primary, right-click: secondary):
  - PRE (SW61) / ATTN (SW141), NB (SW32) / LEVEL (SW142), NR (SW62) / ADJ (SW143)
  - NTCH (SW31) / MANUAL (SW140), FIL (SW33) / APF (SW144), A/B (SW41) / SPLIT (SW145)
  - A→B (SW72) / B→A (SW147), SPOT (SW42) / AUTO (SW146), MODE (SW43) / ALT (SW148)
  - B SET (SW44) / PF1 (SW153), CLR (SW64) / PF2 (SW154), RIT (SW54) / PF3 (SW155), XIT (SW74) / PF4 (SW156)
- RadioState: Filter position tracking for VFO A (FP) and VFO B (FP$) with signals
- GPU-accelerated spectrum and waterfall display via OpenGL (main panadapter and mini-pans)
- Ref Level A/B support: A/B toggle selects which VFO's ref level value is displayed
- Ref Level +/- buttons target correct VFO based on A/B toggle selection
- Ref Level value shows faded grey text when in auto mode (visual indication)
- AUTO button toggles global auto-ref mode (affects both VFOs simultaneously)
- Display popup menu now sends CAT commands to K4 radio
- Button labels update dynamically based on radio state (PAN=A/B/A+B, AVG 1-20, etc.)
- Fixed Tune mode cycling through all 6 states (SLIDE1, SLIDE2, FIXED1, FIXED2, STATIC, TRACK)
- VFO cursor mode toggling (A+/A-/A AUTO/A HIDE)
- Waterfall color cycling (5 colors: Gray, Color, Teal, Blue, Sepia)
- Peak mode and Freeze toggles via right-click
- LCD/EXT target selection for display commands
- Sub RX main panadapter with full spectrum/waterfall display
- Three panadapter display modes: Main Only, Dual (A+B side-by-side), Sub Only
- Independent span control buttons (C/+/-) for both panadapters
- Click-to-tune and scroll-wheel tuning for VFO B panadapter
- SUB RX button cycles through panadapter display modes
- Mini-Pan for VFO B (Sub RX) with green spectrum line
- Mode-dependent Mini-Pan bandwidth for VFO B (CW=3kHz, Voice/Data=10kHz)
- AVERAGE control group in display popup with +/- buttons (cycles 1, 5, 10, 15, 20)
- DDC NB control group in display popup showing mode (OFF/ON/AUTO) and level (0-14)
- Passband indicator visibility synced with VFO cursor mode (A+/A-/B+/B-)
- Span A/B support: A/B toggle selects which VFO's span value is displayed
- Span +/- buttons target correct VFO based on A/B toggle (A+B targets both simultaneously)
- Right side panel expanded with 8 new buttons in two 2x2 grids:
  - PF row: B SET/PF 1, CLR/PF 2, RIT/PF 3, XIT/PF 4
  - Bottom row: FREQ ENT/SCAN, RATE/KHZ, LOCK A/LOCK B, SUB/DIVERSITY

### Changed
- Spectrum/waterfall rendering now uses GPU (OpenGL) instead of CPU (QPainter)
- Dramatically improved performance at high resolutions (CPU usage reduced from ~114% to ~15-25%)
- Smoother waterfall scrolling and spectrum updates
- VFO A passband indicator now uses blue color (matching physical K4 display)
- VFO B passband indicator uses green color for visual distinction
- Frequency markers use darker shades (blue for A, green for B) for clarity
- Mini-Pan now sends CAT commands (#MP / #MP$) to enable/disable streaming
- Mini-Pan spectrum line thickness reduced for cleaner display
- Mini-Pan colors match main panadapter scheme (blue for A, green for B)
- Passband display simplified: removed edge lines, shows only fill and center frequency marker
- Mini-Pan now shows center frequency marker line (darker shade of passband color)

### Fixed
- AUTO button now correctly highlights when radio is in auto-ref mode (was parsing #AR response format incorrectly)
- Mini-Pan VFO B now displays correct Sub RX spectrum (discovered undocumented RX byte at position 4)
- Dual panadapter mode now shows correct frequency alignment (spectrum was only showing partial range)
- PAN button face now updates correctly when radio changes panadapter mode (was stuck on "PAN = A")

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
