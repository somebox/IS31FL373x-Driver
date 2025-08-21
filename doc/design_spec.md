# Design Specification

## Unified Driver for IS31FL373x LED Matrix Controllers

### **1.0 Overview**

This document specifies a focused, minimal C++ library for the Arduino platform to control Lumissil IS31FL3733 (12x16) and IS31FL3737B (12x12) LED matrix driver ICs. The design prioritizes practical functionality required by real-world projects: multi-chip displays, custom hardware layouts, and performance through buffered operations.

**Core Requirements Based on Real Usage:**
- Support IS31FL3737 hardware using IS31FL3733 drivers (hardware compatibility)
- Multi-chip canvas management for large displays (3+ chips)
- Custom coordinate mapping for non-standard layouts (14-segment displays)
- Bulk buffer operations for performance
- Hardware-specific address calculation and coordinate transformations

### **2.0 Core Architecture**

Simple object-oriented design with focus on hardware compatibility and performance.

**2.1. Base Class: `IS31FL373x_Device`**
Abstract base class for shared I2C operations and buffering.
* **Core Responsibilities:**
  * I2C communication with page selection and register operations
  * Frame buffer management and bulk updates
  * Hardware coordinate transformation support
  * Global brightness control
* **Essential Methods:**
  * `bool begin()` - Initialize device with proper I2C sequences
  * `void show()` - Bulk transfer frame buffer to device PWM registers
  * `void setGlobalCurrent(uint8_t current)` - Hardware brightness control
  * `void setPixel(uint16_t index, uint8_t pwm)` - Direct buffer access
  * `void setCoordinateOffset(uint8_t cs_offset, uint8_t sw_offset)` - Hardware compatibility

**2.2. Device Classes: `IS31FL3733` and `IS31FL3737B`**
Concrete implementations with chip-specific address calculation.
* **`IS31FL3733`:** 16x12 matrix, dual ADDR pin addressing (supports 16 unique addresses)
* **`IS31FL3737B`:** 12x12 matrix, single ADDR pin addressing (supports 16 unique addresses)
* **Hardware Compatibility:** Both classes support IS31FL3737 physical hardware through coordinate offset configuration

**2.3. ADDR Pin Constants**
Standardized constants for hardware addressing:
```cpp
enum class ADDR {
    GND = 0,    // Connected to ground
    VCC = 1,    // Connected to VCC
    SDA = 2,    // Connected to SDA line
    SCL = 3     // Connected to SCL line
};

// Usage examples:
IS31FL3733 matrix1(ADDR::GND, ADDR::VCC);     // Address: 0x54
IS31FL3733 matrix2(ADDR::SDA, ADDR::SCL);     // Address: 0x5B
IS31FL3737B matrix3(ADDR::VCC);               // Address: 0x51
```

### **3.0 Buffering and Addressing**

Simple frame buffer approach optimized for real-world usage patterns.

**3.1. Frame Buffer Management**
* **Buffer:** Private `uint8_t` array sized to chip capacity (192 bytes for 3733, 144 bytes for 3737B)
* **Update Strategy:** `show()` method performs single bulk I2C transfer of entire buffer to PWM registers
* **Addressing:** Primary focus on indexed model (`setPixel(index, pwm)`) for maximum flexibility

**3.2. Coordinate Systems**
* **Indexed Access:** `setPixel(uint16_t index, uint8_t pwm)` - Direct buffer manipulation
* **Cartesian Access:** `drawPixel(int16_t x, int16_t y, uint16_t color)` - GFX compatibility 
* **Custom Mapping:** `setLayout(PixelMapEntry* layout, uint16_t size)` - User-defined coordinate transforms
* **Hardware Offsets:** Automatic CS/SW coordinate adjustment for hardware compatibility

**3.3. Bulk Operations**
Essential for performance in multi-chip scenarios:
* `void setPWM(uint8_t* buffer)` - Direct buffer loading (legacy compatibility)
* `void clear()` - Fast buffer reset
* `void dimBuffer(uint8_t amount)` - Software brightness scaling across entire buffer

### **4.0 Multi-Chip Canvas Management**

Essential for supporting large displays with multiple driver chips.

