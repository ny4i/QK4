# Changelog

All notable changes to K4Controller will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Added
- **TX Meter enhancements**: 500ms decay animation, S-meter gradient colors (Po/ALC/COMP/SWR), peak hold indicators
- **SCALE control**: Right-click REF LVL/SCALE in DISPLAY popup shows +/- controls (global, 10-150 range)
- **S-Meter peak indicator**: White vertical line showing signal peaks with 500ms decay time
- **Mode popup improvements**:
  - Left-click on MODE button opens popup (was right-click)
  - Clickable VFO A/B squares open mode popup targeting respective VFO
  - SSB button defaults to band-appropriate sideband (LSB below 10MHz, USB at/above)
- **VFO mode display**: Shows full mode with data sub-modes (AFSK, FSK, PSK, DATA)
- **TX Multifunction Meter Widget**: IC-7760 style horizontal bar meters displayed below VFO section
  - Po (Power): 0-100W with QRP mode support
  - ALC: 0-10 bars
  - COMP: 0-25 dB compression
  - SWR: 1.0-3.0+ ratio with infinity symbol
  - Id (PA Drain Current): 0-25A calculated from power and voltage
- **TLS/PSK Encrypted Connection**: Secure connection option using TLS v1.2 with Pre-Shared Key authentication
  - Port 9204 for encrypted connections (alternative to unencrypted port 9205)
  - "Use TLS (Encrypted)" checkbox in Server Manager dialog
  - PSK (Pre-Shared Key) field appears when TLS is enabled
  - Port auto-switches between 9204/9205 based on TLS selection
  - Supports multiple PSK cipher suites (ECDHE-PSK-AES256, DHE-PSK-AES256, etc.)
- **Self-Contained App Bundle**: macOS app now includes all dependencies for distribution
  - No Homebrew, OpenSSL, or Qt installation required for end users
  - Bundles OpenSSL, Opus, HIDAPI, and all Qt frameworks
  - Use `cmake --build build --target deploy` to create distributable bundle
  - Supports Developer ID code signing for Gatekeeper approval
- **VFO Tuning Rate Indicator**: Visual underline beneath frequency digit showing current tuning rate
  - White underline below the digit that will change when tuning (1Hz, 10Hz, 100Hz, 1kHz, 10kHz)
  - RATE button left-click (SW73) cycles through fine tuning rates
  - RATE button right-click (SW150/KHZ) jumps to 100Hz tuning rate
  - Indicator updates in real-time as VT/VT$ CAT commands are received
  - Supports both VFO A and VFO B with independent rate tracking
- **Spectrum Scale Matching**: Panadapter display now honors K4's #SCL scale setting (25-150)
  - Higher scale values compress display range (signals appear weaker)
  - Lower scale values expand display range (signals appear stronger)
  - dB scale labels update dynamically to match K4 display

### Changed
- **Right side panel layout**: BSET/CLR/RIT/XIT and FREQ/RATE/LOCK/SUB buttons moved down closer to PTT area
- **Spectrum grid rendering**: Grid now drawn behind spectrum fill (standard design pattern for better signal visibility)
- **Scale is global**: Removed incorrect per-VFO scale handling; #SCL applies to both panadapters

### Fixed
- **AVERAGE popup bug**: No longer sends CAT command when opening (was cycling value on click)
- **AVERAGE increment**: Now steps by 1 (was jumping by 5) with optimistic UI update
- **FREEZE toggle**: Button now updates label and can be toggled back (was stuck after first toggle)
- **Spectrum dBm calibration**: Signal levels and display range now match K4 display. Calibrated decompression offset (-146) and display range formula (minDb=refLevel, maxDb=refLevel+scale) for accurate visual match.
- **IF shift passband positioning**: K4 reports IF shift in decahertz (10 Hz units), not 0-99 index. Passband now correctly positions relative to dial marker when IF shift is adjusted.
- **Span control behavior**: Reversed +/- button polarity (+ now increases span, - decreases) to match intuitive behavior
- **Span increment sequence**: Now follows K4 radio pattern - 1kHz increments from 5-144kHz, 4kHz increments from 144-368kHz
- **Mini-pan passband rendering**: Fixed CW mode passband positioning and filter overlay display after QRhi/Metal migration

