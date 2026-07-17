@echo off
REM QG Wavetable VST — Windows Build Script

echo ==========================================
echo QG Wavetable VST Build (Windows)
echo ==========================================
echo.

REM ==========================================
REM Check Python
REM ==========================================
echo [*] Checking Python...
python3 -c "import torch; print(f'  PyTorch version: {torch.__version__}')" || (
    echo  ERROR: PyTorch not installed
    echo  Run: pip install torch numpy omegaconf
    pause
    exit /b 1
)

python3 -c "import qg; print('  QG package: OK')" || (
    echo  [*] Installing QG package...
    pip install git+https://github.com/akhilsadam/qg.git
)

echo.
REM ==========================================
REM Add VST3 SDK path
REM ==========================================
REM Adjust the path below if your SDK lives elsewhere.
REM The script looks for a folder named "vst3-sdk" one level up from this batch file.
set "JUCE_VST3_SDK_PATH=%~dp0..\vst3-sdk"
if not exist "%JUCE_VST3_SDK_PATH%" (
    echo ERROR: VST3 SDK not found at "%JUCE_VST3_SDK_PATH%".
    echo Please extract the Steinberg VST3 SDK there or edit this script to point to its location.
    pause
    exit /b 1
)

echo [*] VST3 SDK path resolved to: "%JUCE_VST3_SDK_PATH%"

echo.
REM ==========================================
REM Building JUCE plugin...
REM ==========================================
if exist build (
    rmdir /s /q build
)
mkdir build
cd build

REM Configure for Visual Studio 2022 (adjust if needed)
cmake .. -G "Visual Studio 17 2022" -DCMAKE_CXX_STANDARD=17 -DJUCE_VST3_SDK_PATH="%JUCE_VST3_SDK_PATH%" || (
    echo ERROR: CMake configuration failed
    pause
    exit /b 1
)

REM Build Release
cmake --build . --config Release || (
    echo ERROR: Build failed
    pause
    exit /b 1
)

cd ..

echo.
echo ==========================================
echo Build Complete!
echo ==========================================
echo.
echo Plugin location:
echo   build\bin\QGWavetableVST_artefacts\VST3\
echo.
echo Next: Copy to VST3 folder
echo   C:\Program Files\Common Files\VST3\
echo.
echo Then restart your DAW and load the plugin!
echo.
pause