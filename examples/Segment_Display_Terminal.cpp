/**
 * @file Segment_Display_Terminal.cpp
 * @brief 14-Segment Display Example - Terminal Pattern
 * 
 * This example demonstrates a large 14-segment display terminal using 16 IS31FL373x chips
 * arranged in a 4x4 grid. Based on the 14-segment terminal project pattern.
 * 
 * Features demonstrated:
 * - Multiple driver management (16 chips)
 * - All possible ADDR pin combinations
 * - Custom coordinate mapping for segment displays
 * - Time-based brightness control
 * - Segment pattern manipulation
 * - Character display with 14-segment encoding
 * 
 * Hardware Setup:
 * - 16x IS31FL3733 chips with all ADDR combinations (0x50-0x5F)
 * - Each chip drives one 4x3 character module
 * - Total display: 32x6 characters (192 14-segment digits)
 * - Arranged in 4x4 module grid
 */

#include "IS31FL373x.h"

// All 16 possible ADDR pin combinations for maximum addressing
IS31FL3733 drivers[16] = {
    IS31FL3733(ADDR::GND, ADDR::GND), IS31FL3733(ADDR::GND, ADDR::VCC),  // 0x50, 0x54
    IS31FL3733(ADDR::GND, ADDR::SDA), IS31FL3733(ADDR::GND, ADDR::SCL),  // 0x52, 0x53
    IS31FL3733(ADDR::VCC, ADDR::GND), IS31FL3733(ADDR::VCC, ADDR::VCC),  // 0x51, 0x55
    IS31FL3733(ADDR::VCC, ADDR::SDA), IS31FL3733(ADDR::VCC, ADDR::SCL),  // 0x56, 0x57
    IS31FL3733(ADDR::SDA, ADDR::GND), IS31FL3733(ADDR::SDA, ADDR::VCC),  // 0x58, 0x5C
    IS31FL3733(ADDR::SDA, ADDR::SDA), IS31FL3733(ADDR::SDA, ADDR::SCL),  // 0x5A, 0x5B
    IS31FL3733(ADDR::SCL, ADDR::GND), IS31FL3733(ADDR::SCL, ADDR::VCC),  // 0x59, 0x5D
    IS31FL3733(ADDR::SCL, ADDR::SDA), IS31FL3733(ADDR::SCL, ADDR::SCL)   // 0x5E, 0x5F
};

// Display configuration
const uint8_t SCREEN_WIDTH = 32;   // Characters wide
const uint8_t SCREEN_HEIGHT = 6;   // Characters tall
const uint8_t NUM_BOARDS = 16;     // Total driver chips
const uint8_t MODULE_WIDTH = 4;    // Characters per module width
const uint8_t MODULE_HEIGHT = 3;   // Characters per module height

// Sample 14-segment font (simplified for demonstration)
// Each character is represented as a 16-bit pattern
const uint16_t SEGMENT_FONT[] = {
    0x0000, // Space
    0x0006, // !
    0x0202, // "
    0x12CE, // #
    0x12ED, // $
    0x3FE4, // %
    0x2359, // &
    0x0200, // '
    0x2400, // (
    0x0900, // )
    0x3FC0, // *
    0x12C0, // +
    0x0800, // ,
    0x00C0, // -
    0x8000, // .
    0x0C00, // /
    0x0C3F, // 0
    0x0406, // 1
    0x00DB, // 2
    0x008F, // 3
    0x00E6, // 4
    0x2069, // 5
    0x00FD, // 6
    0x0007, // 7
    0x00FF, // 8
    0x00EF, // 9
    0x1200, // :
    0x0A00, // ;
    0x2440, // <
    0x00C8, // =
    0x0980, // >
    0x5083, // ?
    0x02BB, // @
    0x00F7, // A
    0x128F, // B
    0x0039, // C
    0x120F, // D
    0x0079, // E
    0x0071, // F
};

void setup() {
    Serial.begin(115200);
    Serial.println("14-Segment Display Terminal Example");
    Serial.println("===================================");
    
    // Initialize all driver chips
    Serial.println("Initializing 16 LED driver chips...");
    
    for (int i = 0; i < NUM_BOARDS; i++) {
        Serial.print("Initializing driver ");
        Serial.print(i);
        Serial.print(" at address 0x");
        Serial.print(drivers[i].calculateAddress(), HEX);
        Serial.print("... ");
        
        if (!drivers[i].begin()) {
            Serial.println("FAILED!");
            Serial.print("Check I2C connections for driver ");
            Serial.println(i);
            while (1) {
                delay(1000);
            }
        }
        
        Serial.println("OK");
        
        // Configure each driver
        drivers[i].setGlobalCurrent(128);     // Hardware current control
        drivers[i].setGammaCorrection(true);  // Enable perceptual linearity
    }
    
    Serial.println("All drivers initialized successfully!");
    
    // Set up time-based brightness control
    setupTimeBrightness();
    
    Serial.println("Starting terminal demonstration...");
    Serial.println();
    
    // Display startup message
    displayMessage("TERMINAL READY - 14 SEGMENT DISPLAY SYSTEM ONLINE");
    delay(3000);
}

