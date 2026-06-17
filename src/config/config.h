#pragma once

#include <Arduino.h>
#include <Arduino_GFX_Library.h>

// ============================================================================
//  Vec2 – 2D vector with basic math helpers
// ============================================================================

struct Vec2 { float x, y; };

static inline Vec2  add(Vec2 a, Vec2 b)       { return {a.x + b.x, a.y + b.y}; }
static inline Vec2  sub(Vec2 a, Vec2 b)       { return {a.x - b.x, a.y - b.y}; }
static inline Vec2  mul(Vec2 v, float s)      { return {v.x * s, v.y * s}; }
static inline Vec2  lerp(Vec2 a, Vec2 b, float t) { return {a.x+(b.x-a.x)*t, a.y+(b.y-a.y)*t}; }
static inline float dot(Vec2 a, Vec2 b)       { return a.x*b.x + a.y*b.y; }
static inline float lengthSq(Vec2 v)          { return dot(v, v); }
static inline float lengthVec(Vec2 v)         { return sqrtf(lengthSq(v)); }

static inline float lerpf(float a, float b, float t)  { return a + (b-a)*t; }
static inline float clampf(float v, float lo, float hi) { return v<lo?lo:(v>hi?hi:v); }
static inline int   clampi(int v, int lo, int hi)       { return v<lo?lo:(v>hi?hi:v); }
static inline float fabsf_f(float v)          { return v<0.0f?-v:v; }

// Smooth Hermite interpolation: 0 at t=0, 1 at t=1, smooth tangent at endpoints
static inline float smoothstep01(float t) { t=clampf(t,0,1); return t*t*(3-2*t); }
// Ease-out cubic: fast start, slow end
static inline float easeOutCubic(float t) { float u=1-t; return 1-u*u*u; }

// ============================================================================
//  Hardware Pins – Waveshare ESP32-S3 Touch LCD 1.47"
// ============================================================================

#define PIN_LCD_CS      21
#define PIN_LCD_DC      45
#define PIN_LCD_RST     47
#define PIN_LCD_SCK     38
#define PIN_LCD_MOSI    39
#define PIN_LCD_BL      46
#define PIN_LCD_HW_RST  40

#define PIN_TOUCH_SDA   42
#define PIN_TOUCH_SCL   41
#define PIN_TOUCH_RST   47
#define PIN_TOUCH_INT   48

// ============================================================================
//  Screen Geometry
// ============================================================================

#define SCREEN_W  172
#define SCREEN_H  320
#define ROTATION  0

static constexpr float SCREEN_CENTER_X = SCREEN_W * 0.5f;
static constexpr float SCREEN_CENTER_Y = SCREEN_H * 0.5f;

// ============================================================================
//  Bubble Radii
//  MIN = size at screen edge    MAX = size at screen center (focused)
// ============================================================================

static constexpr float MIN_BUBBLE_RADIUS = SCREEN_W * 0.080f;   // ~14 px
static constexpr float MAX_BUBBLE_RADIUS = SCREEN_W * 0.115f;   // ~20 px

// ============================================================================
//  Hexagonal Grid Spacing
//  Derived from MAX_BUBBLE_RADIUS so bubbles never overlap.
//  Staggered hex: odd rows shift right by H_SPACING/2.
// ============================================================================

static constexpr float H_SPACING = MAX_BUBBLE_RADIUS * 2.60f;   // ~52 px
static constexpr float V_SPACING = 50.0f;                        // ~50 px

// ============================================================================
//  Three-Zone Focus Model
//
//  A rounded rectangle divides the screen into three regions:
//    Center  – bubble at MAX radius, strongest pull toward center
//    Fringe  – smooth transition between center and outer
//    Outer   – bubble at MIN radius, no center pull
//
//     ┌──────────────────────────────────────────┐
//     │              OUTER (MIN radius)          │
//     │   ┌──────────────────────────────────┐   │
//     │   │          FRINGE (blend)          │   │
//     │   │   ┌──────────────────────────┐   │   │
//     │   │   │     CENTER (MAX radius)  │   │   │
//     │   │   └──────────────────────────┘   │   │
//     │   └──────────────────────────────────┘   │
//     └──────────────────────────────────────────┘
//
// ============================================================================

static constexpr float CENTER_REGION_HALF_W   = SCREEN_W * 0.28f;   // ~48 px
static constexpr float CENTER_REGION_HALF_H   = SCREEN_H * 0.25f;   // ~80 px
static constexpr float CENTER_REGION_CORNER_R = SCREEN_W * 0.10f;   // ~17 px
static constexpr float FRINGE_WIDTH           = SCREEN_W * 0.26f;   // ~45 px

// How much a growing bubble is pulled toward center (0 = none, 1 = full)
static constexpr float COMPACT_PULL_GAIN = 0.90f;

// ============================================================================
//  Menu Items
// ============================================================================

