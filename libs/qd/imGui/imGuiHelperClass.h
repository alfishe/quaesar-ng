#pragma once
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include "qd/base/Tribool.h"


namespace qIm {

struct LockChild {
    qd::Tribool res;

    LockChild(const char* str_id, const ImVec2& size = ImVec2(0, 0), ImGuiChildFlags child_flags = 0,
        ImGuiWindowFlags window_flags = 0)
    {
        res = (qd::Tribool)ImGui::BeginChild(str_id, size, child_flags, window_flags);
    }

    operator bool () const { return res.isTrue(); }

    ~LockChild()
    {
        if (res.hasBool())
            ImGui::EndChild();
    }
}; // struct LockChild
//////////////////////////////////////////////////////////////////////////


struct LockMenu {
    qd::Tribool res;

    LockMenu(const char* label, bool enabled = true) { res = (qd::Tribool)ImGui::BeginMenu(label, enabled); }

    operator bool () const { return res.isTrue(); }

    ~LockMenu()
    {
        if (res.isTrue())
            ImGui::EndMenu();
    }
}; // struct BeginMenu
//////////////////////////////////////////////////////////////////////////


bool menuItem(const char* label, const char* shortcut = nullptr, bool selected = false, bool enabled = true);

struct MenuItem {
    qd::Tribool res;

    MenuItem(const char* label, const char* shortcut = nullptr, bool selected = false, bool enabled = true)
    {
        res = (qd::Tribool)ImGui::MenuItem(label, shortcut, selected, enabled);
    }

    operator bool () const { return res.isTrue(); }

}; // struct MenuItem
//////////////////////////////////////////////////////////////////////////



}; // namespace qIm
