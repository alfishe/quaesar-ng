#include "qsrimp_proxy.h"
#include "uae_app_imp/uae_server_thread.h"

UaeServerThread* g_pUaeThread = nullptr;


void qsrimp_setUaeInitiized(bool init) {
    g_pUaeThread = UaeServerThread::get();
    g_pUaeThread->setUaeInitialized(init);
}


uint32_t* qsrimp_lockUaeScreenTexBuf(int amiga_width, int amiga_height) {
    return g_pUaeThread->lockUaeScreenTexBuf(amiga_width, amiga_height);
}


void qsrimp_unlockUaeScreenTexBuf() {
    g_pUaeThread->unlockUaeScreenTexBuf();
}


bool qsrimp_onUaeHandleEvents() {
    return g_pUaeThread->onUaeHandleEvents();
}


int qsrimp_waitConsoleCmd(char* out, int maxlen) {
    if (!g_pUaeThread)
        return -1;
    return g_pUaeThread->waitConsoleCmd(out, maxlen);
}
