#pragma once
#include "qd/base/base.h"
#include "qd/mem/fnvHash.h"



namespace qd::appMsg {

struct BaseMsg {
    uint32_t id;
    BaseMsg(uint32_t _id = 0)
        : id(_id)
    {}
};

template<uint32_t TID>
struct BaseMsg_ : public BaseMsg {
    constexpr static uint32_t CID = TID;
    BaseMsg_()
        : BaseMsg(TID)
    {}
};


struct OnAppRequestToQuit : MSGID_(OnAppRequestToQuit) {
    bool allowToQuit = true;
};


struct ON_ACTIVE_CHANGE : MSGID_(ON_ACTIVE_CHANGE) {
    bool m_bActive = false;
};


struct ON_VISIBLE_CHANGE : MSGID_(ON_VISIBLE_CHANGE) {
    bool m_bVisible = false;
};


struct ON_Z_ORDER_CHANGE : MSGID_(ON_Z_ORDER_CHANGE) {
    float m_ZOrder = 0;
};


struct IS_NEED_REPAINT : MSGID_(IS_NEED_REPAINT) {
    bool m_bNeedRepaint = false;
};


struct ON_PRE_DESTROY : MSGID_(ON_PRE_DESTROY) {};


struct RENDER_IMGUI_DEBUG_INFO_TREE : MSGID_(RENDER_IMGUI_DEBUG_INFO_TREE) {
    // ImAPI::CImGuiBase* pIm = nullptr;
};


}; // namespace qd::appMsg
