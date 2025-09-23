# IS31FL373x LED Matrix Driver

Arduino driver for Lumissil **IS31FL3733**, **IS31FL3737**, and **IS31FL3737B** LED matrix controllers. Provides unified API with chip-specific optimizations and hardware quirk handling.

## Features

- **Chip-specific drivers** prevent addressing conflicts and visual artifacts
- **Multi-chip canvas** for large displays with seamless coordinate mapping  
- **Adafruit_GFX compatible** for familiar graphics primitives
- **Buffered operations** eliminate flickering with atomic updates
- **Custom layouts** for non-matrix arrangements (clocks, 7-segment displays)
- **Hardware + software brightness control** for flexible dimming

## Supported Chips

| Chip | Driver Class | Matrix | I2C Addresses | Key Feature |
|------|--------------|--------|---------------|-------------|
| **IS31FL3733** | `IS31FL3733` | 12×16 (192 LEDs) | 16 | Largest matrix |
| **IS31FL3737** | `IS31FL3737` | 12×12 (144 LEDs) | 4 | Hardware quirk handling* |
| **IS31FL3737B** | `IS31FL3737B` | 12×12 (144 LEDs) | 4 | Selectable PWM frequency |

**Important:** Use the exact driver class for your hardware chip - they are not interchangeable.

*IS31FL3737 has register mapping gaps that the driver handles automatically.

## Quick Start

```cpp
#include "IS31FL373x.h"

IS31FL3737B matrix(ADDR::GND);  // Match your chip!

void setup() {
  matrix.begin();
  matrix.setGlobalCurrent(128);
}

void loop() {
  matrix.clear();
  matrix.drawPixel(5, 5, 255);
  matrix.show();
  delay(100);
}
```

## Documentation

