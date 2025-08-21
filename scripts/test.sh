#!/bin/bash
# Simplified testing script for IS31FL373x Driver

set -e

echo "🧪 IS31FL373x Driver Tests"
echo "=========================="

# 1. Run comprehensive unit tests (the real validation)
echo "📋 Running Unit Tests..."
pio test -e native_test --verbose

echo ""
echo "🔨 Testing Basic Compilation..."

# 2. Test PlatformIO example compilation (validates library integration)
echo "  - Testing PlatformIO example..."
cd examples/PlatformIO_Basic
pio run -e teensy41
echo "    ✅ Teensy 4.1 compilation successful"
cd ../..

echo ""
echo "✅ Core Tests Complete!"
echo ""
