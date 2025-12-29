# K4Controller Development Log

## December 29, 2025

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
   - Strengthened documentation requirements in `CLAUDE.md`
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
- `CLAUDE.md` - Added mandatory documentation requirements
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
- `src/dsp/CLAUDE.md` - Updated documentation with mode-dependent bandwidth table

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

*Last updated: December 22, 2025*
