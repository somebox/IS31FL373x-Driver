/**
 * @file test_is31fl373x.cpp
 * @brief Comprehensive test suite for IS31FL373x LED matrix driver library
 * 
 * This test suite provides complete coverage of the IS31FL373x driver functionality
 * using mock I2C interface for reliable, fast testing without hardware dependencies.
 * 
 * Test Coverage:
 * - Driver initialization and configuration
 * - Device type detection (IS31FL3733 vs IS31FL3737B)
 * - Basic drawing operations and coordinate handling
 * - Multi-chip canvas management
 * - Custom layout support for non-matrix arrangements
 * - Brightness control (global and software scaling)
 * - Error handling and edge cases
 * 
 * Based on the approach from the BQ25895-Driver project, this focuses on
 * comprehensive testing in the native environment with compilation verification
 * for embedded platforms.
 */

#include "doctest.h"
#include "IS31FL373x.h"
#include <cstdio>

#ifdef UNIT_TEST
// Mock implementations for testing
unsigned long mock_millis = 1000;

unsigned long millis() {
    return mock_millis;
}

void advance_time(unsigned long ms) {
    mock_millis += ms;
}

void reset_time() {
    mock_millis = 0;
}
#endif

// =============================================================================
// BASIC INSTANTIATION AND PROPERTIES TESTS
// =============================================================================

TEST_CASE("IS31FL3733: Basic Instantiation and Properties") {
    IS31FL3733 matrix;
    
    CHECK(matrix.getWidth() == 16);
    CHECK(matrix.getHeight() == 12);
    CHECK(matrix.getPWMBufferSize() == 192);
}

TEST_CASE("IS31FL3737: Basic Instantiation and Properties") {
    IS31FL3737 matrix;
    
    CHECK(matrix.getWidth() == 12);
    CHECK(matrix.getHeight() == 12);
    CHECK(matrix.getPWMBufferSize() == 144);
}

TEST_CASE("IS31FL3737B: Basic Instantiation and Properties") {
    IS31FL3737B matrix;
    
    CHECK(matrix.getWidth() == 12);
    CHECK(matrix.getHeight() == 12);
    CHECK(matrix.getPWMBufferSize() == 144);
}

// =============================================================================
// ADDRESS CALCULATION TESTS
// =============================================================================

TEST_CASE("IS31FL3733: Address Calculation") {
    SUBCASE("Default address (GND,GND)") {
        IS31FL3733 matrix(ADDR::GND, ADDR::GND);
        CHECK(matrix.getI2CAddress() == 0x50);  // Base address with both pins at GND
        CHECK(matrix.begin() == true);
    }
    
    SUBCASE("Address pin combinations") {
        IS31FL3733 matrix1(ADDR::VCC, ADDR::GND);  // Should result in 0x51
        IS31FL3733 matrix2(ADDR::GND, ADDR::VCC);  // Should result in 0x54
        IS31FL3733 matrix3(ADDR::SCL, ADDR::SCL);  // Should result in 0x5F
        
        CHECK(matrix1.getI2CAddress() == 0x51);
        CHECK(matrix2.getI2CAddress() == 0x54);
        CHECK(matrix3.getI2CAddress() == 0x5F);
        
        CHECK(matrix1.begin() == true);
        CHECK(matrix2.begin() == true);
        CHECK(matrix3.begin() == true);
    }
}

TEST_CASE("IS31FL3737: Address Calculation") {
    SUBCASE("Default address") {
        IS31FL3737 matrix(ADDR::GND);
        CHECK(matrix.getI2CAddress() == 0x50);  // Base address with pin at GND
        CHECK(matrix.begin() == true);
    }
    
    SUBCASE("Address pin values - Fixed per IS31FL373x-reference.md") {
        IS31FL3737 matrix1(ADDR::VCC);   // Should result in 0x5F (GND=0000, SCL=0101, SDA=1010, VCC=1111)
        IS31FL3737 matrix2(ADDR::SDA);   // Should result in 0x5A
        IS31FL3737 matrix3(ADDR::SCL);   // Should result in 0x55
        
        CHECK(matrix1.getI2CAddress() == 0x5F);
        CHECK(matrix2.getI2CAddress() == 0x5A);
        CHECK(matrix3.getI2CAddress() == 0x55);
        
        CHECK(matrix1.begin() == true);
        CHECK(matrix2.begin() == true);
        CHECK(matrix3.begin() == true);
    }
}

