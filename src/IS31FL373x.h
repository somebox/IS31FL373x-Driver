#ifndef IS31FL373X_H
#define IS31FL373X_H

#ifdef UNIT_TEST
// Mock includes for unit testing
#include <stdint.h>
#include <string.h>
#include <vector>

// Simple mock tracking for I2C operations during testing
struct MockI2COperation {
    uint8_t addr;
    uint8_t reg;
    uint8_t value;
    bool isWrite;
    std::vector<uint8_t> bulkData;  // For bulk writes, stores all data bytes
};

extern std::vector<MockI2COperation> mockI2COperations;
void clearMockI2COperations();
size_t getMockI2COperationCount();
bool mockI2CContainsWrite(uint8_t reg, uint8_t value);

// Mock Arduino types and classes for testing
class TwoWire {
public:
    static TwoWire& getInstance() { static TwoWire instance; return instance; }
};
extern TwoWire Wire;

class Adafruit_GFX {
public:
    Adafruit_GFX(int16_t w, int16_t h) : _width(w), _height(h) {}
    virtual ~Adafruit_GFX() = default;
    virtual void drawPixel(int16_t x, int16_t y, uint16_t color) = 0;
    int16_t width() const { return _width; }
    int16_t height() const { return _height; }
    // Minimal subset of Adafruit_GFX primitives for UNIT_TEST
    void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
        for (int16_t i = 0; i < w; i++) {
            drawPixel(x + i, y, color);
        }
    }
    void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
        for (int16_t i = 0; i < h; i++) {
            drawPixel(x, y + i, color);
        }
    }
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
        if (w <= 0 || h <= 0) return;
        drawFastHLine(x, y, w, color);
        if (h > 1) drawFastHLine(x, y + h - 1, w, color);
        if (h > 2) {
            drawFastVLine(x, y + 1, h - 2, color);
            if (w > 1) drawFastVLine(x + w - 1, y + 1, h - 2, color);
        }
    }
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
        for (int16_t j = 0; j < h; j++) {
            drawFastHLine(x, y + j, w, color);
        }
    }
protected:
    int16_t _width, _height;
};

class Adafruit_I2CDevice {
public:
    Adafruit_I2CDevice(uint8_t addr, TwoWire* wire = nullptr) : _addr(addr), _wire(wire), _lastReg(0) {}
    virtual ~Adafruit_I2CDevice() = default;
    bool begin() { return true; }
    bool write(uint8_t* buffer, size_t len);
    bool read(uint8_t* buffer, size_t len) {
        (void)buffer; (void)len;
        // Track I2C read operations during UNIT_TESTs
        MockI2COperation op;
        op.addr = _addr;
        op.reg = _lastReg;
        op.value = 0;
        op.isWrite = false;
        mockI2COperations.push_back(op);
        return true;
    }
    uint8_t getAddr() const { return _addr; }
private:
    uint8_t _addr;
    TwoWire* _wire;
    uint8_t _lastReg;  // tracks last addressed register for UNIT_TEST read tracing
};
#else
// Real Arduino includes
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_BusIO_Register.h>
#include <Adafruit_I2CDevice.h>
#endif

// Version
#define IS31FL373X_VERSION "1.0.10"

// Forward declarations for the unified driver architecture
class IS31FL373x_Device;
class IS31FL3733;
class IS31FL3737;
class IS31FL3737B;
class IS31FL373x_Canvas;

// ADDR pin constants for clean addressing
enum class ADDR {
    GND = 0,    // Connected to ground
    VCC = 1,    // Connected to VCC
    SDA = 2,    // Connected to SDA line
    SCL = 3     // Connected to SCL line
};

// Common constants
#define IS31FL373X_REG_UNLOCK      0xFE
#define IS31FL373X_REG_COMMAND     0xFD
#define IS31FL373X_UNLOCK_VALUE    0xC5

// Page definitions
#define IS31FL373X_PAGE_LED_CTRL   0x00
#define IS31FL373X_PAGE_PWM        0x01
#define IS31FL373X_PAGE_ABM        0x02
#define IS31FL373X_PAGE_FUNCTION   0x03

// Pixel mapping structure for custom layouts
struct PixelMapEntry {
    uint8_t cs;  // Column/Source pin (1-16 for 3733, 1-12 for 3737B)
    uint8_t sw;  // Switch/Row pin (1-12 for both)
};

