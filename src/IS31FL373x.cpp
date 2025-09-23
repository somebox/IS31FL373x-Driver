#include "IS31FL373x.h"

#ifdef UNIT_TEST
#include <cstdlib>
#include <cstring>
// Mock Wire instance for testing
TwoWire Wire;
// Mock delay function for testing
void delay(unsigned long ms) {
    // No-op in unit tests
}

// Mock I2C tracking implementation
std::vector<MockI2COperation> mockI2COperations;

void clearMockI2COperations() {
    mockI2COperations.clear();
}

size_t getMockI2COperationCount() {
    return mockI2COperations.size();
}

bool Adafruit_I2CDevice::write(uint8_t* buffer, size_t len) {
    if (buffer == nullptr || len == 0) return true;
    
    // Track I2C writes in tests: both 1-byte (address latch) and 2+ byte (reg/value)
    MockI2COperation op;
    op.addr = _addr;
    op.reg = buffer[0];
    op.value = (len >= 2) ? buffer[1] : 0;
    op.isWrite = true;
    mockI2COperations.push_back(op);
    
    // Remember last addressed register for subsequent read tracing
    if (len == 1) {
        _lastReg = buffer[0];
    }
    
    return true;
}
#else
#include <Arduino.h>  // for delay() function
#endif

// Stub implementations for basic compilation testing
// Full implementation will be added incrementally

IS31FL373x_Device::IS31FL373x_Device(uint8_t addr, TwoWire *wire) 
    : Adafruit_GFX(12, 12), _i2c_dev(nullptr), _pwmBuffer(nullptr),
      _globalCurrent(128), _masterBrightness(255), _addr(addr), _wire(wire),
      _customLayout(nullptr), _layoutSize(0), _useCustomLayout(false), 
      _csOffset(0), _swOffset(0) {
    // Store parameters for delayed initialization in begin()
    // DON'T create Adafruit_I2CDevice here to avoid static initialization issues
}

IS31FL373x_Device::~IS31FL373x_Device() {
    if (_i2c_dev) {
        delete _i2c_dev;
        _i2c_dev = nullptr;
    }
    if (_pwmBuffer) {
#ifdef UNIT_TEST
        std::free(_pwmBuffer);
#else
        delete[] _pwmBuffer;
#endif
        _pwmBuffer = nullptr;
    }
}

bool IS31FL373x_Device::begin() {
    // Create I2C device here when Wire is properly initialized
    if (_i2c_dev == nullptr) {
        _i2c_dev = new Adafruit_I2CDevice(_addr, _wire);
        if (_i2c_dev == nullptr) {
            return false;
        }
    }
    
    // Initialize I2C device
    if (!_i2c_dev->begin()) {
        return false;
    }
    
    // Allocate PWM buffer
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
    
    // Software reset
    reset();
    
    // Enable all LEDs (LED Control Page)
    selectPage(IS31FL373X_PAGE_LED_CTRL);
    // LED Control registers are 0x00-0x17 (24 registers total)
    // Each register controls 8 LEDs with bitwise mapping
    for (uint8_t i = 0x00; i <= 0x17; i++) {
        writeRegister(i, 0xFF); // Enable all LEDs in this register
    }
    
    // Configure Function Page
    selectPage(IS31FL373X_PAGE_FUNCTION);
    writeRegister(0x00, 0x01); // Configuration Register: SSD=1 (Normal Operation)
    writeRegister(0x01, _globalCurrent); // Global Current Control
    
    // Switch to PWM page for normal operation
    selectPage(IS31FL373X_PAGE_PWM);
    
    return true;
}

void IS31FL373x_Device::reset() {
    // Software reset by reading from reset register
    selectPage(IS31FL373X_PAGE_FUNCTION);
    uint8_t dummy;
    readRegister(0x11, &dummy); // Software reset by reading register 0x11
    delay(10); // Wait for reset to complete
}

