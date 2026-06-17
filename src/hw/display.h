#pragma once

#include <Arduino_GFX_Library.h>
#include "config.h"

Arduino_DataBus *createDisplayBus();
Arduino_GFX *createDisplay(Arduino_DataBus *bus);
void lcdRegisterInit(Arduino_DataBus *bus);
