# Arduino Driver for IS31FL373x LED Matrices

A performance-focused, multi-chip Arduino driver for the Lumissil **IS31FL3733 (12x16)**, **IS31FL3737 (12x12)**, and **IS31FL3737B (12x12)** LED matrix controllers. This library provides a unified API for all three chips, with robust, model-specific code to prevent hardware conflicts and visual artifacts.

## Key Features

*   **Chip-Specific Drivers:** Dedicated classes for each chip model correctly handle different memory maps and feature sets, preventing common aliasing issues.
*   **Multi-Chip Canvas:** Seamlessly manage multiple driver chips as a single, large logical display for scrolling text or graphics.
*   **Advanced Brightness Control:**
    *   **Hardware Current:** Sets the maximum brightness via the Global Current Control register.
    *   **Software Master Brightness:** Easy application-level brightness scaling.
*   **Adafruit_GFX Integration:** Full compatibility for familiar graphics primitives (text, shapes, bitmaps).
*   **Buffered Operations:** A frame buffer prevents flickering; drawing operations update a local buffer, then push to hardware atomically with `show()`.
*   **Custom Layout Support:** An indexed drawing model (`setLayout()`, `setPixel(index)`) for non-matrix arrangements like analog clocks or 14-segment displays.
*   **State Inspection:** Methods to verify configuration and buffer state for debugging.

## Chip Comparison & Driver Selection

**Important:** These are physically distinct chips. You **must** use the driver class that exactly matches your hardware to ensure correct operation.

| Physical Chip | Driver Class | Matrix Size | I2C Address Pins | I2C Addresses | Key Hardware Feature |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **IS31FL3733** | `IS31FL3733` | 12×16 (192 LEDs) | 2 (`ADDR1`, `ADDR2`) | **16** | Largest matrix |
| **IS31FL3737** | `IS31FL3737` | 12×12 (144 LEDs) | 1 (`ADDR`) | **4** | Fixed PWM Frequency |
| **IS31FL3737B**| `IS31FL3737B`| 12×12 (144 LEDs) | 1 (`ADDR`) | **4** | **Selectable PWM Frequency** |

### How to Choose the Right Chip for Your Project:

1.  **Need maximum LEDs or 16 I2C addresses?** → **IS31FL3733**
2.  **Building a simple 12x12 matrix and don't need PWM control?** → **IS31FL3737**
3.  **Need to adjust PWM frequency (e.g., to avoid camera flicker or audible noise)?** → **IS31FL3737B**

## API Reference

The full API reference is available in the [IS31FL373x-reference.md](doc/IS31FL373x-reference.md) file.

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

  // For IS31FL3737B only:
  // matrix.setPWMFrequency(PWM_FREQ_26_7_KHZ);
}

void loop() {
  matrix.clear();
  matrix.drawPixel(5, 5, 255);      // Draw at (x=5, y=5) with max brightness
  matrix.show();                     // Push buffer to hardware
  
  // FPS monitoring would require implementation
  Serial.println("Frame updated");
  delay(100);
}
```

#### Multi-Chip Canvas

```cpp
#include "IS31FL373x.h"

// Three IS31FL3737 chips with different addresses
IS31FL3737 matrix1(ADDR::GND), matrix2(ADDR::VCC), matrix3(ADDR::SDA);
IS31FL373x_Device* devices[] = {&matrix1, &matrix2, &matrix3};

IS31FL373x_Canvas canvas(36, 12, devices, 3, LAYOUT_HORIZONTAL);

void setup() {
  canvas.begin();
  canvas.setGlobalCurrent(100);
}

void loop() {
  canvas.clear();
  canvas.setCursor(0, 2);
  canvas.print("SCROLLING TEXT");
  canvas.show();
}
```

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
*   Add software gamma correction to the library, to allow calibration of the LED brightness.
*   API for reading **Open/Short circuit detection** registers for diagnostics.
*   Complete Raspberry Pi Pico platform support.