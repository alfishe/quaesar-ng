#include "qdIce/qdApp/appliction.h"
#include "qdIce/qdApp/appPartsMgr.h"


namespace qd {
Application* g_pApp = nullptr;


 Application::Application(CreateApplicationParams* prm) {
    m_pAppParts = new AppPartsManager(&ModuleCreateParams(this));
}


 Application::~Application() {
    SAFE_DELETE(m_pAppParts);
}


};  // namespace qd
