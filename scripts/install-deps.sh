#!/bin/bash
# Development environment setup for IS31FL373x Driver

set -e

echo "🔧 Setting up IS31FL373x Driver Development Environment..."
echo "========================================================"

# Install PlatformIO if not present
if ! command -v pio &> /dev/null; then
    echo "📦 Installing PlatformIO..."
    pip install --upgrade platformio
else
    echo "✅ PlatformIO already installed"
fi

# Install library dependencies
echo "📚 Installing library dependencies..."
pio pkg install

echo "🔍 Checking platforms..."
pio platform install native
pio platform install teensy  
pio platform install atmelavr
pio platform install espressif32

echo ""
echo "✅ Development environment ready!"
echo ""
echo "🚀 Quick commands:"
echo "  - Run tests:           ./scripts/test.sh"
echo "  - Run specific test:   pio test -e native_test"
echo "  - Compile for Teensy:  pio run -e teensy41"
echo "  - Clean build cache:   pio run --target clean"