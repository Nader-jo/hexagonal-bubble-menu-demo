#include "menu.h"
#include <algorithm>

// ============================================================================
//  Globals
// ============================================================================

MenuRuntime rt;
bool needsRedraw = true;

// ============================================================================
//  Focus Model – Rounded-Rectangle Signed Distance
//
//  Returns negative values inside the center rect, positive outside.
//  The absolute value gives distance to the nearest boundary point.
// ============================================================================

static float roundedRectSignedDistance(Vec2 p)
{
    // Distance from screen center (absolute)
    float dx = fabsf_f(p.x - SCREEN_CENTER_X);
    float dy = fabsf_f(p.y - SCREEN_CENTER_Y);

    // Internal corner center (before rounding)
    float cx = CENTER_REGION_HALF_W - CENTER_REGION_CORNER_R;
    float cy = CENTER_REGION_HALF_H - CENTER_REGION_CORNER_R;

    // Offset from internal corner
    float qx = dx - cx;
    float qy = dy - cy;

    // Outside corner region: Euclidean distance to corner center
    float outsideX = qx > 0.0f ? qx : 0.0f;
    float outsideY = qy > 0.0f ? qy : 0.0f;
    float cornerDist = sqrtf(outsideX * outsideX + outsideY * outsideY);

    // Inside corner region: max signed distance (negative = inside)
    float insideDist = (qx < qy ? qx : qy);
    if (insideDist > 0.0f) insideDist = 0.0f;

    return cornerDist + insideDist - CENTER_REGION_CORNER_R;
}

// ============================================================================
//  Focus Value: 1.0 at center, 0.0 at outer edge, smooth transition in fringe
// ============================================================================

static float calculateFocus(float signedDist)
{
    if (signedDist <= 0.0f) return 1.0f;                   // inside center rect
    if (signedDist >= FRINGE_WIDTH) return 0.0f;           // beyond fringe
    return 1.0f - smoothstep01(signedDist / FRINGE_WIDTH); // smooth blend
}

// ============================================================================
//  Bubble Projection
//  Transforms world position → screen position with focus-based scaling
//  and compaction pull (growing bubbles shift toward center).
// ============================================================================

ProjectedBubble menuProject(int index)
{
    ProjectedBubble b{};
    Vec2 raw = add(ITEMS[index].worldPosition, rt.cameraOffset);

    float signedDist = roundedRectSignedDistance(raw);
    float focus = calculateFocus(signedDist);
    float radius = MIN_BUBBLE_RADIUS + focus * (MAX_BUBBLE_RADIUS - MIN_BUBBLE_RADIUS);

    // Compaction: pull toward center proportional to how much the bubble grew
    Vec2 toCenter = sub(Vec2{SCREEN_CENTER_X, SCREEN_CENTER_Y}, raw);
    float dist = lengthVec(toCenter);
    Vec2 dir = dist > 0.001f ? mul(toCenter, 1.0f / dist) : Vec2{0, 0};
    Vec2 compacted = add(raw, mul(dir, (radius - MIN_BUBBLE_RADIUS) * COMPACT_PULL_GAIN));

    b.itemIndex          = index;
    b.rawCenter          = raw;
    b.center             = compacted;
    b.signedFocusDistance = signedDist;
    b.region             = signedDist <= 0 ? FocusRegion::Center
                                 : signedDist < FRINGE_WIDTH ? FocusRegion::Fringe
                                                              : FocusRegion::Outer;
    b.focus   = focus;
    b.radius  = radius;
    b.color   = ITEMS[index].color;
    b.visible = compacted.x + radius >= 0 && compacted.x - radius < SCREEN_W
             && compacted.y + radius >= 0 && compacted.y - radius < SCREEN_H;

    return b;
}

int menuProjectAll(ProjectedBubble *out)
{
    int count = 0;
    for (int i = 0; i < ITEM_COUNT; ++i)
    {
        ProjectedBubble b = menuProject(i);
        if (b.visible) out[count++] = b;
    }
    return count;
}

// ============================================================================
//  Layout Generation
//
//  Creates a hex grid of candidate positions, then picks the closest
//  ITEM_COUNT positions to screen center. Sorting uses a weighted
//  distance (3x on X) so the vertical axis fills first on the tall screen.
// ============================================================================

static void buildHexLayout()
{
    static constexpr int MAX_CANDIDATES = 300;
    Vec2 candidates[MAX_CANDIDATES];
    int count = 0;

    for (int row = -12; row <= 12 && count < MAX_CANDIDATES; ++row)
        for (int col = -6; col <= 6 && count < MAX_CANDIDATES; ++col)
        {
            float x = col * H_SPACING + (row & 1 ? H_SPACING * 0.5f : 0.0f);
            float y = row * V_SPACING;
            candidates[count++] = {x, y};
        }

    // Weighted sort: prioritize Y axis (taller screen dimension)
    std::sort(candidates, candidates + count, [](Vec2 a, Vec2 b) {
        return a.y*a.y + a.x*a.x*3.0f < b.y*b.y + b.x*b.x*3.0f;
    });

    for (int i = 0; i < ITEM_COUNT && i < count; ++i)
        ITEMS[i].worldPosition = candidates[i];
}

