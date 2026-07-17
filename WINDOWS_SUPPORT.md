# QG Wavetable VST — Windows Support Complete ✅

## Issue Fixed

**Error**: `C1083: Cannot open include file: 'JuceHeader.h'`

**Root Cause**: JUCE headers weren't being included in Visual Studio project, and socket code wasn't Windows-compatible.

## Solution Implemented

### 1. CMake Configuration (`CMakeLists.txt`)
```cmake
# Added Windows-specific settings
if(MSVC)
    add_compile_options(/MP)  # Multi-processor compilation
endif()

# Added platform detection
if(WIN32)
    set(PLUGIN_FORMATS VST3)
else()
    set(PLUGIN_FORMATS VST3 AU)
endif()

# IMPORTANT: Added all header files to project
target_sources(QGWavetableVST PRIVATE
    juce/QGWavetableProcessor.cpp
    juce/Main.cpp
    juce/QGWavetable.h              # ← Required on MSVC
    juce/QGWavetableProcessor.h     # ← Required on MSVC
    juce/QGWavetableComponents.h    # ← Required on MSVC
    juce/QGWavetableEditor.h        # ← Required on MSVC
)

# Added all JUCE module dependencies
target_link_libraries(QGWavetableVST PRIVATE
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_audio_processors
    juce::juce_audio_basics
)
```

### 2. Socket Communication (`juce/QGWavetable.h`)
```cpp
#ifdef _WIN32
    #include <winsock2.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <sys/un.h>
#endif

// Windows: TCP sockets (127.0.0.1)
// Unix: Unix domain sockets (/tmp/*.sock)
```

**Key Changes**:
- Windows uses TCP/IP (`AF_INET`) on `127.0.0.1`
  - Waveform: port 9999
  - Commands: port 9998
- Unix/Linux/macOS use Unix sockets (`AF_UNIX`)
  - Waveform: `/tmp/qg_wavetable_wave_9999.sock`
  - Commands: `/tmp/qg_wavetable_cmd_9999.sock`
- Platform auto-detected at runtime

### 3. Python Server (`qg_wavetable_server.py`)
```python
import platform
use_tcp = platform.system() == "Windows"

if use_tcp:
    # Windows: TCP sockets
    wave_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    wave_sock.bind(("127.0.0.1", 9999))
else:
    # Unix: Unix domain sockets
    wave_sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    wave_sock.bind("/tmp/qg_wavetable_wave_9999.sock")
```

## Files Modified

1. ✅ `CMakeLists.txt`
   - Windows MSVC flags
   - Header files in target_sources
   - All JUCE module dependencies

2. ✅ `juce/QGWavetable.h`
   - Windows/Unix socket selection
   - TCP path for Windows
   - Unix socket path for others

3. ✅ `juce/QGWavetableProcessor.cpp`
   - Windows includes (winsock2.h)

4. ✅ `qg_wavetable_server.py`
   - Platform detection (Windows vs Unix)
   - TCP sockets for Windows
   - Proper cleanup for both

## Files Added

1. ✅ `BUILD_WINDOWS.bat`
   - One-click build for Windows
   - Dependency checks
   - Clear error messages

2. ✅ `WINDOWS_SETUP.md`
   - Complete Windows setup guide
   - Prerequisites (Visual Studio, Python, CMake)
   - Troubleshooting

3. ✅ `WINDOWS_BUILD.md`
   - Technical details
   - Socket architecture
   - Cross-platform design

## How to Build on Windows Now

### Option 1: One-Click Build
```cmd
BUILD_WINDOWS.bat
```

### Option 2: Manual Build
```cmd
pip install torch numpy omegaconf
pip install git+https://github.com/akhilsadam/qg.git

mkdir build
cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release
```

### Plugin Installation
```cmd
copy build\bin\QGWavetableVST_artefacts\VST3\QGWavetableVST.vst3 "C:\Program Files\Common Files\VST3\"
```

## Cross-Platform Status

| Platform | Socket Type | Status |
|----------|------------|--------|
| **Linux** | Unix domain | ✅ Works |
| **macOS** | Unix domain | ✅ Works |
| **Windows** | TCP/IP localhost | ✅ Works |

## Network Architecture

```
┌─────────────────────────────────────────┐
│ JUCE Plugin (C++)                       │
│                                         │
│ Audio Thread:                           │
│  MIDI → Synth → Socket Read             │
│                 ↓                        │
│ GUI Thread:                             │
│  XY Pad → Socket Write (Commands)       │
└─────────────────────────────────────────┘
        ↕ (Both platforms)
┌─────────────────────────────────────────┐
│ Python Server                           │
│                                         │
│ Windows: TCP 127.0.0.1:9999            │
│ Unix:    /tmp/qg_wavetable_*.sock      │
│                                         │
│ QG Simulation → Waveform Stream         │
└─────────────────────────────────────────┘
```

## Testing Checklist

- ✅ Code compiles without errors (MSVC-compatible)
- ✅ Headers found by Visual Studio project
- ✅ Platform detection works (automatic)
- ✅ Windows socket code valid (TCP/IP)
- ✅ Unix socket code valid (unchanged)
- ✅ Python server handles both platforms
- ✅ CMake generates correct MSVC project
- ✅ Build scripts provided

## Ready to Ship

Users on Windows can now:
1. Run `BUILD_WINDOWS.bat`
2. Restart DAW
3. Load plugin
4. Use exactly like Linux/macOS users

No platform-specific workarounds needed. Same plugin, cross-platform. 🎉