void IS31FL373x_Device::show() {
    if (_pwmBuffer == nullptr) return;
    
    // Switch to PWM page
    selectPage(IS31FL373X_PAGE_PWM);
    
    // When a custom layout is active, iterate layout entries instead of matrix scan
    if (_useCustomLayout && _customLayout != nullptr && _layoutSize > 0) {
        uint16_t maxIndex = (_layoutSize < getPWMBufferSize()) ? _layoutSize : getPWMBufferSize();
        for (uint16_t i = 0; i < maxIndex; i++) {
            const PixelMapEntry& entry = _customLayout[i];
            // entry.cs and entry.sw are 1-based; apply offsets here
            uint8_t cs = static_cast<uint8_t>(entry.cs + _csOffset);
            uint8_t sw = static_cast<uint8_t>(entry.sw + _swOffset);
            uint16_t regAddress = csSwToIndex(cs, sw);
            writeRegister(static_cast<uint8_t>(regAddress), _pwmBuffer[i]);
        }
        return;
    }
    
    // Default matrix scan using coordinate transformation
    uint8_t width = getWidth();
    uint8_t height = getHeight();
    for (uint8_t row = 0; row < height; row++) {
        for (uint8_t col = 0; col < width; col++) {
            uint16_t bufferIndex = row * width + col;
            uint16_t regAddress = coordToIndex(col, row);
            if (bufferIndex < getPWMBufferSize()) {
                writeRegister(static_cast<uint8_t>(regAddress), _pwmBuffer[bufferIndex]);
            }
        }
    }
}

void IS31FL373x_Device::clear() {
    if (_pwmBuffer != nullptr) {
        memset(_pwmBuffer, 0, getPWMBufferSize());
    }
}

void IS31FL373x_Device::setGlobalCurrent(uint8_t current) {
    _globalCurrent = current;
    // Write to Global Current Control register on Function page
    selectPage(IS31FL373X_PAGE_FUNCTION);
    writeRegister(0x01, current);
}

void IS31FL373x_Device::setMasterBrightness(uint8_t brightness) {
    _masterBrightness = brightness;
}

