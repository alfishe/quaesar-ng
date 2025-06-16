#include "console_wnd.h"
#include <amDebugger/debugger.h>
#include <qd/ImGui/imgui_eastl.h>
#include <qd/log/log.h>
#include <qd/thread/thread.h>


namespace amD {
namespace window {

class ConsoleLogWriter : public qd::ILogWriter {
    eastl::vector<qd::LogEntry> msgList;
    qd::Mutex mMutex;

public:
    struct EntriesList;
    // friend struct EntriesList;

public:
    EntriesList getEntriesList();

protected:
    virtual void addLogEntry(const qd::LogEntry& entry) override;

    virtual ~ConsoleLogWriter() = default;

};  // class ConsoleLogWriter
//////////////////////////////////////////////////////////////////////////


struct ConsoleLogWriter::EntriesList {
    ConsoleLogWriter* mOwner;
    qd::Mutex* mpMutex;

public:
    EntriesList(ConsoleLogWriter* in_owner) : mOwner(in_owner), mpMutex(&mOwner->mMutex) {
        mpMutex->lock();
    }

    EntriesList(EntriesList&& rh) : mOwner(rh.mOwner), mpMutex(eastl::move(rh.mpMutex)) {
        rh.mpMutex = nullptr;
    }
    const qd::LogEntry* begin() const {
        return &*(mOwner->msgList.begin());
    }

    const qd::LogEntry* end() const {
        return &*(mOwner->msgList.end());
    }

    ~EntriesList() {
        if (mpMutex) {
            mpMutex->unlock();
            mpMutex = nullptr;
        }
    }
};  // struct EnriesList
//////////////////////////////////////////////////////////////////////////


void ConsoleWnd::drawContentImp() {
    ImVec2 rgn = ImGui::GetContentRegionAvail();
    ImVec2 scrollingChildSize = ImVec2(rgn.x, rgn.y - ImGui::GetTextLineHeightWithSpacing());
    if (ImGui::BeginChild("##scrolling", scrollingChildSize, ImGuiChildFlags_None,
                          ImGuiWindowFlags_HorizontalScrollbar)) {
        ConsoleLogWriter::EntriesList list = mpConsoleWriter->getEntriesList();
        for (const qd::LogEntry& curEnt : list)
        {
            ImGui::TextUnformatted(curEnt.message.c_str());
        }
    }
    ImGui::EndChild();

    // constole input
    ImGui::TextUnformatted("> ");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    if (ImGui::InputText("##input", &inputStr, ImGuiInputTextFlags_EnterReturnsTrue)) {
        getDbg()->execConsoleCmd(inputStr.c_str());
        inputStr.clear();
    }
}


void ConsoleWnd::onCreate(UiViewCreateCtx* cp) {
    UiWindow::onCreate(cp);
    m_title = "Console";
    mpConsoleWriter = qd::logConsole().createWriter_<ConsoleLogWriter>();
}


void ConsoleWnd::destroy() {
    qd::logConsole().destroyWriter(mpConsoleWriter);
    mpConsoleWriter = nullptr;

    TSuper::destroy();
}


void ConsoleLogWriter::addLogEntry(const qd::LogEntry& entry) {
    mMutex.lock();
    msgList.push_back(entry);
    mMutex.unlock();
}


amD::window::ConsoleLogWriter::EntriesList ConsoleLogWriter::getEntriesList() {
    return EntriesList(this);
}


};  // namespace window
};  // namespace amD
