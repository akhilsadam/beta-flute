# Reinit on Note Feature

## What It Does

**"Reinit on Note" toggle** controls whether the QG simulation reinitializes (restarts) when you play a new MIDI note.

## Settings

### ✅ ON (Default)
- Every MIDI note triggers a fresh QG simulation
- Each note starts with a unique, evolving waveform
- **Sound character**: Organic, always fresh, slightly unpredictable
- **Use case**: Pads, leads, percussive sounds

### ❌ OFF (Time-based only)
- Simulation continuously evolves without note boundaries
- Only restarts based on **Restart Interval** timer (e.g., every 5s)
- **Sound character**: Smooth, evolving, coherent
- **Use case**: Bass lines, drones, long pad tones

## How It Works

### With Reinit ON
```
Note 1: [Fresh QG sim] ──→ evolves ──→ [ends at some waveform]
Note 2: [Fresh QG sim] ──→ evolves ──→ [ends at some waveform]
Note 3: [Fresh QG sim] ──→ evolves ──→ [ends at some waveform]
```
Each note gets its own simulation cycle.

### With Reinit OFF
```
[Fresh QG sim] ──→ evolves ──→ [5s later: reinit] ──→ evolves ──→ [5s later: reinit]
    ▲                              ▲                                    ▲
  Note 1                         Note 2                              Note 3
```
Notes play within a continuous evolving simulation.

## Practical Workflows

### Create Variation
- **Toggle ON for pads** → Each time you play a chord, different harmonic content
- **Toggle ON for melodies** → Each note is slightly different

### Create Coherence
- **Toggle OFF for bass** → Smooth, evolving low-end texture
- **Toggle OFF for long drones** → Tonal evolution over 30+ seconds

### Hybrid: The Best of Both
1. **Reinit ON** + **Restart Interval: 30s**
2. Each note gets a fresh start
3. Within a note, the sim evolves over 30 seconds
4. = Fresh + Evolving simultaneously

## Settings Interaction

- **Reinit ON + short Restart Interval** → Every note gets reset, very chaotic
- **Reinit ON + long Restart Interval** → Notes start fresh, but within-note evolution is smooth
- **Reinit OFF + short Restart Interval** → Simulation keeps restarting frequently, bouncy feel
- **Reinit OFF + long Restart Interval** → Smooth, long-form evolution (best for ambient)

## GUI Location

**Right panel, bottom**: "✓ Reinit on Note" checkbox
- Checked = ON (reinit per note)
- Unchecked = OFF (time-based restart only)
