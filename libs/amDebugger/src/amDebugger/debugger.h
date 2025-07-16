#pragma once
#include <EASTL/fixed_set.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/string.h>
#include <amDebugger/vm/memory.h>
#include <amDebugger/vm/vm.h>
#include <qd/base/base.h>
#include <qd/app/appPart.h>
#include <qd/base/classIdCC.h>


struct SDL_Window;
struct SDL_Renderer;
union SDL_Event;
typedef size_t csh;

FORWARD_DECLARATION_4S(qd, operation, msg, Base);
FORWARD_DECLARATION_2(qd, QImGuiContext);
FORWARD_DECLARATION_2(qd, UiOperationMgr);


//////////////////////////////////////////////////////////////////////////
namespace amD {
class DbgGuiDesktop;
class VM;


constexpr static int BREAKPOINTS_MAX = 20;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class Breakpoint {
public:
    AddrRef addr1 = {};
    AddrRef addr2 = {};
    bool enabled = false;
    EReg reg;
};  // class Breakpoint
//////////////////////////////////////////////////////////////////////////


class BreakpointsSortedList {
    eastl::fixed_vector<Breakpoint, amD::BREAKPOINTS_MAX, false> mBreakpoints;

    struct OneAddrBp {
        AddrRef addr;
        int bpIdx;

        bool operator<(const OneAddrBp& rh) const {
            return addr < rh.addr;
        }
    };
    eastl::fixed_set<OneAddrBp, amD::BREAKPOINTS_MAX, false> mOneAddrBps;

public:
    void init();
    const amD::Breakpoint* getBpByAddr(AddrRef addr, EReg reg) const;
};  // BreakpointsSortedList
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
class Debugger : public qd::AppPartBase
{
    TS_REFLECT_CLASS(amD::Debugger, qd::AppPartBase);

    SDL_Window* m_pWindow = nullptr;
    SDL_Renderer* m_pRenderer = nullptr;
    qd::QImGuiContext* m_pQimGui = nullptr;

public:
    csh* m_pCapstone = nullptr;
    amD::VM* vm = nullptr;
    amD::DbgGuiDesktop* m_pGui = nullptr;
    qd::UiOperationMgr* m_pOperationMgr = nullptr;

    SDL_Renderer* getRenderer() const {
        return m_pRenderer;
    }

private:
    int mWaitScanLines = 1;
    int mbInit = false;

public:

    Debugger();
    inline static Debugger* g_pInstance = nullptr;
    static Debugger* get();

    void init();
    virtual void destroy() override;
    virtual void update(float dt, float time) override;
    virtual void render() override;
    bool isVisible() const;
    void toggleWndVisible(DebuggerMode mode);
    virtual void onSdlEventProc(SDL_Event& event) override;

    amD::VM* getVm() const {
        return vm;
    }

    qd::UiOperationMgr* getOperations() const {
        return m_pOperationMgr;
    }

    static bool isDebugActivated();
    static bool isDebugActivatedFull();
    void setDebugMode(DebuggerMode debug_mode);

    qd::EFlow applyOperationMsg(qd::operation::msg::Base* p_msg) const;

    void execConsoleCmd(eastl::string&& cmd);

    void applyImmediateConsoleCmd(eastl::string&& cmd);

    int waitConsoleCmd(char* out, int maxlen);

    int getWaitScanLines() const {
        return mWaitScanLines;
    }
    void setWaitScanLines(int waitScanLines) {
        mWaitScanLines = waitScanLines;
    }


    BreakpointsSortedList getBreakpointsSorted() const {
        BreakpointsSortedList bp;
        bp.init();
        return bp;
    }


private:
    void createRenderWindow();
    void initImGui();
    ~Debugger();
};  // class Debugger
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


};  // namespace amD
