/**
 * @file main.cpp
 * @brief Basic usage example for IS31FL373x LED matrix driver
 * 
 * This example demonstrates the fundamental usage of the IS31FL373x library
 * for controlling LED matrix displays. It shows basic initialization,
 * drawing operations, and multi-chip canvas usage.
 */

#include <Arduino.h>
#include <Wire.h>
#include "IS31FL373x.h"

// Single matrix example
IS31FL3737B matrix1;

// Multi-chip example (uncomment to test)
/*
IS31FL3737B matrix1(0);
IS31FL3737B matrix2(1);
IS31FL3737B matrix3(2);

IS31FL373x_Device* devices[] = {&matrix1, &matrix2, &matrix3};
IS31FL373x_Canvas canvas(36, 12, devices, 3, LAYOUT_HORIZONTAL);
*/

void setup() {
    Serial.begin(115200);
    Serial.println("IS31FL373x Basic Example");
    
    // Initialize I2C
    Wire.begin();
    
    // Initialize single matrix
    if (matrix1.begin()) {
        Serial.println("✅ Matrix initialized successfully");
        matrix1.setGlobalCurrent(50); // Set to low brightness for safety
    } else {
        Serial.println("❌ Matrix initialization failed");
    }
    
    // Initialize multi-chip canvas (if enabled)
    /*
    if (canvas.begin()) {
        Serial.println("✅ Canvas initialized successfully");
        canvas.setGlobalCurrent(50);
    } else {
        Serial.println("❌ Canvas initialization failed");
    }
    */
}

void loop() {
    static unsigned long lastUpdate = 0;
    static uint8_t frame = 0;
    
    if (millis() - lastUpdate > 500) { // Update every 500ms
        lastUpdate = millis();
        
        // Single matrix demo
        matrix1.clear();
        
        // Draw a simple pattern
        switch (frame % 4) {
            case 0:
                // Draw corners
                matrix1.drawPixel(0, 0, 255);
                matrix1.drawPixel(11, 0, 255);
                matrix1.drawPixel(0, 11, 255);
                matrix1.drawPixel(11, 11, 255);
                break;
                
            case 1:
                // Draw center cross
                for (int i = 0; i < 12; i++) {
                    matrix1.drawPixel(6, i, 128);
                    matrix1.drawPixel(i, 6, 128);
                }
                break;
                
            case 2:
                // Draw border
                for (int i = 0; i < 12; i++) {
                    matrix1.drawPixel(0, i, 64);
                    matrix1.drawPixel(11, i, 64);
                    matrix1.drawPixel(i, 0, 64);
                    matrix1.drawPixel(i, 11, 64);
                }
                break;
                
            case 3:
                // Draw diagonal
                for (int i = 0; i < 12; i++) {
                    matrix1.drawPixel(i, i, 192);
                    matrix1.drawPixel(11-i, i, 192);
                }
                break;
        }
        
        matrix1.show();
        
        // Multi-chip canvas demo (if enabled)
        /*
        canvas.clear();
        canvas.setCursor(frame % 30, 2);
        canvas.print("HELLO");
        canvas.show();
        */
        
        frame++;
        
        Serial.print("Frame: ");
        Serial.println(frame);
    }
}

void demonstrateCustomLayout() {
    // Example of custom layout for non-matrix arrangements
    // This would be called from setup() if you have a custom LED layout
    
    PixelMapEntry clockLayout[12] = {
        {1, 1}, {2, 1}, {3, 1}, {4, 1},    // 12, 1, 2, 3 o'clock positions
        {5, 1}, {6, 1}, {6, 2}, {6, 3},    // 4, 5, 6 o'clock positions  
        {5, 3}, {4, 3}, {3, 3}, {2, 3}     // 7, 8, 9, 10, 11 o'clock positions
    };
    
    matrix1.setLayout(clockLayout, 12);
    
    // Now you can draw using logical indices (0-11 for clock positions)
    matrix1.setPixel(0, 255);  // 12 o'clock
    matrix1.setPixel(3, 255);  // 3 o'clock  
    matrix1.setPixel(6, 255);  // 6 o'clock
    matrix1.setPixel(9, 255);  // 9 o'clock
    matrix1.show();
}

void demonstrateBrightnessControl() {
    // Global current control (hardware level)
    matrix1.setGlobalCurrent(255);  // Maximum brightness
    matrix1.setGlobalCurrent(128);  // 50% brightness
    matrix1.setGlobalCurrent(64);   // 25% brightness
    
    // Software brightness scaling (applied to all drawing operations)
    matrix1.setMasterBrightness(255);  // Full software brightness
    matrix1.setMasterBrightness(128);  // 50% scaling
    
    // These can be combined for fine control
    matrix1.setGlobalCurrent(200);     // 78% hardware brightness
    matrix1.setMasterBrightness(128);  // 50% software scaling
    // Effective brightness = 78% * 50% = 39%
}

void demonstrateGFXIntegration() {
    // The IS31FL373x classes inherit from Adafruit_GFX
    // so you can use any GFX drawing functions
    
    matrix1.clear();
    matrix1.drawLine(0, 0, 11, 11, 255);        // Diagonal line
    matrix1.drawRect(2, 2, 8, 8, 128);          // Rectangle
    matrix1.drawCircle(6, 6, 4, 192);           // Circle
    matrix1.fillRect(4, 4, 4, 4, 64);           // Filled rectangle
    
    // Text drawing (with appropriate font)
    matrix1.setCursor(1, 1);
    matrix1.setTextColor(255);
    matrix1.print("HI");
    
    matrix1.show();
}