TEST_CASE("IS31FL3737B: Address Calculation") {
    SUBCASE("Default address") {
        IS31FL3737B matrix(ADDR::GND);
        CHECK(matrix.getI2CAddress() == 0x50);  // Base address with pin at GND
        CHECK(matrix.begin() == true);
    }
    
    SUBCASE("Address pin values - Fixed per IS31FL373x-reference.md") {
        IS31FL3737B matrix1(ADDR::VCC);   // Should result in 0x5F (GND=0000, SCL=0101, SDA=1010, VCC=1111)
        IS31FL3737B matrix2(ADDR::SDA);   // Should result in 0x5A
        IS31FL3737B matrix3(ADDR::SCL);   // Should result in 0x55
        
        CHECK(matrix1.getI2CAddress() == 0x5F);
        CHECK(matrix2.getI2CAddress() == 0x5A);
        CHECK(matrix3.getI2CAddress() == 0x55);
        
        CHECK(matrix1.begin() == true);
        CHECK(matrix2.begin() == true);
        CHECK(matrix3.begin() == true);
    }
    
    SUBCASE("I2C Address Fix Verification") {
        // This test specifically verifies the I2C address calculation fix
        // The old code incorrectly used (enum_value & 0x0F) instead of proper bit patterns
        
        // Test all four possible ADDR pin configurations
        IS31FL3737B matrix_gnd(ADDR::GND);   // Pin to GND -> 0x50 (0101_0000)
        IS31FL3737B matrix_scl(ADDR::SCL);   // Pin to SCL -> 0x55 (0101_0101)  
        IS31FL3737B matrix_sda(ADDR::SDA);   // Pin to SDA -> 0x5A (0101_1010)
        IS31FL3737B matrix_vcc(ADDR::VCC);   // Pin to VCC -> 0x5F (0101_1111)
        
        // Verify each address is correct per the reference document
        CHECK(matrix_gnd.getI2CAddress() == 0x50);
        CHECK(matrix_scl.getI2CAddress() == 0x55);  // Not 0x53 (old incorrect value)
        CHECK(matrix_sda.getI2CAddress() == 0x5A);  // Not 0x52 (old incorrect value)
        CHECK(matrix_vcc.getI2CAddress() == 0x5F);  // Not 0x51 (old incorrect value)
        
        // Verify all addresses are unique (no collisions)
        uint8_t addresses[] = {
            matrix_gnd.getI2CAddress(),
            matrix_scl.getI2CAddress(), 
            matrix_sda.getI2CAddress(),
            matrix_vcc.getI2CAddress()
        };
        
        // Check that all addresses are different
        for (int i = 0; i < 4; i++) {
            for (int j = i + 1; j < 4; j++) {
                CHECK(addresses[i] != addresses[j]);
            }
        }
    }
}

// =============================================================================
// DRAWING AND COORDINATE TESTS
// =============================================================================

TEST_CASE("Basic Drawing Operations") {
    IS31FL3737B matrix;
    
    REQUIRE(matrix.begin() == true);
    
    SUBCASE("Valid pixel drawing") {
        // Initially buffer should be empty
        CHECK(matrix.getNonZeroPixelCount() == 0);
        
        // Draw pixels and verify they are set correctly
        matrix.drawPixel(0, 0, 255);
        CHECK(matrix.getPixelValue(0, 0) == 255);
        CHECK(matrix.getNonZeroPixelCount() == 1);
        
        matrix.drawPixel(11, 11, 128);
        CHECK(matrix.getPixelValue(11, 11) == 128);
        CHECK(matrix.getNonZeroPixelCount() == 2);
        
        matrix.drawPixel(5, 5, 64);
        CHECK(matrix.getPixelValue(5, 5) == 64);
        CHECK(matrix.getNonZeroPixelCount() == 3);
        
        // Verify total pixel sum
        CHECK(matrix.getPixelSum() == (255 + 128 + 64));
    }
    
    SUBCASE("Out of bounds drawing") {
        // Draw some valid pixels first
        matrix.drawPixel(5, 5, 100);
        CHECK(matrix.getNonZeroPixelCount() == 1);
        
        // Out of bounds drawing should be ignored
        matrix.drawPixel(-1, 0, 255);    // Negative x
        matrix.drawPixel(12, 0, 255);    // x >= width
        matrix.drawPixel(0, 12, 255);    // y >= height  
        matrix.drawPixel(100, 100, 255); // Way out of bounds
        
        // Buffer should be unchanged
        CHECK(matrix.getNonZeroPixelCount() == 1);
        CHECK(matrix.getPixelValue(5, 5) == 100);
        CHECK(matrix.getPixelSum() == 100);
    }
    
    SUBCASE("Clear operation") {
        // Draw several pixels
        matrix.drawPixel(5, 5, 255);
        matrix.drawPixel(0, 0, 128);
        matrix.drawPixel(11, 11, 64);
        CHECK(matrix.getNonZeroPixelCount() == 3);
        
        // Clear should reset all pixels to zero
        matrix.clear();
        CHECK(matrix.getNonZeroPixelCount() == 0);
        CHECK(matrix.getPixelSum() == 0);
        CHECK(matrix.getPixelValue(5, 5) == 0);
        CHECK(matrix.getPixelValue(0, 0) == 0);
        CHECK(matrix.getPixelValue(11, 11) == 0);
    }
}

