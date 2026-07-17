@echo off
REM QG Wavetable VST — Windows Build Script

echo ==========================================
echo QG Wavetable VST Build (Windows)
echo ==========================================
echo.

<<<<<<< HEAD
REM -----------------------------------------------------------
REM Resolve the SDK path
REM -----------------------------------------------------------
REM 1) Command‑line argument takes highest priority
if not "%~1"=="" (
    set "JUCE_VST3_SDK_PATH=%~1"
) else (
    REM 2) Otherwise look for an existing folder that looks like the SDK
    for /d %%D in (VST3_SDK*) do (
        set "JUCE_VST3_SDK_PATH=%%~fD"
    )
)

REM 3) If still empty, fall back to automatic download
if not defined JUCE_VST3_SDK_PATH (
    echo [*] VST3 SDK not found, attempting download...
    echo     Target URL: %SDK_URL%
    echo.

    powershell -NoProfile -Command ^
        "Invoke-WebRequest -Uri '%SDK_URL%' -OutFile 'VST3_SDK.zip' -UseBasicParsing" ^
        2>nul
    if errorlevel 1 (
        echo ERROR: Failed to download the SDK. Possible reasons:
        echo   • The URL changed or now requires a manual login.
        echo   • Your network blocks the request.
        echo.
        echo Please visit %SDK_URL% in a browser, accept Steinberg's licence,
        echo download the ZIP manually, extract it, and place the folder at:
        echo     %~dp0..\vst3-sdk
        echo Then rerun this script, optionally passing the folder path as an argument.
        pause
        exit /b 1
    )

    echo [*] Downloaded VST3_SDK.zip, extracting now...
    powershell -NoProfile -Command ^
        "Expand-Archive -Path 'VST3_SDK.zip' -DestinationPath '.' -Force" ^
        2>nul
    if errorlevel 1 (
        echo ERROR: Extraction failed. Make sure you have write permission
        echo in the script directory.
        pause
        exit /b 1
    )

    REM The extraction creates a folder that starts with VST3_SDK
    for /d %%D in (VST3_SDK*) do (
        set "JUCE_VST3_SDK_PATH=%%~fD"
    )
)

REM Validate that the resolved folder really looks like an SDK
if not exist "%JUCE_VST3_SDK_PATH%\ReadMe.txt" (
    echo WARNING: The folder at "%JUCE_VST3_SDK_PATH%" does not contain ReadMe.txt.
    echo It may not be the correct SDK version.
=======
REM ==========================================
REM Check Python
REM ==========================================
echo [*] Checking Python...
python3 -c "import torch; print(f'  PyTorch version: {torch.__version__}')" || (
    echo  ERROR: PyTorch not installed
    echo  Run: pip install torch numpy omegaconf
>>>>>>> parent of 5f68e8e (updated installg)
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