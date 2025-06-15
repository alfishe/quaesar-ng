#include "qd/app/moduleManager.h"
#include <qd/stl/vector.h>
#include <qd/app/appPart.h>
#include <qd/base/base.h>
#include <qd/base/ref_ptr.h>
#include <qd/base/types.h>


FORWARD_DECLARATION_3(qd, ImAPI, CImGuiBase);
FORWARD_DECLARATION_3S(qd, appMsg, BaseMsg);
union SDL_Event;

namespace qd {

class AppPartBase;


class AppPartsManager : public IModuleInterface
{
    typedef IModuleInterface TSuper;

private:
    qd::vector<ref_ptr<AppPartBase>> m_pParts;
    Application* m_pApp = nullptr;
    TTime64 m_TimeNowFrame = 0;

public:
    AppPartsManager(ModuleCreateParams* pCP = nullptr)
        : m_pApp(pCP->app)
    {}

    inline int getNumAppParts() const { return (int)m_pParts.size(); }

    void sendAppEventMsg(qd::appMsg::BaseMsg& in_msg);


    template<class TPartClass>
    inline TPartClass* getPart_()
    {
        const string& staticPartIDStr = TPartClass::StaticClassNameID();
        ptr<TPartClass> pPart = findPartByName(staticPartIDStr);
        if (!pPart)
            throw Exception(EException::NOT_FOUND, "AppPart:'%s' not initialized yet!", CC(staticPartIDStr));
        return pPart;
    }


    // TPartClass base of AppPartBase*
    template<class TPartClass>
    inline TPartClass* findPart_() const
    {
        const string& staticPartIDStr = TPartClass::StaticClassNameID();
        assert(staticPartIDStr);
        ptr<TPartClass> pExistPart = findPartByName(staticPartIDStr);
        return pExistPart;
    }

    AppPartBase* getPartByInd(int Index) { return m_pParts[Index]; }



    // TPartClass base of AppPartBase*
    template<class TPartClass>
    inline TPartClass* createPart_(qd::string name, bool bOverride = false)
    {
        TPartClass* pPart = new TPartClass();
        qd::AppPartBase::OnCreate_t prm;
        prm.name = name;
        prm.typeInfo = &qd::typeof_<TPartClass>();
        prm.app = getApp();
        addPart(pPart);
        pPart->onPartCreate(prm);
        return pPart;
    }

    AppPartBase* findPartByName(const qd::string& strPartID) const;


    int findPartIndex(AppPartBase* pPart) const;
    bool addPartTry(ref_ptr<AppPartBase> pPart);
    void addPart(ref_ptr<AppPartBase> p_part);
    void addPart(ref_ptr<AppPartBase> p_part, const qd::string& part_name_id);

    template<class TPart>
    void addPart_(TPart* pPart)
    {
        addPart(pPart, pPart->StaticClassNameID());
    }

    template<class TPartClass>
    void destroyPart_()
    {
        ref_ptr<AppPartBase> pPart = findPart_<TPartClass>();
        destroyPart(pPart);
    }

    void destroyPart(ref_ptr<AppPartBase> pPart);
    void destroy();

    virtual ~AppPartsManager();

    void update(float dt, float time);
    void render();
    void onSdlEventProc(SDL_Event& event);

    Application* getApp() const { return m_pApp; }
    void setApp(Application* pApplication) { m_pApp = pApplication; }

    static inline bool _getZOrderSort(AppPartBase* pl, AppPartBase* pr)
    {
        if (pl->getZOrder() < pr->getZOrder())
            return true;
        return false;
    }

    TTime64 getTimeNowFrame() const { return m_TimeNowFrame; }
    virtual void onModuleMessageProc(qd::moduleMsg::BaseMsg& in_msg) override;
    void _onImGuiDebugControl(ImAPI::CImGuiBase& im);

}; // class AppPartsManager
//////////////////////////////////////////////////////////////////////////


}; // namespace qd
