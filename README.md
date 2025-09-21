# Arduino Driver for IS31FL373x LED Matrices

A performance-focused, multi-chip Arduino driver for the Lumissil **IS31FL3733 (12x16)** and **IS31FL3737B (12x12)** LED matrix controllers. This library provides a unified interface for both chips with proper addressing to prevent hardware conflicts and LED aliasing issues.

## Key Features

* **Correct Chip-Specific Addressing:** Properly handles the different memory layouts of IS31FL3733 (12x16) vs IS31FL3737B (12x12) to prevent LED aliasing and register conflicts
* **Hardware Compatibility Mode:** Supports using IS31FL3737 hardware with IS31FL3733 drivers via coordinate offset configuration
* **Multi-Chip Canvas:** Seamlessly manage multiple driver chips as a single, large logical display for scrolling text or graphics
* **Multi-Level Brightness Control:** Three-tier automatic brightness scaling system (hardware current, gamma correction, global dimming)
* **ADDR Pin Constants:** Clean, readable addressing with `ADDR::GND`, `ADDR::VCC`, `ADDR::SDA`, `ADDR::SCL` constants
* **Performance Monitoring:** Built-in FPS calculation for optimization and debugging
* **Adafruit_GFX Integration:** Full compatibility with Adafruit GFX library for familiar graphics primitives (text, shapes, bitmaps)
* **Buffered Operations:** Frame buffer approach prevents flickering - all drawing operations update locally, then push to hardware with `show()`
* **Custom Layout Support:** Indexed drawing model with `setLayout()` for non-matrix arrangements (analog clocks, 14-segment displays, artistic patterns)
* **Dual Drawing Models:** Standard coordinate-based `drawPixel(x, y)` for matrices, plus indexed `setPixel(index)` for custom layouts

## Architectural Philosophy

The library is designed with a three-layer architecture to separate concerns and provide the right level of abstraction for any project:

1. **Core Device Layer:** Provides direct, low-level control over a single IS31FL373x chip's hardware features.
2. **Graphics Layer:** Integrates the Core Device with `Adafruit_GFX`, enabling a rich set of drawing functions on a single matrix.
3. **Canvas Manager Layer:** The highest-level abstraction, managing an array of Core Devices as one contiguous display canvas.

## Example Usage

#### Basic Usage (Single Matrix)

```cpp
#include "IS31FL373x.h"

// Use the appropriate class for your chip
IS31FL3737B matrix;  // For 12x12 IS31FL3737B chip
// IS31FL3733 matrix; // For 12x16 IS31FL3733 chip

void setup() {
  matrix.begin();
  matrix.setGlobalCurrent(128);     // Hardware brightness control
  matrix.setMasterBrightness(255);  // Software brightness scaling
}

void loop() {
  matrix.clear();
  matrix.drawPixel(5, 5, 255);      // Draw at (x=5, y=5) with max brightness
  matrix.drawLine(0, 0, 11, 11, 128); // Diagonal line
  matrix.show();                     // Push buffer to hardware
  delay(1000);
}
```

#### Modern Addressing with ADDR Constants

```cpp
#include "IS31FL373x.h"

// ✅ CLEAN: Use ADDR constants for readable addressing
IS31FL3737B matrix_3737(ADDR::GND);           // Single ADDR pin
IS31FL3733 matrix_3733(ADDR::VCC, ADDR::SDA); // Dual ADDR pins

void setup() {
  // Multi-level brightness control
  matrix_3737.begin();
  matrix_3737.setGlobalCurrent(128);      // Hardware current (power/heat)
  matrix_3737.setGammaCorrection(true);   // Perceptual linearity
  matrix_3737.setGlobalDimming(200);      // App-level brightness (0-255)
}

void loop() {
  matrix_3737.clear();
  matrix_3737.drawPixel(10, 5, 255);  // Automatic scaling: gamma → dimming → hardware
  matrix_3737.show();
  
  // Performance monitoring
  Serial.print("FPS: ");
  Serial.println(matrix_3737.getFPS());
  delay(1000);
}
```

## Complete Examples

The `examples/` folder contains three complete, buildable examples demonstrating real-world usage patterns:

### [LED_Sign_Multi_Chip.cpp](examples/LED_Sign_Multi_Chip.cpp)
**Multi-chip scrolling LED sign** - Based on the DisplayManager pattern
- 3-chip horizontal canvas (36x12 display)
- Light sensor brightness adaptation
- Multi-level brightness control demonstration
- Performance monitoring with FPS calculation

### [Segment_Display_Terminal.cpp](examples/Segment_Display_Terminal.cpp) 
**Large 14-segment display terminal** - Based on the 14-segment terminal pattern
- 16 chips with all ADDR combinations (32x6 characters)
- Custom coordinate mapping for segment displays
- Time-based brightness control
- Character set and pattern demonstrations

