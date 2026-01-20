#pragma once
#include "qd/app/moduleManager.h"
#include <qd/stl/vector.h>
#include <qd/app/applicationPart.h>
#include <qd/base/base.h>
#include <qd/stl/ref_ptr.h>
#include <qd/base/baseTypes.h>
#include "qd/debug/exception.h"


FORWARD_DECLARATION_3(qd, ImAPI, CImGuiBase);
FORWARD_DECLARATION_3S(qd, appMsg, BaseMsg);
union SDL_Event;

namespace qd {

class ApplicationPart;


class AppPartsManager : public qd::IModuleInterface
{
    TS_REFLECT_CLASS(qd::AppPartsManager, qd::IModuleInterface);

private:
    qd::vector<ref_ptr<ApplicationPart>> m_pParts;
    Application* m_pApp = nullptr;
    TTime64 m_timeNowFrame = 0;

public:
    AppPartsManager(const qd::ModuleCreateParams& cp)
        : TSuper(cp)
        , m_pApp(cp.app)
    {}

    inline int getNumAppParts() const { return (int)m_pParts.size(); }

    void sendAppEventMsg(qd::appMsg::BaseMsg& in_msg);


#if 0 // TODO
    template<class TPartClass>
    inline TPartClass* getPart_()
    {
        const string& staticPartIDStr = TPartClass::getStaticTypeInfo();
        ptr<TPartClass> pPart = findPartByName(staticPartIDStr);
        if (!pPart)
            throw Exception(EException::NOT_FOUND, "ApplicationPart:'%s' not initialized yet!", CC(staticPartIDStr));
        return pPart;
    }

    // TPartClass base of ApplicationPart*
    template<class TPartClass>
    inline TPartClass* findPart_() const
    {
        const string& staticPartIDStr = TPartClass::getStaticTypeInfo();
        assert(staticPartIDStr);
        ptr<TPartClass> pExistPart = findPartByName(staticPartIDStr);
        return pExistPart;
    }
    template<class TPart>
    void addPart_(TPart* pPart)
    {
        addPart(pPart, pPart->StaticClassNameID());
    }

    template<class TPartClass>
    void destroyPart_()
    {
        ref_ptr<ApplicationPart> pPart = findPart_<TPartClass>();
        destroyPart(pPart);
    }
#endif //

    ApplicationPart* getPartByInd(int Index) { return m_pParts[Index]; }


    // TPartClass base of ApplicationPart*
    template<class TAppPartClass, typename ...TArgs>
    inline TAppPartClass* createPart_(qtd::string name, TArgs&&... args)
    {
        qd::ApplicationPart::OnCreate_t prm;
        prm.name = name;
        prm.typeInfo = &qd::typeof_<TAppPartClass>();
        prm.app = getApp();

        TAppPartClass* pNewInst = new TAppPartClass(args...);
        qd::ApplicationPart* pBasePtr(pNewInst);
        pBasePtr->onPartCreate(prm);
        addPart(pBasePtr);
        return pNewInst;
    }

    ApplicationPart* findPartByName(const qtd::string& strPartID) const;


    int findPartIndex(ApplicationPart* pPart) const;
    bool addPartTry(ref_ptr<ApplicationPart> pPart);
    void addPart(ref_ptr<ApplicationPart> p_part);
    void addPart(ref_ptr<ApplicationPart> p_part, const qtd::string& part_name_id);

    void destroyPart(ref_ptr<ApplicationPart> pPart);
    void destroy();
    virtual ~AppPartsManager() override;

    void update(float dt, float time);
    void render();
    qd::EFlow onSdlEventProc(SDL_Event& event);

    Application* getApp() const { return m_pApp; }
    void setApp(Application* pApplication) { m_pApp = pApplication; }

    static inline bool _getZOrderSort(ApplicationPart* pl, ApplicationPart* pr)
    {
        if (pl->getZOrder() < pr->getZOrder())
            return true;
        return false;
    }

    TTime64 getTimeNowFrame() const { return m_timeNowFrame; }
    virtual void onModuleMessageProc(qd::moduleMsg::BaseMsg& in_msg) override;
    void _onImGuiDebugControl(ImAPI::CImGuiBase& im);

}; // class AppPartsManager
//////////////////////////////////////////////////////////////////////////


}; // namespace qd
