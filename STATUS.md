# Status

Current implementation state of K4Controller (January 2026).

## Implemented Features

### Core
- **TLS/PSK encrypted connection** on port 9204 (unencrypted on 9205)
- Dual VFO display (A/B) with frequency, mode, S-meter
- **GPU-accelerated panadapter** via Qt RHI (Metal/DirectX/Vulkan)
- Mini-Pan widget in VFO area (mode-dependent bandwidth: CW=3kHz, Voice/Data=10kHz)
- **Dual-channel audio mixing**: MAIN volume (left), SUB volume (right)
- Opus audio decoding and playback
- Server manager dialog with TLS checkbox and PSK field

### VFO Display
- **Tuning rate indicator**: White underline below frequency digit
- **S-Meter with peak hold**: 500ms decay, gradient transitions from green→yellow→orange→red
- **Mode display**: Full mode with data sub-modes (AFSK, FSK, PSK, DATA)
- Processing indicators: AGC, PRE, ATT, NB, NR, NTCH
- Mini-pan toggle (click S-meter to show spectrum)
- VOX indicator (mode-aware: VXC/VXV/VXD)
- **Clickable VFO squares**: Open mode popup targeting A or B

### Panadapter
- Spectrum styles: Blue gradient, BlueAmplitude (LUT-based)
- Click-to-tune, scroll-wheel tuning
- Peak hold with decay
- Dynamic ref level and span from K4
- Filter passband and frequency marker overlays

### Control Panels
- **Left panel**: Mode-dependent (WPM/PTCH or MIC/CMP), PWR/DLY, BW/SHFT, M.RF/M.SQL, S.RF/S.SQL, TX buttons, MAIN/SUB volume
- **Right panel**: 5×2 button grid (PRE/ATTN, NB/LEVEL, NR/ADJ, etc.)
- **Bottom menu bar**: MENU, Fn, DISPLAY, BAND, MAIN RX, SUB RX, TX

### Popups & Dialogs
- Band popup (14 bands in 2 rows)
- Menu overlay on panadapter
- Feature menu bar (ATTN/NB/NR/NOTCH)
- Display popup (ref level, span, tune mode, waterfall color)
- Options dialog (About, KPOD, KPA1500)

### Distribution
- **Self-contained app bundle** - macOS deploy target bundles all dependencies

## Known Limitations

- None currently tracked

## References

- `docs/UI_COMPONENTS.md` - Complete widget inventory
- `docs/DEVLOG.md` - Development history
- `CHANGELOG.md` - User-facing release notes
