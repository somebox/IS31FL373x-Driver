#include "IS31FL373x.h"

#ifdef UNIT_TEST
#include <cstdlib>
#include <cstring>
// Mock Wire instance for testing
TwoWire Wire;
#endif

// Stub implementations for basic compilation testing
// Full implementation will be added incrementally

IS31FL373x_Device::IS31FL373x_Device(uint8_t addr, TwoWire *wire) 
    : Adafruit_GFX(0, 0), _wire(wire), _addr(addr), _pwmBuffer(nullptr),
      _globalCurrent(128), _masterBrightness(255), _customLayout(nullptr),
      _layoutSize(0), _useCustomLayout(false) {
}

bool IS31FL373x_Device::begin() {
    // TODO: Implement initialization sequence
    // For now, just allocate the PWM buffer
    if (_pwmBuffer == nullptr) {
#ifdef UNIT_TEST
        _pwmBuffer = static_cast<uint8_t*>(std::malloc(getPWMBufferSize()));
#else
        _pwmBuffer = new uint8_t[getPWMBufferSize()];
#endif
        if (_pwmBuffer == nullptr) {
            return false;
        }
        memset(_pwmBuffer, 0, getPWMBufferSize());
    }
    return true;
}

void IS31FL373x_Device::reset() {
    // TODO: Implement reset sequence
}

void IS31FL373x_Device::show() {
    // TODO: Implement buffer transfer to device
}

void IS31FL373x_Device::clear() {
    if (_pwmBuffer != nullptr) {
        memset(_pwmBuffer, 0, getPWMBufferSize());
    }
}

void IS31FL373x_Device::setGlobalCurrent(uint8_t current) {
    _globalCurrent = current;
    // TODO: Write to device register
}

void IS31FL373x_Device::setMasterBrightness(uint8_t brightness) {
    _masterBrightness = brightness;
}

void IS31FL373x_Device::drawPixel(int16_t x, int16_t y, uint16_t color) {
    // Basic bounds checking
    if (x < 0 || y < 0 || x >= getWidth() || y >= getHeight()) {
        return;
    }
    
    uint16_t index = coordToIndex(x, y);
    if (index < getPWMBufferSize() && _pwmBuffer != nullptr) {
        // Apply master brightness scaling
        uint8_t scaledColor = (color * _masterBrightness) / 255;
        _pwmBuffer[index] = scaledColor;
    }
}

void IS31FL373x_Device::setPixel(uint16_t index, uint8_t pwm) {
    if (index < getPWMBufferSize() && _pwmBuffer != nullptr) {
        // Apply master brightness scaling
        uint8_t scaledPWM = (pwm * _masterBrightness) / 255;
        _pwmBuffer[index] = scaledPWM;
    }
}

void IS31FL373x_Device::setLayout(const PixelMapEntry* layout, uint16_t layoutSize) {
    // TODO: Implement custom layout mapping
    _customLayout = const_cast<PixelMapEntry*>(layout);
    _layoutSize = layoutSize;
    _useCustomLayout = (layout != nullptr && layoutSize > 0);
}

bool IS31FL373x_Device::selectPage(uint8_t page) {
    // TODO: Implement page selection I2C sequence
    return true;
}

bool IS31FL373x_Device::writeRegister(uint8_t reg, uint8_t value) {
    // TODO: Implement register write
    return true;
}

bool IS31FL373x_Device::readRegister(uint8_t reg, uint8_t* value) {
    // TODO: Implement register read
    return true;
}

uint16_t IS31FL373x_Device::coordToIndex(uint8_t x, uint8_t y) const {
    // Default mapping: row-major order
    return static_cast<uint16_t>(y * getWidth() + x);
}

void IS31FL373x_Device::indexToCoord(uint16_t index, uint8_t* x, uint8_t* y) const {
    if (x != nullptr) *x = index % getWidth();
    if (y != nullptr) *y = index / getWidth();
}

// IS31FL3733 Implementation
IS31FL3733::IS31FL3733(uint8_t addr1, uint8_t addr2, TwoWire *wire) 
    : IS31FL373x_Device(calculateAddress(addr1, addr2), wire) {
    // Update GFX dimensions
    _width = MATRIX_WIDTH;
    _height = MATRIX_HEIGHT;
}

