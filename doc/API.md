# IS31FL373x Driver API Reference

Quick reference for all public methods, enums, and structures in the IS31FL373x LED matrix driver library.

## Device Classes

### IS31FL3733 (12×16 matrix, 192 LEDs)

```cpp
IS31FL3733(ADDR addr1 = ADDR::GND, ADDR addr2 = ADDR::GND, TwoWire *wire = &Wire);
```

### IS31FL3737 (12×12 matrix, 144 LEDs)

```cpp
IS31FL3737(ADDR addr = ADDR::GND, TwoWire *wire = &Wire);
```

### IS31FL3737B (12×12 matrix, 144 LEDs, selectable PWM frequency)

```cpp
IS31FL3737B(ADDR addr = ADDR::GND, TwoWire *wire = &Wire);
```

## Core Methods (All Device Classes)

### Initialization

```cpp
bool begin();                    // Initialize device, allocate buffers, configure hardware
void reset();                   // Software reset via register read
```

### Display Control

```cpp
void show();                    // Push local buffer to hardware
void clear();                   // Clear local buffer (set all pixels to 0)
```

### Drawing (Adafruit_GFX Compatible)

```cpp
void drawPixel(int16_t x, int16_t y, uint16_t color);  // Draw pixel at (x,y) with brightness 0-255
// Plus all Adafruit_GFX methods: drawLine(), drawRect(), drawCircle(), print(), etc.
```

### Brightness Control

```cpp
void setGlobalCurrent(uint8_t current);        // Hardware current limit (0-255)
void setMasterBrightness(uint8_t brightness);  // Software brightness scaling (0-255)
```

### Custom Layout Support

```cpp
void setPixel(uint16_t index, uint8_t pwm);                          // Set pixel by linear index
void setLayout(const PixelMapEntry* layout, uint16_t layoutSize);    // Define custom pixel mapping
void setCoordinateOffset(uint8_t csOffset, uint8_t swOffset);        // Hardware compatibility offset
```

### State Inspection (Testing/Debugging)

```cpp
// Device Properties
uint8_t getWidth() const;                // Matrix width (12 or 16)
uint8_t getHeight() const;               // Matrix height (always 12)
uint16_t getPWMBufferSize() const;       // Total LEDs (144 or 192)
uint8_t getI2CAddress() const;           // Calculated I2C address

// Configuration State
uint8_t getGlobalCurrent() const;        // Current hardware current setting
uint8_t getMasterBrightness() const;     // Current brightness scaling
bool isCustomLayoutActive() const;       // Whether custom layout is active
uint16_t getLayoutSize() const;          // Number of entries in custom layout

// Buffer Inspection
uint8_t getPixelValue(uint16_t x, uint16_t y) const;     // Get pixel value at (x,y)
uint8_t getPixelValueByIndex(uint16_t index) const;      // Get pixel value by linear index
uint16_t getNonZeroPixelCount() const;                   // Count of non-zero pixels
uint16_t getPixelSum() const;                            // Sum of all pixel values

// Coordinate Utilities
uint16_t coordToIndex(uint8_t x, uint8_t y) const;       // Convert (x,y) to hardware register address
void indexToCoord(uint16_t index, uint8_t* x, uint8_t* y) const;  // Convert register address to (x,y)
```

## IS31FL3737B-Specific Methods

```cpp
void setPWMFrequency(uint8_t freq);      // Set PWM frequency (0-7, see datasheet)
```

## Canvas Class (Multi-Chip Management)

### Constructor

```cpp
IS31FL373x_Canvas(uint16_t width, uint16_t height, 
                  IS31FL373x_Device** devices, uint8_t deviceCount,
                  CanvasLayout layout = LAYOUT_HORIZONTAL);
```

### Canvas Control

```cpp
bool begin();                           // Initialize all devices
void show();                            // Update all devices
void clear();                           // Clear all devices
void drawPixel(int16_t x, int16_t y, uint16_t color);  // Draw across device boundaries
```

### Canvas Configuration

