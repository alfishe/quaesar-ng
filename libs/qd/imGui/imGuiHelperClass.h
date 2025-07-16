#pragma once
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include "qd/base/tribool.h"


namespace qIm {

struct BeginChild {
    qd::Tribool res;

    BeginChild(const char* str_id, const ImVec2& size = ImVec2(0, 0), ImGuiChildFlags child_flags = 0,
        ImGuiWindowFlags window_flags = 0)
    {
        res = (qd::Tribool)ImGui::BeginChild(str_id, size, child_flags, window_flags);
    }

    operator bool () const { return res.isTrue(); }

    ~BeginChild()
    {
        if (res.isTrue())
            ImGui::EndChild();
    }
}; // struct BeginChild
//////////////////////////////////////////////////////////////////////////


struct BeginMenu {
    qd::Tribool res;

    BeginMenu(const char* label, bool enabled = true) { res = (qd::Tribool)ImGui::BeginMenu(label, enabled); }

    operator bool () const { return res.isTrue(); }

    ~BeginMenu()
    {
        if (res.isTrue())
            ImGui::EndMenu();
    }
}; // struct BeginMenu
//////////////////////////////////////////////////////////////////////////


}; // namespace qIm