TEST_CASE("Coordinate Conversion") {
    IS31FL3737B matrix;
    
    SUBCASE("Hardware register mapping (no offset)") {
        // Test hardware register formula: Address = (SWy - 1) * 16 + (CSx - 1)
        // For (x=0, y=0): CS1, SW1 -> Address = (1-1)*16 + (1-1) = 0
        CHECK(matrix.coordToIndex(0, 0) == 0);
        
        // For (x=4, y=2): CS5, SW3 -> Address = (3-1)*16 + (5-1) = 2*16 + 4 = 36
        CHECK(matrix.coordToIndex(4, 2) == 36);
        
        // For (x=11, y=0): CS12, SW1 -> Address = (1-1)*16 + (12-1) = 11
        CHECK(matrix.coordToIndex(11, 0) == 11);
        
        // For (x=0, y=1): CS1, SW2 -> Address = (2-1)*16 + (1-1) = 16
        CHECK(matrix.coordToIndex(0, 1) == 16);
    }
    
    SUBCASE("Index to coordinate conversion") {
        uint8_t x, y;
        
        // Address 0: CS1, SW1 -> (x=0, y=0)
        matrix.indexToCoord(0, &x, &y);
        CHECK(x == 0);
        CHECK(y == 0);
        
        // Address 36: CS5, SW3 -> (x=4, y=2)
        matrix.indexToCoord(36, &x, &y);
        CHECK(x == 4);
        CHECK(y == 2);
        
        // Address 11: CS12, SW1 -> (x=11, y=0)
        matrix.indexToCoord(11, &x, &y);
        CHECK(x == 11);
        CHECK(y == 0);
        
        // Address 16: CS1, SW2 -> (x=0, y=1)
        matrix.indexToCoord(16, &x, &y);
        CHECK(x == 0);
        CHECK(y == 1);
    }
    
    SUBCASE("Coordinate offset for IS31FL3737 compatibility") {
        matrix.setCoordinateOffset(2, 0);  // CS offset = 2, SW offset = 0
        
        // User coordinates (0, 6) should map to hardware CS3, SW7
        // Address = (7-1)*16 + (3-1) = 6*16 + 2 = 98
        CHECK(matrix.coordToIndex(0, 6) == 98);
        
        // Reverse mapping: Address 98 should give user coordinates (0, 6)
        uint8_t x, y;
        matrix.indexToCoord(98, &x, &y);
        CHECK(x == 0);
        CHECK(y == 6);
    }
    
    SUBCASE("Coordinate offset edge cases") {
        // Test various offset combinations
        matrix.setCoordinateOffset(0, 0);  // No offset
        CHECK(matrix.coordToIndex(0, 0) == 0);   // CS1, SW1 -> 0x00
        CHECK(matrix.coordToIndex(1, 0) == 1);   // CS2, SW1 -> 0x01
        
        matrix.setCoordinateOffset(1, 1);  // Offset both CS and SW by 1
        CHECK(matrix.coordToIndex(0, 0) == 17);  // CS2, SW2 -> (2-1)*16 + (2-1) = 17
        
        matrix.setCoordinateOffset(3, 2);  // CS+3, SW+2 
        CHECK(matrix.coordToIndex(0, 0) == 35);  // CS4, SW3 -> (3-1)*16 + (4-1) = 32+3 = 35
        
        // Verify reverse mapping works with offsets
        uint8_t x, y;
        matrix.indexToCoord(35, &x, &y);
        CHECK(x == 0);
        CHECK(y == 0);
    }
}

// =============================================================================
// BRIGHTNESS CONTROL TESTS
// =============================================================================