```cpp
void setGlobalCurrent(uint8_t current);        // Apply to all devices
void setMasterBrightness(uint8_t brightness);  // Apply to all devices
void identifyDevices();                        // Helper for device identification
```

### Canvas State Inspection

```cpp
uint8_t getDeviceCount() const;                     // Number of managed devices
IS31FL373x_Device* getDevice(uint8_t index) const; // Get device by index
CanvasLayout getLayout() const;                     // Current layout mode
uint16_t getTotalNonZeroPixelCount() const;         // Non-zero pixels across all devices
```

## Enums and Constants

### ADDR Pin Configuration

```cpp
enum class ADDR {
    GND = 0,    // Pin connected to ground
    VCC = 1,    // Pin connected to VCC
    SDA = 2,    // Pin connected to SDA line  
    SCL = 3     // Pin connected to SCL line
};
```

### Canvas Layout Options

```cpp
enum CanvasLayout {
    LAYOUT_HORIZONTAL,  // Devices arranged left-to-right
    LAYOUT_VERTICAL     // Devices arranged top-to-bottom
};
```

### Custom Layout Structure

```cpp
struct PixelMapEntry {
    uint8_t cs;  // Column/Source pin (1-16 for IS31FL3733, 1-12 for others)
    uint8_t sw;  // Switch/Row pin (1-12 for all chips)
};
```

## I2C Address Calculation

### IS31FL3733 (2 ADDR pins → 16 addresses)

Base address `0x50` + `(addr2 << 2) + addr1`
- Range: `0x50` to `0x5F`

### IS31FL3737 / IS31FL3737B (1 ADDR pin → 4 addresses)  

Base address `0x50` + address bits per IS31FL373x-reference.md:
- `ADDR::GND` → `0x50` (0000 bits)
- `ADDR::SCL` → `0x55` (0101 bits)  
- `ADDR::SDA` → `0x5A` (1010 bits)
- `ADDR::VCC` → `0x5F` (1111 bits)

**Note:** The ADDR pin connection determines specific bit patterns, not sequential values.

## Hardware-Specific Behavior

### IS31FL3737 Register Mapping Quirk

The IS31FL3737 chip has a hardware quirk in its register mapping that affects columns 6-11 (0-based indexing):

- **Columns 0-5**: Map directly to register addresses 0-5 ✓
- **Columns 6-11**: Map to register addresses 8-13 (NOT 6-11) ⚠️

This means there's a 2-address gap in the register mapping. The driver automatically handles this remapping:

```cpp
IS31FL3737 matrix;
matrix.drawPixel(6, 0, 255);  // User draws at column 6
// Driver automatically writes to register address 8, not 6
```

**Why this matters:**
- Without proper remapping, LEDs would light up in unexpected locations
- This quirk is unique to the IS31FL3737; the IS31FL3733 and IS31FL3737B do not have this issue
- The driver handles this transparently - users don't need to worry about it

### IS31FL3733 and IS31FL3737B
These chips have linear register mapping with no gaps - columns 0-11 map directly to register addresses 0-11.

## Usage Patterns

### Single Device

```cpp
IS31FL3737B matrix(ADDR::GND);
matrix.begin();
matrix.setGlobalCurrent(128);
matrix.drawPixel(5, 5, 255);
matrix.show();
```

### Multi-Device Canvas

```cpp
IS31FL3737 dev1(ADDR::GND), dev2(ADDR::VCC);
IS31FL373x_Device* devices[] = {&dev1, &dev2};
IS31FL373x_Canvas canvas(24, 12, devices, 2);
canvas.begin();
canvas.print("Hello");
canvas.show();
```

### Custom Layout (Non-Matrix)

```cpp
PixelMapEntry clockLayout[12] = {{6,1}, {9,2}, ...};
matrix.setLayout(clockLayout, 12);
matrix.setPixel(3, 255);  // Light up 3 o'clock position
```

## Notes

- All brightness values use 0-255 range
- Coordinate systems are 0-indexed
- Custom layout CS/SW pins are 1-indexed (hardware pins)
- Buffer operations are local until `show()` is called
- State inspection methods are safe to call before `begin()`
