/*
 * Hexagonal Bubble Menu Demo
 * ESP32-S3 Touch LCD 1.47" (172x320)
 *
 * Apple Watch-style honeycomb launcher:
 *   - 30 colored circles in hexagonal layout
 *   - 2D panning with inertia
 *   - Three-zone rounded-rectangle focus model
 *   - Tap to expand, swipe to close
 *
 * Hardware: AXS5106L touch over I2C, ST7789/JD9853 display over SPI
 */

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include "esp_lcd_touch_axs5106l.h"
#include "config.h"
#include "display.h"
#include "touch.h"
#include "menu.h"
#include "renderer.h"

// ============================================================================
//  Hardware globals
// ============================================================================

static Arduino_DataBus *bus = createDisplayBus();
static Arduino_GFX    *gfx = createDisplay(bus);
extern MenuRuntime rt;

// ============================================================================
//  Touch polling – converts raw touch events into menu callbacks
// ============================================================================

static bool wasDown = false;

static void pollTouch()
{
    int x, y;
    bool down = touchRead(x, y);
    uint32_t now = millis();

    if (down && !wasDown)       menuOnTouchDown(x, y, now);
    else if (down && wasDown)   menuOnTouchMove(x, y, now);
    else if (!down && wasDown)  menuOnTouchUp(now);

    wasDown = down;
}

// ============================================================================
//  Setup
// ============================================================================

void setup()
{
    Serial.begin(115200);
    Serial.println("Hexagonal Bubble Menu Demo - ESP32-S3 Touch LCD 1.47");

    // Hardware reset
    pinMode(PIN_LCD_HW_RST, OUTPUT);
    digitalWrite(PIN_LCD_HW_RST, 0);
    delay(10);
    digitalWrite(PIN_LCD_HW_RST, 1);

    // Display init
    gfx->begin();
    lcdRegisterInit(bus);
    gfx->setRotation(ROTATION);
    gfx->fillScreen(RGB565_BLACK);
    pinMode(PIN_LCD_BL, OUTPUT);
    digitalWrite(PIN_LCD_BL, HIGH);

    // Touch, canvas, menu
    touchInit();
    rendererInit(gfx);
    menuInit();

    // Compute radius needed to cover the entire screen corner-to-corner
    rt.transitionFullRadius = sqrtf(SCREEN_CENTER_X * SCREEN_CENTER_X
                                  + SCREEN_CENTER_Y * SCREEN_CENTER_Y) + 2.0f;
}

// ============================================================================
//  Loop
// ============================================================================

void loop()
{
    pollTouch();
    menuUpdate();
    if (menuNeedsRedraw())
        rendererFrame();
}