#define ITEM_COUNT 50

struct BubbleItem {
    uint16_t color;       // RGB565 fill
    Vec2 worldPosition;   // set by layout generator
};

static BubbleItem ITEMS[ITEM_COUNT] = {
    {RGB565(230,  25,  25)},   //  0  Red
    {RGB565(230, 120,  25)},   //  1  Orange
    {RGB565(230, 200,  25)},   //  2  Yellow
    {RGB565(160, 230,  25)},   //  3  Lime
    {RGB565( 25, 200,  25)},   //  4  Green
    {RGB565( 25, 190, 130)},   //  5  Teal
    {RGB565( 25, 190, 220)},   //  6  Cyan
    {RGB565( 25, 120, 230)},   //  7  Blue
    {RGB565( 60,  50, 220)},   //  8  Indigo
    {RGB565(130,  40, 210)},   //  9  Purple
    {RGB565(210,  40, 180)},   // 10  Magenta
    {RGB565(230,  50, 100)},   // 11  Rose
    {RGB565(230,  80,  50)},   // 12  Coral
    {RGB565(220, 160,  30)},   // 13  Amber
    {RGB565( 80, 200,  80)},   // 14  Emerald
    {RGB565( 50, 160, 200)},   // 15  Steel
    {RGB565(180, 100, 200)},   // 16  Lavender
    {RGB565(200,  60, 140)},   // 17  Carmine
    {RGB565( 40, 180, 160)},   // 18  Aquamarine
    {RGB565(170, 170, 170)},   // 19  Silver
    {RGB565(120,  25,  25)},   // 20  Maroon
    {RGB565(220, 180,  30)},   // 21  Gold
    {RGB565(100, 110,  30)},   // 22  Olive
    {RGB565( 25,  40, 120)},   // 23  Navy
    {RGB565(150,  50, 120)},   // 24  Plum
    {RGB565(160,  80,  30)},   // 25  Sienna
    {RGB565( 30, 200, 190)},   // 26  Turquoise
    {RGB565(240,  60, 160)},   // 27  Hot Pink
    {RGB565( 30, 110,  50)},   // 28  Forest
    {RGB565(240, 170, 130)},   // 29  Peach
    {RGB565(200,  30,  60)},   // 30  Cranberry
    {RGB565( 40, 140, 100)},   // 31  Jade
    {RGB565(180, 130,  60)},   // 32  Brass
    {RGB565( 70,  70, 180)},   // 33  Periwinkle
    {RGB565(140, 200, 160)},   // 34  Mint
    {RGB565(190,  50,  50)},   // 35  Brick
    {RGB565( 30, 160, 180)},   // 36  Cerulean
    {RGB565(160, 160,  80)},   // 37  Chartreuse
    {RGB565(120,  80, 140)},   // 38  Mauve
    {RGB565(210, 140,  50)},   // 39  Saffron
    {RGB565( 60, 130, 110)},   // 40  Verdigris
    {RGB565(180,  60, 100)},   // 41  Raspberry
    {RGB565( 50,  90, 150)},   // 42  Sapphire
    {RGB565(200, 190, 140)},   // 43  Sand
    {RGB565( 80,  50, 100)},   // 44  Eggplant
    {RGB565(100, 170, 200)},   // 45  Sky
    {RGB565(170,  40,  70)},   // 46  Wine
    {RGB565( 60, 180, 120)},   // 47  Celadon
    {RGB565(220, 100,  80)},   // 48  Salmon
    {RGB565(110, 110, 160)},   // 49  Dusty Blue
};

// ============================================================================
//  Touch Tuning
// ============================================================================

static constexpr float    DRAG_THRESHOLD     = 5.0f;     // px before drag activates
static constexpr float    MAX_PAN_SPEED      = 2200.0f;  // px/s cap
static constexpr float    MIN_INERTIA_SPEED  = 100.0f;   // below this, inertia stops
static constexpr float    PAN_DAMPING        = 5.0f;     // velocity decay rate
static constexpr float    INERTIA_STOP_SPEED = 18.0f;    // snap-to-idle threshold
static constexpr float    TAP_MOVE_LIMIT     = 7.0f;     // max px movement for a tap
static constexpr uint32_t TAP_MAX_MS         = 350;      // max duration for a tap

// ============================================================================
//  Transitions
// ============================================================================

static constexpr float OPEN_DURATION     = 0.42f;                // seconds for expand
static constexpr float CLOSE_HALF_SWIPE  = SCREEN_H * 0.25f;    // ~80 px to commit close
static constexpr float SPRING_STIFFNESS  = 35.0f;                // spring-back pull
static constexpr float SPRING_DAMPING    = 24.0f;                // spring-back damping

// ============================================================================
//  Rendering
// ============================================================================

#define BACKGROUND_COLOR RGB565(0, 0, 0)
