#pragma once
#include "amDebugger/debugger.h"
#include "amDebugger/debuggerConfig.h"
#include "qd/app/applicationPart.h"
#include "qd/base/base.h"
#include "qd/base/classIdCC.h"
#include "qd/qui/uiOperation.h"
#include <functional>
#include "qd/stl/fixed_vector.h"
#include "qd/stl/string.h"

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
class IVmDbgServiceBridge;


class IVmConnectionsManager
{
    TS_REFLECT_CLASS(amD::IVmConnectionsManager, void);

public:
    bool m_bFullyInitialized = false;
    virtual uint32_t getNumConnections() = 0;
    virtual ref_ptr<amD::IVmDbgServiceBridge> createVmProvider(const char* conn_id) = 0;
}; // class IVmConnectionsManager


//////////////////////////////////////////////////////////////////////////
class DebuggerApp
    : public qd::ApplicationPart
    , public qd::IOperationEnvironment
{
    TS_REFLECT_CLASS(amD::DebuggerApp, qd::ApplicationPart);

private:
    SDL_Window* m_pWindow = nullptr;
    SDL_Renderer* m_pWndRenderer = nullptr;
    qd::QImGuiContext* m_pQimGuiCtx = nullptr;
    uint32_t m_nCurDbgClientIdx = 0;
    int m_init = false;
    // Centralized refresh trigger: entire debugger UI (registers, disassembly,
    // memory, screen preview) refreshes at this rate. See updateAppPart().
    uint64_t m_lastStateFetchMs = 0;
    static constexpr uint64_t kStateFetchIntervalMs = 66; // ~15 FPS

public:
    bool m_bFullyInitialized = false;
    ref_ptr<amD::Debugger> m_pDebugger = nullptr; // current debugger client
    ref_ptr<amD::DebuggerDesktop> m_pGui;
    qd::OperationsRegistry* m_pOperationMgr = nullptr;

public:
    DebuggerApp();
    SDL_Window* getWindow() const { return m_pWindow; }
    SDL_Renderer* getRenderer() const { return m_pWndRenderer; }
    uint32_t getCurDbgClientIdx() const { return m_nCurDbgClientIdx; }

    virtual void onPartCreate(ApplicationPart::OnCreate_t& prm) override;
    void init();
    virtual void destroy() override;
    virtual void updateAppPart(float dt, float time) override;
    virtual void renderAppPart() override;
    
    // CRITICAL: Prevent AppPartsManager::update() from calling us before real VM is bound
    virtual bool isReadyToActivate() const override { return m_bFullyInitialized; }
    
    // Called when the real VM connection replaces the dummy connection
    void onVmBound();
    
    bool isWndVisible() const;
    void setWndVisible(bool v);
    virtual qd::EFlow onSdlEventProc(SDL_Event& event) override;

    IVm::VM* getVm() const;

    amD::Debugger* getDbg() const { return m_pDebugger; }

    qd::OperationsRegistry* getOperations() const { return m_pOperationMgr; }

    qd::EFlow applyOperationMsgProcImp(qd::operation::BaseOpArgs* p_msg) override;

    // Callback to forward operations to the real emulator.
    // Set by the application to bridge debugger operations to the actual emulator thread.
    using ForwardOpToEmulatorCb = std::function<void(qd::operation::BaseOpArgs*)>;
    ForwardOpToEmulatorCb m_forwardOpToEmulatorCb;
    void setForwardOpToEmulatorCb(ForwardOpToEmulatorCb cb) { m_forwardOpToEmulatorCb = std::move(cb); }

    // Forward an operation to the real emulator via the registered callback.
    // Called from DebuggerDesktop before dispatching to the dummy VM.
    void forwardOpToEmulator(qd::operation::BaseOpArgs* args) {
        if (m_forwardOpToEmulatorCb)
            m_forwardOpToEmulatorCb(args);
    }

private:
    void createRenderWindow();
    void initImGui();
    void loadLayoutSettings();
    virtual ~DebuggerApp() override;

}; // class DebuggerApp
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


}; // namespace amD
