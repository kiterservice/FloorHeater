// screensaver.h
#pragma once
#include <stdint.h>

void screensaverInit();
void screensaverEnter();
void screensaverExit();
bool screensaverIsActive();
void screensaverTick(uint32_t nowMs);