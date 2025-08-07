#pragma once
#include <EASTL/fixed_set.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/string.h>
#include "qd/base/base.h"
#include "qd/app/appPart.h"
#include "qd/base/classIdCC.h"
#include "qd/qui/uiOperation.h"
#include "amDebugger/debugger.h"


struct SDL_Window;
struct SDL_Renderer;
union SDL_Event;

FORWARD_DECLARATION_4S(qd, operation, args, Base);
FORWARD_DECLARATION_2(qd, QImGuiContext);
FORWARD_DECLARATION_2(qd, UiOperationMgr);


//////////////////////////////////////////////////////////////////////////
namespace amD {
class DebuggerDesktop;
class AbsVM;
class IDbgConnection;


class IDbgConnectionManager
{
public:
    virtual uint32_t getNumConnections() = 0;
    virtual eastl::shared_ptr<IDbgConnection> getConnection(uint32_t idx) = 0;
};


//////////////////////////////////////////////////////////////////////////
class DebuggerApp : public qd::AppPart, public qd::IOperationEnvironment
{
    TS_REFLECT_CLASS(amD::DebuggerApp, qd::AppPart);
private:
    SDL_Window* m_pWindow = nullptr;
    SDL_Renderer* m_pWndRenderer = nullptr;
    qd::QImGuiContext* m_pQimGui = nullptr;
    uint32_t m_nCurDbgClientIdx = 0;
    int mbInit = false;

public:

    amD::Debugger* m_pCurDbgClient = nullptr;
    qd::vector<ref_ptr<amD::Debugger>> m_pClients;
    amD::DebuggerDesktop* m_pGui = nullptr;
    qd::UiOperationMgr* m_pOperationMgr = nullptr;

public:
    SDL_Renderer* getRenderer() const {
        return m_pWndRenderer;
    }

    uint32_t getCurDbgClientIdx() const {
        return m_nCurDbgClientIdx;
    }
    void setCurDbgClientIdx(uint32_t curDbgClientIdx);

    DebuggerApp();
    inline static DebuggerApp* g_pInstance = nullptr;
    static DebuggerApp* get();

    virtual void onPartCreate(AppPart::OnCreate_t& prm) override;
    void init();
    virtual void destroy() override;
    virtual void update(float dt, float time) override;
    virtual void render() override;
    bool isWndVisible() const;
    void toggleWndVisible(DebuggerMode mode);
    virtual qd::EFlow onSdlEventProc(SDL_Event& event) override;

    amD::AbsVM* getVm() const {
        return m_pCurDbgClient->vm;
    }

    amD::Debugger* getDbg() const {
        return m_pCurDbgClient;
    }

    qd::UiOperationMgr* getOperations() const {
        return m_pOperationMgr;
    }

    qd::EFlow applyOperationMsg(qd::operation::args::Base* p_msg);

    virtual void* getOpEnvPtr(const qd::TypeInfo& classType) const override;

private:
    void createRenderWindow();
    void initImGui();
    ~DebuggerApp();

};  // class DebuggerApp
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


};  // namespace amD
