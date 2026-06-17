#pragma once

#include "config.h"

// ============================================================================
//  Focus Region – which zone a bubble falls into
// ============================================================================

enum class FocusRegion { Center, Fringe, Outer };

// ============================================================================
//  Projected Bubble – a bubble after camera transform and focus scaling
// ============================================================================

struct ProjectedBubble {
    int itemIndex;              // index into ITEMS[]
    Vec2 rawCenter;             // world position + camera offset
    Vec2 center;                // after compaction pull toward center
    float signedFocusDistance;  // signed distance from focus rect boundary
    FocusRegion region;         // Center / Fringe / Outer
    float focus;                // 1.0 = full center, 0.0 = full outer
    float radius;               // interpolated between MIN and MAX
    uint16_t color;
    bool visible;               // within screen bounds
};

// ============================================================================
//  Menu State Machine
//
//  GridIdle    → grid visible, waiting for touch
//  GridTracking → finger down on grid, panning
//  GridInertia → finger lifted, momentum coasting
//  Opening     → bubble expanding to fill screen
//  Open        → full-screen color, waiting for close swipe
//  Closing     → shrinking back to bubble (commit or spring-back)
// ============================================================================

enum class MenuState {
    GridIdle,
    GridTracking,
    GridInertia,
    Opening,
    Open,
    Closing
};

// ============================================================================
//  Menu Runtime – all mutable state in one struct
// ============================================================================

struct MenuRuntime {
    // --- State machine ---
    MenuState state = MenuState::GridIdle;

    // --- Grid panning ---
    Vec2 cameraOffset = {SCREEN_CENTER_X, SCREEN_CENTER_Y};
    Vec2 velocity     = {0, 0};   // current pan velocity (px/s)
    Vec2 filteredTouchVelocity = {0, 0};  // low-pass filtered finger velocity

    // --- Touch tracking ---
    Vec2    touchStart    = {0, 0};
    Vec2    lastTouch     = {0, 0};
    uint32_t touchStartMs = 0;
    uint32_t lastTouchMs  = 0;
    uint32_t lastFrameMs  = 0;
    bool    dragActivated = false;

    // --- Tap / selection ---
    int selectedIndex = -1;

    // --- Opening transition (bubble → full screen) ---
    float transitionProgress    = 0.0f;   // 0→1 over OPEN_DURATION
    Vec2  transitionOrigin      = {0, 0}; // where the tapped bubble was
    float transitionStartRadius = 0.0f;   // bubble radius at tap
    float transitionFullRadius  = 0.0f;   // radius needed to fill screen

    // --- Closing transition (full screen → bubble) ---
    Vec2  savedCameraOffset = {0, 0};     // camera snapshot at tap moment
    float closeProgress     = 0.0f;       // 0 = full screen, 1 = back to bubble
    float closeTarget       = 0.0f;       // 1.0 = commit close, 0.0 = spring back

    // --- Open state swipe tracking ---
    Vec2 openTouchStart = {0, 0};         // finger position on down
    Vec2 openLastTouch  = {0, 0};         // latest finger position
};

// ============================================================================
//  Public API
// ============================================================================

void menuInit();
void menuOnTouchDown(int x, int y, uint32_t nowMs);
void menuOnTouchMove(int x, int y, uint32_t nowMs);
void menuOnTouchUp(uint32_t nowMs);
void menuUpdate();
ProjectedBubble menuProject(int index);
int  menuProjectAll(ProjectedBubble *out);
bool menuNeedsRedraw();