// Canvas layout options
enum CanvasLayout {
    LAYOUT_HORIZONTAL,
    LAYOUT_VERTICAL
    // Future: LAYOUT_TILED
};

/**
 * Base class for IS31FL373x devices
 * Provides common functionality for both IS31FL3733 and IS31FL3737B
 */
class IS31FL373x_Device : public Adafruit_GFX {
public:
    IS31FL373x_Device(uint8_t addr = 0x50, TwoWire *wire = &Wire);
    virtual ~IS31FL373x_Device();

    // Initialization
    virtual bool begin();
    virtual void reset();
    
    // Pure virtual methods to be implemented by derived classes
    virtual uint8_t getWidth() const = 0;
    virtual uint8_t getHeight() const = 0;
    virtual uint16_t getPWMBufferSize() const = 0;
    virtual uint8_t getRegisterStride() const = 0;  // 16 for all chips (IS31FL3733, IS31FL3737, IS31FL3737B)
    
    // Display control
    virtual void show();
    virtual void clear();
    
    // Brightness control
    void setGlobalCurrent(uint8_t current);
    void setMasterBrightness(uint8_t brightness);
    
    // GFX implementation
    void drawPixel(int16_t x, int16_t y, uint16_t color) override;
    
    // Indexed pixel control for custom layouts
    void setPixel(uint16_t index, uint8_t pwm);
    void setLayout(const PixelMapEntry* layout, uint16_t layoutSize);
    
    // Hardware compatibility for IS31FL3737
    void setCoordinateOffset(uint8_t csOffset, uint8_t swOffset);

protected:
    // Convert hardware CS/SW (1-based) to register index. Derived classes can
    // override to apply chip-specific quirks. Offsets are NOT applied here.
    virtual uint16_t csSwToIndex(uint8_t cs1Based, uint8_t sw1Based) const;

    Adafruit_I2CDevice* _i2c_dev;
    uint8_t* _pwmBuffer;
    uint8_t _globalCurrent;
    uint8_t _masterBrightness;
    bool _ownsI2CDevice = true;
    
    // I2C parameters (stored for delayed initialization)
    uint8_t _addr;
    TwoWire* _wire;
    
    // Layout mapping
    PixelMapEntry* _customLayout;
    uint16_t _layoutSize;
    bool _useCustomLayout;
    
    // Coordinate offset for hardware compatibility (IS31FL3737 support)
    uint8_t _csOffset;
    uint8_t _swOffset;
    
    // Low-level I2C operations
    bool selectPage(uint8_t page);
    bool writeRegister(uint8_t reg, uint8_t value);
    bool writeBulk(uint8_t startReg, const uint8_t* data, size_t length);
    bool readRegister(uint8_t reg, uint8_t* value);
    
public:
    // Coordinate conversion (public for testing)
    virtual uint16_t coordToIndex(uint8_t x, uint8_t y) const;
    virtual void indexToCoord(uint16_t index, uint8_t* x, uint8_t* y) const;
    
    // State inspection methods for testing
    uint8_t getGlobalCurrent() const { return _globalCurrent; }
    uint8_t getMasterBrightness() const { return _masterBrightness; }
    uint8_t getPixelValue(uint16_t x, uint16_t y) const;
    uint8_t getPixelValueByIndex(uint16_t index) const;
    uint16_t getNonZeroPixelCount() const;
    uint16_t getPixelSum() const;
    bool isCustomLayoutActive() const { return _useCustomLayout; }
    uint16_t getLayoutSize() const { return _layoutSize; }
    uint8_t getI2CAddress() const { return _addr; }
#ifdef UNIT_TEST
    // Test-only: inject a custom I2C device without transferring ownership
    void setI2CDeviceForTest(Adafruit_I2CDevice* dev) { _i2c_dev = dev; _ownsI2CDevice = false; }
#endif
};

/**
 * Driver for IS31FL3733 (12x16 matrix)
 */
class IS31FL3733 : public IS31FL373x_Device {
public:
    static const uint8_t MATRIX_WIDTH = 16;
    static const uint8_t MATRIX_HEIGHT = 12;
    static const uint16_t PWM_BUFFER_SIZE = 192; // 12 * 16
    
    IS31FL3733(ADDR addr1 = ADDR::GND, ADDR addr2 = ADDR::GND, TwoWire *wire = &Wire);
    
