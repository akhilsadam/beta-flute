# QG Wavetable VST — Quick Reference

## Installation (one-time)

```bash
# 1. Install dependencies
pip install torch numpy omegaconf
pip install git+https://github.com/akhilsadam/qg.git

# 2. Build plugin
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release

# 3. Copy to VST3 folder
# Linux:   ~/.vst3/
# macOS:   ~/Library/Audio/Plug-Ins/VST3/
# Windows: C:\Program Files\Common Files\VST3\
```

## First Load

- Load plugin in your DAW
- Wait 2-3 sec for "✓ Server running" status
- MIDI in should now work

## GUI Controls

### Left: Synth Parameters
- **Attack/Release** — ADSR envelope
- **Cutoff/Resonance** — lowpass filter

### Middle: Visualization
- **Top graph** — current waveform
- **XY Pad** — **click to select sampling position** (this is the fun part!)
  - Click anywhere on the grid
  - Different regions = different waveforms
  - Try sweeping across while playing notes

### Right: QG Simulation Config
- **Grid Nx/Ny** — resolution (higher = more detail, more CPU)
- **Viscosity** — smoothness (higher = less chaotic)
- **Restart Interval** — reinit frequency (shorter = more variety)

## Tips

1. **Start with defaults**, move the XY pad around to hear the difference
2. **Record automation** on the XY pad for evolving textures
3. **Lower grid resolution** (128×128) if CPU is high
4. **Increase viscosity** to get more musical, less noisy tones
5. **Drag in real-time** while holding a MIDI note to modulate

## Troubleshooting

- No sound? Check MIDI routing + status shows "✓ connected"
- Server won't start? `pip install git+https://github.com/akhilsadam/qg.git`
- High CPU? Reduce Nx/Ny or increase viscosity
- First load slow? Normal (PyTorch compilation). Subsequent loads instant.
