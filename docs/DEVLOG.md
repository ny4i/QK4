# K4Controller Development Log

## February 8, 2026

### Feature: WheelAccumulator — Unified Scroll Handling

**Summary:** Replaced per-widget `angleDelta().y() > 0 ? 1 : -1` patterns across 16 wheel handlers with a shared `WheelAccumulator` class that properly accumulates fractional trackpad deltas into discrete steps.

**Problem:** Trackpad scrolling generated many small pixel-based events (angleDelta ~15°) that were each rounded to ±1 step, causing over-sensitivity. Notched mice (Logitech) send 120° per notch and worked fine.

**Solution:** `WheelAccumulator` accumulates deltas per key and emits steps only when a full 120° notch is reached. Supports per-key accumulators for widgets with modifier-based axes.

**Momentum tuning:** Panadapter frequency scrolling enables macOS trackpad momentum (`setFilterMomentum(false)`) for a flick-to-tune feel. All other widgets filter momentum by default.

**Files Added:** `src/ui/wheelaccumulator.h`, `src/ui/wheelaccumulator.cpp`
**Files Modified:** 16 widget files + `CMakeLists.txt`, `panadapter_rhi.h/.cpp`, `mainwindow.h/.cpp`

---

### Fix: PWR Display Shows "--" on Connect at 50W QRO

**Summary:** Power display stayed at "--" when connecting with radio set to exactly 50W QRO.

**Root Cause:** `m_rfPower` was initialized to `50.0` since the initial commit. When the RDY state dump returned `PC050H` (50W QRO), `handlePC()` saw `watts == m_rfPower && qrp == m_isQrpMode` — both matched defaults — so `changed` stayed `false` and `rfPowerChanged` was never emitted. The display, initialized to "--", was never updated.

**Why intermittent:** Only triggers at exactly 50W QRO. Any other power level or QRP mode has at least one field that differs from the default, so the signal fires normally. Testers running at different power levels never saw it.

**Fix:** Changed `m_rfPower = 50.0` to `m_rfPower = -1.0` (sentinel), matching the pattern already used by `m_micGain`, `m_compression`, `m_rfGain`, `m_squelchLevel`, and `m_cwPitch`. The sentinel was introduced in commit `76e12a1` for neighboring fields but `m_rfPower` was overlooked.

**Verified via direct K4 CAT testing:** Connected to radio via `nc 192.168.1.10 9200` and confirmed `RDY;` dump always returns `PC` in K4 extended format (`PCnnnr;`) regardless of K41/AI ordering.

**File Modified:** `src/models/radiostate.h`

---

## February 6, 2026

### Feature: Honor K4 "Mouse L/R Button QSY" Menu Setting

**Summary:** Panadapter click/drag handlers now respect the K4's "Mouse L/R Button QSY" menu setting instead of using hardcoded VFO mapping.

**Modes:**
- **Left Only (value 0):** Only left-click/left-drag QSYs. Right-click/right-drag are ignored on both panadapters.
- **L=A R=B (value 1):** Left button always tunes VFO A, right button always tunes VFO B, regardless of which panadapter is clicked.

**Implementation:**
- Discovers menu item ID dynamically via MEDF on connect (`menuItemAdded` signal)
- Tracks live changes via `menuValueChanged` signal (updates in real-time if changed on radio or menu overlay)
- Modified 6 panadapter handlers: Pan A right-click/drag, Pan B left-click/drag, Pan B right-click/drag

**Files Modified:** `mainwindow.h`, `mainwindow.cpp`

---

## January 30, 2026

### Refactor: Comprehensive Code Quality Review

**Summary:** Major refactoring across TCP/Protocol, RadioState, Audio, and Spectrum/Waterfall sections. Net result: +358 lines but massive maintainability improvements.

**Key Achievement:** RadioState `parseCATCommand()` refactored from ~1500-line if-else chain to 21-line registry dispatch.

---

#### 1. Protocol Section (protocol.h, protocol.cpp, tcpclient.cpp)

**Changes:**
- ODR fix: `static const` → `inline const` for QByteArray markers
- Added buffer overflow protection (MAX_BUFFER_SIZE check)
- Added packet offset namespaces: `PanPacket`, `MiniPanPacket`, `AudioPacket`
- Added `K4Protocol::Commands` namespace for magic strings (READY, PING, DISCONNECT)

---

#### 2. RadioState Command Handler Registry (radiostate.cpp)

**Before:** ~1500-line if-else chain in `parseCATCommand()`
```cpp
if (cmd.startsWith("FA")) { ... }
else if (cmd.startsWith("FB")) { ... }
else if (cmd.startsWith("MD$")) { ... }
// ... 80+ more conditions
```

**After:** 21-line registry dispatch
```cpp
for (const auto &entry : m_commandHandlers) {
    if (cmd.startsWith(entry.prefix)) {
        entry.handler(cmd);
        emit stateUpdated();
        return;
    }
}
```

**New Structure:**
- `registerCommandHandlers()` - Registers ~80 handlers sorted by prefix length
- Individual handlers: `handleFA()`, `handleFB()`, `handleMD()`, `handleMDSub()`, etc.
- Prefix matching sorted longest-first (ensures `MD$` matches before `MD`)

**File Stats:** 3805 → 2285 lines (-1520 lines, 40% reduction)

---

#### 3. Audio Section

**OpusDecoder (opusdecoder.cpp/h):**
- Extracted duplicated mono mixing loop to `mixStereoToMono()` helper
- Added `NORMALIZE_16BIT`, `NORMALIZE_32BIT` constants
- Stats: 275 → 252 lines (-23)

**OpusEncoder (opusencoder.cpp/h):**
- Added `qWarning()` for encoder creation failures
- Added input size validation in `encode()`
- Added `MAX_PACKET_SIZE` constant
- Stats: 71 → 82 lines (+11 for safety)

**AudioEngine (audioengine.cpp/h):**
- Added `OUTPUT_BUFFER_SIZE`, `INPUT_BUFFER_SIZE`, `MIC_GAIN_SCALE` constants
- Stats: 440 → 447 lines (+7)

---

#### 4. Spectrum/Waterfall Section

**New File:** `src/dsp/rhi_utils.h`
- `RhiUtils::loadShader()` - Shared shader loading function
- `RhiUtils::K4_DBM_OFFSET` - Calibration constant (146.0f)

**panadapter_rhi.cpp/h:**
- Replaced inline shader loading lambda with `RhiUtils::loadShader()`
- Added `K4_DBM_OFFSET` constant for dBm calibration
- Stats: 2062 → 2058 lines (-4)

**minipan_rhi.cpp:**
- Replaced inline shader loading lambda with `RhiUtils::loadShader()`
- Stats: 986 → 979 lines (-7)

---

#### Overall Stats

| Section | Before | After | Change |
|---------|--------|-------|--------|
| Protocol | ~591 | 620 | +29 |
| RadioState | 4886 | 3366 | -1520 |
| Audio | 986 | 981 | -5 |
| Spectrum | 3210 | 3226 | +16 |
| **Total** | **~9673** | **8193** | **-1480** |

**Git Stats:** 14 files modified, 1 new file, 2324 insertions, 1993 deletions

---

## January 9, 2026

### Refactor: Centralized K4Styles System

**Summary:** Created `K4Styles` namespace to consolidate duplicate button/popup stylesheets across 6 widgets, reducing ~300 lines of duplicate CSS code.

**New File:** `src/ui/k4styles.h` and `src/ui/k4styles.cpp`

**Functions:**
- `K4Styles::popupButtonNormal()` - Standard dark gradient button for popups
- `K4Styles::popupButtonSelected()` - Light/white button for selected state
- `K4Styles::menuBarButton()` - Bottom menu bar buttons (with padding)
- `K4Styles::menuBarButtonActive()` - Active/inverted state for menu buttons
- `K4Styles::menuBarButtonSmall()` - Compact +/- buttons in FeatureMenuBar

**Constants Namespaces:**
- `K4Styles::Colors::*` - Button gradients, borders, text colors
- `K4Styles::Dimensions::*` - Border width, radius, shadow constants

**Files Modified:**
- `src/ui/bandpopupwidget.cpp/.h` - Removed local style functions
- `src/ui/modepopupwidget.cpp/.h` - Removed local style functions
- `src/ui/buttonrowpopup.cpp/.h` - Removed local style function
- `src/ui/featuremenubar.cpp/.h` - Removed local style functions
- `src/ui/bottommenubar.cpp/.h` - Removed local style functions
- `CMakeLists.txt` - Added k4styles.cpp/.h

---

## January 8, 2026

### Feature: HD Visual Polish for Popups

**Summary:** Added professional drop shadows, 2px borders, and consistent border radius to all popup widgets for HD-quality appearance on Retina/4K displays.

**Problem:** Popups looked flat with no visual separation from background. 1px borders were invisible on high-DPI displays. Border radius varied inconsistently (4-8px).

**Changes:**

1. **Drop Shadows (Phase 1)**: Added 8-layer soft shadow to all popups
   - Blur radius: 16px
   - Offset: 2px right, 4px down
   - Shadow margin: 20px around content
   - Manual drawing in `paintEvent()` for custom-shaped popups

2. **2px Borders (Phase 2)**: Changed all `border: 1px solid` to `border: 2px solid`
   - CSS stylesheets updated across all widgets
   - `QPen` thickness changed from 1 to 2 for painted borders

3. **Consistent Border Radius (Phase 3)**: Standardized to 6px for buttons
   - Changed `border-radius: 4px` and `border-radius: 5px` to `border-radius: 6px`
   - Popup backgrounds use 8px radius

**Implementation Details:**
- Used `WA_TranslucentBackground` for manual shadow painting
- Widget sizing includes `ShadowMargin` on all sides
- Triangle pointer positioning recalculated relative to content area (not widget bounds)

**Files Modified:**
- `src/ui/bandpopupwidget.cpp` - Shadow, margins, positioning, styles
- `src/ui/modepopupwidget.cpp` - Shadow, margins, positioning, styles
- `src/ui/fnpopupwidget.cpp` - Shadow, margins, positioning, styles
- `src/ui/buttonrowpopup.cpp` - Shadow, margins, positioning, styles
- `src/ui/displaypopupwidget.cpp` - Shadow, margins, positioning, styles
- `src/ui/featuremenubar.cpp` - Shadow, margins, positioning, styles
- `src/ui/bottommenubar.cpp` - 2px borders, 6px radius
- `src/ui/sidecontrolpanel.cpp` - 2px borders, 6px radius
- `src/ui/rightsidepanel.cpp` - 2px borders, 6px radius
- `src/ui/dualcontrolbutton.cpp` - QPen 2px border

**Documentation Updated:**
- `STATUS.md` - Added Visual Polish section
- `CONVENTIONS.md` - Added Popup & Button Styling section

---

## January 5, 2026

### Feature: TX Meter Widget Enhancements

**Summary:** Enhanced TX multifunction meters with decay animation, S-meter gradient colors, and peak hold indicators.

**Changes:**
1. **500ms decay rate**: Meter bars rise instantly, decay smoothly over ~500ms
2. **S-meter gradient colors**: Po, ALC, COMP, SWR now use green→yellow→orange→red gradient (matching S-meter style)
3. **Id stays red**: PA drain current meter retains original red/maroon color scheme
4. **Peak hold indicators**: White vertical line at peak position with slower decay (1 second)

**Implementation:**
- Separate target values (instant) and display values (with decay)
- Timer fires every 50ms, decay rate 0.1 per tick = ~500ms full decay
- Peak decay rate 0.05 per tick = ~1 second decay
- MeterType enum to select Gradient vs Red color scheme

**Files Modified:**
- `src/ui/txmeterwidget.h` - Added decay timer, target/display/peak values, MeterType enum
- `src/ui/txmeterwidget.cpp` - Decay logic, S-meter gradient, peak indicators

---

### UI: Right Side Panel Button Layout

**Summary:** Rearranged button layout to move lower button groups closer to PTT area.

**Changes:**
- Added stretch after main 5×2 grid to push remaining buttons down
- BSET/CLR/RIT/XIT cluster now positioned lower
- FREQ/RATE/LOCK/SUB cluster positioned at bottom above PTT
- 33px spacing between the two lower clusters

**Files Modified:**
- `src/ui/rightsidepanel.cpp` - Layout restructuring

---

### DSP: Spectrum Grid Drawn Behind Fill

**Summary:** Changed render order so grid is drawn behind spectrum fill (standard design pattern).

**Rationale:**
- Signal visibility: spectrum data is primary information
- Visual hierarchy: data in front, reference lines in back
- Cleaner appearance: signals don't get "cut up" by grid lines

**Implementation:**
- Grid now drawn after waterfall, before spectrum fill
- Spectrum shader uses `discard` for pixels above signal level
- Grid visible in empty space above signal peaks

**Files Modified:**
- `src/dsp/panadapter_rhi.cpp` - Moved grid rendering before spectrum fill

---

### Feature: SCALE Control in DISPLAY Popup

**Summary:** Added SCALE control page to DISPLAY popup for adjusting panadapter display range.

**Changes:**
- Right-click REF LVL/SCALE button now shows scale +/- controls
- Scale is GLOBAL (applies to both VFO A and B panadapters)
- Scale range: 10-150 (default 75)
- Increment/decrement by 1, sends `#SCL<value>;` command

**Implementation:**
- Removed incorrect `#SCL$` parsing (scale is global, no $ variant)
- `scaleChanged` signal updates both panadapters
- Scale control page with ControlGroupWidget

**Files Modified:**
- `src/ui/displaypopupwidget.h/.cpp` - Added scale control page and signals
- `src/models/radiostate.h/.cpp` - Removed scaleB, scaleBChanged (scale is global)
- `src/mainwindow.cpp` - Scale applies to both panadapters, +/- handlers

---

### Feature: S-Meter Peak Indicator with Decay

**Summary:** Added peak hold indicator to SMeterWidget showing signal peaks with approximately 500ms decay time.

**Implementation:**
- `m_peakValue` tracks highest signal level
- Timer fires every 50ms, decaying peak by 0.1 units per tick
- White vertical line (2px wide) drawn at peak position
- Peak automatically follows current value when signal rises

**Files Modified:**
- `src/ui/smeterwidget.h` - Added QTimer, peak value member, decayPeak slot
- `src/ui/smeterwidget.cpp` - Timer setup, peak tracking, decay logic, peak line rendering

---

### UI: S-Meter Gradient and Size Optimization

**Changes:**
1. **Width constraint fix**: Reduced min/max from 280-380px to 180-200px to fit within VFO stacked widget's 200px limit
2. **Gradient transitions earlier**: Colors now shift sooner for better visibility at typical S5-S9 signals
   - Green → Bright Green → Yellow-Green → Yellow → Orange → Red-Orange → Red
3. **Compact labels**: "1,3,5,7,9,20,40,60" (removed "S" and "+" prefixes), 6pt font

**Files Modified:**
- `src/ui/smeterwidget.cpp` - Width constraints, gradient stops, label format

---

### UI: Mode Popup Improvements

**Changes:**
1. **Trigger changed**: Left-click on MODE button opens popup (was right-click)
2. **VFO squares clickable**: Blue A and green B squares in center column now open mode popup targeting respective VFO
3. **Band-aware SSB**: SSB button defaults to band-appropriate sideband (LSB below 10MHz, USB at/above 10MHz)
4. **Full mode display**: VFO widget shows mode with data sub-modes (AFSK, FSK, PSK, DATA)

**Files Modified:**
- `src/ui/modepopupwidget.h/.cpp` - Added setFrequency(), bandDefaultSideband()
- `src/models/radiostate.h/.cpp` - Added modeStringFull(), modeStringFullB(), dataSubModeToString()
- `src/mainwindow.cpp` - Event filters for VFO squares, mode label updates

