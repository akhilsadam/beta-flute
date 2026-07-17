# QG Wavetable VST — Windows Setup

## Prerequisites

1. **Visual Studio 2022** (Community Edition is free)
   - Install with C++ development tools
   - Download: https://visualstudio.microsoft.com/

2. **Python 3.10+**
   - Download: https://www.python.org/downloads/
   - **Important**: Check "Add Python to PATH" during installation

3. **CMake 3.20+**
   - Download: https://cmake.org/download/
   - Add to PATH during installation

4. **Git** (optional, for cloning if needed)
   - Download: https://git-scm.com/

## Installation Steps

### 1. Open Command Prompt
- Press `Win + R`
- Type `cmd` and hit Enter

### 2. Navigate to the project folder
```cmd
cd C:\Users\YourName\Desktop\Research\beta-flute
```

### 3. Install Python dependencies
```cmd
pip install torch numpy omegaconf
pip install git+https://github.com/akhilsadam/qg.git
```

If you get "pip: command not found", Python wasn't added to PATH. Restart and reinstall Python, checking "Add Python to PATH".

### 4. Run the build script
```cmd
BUILD_WINDOWS.bat
```

This will:
- Check your environment
- Run CMake
- Build the plugin
- Show you where the plugin is located

### 5. Install to DAW
Copy the plugin from the build folder to your VST3 folder:

```
From:  build\bin\QGWavetableVST_artefacts\VST3\
To:    C:\Program Files\Common Files\VST3\
```

You can do this either:
- **File Explorer**: Drag and drop the .vst3 file
- **Command line**:
  ```cmd
  copy "build\bin\QGWavetableVST_artefacts\VST3\QGWavetableVST.vst3" "C:\Program Files\Common Files\VST3\"
  ```

### 6. Restart your DAW
Close and reopen your DAW. The plugin should now appear in your plugin list!

## Troubleshooting

### "CMake not found"
- Download and install CMake: https://cmake.org/download/
- Make sure "Add CMake to system PATH" is checked
- Restart Command Prompt

### "Python not found" or "pip not found"
- Reinstall Python from https://www.python.org/
- **IMPORTANT**: Check "Add Python to PATH" during installation
- Restart Command Prompt after installation

### "PyTorch installation failed"
```cmd
pip install --upgrade pip
pip install torch torchvision torchaudio
```

### "QG package not found"
```cmd
pip install git+https://github.com/akhilsadam/qg.git
```

### Build fails with "JuceHeader.h not found"
- Make sure CMake ran successfully (first time takes 5-10 min to download JUCE)
- Delete the `build` folder and try again:
  ```cmd
  rmdir /s /q build
  BUILD_WINDOWS.bat
  ```

### Plugin not appearing in DAW
- Make sure plugin is in `C:\Program Files\Common Files\VST3\`
- Restart the DAW
- Some DAWs need manual plugin scanning:
  - **Ableton**: Preferences → File Folder → Re-scan
  - **FL Studio**: Options → Plugins → Rescan Plugins
  - **Reaper**: Options → Preferences → Plugins → Re-scan VST3

## Network Access (Important for Server)

The plugin communicates with the Python server via **localhost TCP sockets** on Windows:
- Waveform port: `127.0.0.1:9999`
- Command port: `127.0.0.1:9998`

If you get a firewall warning, **allow it**. The plugin and server only talk to your local machine.

## Testing

1. Load the plugin in your DAW
2. Wait 3-5 seconds for server to start (first time is slower)
3. Play MIDI notes
4. You should hear sound

If you don't hear sound, check:
- MIDI is routed to the plugin
- Plugin UI shows "✓ Server running and connected"
- Volume is up

## Performance Tips

If the plugin is using too much CPU:
- Reduce **Grid Nx/Ny** to 128 in the config
- Increase **Viscosity** slider
- Disable GPU (run `qg_wavetable_server.py --device cpu`)

## Questions?

Check:
- `README.md` — Full feature guide
- `QUICKSTART.md` — 5-minute overview
- `ARCHITECTURE.md` — Technical details
