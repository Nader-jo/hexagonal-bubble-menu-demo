#include "touch.h"
#include "esp_lcd_touch_axs5106l.h"

void touchInit()
{
    Wire.begin(PIN_TOUCH_SDA, PIN_TOUCH_SCL);
    bsp_touch_init(&Wire, PIN_TOUCH_RST, PIN_TOUCH_INT,
                   ROTATION, SCREEN_W, SCREEN_H);
}

bool touchRead(int &x, int &y)
{
    bsp_touch_read();
    touch_data_t td;
    bool pressed = bsp_touch_get_coordinates(&td);

    if (pressed)
    {
        x = td.coords[0].x;
        y = td.coords[0].y;
    }

    return pressed;
}