---

### Fix: AVERAGE Popup Sending Command on Open

**Issue:** Clicking AVERAGE in DISPLAY popup sent #AVG command immediately before user changed any value.

**Root Cause:** `onMenuItemClicked()` had two switch statements - one to show control page (correct) and another to emit CAT commands (bug). The AveragePeak case in the second switch cycled and sent the value.

**Fix:** Removed `case AveragePeak:` from the command-sending switch statement.

**Files Modified:**
- `src/ui/displaypopupwidget.cpp` - Removed lines 845-862

---

### Fix: AVERAGE Increment/Decrement Step Size

**Issue:** +/- buttons stepped by 5 instead of 1.

**Fix:** Changed from step array (1,5,10,15,20) to simple `qMin(current + 1, 20)` / `qMax(current - 1, 1)`.

**Files Modified:**
- `src/mainwindow.cpp` - Updated averagingIncrementRequested/averagingDecrementRequested handlers

---

### Fix: AVERAGE/FREEZE Optimistic Updates

**Issue:** Clicking +/- sent command but UI didn't update until radio echoed value.

**Fix:** Added optimistic update pattern - update local state immediately, then send CAT command.

**Implementation:**
- Added `RadioState::setAveraging(int)` optimistic setter
- Added local `m_freeze` update before sending #FRZ command
- Call `updateMenuButtonLabels()` after local state change

**Files Modified:**
- `src/models/radiostate.h/.cpp` - Added setAveraging() method
- `src/mainwindow.cpp` - Call setAveraging() before sending command
- `src/ui/displaypopupwidget.cpp` - Update m_freeze locally, call updateMenuButtonLabels()

---

### UI: FREEZE Button Label Toggle

**Change:** Button label now toggles between "FREEZE" and "FROZEN" (was "FREEZE"/"RUN").

**Files Modified:**
- `src/ui/displaypopupwidget.cpp` - Updated setAlternateText() logic

---

### UI: MENU Overlay Cleanup

**Change:** Removed circle (○) button and "A" label from right side of MENU popup.

**Files Modified:**
- `src/ui/menuoverlay.h` - Removed m_selectBtn, m_aLabel members
- `src/ui/menuoverlay.cpp` - Removed Row 2 layout with circle button and A label

---

### Removed: Waterfall Color Cycling (#WFC)

**Change:** Removed #WFC command functionality from right-click on NbWtrClrs menu item.

**Files Modified:**
- `src/ui/displaypopupwidget.cpp` - Removed case NbWtrClrs from onMenuItemRightClicked()

---

## January 4, 2026

### Feature: TX Multifunction Meter Widget (IC-7760 Style)

**Summary:** Added a multifunction TX meter widget displaying 5 horizontal bar meters below the VFO section, inspired by the IC-7760 design.

**Meters Displayed:**
- **Po (Power):** 0-100W (QRO) or 0-10W (QRP) - from TM command
- **ALC:** 0-10 bars - from TM command
- **COMP:** 0-25 dB compression - from TM command
- **SWR:** 1.0-3.0+ ratio - from TM command
- **Id (PA Drain Current):** 0-25A - calculated from power/voltage

**PA Current Calculation:**
The K4's SIFP command provides DC input current (IS:), not PA drain current. The Id meter value is calculated:
```cpp
Id = ForwardPower / (SupplyVoltage × 0.34)
```
- Efficiency factor of 0.34 (34%) derived from measurements: 80W @ 17A @ 13.8V
- Example: 100W / (13.8V × 0.34) = 21.3A ✓

