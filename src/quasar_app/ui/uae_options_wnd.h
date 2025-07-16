#pragma once
#include "qd/enum/enumBase.h"
#include "qd/qui/controls/dialog.h"
#include "qd/stl/array.h"
#include "qd/stl/unique_ptr.h"
#include "qd/stl/vector.h"

struct UCategory;


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
    ENUM_DECLARE_BASE(::, EOptionCat, Enum, 0);

    inline static const char* to_string(Enum value) {
        return g_enumNames[value];
    }
};  // struct EOptionCat
//////////////////////////////////////////////////////////////////////////


struct UOption {
public:
    qd::string m_title;
    UCategory* m_pCategory = nullptr;
    eastl::function<void()> m_drawCb;

    UOption(const char* title) : m_title(title) {
    }

    UOption& setDrawCallback(eastl::function<void()> cb) {
        m_drawCb = std::move(cb);
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

public:
    virtual void onNodeCreated(qd::UiNodeCreator* mk) override;
    virtual void drawContentImp() override;

    UCategory* getCategoryById(EOptionCat nOpt) const {
        return m_pCategories[nOpt].get();
    }

    UCategory* createCategory(EOptionCat nParentCat, EOptionCat nOpt);


    template <typename... TArgs>
    UOption* createOption(UCategory* pCategory, TArgs&&... args) {
        UOption* pOpt = new UOption(args...);
        m_pOptions.push_back(qd::unique_ptr<UOption>(pOpt));

        assert(pCategory);
        pOpt->m_pCategory = pCategory;
        pCategory->m_pOptions.push_back(pOpt);
        return pOpt;
    }

    virtual ~UaeOptionsDlg();

};  // class UaeOptionsDlg
