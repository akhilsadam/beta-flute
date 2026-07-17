@echo off
REM QG Wavetable VST — Windows Build Script (SDK auto‑download with custom path option)
REM -------------------------------------------------------------------------------
REM This version adds a small wrapper around the SDK detection logic:
REM   1. You can pass the SDK folder as the first argument to the script.
REM   2. If no argument is supplied the script will try to locate an existing
REM      SDK folder or download it automatically (same behaviour as before).
REM   3. The chosen SDK path is stored in the variable JUCE_VST3_SDK_PATH and
REM      fed to CMake via -DJUCE_VST3_SDK_PATH=...
REM -------------------------------------------------------------------------------

REM -----------------------------------------------------------
REM Configurable download URL – change only if Steinberg updates it.
REM -----------------------------------------------------------
set "SDK_URL=https://developer.steinberg.press/downloads/sdk/VST3_SDK.zip"

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
    echo [*] VST3 SDK not found – attempting download...
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
        echo Then re‑run this script, optionally passing the folder path as an argument.
        pause
        exit /b 1
    )

    echo [*] Downloaded VST3_SDK.zip – extracting now...
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
    pause
    exit /b 1
)

echo [*] Using VST3 SDK at: "%JUCE_VST3_SDK_PATH%"

echo.
REM -----------------------------------------------------------
REM Build JUCE plugin
REM -----------------------------------------------------------
if exist build (
    rmdir /s /q build
)
mkdir build
cd build

REM -----------------------------------------------------------
REM Configure with CMake – inject the SDK path we just resolved
REM -----------------------------------------------------------
cmake .. -G "Visual Studio 17 2022" -DCMAKE_CXX_STANDARD=17 ^
    -DJUCE_VST3_SDK_PATH="%JUCE_VST3_SDK_PATH%" ^
    || (
        echo ERROR: CMake configuration failed
        pause
        exit /b 1
    )

REM -----------------------------------------------------------
REM Build Release
REM -----------------------------------------------------------
cmake --build . --config Release || (
    echo ERROR: Build failed
    pause
    exit /b 1
)

cd ..

echo.
echo =========================================================
echo Build Complete!
echo =========================================================
echo.
echo Plugin location:
echo    build\bin\QGWavetableVST_artefacts\VST3\
echo.
echo Next: copy the plugin to your VST3 host folder, e.g.
echo    C:\Program Files\Common Files\VST3\
echo.
echo Then restart your DAW and load the plugin!
echo.
pause