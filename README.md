# K4Controller

A cross-platform desktop application for remote control of Elecraft K4 radios over TCP/IP with real-time audio streaming and spectrum display.

[![Build macOS](https://github.com/mikeg-dal/K4Controller/actions/workflows/build-macos.yml/badge.svg)](https://github.com/mikeg-dal/K4Controller/actions/workflows/build-macos.yml)
[![Build Windows](https://github.com/mikeg-dal/K4Controller/actions/workflows/build-windows.yml/badge.svg)](https://github.com/mikeg-dal/K4Controller/actions/workflows/build-windows.yml)

## Supported Platforms

| Platform | Minimum Version | Architecture |
|----------|-----------------|--------------|
| macOS | 14 (Sonoma) | Apple Silicon (M1/M2/M3/M4) |
| Windows | 11 | x64 |

## Features

- **Dual VFO Display** — Frequency, mode, and S-meter for VFO A and B
- **Spectrum Analyzer** — Real-time panadapter with waterfall display, click-to-tune, and scroll-wheel tuning
- **Mini-Pan Widget** — Compact spectrum view in VFO area with mode-dependent bandwidth
- **Audio Streaming** — Opus-encoded bidirectional audio (RX playback, TX microphone)
- **Radio Controls** — Full control panel with WPM, power, bandwidth, RF/SQL, and TX functions
- **Band Selection** — Quick band switching via popup menu
- **KPA1500 Support** — Optional integration with Elecraft KPA1500 amplifier

## Requirements

### macOS

- macOS 14 (Sonoma) or later
- Apple Silicon Mac (M1 or newer)
- Homebrew
- Qt 6, libopus, hidapi

### Windows

- Windows 11
- Visual Studio 2019+ Build Tools
- Qt 6, libopus, hidapi (via vcpkg)

## Installation

### macOS

```bash
# Install dependencies
brew install qt@6 opus hidapi cmake

# Clone and build
git clone https://github.com/mikeg-dal/K4Controller.git
cd K4Controller
cmake -B build -DCMAKE_PREFIX_PATH="$(brew --prefix qt@6)"
cmake --build build

# Run
./build/K4Controller
```

### Windows

```powershell
# Install vcpkg dependencies
vcpkg install opus:x64-windows hidapi:x64-windows

# Install Qt 6 (via Qt Online Installer or aqtinstall)

# Clone and build
git clone https://github.com/mikeg-dal/K4Controller.git
cd K4Controller
cmake -B build -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release

# Run
.\build\Release\K4Controller.exe
```

## Usage

1. Launch K4Controller
2. Go to **File → Connect** to open the Radio Manager
3. Enter your K4's IP address and port (default: 9205)
4. Click **Connect**

Once connected, the application displays real-time spectrum, audio, and radio state from your K4.

## Architecture

```
Radio (TCP:9205) → TcpClient → Protocol → RadioState / DSP Widgets
                                        ↓
                                 OpusDecoder → AudioEngine → Speaker
Microphone → AudioEngine → OpusEncoder → Protocol → TcpClient → Radio
```

## Project Structure

```
src/
├── main.cpp              # Application entry point
├── mainwindow.cpp        # Main window and UI orchestration
├── network/              # TCP client and K4 protocol handling
├── audio/                # Opus codec and Qt audio engine
├── dsp/                  # Panadapter and spectrum widgets
├── models/               # Radio state model
├── settings/             # QSettings persistence
├── ui/                   # UI components (VFO, S-meter, controls)
└── hardware/             # KPOD USB device support
```

## License

See [LICENSE](LICENSE) for details.