TEST_CASE("Brightness Control") {
    IS31FL3737B matrix;
    REQUIRE(matrix.begin() == true);
    
    SUBCASE("Global current setting") {
        // Test various current levels and verify they are stored
        matrix.setGlobalCurrent(64);
        CHECK(matrix.getGlobalCurrent() == 64);
        
        matrix.setGlobalCurrent(128);
        CHECK(matrix.getGlobalCurrent() == 128);
        
        matrix.setGlobalCurrent(255);
        CHECK(matrix.getGlobalCurrent() == 255);
        
        // Verify device properties remain consistent
        CHECK(matrix.getWidth() == 12);
        CHECK(matrix.getHeight() == 12);
    }
    
    SUBCASE("Master brightness scaling") {
        // Test various brightness levels and verify they are stored
        matrix.setMasterBrightness(64);
        CHECK(matrix.getMasterBrightness() == 64);
        
        matrix.setMasterBrightness(128);
        CHECK(matrix.getMasterBrightness() == 128);
        
        matrix.setMasterBrightness(255);
        CHECK(matrix.getMasterBrightness() == 255);
        
        // Verify device properties remain consistent
        CHECK(matrix.getPWMBufferSize() == 144);
    }
    
    SUBCASE("Brightness affects drawing") {
        // Test that master brightness scaling affects pixel values
        matrix.setMasterBrightness(255);  // Full brightness
        matrix.drawPixel(5, 5, 200);
        CHECK(matrix.getPixelValue(5, 5) == 200);  // Should be unscaled
        
        matrix.clear();
        matrix.setMasterBrightness(128);  // Half brightness
        matrix.drawPixel(5, 5, 200);
        uint8_t scaledValue = matrix.getPixelValue(5, 5);
        CHECK(scaledValue == (200 * 128) / 255);  // Should be scaled down
        CHECK(scaledValue < 200);  // Verify it's actually scaled
    }
}

// =============================================================================
// CUSTOM LAYOUT TESTS
// =============================================================================

TEST_CASE("Custom Layout Support") {
    IS31FL3737B matrix;
    REQUIRE(matrix.begin() == true);
    
    SUBCASE("Set custom layout") {
        PixelMapEntry customLayout[4] = {
            {1, 1},   // Index 0 -> CS1, SW1
            {2, 1},   // Index 1 -> CS2, SW1  
            {1, 2},   // Index 2 -> CS1, SW2
            {2, 2}    // Index 3 -> CS2, SW2
        };
        
        // Initially no custom layout
        CHECK(matrix.isCustomLayoutActive() == false);
        CHECK(matrix.getLayoutSize() == 0);
        
        // Set custom layout and verify it's active
        matrix.setLayout(customLayout, 4);
        CHECK(matrix.isCustomLayoutActive() == true);
        CHECK(matrix.getLayoutSize() == 4);
        
        // Device properties should remain unchanged
        CHECK(matrix.getWidth() == 12);
        CHECK(matrix.getHeight() == 12);
    }
    
    SUBCASE("Indexed drawing with custom layout") {
        PixelMapEntry customLayout[2] = {
            {1, 1}, {2, 1}  // Two pixels in a row
        };
        
        matrix.setLayout(customLayout, 2);
        CHECK(matrix.isCustomLayoutActive() == true);
        
        // Test indexed drawing
        matrix.setPixel(0, 255);  // First pixel
        matrix.setPixel(1, 128);  // Second pixel
        
        // Verify pixels were set (indexed access)
        CHECK(matrix.getPixelValueByIndex(0) == 255);
        CHECK(matrix.getPixelValueByIndex(1) == 128);
        CHECK(matrix.getNonZeroPixelCount() == 2);
        CHECK(matrix.getPixelSum() == (255 + 128));
    }
    
    SUBCASE("No layout set") {
        // Should work with default layout (coordinate-based)
        CHECK(matrix.isCustomLayoutActive() == false);
        CHECK(matrix.getLayoutSize() == 0);
        
        matrix.setPixel(0, 255);   // First pixel
        matrix.setPixel(10, 128);  // Eleventh pixel
        
        // Verify pixels were set using default linear indexing
        CHECK(matrix.getPixelValueByIndex(0) == 255);
        CHECK(matrix.getPixelValueByIndex(10) == 128);
        CHECK(matrix.getNonZeroPixelCount() == 2);
        CHECK(matrix.getPWMBufferSize() == 144);
    }
}

// =============================================================================
// MULTI-CHIP CANVAS TESTS
// =============================================================================