- [Complete API Reference](doc/API.md)
- [Hardware Reference](doc/IS31FL373x-reference.md)
- Adafruit GFX core used for drawing primitives: [Adafruit-GFX-Library](https://github.com/adafruit/Adafruit-GFX-Library)

## Example Usage

#### Basic Initialization (Single Matrix)

```cpp
#include "IS31FL373x.h"

// Choose the class that matches your hardware chip. They are NOT interchangeable!
// IS31FL3733 matrix(ADDR::GND, ADDR::VCC); // For IS31FL3733 chip (12x16)
// IS31FL3737 matrix(ADDR::GND);               // For IS31FL3737 chip (12x12)
IS31FL3737B matrix(ADDR::GND);                  // For IS31FL3737B chip (12x12)

void setup() {
  matrix.begin();
  matrix.setGlobalCurrent(128);     // Set hardware current (0-255)
  matrix.setMasterBrightness(255);  // Set application brightness (software, 0-255)

  // For IS31FL3737B only (not yet fully implemented):
  // matrix.setPWMFrequency(7); // 0..7 per datasheet
}

void loop() {
  matrix.clear();
  matrix.drawPixel(5, 5, 255);      // Draw at (x=5, y=5) with max brightness
  // Any Adafruit_GFX primitive is available:
  matrix.drawLine(0, 0, 11, 11, 200);
  matrix.drawRect(2, 2, 8, 6, 180);
  matrix.setCursor(0, 0);
  matrix.print("Hi");
  matrix.show();                     // Push buffer to hardware
  
  // FPS monitoring would require implementation
  Serial.println("Frame updated");
  delay(100);
}
```

#### Multi-Board Canvas (Build a Larger XY Matrix)

```cpp
#include "IS31FL373x.h"

// Example: 3 boards of 12×12 each → a 36×12 logical matrix
IS31FL3737 left(ADDR::GND), middle(ADDR::VCC), right(ADDR::SDA);
IS31FL373x_Device* devices[] = { &left, &middle, &right };
IS31FL373x_Canvas canvas(36, 12, devices, 3, LAYOUT_HORIZONTAL);

void setup() {
  canvas.begin();
  canvas.setGlobalCurrent(100);
}

void loop() {
  // Draw on the 36×12 logical surface; the canvas routes to the right board
  canvas.clear();
  // A pixel on the middle board (x in [12, 23])
  canvas.drawPixel(18, 5, 255);
  // A pixel on the right board (x in [24, 35])
  canvas.drawPixel(30, 8, 128);
  // Adafruit_GFX primitives work on the canvas, too:
  canvas.drawLine(0, 0, 35, 11, 90);
  canvas.setCursor(0, 0);
  canvas.print("HELLO");
  canvas.show();
}
```

For vertical stacking, use `LAYOUT_VERTICAL` and set the canvas size to `(max(widths), sum(heights))`. See the API doc “Canvas Sizing and Routing” for sizing and routing rules.

## Coordinate System & Register Mapping

- **User coordinates**: `(x, y)` with origin at the top-left. `x` increases to the right, `y` increases downward.
- **Chip dimensions**:
  - `IS31FL3733`: width 16, height 12
  - `IS31FL3737 / IS31FL3737B`: width 12, height 12
- **Register stride**: All supported chips use a hardware register stride of 16 bytes per row (even when width is 12). The driver maps `(x, y)` to a hardware register address via:

```cpp
// 1-based hardware pins (CSx, SWy), with optional offsets applied
address = (SWy - 1) * 16 + (CSx - 1);
```

- **IS31FL3737 quirk**: Columns 6–11 (0-based) are remapped and occupy addresses 8–13 (gap of +2). The driver applies this automatically. Example (row 0):
  - `(x=5, y=0) → address 5`
  - `(x=6, y=0) → address 8` (remapped)
  - `(x=11, y=0) → address 13` (remapped)

- **Offsets**: `setCoordinateOffset(csOffset, swOffset)` shifts the mapping by hardware pin offsets (used for compatibility across boards or wiring variants). Offsets apply before stride mapping and, for IS31FL3737, before the quirk remap.

### Custom Layouts (Non-Matrix)

If your LEDs are not arranged in a rectangular matrix (e.g., a ring or a 7‑segment display), define a `PixelMapEntry[]` that maps each logical index to a physical `(CS, SW)` pin. When a custom layout is set, `show()` writes each index to its mapped register address using the same stride and quirk handling.

 

## Testing

- Native unit tests (doctest):

```bash
pio test -e native_test           # concise PlatformIO summary
pio test -e native_test -v        # full doctest summary
```

- Full CI-like run with a short doctest summary and hardware example builds:

```bash
./scripts/test.sh
```

Notes:
- Native tests use a mock I2C layer and validate register/page transactions.
- Hardware environments are compile-tested by the script; unit tests run only under the native environment.

#### Custom Layout Mapping (e.g., Analog Clock)

This powerful feature abstracts the physical wiring from your drawing logic.

```cpp
#include "IS31FL373x.h"

IS31FL3737 matrix; // Using a 12x12 chip for this layout

// Map 12 logical indices (0-11 for hours) to physical (CS, SW) pins
PixelMapEntry clockLayout[12] = {
  {6, 1}, {9, 2}, {12, 4}, {12, 6}, {12, 9}, {9, 11},
  {6, 12}, {3, 11}, {1, 9}, {1, 6}, {1, 4}, {3, 2}
};

void setup() {
  matrix.begin();
  matrix.setLayout(clockLayout, 12); // Apply the custom map
  matrix.setGlobalCurrent(128);
}

void loop() {
  matrix.clear();
  // Draw using logical indices, not (x,y) coordinates
  int hour = 3;
  matrix.setPixel(hour, 255); // Light up the LED at logical index 3
  matrix.show();
  delay(1000);
}
```

## Installation

1.  Install this library via the Arduino Library Manager or by cloning this repository.
2.  Install dependencies from the Library Manager: `Adafruit GFX Library` and `Adafruit BusIO`.
3.  Open an example sketch from `File → Examples → IS31FL373x` that matches your hardware setup.

## Troubleshooting

#### LED Aliasing (LEDs light up in wrong locations)

*   **Symptom:** Commanding one LED (e.g., `drawPixel(9, 0, 255)`) causes a different LED (e.g., at x=7) to light up instead or in addition.
*   **Cause:** You are using the wrong driver class for your physical chip (e.g., `IS31FL3733` class with an `IS31FL3737` chip). The driver is writing to memory addresses that don't exist, causing the chip's internal address pointer to corrupt.
*   **Solution:** **Instantiate the class that exactly matches your hardware.**
    *   If you have an IS31FL3737 chip, you **must** use `IS31FL3737 myMatrix;`.
    *   If you have an IS31FL3733 chip, you **must** use `IS31FL3733 myMatrix;`.

#### Custom Layout Not Working

*   Ensure your `PixelMapEntry` array correctly maps to your physical wiring (CS and SW pins are 1-indexed).
*   Verify that `setLayout()` is called in `setup()` before any drawing commands.
*   Check that the coordinates in your map are valid for your chip (e.g., `cs` cannot be greater than 12 for an IS31FL3737).

## Project Roadmap

*   Full support for the hardware **Auto-Breath Mode (ABM)** engine.
*   Complete support for the hardware **PWM Frequency Selector** for the IS31FL3737B.
*   Add software gamma correction to the library, to allow calibration of the LED brightness.
*   API for reading **Open/Short circuit detection** registers for diagnostics.
*   Complete Raspberry Pi Pico platform support.