### [Analog_LED_Clock.cpp](examples/Analog_LED_Clock.cpp)
**Analog-style LED clock** - Based on the Orbitchron clock pattern  
- Single chip with IS31FL3737 hardware compatibility
- Direct coordinate access with automatic offset handling
- Smooth animations with gamma correction
- Power management for battery operation

#### Multi-Chip Canvas Example

```cpp
#include "IS31FL373x.h"

// Three chips with clean ADDR constants
IS31FL3737B matrix1(ADDR::GND), matrix2(ADDR::VCC), matrix3(ADDR::SDA);
IS31FL373x_Device* devices[] = {&matrix1, &matrix2, &matrix3};

IS31FL373x_Canvas canvas(36, 12, devices, 3, LAYOUT_HORIZONTAL);

void setup() {
  canvas.begin();                    // Initialize all chips
  canvas.setGlobalCurrent(100);      // Hardware brightness for all
  canvas.setGammaCorrection(true);   // Smooth transitions
}

void loop() {
  canvas.clear();
  canvas.setCursor(0, 2);
  canvas.print("SCROLLING TEXT");    // Text across all chips
  canvas.show();                     // Update all chips atomically
  delay(1000);
}
```

#### Custom Layout Mapping (Non-Matrix Arrangements)

For projects like analog clocks, 14-segment displays, or artistic LED arrangements, you can map LEDs to custom layouts using the indexed drawing model.

```cpp
#include "IS31FL373x.h"

IS31FL3737B matrix;

// Define custom layout: 12 LEDs arranged in a clock face
PixelMapEntry clockLayout[12] = {
  {6, 1},   // 12 o'clock -> CS6, SW1
  {9, 2},   // 1 o'clock  -> CS9, SW2  
  {12, 4},  // 2 o'clock  -> CS12, SW4
  {12, 6},  // 3 o'clock  -> CS12, SW6
  {12, 9},  // 4 o'clock  -> CS12, SW9
  {9, 11},  // 5 o'clock  -> CS9, SW11
  {6, 12},  // 6 o'clock  -> CS6, SW12
  {3, 11},  // 7 o'clock  -> CS3, SW11
  {1, 9},   // 8 o'clock  -> CS1, SW9
  {1, 6},   // 9 o'clock  -> CS1, SW6
  {1, 4},   // 10 o'clock -> CS1, SW4
  {3, 2}    // 11 o'clock -> CS3, SW2
};

void setup() {
  matrix.begin();
  
  // Apply the custom layout mapping
  matrix.setLayout(clockLayout, 12);
  
  matrix.setGlobalCurrent(128);
}

void loop() {
  matrix.clear();
  
  // Draw clock hands using logical indices (0-11)
  int hour = 3;     // 3 o'clock position  
  int minute = 6;   // 6 o'clock position
  
  matrix.setPixel(hour, 255);    // Hour hand - bright
  matrix.setPixel(minute, 128);  // Minute hand - dimmer
  matrix.setPixel(0, 64);        // 12 o'clock marker - dim
  
  matrix.show();
  delay(1000);
}
```

#### 14-Segment Display Example

```cpp
#include "IS31FL373x.h"

IS31FL3737B matrix;

// Map 14 segments of a single digit to hardware pins
PixelMapEntry segmentLayout[14] = {
  {1, 1}, {2, 1},   // A1, A2 (top segments)
  {3, 2}, {3, 3},   // B, C (right segments)  
  {2, 4}, {1, 4},   // D1, D2 (bottom segments)
  {1, 3}, {1, 2},   // E, F (left segments)
  {2, 2}, {2, 3},   // G1, G2 (middle segments)
  {3, 1}, {1, 5},   // H, J (diagonal segments)
  {2, 5}, {3, 4}    // K, L (diagonal segments)
};

// Segment patterns for digits (bit 0 = segment A1, bit 1 = segment A2, etc.)
uint16_t digitPatterns[10] = {
  0b0000110000111111,  // '0'
  0b0000000000000110,  // '1'
  0b0001100011011011,  // '2'
  // ... more patterns
};

void setup() {
  matrix.begin();
  matrix.setLayout(segmentLayout, 14);
  matrix.setGlobalCurrent(150);
}

void displayDigit(int digit) {
  matrix.clear();
  
  uint16_t pattern = digitPatterns[digit];
  for (int segment = 0; segment < 14; segment++) {
    if (pattern & (1 << segment)) {
      matrix.setPixel(segment, 255);  // Light up this segment
    }
  }
  
  matrix.show();
}

void loop() {
  for (int i = 0; i < 10; i++) {
    displayDigit(i);
    delay(500);
  }
}
```