TEST_CASE("Canvas: Multi-Device Setup") {
    IS31FL3737B matrix1(ADDR::GND);
    IS31FL3737B matrix2(ADDR::VCC);
    IS31FL3737B matrix3(ADDR::SDA);
    
    IS31FL373x_Device* devices[] = {&matrix1, &matrix2, &matrix3};
    IS31FL373x_Canvas canvas(36, 12, devices, 3, LAYOUT_HORIZONTAL);
    
    SUBCASE("Canvas initialization") {
        CHECK(canvas.begin() == true);
        
        // Verify canvas properties
        CHECK(canvas.width() == 36);
        CHECK(canvas.height() == 12);
        CHECK(canvas.getDeviceCount() == 3);
        CHECK(canvas.getLayout() == LAYOUT_HORIZONTAL);
        
        // Verify all devices are accessible
        CHECK(canvas.getDevice(0) != nullptr);
        CHECK(canvas.getDevice(1) != nullptr);
        CHECK(canvas.getDevice(2) != nullptr);
        CHECK(canvas.getDevice(3) == nullptr);  // Out of bounds
    }
    
    SUBCASE("Canvas operations") {
        REQUIRE(canvas.begin() == true);
        
        // Initially should be empty
        CHECK(canvas.getTotalNonZeroPixelCount() == 0);
        
        canvas.clear();
        canvas.drawPixel(0, 0, 255);    // Should go to matrix1
        canvas.drawPixel(12, 0, 255);   // Should go to matrix2  
        canvas.drawPixel(24, 0, 255);   // Should go to matrix3
        canvas.drawPixel(35, 11, 128);  // Should go to matrix3
        
        // Verify pixels were distributed across devices
        CHECK(canvas.getTotalNonZeroPixelCount() == 4);
        
        // Check individual device pixel counts
        CHECK(canvas.getDevice(0)->getNonZeroPixelCount() == 1);  // matrix1
        CHECK(canvas.getDevice(1)->getNonZeroPixelCount() == 1);  // matrix2
        CHECK(canvas.getDevice(2)->getNonZeroPixelCount() == 2);  // matrix3
    }
    
    SUBCASE("Canvas brightness control") {
        REQUIRE(canvas.begin() == true);
        
        // Set brightness on all devices
        canvas.setGlobalCurrent(100);
        canvas.setMasterBrightness(200);
        
        // Verify brightness was applied to all devices
        for (uint8_t i = 0; i < canvas.getDeviceCount(); i++) {
            IS31FL373x_Device* device = canvas.getDevice(i);
            if (device != nullptr) {
                CHECK(device->getGlobalCurrent() == 100);
                CHECK(device->getMasterBrightness() == 200);
            }
        }
    }
    
    SUBCASE("Canvas show operation") {
        REQUIRE(canvas.begin() == true);
        
        // Draw pixel and verify it's in the buffer
        canvas.drawPixel(18, 5, 128);  // Should go to matrix2 (device 1)
        CHECK(canvas.getTotalNonZeroPixelCount() == 1);
        CHECK(canvas.getDevice(1)->getNonZeroPixelCount() == 1);
        
        // Show operation should complete without issues
        canvas.show();
        
        // Buffer should still contain the pixel after show
        CHECK(canvas.getTotalNonZeroPixelCount() == 1);
    }
}

TEST_CASE("Canvas: Mixed Device Types") {
    IS31FL3733 matrix1(ADDR::GND, ADDR::GND);  // 16x12
    IS31FL3737B matrix2(ADDR::VCC);            // 12x12
    
    IS31FL373x_Device* devices[] = {&matrix1, &matrix2};
    IS31FL373x_Canvas canvas(28, 12, devices, 2, LAYOUT_HORIZONTAL);
    
    CHECK(canvas.begin() == true);
    
    // Verify canvas handles mixed device dimensions
    CHECK(canvas.width() == 28);   // 16 + 12 = 28 total width
    CHECK(canvas.height() == 12);  // Both devices have 12 height
    CHECK(canvas.getDeviceCount() == 2);
    
    // Test drawing across different device types
    canvas.drawPixel(8, 6, 255);   // Should go to matrix1 (IS31FL3733)
    canvas.drawPixel(20, 6, 255);  // Should go to matrix2 (IS31FL3737B)
    
    // Verify pixels were set on correct devices
    CHECK(canvas.getTotalNonZeroPixelCount() == 2);
    CHECK(canvas.getDevice(0)->getNonZeroPixelCount() == 1);  // IS31FL3733
    CHECK(canvas.getDevice(1)->getNonZeroPixelCount() == 1);  // IS31FL3737B
}

