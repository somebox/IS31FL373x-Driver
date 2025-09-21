/**
 * @file LED_Sign_Multi_Chip.cpp
 * @brief LED Sign Example - DisplayManager Pattern
 * 
 * This example demonstrates a scrolling LED sign using multiple IS31FL373x chips
 * arranged horizontally. Based on the DisplayManager pattern from the RetroText project.
 * 
 * Features demonstrated:
 * - Multi-chip canvas management
 * - ADDR pin constants for clean addressing
 * - Multi-level brightness control with light sensor
 * - Automatic gamma correction and global dimming
 * - Performance monitoring with FPS calculation
 * - Hardware compatibility for IS31FL3737 chips
 * 
 * Hardware Setup:
 * - 3x IS31FL3737 chips using IS31FL3733 driver
 * - Different ADDR pin configurations for unique addresses
 * - Optional light sensor on analog pin A0
 * - Arranged horizontally for 36x12 logical display (48x6 characters)
 */

#include "IS31FL373x.h"

// Three IS31FL3737 hardware chips with proper driver classes
IS31FL3737 board1(ADDR::GND);  // Address: 0x50
IS31FL3737 board2(ADDR::VCC);  // Address: 0x51  
IS31FL3737 board3(ADDR::SDA);  // Address: 0x52

// Create device array and canvas
IS31FL373x_Device* devices[] = {&board1, &board2, &board3};
IS31FL373x_Canvas sign(36, 12, devices, 3, LAYOUT_HORIZONTAL);

// Configuration
const int LIGHT_SENSOR_PIN = A0;
const char* MESSAGE = "HELLO WORLD! This is a scrolling LED sign demonstration.";
int scrollPosition = 36;  // Start message off-screen to the right

void setup() {
    Serial.begin(115200);
    Serial.println("LED Sign Multi-Chip Example");
    Serial.println("============================");
    
    // Initialize the canvas (begins all chips)
    if (!sign.begin()) {
        Serial.println("Failed to initialize LED sign!");
        Serial.println("Check I2C connections and addresses!");
        while (1) {
            delay(1000);
        }
    }
    
    Serial.println("LED sign initialized successfully!");
    
    // No coordinate offset needed - using proper IS31FL3737 driver class
    Serial.println("Using native IS31FL3737 driver - no coordinate offset required");
    
    // Set up brightness control
    sign.setGlobalCurrent(100);        // Hardware current limit (0-255)
    sign.setGammaCorrection(true);     // Enable gamma correction for smooth transitions
    
    // Configure text display
    sign.setTextWrap(false);           // Allow text to scroll off edges
    sign.setTextColor(255);            // White text
    sign.setTextSize(1);               // Standard size
    
    Serial.println("Configuration complete!");
    Serial.println("Starting scrolling text demonstration...");
    Serial.println();
}

void loop() {
    // Automatic brightness adaptation based on light sensor
    adaptBrightness();
    
    // Clear the display
    sign.clear();
    
    // Position and draw the scrolling text
    sign.setCursor(scrollPosition, 2);  // Center vertically
    sign.print(MESSAGE);
    
    // Update the display
    sign.show();
    
    // Move text position for scrolling effect
    scrollPosition--;
    
    // Reset position when message scrolls completely off screen
    int messageWidth = strlen(MESSAGE) * 6;  // Approximate character width
    if (scrollPosition < -messageWidth) {
        scrollPosition = 36;  // Reset to right edge
    }
    
    // Performance monitoring
    static unsigned long lastReport = 0;
    if (millis() - lastReport > 5000) {  // Report every 5 seconds
        Serial.print("Sign FPS: ");
        Serial.print(sign.getFPS());
        Serial.print(" | Global Dimming: ");
        Serial.print(getCurrentDimming());
        Serial.println("%");
        lastReport = millis();
    }
    
    delay(50);  // Control scroll speed
}

/**
 * Adapt brightness based on ambient light sensor
 * Demonstrates automatic brightness scaling without user code changes
 */
void adaptBrightness() {
    static unsigned long lastUpdate = 0;
    
    // Update brightness every 100ms
    if (millis() - lastUpdate > 100) {
        int lightLevel = analogRead(LIGHT_SENSOR_PIN);
        
        // Map sensor reading to brightness level
        // Bright environment: high brightness
        // Dark environment: lower brightness to avoid eye strain
        uint8_t dimming = map(lightLevel, 0, 1023, 50, 255);
        dimming = constrain(dimming, 50, 255);  // Ensure minimum visibility
        
        // Apply global dimming - all pixel values automatically scaled
        sign.setGlobalDimming(dimming);
        
        lastUpdate = millis();
    }
}

/**
 * Get current dimming level as percentage for monitoring
 */
int getCurrentDimming() {
    // This would need to be exposed by the driver
    // For now, estimate based on light sensor
    int lightLevel = analogRead(LIGHT_SENSOR_PIN);
    return map(lightLevel, 0, 1023, 20, 100);  // 20-100% brightness range
}

/**
 * USAGE NOTES:
 * 
 * 1. ADDRESSING:
 *    - Uses ADDR constants for clear, readable addressing
 *    - Each chip must have unique address combination
 *    - Check datasheet for your specific ADDR pin configuration
 * 
 * 2. BRIGHTNESS CONTROL:
 *    - Hardware current (setGlobalCurrent): Power/heat management
 *    - Gamma correction: Perceptually linear brightness
 *    - Global dimming: Application-level brightness scaling
 *    - All scaling happens automatically - use 0-255 values normally
 * 
 * 3. PERFORMANCE:
 *    - Monitor FPS to optimize refresh rate
 *    - Typical performance: 30-60 FPS for 3-chip setup
 *    - Reduce delay() in loop() for faster scrolling
 * 
 * 4. CUSTOMIZATION:
 *    - Change MESSAGE for different text
 *    - Adjust scrollPosition increment for scroll speed
 *    - Modify brightness mapping for different light sensors
 *    - Add more visual effects using Adafruit_GFX functions
 */
