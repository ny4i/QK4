# QK4 → Icom Network Port Analysis

> Author: NY4I — Tom Schaefer  
> Date: 2026-04-20  
> Repository: [github.com/ny4i/QK4](https://github.com/ny4i/QK4)

---

## Overview

This document captures the analysis of what it would take to extend QK4 — currently a dedicated Elecraft K4 remote control client — to also support Icom radios via their RS-BA1 UDP network protocol. The goal is to reuse QK4's polished panadapter, audio pipeline, and UI framework while swapping in an Icom-specific protocol layer.

Two key reference implementations inform this work:

- **TR4W** ([github.com/n4af/TR4W](https://github.com/n4af/TR4W)) — already has a working Icom network CI-V command/response implementation (IcomNetwork branch), written in Pascal/Delphi. The command protocol logic is proven and debugged.
- **wfview** ([gitlab.com/eliggett/wfview](https://gitlab.com/eliggett/wfview)) — open-source Qt/C++ application that implements the full Icom RS-BA1 UDP protocol stack including audio streaming, scope data, and CI-V — exactly what QK4 needs.

---

## iOS Portability Analysis (Separate Topic)

Before the Icom port analysis, QK4 was also evaluated for iOS portability. Summary:

### Reusable C++ Core (~30% of the app)
- `TcpClient` / `Protocol` — pure network logic, wraps cleanly in Obj-C++ for Swift bridging
- `RadioState` / `RadioSettings` — plain data models
- Opus encode/decode — libopus runs on iOS
- `DxClusterClient`, `KPA1500Client` — network-only, portable

### Requires Full Rewrite for iOS (~70%)

| Component | Problem | iOS Replacement |
|---|---|---|
| ~40 QWidget UI classes | Qt Widgets not available on iOS | SwiftUI from scratch |
| `PanadapterRhiWidget` | QRhi/QRhiWidget not supported on iOS | Metal directly |
| `AudioEngine` | Qt Multimedia unavailable on iOS | AVAudioEngine / AVAudioSession |
| KPOD (HIDAPI) | iOS has no general USB HID access | Not possible — drop |
| `SerialPort` | Qt SerialPort not on iOS | Drop or CoreBluetooth |
| RtMidi / IambicKeyer | Needs CoreMIDI rewrite | CoreMIDI (doable) |
| `CatServer` | Can't run background TCP servers on iOS | Drop or redesign |

### Recommended iOS Path
- Keep C++ network/protocol/state core, wrap in Objective-C++
- Rewrite UI in SwiftUI targeting iOS + macOS simultaneously — skip a macOS-Swift intermediate step
- Use Metal directly for the panadapter (QRhi's Metal backend gives a good reference)
- Replace AudioEngine with AVAudioEngine/AVAudioSession
- The panadapter rendering and C++ core are the hardest parts; the audio engine is straightforward by comparison

---

## Icom Port: Where the K4 DNA Lives in QK4

The surgery is more contained than it might appear. K4-specific code is concentrated in:

| Layer | K4-Specific Parts | Effort to Abstract |
|---|---|---|
| `src/network/protocol.cpp/.h` | Entire K4 binary packet parser | High — full replacement |
| `src/network/tcpclient.cpp` | TLS/PSK auth, port 9204, K4 TCP handshake | Medium — new auth/transport |
| `src/models/radiostate.h` | K4 mode names, menu IDs, macro IDs | Medium — generalize the model |
| `src/network/kpa1500client` | K4-specific amp integration | Drop for Icom |
| `src/network/k4discovery.cpp` | K4 mDNS discovery | Replace with Icom discovery |
| `src/network/catserver.cpp` | Mostly generic CAT — largely reusable | Low |
| UI widgets | K4 menu/button labeling | Low — cosmetic |

**Already radio-agnostic (no changes needed):**
- `AudioEngine` — does not know what radio is on the other end (once codec is abstracted)
- `PanadapterRhiWidget` — receives normalized spectrum bins, completely radio-neutral
- `ConnectionController` — manages threading and lifecycle, not protocol-specific
- `DxClusterController` — fully independent
- `CatServer` — mostly generic

---

## The Icom RS-BA1 UDP Protocol vs K4 TCP

The fundamental transport difference:

| Aspect | K4 | Icom RS-BA1 |
|---|---|---|
| Transport | TCP (single connection) | UDP (3 separate sockets) |
| Audio codec | Opus | μ-law / ADPCM / raw PCM |
| Audio sample rate | 12 kHz stereo | 8 kHz or 16 kHz (model-dependent) |
| Encryption | TLS/PSK | None (plain UDP) |
| Command channel | Binary framed over TCP | CI-V over UDP |
| Audio port | Same TCP connection | Separate UDP port |
| Scope/spectrum | Binary spectrum bins over TCP | Separate UDP scope data stream |
| Keepalive | TCP + ping packets | "Are you there / I am here" UDP exchange |

Icom uses **three separate UDP connections**:
1. **Control channel** — session setup, keepalive, "are you there/I am here"
2. **CI-V data channel** — radio commands and responses
3. **Audio channel** — RX/TX audio stream

Each channel uses sequence-numbered packets with a retransmit buffer. The `myId` is derived from local IP + port. Session establishment requires an explicit handshake before any audio or CI-V flows.

---

## wfview: The Key Reference Implementation

wfview ([gitlab.com/eliggett/wfview](https://gitlab.com/eliggett/wfview)) is an open-source Qt/C++ Icom remote control application that has already implemented the full RS-BA1 stack. Relevant files:

### Protocol Layer (`src/radio/`)
| File | Purpose |
|---|---|
| `icomudpbase.cpp` | Core UDP framing: sequence numbers, retransmit buffer, ping, keepalive handshake, myId/remoteId |
| `icomudpaudio.cpp` | Audio UDP stream: rx/tx threads, 30s watchdog, 1364-byte TX chunking, ident bytes |
| `icomudpcivdata.cpp` | CI-V command/response channel over UDP |
| `icomudphandler.cpp` | Orchestrates all three UDP channels |
| `icomcommander.cpp` | High-level radio command API (frequency, mode, etc.) |

### Audio Layer (`src/audio/`)
| File | Purpose |
|---|---|
| `audiohandlerqtinput/output` | Qt Multimedia-based audio I/O (most relevant for QK4) |
| `audiohandlerpaoutput` | PortAudio backend |
| `audiohandlerrtinput/output` | RtAudio backend |
| `resampler/` | Sample rate conversion (needed: Icom 8/16 kHz → output rate) |
| `adpcm/` | ADPCM codec implementation |
| `rxaudioprocessor.cpp` | RX audio pipeline (decode, resample, NR) |
| `txaudioprocessor.cpp` | TX audio pipeline (encode, resample) |
| `spectrumwidget.cpp` | Shows how Icom scope bucket data maps to display |

### Key Protocol Details Extracted from wfview

**UDP packet framing (`icomudpbase`):**
- Each packet has: `len`, `sentid`, `rcvdid`, `type`, `seq` fields
- `myId` = `(localIP[2] << 24) | (localIP[3] << 16) | localPort`
- Retransmit timer fires periodically; lost packets tracked in `txSeqBuf` map
- Keepalive: "are you there" (type 0x03) → "I am here" (type 0x04) → "are you ready" (type 0x06)

**Audio framing (`icomudpaudio`):**
- TX audio chunked to 1364 bytes max per packet
- `ident` = `0x9781` for 160-byte (0xa0) chunks, `0x0080` otherwise
- `datalen` field is big-endian; `sendseq` is big-endian
- 30-second watchdog — if no audio received, rx/tx threads are stopped
- Separate `rxAudioThread` and `txAudioThread`

---

## Recommended Architecture: `RadioProtocol` Abstraction

### Step 1: Define a Protocol Interface

```cpp
class RadioProtocol : public QObject {
    Q_OBJECT
public:
    virtual void connectToRadio(const RadioEntry &radio) = 0;
    virtual void disconnectFromRadio() = 0;
    virtual void sendFrequency(quint64 hz, int vfo = 0) = 0;
    virtual void sendMode(RadioState::Mode mode, int vfo = 0) = 0;
    virtual void sendCatCommand(const QString &cmd) = 0;
    virtual AudioFormat audioFormat() const = 0;  // sample rate, codec, channels

signals:
    void radioReady();
    void connectionLost();
    void frequencyChanged(int vfo, quint64 hz);
    void modeChanged(int vfo, RadioState::Mode mode);
    void spectrumData(int receiver, const QByteArray &bins, int offset,
                      int count, qint64 centerFreq, qint32 sampleRate, float noiseFloor);
    void audioReceived(const QByteArray &pcmData);
};
```

`K4Protocol` and `IcomNetworkProtocol` become concrete implementations. `ConnectionController` and all other controllers talk only to `RadioProtocol` — they never touch radio-specific code.

### Step 2: Lift wfview's Icom UDP Stack

Copy and adapt these files into `src/radio/icom/`:
- `icomudpbase.cpp/.h`
- `icomudpaudio.cpp/.h`
- `icomudpcivdata.cpp/.h`
- `icomudphandler.cpp/.h`
- `icomcommander.cpp/.h`
- `adpcm/` codec
- `resampler/`

Adapt to implement `RadioProtocol` interface. Strip wfview-specific logging categories and UI dependencies.

### Step 3: Port CI-V Commands from TR4W

TR4W's IcomNetwork branch (Pascal/Delphi) has the CI-V command set already proven in contest operation. Translate the command/response handling into `IcomCommander` methods. This avoids re-reverse-engineering the CI-V protocol.

### Step 4: Abstract the AudioEngine Codec

QK4's `AudioEngine` currently hardcodes Opus decode. Add a codec abstraction:

```cpp
class AudioCodec {
public:
    virtual QByteArray decode(const QByteArray &encoded) = 0;
    virtual QByteArray encode(const QByteArray &pcm) = 0;
    virtual int sampleRate() const = 0;
};
```

`OpusCodec` (existing), `AdpcmCodec` (from wfview), `PcmCodec` (passthrough). The `AudioEngine` jitter buffer, volume routing, and `QAudioSink` output stay completely unchanged.

### Step 5: Map Icom Scope Data to Panadapter

The panadapter already accepts normalized float arrays — it's fully radio-agnostic. The only work is mapping Icom's scope bucket data format to the `spectrumDataReceived` signal format that `SpectrumController` already handles. wfview's `spectrumwidget.cpp` documents the mapping.

---

## Scope Summary

| Work Item | Source | Effort |
|---|---|---|
| `RadioProtocol` abstract interface | New | Low |
| `K4Protocol` (refactor existing code) | QK4 existing | Low |
| `IcomNetworkProtocol` + UDP stack | Lift from wfview | Medium |
| CI-V command set | Port from TR4W IcomNetwork branch | Medium |
| `AudioCodec` abstraction + ADPCM | wfview adpcm/ + new interface | Low–Medium |
| Icom scope → panadapter mapping | wfview reference | Low |
| UI: Icom radio selector / settings page | New | Medium |
| `PanadapterRhiWidget` | No changes needed | None |
| `AudioEngine` jitter buffer / output | No changes needed | None |
| `DxClusterController`, `CatServer` | No changes needed | None |

**The panadapter and audio output pipeline — the hardest pieces of QK4 — require zero changes.**

---

## Target Icom Models

The RS-BA1 network interface is supported on:
- IC-7610, IC-7300, IC-9700 — CI-V over LAN via RS-BA1
- IC-705 — built-in WLAN, lighter variant of the protocol
- IC-905 — similar to IC-705 network approach
- IC-7851, IC-7700 — with optional LAN adapter

Model-specific CI-V command sets are handled in `icomcommander.cpp` in wfview and can be extended from TR4W's existing model table.

---

## Open Questions

1. **TR4W IcomNetwork branch scope coverage** — does it include spectrum/scope commands or only CAT + audio framing? This determines how much of the scope mapping needs new work.
2. **Icom model priority** — which radio(s) to validate against first (IC-7610 is the most common LAN-capable HF model).
3. **TX audio for iOS** — if the iOS port happens in parallel, the `AudioCodec` abstraction layer also solves the AVAudioEngine integration point.

---

*See also: [ICOM_PORT_ANALYSIS.md](ICOM_PORT_ANALYSIS.md) in this repository*
