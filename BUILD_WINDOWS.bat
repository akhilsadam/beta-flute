@echo off
REM QG Wavetable VST — Windows Build Script

echo ==========================================
echo QG Wavetable VST Build (Windows)
echo ==========================================
echo.

REM Check Python
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
echo [*] Building JUCE plugin...
if exist build (
    rmdir /s /q build
)
mkdir build
cd build

REM Configure for Visual Studio 2022 (adjust if needed)
cmake .. -G "Visual Studio 17 2022" -DCMAKE_CXX_STANDARD=17 || (
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
