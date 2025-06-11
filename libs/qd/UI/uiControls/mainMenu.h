#pragma once
#include "imgui/imgui.h"
#include "qd/UI/uiNode.h"


namespace qd {
class TypeInfo;
class UiOperation;


class UiMainMenu : public qd::UiNode
{
    TS_REFLECT_CLASS(qd::UiMainMenu, qd::UiNode);

public:
    virtual EFlow onNodeMessageProc(qd::NodeMessage* in_msg) override { return EFlow::DONE; }

    virtual void drawContentImp() override { uinode_draw_child(this); }

    virtual void draw() override
    {
        if (ImGui::BeginMainMenuBar())
        {
            drawContentImp();
            ImGui::EndMainMenuBar();
        }
    }

}; // class
//////////////////////////////////////////////////////////////////////////



class UiMenu : public qd::UiNode
{
    TS_REFLECT_CLASS(qd::UiMenu, qd::UiNode);
    qd::string m_name;

public:
    virtual EFlow onNodeMessageProc(qd::NodeMessage* in_msg) override { return EFlow::DONE; }

    void setup(const qd::string& in_name) { m_name = in_name; }

    virtual void draw() override
    {
        if (ImGui::BeginMenu(m_name.c_str()))
        {
            drawContentImp();
            ImGui::EndMenu();
        }
    }

}; // class
//////////////////////////////////////////////////////////////////////////



class UiMenuItem : public qd::UiNode
{
    TS_REFLECT_CLASS(qd::UiMenuItem, qd::UiNode);
    qd::string m_name;

public:
    virtual EFlow onNodeMessageProc(qd::NodeMessage* in_msg) override { return EFlow::DONE; }

    void setup(const qd::string& in_name) { m_name = in_name; }

    virtual void draw() override
    {
        if (ImGui::BeginMenu(m_name.c_str()))
        {
            drawContentImp();
            ImGui::EndMenu();
        }
    }
}; // class


class UiSeparator : public qd::UiNode
{
    TS_REFLECT_CLASS(qd::UiSeparator, qd::UiNode);

public:
    virtual void draw() override { ImGui::Separator(); }
}; // class



class UiMenuOperation : public qd::UiNode
{
    TS_REFLECT_CLASS(qd::UiMenuOperation, qd::UiNode);
    const qd::TypeInfo* m_pOperationType = nullptr;
    qd::UiOperation* m_pOperation = nullptr;


public:
    void setup(const char* operation_class_name);

    virtual void draw() override;
}; // class



}; // namespace qd
