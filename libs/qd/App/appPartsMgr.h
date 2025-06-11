#include <EASTL/vector.h>
#include <qd/Base/base.h>
#include <qd/Base/ref_ptr.h>
#include <qd/App/appPart.h>
#include "qd/App/moduleManager.h"
#include <qd/Base/types.h>


FORWARD_DECLARATION_3(qd, ImAPI, CImGuiBase);


namespace qd {

class BaseAppPart;


class AppPartsManager : public IModuleInterface {
    typedef IModuleInterface TSuper;

public:
    static ECgModuleID getModuleTypeId() {
        return ECgModuleID::APP_PARTS;
    };

private:
    eastl::vector<ref_ptr<BaseAppPart>> m_pParts;
    Application* m_pApp = nullptr;
    TTime64 m_TimeNowFrame = 0;

public:
    AppPartsManager(ModuleCreateParams* pCP = nullptr) : m_pApp(pCP->app) {
    }

    inline int getNumAppParts() const {
        return (int)m_pParts.size();
    }

    template <class TPartClass>
    inline TPartClass* getPart_() {
        const string& staticPartIDStr = TPartClass::StaticClassNameID();
        ptr<TPartClass> pPart = findPartByName(staticPartIDStr);
        if (!pPart)
            throw Exception(EException::NOT_FOUND, "AppPart:'%s' not initialized yet!", CC(staticPartIDStr));
        return pPart;
    }


    // TPartClass base of BaseAppPart*
    template <class TPartClass>
    inline TPartClass* findPart_() const {
        const string& staticPartIDStr = TPartClass::StaticClassNameID();
        assert(staticPartIDStr);
        ptr<TPartClass> pExistPart = findPartByName(staticPartIDStr);
        return pExistPart;
    }


    BaseAppPart* getPartByInd(int Index) {
        return m_pParts[Index];
    }


    BaseAppPart* createPart(ref_ptr<BaseAppPart> pPart, const string& staticPartIDStr, bool bOverride = false) {
        BaseAppPart* pExistPart = findPartByName(staticPartIDStr);
        if (pExistPart) {
            if (!bOverride)
                return pExistPart;
            else
                destroyPart(pExistPart);
        }
        pPart->setPartName(staticPartIDStr);
        addPart(pPart);
        return pPart;
    }


    // TPartClass base of BaseAppPart*
    template <class TPartClass>
    inline TPartClass* createPart_(bool bOverride = false) {
        const string& staticPartIDStr = TPartClass::StaticClassNameID();
        assert(staticPartIDStr);
        ptr<TPartClass> pExistPart = findPartByName(staticPartIDStr);
        if (pExistPart) {
            if (!bOverride)
                return pExistPart;
            else
                destroyPart(pExistPart);
        }

        ref_ptr<TPartClass> pPart = new TPartClass(getApp());
        pPart->setPartName(staticPartIDStr);
        addPart(pPart);
        return pPart;
    }


    BaseAppPart* findPartByName(const string& strPartID) const;


    int findPartIndex(BaseAppPart* pPart) const {
        int nSize = (int)m_pParts.size();
        for (int i = 0; i < nSize; ++i) {
            BaseAppPart* pCurPart = m_pParts[i];
            if (pCurPart == pPart)
                return i;
        }
        return -1;
    }

    bool addPartTry(ref_ptr<BaseAppPart> pPart);

    void addPart(ref_ptr<BaseAppPart> p_part);

    inline void addPart(ref_ptr<BaseAppPart> p_part, const string& part_name_id) {
        p_part->setPartName(part_name_id);
        addPart(p_part);
    }

    template <class TPart>
    void addPart_(TPart* pPart) {
        addPart(pPart, pPart->StaticClassNameID());
    }

    inline bool addPartTry(ref_ptr<BaseAppPart> pPart, const string& PartNameID) {
        pPart->setPartName(PartNameID);
        return addPartTry(pPart);
    }

    template <class TPart>
    bool addPartTry_(ref_ptr<TPart> pPart) {
        return addPartTry(pPart, TPart::StaticClassNameID());
    }


    template <class TPartClass>
    void destroyPart_() {
        ref_ptr<BaseAppPart> pPart = findPart_<TPartClass>();
        destroyPart(pPart);
    }

    void destroyPart(ref_ptr<BaseAppPart> pPart);

    void destroy();

    virtual ~AppPartsManager();

    void update(CFixed32 Delta, CFixed32 Time);

    void updateWhileLoading(CFixed32 Delta, CFixed32 Time);

    void render();

    Application* getApp() const {
        return m_pApp;
    }

    void setApp(Application* pApplication) {
        m_pApp = pApplication;
    }

    static inline bool _getZOrderSort(BaseAppPart* pl, BaseAppPart* pr) {
        if (pl->getZOrder() < pr->getZOrder())
            return true;
        return false;
    }

    TTime64 getTimeNowFrame() const {
        return m_TimeNowFrame;
    }

    virtual void onModuleMessageProc(Enm::EModuleMsg::Msg_t MsgId, void* pMsgData = nullptr) override;

    void _onImGuiDebugControl(ImAPI::CImGuiBase& im);

};  // class AppPartsManager
//////////////////////////////////////////////////////////////////////////


}; // namespace qd