**Visual Design:**
- Dark background (#0d0d0d) with dark red meter bars (#8B0000)
- Label boxes on left (Po, ALC, COMP, SWR, Id)
- Scale labels below each meter bar
- Fixed height 95px, width 200-380px

**Key Implementation Detail:**
- `setAttribute(Qt::WA_OpaquePaintEvent)` required for proper custom painting

**Files Created:**
- `src/ui/txmeterwidget.h` - Widget class declaration
- `src/ui/txmeterwidget.cpp` - Painting and meter logic

**Files Modified:**
- `src/mainwindow.h` - Added TxMeterWidget members
- `src/mainwindow.cpp` - Created widgets, connected TM signal, PA current calculation
- `CMakeLists.txt` - Added new source files

---

### UI: Volume Slider Color Update

**Change:** Updated MAIN and SUB volume slider colors for consistency:
- **MAIN slider:** Cyan (#00BFFF) - matches VFO B color scheme
- **SUB slider:** Green (#00FF00) - matches Sub RX indicators

**Files Modified:**
- `src/ui/sidecontrolpanel.cpp` - Updated slider and label stylesheets

---

### Fix: Spectrum dBm Calibration and Scale Support

**Issue:** Spectrum display showed signals at incorrect dBm levels compared to the K4's physical display, and the display range wasn't tracking the K4's ref level and scale settings.

**Calibration Process:**
1. Disconnected antenna to compare noise floor readings
2. Compared peak signal levels between K4 and app
3. Iteratively adjusted offset until visual match achieved

**Final Implementation:**

1. **Decompression formula** (`decompressBins()`):
   ```cpp
   out[i] = static_cast<quint8>(bins[i]) - 146.0f;
   ```
   - Offset of -146 calibrated by comparing peak signals with K4 display
   - Original -160 offset was ~14 dB too low

2. **Display range** (`updateDbRangeFromRefAndScale()`):
   ```cpp
   m_minDb = refLevel;
   m_maxDb = refLevel + scale;
   ```
   - RefLevel (#REF) is the bottom of the display
   - Scale (#SCL, 25-150) is the total dB range shown
   - Display dynamically tracks K4's ref and scale settings

3. **RadioState** added `#SCL`/`#SCL$` parsing with signals

4. **MainWindow** connects scale signals to both panadapters

**Files Modified:**
- `src/models/radiostate.h/.cpp` - Added scale state, signals, getters, parsing
- `src/dsp/panadapter_rhi.h/.cpp` - Calibrated decompression, simplified dB range calc
- `src/mainwindow.cpp` - Connected scale signals

---

### Feature: TLS/PSK Encrypted Connection Support

**Summary:** Added support for secure TLS v1.2 connections using Pre-Shared Key (PSK) authentication on port 9204, as an alternative to the existing unencrypted port 9205.

**K4 Protocol:**
- Port 9204: TCP/TLS v1.2 with PSK encryption
- Port 9205: Unencrypted with SHA-384 password authentication
- Supported ciphers: ECDHE-PSK-AES256-CBC-SHA384, ECDHE-PSK-CHACHA20-POLY1305, DHE-PSK-AES256-GCM-SHA384, and others

**Implementation:**

1. **RadioEntry struct** (`radiosettings.h`):
   - Added `useTls` boolean field (default: false)
   - Added `psk` QString field for Pre-Shared Key storage

2. **RadioSettings** (`radiosettings.cpp`):
   - Persists `useTls` and `psk` fields to QSettings

3. **TcpClient** (`tcpclient.h/.cpp`):
   - Changed `QTcpSocket` to `QSslSocket` for unified handling
   - Extended `connectToHost()` with `useTls` and `psk` parameters
   - Added `onSslErrors()` handler (ignores cert errors for PSK mode)
   - Added `onPreSharedKeyAuthenticationRequired()` handler for PSK auth
   - Configured TLS v1.2 with `VerifyNone` peer mode (PSK doesn't use certs)
   - Uses `connectToHostEncrypted()` for TLS, regular `connectToHost()` for plain TCP

4. **RadioManagerDialog** (`radiomanagerdialog.h/.cpp`):
   - Added "Use TLS (Encrypted)" checkbox
   - Added PSK field (hidden until TLS is checked)
   - Auto-updates port when TLS checkbox is toggled (9204 ↔ 9205)
   - Persists TLS settings per radio entry

5. **Protocol constants** (`protocol.h`):
   - Added `K4Protocol::TLS_PORT = 9204`

**Files Modified:**
- `src/network/protocol.h` - Added TLS_PORT constant
- `src/network/tcpclient.h` - QSslSocket, new parameters, PSK slot
- `src/network/tcpclient.cpp` - TLS connection logic, PSK auth handler
- `src/settings/radiosettings.h` - RadioEntry useTls/psk fields
- `src/settings/radiosettings.cpp` - Persist TLS settings
- `src/ui/radiomanagerdialog.h` - TLS checkbox, PSK field members
- `src/ui/radiomanagerdialog.cpp` - TLS UI and logic
- `src/mainwindow.cpp` - Pass TLS params to connectToHost

**macOS OpenSSL Discovery:**

Initial testing showed "No PSK ciphers available" because Qt on macOS defaults to Secure Transport, which lacks PSK support. However, the Qt Online Installer includes the OpenSSL TLS backend plugin (`libqopensslbackend.dylib`), it just can't find Homebrew's OpenSSL libraries at runtime.

**Solution:** Set `DYLD_LIBRARY_PATH` to include Homebrew's OpenSSL before Qt loads the TLS backend:

```cpp
// In main.cpp, before QApplication
const char *opensslPath = "/opt/homebrew/opt/openssl@3/lib";
if (QFileInfo::exists(opensslPath)) {
    QString newPath = QString("%1:%2").arg(opensslPath, qgetenv("DYLD_LIBRARY_PATH"));
    qputenv("DYLD_LIBRARY_PATH", newPath.toLocal8Bit());
}
```

This works because Qt's OpenSSL plugin uses `dlopen()` which respects `DYLD_LIBRARY_PATH` at call time.

**Requirements:**
- macOS: `brew install openssl@3`
- Windows: OpenSSL should work out of the box
- Qt Online Installer build (includes OpenSSL TLS backend plugin)

**Testing:**
```bash
# Verify TLS connection with openssl (PSK must be hex-encoded)
echo -n "yourpassword" | xxd -p  # Convert to hex
openssl s_client -tls1_2 -psk <hex> -connect 192.168.x.x:9204
```

---

### Feature: Self-Contained App Bundle for Distribution

**Summary:** The macOS app bundle now includes all dependencies, allowing distribution without requiring users to install Homebrew, OpenSSL, or Qt.

**Bundled Dependencies:**
- OpenSSL 3.x (`libssl.3.dylib`, `libcrypto.3.dylib`) - for TLS/PSK encryption
- Opus codec (`libopus.dylib`) - for audio encoding/decoding
- HIDAPI (`libhidapi.dylib`) - for USB device communication
- Qt frameworks and plugins (via macdeployqt)
- Qt TLS plugins including `libqopensslbackend.dylib`

**Implementation:**

1. **Info.plist.in** (`resources/Info.plist.in`):
   - macOS app bundle metadata template
   - Bundle identifier: `com.ai5qk.k4controller`
   - Minimum macOS version: 14.0

2. **CMakeLists.txt** - Added `deploy` target:
   - Runs `macdeployqt` to bundle Qt frameworks
   - Copies OpenSSL, Opus, and HIDAPI libraries to `Contents/Frameworks/`
   - Uses `install_name_tool` to fix library paths with `@executable_path/../Frameworks/`
   - Resolves Homebrew symlinks to find actual library paths
   - Re-signs the bundle after modifications (ad-hoc or Developer ID)

3. **main.cpp** - Updated OpenSSL discovery:
   - Checks bundled `Frameworks/` directory first
   - Falls back to Homebrew paths if not bundled
   - Sets `DYLD_LIBRARY_PATH` for Qt's dynamic OpenSSL loading

**Build Commands:**
```bash
cmake -B build -DCMAKE_PREFIX_PATH="$(brew --prefix qt@6)"
cmake --build build
cmake --build build --target deploy  # Creates distributable bundle
```

**Code Signing:**
The deploy target re-signs the app after modifying libraries. For Developer ID distribution:
```bash
export CODESIGN_IDENTITY="Developer ID Application: Your Name (TEAMID)"
cmake --build build --target deploy
```

**Files Modified:**
- `CMakeLists.txt` - MACOSX_BUNDLE config, deploy target with library bundling
- `resources/Info.plist.in` - New macOS plist template
- `src/main.cpp` - Check bundled Frameworks first for OpenSSL

**Total Bundle Size:** ~100MB (includes Qt frameworks, FFmpeg, OpenSSL, Opus, HIDAPI)

---

## January 3, 2026

### Fix: VFO B Auto Notch (NTCH) Indicator

**Summary:** Fixed VFO B's NTCH indicator not updating when toggling notch with B SET enabled.

**Root Cause:** The K4 sends `NA$` (Auto Notch Sub RX) commands when toggling notch via SW31 with B SET on, but we weren't parsing `NA$` commands. The VFO B setNotch call was also hardcoded to `false` for auto notch.

**Implementation:**
1. Added `m_autoNotchEnabledB` state variable and getter to RadioState
2. Added `NA$` command parsing that emits `notchBChanged` signal
3. Updated VFO B's setNotch call to use `autoNotchEnabledB()` instead of hardcoded `false`

**CAT Commands:**
- Main RX: `NA0;` / `NA1;` (auto notch off/on)
- Sub RX: `NA$0;` / `NA$1;` (auto notch off/on)

**Files Modified:**
- `src/models/radiostate.h` - Added `autoNotchEnabledB()` getter and `m_autoNotchEnabledB` member
- `src/models/radiostate.cpp` - Added NA$ parsing
- `src/mainwindow.cpp` - Updated VFO B notch indicator to include auto notch state

---

### Fix: NB/NR Level Control Increments

**Summary:** Fixed Noise Blanker (NB) and Noise Reduction (NR) level controls that were skipping values and not incrementing properly.

**Root Causes:**
1. **NB Parsing**: Required 4 characters (level+enabled+filter) but K4 responds with only 3 characters (level+enabled, no filter field). Parsing silently failed.
2. **No Echo**: K4 doesn't echo NB/NR set commands back, so RadioState never updated. Subsequent increments read stale values.

**Fixes Applied:**
1. Made NB filter field optional in parsing - accept both 3-char and 4-char formats
2. Added optimistic state setters: `setNoiseBlankerLevel()`, `setNoiseBlankerLevelB()`, `setNoiseReductionLevel()`, `setNoiseReductionLevelB()`
3. Increment/decrement handlers now update RadioState immediately after sending CAT command
4. NB+/- handlers now increment/decrement level properly

**Files Modified:**
- `src/models/radiostate.h` - Added 4 optimistic setter declarations
- `src/models/radiostate.cpp` - Fixed NB/NB$ parsing to handle 3-char format, added setter implementations
- `src/mainwindow.cpp` - Increment/decrement handlers now call optimistic setters

---

### Fix: Manual Notch Filter Turns Grid Red

**Summary:** Fixed issue where enabling manual notch filter would turn the spectrum grid red.

**Root Cause:** The notch marker and grid both shared `m_overlayUniformBuffer`. RHI batches GPU commands, so both the grid and notch writes to the same buffer occurred before draws executed - resulting in all overlay draws using the notch's red color uniforms.

**Fix:** Created dedicated GPU buffers for the notch filter marker:
- `m_notchVbo` - Vertex buffer for notch line
- `m_notchUniformBuffer` - Uniform buffer for notch color
- `m_notchSrb` - Shader resource bindings for notch

This follows the same pattern used for passband and frequency marker overlays.

**Files Modified:**
- `src/dsp/panadapter_rhi.h` - Added notch buffer and SRB declarations
- `src/dsp/panadapter_rhi.cpp` - Created notch buffers, SRB, and updated notch drawing to use dedicated resources

---

### Fix: Manual Notch A/B Separation

**Summary:** Fixed manual notch to properly track separate state for VFO A (Main RX) and VFO B (Sub RX).

**Issues Fixed:**
1. VFO A and B were sharing the same notch state
2. B SET mode notch adjustments affected VFO A
3. VFO B panadapter/mini-pan displayed VFO A's notch marker

**Implementation:**
1. Added `m_manualNotchEnabledB` and `m_manualNotchPitchB` state to RadioState
2. Added NM$ command parsing for Sub RX notch
3. Added `notchBChanged` signal
4. Added `setManualNotchPitch()` and `setManualNotchPitchB()` optimistic setters
5. Updated all notch handlers to use correct A/B state based on B SET
6. Connected VFO B panadapter/mini-pan to `notchBChanged` signal

**CAT Commands:**
- Main RX: `NMnnnnm;` (nnnn=pitch 150-5000, m=on/off)
- Sub RX: `NM$nnnnm;`

**Files Modified:**
- `src/models/radiostate.h` - Added notch B getters, signals, setters, member variables
- `src/models/radiostate.cpp` - Added NM$ parsing, notch pitch setters
- `src/mainwindow.cpp` - Updated all notch handlers for A/B, connected notchBChanged

---

### Fix: NB Toggle vs Filter Button Separation

**Summary:** Fixed FeatureMenuBar NB controls so toggle button toggles NB on/off and extra button cycles filter.

**The Bug:** Toggle button was incorrectly cycling the filter (NONE→NARROW→WIDE) instead of toggling NB on/off. This meant the ON/OFF toggle and the NONE/NARROW/WIDE filter button were both doing the same thing.

**FeatureMenuBar NB Controls:**
| Button | Signal | Correct Action |
|--------|--------|----------------|
| Toggle (ON/OFF) | `toggleRequested()` | Toggle NB on/off with `NB/;` |
| Filter (NONE/NARROW/WIDE) | `extraButtonClicked()` | Cycle filter 0→1→2→0 |
| +/- | `increment/decrementRequested()` | Change level 0-15 |

**Fix:**
1. Restored toggle handler to send `NB/;` or `NB$/;` to toggle on/off
2. Added optimistic state update to extraButtonClicked handler
3. Added `setNoiseBlankerFilter()` and `setNoiseBlankerFilterB()` optimistic setters

**Files Modified:**
- `src/models/radiostate.h` - Added NB filter setter declarations
- `src/models/radiostate.cpp` - Added NB filter setter implementations
- `src/mainwindow.cpp` - Fixed toggle handler, added optimistic update to extra button handler

---

### Feature: VFO Tuning Rate Indicator

**Summary:** Added visual indicator showing current tuning rate (step size) for both VFO A and VFO B. A small underline appears beneath the frequency digit that corresponds to the active tuning rate.

**Implementation:**
1. RadioState now parses VT (Main) and VT$ (Sub) commands and emits signals on change
2. VFOWidget draws a white underline below the target digit in paintEvent()
3. RATE button wired: left-click sends SW73 (cycle 1Hz↔10Hz), right-click sends SW150 (jump to 100Hz)

**VT Rate to Digit Position Mapping:**
- VT0-4 map directly to positions 0-4 (1Hz, 10Hz, 100Hz, 1kHz, 10kHz)
- VT5 (from SW150/KHZ) maps to position 2 (100Hz) - special case matching K4 behavior

**Files Modified:**
- `src/models/radiostate.h` - Added `tuningStepB()`, `tuningStepChanged()`, `tuningStepBChanged()` signals
- `src/models/radiostate.cpp` - Added VT$ parsing, updated VT parsing to emit signals
- `src/ui/vfowidget.h/.cpp` - Added `setTuningRate()`, `paintEvent()`, `drawTuningRateIndicator()`
- `src/ui/rightsidepanel.h/.cpp` - Added `khzClicked()` signal for right-click on RATE button
- `src/mainwindow.cpp` - Connected tuningStep signals to VFOWidgets, wired RATE button clicks

---

### Fix: Mini-Pan Passband Indicator

**Summary:** Fixed passband filter overlay rendering in mini-pan widget after QRhi/Metal migration.

**Issues Fixed:**
1. Passband not rendering at all - uniform buffer struct had wrong layout (padding must come BEFORE color for std140)
2. Passband only rendering in bottom portion - changed Y coordinates from waterfallHeight→h to 0→h for full height
3. Passband drawn off-screen - IF shift multiplier was 100 instead of 10 (140 → 1400 Hz, not 14000 Hz)
4. CW mode passband positioning - now relative to CW pitch (centered when shift=pitch)

**Files Modified:**
- `src/dsp/minipan_rhi.cpp` - Fixed uniform buffer layout, Y coordinates, shift calculation, CW mode handling
- Removed debug output from minipan_rhi.cpp

---

### Enhancement: Span Control Improvements

**Summary:** Updated span +/- button behavior and increment logic to match K4 native behavior.

**Changes:**
1. Inverted button behavior: - zooms out (increase span), + zooms in (decrease span)
2. K4 span increment sequence: +1kHz steps from 5→144, then +4kHz steps from 144→368 (and reverse for decrement)
3. Applied to both panadapter buttons and Display popup SPAN controls

**Files Modified:**
- `src/mainwindow.cpp` - Added `getNextSpanUp()`, `getNextSpanDown()` helpers, updated all span button handlers

---

## January 2, 2026

### Enhancement: Dual-Channel Audio Mixing with Volume Controls

**Summary:** Added independent volume controls for Main RX (VFO A) and Sub RX (VFO B) audio channels. K4 sends interleaved stereo audio where left channel = Main RX and right channel = Sub RX.

**Problem:** Previously only the main (left) channel was extracted and played. Sub RX audio was discarded entirely.

**Solution:**
1. Added SUB volume slider below existing MAIN slider in SideControlPanel
2. Modified OpusDecoder to extract both stereo channels and mix them with independent volume controls
3. Both sliders persist their values via RadioSettings

**Implementation Details:**
- `OpusDecoder::decodeK4Packet()` now extracts both channels from interleaved stereo
- Each channel gets its own volume multiplier (0.0-1.0) applied before mixing
- Mixed output is clamped to prevent clipping when both channels are at high volume
- Main slider uses amber handle (#FFB000), Sub slider uses cyan (#00BFFF) to match VFO colors

**Files Modified:**
- `src/settings/radiosettings.h/.cpp` - Added `subVolume()` and `setSubVolume()` for persistence
- `src/ui/sidecontrolpanel.h/.cpp` - Added SUB slider, renamed VOL label to MAIN
- `src/audio/opusdecoder.h/.cpp` - Added `setMainVolume()`, `setSubVolume()`, dual-channel mixing
- `src/mainwindow.cpp` - Connected sliders to OpusDecoder volume methods

---

### Enhancement: SUB/DIVERSITY Button Wiring

**Summary:** Wired up SUB and DIVERSITY buttons on the right side panel to send SW commands.

**Buttons:**
- Left-click SUB → sends `SW83;` (toggle Sub Receiver)
- Right-click SUB (DIVERSITY) → sends `SW152;` (toggle Diversity mode)

**CAT State Parsing:**
- `SB0`/`SB1` - Sub Receiver off/on → emits `subRxEnabledChanged(bool)`
- `DV0`/`DV1` - Diversity off/on → emits `diversityChanged(bool)`

**Files Modified:**
- `src/models/radiostate.h` - Added `m_diversityEnabled`, `diversityEnabled()`, signals
- `src/models/radiostate.cpp` - Added DV parsing, updated SB parsing to emit signal
- `src/ui/rightsidepanel.h/.cpp` - Added `diversityClicked()` signal and event filter
- `src/mainwindow.cpp` - Connected subClicked and diversityClicked to SW commands

---

### Fix: IF Shift Passband Positioning

**Summary:** Fixed IF shift calculation for passband overlay positioning. The K4 reports IF shift in decahertz (10 Hz units), not as a 0-99 index value.

**Problem:** Passband overlay was positioned incorrectly when IF shift was adjusted. With a displayed shift of "1.50" (1.5 kHz), the passband was far from where it should be relative to the dial marker.

**Root Cause:** The code assumed IF shift was a 0-99 value with 50 as center, then multiplied by 42 Hz/step. Debug output revealed the K4 actually reports:
- `m_ifShift = 150` for a 1.50 kHz shift (150 decahertz = 1500 Hz)
- `m_ifShift = 50` for CW mode (50 decahertz = 500 Hz, the CW pitch)

**Solution:** Changed IF shift calculation from `(m_ifShift - 50) * 42` to `m_ifShift * 10`:
```cpp
// K4 IF shift is reported in decahertz (10 Hz units)
int shiftOffsetHz = m_ifShift * 10;
```

**Mode-Specific Passband Positioning:**
- **USB/DATA**: Passband center = dial + shiftOffsetHz
- **LSB**: Passband center = dial - shiftOffsetHz
- **CW/CW-R**: Passband centered at dial + pitch offset (shift value is the pitch)
- **AM/FM**: Passband centered at dial + shiftOffsetHz

**Files Modified:**
- `src/dsp/panadapter_rhi.cpp` - Fixed IF shift calculation in passband and marker drawing

---

### Enhancement: Fragment Shader Spectrum Rendering

**Summary:** Replaced triangle strip spectrum rendering with a fragment shader approach for per-pixel control of spectrum fill. This eliminates the "flashing columns" artifact when strong signals appeared.

**Problem:** Triangle strip spectrum rendering caused entire vertical columns to flash bright when strong signals appeared. This happened because triangle strips connect adjacent samples horizontally, causing the GPU to interpolate colors between peaks of different heights. There was no way to have independent per-column control with this approach.

**Solution:** Fragment shader spectrum rendering:
1. Upload spectrum data as a 1D texture (R32F format, 2048 texels)
2. Render a fullscreen quad covering the spectrum area
3. Fragment shader samples the texture at each X position
4. Discard pixels above the spectrum value, fill pixels below
5. Color based on absolute Y position (higher = brighter)

**Key Implementation Details:**
- Texture uploads must happen BEFORE `beginPass()` in Metal (via `resourceUpdate()`)
- Color gradient based on absolute screen position, not relative to each signal
- Signals that reach high get bright colors; signals at noise floor stay dim
- Smooth gradient using `smoothstep()` for natural color transitions

**New Files:**
- `src/dsp/shaders/spectrum_fill.vert` - Vertex shader for fullscreen quad
- `src/dsp/shaders/spectrum_fill.frag` - Fragment shader with per-pixel spectrum sampling

**Files Modified:**
- `src/dsp/panadapter_rhi.h` - Added m_spectrumDataTexture, m_spectrumFillPipeline, m_spectrumFillSrb, m_spectrumFillVbo, m_spectrumFillUniformBuffer
- `src/dsp/panadapter_rhi.cpp` - Initialize texture/pipeline, upload spectrum data before render pass, draw with new pipeline
- `CMakeLists.txt` - Added spectrum_fill shaders to all qt6_add_shaders blocks

**Color Configuration:**
- Peak color: Bright lime (100, 255, 100) at full alpha
- Base color: Visible green (30, 100, 30) at 70% alpha
- Gradient reaches peak brightness at 40% height
- Gentle fade at bottom edge with 15% minimum visibility

**Result:** Smooth, professional spectrum fill where color intensity properly corresponds to signal strength. No more flashing artifacts.

---

### Major: OpenGL to Metal/RHI Migration for Panadapter Overlays

**Summary:** Migrated PanadapterRhiWidget overlay rendering (grid, passband, frequency marker) to properly work with Qt RHI Metal backend. This required significant changes to buffer management to avoid GPU conflicts.

**Problem:** After the initial Metal migration, overlay elements (grid, passband, marker) were rendering incorrectly:
- Grid turning blue instead of gray
- Passband rendering as opaque triangles instead of translucent rectangles
- VFO B passband fully opaque green instead of translucent
- Overlays extending into waterfall area instead of only spectrum area
- CW mode passband/marker not positioned at CW pitch offset

**Root Cause:** Multiple overlay draw calls were sharing the same GPU buffers (`m_overlayVbo`, `m_overlayUniformBuffer`), causing corruption when data from one draw was overwritten before the GPU consumed it.

**Solution:** Created separate RHI buffers and shader resource bindings for each overlay element:
- `m_passbandVbo`, `m_passbandUniformBuffer`, `m_passbandSrb` - Filter passband quad
- `m_markerVbo`, `m_markerUniformBuffer`, `m_markerSrb` - Frequency marker line

**Additional Fixes:**
1. **Passband height**: Changed from full widget height `h` to `spectrumHeight` so passband only appears in spectrum area
2. **VFO B translucency**: Fixed color from `QColor(0, 200, 0)` to `QColor(0, 200, 0, 64)` for proper alpha
3. **Marker height**: Changed to `spectrumHeight` to not extend into waterfall
4. **CW mode positioning**: Added pitch offset to marker frequency calculation:
   ```cpp
   qint64 markerFreq = m_tunedFreq;
   if (m_mode == "CW") {
       markerFreq = m_tunedFreq + m_cwPitch;
   } else if (m_mode == "CW-R") {
       markerFreq = m_tunedFreq - m_cwPitch;
   }
   ```

**Missing VFO B Connections:** Added 6 missing signal connections for VFO B panadapters:
- `ifShiftBChanged` → `setIfShift` (main panadapter B)
- `cwPitchChanged` → `setCwPitch` (main panadapter B)
- `notchChanged` → `setNotchFilter` (main panadapter B)
- Same three connections for mini-pan B

**Files Modified:**
- `src/dsp/panadapter_rhi.h` - Added separate buffer/SRB members for passband and marker
- `src/dsp/panadapter_rhi.cpp` - Buffer creation, SRB creation, height fixes, CW pitch offset
- `src/mainwindow.cpp` - VFO B passband color alpha, 6 missing VFO B connections

**Known Bug - Notch Filter Indicator:**
The notch filter indicator (red vertical line) was attempted but renders incorrectly - it creates the "entire grid in red" instead of a single vertical line. The code reuses `m_overlayVbo`/`m_overlayUniformBuffer` after other draws complete, but something is still causing the corruption. Needs dedicated buffers like passband/marker.

**Status:** Passband, marker, grid all working correctly. Notch indicator is a known bug requiring further investigation.

---

### Fix: QRhiWidget Metal Initialization (Qt 6.10.1 + macOS Tahoe)

**Problem:** Panadapter displayed black screen with two distinct failure modes:
1. "QRhiWidget: No QRhi" - initialize() never called
2. "QMetalSwapChain only supports MetalSurface windows" - crash after initialize()

**Root Cause:** Two Qt 6.10.1 bugs on macOS Tahoe 26.2:

1. **menuBar() Order**: Calling `menuBar()` before creating QRhiWidget prevents the RHI backing store from being set up correctly on the top-level window.

2. **WA_NativeWindow Attribute**: Setting `Qt::WA_NativeWindow` forces native window creation before QRhiWidget can configure it for MetalSurface, causing a swapchain crash.

**Fix 1 - menuBar Order** (already applied):
```cpp
// In MainWindow constructor:
// IMPORTANT: setupUi() MUST be called BEFORE setupMenuBar()!
setupUi();      // Creates QRhiWidgets first
setupMenuBar(); // Menu bar after - no interference
```

**Fix 2 - Remove WA_NativeWindow**:
```cpp
// REMOVED from setupUi():
// setAttribute(Qt::WA_NativeWindow, true);  // Causes Metal swapchain crash
```

Added explanatory comments in code to prevent regression.

**Files Modified:**
- `src/mainwindow.cpp` - Swapped setupUi/setupMenuBar order, removed WA_NativeWindow

**Test Files Created (for reproduction):**
- `test_with_menubar.cpp` - Proves menuBar-first fails
- `test_menubar_after.cpp` - Proves panadapter-first works
- `test_minimal_mainwindow.cpp` - Proves WA_NativeWindow causes crash

**Result:** All 4 QRhiWidgets (PanadapterA, PanadapterB, MiniPanA, MiniPanB) now initialize with Metal backend on macOS.

---

### Enhancement: Panadapter Raw Data, Alignment Fix, UI Restoration

**Changes:**

1. **Removed EMA Smoothing** - Spectrum and waterfall now display raw K4 data without client-side smoothing. This allows testing whether the K4 server sends already-averaged data or raw data that clients must process.

2. **Fixed Frequency Alignment (~25Hz offset)** - Integer division truncation in bin extraction caused asymmetric extraction when `(totalBins - requestedBins)` was odd. Changed from truncating to rounding:
   ```cpp
   // Before: int centerStart = (totalBins - requestedBins) / 2;
   // After:  int centerStart = (totalBins - requestedBins + 1) / 2;
   ```

3. **Restored UI Elements (lost in OpenGL conversion)**:
   - Reference level display in upper left ("REF: -10 dBm")
   - Gradient background (light center → dark edges) in spectrum area
   - Frequency indicators on waterfall: lower (left), center, upper (right) frequencies

**Files Modified:**
- `src/dsp/panadapter.cpp` - Removed smoothing, fixed alignment, added UI overlays
- `src/dsp/minipanwidget.cpp` - Removed smoothing

---

### Fix: B SET Filter Targeting

**Problem:** When B SET was enabled, adjusting BW/SHFT/HI/LO controls via scroll wheel still targeted VFO A instead of VFO B.

**Root Cause:** The filter control handlers in `mainwindow.cpp` did not check `bSetEnabled()` state. They always:
1. Read VFO A values (`filterBandwidth()`, `ifShift()`)
2. Sent VFO A CAT commands (`BW`, `IS`)
3. Updated VFO A state optimistically

**Fix:**
- Added `setFilterBandwidthB()` and `setIfShiftB()` optimistic setters to RadioState
- Updated all 4 filter handlers (bandwidthChanged, highCutChanged, shiftChanged, lowCutChanged) to:
  - Check `bSetEnabled()` state
  - Use VFO B getters when B SET is active
  - Send VFO B commands with `$` suffix (`BW$`, `IS$`) when B SET is active
  - Update VFO B state optimistically when B SET is active

**Files Modified:**
- `src/models/radiostate.h` - Added `setFilterBandwidthB()`, `setIfShiftB()` declarations
- `src/models/radiostate.cpp` - Added setter implementations
- `src/mainwindow.cpp` - Updated filter control handlers (lines 823-872)

---

## January 1, 2026

### Fix: RF Gain and Power Command Formats

**RF Gain (Main & Sub):**
- Fixed command format to `RG-nn;` and `RG$-nn;` (with minus sign)
- Range corrected to 0-60 (was incorrectly 0-100)
- Button faces now display with minus sign (e.g., "-0", "-30", "-60")
- Scroll direction: up = less attenuation, down = more attenuation

**Power (QRP/QRO Modes):**
- QRP (≤10W): Uses 0.1W increments with `PCnnnL;` format
- QRO (>10W): Uses 1W increments with `PCnnnH;` format
- Smooth transitions: 10.0W ↔ 11W when crossing threshold
- Button face shows decimals for QRP ("9.9"), whole numbers for QRO ("50")

**Files Modified:**
- `src/mainwindow.cpp` - Fixed RG/RG$ format, power mode transitions
- `src/ui/sidecontrolpanel.h` - Changed `setPower(int)` to `setPower(double)`
- `src/ui/sidecontrolpanel.cpp` - RF gain displays minus sign, power shows decimals for QRP

---

### Fix: Scroll Values Continue Changing and Button Faces Update

**Problem:** When scrolling DualControlButtons, values would only change once then stop. Button faces didn't update to show new values.

**Root Cause:** The K4 radio does NOT echo back setting commands (KS, BW, PC, etc.). When we sent a CAT command:
1. RadioState never received the echo → internal state stayed stale
2. Next scroll used stale value → sent same command again
3. Signal never emitted → button face never updated

**Secondary Issue:** BW command format was wrong - we sent Hz value but K4 expects value/10.

**Fix - Optimistic Updates:**
After sending each CAT command, immediately update RadioState with the new value:
- Ensures next scroll uses correct current value
- Emits signal → triggers SideControlPanel setter → updates button face

**RadioState Setters Added:**
- `setKeyerSpeed(int)` - WPM
- `setCwPitch(int)` - CW pitch in Hz
- `setRfPower(double)` - Power in watts
- `setFilterBandwidth(int)` - BW in Hz
- `setIfShift(int)` - IF shift
- `setRfGain(int)`, `setRfGainB(int)` - RF gain main/sub
- `setSquelchLevel(int)`, `setSquelchLevelB(int)` - Squelch main/sub
- `setMicGain(int)`, `setCompression(int)` - Voice mode controls

**Command Format Fixes:**
- BW: Now divides by 10 before sending (BW2000 → BW0200)
- PC: Uses correct PCnnnL/H format (L=QRP 0.1-10W, H=QRO 1-110W)

**Files Modified:**
- `src/models/radiostate.h` - Added public setter declarations
- `src/models/radiostate.cpp` - Implemented setters (update + emit)
- `src/mainwindow.cpp` - Call setters after sendCAT, fix BW/PC format

---

### Fix: DualControlButton Click Behavior and Scroll Signal Wiring

Fixed two issues with the left side panel DualControlButtons:

**Issue 1: Click always swapped labels**
Clicking an inactive button immediately swapped primary/alternate labels. This was wrong - the first click should only activate the button (show the indicator bar), and subsequent clicks should swap.

**Fix:**
- Changed `mousePressEvent()` to check `m_showIndicator` before swapping
- Added new `swapped()` signal that only emits when an actual swap occurs
- Updated SideControlPanel to connect to `swapped()` instead of `clicked()` for tracking primary/alternate state

**Issue 2: Scroll wheel didn't change values**
Mouse wheel over buttons didn't adjust values or send CAT commands. The `valueScrolled` signals were never connected in MainWindow.

**Fix:**
Added all 14 scroll signal connections in MainWindow:
- **Group 1 (Global):** WPM→KS, PTCH→CW, MIC→MG, CMP→CP, PWR→PC, DLY→SD
- **Group 2 (MainRx):** BW→BW, HI→BW, SHFT→IS, LO→IS
- **Group 3 (RF/SQL):** M.RF→RG, M.SQL→SQ, S.RF→RG$, S.SQL→SQ$

**CAT Command Reference:**
| Signal | Command | Range | Description |
|--------|---------|-------|-------------|
| wpmChanged | KSnnn; | 008-050 | Keyer speed WPM |
| pitchChanged | CWnn; | 30-99 | CW pitch (×10 = Hz) |
| powerChanged | PCnnn; | 000-110 | Power control watts |
| delayChanged | SDnnnn; | 0000-2500 | QSK/VOX delay (×10ms) |
| bandwidthChanged | BWnnnn; | 0050-5000 | Filter bandwidth Hz |
| shiftChanged | IS±nnnn; | -9999 to +9999 | IF shift (×10Hz) |
| mainRfGainChanged | RGnnn; | 000-100 | RF gain |
| mainSquelchChanged | SQnnn; | 000-029 | Squelch level |
| subRfGainChanged | RG$nnn; | 000-100 | Sub RX RF gain |
| subSquelchChanged | SQ$nnn; | 000-029 | Sub RX squelch |

**Files Modified:**
- `src/ui/dualcontrolbutton.h` - Added `swapped()` signal
- `src/ui/dualcontrolbutton.cpp` - Only swap when already active
- `src/ui/sidecontrolpanel.cpp` - Changed from `clicked` to `swapped`
- `src/mainwindow.cpp` - Added all 14 scroll signal → CAT command connections

---

### Feature: Mode-Dependent WPM/PTCH vs MIC/CMP Display

The left side panel's first button group now switches between CW and Voice mode controls based on the current operating mode.

**Behavior:**
- **CW/CW-R modes**: Shows WPM/PTCH (keyer speed / CW pitch)
- **Voice/Data modes (LSB, USB, AM, FM, DATA, DATA-R)**: Shows MIC/CMP (mic gain / compression)

**CAT Commands:**
- `MG;` / `MGxxx;` - Mic gain query/set (000-080)
- `CP;` / `CPnnn;` - Compression query/set (000-030, SSB only)

**Implementation Details:**
- `setDisplayMode(bool isCWMode)` switches button labels between WPM/PTCH and MIC/CMP
- Mode change triggers value refresh from RadioState to populate current values
- Scroll wheel adjusts active value and sends CAT commands

**Files Modified:**
- `src/models/radiostate.h` - Added `keyerSpeed()`, `compression()`, `micGainChanged`, `compressionChanged` signals
- `src/models/radiostate.cpp` - Added CP parsing, fixed MG to emit signal
- `src/ui/sidecontrolpanel.h` - Added `setDisplayMode()`, `setMicGain()`, `setCompression()`, new signals
- `src/ui/sidecontrolpanel.cpp` - Implemented mode switching, updated scroll handler
- `src/mainwindow.cpp` - Connected mode changes, MG/CP signals, added CAT queries

---

### Feature: B SET Indicator and Sub RX Filter Integration

When B SET is enabled (BS1), a green "B SET" indicator appears in the center column and the side panel BW/SHFT filter controls switch to display VFO B (Sub RX) values with green indicator color.

**Features:**
- B SET indicator: Green rounded rectangle with black "B SET" text
- Position: In center column, replaces SPLIT label when B SET is active
- Side panel indicator: BW/SHFT bar changes from cyan to green when B SET enabled
- Filter values: Display VFO B bandwidth and shift values when B SET is active
- IS$ parsing: Added Sub RX IF shift command parsing
- BS command parsing: B SET state (BS0=off, BS1=on) parsed from RDY; response

**Implementation Details:**
- B SET label created in setupVfoSection() with green background (#00FF00), black text, 4px border-radius
- When B SET enabled: Show B SET label, hide SPLIT label, change side panel indicator color via `setActiveReceiver(true)`
- When B SET disabled: Hide B SET label, show SPLIT label, reset indicator color via `setActiveReceiver(false)`
- `updateFilterDisplay` lambda now checks `bSetEnabled()` to select VFO A or B filter values
- B SET is toggled via SW44; command, radio responds with BS0/BS1 state

**CAT Commands:**
- `SW44;` - Toggle B SET on/off
- `BS0;` / `BS1;` - Radio response indicating B SET state (included in RDY;)

**Files Modified:**
- `src/models/radiostate.h` - Added `m_ifShiftB`, `ifShiftB()`, `shiftBHz()`, `ifShiftBChanged` signal, `setBSetEnabled()`, `toggleBSet()`
- `src/models/radiostate.cpp` - Added IS$ and BS command parsing
- `src/mainwindow.h` - Added `QLabel *m_bSetLabel;`
- `src/mainwindow.cpp` - Created B SET label, connected bSetChanged for visibility/color, updated filter display logic

---

### Feature: Bottom Menu Bar Popup Widgets

Added popup menus for Fn, MAIN RX, SUB RX, and TX buttons in the bottom menu bar. These popups use the same visual style as the BAND popup but with a single row of 7 buttons.

**Features:**
- ButtonRowPopup widget: Single-row popup with 7 buttons
- Same styling as BandPopupWidget (dark gradient, gray bottom strip, triangle indicator)
- Triangle indicator points to the triggering button
- Toggle behavior: second press closes popup, switching buttons auto-closes previous popup
- All bottom menu popups now use consistent toggle behavior including BAND

**Popup Layout:**
```
┌─────────────────────────────────────────────────────────────────┐
│  [ 1 ]  [ 2 ]  [ 3 ]  [ 4 ]  [ 5 ]  [ 6 ]  [ 7 ]               │
│─────────────────────────────────────────────────────────────────│
│                            ▼                                    │
└─────────────────────────────────────────────────────────────────┘
```

**Button Sizing:**
- Button: 70x44px (same as BAND)
- Spacing: 8px between buttons
- Margins: 12px
- Bottom strip: 8px
- Triangle: 24x12px
- Total width: 562px, height: ~88px

**Implementation Details:**
- `closeAllPopups()` helper closes all open popups (menu overlay, band, display, Fn, Main RX, Sub RX, TX)
- Each toggle method checks if popup was visible before closing all, then opens if it wasn't
- Active button state (inverse colors) syncs with popup visibility

**Files Created:**
- `src/ui/buttonrowpopup.h` - Single-row popup widget header
- `src/ui/buttonrowpopup.cpp` - Implementation based on BandPopupWidget

**Files Modified:**
- `src/ui/bottommenubar.h` - Added button getters, setActive methods for all buttons
- `src/ui/bottommenubar.cpp` - Implemented setActive methods
- `src/mainwindow.h` - Added 4 popup members, toggle methods, closeAllPopups()
- `src/mainwindow.cpp` - Created popups, wired signals, implemented toggle methods
- `CMakeLists.txt` - Added buttonrowpopup.cpp/h to build

---

### Feature: Feature Menu Bar Widget (Phase 1 - UI Complete)

Added a reusable menu bar widget that appears above the 7 bottom buttons when alternate actions are triggered on PRE, NB, NR, NTCH buttons.

**Menu Configurations:**

| Trigger | Signal | Menu Title | Has Extra Button | Value Unit |
|---------|--------|------------|------------------|------------|
| ATTN (right-click PRE) | `attnClicked()` | ATTENUATOR | No | dB |
| LEVEL (right-click NB) | `levelClicked()` | NB LEVEL | Yes (FILTER NONE) | (none) |
| ADJ (right-click NR) | `adjClicked()` | NR ADJUST | No | (none) |
| MANUAL (right-click NTCH) | `manualClicked()` | MANUAL NOTCH | No | Hz |

**Menu Layout:**

```
┌────────────────────────────────────────────────────────────────────┐
│  [TITLE] │ [OFF] │ [EXTRA?] │ [VALUE] │ [-] │ [+] │ [⟳] │ [A] │ [←] │
└────────────────────────────────────────────────────────────────────┘
```

**Elements:**
1. **Title** - Feature name in framed box (gradient background, border)
2. **OFF/ON** - Toggle button (shows current state)
3. **Extra** - Optional button (only NB LEVEL has "FILTER NONE")
4. **Value** - Current setting value with unit
5. **-/+** - Decrement/increment buttons
6. **⟳** - Encoder hint icon (indicates VFO A encoder can adjust value)
7. **[A]** - Encoder hint label (static indicator, not a button)
8. **←** - Back/close button

**Visual Styling:**
- Custom `paintEvent()` for outer border (rounded rect with gradient)
- Vertical delimiter lines between widget groups
- Border wraps tightly around content (not full window width)
- Centered over bottom menu bar (105px left margin to align with side panel)

**Toggle Behavior:**
- Right-click same button again → hides menu
- Right-click different button → switches to that menu
- Close button (←) → hides menu

**Files Created/Modified:**
- `src/ui/featuremenubar.h` - Widget header with Feature enum, paintEvent override
- `src/ui/featuremenubar.cpp` - Full implementation with custom painting
- `src/mainwindow.cpp` - Toggle logic for all 4 feature menus

---

### Feature: Feature Menu Bar CAT Wiring (Phase 2 - Complete)

**CAT Commands Implemented:**

| Feature | Toggle | Increment | Decrement |
|---------|--------|-----------|-----------|
| **ATTENUATOR** | `RA/;` | `RA+;` (3dB step) | `RA-;` (3dB step) |
| **NB LEVEL** | `NB/;` | `NBnnmf;` (+1 level) | `NBnnmf;` (-1 level) |
| **NR ADJUST** | `NR/;` | `NRnnm;` (+1 level) | `NRnnm;` (-1 level) |
| **MANUAL NOTCH** | `NM/;` | `NMnnnnm;` (+10Hz) | `NMnnnnm;` (-10Hz) |

**NB LEVEL Extra Button:**
- Cycles filter: NONE(0) → NARROW(1) → WIDE(2) → NONE(0)
- Command: `NBnnmf;` with updated filter value

**RadioState Integration:**
- Menu bar displays current state from RadioState on open
- Live updates via `processingChanged` and `notchChanged` signals
- Added `noiseBlankerFilterWidth()` getter for Main and Sub RX

**Files Modified:**
- `src/models/radiostate.h` - Added `noiseBlankerFilterWidth()`, `noiseBlankerFilterWidthB()` getters
- `src/models/radiostate.cpp` - Updated NB parsing to store filter width for Sub RX
- `src/ui/featuremenubar.h` - Added `setNbFilter()` method
- `src/ui/featuremenubar.cpp` - Implemented `setNbFilter()` to update extra button text
- `src/mainwindow.cpp` - Added CAT command connections for all feature menu actions

**Phase 2 Scope (Completed):**
- ✅ CAT commands for toggle (all 4 features)
- ✅ CAT commands for increment/decrement (all 4 features)
- ✅ NB filter cycling via extra button
- ✅ RadioState integration for state display
- ✅ Live updates from radio state changes

---

### Feature: Feature Menu Bar B SET VFO Targeting (Phase 3 - Complete)

When B SET is enabled, feature menu bar commands now target Sub RX instead of Main RX.

**RadioState Changes:**
- Added `m_bSetEnabled` member to track B SET state
- Added `bSetEnabled()` getter
- Added `bSetChanged(bool)` signal
- Added parsing for `TB` command (TB0=off, TB1=on)

**CAT Command Targeting:**
| B SET | Attenuator | NB | NR | Manual Notch |
|-------|------------|----|----|--------------|
| OFF | `RA/;`, `RA+;`, `RA-;` | `NBnnmf;` | `NRnnm;` | `NMnnnnm;` |
| ON | `RA$/;`, `RA$+;`, `RA$-;` | `NB$nnmf;` | `NR$nnm;` | `NM$nnnnm;` |

**State Display:**
- When B SET is enabled, display reads from Sub RX state (e.g., `attenuatorLevelB()` instead of `attenuatorLevel()`)
- Connected `processingChangedB` signal to update menu bar when Sub RX state changes
- Connected `bSetChanged` signal to refresh display when B SET toggles

**Optimistic Updates:**
- All button handlers read current state from correct VFO based on B SET
- Increment/decrement calculate new values from correct source

**Files Modified:**
- `src/models/radiostate.h` - Added `m_bSetEnabled`, `bSetEnabled()`, `bSetChanged()`
- `src/models/radiostate.cpp` - Added `TB` command parsing
- `src/mainwindow.cpp` - Updated all feature menu handlers and state display for B SET

---

### Feature: Filter Position Indicators (FIL1/FIL2/FIL3)

Added filter position indicators below the A/B VFO squares, horizontally aligned with the RIT/XIT display.

**Layout:**
- Created `filterRitXitRow` horizontal layout containing:
  - VFO A filter label (45px container, aligned with A square)
  - Stretch
  - RIT/XIT box (centered)
  - Stretch
  - VFO B filter label (45px container, aligned with B square)

**Styling:**
- Color: `#FFD040` (lighter golden yellow, distinct from TX SPLIT amber)
- Font: 10px bold (matches RIT/XIT labels)
- Text: "FIL1", "FIL2", or "FIL3" based on filter position

**Signal Connections:**
- `RadioState::filterPositionChanged` → updates `m_filterALabel`
- `RadioState::filterPositionBChanged` → updates `m_filterBLabel`

**Files Modified:**
- `src/mainwindow.h` - Added `m_filterALabel`, `m_filterBLabel` members
- `src/mainwindow.cpp` - Created filter layout in `setupVfoSection()`, connected signals

---

### Feature: TX Function Button Wiring and Status Indicators

Wired up the 6 TX function buttons on the left side panel to send CAT commands, with left-click for primary actions and right-click for secondary actions. Added UI indicators for TEST, QSK, and ATU states.

**Button Commands:**

| Button | Left-Click | Right-Click |
|--------|------------|-------------|
| TUNE | SW16; | SW131; (TUNE LP) |
| XMIT | SW30; | SW132; (TEST) |
| ATU | SW158; | SW40; (ATU TUNE) |
| VOX | SW50; | SW134; (QSK) |
| ANT | SW60; | TBD (REM ANT) |
| RX ANT | SW70; | SW157; (SUB ANT) |

**State Parsing:**

| State | CAT Command | RadioState Member |
|-------|-------------|-------------------|
| TEST mode | `TSn;` (n=0/1) | `m_testMode`, `testModeChanged()` |
| QSK enable | `SDxyzzz;` (x=1 full QSK) | `m_qskEnabled`, `qskEnabledChanged()` |
| ATU mode | `ATn;` (n=1 bypass, n=2 auto) | `m_atuMode`, `atuModeChanged()` |

**UI Indicators Added:**

1. **QSK indicator** - Next to VOX in antenna row
   - Grey (#999999) when off, white (#FFFFFF) when enabled
   - Updates when SD command parsed with QSK flag

2. **TEST indicator** - Above TX label in center column
   - Red (#FF0000), 14px bold
   - Visible only when test mode active (TS1;)

3. **ATU indicator** - Below RIT/XIT box in center column
   - Orange (#FFB000), 12px bold
   - Visible only when ATU in AUTO mode (AT2;)

**Implementation:**

- SideControlPanel: Added 12 signals for button clicks, event filter for right-click handling
- RadioState: Added TS parser, modified SD parser to extract QSK flag, added atuModeChanged signal
- MainWindow: Connected all signals to sendCAT lambdas, created indicator labels, connected state signals

**Files Modified:**
- `src/models/radiostate.h` - Added m_testMode, m_qskEnabled, getters, signals
- `src/models/radiostate.cpp` - Added TS parser, modified SD/AT parsers
- `src/ui/sidecontrolpanel.h` - Added 12 TX button signals, eventFilter override
- `src/ui/sidecontrolpanel.cpp` - Connected buttons, installed event filters, implemented right-click handling
- `src/mainwindow.h` - Added onAtuModeChanged slot, m_atuLabel member
- `src/mainwindow.cpp` - Wired button signals, created indicators, connected state signals

---

### Feature: Right Side Panel Button Wiring

Wired up 14 buttons on the right side panel (5×2 main grid + 2×2 PF grid) to send CAT commands, with left-click for primary actions and right-click for secondary actions.

**Button Commands (Main 5×2 Grid):**

| Button | Left-Click | Right-Click |
|--------|------------|-------------|
| PRE | SW61; | SW141; (ATTN) |
| NB | SW32; | SW142; (LEVEL) |
| NR | SW62; | SW143; (ADJ) |
| NTCH | SW31; | SW140; (MANUAL) |
| FIL | SW33; | SW144; (APF) |
| A/B | SW41; | SW145; (SPLIT) |
| REV | TBD | TBD |
| A→B | SW72; | SW147; (B→A) |
| SPOT | SW42; | SW146; (AUTO) |
| MODE | SW43; | SW148; (ALT) |

**Button Commands (PF 2×2 Grid):**

| Button | Left-Click | Right-Click |
|--------|------------|-------------|
| B SET | SW44; | SW153; (PF1) |
| CLR | SW64; | SW154; (PF2) |
| RIT | SW54; | SW155; (PF3) |
| XIT | SW74; | SW156; (PF4) |

**Note:** MODE (SW43) targets Main RX by default. When B SET is engaged, MODE targets Sub RX/VFO B.

**State Tracking Added:**

| State | CAT Command | RadioState Changes |
|-------|-------------|-------------------|
| Filter Position (A) | FP | Modified to emit `filterPositionChanged(int)` |
| Filter Position (B) | FP$ | Added `filterPositionB()`, `filterPositionBChanged(int)` |

**Implementation:**

- RadioState: Added FP$ parser (before FP parser), added filterPositionB getter and signals for both A/B
- RightSidePanel: Added 14 secondary signals for right-click, eventFilter for right-click handling
- MainWindow: Wired 26 button signals (13 primary + 13 secondary) to sendCAT lambdas

**Files Modified:**
- `src/models/radiostate.h` - Added filterPositionB(), filterPositionChanged, filterPositionBChanged signals
- `src/models/radiostate.cpp` - Added FP$ parser, modified FP parser to emit signal
- `src/ui/rightsidepanel.h` - Added 14 secondary signals, eventFilter override
- `src/ui/rightsidepanel.cpp` - Installed event filters, implemented right-click handling
- `src/mainwindow.cpp` - Wired all right side panel button signals to sendCAT

**Open Items:**
- REV button needs press/release pattern (SW160 down, SW161 up) - TBD

---

## December 31, 2025

### Feature: PanadapterWidget OpenGL Migration

Migrated PanadapterWidget from CPU-based QPainter to GPU-accelerated QOpenGLWidget for dramatically improved performance at high resolutions.

**Problem:**
At high resolutions (2500×1300), the app showed:
- 114% CPU usage, 0% GPU usage (Activity Monitor)
- Choppy/sluggish waterfall scrolling
- Blocky/pixelated spectrum display
- Per-frame waterfall image rebuilding (~640K float operations)

**Root Cause:**
QPainter uses macOS raster paint engine - all rendering is CPU-bound. The waterfall QImage was being rebuilt from scratch every frame.

**Solution:**
Complete OpenGL rewrite using QOpenGLWidget with GLSL shaders.

**Architecture:**
```
GPU Memory:
├─ Waterfall Texture (256 rows × 2048 width, GL_LUMINANCE)
├─ Color LUT Texture (256×1, RGBA)
└─ Spectrum VBO (line strip vertices)

Per-Frame:
├─ Upload new row → glTexSubImage2D (single row)
├─ Update spectrum vertices
├─ Draw waterfall quad (scrolling via texture coords)
└─ Draw spectrum line strip
```

**Key Technical Decisions:**

1. **GLSL 120 for macOS compatibility** - GLSL 330 not supported on macOS OpenGL
   - Use `attribute`/`varying` instead of `layout`/`in`/`out`
   - Use `sampler2D` instead of `sampler1D` for color LUT

2. **GL_LUMINANCE texture format** - `GL_R8` not supported in OpenGL 2.1
   - Single-channel texture stores normalized dB values (0-255)

3. **Texture coordinate wraparound fix** - Range `[0, 1]` caused top/bottom to sample same row
   - Changed to `[0, (N-1)/N]` = `[0, 0.996]` for 256 rows
   - Prevents GL_REPEAT aliasing at texture boundaries

4. **Device pixel ratio for HiDPI** - macOS Retina displays have DPR=2.0
   - `width()` returns logical pixels, but OpenGL needs physical pixels
   - All viewport and coordinate calculations multiply by `devicePixelRatioF()`

**Shaders:**
- **Waterfall shader**: Samples dB texture, looks up color in LUT texture
- **Spectrum shader**: Transforms pixel coords to NDC, draws line strip
- **Overlay shader**: Draws grid, passband, frequency marker, notch filter

**Files Modified:**
- `CMakeLists.txt` - Added Qt::OpenGL, Qt::OpenGLWidgets dependencies
- `src/dsp/panadapter.h` - Changed QWidget → QOpenGLWidget, added GL members
- `src/dsp/panadapter.cpp` - Complete rewrite with OpenGL rendering

**Performance Results:**
| Metric | Before (QPainter) | After (OpenGL) |
|--------|-------------------|----------------|
| CPU usage | ~114% | ~15-25% |
| GPU usage | 0% | ~5-10% |
| Scaling | Degrades with size | Constant |

---

### Feature: MiniPanWidget OpenGL Migration

Migrated compact VFO-area spectrum widget to OpenGL using same pattern as PanadapterWidget.

**Changes:**
- Base class: QWidget → QOpenGLWidget, protected QOpenGLFunctions
- Same GLSL 120 shaders as main panadapter
- Smaller textures: 100 rows × 512 width (vs 256 × 2048)
- Device pixel ratio support for HiDPI displays
- Peak-hold downsampling for spectrum, average downsampling for waterfall

**Features Preserved:**
- Filter passband overlay
- Center frequency marker
- Notch filter marker (red line)
- Mode-dependent bandwidth (CW=3kHz, Voice/Data=10kHz)
- Click-to-toggle signal
- VFO A/B color customization

**Files Modified:**
- `src/dsp/minipanwidget.h` - Changed to QOpenGLWidget, added GL members
- `src/dsp/minipanwidget.cpp` - Complete rewrite with OpenGL rendering

---

### Feature: Right Side Panel Button Expansion

Added 8 new buttons to RightSidePanel below the existing 5x2 grid.

**New Button Layout:**

| PF Row (2x2) | |
|--------------|--------------|
| B SET / PF 1 | CLR / PF 2 |
| RIT / PF 3 | XIT / PF 4 |

| Bottom Row (2x2) | |
|------------------|-----------------|
| FREQ ENT / SCAN | RATE / KHZ |
| LOCK A / LOCK B | SUB / DIVERSITY |

**Implementation:**
- Added 8 new button pointers and signals to `rightsidepanel.h`
- Added two QGridLayout grids after existing 5x2 grid
- 16px spacing between main grid and PF row
- 32px spacing between PF row and bottom row
- FREQ ENT uses stacked text (`"FREQ\nENT"`)
- Same styling as existing buttons (gradient, white text, orange sub-label)

**Files Modified:**
- `src/ui/rightsidepanel.h` - Added button pointers, signals, updated docs
- `src/ui/rightsidepanel.cpp` - Added grids, spacing, signal connections

---

### Feature: SPAN Control A/B Support

**Problem:**
When PAN = B, the SPAN +/- buttons didn't work correctly:
- CAT commands were sent correctly (`#SPN$` for B)
- But the display didn't update to show VFO B's span value
- Only `spanChanged` (VFO A) was connected to update the display

**Root Cause:**
SPAN control didn't follow the A/B pattern used by REF level:
- Only single `m_currentSpanKHz` value instead of A/B
- Only `setSpanValue()` instead of A/B setters
- No `updateSpanControlGroup()` function
- Only `spanChanged` signal connected, not `spanBChanged`

**Solution:**
Added A/B support following REF level pattern, plus A+B dual targeting.

**DisplayPopupWidget:**
- Replaced `m_currentSpanKHz` with `m_spanA`/`m_spanB`
- Added `setSpanValueA(double)` and `setSpanValueB(double)` setters
- Added `updateSpanControlGroup()` that shows correct value based on A/B toggle
- Added `updateSpanControlGroup()` calls alongside `updateRefLevelControlGroup()` in toggle handlers

**MainWindow:**
- Connected `spanBChanged` signal to `setSpanValueB()`
- Updated SPAN +/- handlers to support A+B dual targeting:

| Toggle State | Commands Sent |
|--------------|---------------|
| A only | `#SPN` |
| B only | `#SPN$` |
| A+B both | `#SPN` AND `#SPN$` |

**Files Modified:**
- `src/ui/displaypopupwidget.h` - Added A/B span tracking, method declarations
- `src/ui/displaypopupwidget.cpp` - Added setters, update function, toggle handler calls
- `src/mainwindow.cpp` - Added `spanBChanged` connection, fixed +/- handlers for A+B

---

### Feature: VFO Cursor Mode → Passband Indicator Sync

**Problem:**
Clicking CURS A+/A- or CURS B+/B- buttons sent the correct CAT commands (`#VFA/;`, `#VFB/;`) but the panadapter passband indicator didn't reflect the cursor visibility state.

**Solution:**
Connected `RadioState::vfoACursorChanged` and `vfoBCursorChanged` signals to `PanadapterWidget::setCursorVisible()`.

**VFO Cursor Modes (#VFA / #VFB):**
| Mode | Name | Passband Visible |
|------|------|------------------|
| 0 | OFF | No |
| 1 | ON | Yes |
| 2 | AUTO | Yes |
| 3 | HIDE | No |

**Rule:** `visible = (mode == 1 || mode == 2)`

**Changes:**

**PanadapterWidget:**
- Added `m_cursorVisible` member variable (default: true)
- Added `setCursorVisible(bool)` setter
- Updated `drawFilterPassband()` guard: `if (!m_cursorVisible || m_filterBw <= 0)`

**MainWindow:**
- Added connections from cursor signals to panadapter visibility:
```cpp
connect(m_radioState, &RadioState::vfoACursorChanged, this, [this](int mode) {
    m_panadapterA->setCursorVisible(mode == 1 || mode == 2);
});
connect(m_radioState, &RadioState::vfoBCursorChanged, this, [this](int mode) {
    m_panadapterB->setCursorVisible(mode == 1 || mode == 2);
});
```

**Files Modified:**
- `src/dsp/panadapter.h` - Added `setCursorVisible()`, `m_cursorVisible`
- `src/dsp/panadapter.cpp` - Added setter, modified guard
- `src/mainwindow.cpp` - Added cursor signal connections to panadapters

---

### Fix: Auto-Ref Parsing - Correct #AR Response Format

**Problem:**
AUTO button never highlighted when radio was in auto mode.

**Root Cause:**
We were parsing the `#AR` response completely wrong. Assumed format was `#ARA` or `#ARM`, but actual K4 format is:

**Format:** `#ARaadd+oom;`
- `aa` = averaging constant (01-20)
- `dd` = debounce constant (04-09)
- `+oo` = user offset (-08 to +08)
- `m` = **mode (1=auto, 0=manual)**

**Example:** `#AR1203+041;` → mode=1 (AUTO)

**Broken code:**
```cpp
bool enabled = (cmd.mid(3, 1) == "A");  // Got '1' from "1203", compared to "A" → false!
```

**Fix:**
```cpp
QChar modeChar = cmd.at(cmd.length() - 2); // Last char before ';'
bool enabled = (modeChar == '1');          // 1=auto, 0=manual
```

**Also fixed:**
- Added `updateRefLevelControlGroup()` call after creating REF control to sync initial state

**Files Modified:**
- `src/models/radiostate.cpp` - Fixed #AR parsing to read mode from last digit
- `src/ui/displaypopupwidget.cpp` - Added initial sync for AUTO button state

---

### Fix: Auto-Ref is GLOBAL (not per-VFO)

**Problem Discovered:**
The previous implementation incorrectly treated Auto-Ref as separate A/B settings. Auto-Ref Level (`#AR`) is a **GLOBAL setting** - it affects BOTH VFOs simultaneously. There is no `#AR$` command.

**What was wrong:**
1. `m_autoRefLevelB` existed in RadioState (should not exist)
2. `autoRefLevelBChanged` signal existed (should not exist)
3. `#AR$` parsing existed (command doesn't exist)
4. DisplayPopupWidget tracked `m_autoRefA` and `m_autoRefB` separately
5. Auto button sent `#AR$/;` when B selected

**Correct behavior:**
- **Auto-Ref Mode** is GLOBAL - same for both VFOs
- **Ref Level Value** is per-VFO - A and B can have different values
- **A/B toggle** controls which VALUE is displayed and which VFO +/- targets
- **AUTO button** always sends `#AR/;` (no suffix)

**Changes:**

**RadioState:**
- Removed `m_autoRefLevelB` member variable
- Removed `autoRefLevelB()` getter
- Removed `autoRefLevelBChanged` signal
- Removed `#AR$` parsing block
- Updated comment for `#AR` to indicate GLOBAL scope

**DisplayPopupWidget:**
- Replaced `m_autoRefA` and `m_autoRefB` with single `m_autoRef` boolean
- Removed `setAutoRefLevelA()` and `setAutoRefLevelB()` slots
- Updated `setAutoRefLevel()` to set `m_autoRef` directly
- Updated `updateRefLevelControlGroup()` to use `m_autoRef` for both VFOs
- Updated auto button click handler to send `#AR/;` (no suffix)

**MainWindow:**
- Changed `setAutoRefLevelA` -> `setAutoRefLevel` connection
- Removed `autoRefLevelBChanged` connection

**Files Modified:**
- `src/models/radiostate.h` - Removed B auto-ref state
- `src/models/radiostate.cpp` - Removed #AR$ parsing
- `src/mainwindow.cpp` - Simplified connection
- `src/ui/displaypopupwidget.h` - Simplified to single m_autoRef
- `src/ui/displaypopupwidget.cpp` - Updated handlers for global auto-ref

---

### Ref Level A/B Support (Original - Partially Incorrect)

**Goal:** Each RX (A and B) has independent auto/manual ref level state and value. Display popup should track and display the correct state based on A/B toggle selection.

**Changes:**

**RadioState:**
- Added `m_autoRefLevelB` member variable and `autoRefLevelBChanged(bool)` signal
- Added parsing for `#AR$` commands (Sub RX auto-ref) - parsed before `#AR` (longer prefix first)
- `autoRefLevelB()` getter returns true if auto mode enabled for Sub RX

**DisplayPopupWidget:**
- Added A/B ref level state tracking: `m_refLevelA`, `m_refLevelB`, `m_autoRefA`, `m_autoRefB`
- Added slots: `setRefLevelValueA()`, `setRefLevelValueB()`, `setAutoRefLevelA()`, `setAutoRefLevelB()`
- Added `updateRefLevelControlGroup()` helper to sync display with A/B selection
- Ref level control group now:
  - Displays value for selected VFO (A or B based on toggle)
  - Shows faded grey text when in auto mode
  - AUTO button toggles sends correct CAT command (#AR/ or #AR$/) based on A/B
  - +/- buttons send correct CAT command (#REF+/- or #REF$+/-) based on A/B
- A/B toggle click handlers now call `updateRefLevelControlGroup()`
- PAN mode auto-sync now calls `updateRefLevelControlGroup()` to keep in sync

**ControlGroupWidget:**
- Added `setValueFaded(bool)` method and `m_valueFaded` member
- `paintEvent()` draws value text in grey when faded (visual indication of auto mode)

**MainWindow:**
- Connected `autoRefLevelChanged` -> `setAutoRefLevelA` (renamed)
- Connected `autoRefLevelBChanged` -> `setAutoRefLevelB` (new)
- Connected `refLevelChanged` -> `setRefLevelValueA` (renamed)
- Connected `refLevelBChanged` -> `setRefLevelValueB` (new)

**Optimistic Updates:**
K4 doesn't echo #AR commands, so auto toggle uses optimistic local update pattern (like PAN mode).

**Files Modified:**
- `src/models/radiostate.h` - Added autoRefLevelB signal, m_autoRefLevelB member
- `src/models/radiostate.cpp` - Added #AR$ parsing before #AR
- `src/ui/displaypopupwidget.h` - Added A/B ref state, setValueFaded, updateRefLevelControlGroup
- `src/ui/displaypopupwidget.cpp` - Implemented A/B ref level tracking, faded value support
- `src/mainwindow.cpp` - Connected B ref level signals

**CAT Commands:**
- `#AR;` / `#AR$;` - Query auto-ref state (A/B)
- `#ARA;` / `#AR$A;` - Set auto mode (A/B)
- `#ARM;` / `#AR$M;` - Set manual mode (A/B)
- `#AR/;` / `#AR$/;` - Toggle auto/manual (A/B)
- `#REF+;` / `#REF$+;` - Increment ref level (A/B)
- `#REF-;` / `#REF$-;` - Decrement ref level (A/B)

---

### Cleanup - Debug Logging Removal

Removed debug logging from previous PAN mode fixes:
- Removed `qDebug()` calls from `#DPM` and `#HDPM` parsing
- Removed `qDebug()` calls from `#SPN` and `#SPN$` parsing
- Removed `qDebug()` calls from `setDualPanModeLcd()` state setter

---

## December 30, 2025

### Display Popup Menu Fixes - AVERAGE/NB Control Groups and Button Init

**Issues Fixed:**

1. **PAN button face stuck on "PAN = A"**: Button wasn't updating when radio changed dual panadapter mode
   - Root cause: `updateMenuButtonLabels()` wasn't called during `setupUi()`, and RadioState initial values matched defaults so signals didn't fire
   - Fix: Added `updateMenuButtonLabels()` call to `setupUi()` and changed RadioState display state initial values to -1

2. **Missing AVERAGE control group**: When AVERAGE menu item selected, no control was shown
   - Added `m_averageControlPage` with ControlGroupWidget showing current averaging value and +/- buttons
   - Connected to `#AVG` CAT commands with step values: 1, 5, 10, 15, 20

3. **Missing NB control group**: When NB menu item selected, no control was shown
   - Added `m_nbControlPage` with ControlGroupWidget showing DDC NB mode (OFF/ON/AUTO) and level (0-14)
   - Connected to `#NB$` and `#NBL$` CAT commands (DDC Noise Blanker, not radio NB)

**Files Modified:**
- `src/ui/displaypopupwidget.h` - Added control pages, DDC NB state, setters, signals
- `src/ui/displaypopupwidget.cpp` - Implemented createAverageControlPage(), createNbControlPage(), updateNbControlGroupValue()
- `src/models/radiostate.h` - Changed display state inits to -1, added ddcNbMode/ddcNbLevel
- `src/models/radiostate.cpp` - Added #NB$ and #NBL$ parsing
- `src/mainwindow.cpp` - Wired averaging/NB control signals, added DDC NB state queries

**Key CAT Commands:**
- `#AVG01;` to `#AVG20;` - Set averaging (1-20)
- `#NB$0/1/2;` - DDC NB mode (OFF/ON/AUTO)
- `#NBL$00;` to `#NBL$14;` - DDC NB level (0-14)

---

### Display Popup Menu CAT Command Wiring - COMPLETED

**Goal:** Wire up the DisplayPopupWidget menu items to send K4 CAT commands and update button labels based on radio state.

**Changes:**

**DisplayPopupWidget:**
- Added state tracking variables for all display settings (DPM, DSM, WFC, AVG, PKM, FXT, FXA, FRZ, VFA, VFB)
- Added `catCommandRequested(QString)` signal to send CAT commands via MainWindow
- Added setter slots to receive state updates from RadioState
- Added `updateMenuButtonLabels()` to dynamically update button face text
- Added `setPrimaryText()`/`setAlternateText()` to DisplayMenuButton
- Implemented `onMenuItemClicked()` to emit CAT commands with LCD/EXT prefix logic
- Implemented `onMenuItemRightClicked()` for alternate actions (right-click)

**Menu Item -> CAT Command Mapping:**
| Item | Primary Click | Right-Click |
|------|--------------|-------------|
| PanWaterfall | `#DPM` cycle (A→B→Dual) | `#DSM` toggle (spectrum/waterfall) |
| NbWtrClrs | `NB;` toggle | `#WFC` cycle (5 colors) |
| RefLvlScale | Show REF control | `#AR/;` toggle auto-ref |
| SpanCenter | Show SPAN control | `FC;` / `FC$;` center on VFO |
| AveragePeak | `#AVG` cycle (1,5,10,15,20) | `#PKM/;` toggle peak |
| FixedFreeze | `#FXT`+`#FXA` cycle (6 modes) | `#FRZ` toggle freeze |
| CursAB | `#VFA/;` toggle | `#VFB/;` toggle |

**RadioState:**
- Added display state variables and signals (dualPanModeChanged, displayModeChanged, etc.)
- Added parsing for display CAT commands (#DPM, #DSM, #WFC, #AVG, #PKM, #FXT, #FXA, #FRZ, #VFA, #VFB, #AR)

**MainWindow:**
- Connected DisplayPopupWidget::catCommandRequested to TcpClient::sendCAT
- Connected RadioState display signals to DisplayPopupWidget setters
- Added display state queries in onAuthenticated() after connection

**Fixed Tune 6-State Cycle:**
- TRACK (FXT=0)
- SLIDE1 (FXT=1, FXA=0)
- SLIDE2 (FXT=1, FXA=4)
- FIXED1 (FXT=1, FXA=1)
- FIXED2 (FXT=1, FXA=2)
- STATIC (FXT=1, FXA=3)

**Files Modified:**
- `src/ui/displaypopupwidget.h` - Added state variables, setters, signals
- `src/ui/displaypopupwidget.cpp` - Implemented click handlers, button label updates
- `src/models/radiostate.h` - Added display state variables and signals
- `src/models/radiostate.cpp` - Added display CAT command parsing
- `src/mainwindow.cpp` - Wired signal connections, added state queries

---

### Passband Display Simplification - COMPLETED

**Goal:** Cleaner passband visualization with single center frequency marker.

**Changes:**
- Removed passband edge lines from both main panadapter and Mini-Pan
- Passband now shows only semi-transparent fill (indicating filter width)
- Added `drawFrequencyMarker()` to MiniPanWidget for center frequency line
- Mini-Pan frequency marker uses darker shade of passband color (blue for A, green for B)

**Files Modified:**
- `src/dsp/panadapter.cpp` - removed edge line drawing from `drawFilterPassband()`
- `src/dsp/minipanwidget.h` - added `drawFrequencyMarker()` declaration
- `src/dsp/minipanwidget.cpp` - removed edge lines, added frequency marker implementation

---

### Dual Panadapter Frequency Offset Fix - COMPLETED

**Bug:** In Dual mode (both panadapters visible), signals appeared offset from the filter/passband indicator. MainOnly mode worked correctly.

**Root Cause:** Bug in `upsampleSpectrum()` function that bypassed resampling when input bins exceeded widget width.

```cpp
// BUG: This was incorrect!
if (input.size() >= targetWidth) {
    output = input;  // Copies ALL elements, doesn't resize to targetWidth
    return;
}
```

**Why This Caused the Offset:**
| Mode | Widget Width | K4 Bins | Behavior | Result |
|------|-------------|---------|----------|--------|
| MainOnly | ~1200 px | ~1000 | Upsamples to 1200 | ✅ Correct |
| Dual | ~600 px | ~1000 | Copied all 1000 bins | ❌ Only left 60% displayed |

The draw loop only read bins 0-599, but passband assumed full frequency range.

**Fix** (`src/dsp/panadapter.cpp`):
- Removed the early-return that skipped resampling
- Function now always resamples to `targetWidth` (handles both up/downsampling)
- The existing interpolation code already worked correctly for both cases

---

### Mini-PAN RX Byte Discovery & Styling - COMPLETED

**Bug:** Both Mini-Pans displayed identical data (VFO A spectrum on both).

**Root Cause:** The K4-Remote Protocol PDF (rev. A1) incorrectly documented Mini-PAN as having no RX byte. Through packet analysis, we discovered Mini-PAN packets **DO** contain an RX byte at position 4 (undocumented).

**Packet Analysis (via debug logging):**
```
PKT TYPE= 3 Bytes[0-7]: "03 00 f4 00 00 00 04 00"  ← byte4=00 (Main)
PKT TYPE= 3 Bytes[0-7]: "03 00 f5 00 01 00 04 00"  ← byte4=01 (Sub)
```

**Actual Mini-PAN Structure (corrected):**
| Byte | Description |
|------|-------------|
| 0 | TYPE (0x03) |
| 1 | VER (0x00) |
| 2 | SEQ (sequence) |
| 3 | Reserved (0x00) |
| 4 | **RX (0=Main, 1=Sub)** |
| 5+ | Mini PAN Data |

**Protocol Changes** (`src/network/protocol.cpp`):
- Read receiver from `payload[4]` instead of hardcoding 0
- Data starts at `payload.mid(5)` instead of `payload.mid(3)`

**MainWindow Changes** (`src/mainwindow.cpp`):
- Route Mini-PAN based on `receiver` parameter (0→VFO A, 1→VFO B)
- Mini-Pan A: Blue spectrum (0, 128, 255) and blue passband
- Mini-Pan B: Green spectrum (0, 200, 0) and green passband
- Colors match main panadapter scheme for visual consistency

**MiniPanWidget Changes** (`src/dsp/minipanwidget.h/.cpp`):
- Reduced spectrum line thickness from 1.5 to 1.0 for cleaner display
- Added `setPassbandColor(QColor)` method for customizable passband color
- Added `m_passbandColor` member variable (default: blue)
- Updated `drawFilterPassband()` to use member variable instead of hardcoded green

---

### Mini-Pan for VFO B / Sub RX - COMPLETED

Implemented Mini-Pan display for VFO B (Sub RX), mirroring VFO A functionality.

**RadioState Changes** (`src/models/radiostate.h/.cpp`):
- Added `m_miniPanAEnabled`, `m_miniPanBEnabled` state tracking
- Added `miniPanAEnabled()`, `miniPanBEnabled()` getters
- Added `setMiniPanAEnabled(bool)`, `setMiniPanBEnabled(bool)` setters (optimistic updates)
- Added signals `miniPanAEnabledChanged`, `miniPanBEnabledChanged`

**MainWindow Changes** (`src/mainwindow.cpp`):
- VFO A toggle sends `#MP1;` / `#MP0;` CAT commands
- VFO B toggle sends `#MP$1;` / `#MP$0;` CAT commands
- Set optimistic state BEFORE sending CAT (K4 doesn't echo these commands)
- Connected `modeBChanged` and `filterBandwidthBChanged` to VFO B Mini-Pan

**MiniPanWidget Changes** (`src/dsp/minipanwidget.h/.cpp`):
- Added `setSpectrumColor(QColor)` method for customizable spectrum line color
- Added `m_spectrumColor` member variable (default: amber for VFO A)

**CAT Commands:**
| Function | Main RX | Sub RX |
|----------|---------|--------|
| Enable Mini-Pan | `#MP1;` | `#MP$1;` |
| Disable Mini-Pan | `#MP0;` | `#MP$0;` |

**Color Scheme:**
- VFO A Mini-Pan: Cyan `K4Styles::Colors::VfoACyan`
- VFO B Mini-Pan: Green `K4Styles::Colors::VfoBGreen`

---

### Sub RX Main Panadapter - COMPLETED

Implemented full Sub RX (VFO B) main panadapter with side-by-side layout.

**Features Implemented:**
- Three display modes: MainOnly, Dual, SubOnly (via `setPanadapterMode()`)
- Side-by-side layout using QHBoxLayout (A on left, B on right)
- Independent C/+/- span control buttons for each panadapter
- Click-to-tune and scroll-wheel tuning for VFO B
- VFO-specific passband and frequency marker colors

**RadioState Changes** (`src/models/radiostate.h/.cpp`):
- Added `refLevelB()`, `spanHzB()`, `setSpanHzB()` for Sub RX state
- Added signals `refLevelBChanged`, `spanBChanged`
- Added CAT parsing for `#REF$` and `#SPN$` commands (Sub RX variants)

**MainWindow Changes** (`src/mainwindow.cpp`):
- Changed spectrum layout from QVBoxLayout to QHBoxLayout
- Added `PanadapterMode` enum (MainOnly, Dual, SubOnly)
- Added `setPanadapterMode()` method for display mode switching
- Created span control buttons for panadapter B (`m_spanDownBtnB`, `m_spanUpBtnB`, `m_centerBtnB`)
- Wired SUB RX button to cycle through panadapter modes
- Added VFO B mouse controls: `FB{freq};` for click, `UP$;`/`DN$;` for scroll

**PanadapterWidget Changes** (`src/dsp/panadapter.h/.cpp`):
- Added `setPassbandColor()` and `setFrequencyMarkerColor()` methods
- Added `m_passbandColor` and `m_frequencyMarkerColor` member variables
- Updated `drawFilterPassband()` to use `m_passbandColor`
- Updated `drawFrequencyMarker()` to use `m_frequencyMarkerColor`
- Default colors: Blue passband `(0, 128, 255)`, darker blue marker `(0, 80, 200)`

**Color Scheme (per physical K4 rig):**
- VFO A: Blue passband, darker blue frequency marker (defaults)
- VFO B: Green passband `(0, 200, 0)`, darker green marker `(0, 140, 0)`

**CAT Commands:**
| Function | VFO A | VFO B |
|----------|-------|-------|
| Span | `#SPN{value};` | `#SPN${value};` |
| Center | `FC;` | `FC$;` |
| Ref Level | `#REF` response | `#REF$` response |
| Tune Up | `UP;` | `UP$;` |
| Tune Down | `DN;` | `DN$;` |
| Set Freq | `FA{freq};` | `FB{freq};` |

---

## December 29, 2025

### macOS Release Packaging - COMPLETED

Fixed macOS release bundle to run standalone without Homebrew Qt installed.

**Problem:** `macdeployqt` doesn't bundle all transitive dependencies. App crashed with:
- "Library not loaded: QtDBus.framework" (needed by QtGui)
- "Library not loaded: libbrotlicommon.1.dylib" (needed by libbrotlidec)
- "Library not loaded: libdbus-1.3.dylib" (needed by QtDBus)

**Root Causes:**
1. Homebrew stores frameworks as symlinks; `cp -R` and `cp -RL` failed silently
2. `macdeployqt` misses transitive dependencies not directly linked by the app

**Solution** (`.github/workflows/release.yml`):

```bash
# 1. Use ditto instead of cp (handles macOS frameworks correctly)
ditto "$QTDBUS_REAL" K4Controller.app/Contents/Frameworks/QtDBus.framework

# 2. Copy missing transitive dependencies
for lib in libbrotlicommon.1.dylib libbrotlidec.1.dylib libbrotlienc.1.dylib; do
  ditto "$(brew --prefix)/lib/$lib" "K4Controller.app/Contents/Frameworks/$lib"
done
ditto "$DBUS_LIB" "K4Controller.app/Contents/Frameworks/libdbus-1.3.dylib"

# 3. Fix permissions (Homebrew files are read-only)
chmod -R u+rw K4Controller.app/Contents/Frameworks/

# 4. Rewrite library paths to use @rpath
install_name_tool -change "$(brew --prefix)/lib/libbrotlicommon.1.dylib" \
  "@rpath/libbrotlicommon.1.dylib" "$FRAMEWORKS/libbrotlidec.1.dylib"
install_name_tool -change "$(brew --prefix)/opt/dbus/lib/libdbus-1.3.dylib" \
  "@rpath/libdbus-1.3.dylib" "$FRAMEWORKS/QtDBus.framework/Versions/A/QtDBus"

# 5. Re-sign after modifications
codesign --force --deep --sign - K4Controller.app
```

**Key Learnings:**
- `ditto` is the macOS-native tool for copying frameworks (preserves structure)
- `readlink -f` resolves symlink chains to actual paths
- `install_name_tool -change` rewrites hardcoded library paths to @rpath
- Ad-hoc code signing (`codesign --sign -`) required after modifying binaries

**Versions:** v0.1.0-alpha.104 through v0.1.0-alpha.108

---

### GitHub CI/CD and Project Infrastructure - COMPLETED

Set up professional GitHub project infrastructure for cross-platform builds.

**Changes:**

1. **Created .gitignore** - Excludes build/, IDE files, Qt generated files, Windows/macOS artifacts

2. **Removed tracked build artifacts** - Cleaned 1,456 files (225K lines) that were accidentally committed

3. **GitHub Actions Workflows:**
   - `build-macos.yml` - macOS 14 Apple Silicon builds
   - `build-windows.yml` - Windows 11 x64 builds via vcpkg
   - `lint.yml` - clang-format code style checking

4. **Cross-platform CMakeLists.txt** - Added WIN32 platform detection:
   ```cmake
   elseif(WIN32)
       find_package(Opus CONFIG REQUIRED)
       find_package(hidapi CONFIG REQUIRED)
       set(OPUS_LIBRARIES Opus::opus)
       set(HIDAPI_LIBRARIES hidapi::hidapi)
   ```

5. **Code Formatting** - Standardized all 47 source files with clang-format (LLVM style, 4-space indent, 120 char lines)

6. **Documentation:**
   - Created `CHANGELOG.md` for user-facing release notes
   - Added README badges for build status and linting

**Files Created:**
- `.gitignore`
- `.clang-format`
- `.github/workflows/build-macos.yml`
- `.github/workflows/build-windows.yml`
- `.github/workflows/lint.yml`
- `CHANGELOG.md`

**Files Modified:**
- `CMakeLists.txt` - Added Windows/Linux platform handling
- `README.md` - Added badges, cross-platform instructions
- All 47 `.cpp/.h` files - Reformatted with clang-format

---

### Removed Legacy DSP Files - COMPLETED

Removed unused legacy spectrum and waterfall widgets that were replaced by PanadapterWidget.

**Files Deleted:**
- `src/dsp/spectrum.cpp` / `spectrum.h`
- `src/dsp/waterfall.cpp` / `waterfall.h`

**Reason:** These were marked as "LEGACY (unused)" in documentation. All spectrum/waterfall functionality now handled by `src/dsp/panadapter.cpp`.

---

### Center Button Added to Panadapter - COMPLETED

Added a "C" (center) button to the panadapter control buttons, forming a triangle layout with the existing +/- span buttons.

**Layout:**
```
        [C]        ← center button
      /     \
   [-]       [+]   ← span buttons
```

**Functionality:**
- Click C button sends `FC;` CAT command to center VFO on current frequency
- Same styling as +/- buttons (semi-transparent, 28x24px)
- Dynamically repositions on window resize

**Files Modified:**
- `src/mainwindow.h` - Added `m_centerBtn` member variable
- `src/mainwindow.cpp` - Created button, wired signal, added resize handling

---

## December 28, 2025

### MiniPan Mode-Dependent Bandwidth - COMPLETED

Fixed MiniPan bandwidth to vary by mode, matching K4 radio behavior.

**Problem:** MiniPan assumed fixed 3 kHz bandwidth, but K4 sends different spans:
- CW/CW-R: ±1.5 kHz (3 kHz total)
- USB/LSB/DATA/DATA-R/AM/FM: ±5 kHz (10 kHz total)

This caused notch filter marker to appear in wrong position for voice/data modes.

**Solution:**
- Added `m_bandwidthHz` member variable (defaults to 10 kHz for voice modes)
- Added `bandwidthForMode()` helper that returns 3000 for CW, 10000 otherwise
- Updated `setMode()` to recalculate bandwidth when mode changes
- Updated `drawNotchFilter()` to use dynamic bandwidth instead of hardcoded constant

**Files Modified:**
- `src/dsp/minipanwidget.h` - Added `m_bandwidthHz` member and `bandwidthForMode()` declaration
- `src/dsp/minipanwidget.cpp` - Implemented `bandwidthForMode()`, updated `setMode()` and `drawNotchFilter()`

---

### Removed Verbose Debug Logging - COMPLETED

Removed high-frequency debug logging to reduce CPU overhead.

**Logs Removed:**
- `CAT response:` - was logged for every CAT response (~50+/sec)
- `sendCAT called:` - was logged for every command sent
- `Audio packet received:` - was logged every 100th packet
- `OpusDecoder: K4 packet decoded` - was logged every 100th decode
- `PAN Type 2 Validation` - was logged for first 5 spectrum packets

**Files Modified:**
- `src/network/tcpclient.cpp` - Removed sendCAT and CAT response logging
- `src/network/protocol.cpp` - Removed audio packet and PAN validation logging
- `src/audio/opusdecoder.cpp` - Removed decode status logging

**Kept:** Error/warning logs (socket errors, auth failures, decode failures)

---

### Manual Notch Filter Visualization - COMPLETED

Added visual representation of the manual notch filter in panadapter and mini-pan displays.

**CAT Commands Parsed:**
- `NAn;` - Auto Notch (tracked for state, no visual)
- `NMnnnnm;` - Manual Notch: nnnn=pitch (150-5000 Hz), m=0/1

**Visual:**
- Red 2px vertical line at notch frequency position
- Mode-aware positioning: USB/CW/DATA adds pitch, LSB subtracts pitch
- Visible in both spectrum and waterfall areas

**Files Modified:**
- `src/models/radiostate.h/.cpp` - Added `m_autoNotchEnabled`, `m_manualNotchEnabled`, `m_manualNotchPitch`, `notchChanged()` signal
- `src/dsp/panadapter.h/.cpp` - Added `setNotchFilter()`, `drawNotchFilter()`
- `src/dsp/minipanwidget.h/.cpp` - Added `setNotchFilter()`, `setMode()`, `drawNotchFilter()`
- `src/mainwindow.cpp` - Wired `RadioState::notchChanged` to panadapter/mini-pan

---

### NTCH Indicator in VFO Processing Row - COMPLETED

Added NTCH indicator to the processing indicators row (after NR) showing notch filter state.

**Display States:**
| State | Label | Color |
|-------|-------|-------|
| Neither enabled | `NTCH` | Grey #999999 |
| Auto only | `NTCH-A` | White #FFFFFF |
| Manual only | `NTCH-M` | White #FFFFFF |
| Both enabled | `NTCH-A/M` | White #FFFFFF |

**Files Modified:**
- `src/ui/vfowidget.h` - Added `m_ntchLabel`, `setNotch(bool autoEnabled, bool manualEnabled)`
- `src/ui/vfowidget.cpp` - Added NTCH label creation in `setupUi()`, added `setNotch()` implementation
- `src/mainwindow.cpp` - Wired `RadioState::notchChanged` to `m_vfoA->setNotch()`

---

### Panadapter Connection Guard - COMPLETED

Fixed CAT commands being sent when app is disconnected from radio.

**Problem**: Clicking or scrolling on the panadapter while disconnected would attempt to send CAT commands:
```
sendCAT called: "FA-0000003129;" state= TcpClient::Disconnected
CAT command DROPPED - not connected
```

**Root Cause**:
- `frequencyClicked` and `frequencyScrolled` handlers sent CAT without checking connection state
- `xToFreq()` returned invalid (negative) frequencies when `m_centerFreq = 0`

**Solution**: Added guards in MainWindow panadapter signal handlers.

**src/mainwindow.cpp** (lines 722-743)
```cpp
// frequencyClicked handler
if (!m_tcpClient->isConnected() || freq <= 0) return;

// frequencyScrolled handler
if (!m_tcpClient->isConnected()) return;
```

---

### TX Function Buttons Moved to Top - COMPLETED

Moved the 2×3 TX function button grid to the top of the left side panel.

**New Layout Order:**
1. TX Function Buttons (TUNE, XMIT, ATU, VOX, ANT, RX ANT)
2. WPM/PTCH, PWR/DLY
3. BW/HI, SHFT/LO
4. M.RF/M.SQL, S.SQL/S.RF
5. Status Area + Icon Buttons

**src/ui/sidecontrolpanel.cpp** - Reordered `setupUi()` to place TX grid first.

---

### MiniPan Edge Trimming - COMPLETED

Removed blank/empty areas on left and right edges of MiniPanWidget spectrum display.

**Problem**: The MiniPAN data from K4 (~1033 bins for 3kHz) contains padding/empty bins at the edges, causing visible blank areas in the spectrum and waterfall display.

**Solution**: Increased the number of bins skipped at each edge during data processing.

**src/dsp/minipanwidget.cpp** (lines 85-87)
```cpp
// Skip bins at edges to remove blank areas (~7.5% each side of ~1033 bins)
const int SKIP_START = 75;
const int SKIP_END = 75;
```

| Parameter | Before | After |
|-----------|--------|-------|
| SKIP_START | 5 | 75 |
| SKIP_END | 10 | 75 |
| Total trimmed | ~1.5% | ~15% |

---

### KPA1500 Amplifier Integration - COMPLETED

Added TCP client for KPA1500 amplifier communication with configurable polling.

#### Files Created

**src/network/kpa1500client.h/.cpp** (NEW)
- TCP client class for KPA1500 amplifier
- Polling timer with configurable interval (100-5000ms, default 800ms)
- Parses KPA1500 CAT command responses
- Signals for state changes (power, SWR, temperature, fault status, etc.)

**Poll Commands:**
```
^BN;^WS;^TM;^FS;^OS;^VI;^FC;^ON;^FL;^AN;^IP;^SN;^PC;^VM1;^VM2;^VM3;^VM5;^LR;^CR;^PWF;^PWR;^PWD;
```

**Parsed Responses:**
| Command | Description | Parsing |
|---------|-------------|---------|
| ^BN | Band Name | String (e.g., "20M") |
| ^WS | SWR | Integer / 10 (e.g., 15 → 1.5:1) |
| ^TM | Temperature | Celsius (double) |
| ^FS | Fault Status | 0=None, 1=Active, 2=History |
| ^ON | Operating State | 0=Standby, 1=Operate |
| ^PWF | Forward Power | Watts (double) |
| ^PWR | Reflected Power | Watts (double) |
| ^PWD | Drive Power | Watts (double) |
| ^VM1 | PA Voltage | millivolts → volts |
| ^VM2 | PA Current | milliamps → amps |
| ^AN | ATU Status | xy (x=present, y=active) |
| ^SN | Serial Number | String |
| ^VI | Firmware Version | String |

#### Files Modified

**src/settings/radiosettings.h/.cpp**
- Added `kpa1500PollInterval()` / `setKpa1500PollInterval(int)` - polling interval in ms
- Added `kpa1500PollIntervalChanged(int)` signal
- Persisted to QSettings under `kpa1500/pollInterval`

**src/ui/optionsdialog.cpp**
- Added polling interval input field to KPA1500 page (below Port)
- Validates range 100-5000ms
- Saves to RadioSettings on edit finished

**src/mainwindow.h/.cpp**
- Added `m_kpa1500Client` member
- Added slots: `onKpa1500Connected()`, `onKpa1500Disconnected()`, `onKpa1500Error()`,
  `onKpa1500EnabledChanged()`, `onKpa1500SettingsChanged()`
- Auto-connects when enabled and host configured
- Starts polling on connect with configured interval

---

### KPA1500 Connection Status Indicator - COMPLETED

Added status indicator to Options > KPA1500 page showing connection state.

**src/ui/optionsdialog.h**
- Added `KPA1500Client*` constructor parameter
- Added `m_kpa1500StatusLabel` member
- Added `updateKpa1500Status()` helper
- Added `onKpa1500ConnectionStateChanged()` slot

**src/ui/optionsdialog.cpp**
- Constructor connects to KPA1500Client signals for live status updates
- Status indicator shows:
  - "Connected" (green `#00FF00`) when TCP connection active
  - "Not Connected" (red `#FF6666`) when disconnected
- Updates automatically when connection state changes

**src/mainwindow.cpp**
- Updated OptionsDialog constructor call to pass `m_kpa1500Client`

---

### Header Connection Status Enhancement - COMPLETED

Modified header status bar to show K4 and KPA1500 connection status separately.

**Changes:**
1. Renamed "Connected" → "K4 Connected" for all K4 connection states
2. Added KPA1500 status label to the left of K4 status
3. KPA1500 status shows:
   - "KPA1500 Connected" (green) when enabled and connected
   - "KPA1500 Not Connected" (red) when enabled but disconnected
   - Hidden when KPA1500 is disabled in settings

**src/mainwindow.h**
- Added `QLabel *m_kpa1500StatusLabel` member
- Added `void updateKpa1500Status()` slot

**src/mainwindow.cpp**
- Added KPA1500 status label in `setupTopStatusBar()` (before K4 status)
- Updated `updateConnectionState()` to use "K4" prefix for all states
- Added `updateKpa1500Status()` method to manage KPA1500 status display
- Updated KPA1500 slots to call `updateKpa1500Status()` on state changes
- Initial status update after client creation

**Header Layout:**
```
[Title] [DateTime] [stretch] [Power] [SWR] [Voltage] [Current] [stretch] [KPA1500 Status] [K4 Status]
```

---

### Right Side Panel - COMPLETED

Added right-hand vertical panel matching left SideControlPanel dimensions and styling.

**src/ui/rightsidepanel.h/.cpp** (NEW)
- Fixed width: 105px (matches left panel)
- Margins: 6, 8, 6, 8
- Spacing: 4
- Empty shell ready for content population
- Exposes `contentLayout()` for adding widgets

**src/mainwindow.h/.cpp**
- Added `RightSidePanel *m_rightSidePanel` member
- Added to middleLayout after content area (left panel | content | right panel)

**Layout:**
```
┌─────────┬──────────────────────────────┬─────────┐
│  Left   │        Main Content          │  Right  │
│  Panel  │  (VFO section + Spectrum)    │  Panel  │
│  105px  │         (stretch)            │  105px  │
└─────────┴──────────────────────────────┴─────────┘
```

---

### VFO Mode Label Centering Fix - COMPLETED

Fixed mode labels ("CW", "DATA", etc.) not properly centered under A/B VFO squares.

**Root Cause**: QVBoxLayout doesn't have its own fixed width - it relies on parent layout allocation. When added to txRow with stretches, the column width varied, causing misalignment between the 30px square and 45px mode label.

**Solution**: Wrapped each VFO indicator column in a QWidget container with fixed 45px width.

**src/mainwindow.cpp** (lines 408-475)
```cpp
// Before: QVBoxLayout added directly to txRow
auto *vfoAColumn = new QVBoxLayout();
txRow->addLayout(vfoAColumn);

// After: QVBoxLayout inside fixed-width container
auto *vfoAContainer = new QWidget(centerWidget);
vfoAContainer->setFixedWidth(45);  // Match mode label width
auto *vfoAColumn = new QVBoxLayout(vfoAContainer);
vfoAColumn->setContentsMargins(0, 0, 0, 0);
// ... widgets added with Qt::AlignHCenter ...
txRow->addWidget(vfoAContainer);
```

Same pattern applied to VFO B column.

---

### VFO Center Section Layout Adjustments - COMPLETED

Adjusted center widget and spacing for better fit.

**src/mainwindow.cpp**
| Location | Before | After | Purpose |
|----------|--------|-------|---------|
| Line 396 | `setFixedWidth(180)` | `setFixedWidth(200)` | More room for content |
| Line 421 | `setFixedWidth(50)` | `setFixedWidth(45)` | Mode A label width |
| Line 428 | `addSpacing(30)` | `addSpacing(15)` | Reduced left spacing |
| Line 450 | `addSpacing(30)` | `addSpacing(15)` | Reduced right spacing |
| Line 466 | `setFixedWidth(50)` | `setFixedWidth(45)` | Mode B label width |

---

### TX Function Buttons (2×3 Grid) - COMPLETED

Added 6 TX function buttons to side control panel between DualControlButtons and status area.

**Layout**: 2 columns × 3 rows
| Row | Col 0 | Col 1 |
|-----|-------|-------|
| 0 | TUNE (TUNE LP) | XMIT (TEST) |
| 1 | ATU (ATU TUNE) | VOX (QSK) |
| 2 | ANT (REM ANT) | RX ANT (SUB ANT) |

**src/ui/sidecontrolpanel.h**
- Added `createTxFunctionButton()` helper method
- Added member pointers: `m_tuneBtn`, `m_xmitBtn`, `m_atuTuneBtn`, `m_voxBtn`, `m_antBtn`, `m_rxAntBtn`

**src/ui/sidecontrolpanel.cpp**
- Added QGridLayout with 4px horizontal, 8px vertical spacing
- Each button: 28px height, gradient background matching bottom menu bar style
- Orange sub-label below each button (8px font, 12px fixed height)

---

### Spectrum Gradient Enhancement - COMPLETED

Enhanced panadapter spectrum fill gradient for deeper green at base.

**src/dsp/panadapter.cpp** (lines 484-490)
```cpp
// Gradient fill: bright lime at top, richer green at base
QLinearGradient gradient(0, y0, 0, y0 + h);
gradient.setColorAt(0.0, QColor(140, 255, 140, 230));  // Bright lime at top
gradient.setColorAt(0.4, QColor(60, 220, 60, 200));    // Strong green
gradient.setColorAt(0.7, QColor(30, 180, 30, 170));    // Medium green
gradient.setColorAt(1.0, QColor(0, 140, 0, 150));      // Deep green base
```

---

### KPA1500 Settings Tab - COMPLETED

Added KPA1500 configuration tab to Options dialog.

**src/settings/radiosettings.h/.cpp**
- Added settings: `kpa1500Host`, `kpa1500Port`, `kpa1500Enabled`
- Default values: host="", port=1500, enabled=false

**src/ui/optionsdialog.cpp**
- Added "KPA1500" tab with:
  - IP Address field
  - Port field (default 4626)
  - Enable checkbox

---

## December 22, 2025

### Options Dialog - COMPLETED

Added File > Options dialog with vertical tab navigation and About tab.

#### Files Created/Modified

**src/ui/optionsdialog.cpp/.h** (NEW)
- Vertical tab list on left, stacked pages on right
- About tab displaying connected radio information:
  - Radio ID (from RDY response)
  - Radio Model (from RDY response)
  - Installed Options (decoded from OM string)
  - Software Versions (from firmware info)

**OM String Decoding:**
| Position | Character | Option |
|----------|-----------|--------|
| 0 | A | KAT4 (ATU) |
| 1 | P | KPA4 (PA) |
| 2 | X | XVTR |
| 3 | S | KRX4 (Sub RX) |
| 4 | H | KHDR4 (HDR) |
| 5 | M | K40 (Mini) |
| 6 | L | Linear Amp |
| 7 | 1 | KPA1500 |

**src/mainwindow.cpp**
- Added File menu with Options action
- Connected to showOptionsDialog() slot

---

### Sub RX State (S.RF/S.SQL) - COMPLETED

Added Sub RX RF Gain and Squelch state tracking.

#### Changes Made

**src/models/radiostate.h**
- Added `m_rfGainB` and `m_squelchLevelB` members
- Added `rfGainBChanged(int)` and `squelchBChanged(int)` signals
- Added getter methods: `rfGainB()`, `squelchLevelB()`

**src/models/radiostate.cpp**
- Parse `RG$` command (Sub RX RF Gain) - MUST check before `RG`
- Parse `SQ$` command (Sub RX Squelch) - MUST check before `SQ`

**src/network/tcpclient.cpp**
- Added init commands: `RG$;` and `SQ$;`

**src/mainwindow.cpp**
- Connected signals to SideControlPanel:
  ```cpp
  rfGainBChanged -> setSubRfGain
  squelchBChanged -> setSubSquelch
  ```

---

### VFO A/B Layout Spacing - COMPLETED

Moved VFO A/B indicators outward to make room for future filter UI elements.

**src/mainwindow.cpp** (TX Row layout)
- Added `txRow->addSpacing(30)` after first stretch (before left triangle)
- Added `txRow->addSpacing(30)` before second stretch (after right triangle)

Layout: `[A+Mode] [stretch] [30px] [◀] [TX] [▶] [30px] [stretch] [B+Mode]`

---

### RIT/XIT Separator Line - COMPLETED

Added visual separator between RIT/XIT labels and offset value.

**src/mainwindow.cpp** (RIT/XIT box)
```cpp
auto *ritXitSeparator = new QFrame(m_ritXitBox);
ritXitSeparator->setFrameShape(QFrame::HLine);
ritXitSeparator->setStyleSheet(QString("background-color: %1; border: none;").arg(K4Colors::InactiveGray));
ritXitSeparator->setFixedHeight(1);
```

---

## December 21, 2025

### Side Panel Value Population - COMPLETED

All side panel control values now populate from the radio on connect.

#### Changes Made

**src/network/tcpclient.cpp**
- Added CAT command requests on connect:
  - `KS;` - Keyer speed (WPM)
  - `PC;` - Power level
  - `SD;` - VOX/QSK delay
  - `SQ;` - Squelch
  - `RG;` - RF Gain

**src/models/radiostate.h**
- Added signals:
  - `keyerSpeedChanged(int wpm)`
  - `qskDelayChanged(int delay)`
  - `rfGainChanged(int gain)`
  - `squelchChanged(int level)`
- Added member variables: `m_keyerSpeed`, `m_qskDelay`
- Fixed initial values to ensure first signal emission:
  - `m_ifShift = -1` (was 50, matched IS0050)
  - `m_cwPitch = -1` (was 500, matched CW50)
  - `m_rfGain = -999` (was 0, matched RG-00)
  - `m_squelchLevel = -1` (was 0, matched SQ000)
  - `m_keyerSpeed = -1`
  - `m_qskDelay = -1`

**src/models/radiostate.cpp**
- Added KS parsing: `KS015` -> 15 WPM
- Added SD parsing: `SD0C005` -> delay 5 (×10 = 50ms)
  - Format: `SDxyzzz` where x=QSK flag, y=mode, zzz=delay
- Added signal emissions for RG and SQ

**src/mainwindow.cpp**
- Connected RadioState signals to SideControlPanel:
  ```cpp
  filterBandwidthChanged -> setBandwidth(bw / 1000.0)  // Hz to kHz
  ifShiftChanged -> setShift(shift / 100.0)           // 0-99 to decimal
  keyerSpeedChanged -> setWpm
  cwPitchChanged -> setPitch(pitch / 1000.0)          // Hz to kHz
  rfPowerChanged -> setPower
  qskDelayChanged -> setDelay(delay * 10)             // to ms
  rfGainChanged -> setMainRfGain
  squelchChanged -> setMainSquelch
  ```

**src/ui/sidecontrolpanel.h/.cpp**
- Changed `setPitch(int)` to `setPitch(double)` for decimal display
- Now formats with 2 decimal places like BW and SHFT

#### CAT Command Reference

| Control | CAT Cmd | Example | Parsing |
|---------|---------|---------|---------|
| BW | BW | BW0040 | 40 × 10 = 400Hz = 0.40 kHz |
| SHFT | IS | IS0050 | 50 / 100 = 0.50 |
| WPM | KS | KS015 | 15 WPM |
| PTCH | CW | CW50 | 50 × 10 = 500Hz = 0.50 kHz |
| PWR | PC | PC053H | 53W (H=high power) |
| DLY | SD | SD0C005 | 5 × 10 = 50ms |
| M.RF | RG | RG-00 | 0 (can be negative) |
| M.SQL | SQ | SQ000 | 0 |

---

## Next Steps / TODO

### Side Panel Actions (Not Yet Implemented)
Wire up tap/hold actions for each button to send CAT commands:

| Button | Tap Action | Hold Action |
|--------|------------|-------------|
| WPM/PTCH | Toggle primary/alt display | Open adjustment dialog |
| PWR/DLY | Toggle primary/alt display | Open adjustment dialog |
| BW/SHFT | Cycle filter bandwidth | Open filter settings |
| M.RF/M.SQL | Adjust RF gain | Adjust squelch |
| S.RF/S.SQL | Adjust sub RF gain | Adjust sub squelch |

### Bottom Menu Bar Actions (Not Yet Implemented)
- MENU: Open main menu
- Fn: Function key menu
- DISPLAY: Display settings
- BAND: Band selector
- MAIN RX: Main receiver settings
- SUB RX: Sub receiver settings
- TX: Transmit settings

### Future Features
- VFO B panadapter (created but hidden)
- TX meter display during transmit
- Audio input (microphone) for TX
- Memory channels
- CW keyer integration

---

*Last updated: January 2, 2026*