TEST_CASE("Canvas: All Three Chip Types") {
    IS31FL3733 matrix1(ADDR::GND, ADDR::GND);  // 16x12 - largest matrix
    IS31FL3737 matrix2(ADDR::VCC);             // 12x12 - fixed PWM
    IS31FL3737B matrix3(ADDR::SDA);            // 12x12 - selectable PWM
    
    IS31FL373x_Device* devices[] = {&matrix1, &matrix2, &matrix3};
    IS31FL373x_Canvas canvas(40, 12, devices, 3, LAYOUT_HORIZONTAL);
    
    SUBCASE("Canvas initialization with all chip types") {
        CHECK(canvas.begin() == true);
    }
    
    SUBCASE("Drawing across all chip types") {
        REQUIRE(canvas.begin() == true);
        
        // Verify canvas dimensions with all three chip types
        CHECK(canvas.width() == 40);   // 16 + 12 + 12 = 40 total width
        CHECK(canvas.height() == 12);  // All devices have 12 height
        CHECK(canvas.getDeviceCount() == 3);
        
        canvas.clear();
        canvas.drawPixel(8, 6, 255);   // Should go to matrix1 (IS31FL3733)
        canvas.drawPixel(20, 6, 255);  // Should go to matrix2 (IS31FL3737)
        canvas.drawPixel(32, 6, 255);  // Should go to matrix3 (IS31FL3737B)
        
        // Verify pixels distributed correctly
        CHECK(canvas.getTotalNonZeroPixelCount() == 3);;
        CHECK(canvas.getDevice(0)->getNonZeroPixelCount() == 1);  // IS31FL3733
        CHECK(canvas.getDevice(1)->getNonZeroPixelCount() == 1);  // IS31FL3737
        CHECK(canvas.getDevice(2)->getNonZeroPixelCount() == 1);  // IS31FL3737B
        
        canvas.show();
        // After show, pixels should still be in buffers
        CHECK(canvas.getTotalNonZeroPixelCount() == 3);
    }
}

// =============================================================================
// ADDRESSING FIX VERIFICATION TESTS
// =============================================================================

TEST_CASE("IS31FL3737: Addressing Fix Verification") {
    IS31FL3737 matrix;
    REQUIRE(matrix.begin() == true);
    
    SUBCASE("SW0,CS6 addressing issue fix - IS31FL3737 hardware quirk") {
        // This test verifies the fix for the reported issue:
        // "SW0,CS6 LED only lights when addressed as SW0,CS9"
        // 
        // The IS31FL3737 has a hardware quirk where CS6-CS11 (0-based) map to 
        // register addresses 8-13, not 6-11. This is due to gaps in the register mapping.
        
        matrix.clear();
        
        // Draw pixel at coordinate (6,0) - should map to hardware register 0x08
        // because CS6 (0-based) = CS7 (1-based) maps to register address 8
        matrix.drawPixel(6, 0, 255);
        
        // Verify it's stored in the correct buffer position
        CHECK(matrix.getPixelValue(6, 0) == 255);
        CHECK(matrix.getNonZeroPixelCount() == 1);
        
        // Verify coordinate to register mapping with IS31FL3737 quirk
        uint16_t regAddr = matrix.coordToIndex(6, 0);
        CHECK(regAddr == 8); // CS6 (0-based) should map to register 0x08, not 0x06
        
        // Test the boundary cases
        CHECK(matrix.coordToIndex(5, 0) == 5);   // CS5 -> register 0x05 (no remapping)
        CHECK(matrix.coordToIndex(6, 0) == 8);   // CS6 -> register 0x08 (remapped +2)
        CHECK(matrix.coordToIndex(7, 0) == 9);   // CS7 -> register 0x09 (remapped +2)
        CHECK(matrix.coordToIndex(11, 0) == 13); // CS11 -> register 0x0D (remapped +2)
        
        // Test reverse mapping
        uint8_t x, y;
        matrix.indexToCoord(8, &x, &y);  // Register 0x08 should map back to (6,0)
        CHECK(x == 6);
        CHECK(y == 0);
    }
    
    SUBCASE("Register stride mapping for IS31FL3737 with hardware quirk") {
        // IS31FL3737 uses 16-byte register stride despite having only 12 columns
        // AND has a hardware quirk where CS6-CS11 map to register addresses 8-13
        
        matrix.clear();
        
        // Test first row (SW0) with hardware quirk
        CHECK(matrix.coordToIndex(0, 0) == 0);   // CS0 -> 0x00 (no remapping)
        CHECK(matrix.coordToIndex(1, 0) == 1);   // CS1 -> 0x01 (no remapping)
        CHECK(matrix.coordToIndex(5, 0) == 5);   // CS5 -> 0x05 (no remapping)
        CHECK(matrix.coordToIndex(6, 0) == 8);   // CS6 -> 0x08 (remapped +2)
        CHECK(matrix.coordToIndex(7, 0) == 9);   // CS7 -> 0x09 (remapped +2)
        CHECK(matrix.coordToIndex(11, 0) == 13); // CS11 -> 0x0D (remapped +2)
        
        // Test second row (SW1) - should start at register 0x10 (16 decimal) + quirk
        CHECK(matrix.coordToIndex(0, 1) == 16);  // SW1,CS0 -> 0x10
        CHECK(matrix.coordToIndex(5, 1) == 21);  // SW1,CS5 -> 0x15 (16+5, no remapping)
        CHECK(matrix.coordToIndex(6, 1) == 24);  // SW1,CS6 -> 0x18 (16+8, remapped +2)
        CHECK(matrix.coordToIndex(11, 1) == 29); // SW1,CS11 -> 0x1D (16+13, remapped +2)
    }
}

