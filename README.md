# K4Controller

A cross-platform desktop application for remote control of Elecraft K4 radios over TCP/IP with real-time audio streaming and spectrum display.

[![Release](https://img.shields.io/github/v/release/mikeg-dal/K4Controller?include_prereleases)](https://github.com/mikeg-dal/K4Controller/releases)
[![Build macOS](https://github.com/mikeg-dal/K4Controller/actions/workflows/build-macos.yml/badge.svg)](https://github.com/mikeg-dal/K4Controller/actions/workflows/build-macos.yml)
[![Build Windows](https://github.com/mikeg-dal/K4Controller/actions/workflows/build-windows.yml/badge.svg)](https://github.com/mikeg-dal/K4Controller/actions/workflows/build-windows.yml)
[![Lint](https://github.com/mikeg-dal/K4Controller/actions/workflows/lint.yml/badge.svg)](https://github.com/mikeg-dal/K4Controller/actions/workflows/lint.yml)

## Supported Platforms

| Platform | Minimum Version | Architecture |
|----------|-----------------|--------------|
| macOS | 14 (Sonoma) | Apple Silicon (M1/M2/M3/M4) |
| Windows | 11 | x64 |

## Features

- **TLS/PSK Encrypted Connection** — Secure connection via TLS v1.2 with Pre-Shared Key on port 9204
- **Dual VFO Display** — Frequency, mode, S-meter, and tuning rate indicator for VFO A and B
- **GPU-Accelerated Spectrum** — Real-time panadapter with waterfall via Qt RHI (Metal/DirectX/Vulkan)
- **Mini-Pan Widget** — Compact spectrum view in VFO area with mode-dependent bandwidth
- **Dual-Channel Audio** — Opus-encoded stereo with independent MAIN/SUB volume controls
- **Radio Controls** — Full control panel with mode-dependent controls, TX functions, and feature popups
- **Band Selection** — Quick band switching via popup menu
- **KPA1500 Support** — Optional integration with Elecraft KPA1500 amplifier
- **Self-Contained App Bundle** — macOS releases include all dependencies (no Homebrew required)

## Requirements

### macOS

**To run pre-built release:**
- macOS 14 (Sonoma) or later
- Apple Silicon Mac (M1 or newer)
- No additional dependencies required (self-contained app bundle)

**To build from source:**
- Homebrew
- Qt 6, libopus, OpenSSL 3, hidapi

### Windows

**To run pre-built release:**
- Windows 11
- [Visual C++ Redistributable 2019+](https://aka.ms/vs/17/release/vc_redist.x64.exe)

**To build from source:**
- Visual Studio 2019+ Build Tools
- Qt 6, libopus, hidapi (via vcpkg)

## Installation

### macOS

```bash
# Install dependencies
brew install qt@6 opus openssl@3 hidapi cmake

# Clone and build
git clone https://github.com/mikeg-dal/K4Controller.git
cd K4Controller
cmake -B build -DCMAKE_PREFIX_PATH="$(brew --prefix qt@6)"
cmake --build build

# Run
./build/K4Controller

# Create distributable app bundle (optional)
cmake --build build --target deploy
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
3. Enter your K4's IP address
4. **For encrypted connection**: Check "Use TLS", enter your PSK, port auto-sets to 9204
5. **For unencrypted connection**: Leave TLS unchecked, port defaults to 9205
6. Click **Connect**

Once connected, the application displays real-time spectrum, audio, and radio state from your K4.

## Architecture

```
Radio (TCP:9204 TLS / 9205 unencrypted) → TcpClient → Protocol → RadioState / DSP Widgets
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