// ============================================================================
//  Init
// ============================================================================

void menuInit()
{
    buildHexLayout();
    rt.cameraOffset = {SCREEN_CENTER_X, SCREEN_CENTER_Y};
    rt.state = MenuState::GridIdle;
    needsRedraw = true;
}

// ============================================================================
//  Touch Handlers
// ============================================================================

void menuOnTouchDown(int x, int y, uint32_t nowMs)
{
    Vec2 pos = {(float)x, (float)y};

    if (rt.state == MenuState::GridIdle || rt.state == MenuState::GridInertia)
    {
        // Start tracking a new pan gesture
        rt.state = MenuState::GridTracking;
        rt.touchStart = pos;
        rt.lastTouch = pos;
        rt.touchStartMs = nowMs;
        rt.lastTouchMs = nowMs;
        rt.velocity = {0, 0};
        rt.filteredTouchVelocity = {0, 0};
        rt.dragActivated = false;
    }
    else if (rt.state == MenuState::Open)
    {
        // Start tracking a close-swipe gesture
        rt.openTouchStart = pos;
        rt.openLastTouch = pos;
        rt.touchStartMs = nowMs;
    }

    needsRedraw = true;
}

void menuOnTouchMove(int x, int y, uint32_t nowMs)
{
    Vec2 pos = {(float)x, (float)y};

    if (rt.state == MenuState::GridTracking)
    {
        // Activate drag once finger moves past threshold
        if (!rt.dragActivated && lengthSq(sub(pos, rt.touchStart)) >= DRAG_THRESHOLD * DRAG_THRESHOLD)
            rt.dragActivated = true;

        if (rt.dragActivated)
        {
            Vec2 delta = sub(pos, rt.lastTouch);
            rt.cameraOffset = add(rt.cameraOffset, delta);

            // Low-pass filter velocity for smooth inertia
            uint32_t dtMs = clampi((int)(nowMs - rt.lastTouchMs), 1, 50);
            Vec2 measured = mul(delta, 1000.0f / dtMs);
            rt.filteredTouchVelocity = add(mul(rt.filteredTouchVelocity, 0.70f), mul(measured, 0.30f));
            rt.velocity = rt.filteredTouchVelocity;

            float speed = lengthVec(rt.velocity);
            if (speed > MAX_PAN_SPEED)
                rt.velocity = mul(rt.velocity, MAX_PAN_SPEED / speed);
        }

        rt.lastTouch = pos;
        rt.lastTouchMs = nowMs;
    }
    else if (rt.state == MenuState::Open)
    {
        // Drive closing progress directly from swipe distance
        rt.openLastTouch = pos;
        float swipeDist = lengthVec(sub(pos, rt.openTouchStart));
        rt.closeProgress = clampf(swipeDist / CLOSE_HALF_SWIPE, 0.0f, 1.0f);
    }

    needsRedraw = true;
}

void menuOnTouchUp(uint32_t nowMs)
{
    if (rt.state == MenuState::GridTracking)
    {
        Vec2 total = sub(rt.lastTouch, rt.touchStart);
        uint32_t duration = nowMs - rt.touchStartMs;

        bool tap = !rt.dragActivated
                && lengthSq(total) < TAP_MOVE_LIMIT * TAP_MOVE_LIMIT
                && duration <= TAP_MAX_MS;

        if (tap)
        {
            // Hit-test: find topmost bubble under finger
            ProjectedBubble visible[ITEM_COUNT];
            int count = menuProjectAll(visible);

            // Sort by focus ascending → most focused bubble tested last (on top)
            for (int i = 1; i < count; ++i)
            {
                ProjectedBubble key = visible[i];
                int j = i - 1;
                while (j >= 0 && visible[j].focus > key.focus)
                    visible[j + 1] = visible[j], --j;
                visible[j + 1] = key;
            }

            for (int i = count - 1; i >= 0; --i)
            {
                if (lengthSq(sub(rt.lastTouch, visible[i].center)) <= visible[i].radius * visible[i].radius)
                {
                    // Begin opening animation
                    rt.selectedIndex = visible[i].itemIndex;
                    rt.transitionOrigin = visible[i].center;
                    rt.transitionStartRadius = visible[i].radius;
                    rt.savedCameraOffset = rt.cameraOffset;
                    rt.transitionProgress = 0.0f;
                    rt.state = MenuState::Opening;
                    needsRedraw = true;
                    return;
                }
            }
        }
        else
        {
            // Not a tap → transition to inertia or idle
            rt.state = lengthVec(rt.velocity) > MIN_INERTIA_SPEED
                     ? MenuState::GridInertia
                     : MenuState::GridIdle;
        }

        needsRedraw = true;
    }
    else if (rt.state == MenuState::Open)
    {
        // Decide: commit close or spring back based on swipe distance
        float swipeDist = lengthVec(sub(rt.openLastTouch, rt.openTouchStart));
        rt.state = MenuState::Closing;
        rt.closeTarget = swipeDist >= CLOSE_HALF_SWIPE ? 1.0f : 0.0f;
        needsRedraw = true;
    }
}