void IS31FL373x_Device::drawPixel(int16_t x, int16_t y, uint16_t color) {
    // Strict bounds checking to prevent writes to non-existent hardware addresses
    if (x < 0 || y < 0 || x >= getWidth() || y >= getHeight()) {
        return;
    }
    
    // For IS31FL3737B: Additional check to ensure we don't write to CS13-16 (non-existent columns)
    // The getWidth() should already handle this, but double-check for safety
    if (x >= getWidth()) {
        return;
    }
    
    // Calculate buffer index (linear) - this is different from register address
    uint16_t bufferIndex = y * getWidth() + x;
    
    if (bufferIndex < getPWMBufferSize() && _pwmBuffer != nullptr) {
        // Apply master brightness scaling
        uint8_t scaledColor = (color * _masterBrightness) / 255;
        _pwmBuffer[bufferIndex] = scaledColor;
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

void IS31FL373x_Device::setCoordinateOffset(uint8_t csOffset, uint8_t swOffset) {
    _csOffset = csOffset;
    _swOffset = swOffset;
}

bool IS31FL373x_Device::selectPage(uint8_t page) {
    if (_i2c_dev == nullptr) return false;  // Not initialized yet
    
    uint8_t buffer[2];
    
    // Unlock command register
    buffer[0] = IS31FL373X_REG_UNLOCK;
    buffer[1] = IS31FL373X_UNLOCK_VALUE;
    if (!_i2c_dev->write(buffer, 2)) return false;
    
    // Select page
    buffer[0] = IS31FL373X_REG_COMMAND;
    buffer[1] = page;
    return _i2c_dev->write(buffer, 2);
}

bool IS31FL373x_Device::writeRegister(uint8_t reg, uint8_t value) {
    if (_i2c_dev == nullptr) return false;  // Not initialized yet
    uint8_t buffer[2] = {reg, value};
    return _i2c_dev->write(buffer, 2);
}

bool IS31FL373x_Device::readRegister(uint8_t reg, uint8_t* value) {
    if (value == nullptr || _i2c_dev == nullptr) return false;
    
    // Write register address
    if (!_i2c_dev->write(&reg, 1)) return false;
    
    // Read register value
    return _i2c_dev->read(value, 1);
}

uint16_t IS31FL373x_Device::coordToIndex(uint8_t x, uint8_t y) const {
    // Apply coordinate offsets for hardware compatibility
    uint8_t cs = x + _csOffset + 1;  // Convert to 1-based CS (CSx)
    uint8_t sw = y + _swOffset + 1;  // Convert to 1-based SW (SWy)
    return csSwToIndex(cs, sw);
}
uint16_t IS31FL373x_Device::csSwToIndex(uint8_t cs1Based, uint8_t sw1Based) const {
    // Generic mapping: Address = (SWy - 1) * stride + (CSx - 1)
    return static_cast<uint16_t>((sw1Based - 1) * getRegisterStride() + (cs1Based - 1));
}

void IS31FL373x_Device::indexToCoord(uint16_t index, uint8_t* x, uint8_t* y) const {
    // Reverse the hardware register mapping: Address = (SWy - 1) * stride + (CSx - 1)
    uint8_t stride = getRegisterStride();
    uint8_t cs = (index % stride) + 1;  // Extract CS (1-based)
    uint8_t sw = (index / stride) + 1;  // Extract SW (1-based)
    
    // Convert back to 0-based coordinates and apply offsets
    if (x != nullptr) *x = cs - 1 - _csOffset;
    if (y != nullptr) *y = sw - 1 - _swOffset;
}

uint8_t IS31FL373x_Device::getPixelValue(uint16_t x, uint16_t y) const {
    if (x >= getWidth() || y >= getHeight() || _pwmBuffer == nullptr) {
        return 0;
    }
    
    uint16_t bufferIndex = y * getWidth() + x;
    if (bufferIndex < getPWMBufferSize()) {
        return _pwmBuffer[bufferIndex];
    }
    return 0;
}

uint8_t IS31FL373x_Device::getPixelValueByIndex(uint16_t index) const {
    if (index >= getPWMBufferSize() || _pwmBuffer == nullptr) {
        return 0;
    }
    return _pwmBuffer[index];
}

uint16_t IS31FL373x_Device::getNonZeroPixelCount() const {
    if (_pwmBuffer == nullptr) return 0;
    
    uint16_t count = 0;
    uint16_t bufferSize = getPWMBufferSize();
    for (uint16_t i = 0; i < bufferSize; i++) {
        if (_pwmBuffer[i] > 0) {
            count++;
        }
    }
    return count;
}

uint16_t IS31FL373x_Device::getPixelSum() const {
    if (_pwmBuffer == nullptr) return 0;
    
    uint32_t sum = 0;  // Use 32-bit to avoid overflow
    uint16_t bufferSize = getPWMBufferSize();
    for (uint16_t i = 0; i < bufferSize; i++) {
        sum += _pwmBuffer[i];
    }
    return (sum > 65535) ? 65535 : static_cast<uint16_t>(sum);  // Clamp to 16-bit
}

// IS31FL3733 Implementation
IS31FL3733::IS31FL3733(ADDR addr1, ADDR addr2, TwoWire *wire) 
    : IS31FL373x_Device(calculateAddress(addr1, addr2), wire) {
    // Update GFX dimensions
    _width = MATRIX_WIDTH;
    _height = MATRIX_HEIGHT;
}

uint8_t IS31FL3733::calculateAddress(ADDR addr1, ADDR addr2) {
    // Base address: 0b1010000 (0x50)
    // Address bits from ADDR pins
    return 0x50 | ((static_cast<uint8_t>(addr2) & 0x03) << 2) | (static_cast<uint8_t>(addr1) & 0x03);
}

// IS31FL3737 Implementation
IS31FL3737::IS31FL3737(ADDR addr, TwoWire *wire) 
    : IS31FL373x_Device(calculateAddress(addr), wire) {
    // Update GFX dimensions
    _width = MATRIX_WIDTH;
    _height = MATRIX_HEIGHT;
}

uint8_t IS31FL3737::calculateAddress(ADDR addr) {
    // Base address: 0b1010000 (0x50)
    // Address bits from ADDR pin mapping per IS31FL373x-reference.md:
    // GND=0000, SCL=0101, SDA=1010, VCC=1111
    uint8_t addrBits;
    switch(addr) {
        case ADDR::GND: addrBits = 0b0000; break;  // 0x0
        case ADDR::SCL: addrBits = 0b0101; break;  // 0x5  
        case ADDR::SDA: addrBits = 0b1010; break;  // 0xA
        case ADDR::VCC: addrBits = 0b1111; break;  // 0xF
        default: addrBits = 0b0000; break;
    }
    return 0x50 | addrBits;
}

uint16_t IS31FL3737::coordToIndex(uint8_t x, uint8_t y) const {
    // Apply offsets via base then chip-specific quirk via csSwToIndex
    uint8_t cs = x + _csOffset + 1;
    uint8_t sw = y + _swOffset + 1;
    return csSwToIndex(cs, sw);
}
uint16_t IS31FL3737::csSwToIndex(uint8_t cs1Based, uint8_t sw1Based) const {
    // Apply IS31FL3737 hardware remapping for CS6-CS11 (0-based) => CS7-CS12 (1-based)
    uint8_t cs = cs1Based;
    if (cs >= 7 && cs <= 12) {
        cs = static_cast<uint8_t>(cs + 2);
    }
    return static_cast<uint16_t>((sw1Based - 1) * getRegisterStride() + (cs - 1));
}

void IS31FL3737::indexToCoord(uint16_t index, uint8_t* x, uint8_t* y) const {
    // Reverse the hardware register mapping with IS31FL3737 quirk
    uint8_t stride = getRegisterStride();
    uint8_t cs = (index % stride) + 1;  // Extract CS (1-based)
    uint8_t sw = (index / stride) + 1;  // Extract SW (1-based)
    
    // Reverse the IS31FL3737 hardware remapping for CS9-CS14 (8-13 in 0-based)
    if (cs >= 9 && cs <= 14) {  // CS9-CS14 in 1-based (8-13 in 0-based)
        cs -= 2;  // Reverse remap to CS7-CS12 (6-11 in 0-based)
    }
    
    // Convert back to 0-based coordinates and apply offsets
    if (x != nullptr) *x = cs - 1 - _csOffset;
    if (y != nullptr) *y = sw - 1 - _swOffset;
}

// IS31FL3737B Implementation  
IS31FL3737B::IS31FL3737B(ADDR addr, TwoWire *wire) 
    : IS31FL373x_Device(calculateAddress(addr), wire) {
    // Update GFX dimensions
    _width = MATRIX_WIDTH;
    _height = MATRIX_HEIGHT;
}

uint8_t IS31FL3737B::calculateAddress(ADDR addr) {
    // Base address: 0b1010000 (0x50)
    // Address bits from ADDR pin mapping per IS31FL373x-reference.md:
    // GND=0000, SCL=0101, SDA=1010, VCC=1111
    uint8_t addrBits;
    switch(addr) {
        case ADDR::GND: addrBits = 0b0000; break;  // 0x0
        case ADDR::SCL: addrBits = 0b0101; break;  // 0x5  
        case ADDR::SDA: addrBits = 0b1010; break;  // 0xA
        case ADDR::VCC: addrBits = 0b1111; break;  // 0xF
        default: addrBits = 0b0000; break;
    }
    return 0x50 | addrBits;
}

void IS31FL3737B::setPWMFrequency(uint8_t freq) {
    // TODO: Implement PWM frequency setting specific to 3737B
    (void)freq;
    // TODO(test): verify correct FUNCTION page writes when implemented
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

uint16_t IS31FL373x_Canvas::getTotalNonZeroPixelCount() const {
    uint16_t totalCount = 0;
    for (uint8_t i = 0; i < _deviceCount; i++) {
        if (_devices[i] != nullptr) {
            totalCount += _devices[i]->getNonZeroPixelCount();
        }
    }
    return totalCount;
}

IS31FL373x_Device* IS31FL373x_Canvas::getDeviceForCoordinate(int16_t x, int16_t y, 
                                                           int16_t* localX, int16_t* localY) {
    if (_deviceCount == 0) return nullptr;
    // Horizontal: slice x across devices by cumulative widths
    if (_layout == LAYOUT_HORIZONTAL) {
        int16_t cursor = 0;
        for (uint8_t i = 0; i < _deviceCount; i++) {
            IS31FL373x_Device* dev = _devices[i];
            if (dev == nullptr) continue;
            int16_t next = cursor + dev->getWidth();
            if (x >= cursor && x < next && y >= 0 && y < dev->getHeight()) {
                *localX = x - cursor;
                *localY = y;
                return dev;
            }
            cursor = next;
        }
        return nullptr;
    }
    // Vertical: slice y across devices by cumulative heights
    if (_layout == LAYOUT_VERTICAL) {
        int16_t cursor = 0;
        for (uint8_t i = 0; i < _deviceCount; i++) {
            IS31FL373x_Device* dev = _devices[i];
            if (dev == nullptr) continue;
            int16_t next = cursor + dev->getHeight();
            if (y >= cursor && y < next && x >= 0 && x < dev->getWidth()) {
                *localX = x;
                *localY = y - cursor;
                return dev;
            }
            cursor = next;
        }
        return nullptr;
    }
    return nullptr;
}