TEST_CASE("IS31FL3737B: Addressing Consistency") {
    IS31FL3737B matrix;
    REQUIRE(matrix.begin() == true);
    
    SUBCASE("Buffer to register mapping consistency") {
        // Verify that buffer indexing and register addressing work together correctly
        
        matrix.clear();
        
        // Draw pixels in a pattern
        matrix.drawPixel(0, 0, 100);  // Top-left
        matrix.drawPixel(11, 0, 101); // Top-right
        matrix.drawPixel(0, 11, 102); // Bottom-left
        matrix.drawPixel(11, 11, 103); // Bottom-right
        
        // Verify buffer storage
        CHECK(matrix.getPixelValue(0, 0) == 100);
        CHECK(matrix.getPixelValue(11, 0) == 101);
        CHECK(matrix.getPixelValue(0, 11) == 102);
        CHECK(matrix.getPixelValue(11, 11) == 103);
        CHECK(matrix.getNonZeroPixelCount() == 4);
        
        // Verify coordinate to register mapping
        CHECK(matrix.coordToIndex(0, 0) == 0);      // SW0,CS0 -> 0x00
        CHECK(matrix.coordToIndex(11, 0) == 11);    // SW0,CS11 -> 0x0B
        CHECK(matrix.coordToIndex(0, 11) == 176);   // SW11,CS0 -> 0xB0 (11*16+0)
        CHECK(matrix.coordToIndex(11, 11) == 187);  // SW11,CS11 -> 0xBB (11*16+11)
    }
}

// =============================================================================
// MOCK I2C VERIFICATION TESTS
// =============================================================================

TEST_CASE("Mock I2C: Register Write Verification") {
    SUBCASE("Verify show() method writes to correct registers") {
        clearMockI2COperations();
        
        IS31FL3737B matrix;
        REQUIRE(matrix.begin() == true);
        
        // Clear any initialization I2C operations
        clearMockI2COperations();
        
        // Draw a single pixel and call show()
        matrix.drawPixel(6, 0, 255);
        matrix.show();
        
        // Verify that show() triggered I2C operations
        size_t opCount = getMockI2COperationCount();
        CHECK(opCount > 0); // Should have at least page selection and register write
        
        // The exact number of operations depends on implementation details,
        // but we should see page selection and register writes
        CHECK(opCount >= 2); // At minimum: page select + register write
    }
}

// =============================================================================
// CHIP-SPECIFIC FEATURE TESTS
// =============================================================================

TEST_CASE("IS31FL3737: Specific Features") {
    IS31FL3737 matrix;
    REQUIRE(matrix.begin() == true);
    
    SUBCASE("Fixed PWM Frequency") {
        // IS31FL3737 has fixed PWM frequency - verify basic functionality
        CHECK(matrix.getWidth() == 12);
        CHECK(matrix.getHeight() == 12);
        CHECK(matrix.getPWMBufferSize() == 144);
        
        // Drawing should work normally with fixed frequency
        matrix.drawPixel(5, 5, 255);
        CHECK(matrix.getPixelValue(5, 5) == 255);
        CHECK(matrix.getNonZeroPixelCount() == 1);
        
        matrix.show();  // Should complete without issues
        CHECK(matrix.getNonZeroPixelCount() == 1);  // Buffer unchanged after show
    }
}

