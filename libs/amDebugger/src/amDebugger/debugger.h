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


//////////////////////////////////////////////////////////////////////////
namespace qd {
class GuiManager;
class VM;
class UiOperationMgr;


constexpr static int BREAKPOINTS_MAX = 20;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum DebuggerMode {
    DebuggerMode_Live,
    DebuggerMode_Break,
};
//////////////////////////////////////////////////////////////////////////

class Breakpoint {
public:
    AddrRef addr1 = {};
    AddrRef addr2 = {};
    bool enabled = false;
    EReg reg;
};  // class Breakpoint
//////////////////////////////////////////////////////////////////////////


class BreakpointsSortedList {
    eastl::fixed_vector<Breakpoint, qd::BREAKPOINTS_MAX, false> mBreakpoints;

    struct OneAddrBp {
        AddrRef addr;
        int bpIdx;

        bool operator<(const OneAddrBp& rh) const {
            return addr < rh.addr;
        }
    };
    eastl::fixed_set<OneAddrBp, qd::BREAKPOINTS_MAX, false> mOneAddrBps;

public:
    void init();
    const qd::Breakpoint* getBpByAddr(AddrRef addr, EReg reg) const;
};  // BreakpointsSortedList
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
class Debugger : public qd::AppPartBase
{
    TS_REFLECT_CLASS(qd::Debugger, qd::AppPartBase);

    SDL_Window* m_pWindow = nullptr;
    SDL_Renderer* m_pRenderer = nullptr;

public:
    csh* m_pCapstone = nullptr;
    VM* vm = nullptr;
    GuiManager* gui = nullptr;
    UiOperationMgr* m_pOperations = nullptr;

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
    void destroy();
    void update();
    void render();
    bool isVisible() const;
    void toggleWndVisible(DebuggerMode mode);
    void sdlEventProc(SDL_Event* event);

    qd::VM* getVm() const {
        return vm;
    }

    UiOperationMgr* getOperations() const {
        return m_pOperations;
    }

    static bool isDebugActivated();
    static bool isDebugActivatedFull();
    void setDebugMode(DebuggerMode debug_mode);

    EFlow applyOperationMsg(qd::operation::msg::Base* p_msg) const;

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


};  // namespace qd
