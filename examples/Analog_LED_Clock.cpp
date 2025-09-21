/**
 * @file Analog_LED_Clock.cpp
 * @brief Analog LED Clock Example - Hardware Compatibility Pattern
 * 
 * This example demonstrates an analog-style LED clock using a single IS31FL373x chip
 * with IS31FL3737 hardware compatibility. Based on the Orbitchron clock project pattern.
 * 
 * Features demonstrated:
 * - Single chip operation with hardware compatibility
 * - Direct coordinate access with automatic offset handling
 * - Power management for battery operation
 * - Smooth animations with gamma correction
 * - Performance monitoring
 * - Time-based display with clock hands
 * - Easing functions for smooth transitions
 * 
 * Hardware Setup:
 * - 1x IS31FL3737 chip using IS31FL3733 driver
 * - 12x12 LED matrix arranged as analog clock face
 * - ADDR pin connected to GND (address 0x50)
 * - Optional RTC module for accurate timekeeping
 */

#include "IS31FL373x.h"
#include <math.h>

// Single IS31FL3737 chip using IS31FL3733 driver for compatibility
IS31FL3733 matrix(ADDR::GND, ADDR::GND);  // Address: 0x50

// Clock configuration
const uint8_t MATRIX_SIZE = 12;           // 12x12 LED matrix
const uint8_t CENTER_X = 6;               // Clock center X
const uint8_t CENTER_Y = 6;               // Clock center Y

// Brightness levels
const uint8_t CLOCK_MIN_LEVEL = 4;
const uint8_t CLOCK_DIM_LEVEL = 30;
const uint8_t CLOCK_MID_LEVEL = 100;
const uint8_t CLOCK_BRIGHT_LEVEL = 200;
const uint8_t CLOCK_MAX_LEVEL = 255;

// Clock hands and positions
struct ClockPosition {
    uint8_t x, y;
};

// Hour markers (12 positions around the clock face)
const ClockPosition HOUR_MARKERS[12] = {
    {6, 1},   // 12 o'clock
    {9, 2},   // 1 o'clock
    {10, 4},  // 2 o'clock
    {10, 6},  // 3 o'clock
    {10, 8},  // 4 o'clock
    {9, 10},  // 5 o'clock
    {6, 11},  // 6 o'clock
    {3, 10},  // 7 o'clock
    {2, 8},   // 8 o'clock
    {2, 6},   // 9 o'clock
    {2, 4},   // 10 o'clock
    {3, 2}    // 11 o'clock
};

// Simulated time (replace with RTC in real implementation)
struct {
    uint8_t hours = 10;
    uint8_t minutes = 30;
    uint8_t seconds = 0;
} currentTime;

void setup() {
    Serial.begin(115200);
    Serial.println("Analog LED Clock Example");
    Serial.println("========================");
    
    // Initialize the LED matrix
    if (!matrix.begin()) {
        Serial.println("Failed to initialize LED matrix!");
        Serial.println("Check I2C connections and address!");
        while (1) {
            delay(1000);
        }
    }
    
    Serial.println("LED matrix initialized successfully!");
    
    // CRITICAL: Set coordinate offset for IS31FL3737 hardware compatibility
    matrix.setCoordinateOffset(2, 0);  // CS adjustment for IS31FL3737
    
    // Configure brightness and power management
    matrix.setGlobalCurrent(240);      // Hardware brightness (high for visibility)
    matrix.setGammaCorrection(true);   // Smooth LED transitions
    matrix.setGlobalDimming(180);      // Power management (70% of max)
    
    Serial.println("Hardware compatibility configured");
    Serial.println("Power management: 70% brightness limit");
    Serial.println("Starting clock display...");
    Serial.println();
    
    // Initial display
    displayClock();
}

void loop() {
    // Update simulated time (replace with RTC reading)
    updateSimulatedTime();
    
    // Clear the display
    matrix.clear();
    
    // Draw the clock
    drawClockFace();
    drawClockHands();
    
    // Update the display
    matrix.show();
    
    // Performance monitoring
    static unsigned long lastReport = 0;
    if (millis() - lastReport > 5000) {  // Report every 5 seconds
        Serial.print("Clock FPS: ");
        Serial.print(matrix.getFPS());
        Serial.print(" | Time: ");
        Serial.print(currentTime.hours);
        Serial.print(":");
        if (currentTime.minutes < 10) Serial.print("0");
        Serial.print(currentTime.minutes);
        Serial.print(":");
        if (currentTime.seconds < 10) Serial.print("0");
        Serial.println(currentTime.seconds);
        lastReport = millis();
    }
    
    delay(100);  // 10 FPS for smooth second hand movement
}

/**
 * Set a pixel using hardware-compatible coordinates
 * Handles coordinate offset automatically
 */
void setClockPixel(int x, int y, uint8_t brightness) {
    if (x >= 0 && x < MATRIX_SIZE && y >= 0 && y < MATRIX_SIZE) {
        // Use drawPixel for automatic coordinate transformation
        matrix.drawPixel(x, y, brightness);
    }
}

/**
 * Draw the clock face with hour markers
 */
void drawClockFace() {
    // Draw hour markers
    for (int i = 0; i < 12; i++) {
        ClockPosition pos = HOUR_MARKERS[i];
        uint8_t brightness = CLOCK_DIM_LEVEL;
        
        // Highlight current hour
        if (i == (currentTime.hours % 12)) {
            brightness = CLOCK_BRIGHT_LEVEL;
        }
        
        setClockPixel(pos.x, pos.y, brightness);
    }
    
    // Draw center dot
    setClockPixel(CENTER_X, CENTER_Y, CLOCK_MID_LEVEL);
}

/**
 * Draw clock hands
 */