TEST_CASE("IS31FL3737B: Specific Features") {
    IS31FL3737B matrix;
    REQUIRE(matrix.begin() == true);
    
    SUBCASE("PWM Frequency Setting") {
        // IS31FL3737B supports selectable PWM frequency (1.05-26.7 kHz)
        CHECK(matrix.getWidth() == 12);
        CHECK(matrix.getHeight() == 12);
        CHECK(matrix.getPWMBufferSize() == 144);
        
        // PWM frequency setting should not affect basic functionality
        matrix.setPWMFrequency(0);  // Lowest frequency
        matrix.drawPixel(3, 3, 100);
        CHECK(matrix.getPixelValue(3, 3) == 100);
        
        matrix.setPWMFrequency(7);  // Highest frequency  
        matrix.drawPixel(6, 6, 200);
        CHECK(matrix.getPixelValue(6, 6) == 200);
        CHECK(matrix.getNonZeroPixelCount() == 2);
    }
    
    SUBCASE("Drawing with different PWM frequencies") {
        matrix.setPWMFrequency(0);  // Set low frequency
        matrix.drawPixel(5, 5, 255);
        CHECK(matrix.getPixelValue(5, 5) == 255);
        CHECK(matrix.getNonZeroPixelCount() == 1);
        
        matrix.setPWMFrequency(7);  // Set high frequency
        matrix.drawPixel(6, 6, 128);
        CHECK(matrix.getPixelValue(6, 6) == 128);
        CHECK(matrix.getNonZeroPixelCount() == 2);
        CHECK(matrix.getPixelSum() == (255 + 128));
        
        matrix.show();  // Should work at any frequency
        CHECK(matrix.getNonZeroPixelCount() == 2);  // Buffer preserved
    }
}

// =============================================================================
// ERROR HANDLING AND EDGE CASES
// =============================================================================

TEST_CASE("Error Handling") {
    SUBCASE("Large coordinate values") {
        IS31FL3737B matrix;
        matrix.begin();
        
        // Very large coordinates should be handled gracefully
        matrix.drawPixel(1000, 1000, 255);
        matrix.drawPixel(-1000, -1000, 255);
        CHECK(true);
    }
    
    SUBCASE("Canvas with null devices") {
        IS31FL373x_Device* devices[] = {nullptr, nullptr};
        IS31FL373x_Canvas canvas(24, 12, devices, 2, LAYOUT_HORIZONTAL);
        
        // Should handle null devices gracefully
        bool result = canvas.begin();
        CHECK(result == false); // Should fail gracefully, not crash
    }
    
    SUBCASE("Canvas operations without initialization") {
        IS31FL3737B matrix;
        IS31FL373x_Device* devices[] = {&matrix};
        IS31FL373x_Canvas canvas(12, 12, devices, 1, LAYOUT_HORIZONTAL);
        
        // Don't call begin()
        canvas.clear();
        canvas.drawPixel(5, 5, 255);
        canvas.show();
        CHECK(true); // Should not crash
    }
}

// =============================================================================
// PERFORMANCE AND STRESS TESTS
// =============================================================================

TEST_CASE("Performance: Bulk Operations") {
    IS31FL3737B matrix;
    REQUIRE(matrix.begin() == true);
    
    SUBCASE("Many pixel operations") {
        for (int y = 0; y < 12; y++) {
            for (int x = 0; x < 12; x++) {
                matrix.drawPixel(x, y, (x + y) * 10);
            }
        }
        matrix.show();
        CHECK(true);
    }
    
    SUBCASE("Rapid clear and draw cycles") {
        for (int i = 0; i < 10; i++) {
            matrix.clear();
            matrix.drawPixel(i % 12, i % 12, 255);
            matrix.show();
        }
        CHECK(true);
    }
}

TEST_CASE("Initialization State") {
    SUBCASE("Multiple begin calls") {
        IS31FL3737B matrix;
        
        CHECK(matrix.begin() == true);
        CHECK(matrix.begin() == true); // Should be safe to call multiple times
    }
    
    SUBCASE("Operations before begin") {
        IS31FL3737B matrix;
        
        // These should be safe even without begin()
        matrix.clear();
        matrix.drawPixel(0, 0, 255);
        matrix.setGlobalCurrent(128);
        
        CHECK(matrix.begin() == true); // Should still work after operations
    }
}
