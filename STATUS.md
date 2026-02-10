# Status

Current implementation state of QK4 (January 2026).

## Implemented Features

### Core
- **TLS/PSK encrypted connection** on port 9204 (unencrypted on 9205)
- Dual VFO display (A/B) with frequency, mode, S-meter
- **GPU-accelerated panadapter** via Qt RHI (Metal/DirectX/Vulkan)
- Mini-Pan widget in VFO area (mode-dependent bandwidth: CW=3kHz, Voice/Data=10kHz)
- **Dual-channel audio mixing**: MAIN volume (left), SUB volume (right)
- Opus audio decoding and playback
- Server manager dialog with TLS checkbox and PSK field
- **HD fonts**: Inter with tabular figures for all UI and data displays

### VFO Display
- **Tuning rate indicator**: White underline below frequency digit
- **Direct frequency entry**: Click frequency to type new value (1-11 digits, Enter to submit, Escape to cancel)
- **Multifunction S/Po meter**: Shows S-meter when RX, power when TX (scales: K4 0-110W, QRP 0-10W)
- **TX meters**: ALC, COMP, SWR, Id (shown in combined meter widget)
- **Mode display**: Full mode with data sub-modes (AFSK, FSK, PSK, DATA)
- Processing indicators: AGC, PRE, ATT, NB, NR, NTCH
- Mini-pan toggle (click meter area to show spectrum)
- **Mini-pan B restriction**: Blocked when VFOs on different bands and SUB RX is off (auto-hides if conditions change)
- VOX indicator (mode-aware: VXC/VXV/VXD)
- **Clickable VFO squares**: Open mode popup targeting A or B
- **Memory buttons**: M1-M4, REC, STORE, RCL
- **SUB/DIV indicators**: LED-style indicators next to VFO B (DIV only lights when both SUB and DIV are enabled)
- **Filter indicator**: Visual bandwidth shape (triangle→trapezoid) with mode-aware shift positioning (VFO A=cyan, VFO B=green)

### Panadapter
- Spectrum styles: Blue gradient, BlueAmplitude (LUT-based)
- **Click-to-tune**: Single click tunes to frequency
- **Drag-to-tune**: Click, hold, and drag passband to change frequency in real-time
- **Scroll-wheel tuning**: Mouse wheel adjusts frequency using K4's native step
- Peak hold with decay
- Dynamic ref level and span from K4
- Filter passband and frequency marker overlays

### Control Panels
- **Left panel**: Mode-dependent (WPM/PTCH or MIC/CMP), PWR/DLY, BW/SHFT, M.RF/M.SQL, S.RF/S.SQL, TX buttons, MAIN/SUB volume
- **Right panel**: 5×2 button grid (PRE/ATTN, NB/LEVEL, NR/ADJ, etc.)
- **Bottom menu bar**: MENU, Fn, DISPLAY, BAND, MAIN RX, SUB RX, TX

### Popups & Dialogs
- **Band popup** (14 bands in 2 rows) - BSET-aware: targets VFO B when BSET enabled (BN$ commands)
- Mode popup (2×4 grid with sub-mode toggle)
- Menu overlay on panadapter
- Feature menu bar (ATTN/NB/NR/NOTCH)
- Display popup (ref level, span, tune mode, waterfall color)
- Options dialog (About, KPOD, KPA1500)
- **Fn popup**: Dual-action buttons for Fn.F1-F8 macros
- **Macro dialog**: Configure 29 macro slots (PF1-4, Fn.F1-F8, REM ANT, KPOD tap/hold)
- **Notification popup**: Center-screen auto-dismiss for K4 error/status messages (ERxx:)
- **MAIN RX / SUB RX popups**: 7 dual-line buttons with working controls:
  - AFX: Cycle audio effects (OFF/DELAY/PITCH)
  - AGC: Toggle speed (Fast/Slow), right-click toggles ON/OFF
  - APF: Cycle audio peak filter bandwidth (30/50/150 Hz, CW mode only)
  - VFO LINK: Right-click on LINE OUT toggles VFO linking

### Visual Polish (HD Quality)
- **Drop shadows**: All popups have 8-layer soft shadows (16px blur, offset 2,4)
- **2px borders**: Visible on Retina/4K displays (was 1px)
- **Consistent border radius**: 6px for buttons, 8px for popup backgrounds
- **Triangle pointer**: Points to trigger button on Band/Mode/Fn/RX/TX popups

### KPA1500 Integration
- K4 pushes KPA1500 status via ERxx: messages (ER41=power on, ER43=standby, ER44=operate)
- Notification widget displays KPA1500 status changes

### Hardware
- **KPOD support**: VFO encoder, rocker switch, 8 buttons (tap/hold)

### Distribution
- **Self-contained app bundle** - macOS deploy target bundles all dependencies

## Known Limitations

- None currently tracked

## References

- `docs/UI_COMPONENTS.md` - Complete widget inventory
- `docs/DEVLOG.md` - Development history
- `CHANGELOG.md` - User-facing release notes
