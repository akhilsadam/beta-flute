#!/bin/bash
# QG Wavetable VST — Quick Build Script

set -e

echo "=========================================="
echo "QG Wavetable VST Build"
echo "=========================================="

# Check Python
echo "✓ Checking Python..."
python3 -c "import torch; print(f'  PyTorch version: {torch.__version__}')"
python3 -c "import qg; print('  QG package: OK')" || {
    echo "  ⚠ QG not installed, installing..."
    pip install git+https://github.com/akhilsadam/qg.git
}

# Build
echo ""
echo "✓ Building JUCE plugin..."
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release

echo ""
echo "=========================================="
echo "Build Complete!"
echo "=========================================="
echo ""
echo "Plugin location:"
echo "  $(pwd)/bin/QGWavetableVST_artefacts/"
echo ""
echo "Next: Copy to VST3 folder"
echo "  Linux:   ~/.vst3/"
echo "  macOS:   ~/Library/Audio/Plug-Ins/VST3/"
echo "  Windows: C:\\Program Files\\Common Files\\VST3\\"
echo ""
echo "Then load in your DAW!"