### Changed
- **S-Meter gradient**: Colors now transition earlier for better visibility at typical signal levels
- **S-Meter labels**: Compact format (1,3,5,7,9,20,40,60) for 200px width constraint
- **FREEZE button**: Label toggles between "FREEZE" and "FROZEN" (was "FREEZE"/"RUN")
- **MENU overlay**: Removed circle (○) button and "A" label from right side
- **Waterfall color cycling**: Removed #WFC command functionality from right-click
- **Volume slider colors**: Updated for visual consistency
  - MAIN slider: Cyan (#00BFFF) - matches Main RX theme
  - SUB slider: Green (#00FF00) - matches Sub RX indicators

### Added
- **Dual-channel audio mixing**: Main RX (VFO A) and Sub RX (VFO B) audio now independently controllable
  - MAIN volume slider controls left channel (Main RX)
  - SUB volume slider controls right channel (Sub RX)
  - Both sliders persist values between sessions
- **SUB/DIVERSITY buttons**: Right panel SUB button now functional
  - Left-click: Toggle Sub Receiver (SW83)
  - Right-click: Toggle Diversity mode (SW152)
- RadioState: DV (diversity) and SB (sub receiver) command parsing with signals
- **Fragment shader spectrum rendering**: Per-pixel spectrum fill using GPU fragment shader for smooth, professional appearance
  - Color intensity based on absolute screen position (higher signals = brighter)
  - Eliminates "flashing columns" artifact from previous triangle strip approach
  - Smooth gradient from dark green at noise floor to bright lime at strong signals
- **Metal/RHI panadapter overlays**: Grid, passband, and frequency marker rendering fixed for Qt RHI Metal backend
- Separate GPU buffer management for each overlay element to prevent rendering corruption
- CW mode passband/marker positioning now accounts for CW pitch offset
- VFO B panadapter connections for IF shift, CW pitch, and notch filter state
- Mode-dependent left panel controls: WPM/PTCH in CW modes, MIC/CMP in Voice/Data modes
  - Automatically switches when operating mode changes
  - Scroll wheel adjusts values and sends CAT commands (MG/CP)
- RadioState: MG (mic gain) and CP (compression) command parsing with signals
- B SET indicator in center column (green rounded rectangle with black text)
  - Appears when B SET is enabled (BS1), hides SPLIT label
  - Side panel BW/SHFT indicator changes from cyan to green when B SET active
  - Filter values (BW/SHFT/HI/LO) display VFO B values when B SET enabled
- RadioState: IS$ (Sub RX IF shift) command parsing
- RadioState: BS (B SET state) command parsing (BS0=off, BS1=on)
- Bottom menu bar popup widgets for Fn, MAIN RX, SUB RX, and TX buttons
  - Single-row popup with 7 placeholder buttons (same style as BAND popup)
  - Toggle behavior: second press closes popup, clicking different button switches popups
  - Gray bottom strip with triangle indicator pointing to trigger button
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
- Filter position indicators (FIL1/FIL2/FIL3) below A/B VFO squares, aligned with RIT/XIT display
- Feature menu bar for ATTN, NB LEVEL, NR ADJUST, MANUAL NOTCH with full CAT command support:
  - ATTENUATOR: toggle (RA/), +/- in 3dB steps (RA+/RA-)
  - NB LEVEL: toggle (NB/), level 0-15, filter cycling (NONE/NARROW/WIDE)
  - NR ADJUST: toggle (NR/), level 0-10
  - MANUAL NOTCH: toggle (NM/), frequency 150-5000Hz in 10Hz steps
- Feature menu bar displays live state from radio and updates in real-time
- Feature menu bar B SET support: when B SET is enabled, commands target Sub RX using $ suffix (RA$, NB$, NR$, NM$)
- RadioState: B SET (BS) command parsing with bSetEnabled() getter and bSetChanged() signal
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
- Panadapter UI overlays restored:
  - Reference level display in upper left corner (e.g., "REF: -10 dBm")
  - Gradient background (lighter gray center → darker gray edges)
  - Frequency indicators at bottom of waterfall (lower/center/upper frequencies)

### Changed
- Panadapter now displays raw K4 spectrum data (removed client-side EMA smoothing)
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
- **Metal overlay rendering**: Fixed passband showing as opaque triangles instead of translucent rectangles
- **Metal overlay rendering**: Fixed grid turning blue instead of proper gray color
- **Metal overlay height**: Passband and marker now correctly stop at spectrum area boundary (don't extend into waterfall)
- **VFO B passband alpha**: Fixed VFO B passband color from opaque green to translucent (25% alpha)
- **CW mode marker position**: Frequency marker now positioned at CW pitch offset (not at VFO frequency)
- **VFO B state sync**: Added missing connections for IF shift, CW pitch, and notch filter to VFO B panadapters
- Panadapter frequency alignment: Fixed ~25Hz offset caused by integer truncation in bin extraction (now rounds for symmetric extraction)
- RF Gain button faces now show "0" instead of "-0" when value is zero
- B SET filter targeting: BW/SHFT/HI/LO controls now correctly adjust VFO B filter when B SET is enabled
- AUTO button now correctly highlights when radio is in auto-ref mode (was parsing #AR response format incorrectly)
- Mini-Pan VFO B now displays correct Sub RX spectrum (discovered undocumented RX byte at position 4)
- Dual panadapter mode now shows correct frequency alignment (spectrum was only showing partial range)
- PAN button face now updates correctly when radio changes panadapter mode (was stuck on "PAN = A")
- DualControlButton click behavior: first click on inactive button now only activates it, subsequent clicks swap labels
- Left panel scroll wheel now adjusts values and sends CAT commands for all controls:
  - WPM/PTCH (KS, CW), PWR/DLY (PC, SD), BW/SHFT (BW, IS), M.RF/M.SQL (RG, SQ), S.RF/S.SQL (RG$, SQ$)
- Scroll wheel values now continue changing correctly (optimistic updates since radio doesn't echo commands)
- Button faces now update immediately after scroll wheel changes
- BW command format corrected (now sends value/10 as K4 expects)
- PC command format corrected (uses PCnnnL for QRP, PCnnnH for QRO mode)
- RF Gain command format corrected (RG-nn; and RG$-nn; with minus sign, range 0-60)
- RF Gain button faces now display with minus sign (e.g., "-0", "-30")
- Power button face shows decimals for QRP mode (≤10W: "9.9"), whole numbers for QRO (>10W: "50")
- Power transitions smoothly between QRP (0.1W steps) and QRO (1W steps) at 10W threshold

### Known Issues
- **Notch filter indicator**: Red vertical line renders entire grid red instead of single line (needs dedicated GPU buffers)

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
