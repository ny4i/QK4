# K4Controller Development Log

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
- `src/models/CLAUDE.md` - Updated documentation

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
- VFO A Mini-Pan: Amber `K4Colors::VfoAAmber`
- VFO B Mini-Pan: Cyan `K4Colors::VfoBCyan`

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

*Last updated: December 29, 2025*