**4.1. Canvas Architecture**
* **Purpose:** Manage arrays of IS31FL373x devices as single logical display
* **Real-world requirement:** Support 3-16 chips in various physical arrangements
* **Inheritance:** Extends `Adafruit_GFX` for familiar graphics API

**4.2. Canvas Class: `IS31FL373x_Canvas`**
```cpp
IS31FL373x_Canvas(uint16_t width, uint16_t height, 
                  IS31FL373x_Device** devices, uint8_t deviceCount,
                  CanvasLayout layout = LAYOUT_HORIZONTAL);
```

**Key Features:**
* **Device Management:** Coordinate mapping to individual chips
* **Unified Control:** Single `show()`, `clear()`, brightness control for all devices
* **Layout Support:** Horizontal, vertical, and custom arrangements
* **GFX Integration:** Standard drawing primitives across entire canvas

**4.3. Layout Calculation**
Based on analysis of real projects:
* **Horizontal Layout:** Simple left-to-right arrangement (DisplayManager pattern)
* **Grid Layout:** 2D arrangement of modules (14-segment terminal pattern)
* **Custom Layouts:** User-defined coordinate transformation functions

### **5.0 Essential Features**

Focus on functionality required by real-world projects.

**5.1. Multi-Level Brightness Control**
Comprehensive brightness management system with automatic scaling:

* **Level 1 - Hardware Current:** `setGlobalCurrent(uint8_t current)` - IC-level current control (0-255)
  - Sets overall power consumption and maximum brightness capability
  - Directly controls the Global Current Control register

* **Level 2 - Gamma Correction:** `setGammaCorrection(bool enabled)` - Perceptual linearity
  - When enabled, applies gamma lookup table to all PWM values
  - Provides visually linear brightness progression from 0-255

* **Level 3 - Global Dimming:** `setGlobalDimming(uint8_t level)` - Application-level brightness (0-255)
  - Context-sensitive brightness control (light sensor, time of day, power management)
  - Sets the effective maximum brightness, all values scaled proportionally
  - Example: `setGlobalDimming(128)` makes 255 → 128, 100 → 50, etc.

* **Automatic Scaling Pipeline:**
```cpp
// User calls: setPixel(index, 200)
// Internal processing:
uint8_t final_pwm = user_value;                    // 200
if (gamma_enabled) final_pwm = gamma_lut[final_pwm]; // Apply gamma
final_pwm = (final_pwm * global_dimming) / 255;    // Apply dimming
// Result written to buffer and transferred to hardware
```

* **User Benefits:**
  - Simple 0-255 interface - no manual scaling required
  - Automatic brightness adaptation without code changes
  - Consistent visual experience across different environments

**5.2. Hardware Compatibility**
* **Coordinate Offsets:** Support for IS31FL3737 hardware using IS31FL3733 drivers
* **Address Calculation:** Flexible addressing to support various ADDR pin configurations
* **I2C Interface:** Function pointer support for custom I2C implementations (legacy compatibility)

**5.3. Performance Monitoring**
* **FPS Calculation:** `float getFPS()` - Real-time performance measurement
  - Automatic tracking of `show()` call frequency
  - Useful for optimization and debugging multi-chip displays
  - Resets every second for current performance reading

**5.4. Initialization and Control**
* **Device Initialization:** Proper reset sequences and register configuration
* **Page Management:** Automatic switching between LED control, PWM, and function pages
* **Basic Power Management:** Shutdown and reset functionality

### **6.0 Practical Usage Examples**

Based on real-world project requirements.

**6.1. LED Sign (DisplayManager Pattern)**
```cpp
#include "IS31FL373x.h"

// Three IS31FL3737B chips using 3733 driver for hardware compatibility  
IS31FL3733 board1(ADDR::GND, ADDR::GND);  
IS31FL3733 board2(ADDR::VCC, ADDR::VCC);  
IS31FL3733 board3(ADDR::SDA, ADDR::SCL);

IS31FL373x_Device* devices[] = {&board1, &board2, &board3};
IS31FL373x_Canvas sign(48, 6, devices, 3, LAYOUT_HORIZONTAL);

int lightSensor = A0;

void setup() {
  sign.begin();
  sign.setGlobalCurrent(100);        // Hardware current limit
  sign.setGammaCorrection(true);     // Enable gamma correction
  // Configure for IS31FL3737 hardware
  board1.setCoordinateOffset(2, 0);  // CS adjustment
  board2.setCoordinateOffset(2, 0);
  board3.setCoordinateOffset(2, 0);
}

void loop() {
  // Automatic brightness adaptation
  int light_level = analogRead(lightSensor);
  uint8_t dimming = map(light_level, 0, 1023, 50, 255);
  sign.setGlobalDimming(dimming);    // Automatic scaling applied
  
  sign.clear();
  sign.setCursor(0, 0);
  sign.print("HELLO WORLD");
  sign.show();
  
  // Performance monitoring
  Serial.print("FPS: ");
  Serial.println(sign.getFPS());
  delay(1000);
}
```