// ============================================================================
//  Animation Updates (called each frame)
// ============================================================================

// Inertia: coast with exponential velocity decay
static void updateGridInertia(float dt)
{
    rt.cameraOffset = add(rt.cameraOffset, mul(rt.velocity, dt));
    rt.velocity = mul(rt.velocity, expf(-PAN_DAMPING * dt));

    if (lengthVec(rt.velocity) < INERTIA_STOP_SPEED)
    {
        rt.velocity = {0, 0};
        rt.state = MenuState::GridIdle;
    }

    needsRedraw = true;
}

// Spring-back: pull camera toward screen center when idle,
// but only if the bubble cluster doesn't already fill most of the screen.
static void updateGridSpringBack(float dt)
{
    // Estimate how much of the screen the cluster covers
    float minX = 1e9f, maxX = -1e9f, minY = 1e9f, maxY = -1e9f;
    int visCount = 0;

    for (int i = 0; i < ITEM_COUNT; ++i)
    {
        ProjectedBubble b = menuProject(i);
        if (!b.visible) continue;
        visCount++;
        if (b.center.x - b.radius < minX) minX = b.center.x - b.radius;
        if (b.center.x + b.radius > maxX) maxX = b.center.x + b.radius;
        if (b.center.y - b.radius < minY) minY = b.center.y - b.radius;
        if (b.center.y + b.radius > maxY) maxY = b.center.y + b.radius;
    }

    if (visCount > 0)
    {
        // Clip to screen and compute coverage ratio
        float w = (maxX > SCREEN_W ? SCREEN_W : maxX) - (minX < 0 ? 0 : minX);
        float h = (maxY > SCREEN_H ? SCREEN_H : maxY) - (minY < 0 ? 0 : minY);
        if (w * h / (SCREEN_W * SCREEN_H) >= 0.50f)
            return;  // cluster fills enough → skip spring-back
    }

    // Damped spring toward center
    Vec2 target = {SCREEN_CENTER_X, SCREEN_CENTER_Y};
    Vec2 error = sub(target, rt.cameraOffset);

    if (fabsf_f(error.x) < 0.3f && fabsf_f(error.y) < 0.3f
     && fabsf_f(rt.velocity.x) < 1.0f && fabsf_f(rt.velocity.y) < 1.0f)
    {
        rt.cameraOffset = target;
        rt.velocity = {0, 0};
        return;
    }

    Vec2 accel = {
        SPRING_STIFFNESS * error.x - SPRING_DAMPING * rt.velocity.x,
        SPRING_STIFFNESS * error.y - SPRING_DAMPING * rt.velocity.y
    };

    rt.velocity = add(rt.velocity, mul(accel, dt));
    rt.cameraOffset = add(rt.cameraOffset, mul(rt.velocity, dt));
    needsRedraw = true;
}

// Opening: expand bubble from its position to fill the screen
static void updateOpening(float dt)
{
    rt.transitionProgress = clampf(rt.transitionProgress + dt / OPEN_DURATION, 0.0f, 1.0f);
    if (rt.transitionProgress >= 1.0f)
        rt.state = MenuState::Open;
    needsRedraw = true;
}

// Closing: ease closeProgress toward target (1.0 = closed, 0.0 = reopened)
static void updateClosing(float dt)
{
    rt.closeProgress += (rt.closeTarget - rt.closeProgress) * 6.0f * dt;

    if (rt.closeTarget > 0.5f && rt.closeProgress > 0.98f)
    {
        // Fully closed → return to grid
        rt.state = MenuState::GridIdle;
        rt.selectedIndex = -1;
        rt.closeProgress = 0.0f;
    }
    else if (rt.closeTarget < 0.5f && rt.closeProgress < 0.02f)
    {
        // Fully reopened → back to open state
        rt.state = MenuState::Open;
        rt.closeProgress = 0.0f;
    }

    needsRedraw = true;
}

// ============================================================================
//  Public Update – dispatch to current state's update function
// ============================================================================

void menuUpdate()
{
    uint32_t now = millis();
    uint32_t dtMs = now - rt.lastFrameMs;
    if (dtMs < 1) return;
    if (dtMs > 33) dtMs = 33;
    rt.lastFrameMs = now;

    float dt = dtMs / 1000.0f;

    switch (rt.state)
    {
        case MenuState::GridIdle:     updateGridSpringBack(dt); break;
        case MenuState::GridInertia:  updateGridInertia(dt);    break;
        case MenuState::Opening:      updateOpening(dt);        break;
        case MenuState::Closing:      updateClosing(dt);        break;
        default: break;  // GridTracking, Open – no per-frame update needed
    }
}

bool menuNeedsRedraw() { return needsRedraw; }
