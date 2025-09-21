/**
 * @file IS31FL3737_Usage_Example.cpp
 * @brief Example showing how to use IS31FL3737 hardware with the IS31FL373x driver
 * 
 * This example demonstrates the correct setup for IS31FL3737 hardware that uses
 * CS0 and SW0-10 pin labeling. The key is using setCoordinateOffset() to map
 * the 0-based hardware labels to the chip's internal 1-based register addressing.
 * 
 * Hardware Setup:
 * - IS31FL3737 chip (not IS31FL3737B)
 * - LEDs connected to CS0 and SW0-10
 * - I2C address determined by ADDR pin (default: 0x50 when ADDR=GND)
 */

#include "IS31FL373x.h"

// Use IS31FL3733 driver class for IS31FL3737 hardware compatibility
IS31FL3733 ledMatrix(0, 0);  // ADDR1=GND, ADDR2=GND -> I2C address 0x50

void setup() {
    Serial.begin(115200);
    Serial.println("IS31FL3737 Hardware Example");
    
    // IMPORTANT: Call begin() in setup(), not during static initialization
    // This ensures Wire is properly initialized before creating I2C devices
    if (!ledMatrix.begin()) {
        Serial.println("Failed to initialize LED matrix!");
        while (1);
    }
    
    // CRITICAL: Set coordinate offset for IS31FL3737 hardware compatibility
    // This maps user coordinates (0-based) to hardware registers (1-based)
    // CS offset = 0: CS0 hardware -> CS1 register
    // SW offset = 0: SW0 hardware -> SW1 register
    ledMatrix.setCoordinateOffset(0, 0);
    
    // Configure brightness
    ledMatrix.setGlobalCurrent(128);      // Hardware current control (0-255)
    ledMatrix.setMasterBrightness(255);   // Software brightness scaling (0-255)
    
    Serial.println("LED matrix initialized successfully!");
    Serial.println("Testing SW0-SW10 on CS0...");
}

void loop() {
    // Test each SW pin from SW0 to SW10 on CS0
    // This should now work correctly with the coordinate offset
    
    ledMatrix.clear();
    
    // Light up each SW pin sequentially
    for (int sw = 0; sw <= 10; sw++) {
        Serial.print("Lighting SW");
        Serial.print(sw);
        Serial.println(" on CS0");
        
        // drawPixel(x, y, brightness) where:
        // x = CS pin (0 = CS0)
        // y = SW pin (0-10 = SW0-SW10)
        // brightness = PWM value (0-255)
        ledMatrix.drawPixel(0, sw, 255);
        ledMatrix.show();  // Push changes to hardware
        
        delay(500);
        
        // Turn off this LED
        ledMatrix.drawPixel(0, sw, 0);
        ledMatrix.show();
        
        delay(100);
    }
    
    Serial.println("Test complete. SW6 should have worked correctly!");
    Serial.println("Repeating test...\n");
    
    delay(1000);
}

/**
 * TROUBLESHOOTING NOTES:
 * 
 * If SW6 still doesn't work but SW8 affects SW6:
 * 1. Double-check hardware connections
 * 2. Try different coordinate offsets:
 *    - setCoordinateOffset(1, 0) if CS pins are labeled CS1-CS16
 *    - setCoordinateOffset(0, 1) if SW pins are labeled SW1-SW12
 *    - setCoordinateOffset(2, 0) for some hardware variants
 * 
 * The coordinate offset compensates for differences between:
 * - Hardware pin labeling (often 0-based: CS0, SW0)
 * - Chip register addressing (always 1-based: CS1-CS16, SW1-SW12)
 * 
 * REGISTER MAPPING EXPLANATION:
 * Without offset: drawPixel(0, 6) -> CS1, SW7 -> Register address 96
 * With offset(2,0): drawPixel(0, 6) -> CS3, SW7 -> Register address 98
 * 
 * The hardware register formula is: Address = (SWy - 1) * 16 + (CSx - 1)
 * Where CSx and SWy are 1-based chip pin numbers.
 */