    uint8_t getWidth() const override { return MATRIX_WIDTH; }
    uint8_t getHeight() const override { return MATRIX_HEIGHT; }
    uint16_t getPWMBufferSize() const override { return PWM_BUFFER_SIZE; }
    uint8_t getRegisterStride() const override { return 16; }  // IS31FL3733 uses 16-byte stride

private:
    uint8_t calculateAddress(ADDR addr1, ADDR addr2);
};

/**
 * Driver for IS31FL3737 (12x12 matrix)
 */
class IS31FL3737 : public IS31FL373x_Device {
public:
    static const uint8_t MATRIX_WIDTH = 12;
    static const uint8_t MATRIX_HEIGHT = 12;
    static const uint16_t PWM_BUFFER_SIZE = 144; // 12 * 12
    
    IS31FL3737(ADDR addr = ADDR::GND, TwoWire *wire = &Wire);
    
    uint8_t getWidth() const override { return MATRIX_WIDTH; }
    uint8_t getHeight() const override { return MATRIX_HEIGHT; }
    uint16_t getPWMBufferSize() const override { return PWM_BUFFER_SIZE; }
    uint8_t getRegisterStride() const override { return 16; }  // IS31FL3737 uses 16-byte stride (same as others)
    
    // Override coordinate mapping for IS31FL3737 hardware quirk
    uint16_t coordToIndex(uint8_t x, uint8_t y) const override;
    void indexToCoord(uint16_t index, uint8_t* x, uint8_t* y) const override;
protected:
    uint16_t csSwToIndex(uint8_t cs1Based, uint8_t sw1Based) const override;

private:
    uint8_t calculateAddress(ADDR addr);
};

/**
 * Driver for IS31FL3737B (12x12 matrix)
 */
class IS31FL3737B : public IS31FL373x_Device {
public:
    static const uint8_t MATRIX_WIDTH = 12;
    static const uint8_t MATRIX_HEIGHT = 12;
    static const uint16_t PWM_BUFFER_SIZE = 144; // 12 * 12
    
    IS31FL3737B(ADDR addr = ADDR::GND, TwoWire *wire = &Wire);
    
    uint8_t getWidth() const override { return MATRIX_WIDTH; }
    uint8_t getHeight() const override { return MATRIX_HEIGHT; }
    uint16_t getPWMBufferSize() const override { return PWM_BUFFER_SIZE; }
    uint8_t getRegisterStride() const override { return 16; }  // IS31FL3737B still uses 16-byte stride in registers
    
    // IS31FL3737B-specific features
    void setPWMFrequency(uint8_t freq);  // Selectable PWM frequency: 1.05-26.7 kHz
    // TODO(test): write FUNCTION page frequency bits when implemented

private:
    uint8_t calculateAddress(ADDR addr);
};

/**
 * Multi-chip canvas manager
 * Combines multiple IS31FL373x devices into a single logical display
 */
class IS31FL373x_Canvas : public Adafruit_GFX {
public:
    IS31FL373x_Canvas(uint16_t width, uint16_t height, 
                      IS31FL373x_Device** devices, uint8_t deviceCount,
                      CanvasLayout layout = LAYOUT_HORIZONTAL);
    virtual ~IS31FL373x_Canvas();
    
    // Canvas control
    bool begin();
    void show();
    void clear();
    
    // Brightness control for all devices
    void setGlobalCurrent(uint8_t current);
    void setMasterBrightness(uint8_t brightness);
    
    // GFX implementation
    void drawPixel(int16_t x, int16_t y, uint16_t color) override;
    
    // Device identification helper
    void identifyDevices();
    
    // State inspection methods for testing
    uint8_t getDeviceCount() const { return _deviceCount; }
    IS31FL373x_Device* getDevice(uint8_t index) const { 
        return (index < _deviceCount) ? _devices[index] : nullptr; 
    }
    CanvasLayout getLayout() const { return _layout; }
    uint16_t getTotalNonZeroPixelCount() const;

private:
    IS31FL373x_Device** _devices;
    uint8_t _deviceCount;
    CanvasLayout _layout;
    
    // Helper methods
    IS31FL373x_Device* getDeviceForCoordinate(int16_t x, int16_t y, int16_t* localX, int16_t* localY);
};

#endif // IS31FL373X_H
