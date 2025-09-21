#!/bin/bash
# Comprehensive testing script for IS31FL373x Driver

# Don't exit on first error - we want to test all environments
set +e

echo "🧪 IS31FL373x Driver Tests"
echo "=========================="

# Track overall success
OVERALL_SUCCESS=true
FAILED_TESTS=()

# 1. Run comprehensive unit tests (the real validation)
echo "📋 Running Unit Tests..."
if pio test -e native_test --verbose; then
    echo "    ✅ Unit tests passed"
else
    echo "    ❌ Unit tests failed"
    OVERALL_SUCCESS=false
    FAILED_TESTS+=("unit_tests")
fi

echo ""
echo "🔨 Testing Hardware Compilation..."

# 2. Test PlatformIO example compilation for all environments
cd examples/PlatformIO_Basic

# Define environments to test
ENVIRONMENTS=("teensy41" "esp32")
# Note: pico environment disabled - WIP due to build issues

for env in "${ENVIRONMENTS[@]}"; do
    echo "  - Testing $env environment..."
    
    # Run the build and capture output for debugging
    if pio run -e "$env" --silent > /tmp/build_${env}.log 2>&1; then
        echo "    ✅ $env compilation successful"
    else
        echo "    ❌ $env compilation failed"
        echo "    📋 Last few lines of build log:"
        tail -5 /tmp/build_${env}.log | sed 's/^/       /'
        OVERALL_SUCCESS=false
        FAILED_TESTS+=("$env")
    fi
    
    # Clean up build cache to prevent cross-contamination
    pio run -t clean -e "$env" --silent > /dev/null 2>&1 || true
done

cd ../..

echo ""
echo "📊 Test Summary"
echo "==============="

if [ "$OVERALL_SUCCESS" = true ]; then
    echo "✅ All tests passed successfully!"
    exit 0
else
    echo "❌ Some tests failed:"
    for test in "${FAILED_TESTS[@]}"; do
        echo "   - $test"
    done
    echo ""
    echo "Please check the output above for details."
    exit 1
fi
