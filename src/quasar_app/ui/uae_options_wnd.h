#pragma once
#include "qd/stl/unique_ptr.h"
#include "qd/stl/vector.h"
#include "qd/ui/controls/dialog.h"

struct UCategory;


struct UOption {
public:
    qd::string m_title;
    UCategory* m_pCategory = nullptr;

    UOption(const char* title) : m_title(title) {
    }
};


struct UCategory {
public:
    qd::string m_title;
    int m_ident = 0;
    UCategory* m_pParentCat = nullptr;
    qd::vector<UOption*> m_pOptions;

    UCategory(const char* title) : m_title(title) {
    }
};


//////////////////////////////////////////////////////////////////////////
class UaeOptionsDlg : public qd::UiDialog {
    TS_REFLECT_CLASS(UaeOptionsDlg, qd::UiDialog);

    qd::vector<qd::unique_ptr<UCategory>> m_pCategories;
    qd::vector<qd::unique_ptr<UOption>> m_pOptions;

    UCategory* m_pSelectedCat = nullptr;

public:
    virtual void onNodeCreated(qd::NodeCreator* mk) override;
    virtual void drawContentImp() override;

    template <typename... TArgs>
    UCategory* createCategory(UCategory* pParent, TArgs&&... args) {
        UCategory* pCategory = new UCategory(args...);
        int nIdent = 0;
        if (pParent)
            nIdent = pParent->m_ident + 1;
        pCategory->m_ident = nIdent;
        m_pCategories.push_back(qd::unique_ptr<UCategory>(pCategory));
        return pCategory;
    }

    template <typename... TArgs>
    UOption* createOption(UCategory* pCategory, TArgs&&... args) {
        UOption* pOpt = new UOption(args...);
        m_pOptions.push_back(qd::unique_ptr<UOption>(pOpt));

        assert(pCategory);
        pOpt->m_pCategory = pCategory;
        return pOpt;
    }

    virtual ~UaeOptionsDlg();

};  // class UaeOptionsDlg