void loop() {
    // Update brightness based on time of day
    updateTimeBrightness();
    
    // Demonstrate different display modes
    static int mode = 0;
    static unsigned long lastModeChange = 0;
    
    if (millis() - lastModeChange > 10000) {  // Change mode every 10 seconds
        mode = (mode + 1) % 4;
        lastModeChange = millis();
        clearDisplay();
    }
    
    switch (mode) {
        case 0:
            demoScrollingText();
            break;
        case 1:
            demoCharacterSet();
            break;
        case 2:
            demoPatternDisplay();
            break;
        case 3:
            demoAddressTest();
            break;
    }
    
    // Performance monitoring
    static unsigned long lastReport = 0;
    if (millis() - lastReport > 5000) {
        Serial.print("Average FPS: ");
        float totalFPS = 0;
        for (int i = 0; i < NUM_BOARDS; i++) {
            totalFPS += drivers[i].getFPS();
        }
        Serial.println(totalFPS / NUM_BOARDS);
        lastReport = millis();
    }
    
    delay(50);
}

/**
 * Draw a segment pattern on a specific board and position
 */
void drawSegmentPattern(uint8_t board, uint8_t pos, uint16_t pattern, uint8_t level) {
    if (board >= NUM_BOARDS || pos >= 12) return;
    
    // Each position uses 16 bits (segments) in the buffer
    for (int bit = 0; bit < 16; bit++) {
        uint8_t value = (pattern & (1 << bit)) ? level : 0;
        uint16_t index = pos * 16 + bit;
        drivers[board].setPixel(index, value);  // Automatic brightness scaling applied
    }
}

/**
 * Draw a character at screen coordinates (row, col)
 */
void drawCharacter(char c, uint8_t row, uint8_t col, uint8_t level = 255) {
    if (row >= SCREEN_HEIGHT || col >= SCREEN_WIDTH) return;
    
    // Calculate which board and position within that board
    uint8_t board = (col / MODULE_WIDTH) + (row / MODULE_HEIGHT) * 4;
    uint8_t pos = (col % MODULE_WIDTH) + (row % MODULE_HEIGHT) * MODULE_WIDTH;
    
    // Get character pattern
    uint16_t pattern = 0;
    if (c >= 32 && c < 32 + (sizeof(SEGMENT_FONT) / sizeof(SEGMENT_FONT[0]))) {
        pattern = SEGMENT_FONT[c - 32];
    }
    
    drawSegmentPattern(board, pos, pattern, level);
}

/**
 * Display a message across the entire screen
 */
void displayMessage(const char* message) {
    clearDisplay();
    
    int len = strlen(message);
    int row = 0, col = 0;
    
    for (int i = 0; i < len && row < SCREEN_HEIGHT; i++) {
        if (message[i] == '\n' || col >= SCREEN_WIDTH) {
            row++;
            col = 0;
            if (message[i] == '\n') continue;
        }
        
        drawCharacter(message[i], row, col, 255);
        col++;
    }
    
    updateAllDisplays();
}

/**
 * Clear entire display
 */
void clearDisplay() {
    for (int i = 0; i < NUM_BOARDS; i++) {
        drivers[i].clear();
    }
    updateAllDisplays();
}

/**
 * Update all displays
 */
void updateAllDisplays() {
    for (int i = 0; i < NUM_BOARDS; i++) {
        drivers[i].show();
    }
}

/**
 * Set up time-based brightness control
 */
void setupTimeBrightness() {
    // Example: Bright during day (8 AM - 6 PM), dim at night
    uint8_t hour = 14;  // Simulate 2 PM
    uint8_t dimming = (hour >= 8 && hour <= 18) ? 255 : 100;
    
    for (int i = 0; i < NUM_BOARDS; i++) {
        drivers[i].setGlobalDimming(dimming);
    }
    
    Serial.print("Time-based brightness set to ");
    Serial.print((dimming * 100) / 255);
    Serial.println("%");
}

/**
 * Update brightness based on simulated time
 */
