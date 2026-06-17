#pragma once

#include <Arduino.h>
#include <Wire.h>
#include "config.h"

void touchInit();
bool touchRead(int &x, int &y);
