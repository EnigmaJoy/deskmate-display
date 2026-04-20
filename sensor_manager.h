#pragma once
#include "data_store.h"

void sensorInit();
void sensorReadIfDue();   // chiama ogni loop(); legge ogni 5 s
