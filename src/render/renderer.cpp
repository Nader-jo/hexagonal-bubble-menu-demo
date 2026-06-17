#include "renderer.h"
#include "menu.h"
#include "config.h"

// ============================================================================
//  Canvas (off-screen double buffer) and display handle
// ============================================================================

static Arduino_Canvas *canvas = nullptr;
static Arduino_GFX    *gfx    = nullptr;

extern bool needsRedraw;
extern MenuRuntime rt;

// ============================================================================
//  Init
// ============================================================================

void rendererInit(Arduino_GFX *display)
{
    gfx = display;
    canvas = new Arduino_Canvas(SCREEN_W, SCREEN_H, gfx, 0, 0, 0);
    canvas->begin(GFX_SKIP_OUTPUT_BEGIN);
}

// ============================================================================
//  Insertion Sort by Focus (ascending) – draws smallest/outer bubbles first
// ============================================================================

static void sortByFocusAscending(ProjectedBubble *bubbles, int count)
{
    for (int i = 1; i < count; ++i)
    {
        ProjectedBubble key = bubbles[i];
        int j = i - 1;
        while (j >= 0 && bubbles[j].focus > key.focus)
            bubbles[j + 1] = bubbles[j], --j;
        bubbles[j + 1] = key;
    }
}

// ============================================================================
//  Draw Single Bubble – shadow + filled circle
// ============================================================================

static void drawBubble(const ProjectedBubble &b)
{
    int x = lroundf(b.center.x);
    int y = lroundf(b.center.y);
    int r = max(2, (int)lroundf(b.radius));
    int shadow = 1 + lroundf(2.0f * b.focus);

    canvas->fillCircle(x + shadow, y + shadow, r, RGB565(18, 18, 24));
    canvas->fillCircle(x, y, r, b.color);
}

// ============================================================================
//  Grid Renderer – black background, all visible bubbles
// ============================================================================

static void renderGrid()
{
    canvas->fillScreen(BACKGROUND_COLOR);

    ProjectedBubble visible[ITEM_COUNT];
    int count = menuProjectAll(visible);
    sortByFocusAscending(visible, count);

    for (int i = 0; i < count; ++i)
        drawBubble(visible[i]);
}

// ============================================================================
//  Opening Renderer – grid + expanding circle toward center
// ============================================================================

static void renderOpening()
{
    renderGrid();

    float t = easeOutCubic(rt.transitionProgress);
    Vec2 center = lerp(rt.transitionOrigin, Vec2{SCREEN_CENTER_X, SCREEN_CENTER_Y}, t);
    float radius = lerpf(rt.transitionStartRadius, rt.transitionFullRadius, t);

    canvas->fillCircle(lroundf(center.x), lroundf(center.y), lroundf(radius),
                       ITEMS[rt.selectedIndex].color);
}

// ============================================================================
//  Open Renderer – solid color filling the entire screen
// ============================================================================

static void renderOpen()
{
    canvas->fillScreen(ITEMS[rt.selectedIndex].color);
}

// ============================================================================
//  Closing Renderer – grid frozen at tap-moment + shrinking circle
//
//  During a swipe (Open state with closeProgress > 0) or after commit
//  (Closing state), this renders the circle shrinking from full-screen
//  back toward the original bubble position.
// ============================================================================

static void renderClosing()
{
    // Freeze grid at the camera offset saved when the bubble was tapped
    Vec2 saved = rt.cameraOffset;
    rt.cameraOffset = rt.savedCameraOffset;
    renderGrid();
    rt.cameraOffset = saved;

    // Shrink circle: full-screen → original bubble position
    float t = smoothstep01(clampf(rt.closeProgress, 0.0f, 1.0f));
    Vec2 center = lerp(Vec2{SCREEN_CENTER_X, SCREEN_CENTER_Y}, rt.transitionOrigin, t);
    float radius = lerpf(rt.transitionFullRadius, rt.transitionStartRadius, t);

    canvas->fillCircle(lroundf(center.x), lroundf(center.y), lroundf(radius),
                       ITEMS[rt.selectedIndex].color);
}

// ============================================================================
//  Frame Render – dispatch to state-specific renderer, then push to display
// ============================================================================

void rendererFrame()
{
    switch (rt.state)
    {
        case MenuState::GridIdle:
        case MenuState::GridTracking:
        case MenuState::GridInertia:
            renderGrid();
            break;

        case MenuState::Opening:
            renderOpening();
            break;

        case MenuState::Open:
            // During swipe, show shrinking; otherwise full-screen color
            rt.closeProgress > 0.0f ? renderClosing() : renderOpen();
            break;

        case MenuState::Closing:
            renderClosing();
            break;
    }

    // Push canvas to physical display
    gfx->draw16bitRGBBitmap(0, 0, (uint16_t *)canvas->getFramebuffer(),
                            SCREEN_W, SCREEN_H);
    needsRedraw = false;
}
