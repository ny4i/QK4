# K4Controller

A macOS desktop application for remote control of Elecraft K4 radios over TCP/IP with real-time audio streaming and spectrum display.

## Features

- **Dual VFO Display** — Frequency, mode, and S-meter for VFO A and B
- **Spectrum Analyzer** — Real-time panadapter with waterfall display, click-to-tune, and scroll-wheel tuning
- **Mini-Pan Widget** — Compact spectrum view in VFO area with mode-dependent bandwidth
- **Audio Streaming** — Opus-encoded bidirectional audio (RX playback, TX microphone)
- **Radio Controls** — Full control panel with WPM, power, bandwidth, RF/SQL, and TX functions
- **Band Selection** — Quick band switching via popup menu
- **KPA1500 Support** — Optional integration with Elecraft KPA1500 amplifier

## Requirements

- macOS (tested on macOS 14+)
- Qt 6
- libopus
- hidapi
- CMake 3.16+

## Installation

### Install Dependencies

```bash
brew install qt@6 opus hidapi cmake
```

### Build

```bash
git clone https://github.com/mikeg-dal/K4Controller.git
cd K4Controller
cmake -B build -DCMAKE_PREFIX_PATH="$(brew --prefix qt@6)"
cmake --build build
```

### Run

```bash
./build/K4Controller
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

MIT License. See [LICENSE](LICENSE) for details.
