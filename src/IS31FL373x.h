#ifndef IS31FL373X_H
#define IS31FL373X_H

#ifdef UNIT_TEST
// Mock includes for unit testing
#include <stdint.h>
#include <string.h>
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
protected:
    int16_t _width, _height;
};

class Adafruit_I2CDevice {
public:
    Adafruit_I2CDevice(uint8_t addr, TwoWire* wire = nullptr) : _addr(addr), _wire(wire) {}
    virtual ~Adafruit_I2CDevice() = default;
    bool begin() { return true; }
    bool write(uint8_t* buffer, size_t len) { return true; }
    bool read(uint8_t* buffer, size_t len) { return true; }
private:
    uint8_t _addr;
    TwoWire* _wire;
};
#else
// Real Arduino includes
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_BusIO_Register.h>
#include <Adafruit_I2CDevice.h>
#endif

// Forward declarations for the unified driver architecture
class IS31FL373x_Device;
class IS31FL3733;
class IS31FL3737B;
class IS31FL373x_Canvas;

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

protected:
    Adafruit_I2CDevice* _i2c_dev;
    uint8_t* _pwmBuffer;
    uint8_t _globalCurrent;
    uint8_t _masterBrightness;
    
    // Layout mapping
    PixelMapEntry* _customLayout;
    uint16_t _layoutSize;
    bool _useCustomLayout;
    
    // Low-level I2C operations
    bool selectPage(uint8_t page);
    bool writeRegister(uint8_t reg, uint8_t value);
    bool readRegister(uint8_t reg, uint8_t* value);
    
public:
    // Coordinate conversion (public for testing)
    virtual uint16_t coordToIndex(uint8_t x, uint8_t y) const;
    virtual void indexToCoord(uint16_t index, uint8_t* x, uint8_t* y) const;
};

/**
 * Driver for IS31FL3733 (12x16 matrix)
 */
class IS31FL3733 : public IS31FL373x_Device {
public:
    static const uint8_t MATRIX_WIDTH = 16;
    static const uint8_t MATRIX_HEIGHT = 12;
    static const uint16_t PWM_BUFFER_SIZE = 192; // 12 * 16
    
    IS31FL3733(uint8_t addr1 = 0, uint8_t addr2 = 0, TwoWire *wire = &Wire);
    
    uint8_t getWidth() const override { return MATRIX_WIDTH; }
    uint8_t getHeight() const override { return MATRIX_HEIGHT; }
    uint16_t getPWMBufferSize() const override { return PWM_BUFFER_SIZE; }

private:
    uint8_t calculateAddress(uint8_t addr1, uint8_t addr2);
};

/**
 * Driver for IS31FL3737B (12x12 matrix)
 */
class IS31FL3737B : public IS31FL373x_Device {
public:
    static const uint8_t MATRIX_WIDTH = 12;
    static const uint8_t MATRIX_HEIGHT = 12;
    static const uint16_t PWM_BUFFER_SIZE = 144; // 12 * 12
    
    IS31FL3737B(uint8_t addr = 0, TwoWire *wire = &Wire);
    
    uint8_t getWidth() const override { return MATRIX_WIDTH; }
    uint8_t getHeight() const override { return MATRIX_HEIGHT; }
    uint16_t getPWMBufferSize() const override { return PWM_BUFFER_SIZE; }
    
    // 3737B-specific features
    void setPWMFrequency(uint8_t freq);

private:
    uint8_t calculateAddress(uint8_t addr);
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

private:
    IS31FL373x_Device** _devices;
    uint8_t _deviceCount;
    CanvasLayout _layout;
    
    // Helper methods
    IS31FL373x_Device* getDeviceForCoordinate(int16_t x, int16_t y, int16_t* localX, int16_t* localY);
};

#endif // IS31FL373X_H