uint8_t IS31FL3733::calculateAddress(uint8_t addr1, uint8_t addr2) {
    // Base address: 0b1010000 (0x50)
    // Address bits from ADDR pins
    return 0x50 | ((addr2 & 0x03) << 2) | (addr1 & 0x03);
}

// IS31FL3737B Implementation  
IS31FL3737B::IS31FL3737B(uint8_t addr, TwoWire *wire) 
    : IS31FL373x_Device(calculateAddress(addr), wire) {
    // Update GFX dimensions
    _width = MATRIX_WIDTH;
    _height = MATRIX_HEIGHT;
}

uint8_t IS31FL3737B::calculateAddress(uint8_t addr) {
    // Base address: 0b1010000 (0x50)
    // Address bits from ADDR pin
    return 0x50 | (addr & 0x0F);
}

void IS31FL3737B::setPWMFrequency(uint8_t freq) {
    // TODO: Implement PWM frequency setting specific to 3737B
}

// Canvas Implementation
IS31FL373x_Canvas::IS31FL373x_Canvas(uint16_t width, uint16_t height,
                                     IS31FL373x_Device** devices, uint8_t deviceCount,
                                     CanvasLayout layout)
    : Adafruit_GFX(width, height), _devices(devices), _deviceCount(deviceCount), _layout(layout) {
}

IS31FL373x_Canvas::~IS31FL373x_Canvas() {
    // Note: We don't delete the devices as they're owned by the caller
}

bool IS31FL373x_Canvas::begin() {
    bool success = true;
    for (uint8_t i = 0; i < _deviceCount; i++) {
        if (_devices[i] != nullptr) {
            success &= _devices[i]->begin();
        } else {
            // If any device is null, initialization should fail
            success = false;
        }
    }
    return success;
}

void IS31FL373x_Canvas::show() {
    for (uint8_t i = 0; i < _deviceCount; i++) {
        if (_devices[i] != nullptr) {
            _devices[i]->show();
        }
    }
}

void IS31FL373x_Canvas::clear() {
    for (uint8_t i = 0; i < _deviceCount; i++) {
        if (_devices[i] != nullptr) {
            _devices[i]->clear();
        }
    }
}

void IS31FL373x_Canvas::setGlobalCurrent(uint8_t current) {
    for (uint8_t i = 0; i < _deviceCount; i++) {
        if (_devices[i] != nullptr) {
            _devices[i]->setGlobalCurrent(current);
        }
    }
}

void IS31FL373x_Canvas::setMasterBrightness(uint8_t brightness) {
    for (uint8_t i = 0; i < _deviceCount; i++) {
        if (_devices[i] != nullptr) {
            _devices[i]->setMasterBrightness(brightness);
        }
    }
}

void IS31FL373x_Canvas::drawPixel(int16_t x, int16_t y, uint16_t color) {
    int16_t localX, localY;
    IS31FL373x_Device* device = getDeviceForCoordinate(x, y, &localX, &localY);
    if (device != nullptr) {
        device->drawPixel(localX, localY, color);
    }
}

void IS31FL373x_Canvas::identifyDevices() {
    // TODO: Implement device identification sequence
    // For now, just a placeholder
}

IS31FL373x_Device* IS31FL373x_Canvas::getDeviceForCoordinate(int16_t x, int16_t y, 
                                                           int16_t* localX, int16_t* localY) {
    // TODO: Implement coordinate to device mapping based on layout
    // For now, simple horizontal layout
    if (_layout == LAYOUT_HORIZONTAL && _deviceCount > 0 && _devices[0] != nullptr) {
        uint8_t deviceWidth = _devices[0]->getWidth();
        uint8_t deviceIndex = x / deviceWidth;
        
        if (deviceIndex < _deviceCount && _devices[deviceIndex] != nullptr) {
            *localX = x % deviceWidth;
            *localY = y;
            return _devices[deviceIndex];
        }
    }
    
    return nullptr;
}
