#pragma once
#include <EASTL/fixed_set.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/string.h>
#include "qd/base/base.h"
#include "qd/app/applicationPart.h"
#include "qd/base/classIdCC.h"
#include "qd/qui/uiOperation.h"
#include "amDebugger/debugger.h"
#include "amDebugger/debuggerConfig.h"

struct SDL_Window;
struct SDL_Renderer;
union SDL_Event;

FORWARD_DECLARATION_4S(qd, operation, args, Base);
FORWARD_DECLARATION_2(qd, QImGuiContext);
FORWARD_DECLARATION_2(qd, OperationsRegistry);
FORWARD_DECLARATION_2(IVm, VM);


//////////////////////////////////////////////////////////////////////////
namespace amD {

class DebuggerDesktop;
class IVmServiceConnection;


class IVmConnectionsManager
{
    TS_REFLECT_CLASS(amD::IVmConnectionsManager, void);
public:
    virtual uint32_t getNumConnections() = 0;
    virtual ref_ptr<amD::IVmServiceConnection> createVmConnectionByInd(uint32_t idx) = 0;
}; // class IVmConnectionsManager


//////////////////////////////////////////////////////////////////////////
class DebuggerApp : public qd::ApplicationPart, public qd::IOperationEnvironment
{
    TS_REFLECT_CLASS(amD::DebuggerApp, qd::ApplicationPart);
private:
    SDL_Window* m_pWindow = nullptr;
    SDL_Renderer* m_pWndRenderer = nullptr;
    qd::QImGuiContext* m_pQimGuiCtx = nullptr;
    uint32_t m_nCurDbgClientIdx = 0;
    int m_init = false;

public:

    ref_ptr<amD::Debugger> m_pDebugger = nullptr; // current debugger client
    ref_ptr<amD::DebuggerDesktop> m_pGui;
    qd::OperationsRegistry* m_pOperationMgr = nullptr;

public:
    DebuggerApp();
    inline static DebuggerApp* g_pInstance = nullptr;
    static DebuggerApp* get();
    SDL_Renderer* getRenderer() const { return m_pWndRenderer; }
    uint32_t getCurDbgClientIdx() const { return m_nCurDbgClientIdx; }

    virtual void onPartCreate(ApplicationPart::OnCreate_t& prm) override;
    void init();
    virtual void destroy() override;
    virtual void update(float dt, float time) override;
    virtual void render() override;
    bool isWndVisible() const;
    void setWndVisible(bool v);
    virtual qd::EFlow onSdlEventProc(SDL_Event& event) override;

    IVm::VM* getVm() const {
        return m_pDebugger->getVm();
    }

    amD::Debugger* getDbg() const {
        return m_pDebugger;
    }

    qd::OperationsRegistry* getOperations() const {
        return m_pOperationMgr;
    }

    qd::EFlow applyOperationMsgProcImp(qd::operation::BaseOpArgs* p_msg) override;

private:
    void createRenderWindow();
    void initImGui();
    virtual ~DebuggerApp() override;

};  // class DebuggerApp
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


};  // namespace amD
