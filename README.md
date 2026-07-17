# QG Wavetable VST

A JUCE VST3/AU plugin that uses quasi-geostrophic flow simulation as a wavetable oscillator. The plugin is **fully self-contained**—load it in your DAW and it auto-launches the Python server.

## Features

✅ **Real-time QG simulation** as the wavetable oscillator  
✅ **Interactive XY sampling pad** — click to select which part of the simulation to sample  
✅ **Full config controls** — adjust grid resolution, viscosity, restart interval on-the-fly  
✅ **Live waveform visualization** — see the current wavetable being played  
✅ **ADSR envelope + lowpass filter** — classic synth controls  
✅ **Fully responsive** — MIDI pitch, gate, velocity all work  
✅ **Self-launching** — no separate server to run  

## Installation

### Prerequisites

Install PyTorch and the QG package:

```bash
pip install torch numpy omegaconf
pip install git+https://github.com/akhilsadam/qg.git
```

### Build Plugin

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

Output: `build/bin/QGWavetableVST_artefacts/`

### Install to DAW

Copy to your VST3 folder:
- **Linux**: `~/.vst3/`
- **macOS**: `~/Library/Audio/Plug-Ins/VST3/`
- **Windows**: `C:\Program Files\Common Files\VST3\`

Then load in your DAW like any plugin. It will auto-start the Python server on first load (takes 2-3 seconds).

## GUI Overview

```
┌─────────────────────────────────────────────────────────┐
│ QG Wavetable Synth                                      │
│ Status: ✓ Server running and connected                 │
├──────────────────┬──────────────────────┬───────────────┤
│  ADSR + FILTER   │  WAVEFORM DISPLAY    │   CONFIG      │
│  ─────────────   │  ──────────────────  │   ──────      │
│  [◯] Attack      │  ▁▂▃▄▅▆▇█▇▆▅▄▃▂▁    │  Grid Nx: ≡   │
│  [◯] Release     │  ▂▃▄▅▆▇█▇▆▅▄▃▂▁▂▃▄  │  Grid Ny: ≡   │
│  [◯] Cutoff      │                      │  Viscosity: ≡ │
│  [◯] Resonance   │  XY SAMPLING PAD     │  Restart: ≡   │
│                  │  ┌──────────────┐    │               │
│                  │  │    ●         │ ← click/drag      │
│                  │  │              │    to sample      │
│                  │  │              │    @ 128, 64      │
│                  │  └──────────────┘    │               │
└──────────────────┴──────────────────────┴───────────────┘
```

### Left Panel: ADSR + Filter

- **Attack** — envelope rise (0-0.5s)
- **Release** — envelope decay (0-2s)
- **Cutoff** — lowpass filter frequency (20Hz-20kHz)
- **Resonance** — filter Q factor (0.1-10)

### Middle Panel: Waveform + XY Sampler

- **Top graph** — live waveform being played (updates 20x/sec)
- **XY Pad** — click/drag to select sampling position in QG grid
  - Orange dot shows current position
  - Coordinates displayed in top-left
  - Grid is 256×256 (configurable)

### Right Panel: QG Config

- **Grid Nx/Ny** — simulation resolution (64-512)
- **Viscosity** — dampening (log scale, 1e-10 to 1e-1)
- **Restart Interval** — how often to reinitialize the simulation (1-30s)
- **Reinit on Note** — toggle whether simulation reinitializes on each new MIDI note

Changes apply immediately to the running server.

## How It Works

1. **Plugin loads** → spawns `qg_wavetable_server.py` subprocess
2. **Server runs QG simulation** at ~100 fps on your GPU
3. **You select a position** on the XY pad (e.g., middle of the grid)
4. **Server extracts a 1D slice** from that position each frame
5. **Plugin reads waveforms** via Unix socket, plays at MIDI pitch
6. **ADSR + filter** shape the tone in real-time

The simulation continuously evolves. Every 5 seconds (configurable), it reinitializes to keep things fresh.

## Workflow Examples

### Reinit on Note (Default)
- **Each time you hit a key** → fresh QG simulation starts
- **Pro**: Every note gets a unique waveform, very organic
- **Use for**: pads, leads, evolving textures per note

### Time-Based Restart (Toggle OFF)
- **Simulation continuously evolves** based on Restart Interval
- **Pro**: Smoother, more coherent evolution over longer periods
- **Use for**: bass lines, drones, sustained pads

### Hybrid Approach
- Keep **Reinit on Note** ON
- Set **Restart Interval** to 30s (very long)
- Each note gets a fresh start, but within a note the sim evolves over 30s

### Pad Sweep

- **Record automation** on the XY pad to sweep across the simulation
- Different regions = different timbres
- Creates smooth morphing motion through parameter space

### Config Tweaking

- Increase **viscosity** → smoother, less chaotic waveforms
- Decrease **grid resolution** → lower CPU, more obvious patterns
- **Reinit on Note OFF** → longer, evolving tones
- **Reinit on Note ON** → always-fresh, never boring

### Performance Tuning

If CPU is high:
- Reduce grid Nx/Ny to 128×128
- Increase restart interval to 10-30s
- Use CPU device instead of CUDA (modify Python startup)

## Troubleshooting

| Issue | Solution |
|-------|----------|
| No audio output | Check MIDI routing, ensure "✓ Server running" in status |
| "Server failed to start" | Ensure Python 3 + PyTorch installed (`pip install torch`) |
| Plugin crashes on load | Check console for Python errors; may need `pip install git+https://github.com/akhilsadam/qg.git` |
| High CPU usage | Reduce grid size or increase viscosity in config |
| Laggy XY pad response | Server is computing; normal on first startup |

## Advanced: Custom QG Config

Place a YAML config file at `~/.config/qg_wavetable.yaml` (Linux/Mac) or same directory as plugin:

```yaml
grid:
  Nx: 256
  Ny: 256
  Lx: 1.0
  Ly: 1.0

pde:
  nu: 1e-5      # viscosity
  mu: 0.0       # drag
  beta: 0.0     # coriolis effect

time:
  dt: 0.01
  T: 10.0

integrator:
  order: 2
  split_bc: false
```

## Notes

- **Monophonic** (one note at a time, nice and simple)
- First startup: ~2-3 sec (PyTorch compilation)
- Subsequent loads: instant
- Waveform updates 20x/sec; plenty smooth for audio
- Works on **Linux, macOS, Windows** (Unix socket fallback on Windows)

## License

QG solver from [github.com/akhilsadam/qg](https://github.com/akhilsadam/qg)

Plugin code: MIT
