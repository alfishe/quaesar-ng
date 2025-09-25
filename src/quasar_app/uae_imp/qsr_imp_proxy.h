#pragma once
#include <stdint.h>


void qsr_setUaeInitiized(bool init);

uint32_t* qsr_lockUaeScreenTexBuf(int amiga_width, int amiga_height);

void qsr_unlockUaeScreenTexBuf();

bool qsr_onUaeHandleEvents();

int qsr_waitConsoleCmd(char* out, int maxlen);