**6.2. 14-Segment Display (Custom Layout)**
```cpp
#include "IS31FL373x.h"

IS31FL3733 drivers[16] = {
  IS31FL3733(ADDR::GND, ADDR::GND), IS31FL3733(ADDR::GND, ADDR::VCC), 
  IS31FL3733(ADDR::GND, ADDR::SDA), IS31FL3733(ADDR::GND, ADDR::SCL),
  IS31FL3733(ADDR::VCC, ADDR::GND), IS31FL3733(ADDR::VCC, ADDR::VCC),
  IS31FL3733(ADDR::VCC, ADDR::SDA), IS31FL3733(ADDR::VCC, ADDR::SCL),
  IS31FL3733(ADDR::SDA, ADDR::GND), IS31FL3733(ADDR::SDA, ADDR::VCC),
  IS31FL3733(ADDR::SDA, ADDR::SDA), IS31FL3733(ADDR::SDA, ADDR::SCL),
  IS31FL3733(ADDR::SCL, ADDR::GND), IS31FL3733(ADDR::SCL, ADDR::VCC),
  IS31FL3733(ADDR::SCL, ADDR::SDA), IS31FL3733(ADDR::SCL, ADDR::SCL)
};

// Custom coordinate mapping for segment displays
void drawSegmentPattern(uint8_t board, uint8_t pos, uint16_t pattern, uint8_t level) {
  for (int bit = 0; bit < 16; bit++) {
    if (pattern & (1 << bit)) {
      uint16_t index = pos * 16 + bit;
      drivers[board].setPixel(index, level);  // Automatic brightness scaling applied
    }
  }
}

void setup() {
  for (int i = 0; i < 16; i++) {
    drivers[i].begin();
    drivers[i].setGlobalCurrent(128);    // Hardware current control
    drivers[i].setGammaCorrection(true); // Enable perceptual linearity
  }
  
  // Time-based dimming example
  uint8_t hour = 14; // 2 PM
  uint8_t dimming = (hour >= 8 && hour <= 18) ? 255 : 100; // Bright during day
  for (int i = 0; i < 16; i++) {
    drivers[i].setGlobalDimming(dimming);
  }
}
```

**6.3. Analog Clock (Hardware-Specific Mapping)**
```cpp
#include "IS31FL373x.h"

IS31FL3733 matrix(ADDR::GND, ADDR::GND);  // Single chip

void setup() {
  matrix.begin();
  matrix.setCoordinateOffset(2, 0);  // IS31FL3737 hardware compatibility
  matrix.setGlobalCurrent(240);      // Hardware brightness
  matrix.setGammaCorrection(true);   // Smooth LED transitions
  
  // Power management for battery operation
  matrix.setGlobalDimming(180);      // Reduce power consumption
}

void setPixel(int cs, int sw, int value) {
  // Direct coordinate access with hardware offset handling
  uint16_t index = matrix.coordToIndex(cs, sw);
  matrix.setPixel(index, value);     // Automatic gamma + dimming applied
}

void loop() {
  matrix.clear();
  
  // Clock rendering logic with full 0-255 brightness range
  setPixel(6, 6, 255);    // Center dot - full brightness
  setPixel(6, 2, 200);    // Hour hand - bright
  setPixel(6, 1, 150);    // Minute hand - medium
  setPixel(6, 0, 100);    // Second hand - dim
  
  matrix.show();
  
  // Monitor performance
  if (millis() % 5000 == 0) {
    Serial.print("Clock FPS: ");
    Serial.println(matrix.getFPS());
  }
  
  delay(100);
}
```
