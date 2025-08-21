# Exploring Use Cases

### Example Real-World Use Cases

- The [DisplayManager](https://raw.githubusercontent.com/PixelTheater/retrotext/refs/heads/main/src/DisplayManager.cpp) class, used to drive three chips the run an LED message sign
- A project that runs 4 drivers that control PCBs populated with 14-segment vintage LED digits [main.cpp](https://raw.githubusercontent.com/somebox/14-segment-terminal/refs/heads/main/src/main.cpp)
- An [Analog-style LED clockfirmware](https://gist.githubusercontent.com/somebox/49780beecc1b8a3f6d2094c7e4d0cc88/raw/677ca76d61ff7a792da812fe8b202a87c7786707/main.cpp) I developed using the same chip series

---

### **Part 1: Key Use Cases from Firmware Analysis**

#### **1. Non-Matrix & Custom Layouts**

This is the most critical, shared requirement between the analog clock and the 14-segment terminal projects. Both treat the driver not as a grid, but as a "bag of pixels" that can be individually mapped to any physical layout.

*   **Verified Need:**
    *   The **analog clock** firmware defines a `struct led` and an array `leds[60]` to map a linear sequence (the 60 seconds/minutes of a clock face) to the chip's physical `(cs, sw)` outputs. Rendering logic operates on the linear index (e.g., `leds[15]`), not on an (x,y) coordinate.
    *   The **14-segment terminal** uses two levels of mapping: a `char_map` to convert ASCII to a segment bitmask, and a `segment_map` to convert a segment index (0-13) to a `(cs, sw)` pair. This is a clear indication that a simple grid abstraction is insufficient.

*   **Required Abstractions:**
    1.  **Layout Mapping:** The library must provide a mechanism for the user to define a custom layout by supplying a map. This map translates a logical index (e.g., pixel `0` to `59` for the clock, or segment `0` to `13` for a digit) to a hardware `(cs, sw)` coordinate. A simple data structure is sufficient:
        ```cpp
        // A structure to define a single entry in a custom layout map.
        struct PixelMapEntry {
          uint8_t cs; // The Column/Source pin (1-16 or 1-12)
          uint8_t sw; // The Row/Switch pin (1-12)
        };
        ```
    2.  **Indexed Drawing Model:** The primary drawing API for these use cases must be index-based, not coordinate-based. A function like `drawPixel(uint16_t index, uint8_t pwm)` allows the developer's rendering code to remain clean and agnostic of the physical hardware layout.

#### **2. Advanced Brightness Management**

All three projects manage brightness, but the `DisplayManager` reveals a more sophisticated need for layered control.

*   **Verified Need:**
    *   The **analog clock** and **14-segment terminal** use a simple global brightness variable that is applied uniformly.
    *   The **RetroText `DisplayManager`** implements two distinct levels:
        *   `setBrightness()`: Controls the driver's hardware Global Current Control (GCC) register. This is the master, hardware-level brightness.
        *   `setIntensity()`: A software-based `float` scaler (`0.0` to `1.0`) that is multiplied with the font's pixel data during rendering. This allows for per-pixel or per-element brightness *relative* to the global setting.

*   **Required Abstractions:**
    1.  **Hardware Brightness API:** A clear method, `setGlobalCurrent(uint8_t gcc)`, that directly controls the chip's master current setting (`0-255`).
    2.  **Software Brightness Scaling:** The library should provide an optional software brightness scaler, `setMasterBrightness(uint8_t scale)`. When a pixel is drawn, its requested PWM value is scaled by this master value before being written to the buffer (e.g., `final_pwm = (requested_pwm * scale) / 255`). This allows users to design all their assets (fonts, sprites) at full intensity and then apply a global software dimming effect without altering the hardware GCC, which is useful for smooth, calculated fades.

#### **3. Font & Text Rendering**

The RetroText project is entirely centered around displaying text, highlighting the need for robust font support beyond basic GFX capabilities.

*   **Verified Need:**
    *   The `DisplayManager` uses custom bitmap fonts, defined as large byte arrays.
        ```cpp
        // Simplified structure from the RetroText project's font file
        typedef struct {
          const uint8_t *data; // Pointer to bitmap data
          uint8_t width;
          uint8_t height;
          // ... other metadata
        } Font;
        ```
    *   The `scrollMessage` logic is complex, involving calculating character positions, handling partial characters at the screen edges, and managing the translation of coordinates across multiple physical driver chips.

*   **Required Abstractions:**
    1.  **GFX Integration:** Inheriting from `Adafruit_GFX` is the baseline, providing standard font support and a `print()` method.
    2.  **Canvas-Level Rendering:** The multi-chip `Canvas` object must handle all the complex scrolling and character-placement logic internally. The user should simply be able to `setCursor(x, y)` on the large logical canvas and `print("message")`, and the library will correctly render characters that span across physical chip boundaries.
    3.  **Text Styling:** The need for styles like "bold" can be implemented as a library feature. For example, a `printBold()` method could simply render the same text a second time at a one-pixel offset (`x+1`), which is a common and effective technique.

#### **4. Multi-Device Management & Layout**

Both the RetroText and 14-segment projects chain multiple drivers together to form a larger logical display.

*   **Verified Need:**
    *   Both projects initialize an array of driver objects (`IS31FL3737B[num_drivers]`).
    *   Rendering logic in both projects contains calculations to determine which driver in the array a given pixel belongs to (e.g., `driver_idx = x / driver_width; local_x = x % driver_width;`). This logic is boilerplate and error-prone.

*   **Required Abstractions:**
    1.  **Canvas Manager:** A high-level `Canvas` class is essential. It should be initialized with an array of device objects and the desired physical layout (e.g., horizontal, vertical, tiled).
        ```cpp
        // Enum to define the physical arrangement of chained drivers
        enum CanvasLayout {
          LAYOUT_HORIZONTAL,
          LAYOUT_VERTICAL,
          // Future: LAYOUT_TILED
        };
        ```
    2.  **Device Identification:** To aid in setup and debugging, a helper function is needed. A method like `identifyDevices()` could iterate through the registered drivers, briefly illuminating a unique pattern on each (e.g., displaying its index '0', '1', '2'...) while printing its I2C address and logical position to the Serial monitor. This helps the user instantly verify that their physical wiring matches their software configuration.

---

### **Part 2: Test Plan Based on Real-World Use Cases**

This test plan validates the fitness of the proposed driver by recreating the core challenges of the provided firmware projects.

*   **Test Category 1: Custom Layout Rendering**
    *   **T-1.1: Analog Clock Face Integrity**
        *   **Objective:** Verify the library's ability to render a non-linear, circular layout using the indexed drawing model.
        *   **Setup:** A single IS31FL3737B wired to 60 LEDs in a circle. A `PixelMapEntry` array mapping indices 0-59 to the physical `(cs, sw)` coordinates of the LEDs is provided to the driver.
        *   **Procedure:**
            1.  Command the driver to draw pixels at indices 0, 15, 30, and 45.
            2.  Command the driver to draw a "hand" by illuminating a sequence of 5 consecutive LEDs (e.g., indices 5, 6, 7, 8, 9).
        *   **Expected Result:**
            1.  The LEDs at the 12, 3, 6, and 9 o'clock positions illuminate correctly.
            2.  A solid arc of 5 LEDs illuminates, representing the clock hand. There should be no gaps or incorrectly lit LEDs.

    *   **T-1.2: 14-Segment Character Display**
        *   **Objective:** Verify the library can correctly render complex characters on a multi-digit, non-matrix display.
        *   **Setup:** Two IS31FL3737B chips, each driving four 14-segment digits. A `PixelMapEntry` array maps logical indices 0-111 (8 digits * 14 segments) to the correct `(chip_index, cs, sw)` hardware outputs.
        *   **Procedure:** Write a high-level function that takes a character and a digit position, and uses the library's `drawPixel(index)` to illuminate the correct segments. Command it to display the string "CODE 123".
        *   **Expected Result:** The physical LED display should clearly and correctly show "CODE 123". All segments for each character must be correct, with no bleed-over or incorrect segments lit.

*   **Test Category 2: Layered Brightness Control**
    *   **T-2.1: Hardware and Software Brightness Interaction**
        *   **Objective:** Verify that the global hardware current and the master software scaling function independently and predictably.
        *   **Setup:** A single, fully illuminated matrix.
        *   **Procedure:**
            1.  Set hardware `setGlobalCurrent(255)` and software `setMasterBrightness(128)`.
            2.  Set hardware `setGlobalCurrent(128)` and software `setMasterBrightness(255)`.
            3.  Set hardware `setGlobalCurrent(128)` and software `setMasterBrightness(128)`.
        *   **Expected Result:**
            1.  The display should appear at approximately 50% brightness.
            2.  The display should also appear at approximately 50% brightness.
            3.  The display should appear at approximately 25% brightness (`0.5 * 0.5`). The visual result should be noticeably dimmer than the first two steps.

*   **Test Category 3: Multi-Device Canvas Rendering**
    *   **T-3.1: Cross-Chip Character Rendering**
        *   **Objective:** Verify that the `Canvas` class can render a single character that spans the boundary between two physical chips.
        *   **Setup:** Two 12x12 matrices configured as a single 24x12 horizontal canvas. An 8-pixel wide font is used.
        *   **Procedure:** Command the canvas to `setCursor(10, 0)` and `print("O")`.
        *   **Expected Result:** The character "O" should appear seamlessly across the two matrices. The left-most 2 columns of pixels should be on the right edge of the first chip, and the remaining 6 columns should be on the left edge of the second chip. There should be no visible seam, gap, or distortion.

    *   **T-3.2: Full Canvas Scrolling**
        *   **Objective:** Replicate the core functionality of the RetroText project by smoothly scrolling text across the entire canvas.
        *   **Setup:** Three matrices in a 36x12 horizontal canvas.
        *   **Procedure:** Command the canvas to scroll a long sentence from right to left.
        *   **Expected Result:** The text must scroll smoothly across all three displays. Pay close attention to the moments when characters cross the boundaries at pixel columns 11-12 and 23-24. The animation must be fluid and tear-free.

*   **Test Category 4: Setup and Configuration**
    *   **T-4.1: Device Order Identification**
        *   **Objective:** Verify the `identifyDevices()` helper function aids the user in correctly configuring the canvas.
        *   **Setup:** Three matrices wired to I2C addresses 0x50, 0x51, 0x52, but physically arranged in the order 0x51, 0x52, 0x50. The canvas is initialized with the incorrect logical order `[0x50, 0x51, 0x52]`.
        *   **Procedure:** Call the `identifyDevices()` function.
        *   **Expected Result:** The user should observe the matrices lighting up in the sequence: middle, right, left. The Serial Monitor should output a log indicating the I2C address for each logical index, allowing the user to immediately spot the discrepancy between the physical layout and the software configuration.