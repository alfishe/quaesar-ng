#include "qsr_imp_proxy.h"
#include "uae_imp/uae_server_thread.h"

UaeServerThread* g_pUaeThread = nullptr;


void qsr_setUaeInitiized(bool init) {
    g_pUaeThread = UaeServerThread::get();
    g_pUaeThread->setUaeInitialized(init);
}


uint32_t* qsr_lockUaeScreenTexBuf(int amiga_width, int amiga_height) {
    return g_pUaeThread->lockUaeScreenTexBuf(amiga_width, amiga_height);
}


void qsr_unlockUaeScreenTexBuf() {
    g_pUaeThread->unlockUaeScreenTexBuf();
}


bool qsr_onUaeHandleEvents() {
    return g_pUaeThread->onUaeHandleEvents();
}


int qsr_waitConsoleCmd(char* out, int maxlen) {
    if (!g_pUaeThread)
        return -1;
    return g_pUaeThread->uaeWaitConsoleCmdImpl(out, maxlen);
}
