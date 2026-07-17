# QG Wavetable VST — Complete System

## Files Overview

### Python Backend
- **qg_wavetable_server.py** — Main server
  - Imports QG from github.com/akhilsadam/qg
  - Runs simulation in background thread
  - Streams waveforms via Unix socket
  - Accepts XY position updates via command socket

### JUCE Plugin
- **juce/QGWavetable.h** — Socket I/O
  - `QGWavetableBuffer` — circular waveform buffer
  - `QGServerProcess` — auto-launches Python server
  - `QGSocketClient` — waveform + command channels

- **juce/QGWavetableProcessor.h/cpp** — Audio engine
  - ADSR envelope
  - Lowpass filter
  - MIDI note → pitch conversion
  - Sampling position control

- **juce/QGWavetableComponents.h** — UI widgets
  - `QGWavetableVisualizer` — live waveform display
  - `QGSamplingPad` — XY grid selector
  - `QGConfigPanel` — Nx/Ny/viscosity/restart sliders

- **juce/QGWavetableEditor.h** — Main UI
  - Layouts all components
  - Syncs parameters bidirectionally
  - Status display

- **juce/Main.cpp** — VST3/AU entry point

### Config
- **CMakeLists.txt** — JUCE/plugin build

## Data Flow

```
┌──────────────────────────────────────┐
│   JUCE Plugin (Audio Thread)         │
│                                      │
│  MIDI in → Pitch/Gate/Velocity      │
│         ↓                            │
│  Read waveform from buffer           │
│         ↓                            │
│  Linear interpolation @ pitch        │
│         ↓                            │
│  ADSR envelope × sample              │
│         ↓                            │
│  Lowpass filter                      │
│         ↓                            │
│  Audio out (mono → stereo)          │
│                                      │
│  GUI updates:                        │
│  - XY pad → sendSamplingPosition()  │
│  - Config changes → TODO (not used) │
└──────────────────────────────────────┘
            ↑       ↓
     Socket (text)  (waveform binary)
            ↑       ↓
┌──────────────────────────────────────┐
│   Python Server (Compute)            │
│                                      │
│  QG simulation @ 100 fps             │
│         ↓                            │
│  Read sampling position (x, y)      │
│         ↓                            │
│  Extract 1D waveform from state      │
│         ↓                            │
│  Normalize & resample to 2048 samp  │
│         ↓                            │
│  Send to socket (QGWT header + data)│
│                                      │
│  Auto-restart every N seconds        │
└──────────────────────────────────────┘
```

## Socket Protocol

### Waveform Stream (plugin reads)
```
[magic: 4 bytes "QGWT"]
[size:  4 bytes unsigned int, little-endian]
[data:  size × 4 bytes, float32 samples]
```

### Command Channel (plugin writes)
```
"POS x y\n"  →  Server updates sampling position
"PING\n"     →  Server responds "PONG\n"
```

## Extension Points

### Adding Config Persistence
- Implement `setSamplingPosition()` parameter knobs → send via command socket
- Server already parses `CONFIG key value` commands

### Polyphony
- Add voice allocator in processor
- Per-voice phase tracking instead of global
- Multiply grid/buffer for multi-voice streams

### Visualization
- Expand waveform display to spectrogram
- Show QG grid heatmap (requires sending full 2D state)
- Streaming would need binary protocol upgrade

## Performance Targets

- **QG sim** — 100 fps @ 256×256 on mid-range GPU
- **Audio** — <5% CPU for filter + pitch conversion
- **Latency** — <100ms socket round-trip (not real-time critical)
- **Memory** — ~50MB for waveform buffers + PyTorch runtime

## Known Limitations

1. **Monophonic** — one voice at a time
2. **Unix socket** — Linux/macOS only (Windows would need named pipes)
3. **Config changes** — sent but not yet implemented on server side
4. **No preset system** — XY position + ADSR not saved between sessions
5. **No visuals of full QG grid** — only waveform slice

## Next Steps (if you want to extend it)

1. **Windows support** — use `CreateNamedPipe` instead of Unix sockets
2. **Polyphony** — add voice allocator + duplicate waveform streams
3. **Presets** — save/load XY + ADSR + config to plugin state
4. **Full grid viz** — send 2D state periodically for heatmap display
5. **Config GUI** — implement the right panel config changes on server side
