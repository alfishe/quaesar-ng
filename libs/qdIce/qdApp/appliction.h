#pragma once
#include <qdIce/qdBase/ref_ptr.h>

namespace qd
{
class AppPartsManager;
class ModuleManager;
struct CreateApplicationParams;

	
class Application
{

    ModuleManager* m_pModuleManager;
    AppPartsManager* m_pAppParts = nullptr;


public:
    Application(CreateApplicationParams *prm);


    virtual ~Application();

    AppPartsManager* getAppParts() const {
        return m_pAppParts;
    }


}; // Application
//////////////////////////////////////////////////////////////////////////



struct CreateApplicationParams
{

};


// GLOBAL VARIABLES
extern Application* g_pApp;


}; // namespace qd
