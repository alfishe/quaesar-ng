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
class BaseOptionsDlg;

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


struct OptionDrawCtx {
    IVm::VM* vm = nullptr;
    BaseOptionsDlg* m_pDlg = nullptr;
};


struct UOption {
public:
    qd::string title;
    UCategory* parentCat = nullptr;
    using TDrawOptionCb = eastl::fixed_function<2 * sizeof(void*), void(OptionDrawCtx*)>;
    TDrawOptionCb drawOptionCb;

    UOption(const char* title) : title(title) {
    }

    UOption& setDrawCb(UOption::TDrawOptionCb&& cb) {
        drawOptionCb = std::move(cb);
        return *this;
    }
};  // struct UOption
//////////////////////////////////////////////////////////////////////////


struct UCategory {
public:
    EOptionCat id;
    EOptionCat parentId;
    qd::string title;
    UCategory* parentCat = nullptr;
    qd::vector<UCategory*> childCats;
    qd::vector<UOption*> options;
    int ident = 0;

    UCategory(EOptionCat id, EOptionCat parent_id) : id(id), parentId(parent_id) {
    }
};


//////////////////////////////////////////////////////////////////////////
class BaseOptionsDlg : public qd::UiDialog {
    TS_REFLECT_CLASS(BaseOptionsDlg, qd::UiDialog);

    qd::array<qd::unique_ptr<UCategory>, EOptionCat::MAX_COUNT> m_pCategories = {};
    qd::vector<qd::unique_ptr<UOption>> m_pOptions;
    UCategory* m_pSelectedCat = nullptr;
    IVm::VM* m_pVm = nullptr;

public:
    virtual void onUiNodeCreated(qd::UiNodeCreator* mk) override {
        TSuper::onUiNodeCreated(mk);
    }
    virtual void drawContentImp() override;
    virtual EFlow onUiNodeMessageProc(qd::UiMessage* in_msg) override;

    UCategory* getCategoryById(EOptionCat nOpt) const {
        return m_pCategories[nOpt].get();
    }

    UCategory* createCategory(EOptionCat nOpt, EOptionCat nParentCat);

    template <typename... TArgs>
    UOption* createOption(UCategory* pCategory, TArgs&&... args) {
        assert(pCategory);
        UOption* pOpt = new UOption(args...);
        m_pOptions.push_back(qd::unique_ptr<UOption>(pOpt));
        pOpt->parentCat = pCategory;
        pCategory->options.push_back(pOpt);
        return pOpt;
    }

    virtual ~BaseOptionsDlg() override {
    }

    IVm::VM* getVm() const {
        return m_pVm;
    }
    void setVm(IVm::VM* Vm) {
        m_pVm = Vm;
    }

protected:
    void drawOptionContent(OptionDrawCtx* ctx, UOption* pOption);

};  // class BaseOptionsDlg
//////////////////////////////////////////////////////////////////////////


class UaeOptionsDlg : public qsr::BaseOptionsDlg {
    TS_REFLECT_CLASS(UaeOptionsDlg, qsr::BaseOptionsDlg);

public:
    virtual void onUiNodeCreated(qd::UiNodeCreator* mk) override;
};


extern void open_file_dlg_select_adf(IVm::Floppy& cfgFloppy, SDL_Window* pParentWnd = nullptr);

};  // namespace qsr