## Custom Layout Design Guide

### PixelMapEntry Structure
```cpp
struct PixelMapEntry {
  uint8_t cs;  // Column/Source pin (1-16 for IS31FL3733, 1-12 for IS31FL3737B)  
  uint8_t sw;  // Switch/Row pin (1-12 for both chips)
};
```

### Design Process
1. **Physical Mapping:** Identify which LED connects to which (CS, SW) pin on your hardware
2. **Logical Ordering:** Decide on a logical sequence (e.g., clockwise for clock, left-to-right for digits)
3. **Create Array:** Build `PixelMapEntry` array mapping logical index → hardware pins
4. **Apply Layout:** Call `matrix.setLayout(yourLayout, arraySize)` 
5. **Use Indexed Drawing:** Draw using `matrix.setPixel(logicalIndex, brightness)`

### Benefits of Custom Layouts
- **Clean Code:** Rendering logic uses meaningful indices instead of hardware coordinates
- **Hardware Abstraction:** Change physical wiring without changing rendering code
- **Reusable Patterns:** Same layout can work across different projects
- **Easy Debugging:** Logical sequence makes it easy to test individual LEDs

## Chip Differences & Compatibility

| Feature | IS31FL3733 | IS31FL3737B |
|---------|------------|-------------|
| **Matrix Size** | 12×16 (192 LEDs) | 12×12 (144 LEDs) |
| **Register Layout** | 16-byte stride with all columns valid | 16-byte stride with gaps (CS13-16 invalid) |
| **I2C Address** | 2 ADDR pins (16 addresses) | 1 ADDR pin (16 addresses) |
| **Driver Class** | `IS31FL3733` | `IS31FL3737B` |

**⚠️ Important:** The IS31FL3737B uses a sparse register layout. Writing to non-existent CS13-16 addresses causes address pointer corruption and LED aliasing. This driver properly handles the differences.

## Platform Support

| Platform | Status | Notes |
|----------|--------|-------|
| **Teensy 4.1** | ✅ Fully Supported | Tested and working |
| **ESP32** | ✅ Fully Supported | Tested and working |
| **Raspberry Pi Pico** | 🚧 Work in Progress | Build issues under investigation |
| **Arduino Uno/Nano** | 🔄 Not Tested | Should work but untested |

## Installation & Testing

### Arduino IDE
1. Download and install this library
2. Install dependencies: `Adafruit_GFX` and `Adafruit_BusIO`
3. Open one of the examples: `File → Examples → IS31FL373x → LED_Sign_Multi_Chip`
4. Upload and test with your hardware

### PlatformIO
1. Add to `platformio.ini`:
```ini
lib_deps = 
    adafruit/Adafruit GFX Library
    adafruit/Adafruit BusIO
    # Add your IS31FL373x library here
```
2. Copy an example to your `src/main.cpp`
3. Build and upload: `pio run -t upload`

### Testing Your Setup
The examples are designed to be buildable and testable:
- **LED_Sign_Multi_Chip.cpp**: Test multi-chip canvas and brightness control
- **Segment_Display_Terminal.cpp**: Verify all 16 address combinations
- **Analog_LED_Clock.cpp**: Test hardware compatibility and coordinate mapping

## Dependencies

* Arduino `Wire.h` library
* `Adafruit_GFX.h` library  
* `Adafruit_BusIO.h` library

## Troubleshooting

### LED Aliasing Issues (IS31FL3737 hardware)

**Symptoms:** LEDs light up in unexpected locations (e.g., CS9 command affects CS7)
**Cause:** Writing to non-existent CS13-16 registers causes address pointer corruption
**Solution:** Use the correct driver class for your hardware:
- **IS31FL3737 hardware** → Use `IS31FL3737B` class (automatically handles 12x12 layout)
- **IS31FL3733 hardware** → Use `IS31FL3733` class (supports full 12x16 layout)

The driver automatically prevents writes to invalid registers when you use the correct class.

### Custom Layout Issues

**Wrong LEDs lighting up:** Check your `PixelMapEntry` array - ensure CS/SW values match your physical wiring
**Nothing lights up:** Verify `setLayout()` is called before drawing, and array size matches the number of entries
**Layout works partially:** Some entries may have invalid CS/SW coordinates (CS > chip width, SW > 12)

### Build Issues

**Pico Platform:** Currently experiencing dependency resolution issues.
**Missing Libraries:** Ensure Adafruit_GFX and Adafruit_BusIO are installed via Library Manager.

## Project Roadmap (Post V1.0)

* Complete Raspberry Pi Pico platform support
* Full API for the hardware **Auto-Breath Mode (ABM)** engine
* API for reading **Open/Short circuit detection** registers
* Support for the **audio modulation** feature
