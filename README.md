# Arduino Driver for IS31FL373x LED Matrices

This document outlines the design for a performance-focused, multi-chip Arduino driver for the Lumissil **IS31FL3733 (12x16)** and **IS31FL3737B (12x12)** LED matrix controllers. The architecture is derived from real-world use cases to provide a flexible and developer-friendly interface for both simple and complex projects.

## Key Features

*   **Unified Chip Support:** A single library provides a consistent API for both the IS31FL3733 and IS31FL3737B.
*   **High-Level Canvas:** Seamlessly manage multiple driver chips as a single, large logical display for scrolling text or large graphics.
*   **Custom Layout Mapping:** Go beyond the grid. Provides tools to easily drive non-matrix layouts like analog clock faces, 14-segment digits, or artistic LED arrangements.
*   **Adafruit_GFX Compatibility:** Full integration with the Adafruit GFX library provides a powerful and familiar set of graphics primitives, including text, shapes, and bitmaps.
*   **Layered Brightness Control:** Manage both the hardware's global current (master brightness) and apply a separate software-based intensity scaling for dynamic effects and fades.
*   **Performance Focused:** All drawing operations write to a local frame buffer, allowing the entire display to be updated in a single, efficient I2C transaction to prevent flickering and tearing.
*   **Developer Utilities:** Includes helper functions to visually identify the physical order of chained devices, simplifying setup and debugging.

## Architectural Philosophy

The library is designed with a three-layer architecture to separate concerns and provide the right level of abstraction for any project:

1.  **Core Device Layer:** Provides direct, low-level control over a single IS31FL373x chip's hardware features.
2.  **Graphics Layer:** Integrates the Core Device with `Adafruit_GFX`, enabling a rich set of drawing functions on a single matrix.
3.  **Canvas Manager Layer:** The highest-level abstraction, managing an array of Core Devices as one contiguous display canvas.

## Example Usage

#### Basic Usage (Single Matrix with GFX)
This is the simplest use case, treating a single chip as a standard graphics display.
```cpp
#include "IS31FL3737B.h" // Use the driver for your specific chip

IS31FL3737B matrix;

void setup() {
  matrix.begin();
  matrix.setGlobalCurrent(128); // Set master brightness to ~50%
}

void loop() {
  matrix.clear();
  matrix.setCursor(1, 2);
  matrix.print("OK");
  matrix.drawCircle(6, 6, 5, 255);
  matrix.show(); // Push the buffer to the chip
  delay(1000);
}
```

#### Multi-Chip Canvas (Scrolling Text Sign)
Combine multiple matrices into a single, logical display for complex rendering.
```cpp
#include "IS31FL373x_Canvas.h"
#include "IS31FL3737B.h"

IS31FL3737B matrix1(0x50), matrix2(0x51), matrix3(0x52);
IS31FL373x_Device* devices[] = {&matrix1, &matrix2, &matrix3};

// Create a 36x12 logical canvas from three 12x12 chips
IS31FL373x_Canvas sign(36, 12, devices, 3);

void setup() {
  sign.begin(); // Initializes all 3 chips
  sign.print("Loading...");
  sign.show(); // Updates all 3 chips
}
```

#### Custom Layouts (Analog Clock Face)
Break free from the grid by mapping LEDs to a logical index for custom hardware.
```cpp
#include "IS31FL3737B.h"

IS31FL3737B driver;

// User defines a map of their 60 clock LEDs to the chip's (cs, sw) pins
PixelMapEntry clock_layout[60] = {
  {1, 1}, {2, 1}, {3, 1}, // ... and so on for all 60 LEDs
};

void setup() {
  driver.begin();
  driver.setLayout(clock_layout, 60); // Apply the custom layout map
}

void loop() {
  driver.clear();
  // Draw the second hand using the logical index (0-59)
  int second_index = second();
  driver.drawPixel(second_index, 255); // Uses the indexed drawing model
  driver.show();
  delay(1000);
}
```

## Supported Hardware
*   Lumissil IS31FL3733 (12x16 Matrix Driver)
*   Lumissil IS31FL3737B (12x12 Matrix Driver)

## Dependencies
*   Arduino `Wire.h` library
*   `Adafruit_GFX.h` library

## Project Roadmap (Post V1.0)
*   Full API for the hardware **Auto-Breath Mode (ABM)** engine.
*   API for reading **Open/Short circuit detection** registers.
*   Support for the **audio modulation** feature.