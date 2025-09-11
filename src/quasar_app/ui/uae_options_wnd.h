#pragma once
#include "EASTL/fixed_function.h"
#include "qd/enum/enumBase.h"
#include "qd/qui/controls/dialog.h"
#include "qd/stl/array.h"
#include "qd/stl/unique_ptr.h"
#include "qd/stl/vector.h"


FORWARD_DECLARATION_2(IVm, VM);
FORWARD_DECLARATION_2(IVm, Floppy);
struct SDL_Window;

namespace qsr {

struct UCategory;
class UaeOptionsDlg;

struct EOptionCat {
#define OPTIONS_LIST(IT)          \
    IT(UNDEF, "")                 \
    IT(ROOT, "")                  \
    IT(QUICK_START, "Quickstart") \
    IT(SOUND, "Sound")            \
    IT(HARDWARE, "Hardware")      \
    IT(CPU, "Cpu")                \
    IT(HOST, "Host")              \
    IT(FLOPPY, "Floppy drives")   \
    IT(ADVANCED, "Advanced")      \
    IT(MAX_COUNT, "")

    enum Enum { OPTIONS_LIST(ENUM_TAKE_ITEM_1) };
    inline static const char* g_enumNames[] = {OPTIONS_LIST(ENUM_TAKE_ITEM_2)};
    ENUM_DECLARE_BASE(qsr::, EOptionCat, Enum, 0);

    inline static const char* to_string(Enum value) {
        return g_enumNames[value];
    }
};  // struct EOptionCat
//////////////////////////////////////////////////////////////////////////


struct OptionDrawContext {
    IVm::VM* vm = nullptr;
    UaeOptionsDlg* m_pDlg = nullptr;
};

struct UOption {
public:
    qd::string m_title;
    UCategory* m_pCategory = nullptr;
    using TDrawOptionCb = eastl::fixed_function<2 * sizeof(void*), void(OptionDrawContext*)>;
    TDrawOptionCb m_drawOptionCb;

    UOption(const char* title) : m_title(title) {
    }

    UOption& setDrawCallback(UOption::TDrawOptionCb&& cb) {
        m_drawOptionCb = std::move(cb);
        return *this;
    }
};


struct UCategory {
public:
    EOptionCat m_id;
    EOptionCat m_parentId;
    int m_ident = 0;
    UCategory* m_pParentCat = nullptr;
    qd::vector<UCategory*> m_pChildCat;
    qd::vector<UOption*> m_pOptions;

    UCategory(EOptionCat id, EOptionCat parent_id) : m_id(id), m_parentId(parent_id) {
    }
};


//////////////////////////////////////////////////////////////////////////
class UaeOptionsDlg : public qd::UiDialog {
    TS_REFLECT_CLASS(UaeOptionsDlg, qd::UiDialog);

    qd::array<qd::unique_ptr<UCategory>, EOptionCat::MAX_COUNT> m_pCategories = {};
    qd::vector<qd::unique_ptr<UOption>> m_pOptions;
    UCategory* m_pSelectedCat = nullptr;
    IVm::VM* m_pVm = nullptr;

public:
    virtual void onNodeCreated(qd::UiNodeCreator* mk) override;
    virtual void drawContentImp() override;
    virtual EFlow onUiNodeMessageProc(qd::UiMessage* in_msg) override;

    UCategory* getCategoryById(EOptionCat nOpt) const {
        return m_pCategories[nOpt].get();
    }

    UCategory* createCategory(EOptionCat nParentCat, EOptionCat nOpt);

    template <typename... TArgs>
    UOption* createOption(UCategory* pCategory, TArgs&&... args) {
        assert(pCategory);
        UOption* pOpt = new UOption(args...);
        m_pOptions.push_back(qd::unique_ptr<UOption>(pOpt));
        pOpt->m_pCategory = pCategory;
        pCategory->m_pOptions.push_back(pOpt);
        return pOpt;
    }

    virtual ~UaeOptionsDlg();

    IVm::VM* getVm() const {
        return m_pVm;
    }
    void setVm(IVm::VM* Vm) {
        m_pVm = Vm;
    }
};  // class UaeOptionsDlg


extern void open_file_dlg_select_adf(IVm::Floppy& cfgFloppy, SDL_Window* pParentWnd = nullptr);

};  // namespace qsr
