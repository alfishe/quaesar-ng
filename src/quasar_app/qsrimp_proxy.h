#pragma once
#include <stdint.h>


void qsrimp_setUaeInitiized(bool init);

uint32_t* qsrimp_lockUaeScreenTexBuf(int amiga_width, int amiga_height);

void qsrimp_unlockUaeScreenTexBuf();

bool qsrimp_onUaeHandleEvents();

int qsrimp_waitConsoleCmd(char* out, int maxlen);
