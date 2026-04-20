#pragma once
#include "display_manager.h"

enum class TouchEvent { NONE, TAP, LONG_PRESS };

struct TouchPoint { int x = 0; int y = 0; };

void       touchInit();
TouchEvent touchPoll(TouchPoint& out);   // chiama ogni loop()
