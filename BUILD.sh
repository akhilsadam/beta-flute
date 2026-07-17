#!/bin/bash
# QG Wavetable VST — Quick Build Script
# -------------------------------------------------------------------------------
# Updated to automatically download the Steinberg VST3 SDK if it is not already
# present. This fixes the “Use juce_set_vst2_sdk_path to set up the VST sdk
# before adding VST targets” CMake error.
# -------------------------------------------------------------------------------

set -e

echo "=========================================="
echo "QG Wavetable VST Build"
echo "=========================================="

# -------------------------------------------------------------------------------
# 1️⃣  Locate (or download) the VST SDK
# -------------------------------------------------------------------------------
# The SDK is expected to be extracted into a folder named something like
#   VST3_SDK-<version>
# in the parent directory of this script. If it is missing we will download
# the official ZIP from Steinberg, unzip it, and use the first folder that
# matches the pattern as the SDK root.
# -------------------------------------------------------------------------------
SDK_ZIP="VST3_SDK.zip"
SDK_URL="https://developer.steinberg.press/downloads/sdk/VST3_SDK.zip"
VST_SDK_ROOT=""

# Check if a known SDK folder already exists (e.g. VST3_SDK-*)
for candidate in ../VST3_SDK*; do
    if [[ -d "$candidate" && -f "$candidate/ReadMe.txt" ]]; then
        VST_SDK_ROOT="$(realpath "$candidate")"
        break
    fi
done

if [[ -z "$VST_SDK_ROOT" ]]; then
    echo "⚠ VST SDK not found locally."
    echo "   Attempting automatic download from $SDK_URL ..."
    echo

    # Ensure we have a downloader
    if command -v curl >/dev/null 2>&1; then
        downloader="curl -L -f -o"
    elif command -v wget >/dev/null 2>&1; then
        downloader="wget -q -O"
    else
        echo "❌ Neither 'curl' nor 'wget' is installed – please install one."
        exit 1
    fi

    # Download the zip
    $downloader "$SDK_ZIP" "$SDK_URL"
    if [[ ! -f "$SDK_ZIP" ]]; then
        echo "❌ Failed to download $SDK_URL"
        exit 1
    fi

    # Extract – the archive creates a folder that starts with VST3_SDK
    unzip -q "$SDK_ZIP" -D .
    extracted_dir=$(find . -maxdepth 1 -type d -name "VST3_SDK*" | head -n1)
    if [[ -z "$extracted_dir" ]]; then
        echo "❌ Extraction did not produce a VST3_SDK* folder."
        exit 1
    fi
    VST_SDK_ROOT="$(realpath "$extracted_dir")"
    echo "✓ Downloaded and extracted SDK to: $VST_SDK_ROOT"
fi

# Export the path so JUCE can see it
export JUCE_VST2_SDK_PATH="${VST_SDK_ROOT}"
echo "✓ VST SDK path set for CMake: $VST_SDK_ROOT"

# -------------------------------------------------------------------------------
# 2️⃣  Python & QG dependencies
# -------------------------------------------------------------------------------
echo ""
echo "✓ Checking Python environment..."
python3 -c "import torch; print(f'  PyTorch version: {torch.__version__}')" || {
    echo "  ⚠ torch not installed – installing..."
    python3 -m pip install torch
}
python3 -c "import qg; print('  QG package: OK')" || {
    echo "  ⚠ QG package not installed – installing..."
    python3 -m pip install git+https://github.com/akhilsadam/qg.git
}

# -------------------------------------------------------------------------------
# 3️⃣  Build the plugin
# -------------------------------------------------------------------------------
echo ""
echo "✓ Building JUCE plugin..."
mkdir -p build
cd build

# Configure – tell CMake where the SDK lives
cmake .. -DCMAKE_BUILD_TYPE=Release -DJUCE_VST2_SDK_PATH="${JUCE_VST2_SDK_PATH}" \
    || { echo "❌ CMake configuration failed"; exit 1; }

cmake --build . --config Release || { echo "❌ Build failed"; exit 1; }

# -------------------------------------------------------------------------------
# 4️⃣  Finish up
# -------------------------------------------------------------------------------
cd ..
echo ""
echo "=========================================="
echo "Build Complete!"
echo "=========================================="
echo ""
echo "Plugin location:"
echo "  $(pwd)/bin/QGWavetableVST_artefacts/"
echo ""
echo "Next: Copy to your VST3 host folder:"
echo "  Linux:   ~/.vst3/"
echo "  macOS:   ~/Library/Audio/Plug-Ins/VST3/"
echo "  Windows: C:\\Program Files\\Common Files\\VST3\\"
echo ""
echo "Then load the plugin in your DAW."