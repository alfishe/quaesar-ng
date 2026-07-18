#pragma once
#include "amDebugger/debuggerOps.h"
#include "qd/stl/string.h"
#include "qsr_application.h"

namespace qsr::operations {


struct ShowDebuggerWnd : public amD::operation::OperationArgs {
    DECLARE_OPERATION_1(qsr::operations::ShowDebuggerWnd);

    static void setup(qd::operation::OpDesc& d) {
        d.m_name = "Activate debugger";
        d.addShortcut(amD::shortcut::EId::ShowDebuggerWnd);
    }
};

struct ShowUaeOptionsWnd : public amD::operation::OperationArgs {
    DECLARE_OPERATION_1(qsr::operations::ShowUaeOptionsWnd);

    static void setup(qd::operation::OpDesc& d) {
        d.m_name = "Options...";
        d.addShortcut(amD::shortcut::EId::ShowUaeOptionsWnd);
    }
};


struct QuitQuasarApp : public amD::operation::OperationArgs {
    DECLARE_OPERATION_1(qsr::operations::QuitQuasarApp);

    static void setup(qd::operation::OpDesc& d) {
        d.m_name = "Quit";
    }
};


//------------------------------------------------------------------------
// Snapshot operations
//
// SaveSnapshot / LoadSnapshot carry a file path through the operations
// pipeline to the emulator thread. Each backend (UaeVmImp, VAmVmImp)
// handles the save/load using its native API.
//
struct SaveSnapshot : public amD::operation::OperationArgs {
    DECLARE_OPERATION_1(qsr::operations::SaveSnapshot);

    qtd::string path;  // destination file path for the snapshot

    static void setup(qd::operation::OpDesc& d) {
        d.m_name = "Save Snapshot";
    }
};

struct LoadSnapshot : public amD::operation::OperationArgs {
    DECLARE_OPERATION_1(qsr::operations::LoadSnapshot);

    qtd::string path;  // source file path for the snapshot

    static void setup(qd::operation::OpDesc& d) {
        d.m_name = "Load Snapshot";
    }
};


//------------------------------------------------------------------------
/// Check if a file is a snapshot by reading its magic bytes, then
/// falling back to extension matching.
/// Recognized: .uss (UAE), .vasnap (vAmiga)
bool isSnapshotFile(const std::string& path);

/// Returns the full path for the quicksave snapshot.
/// Uses <base_path>/data/snapshots/quicksave.uss, creating the
/// directory if needed. Falls back to "data/snapshots/quicksave.uss"
/// (relative to CWD) if SDL_GetBasePath() is unavailable.
std::string getQuickSavePath();

};  //namespace qsr::operations
