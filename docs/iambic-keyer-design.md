# Software Iambic Keyer for QK4 (HaliKey MIDI Squeeze)

## Problem

QK4's MIDI paddle input sends a single `KZ.;`/`KZ-;` CAT command on paddle press, then does nothing. The K4 radio's internal keyer doesn't receive repeated commands, so dit/dah alternation (iambic squeeze) never happens. Both NetKeyer and TR4QT solve this with a software iambic keyer state machine.

## Solution

Ported TR4QT's `IambicKeyer` class (~240 lines, pure Qt) into `src/keyer/iambickeyer.{h,cpp}`. The keyer is a state machine that converts paddle press/release events into timed keyDown/keyUp signals.

### State Machine

```
Idle → TonePlaying → InterElementSpace → (next element or Idle)
```

- **dit** = 1200 / WPM ms
- **dah** = 3 x dit ms
- **inter-element space** = 1 dit

### Iambic Modes

- **Mode A**: element stops when paddle is released
- **Mode B**: squeeze (both paddles held then released) sends one extra alternate element

### Signal Flow

```
HalikeyDevice::paddleStateChanged(dit, dah)
    → IambicKeyer::updatePaddleState(dit, dah)
        → IambicKeyer::keyDown(bool isDit)
            → MainWindow sends KZ.;/KZ-; CAT command
            → SidetoneGenerator::playSingleDit()/playSingleDah()
        → IambicKeyer::keyUp()
            → SidetoneGenerator::stopElement()

RadioState::keyerSpeedChanged(wpm)
    → IambicKeyer::setWpm(wpm)
    → SidetoneGenerator::setKeyerSpeed(wpm)

HalikeyDevice::disconnected
    → IambicKeyer::stop()
    → SidetoneGenerator::stopElement()
```

## Files Created

| File | Description |
|------|-------------|
| `src/keyer/iambickeyer.h` | IambicKeyer class header with IambicMode enum |
| `src/keyer/iambickeyer.cpp` | State machine implementation (ported from TR4QT) |
| `tests/test_iambickeyer.cpp` | Unit tests: dit/dah, repeat, squeeze, Mode A/B, stop |
| `docs/iambic-keyer-design.md` | This document |

## Files Modified

| File | Change |
|------|--------|
| `src/hardware/halikeydevice.h` | Added `paddleStateChanged(bool dit, bool dah)` signal |
| `src/hardware/halikeydevice.cpp` | Emit `paddleStateChanged` from all 4 state-change paths (2 immediate + 2 debounce timer) |
| `src/mainwindow.h` | Replaced `m_ditRepeatTimer`/`m_dahRepeatTimer` with `IambicKeyer *m_iambicKeyer` |
| `src/mainwindow.cpp` | Removed old repeat timers and direct KZ sends; wired paddleStateChanged → keyer → CAT + sidetone |
| `src/audio/sidetonegenerator.cpp` | `stopElement()` now flushes audio sink buffer via `reset()` |
| `CMakeLists.txt` | Added keyer sources/headers + `test_iambickeyer` test target |

## Known Issues

- **Sidetone audio artifacts**: `stopElement()` calls `m_audioSink->reset()` to flush buffered audio, which causes audible clicks/artifacts. The root cause is that `playElement()` writes the entire element (tone + silence) as a pre-rendered PCM buffer. Cutting this buffer mid-playback via `reset()` produces discontinuities. A proper fix would require continuous tone generation (start/stop oscillator) rather than pre-rendered buffers.
- **Sidetone latency**: The QAudioSink push-mode buffer introduces latency between keyDown and audible tone. The pre-rendered buffer approach adds the full element duration to the pipeline before any audio plays.

## What Was Removed

- `m_ditRepeatTimer` / `m_dahRepeatTimer` (500ms interval timers in MainWindow)
- Direct `ditStateChanged`/`dahStateChanged` → `KZ` CAT send connections
- `SidetoneGenerator::ditRepeated`/`dahRepeated` → `KZ` CAT send connections

## Debug Logging

The keyer includes diagnostic logging (prefix `[KEYER ...]`) with millisecond timestamps covering every state transition, paddle update, latch change, and safety timeout. MainWindow logs `[KZ-SEND]` for every CAT command sent. Filter with:

```bash
tail -f /tmp/qk4_keyer_debug.log | grep -E '\[KEYER|\[KZ'
```