void updateTimeBrightness() {
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate < 30000) return;  // Update every 30 seconds
    
    // Simulate time progression
    static uint8_t simulatedHour = 8;
    simulatedHour = (simulatedHour + 1) % 24;
    
    uint8_t dimming = (simulatedHour >= 8 && simulatedHour <= 18) ? 255 : 80;
    
    for (int i = 0; i < NUM_BOARDS; i++) {
        drivers[i].setGlobalDimming(dimming);
    }
    
    lastUpdate = millis();
}

/**
 * Demo: Scrolling text
 */
void demoScrollingText() {
    static int scrollPos = SCREEN_WIDTH;
    static unsigned long lastScroll = 0;
    
    if (millis() - lastScroll > 200) {
        clearDisplay();
        
        const char* text = "SCROLLING TEXT DEMONSTRATION";
        int textLen = strlen(text);
        
        for (int i = 0; i < textLen; i++) {
            int pos = scrollPos + i;
            if (pos >= 0 && pos < SCREEN_WIDTH) {
                drawCharacter(text[i], 2, pos);  // Center row
            }
        }
        
        updateAllDisplays();
        
        scrollPos--;
        if (scrollPos < -textLen) {
            scrollPos = SCREEN_WIDTH;
        }
        
        lastScroll = millis();
    }
}

/**
 * Demo: Display character set
 */
void demoCharacterSet() {
    static int startChar = 32;
    static unsigned long lastUpdate = 0;
    
    if (millis() - lastUpdate > 1000) {
        clearDisplay();
        
        int charCount = 0;
        for (int row = 0; row < SCREEN_HEIGHT && charCount < 64; row++) {
            for (int col = 0; col < SCREEN_WIDTH && charCount < 64; col++) {
                char c = startChar + charCount;
                if (c > 126) c = 32;  // Wrap to space
                drawCharacter(c, row, col);
                charCount++;
            }
        }
        
        updateAllDisplays();
        startChar = (startChar + 1) % 95 + 32;  // Cycle through printable chars
        lastUpdate = millis();
    }
}

/**
 * Demo: Pattern display
 */
void demoPatternDisplay() {
    static unsigned long lastUpdate = 0;
    static uint16_t pattern = 0x0001;
    
    if (millis() - lastUpdate > 500) {
        clearDisplay();
        
        // Display rotating pattern on all positions
        for (int row = 0; row < SCREEN_HEIGHT; row++) {
            for (int col = 0; col < SCREEN_WIDTH; col++) {
                uint8_t board = (col / MODULE_WIDTH) + (row / MODULE_HEIGHT) * 4;
                uint8_t pos = (col % MODULE_WIDTH) + (row % MODULE_HEIGHT) * MODULE_WIDTH;
                drawSegmentPattern(board, pos, pattern, 128);
            }
        }
        
        updateAllDisplays();
        
        // Rotate pattern
        pattern = (pattern << 1) | (pattern >> 15);
        lastUpdate = millis();
    }
}

/**
 * Demo: Address test - light up each board sequentially
 */
void demoAddressTest() {
    static int currentBoard = 0;
    static unsigned long lastUpdate = 0;
    
    if (millis() - lastUpdate > 500) {
        clearDisplay();
        
        // Light up all positions on current board
        for (int pos = 0; pos < 12; pos++) {
            drawSegmentPattern(currentBoard, pos, 0xFFFF, 100);
        }
        
        drivers[currentBoard].show();
        
        Serial.print("Testing board ");
        Serial.print(currentBoard);
        Serial.print(" (0x");
        Serial.print(drivers[currentBoard].calculateAddress(), HEX);
        Serial.println(")");
        
        currentBoard = (currentBoard + 1) % NUM_BOARDS;
        lastUpdate = millis();
    }
}

/**
 * USAGE NOTES:
 * 
 * 1. ADDRESSING:
 *    - Uses all 16 possible ADDR combinations
 *    - Each combination creates unique I2C address (0x50-0x5F)
 *    - Verify your hardware matches these combinations
 * 
 * 2. COORDINATE MAPPING:
 *    - Screen coordinates (row, col) map to specific board and position
 *    - Each board handles 4x3 character module
 *    - Total display: 32 characters wide, 6 characters tall
 * 
 * 3. SEGMENT PATTERNS:
 *    - 16-bit patterns define which segments are lit
 *    - Customize SEGMENT_FONT array for different character sets
 *    - Pattern 0xFFFF lights all segments
 * 
 * 4. PERFORMANCE:
 *    - Monitor FPS across all 16 drivers
 *    - Optimize update frequency for smooth animation
 *    - Use bulk operations when possible
 */
