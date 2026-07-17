# Windows Support — Complete

## What Was Fixed

### 1. **CMake Configuration**
- ✅ Added Windows-specific compiler flags
- ✅ Auto-detect platform (Windows uses VST3 only)
- ✅ Added all required JUCE module dependencies
- ✅ Include header files in project (required on Windows)

### 2. **Socket Communication**
- ✅ Unix sockets (Linux/macOS) — works as before
- ✅ TCP sockets (Windows) — new, uses `127.0.0.1` localhost
- ✅ Automatic platform detection in both C++ and Python
- ✅ Clean socket cleanup on both platforms

### 3. **Python Server**
- ✅ Auto-detects Windows vs Unix
- ✅ Uses TCP on Windows (port 9999 for waveforms, 9998 for commands)
- ✅ Uses Unix sockets on Linux/macOS (as before)
- ✅ Proper socket cleanup on both

### 4. **C++ Code**
- ✅ `#ifdef _WIN32` guards for platform-specific code
- ✅ Uses `winsock2.h` on Windows, standard sockets elsewhere
- ✅ Proper socket close (`closesocket` vs `close`)
- ✅ Windows includes properly linked in CMake

### 5. **Build System**
- ✅ Added `target_sources()` with all headers (required on MSVC)
- ✅ Platform-specific plugin format selection
- ✅ Multi-processor compilation (`/MP` flag)
- ✅ Warning level settings for clean build

## Windows Build Instructions

### Quick Start
```cmd
cd C:\path\to\beta-flute
BUILD_WINDOWS.bat
```

### Manual Build
```cmd
# 1. Install dependencies
pip install torch numpy omegaconf
pip install git+https://github.com/akhilsadam/qg.git

# 2. Build
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release

# 3. Install
copy build\bin\QGWavetableVST_artefacts\VST3\QGWavetableVST.vst3 "C:\Program Files\Common Files\VST3\"
```

## Cross-Platform Socket Design

### **Linux/macOS** (Unix Sockets)
```
/tmp/qg_wavetable_wave_9999.sock  → waveforms
/tmp/qg_wavetable_cmd_9999.sock   → commands
```

### **Windows** (TCP Sockets)
```
127.0.0.1:9999  → waveforms
127.0.0.1:9998  → commands
```

Both:
- Auto-connect on plugin load
- Auto-reconnect if server restarts
- Clean shutdown on unload
- No firewall issues (localhost only)

## Files Updated

- ✅ `CMakeLists.txt` — Windows compiler settings + all headers
- ✅ `qg_wavetable_server.py` — Platform detection + TCP fallback
- ✅ `juce/QGWavetable.h` — Winsock2 includes + TCP path
- ✅ `juce/QGWavetableProcessor.cpp` — Windows includes

## Files Added

- ✅ `BUILD_WINDOWS.bat` — One-click Windows build
- ✅ `WINDOWS_SETUP.md` — Complete Windows setup guide

## Testing Status

- ✅ Code compiles (no syntax errors)
- ✅ Platform detection works
- ✅ Socket code paths correct
- ✅ CMake properly configured
- ✅ Headers included in MSVC project

## Known Limitations

- Windows Firewall may prompt on first run (allow it)
- First run slower (PyTorch compilation on server)
- Requires Visual Studio 2022 (or adjust CMake generator)
- CPU device recommended if no NVIDIA GPU

## What Users See

On Windows:
1. Run `BUILD_WINDOWS.bat`
2. Wait for compilation (~5-10 min on first build)
3. Copy plugin to `C:\Program Files\Common Files\VST3\`
4. Open DAW, load plugin
5. Same experience as Linux/macOS users

Done! 🎉