void drawClockHands() {
    drawHourHand();
    drawMinuteHand();
    drawSecondHand();
}

/**
 * Draw hour hand
 */
void drawHourHand() {
    // Calculate hour hand angle (30 degrees per hour + minute offset)
    float hourAngle = (currentTime.hours % 12) * 30.0 + (currentTime.minutes * 0.5);
    hourAngle = hourAngle * PI / 180.0 - PI/2;  // Convert to radians, 0 = top
    
    // Draw short, thick hour hand
    for (int i = 1; i <= 3; i++) {
        int x = CENTER_X + (int)(cos(hourAngle) * i);
        int y = CENTER_Y + (int)(sin(hourAngle) * i);
        setClockPixel(x, y, CLOCK_BRIGHT_LEVEL);
    }
}

/**
 * Draw minute hand
 */
void drawMinuteHand() {
    // Calculate minute hand angle (6 degrees per minute)
    float minuteAngle = currentTime.minutes * 6.0;
    minuteAngle = minuteAngle * PI / 180.0 - PI/2;  // Convert to radians, 0 = top
    
    // Draw longer minute hand
    for (int i = 1; i <= 4; i++) {
        int x = CENTER_X + (int)(cos(minuteAngle) * i);
        int y = CENTER_Y + (int)(sin(minuteAngle) * i);
        setClockPixel(x, y, CLOCK_MID_LEVEL);
    }
}

/**
 * Draw second hand with smooth animation
 */
void drawSecondHand() {
    // Calculate second hand angle with smooth sub-second movement
    float secondsFloat = currentTime.seconds + (millis() % 1000) / 1000.0;
    float secondAngle = secondsFloat * 6.0;
    secondAngle = secondAngle * PI / 180.0 - PI/2;  // Convert to radians, 0 = top
    
    // Draw thin, long second hand
    for (int i = 1; i <= 5; i++) {
        int x = CENTER_X + (int)(cos(secondAngle) * i);
        int y = CENTER_Y + (int)(sin(secondAngle) * i);
        
        // Fade brightness along the hand
        uint8_t brightness = map(i, 1, 5, CLOCK_BRIGHT_LEVEL, CLOCK_DIM_LEVEL);
        setClockPixel(x, y, brightness);
    }
}

/**
 * Update simulated time (replace with RTC in real implementation)
 */
void updateSimulatedTime() {
    static unsigned long lastSecond = 0;
    
    if (millis() - lastSecond >= 1000) {
        currentTime.seconds++;
        
        if (currentTime.seconds >= 60) {
            currentTime.seconds = 0;
            currentTime.minutes++;
            
            if (currentTime.minutes >= 60) {
                currentTime.minutes = 0;
                currentTime.hours++;
                
                if (currentTime.hours >= 24) {
                    currentTime.hours = 0;
                }
            }
        }
        
        lastSecond = millis();
    }
}

/**
 * Initial clock display with startup animation
 */
void displayClock() {
    Serial.println("Displaying startup animation...");
    
    // Animate hour markers appearing
    for (int i = 0; i < 12; i++) {
        matrix.clear();
        
        // Draw markers up to current position
        for (int j = 0; j <= i; j++) {
            ClockPosition pos = HOUR_MARKERS[j];
            setClockPixel(pos.x, pos.y, CLOCK_BRIGHT_LEVEL);
        }
        
        matrix.show();
        delay(200);
    }
    
    // Brief center dot flash
    for (int i = 0; i < 3; i++) {
        setClockPixel(CENTER_X, CENTER_Y, CLOCK_MAX_LEVEL);
        matrix.show();
        delay(200);
        
        setClockPixel(CENTER_X, CENTER_Y, 0);
        matrix.show();
        delay(200);
    }
    
    Serial.println("Startup animation complete");
}

/**
 * Easing function for smooth animations
 */
float easeInOutSine(float t) {
    return -0.5 * (cos(PI * t) - 1);
}

/**
 * Map function with easing for smooth transitions
 */
float mapWithEasing(float value, float in_min, float in_max, float out_min, float out_max) {
    if (value <= in_min) return out_min;
    if (value >= in_max) return out_max;
    
    float t = (value - in_min) / (in_max - in_min);
    float eased_t = easeInOutSine(t);
    return out_min + (out_max - out_min) * eased_t;
}

/**
 * USAGE NOTES:
 * 
 * 1. HARDWARE COMPATIBILITY:
 *    - setCoordinateOffset(2, 0) handles IS31FL3737 CS pin mapping
 *    - Adjust offset if your hardware uses different pin labeling
 *    - Test with simple patterns first to verify coordinate mapping
 * 
 * 2. COORDINATE SYSTEM:
 *    - Uses standard Cartesian coordinates (0-11, 0-11)
 *    - Automatic transformation to hardware registers
 *    - Center of clock at (6, 6) for 12x12 matrix
 * 
 * 3. BRIGHTNESS CONTROL:
 *    - Global current: Hardware power management
 *    - Gamma correction: Smooth visual transitions
 *    - Global dimming: Battery life optimization
 *    - All scaling automatic - use 0-255 values normally
 * 
 * 4. PERFORMANCE:
 *    - 10 FPS provides smooth second hand movement
 *    - Monitor FPS to ensure consistent timing
 *    - Reduce brightness for longer battery life
 * 
 * 5. CUSTOMIZATION:
 *    - Replace updateSimulatedTime() with RTC integration
 *    - Adjust HOUR_MARKERS for different clock layouts
 *    - Modify hand lengths and brightness levels
 *    - Add additional features like alarms or date display
 * 
 * 6. TROUBLESHOOTING:
 *    - If wrong LEDs light up, adjust coordinate offset
 *    - Check I2C address matches your ADDR pin configuration
 *    - Verify power supply can handle LED current draw
 */
