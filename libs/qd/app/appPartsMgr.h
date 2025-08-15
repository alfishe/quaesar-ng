#include "qd/app/moduleManager.h"
#include <qd/stl/vector.h>
#include <qd/app/appPart.h>
#include <qd/base/base.h>
#include <qd/stl/ref_ptr.h>
#include <qd/base/baseTypes.h>


FORWARD_DECLARATION_3(qd, ImAPI, CImGuiBase);
FORWARD_DECLARATION_3S(qd, appMsg, BaseMsg);
union SDL_Event;

namespace qd {

class AppPart;


class AppPartsManager : public IModuleInterface
{
    TS_REFLECT_CLASS(qd::AppPartsManager, qd::IModuleInterface);

private:
    qd::vector<ref_ptr<AppPart>> m_pParts;
    Application* m_pApp = nullptr;
    TTime64 m_timeNowFrame = 0;

public:
    AppPartsManager(const qd::ModuleCreateParams& cp)
        : TSuper(cp)
        , m_pApp(cp.app)
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


    // TPartClass base of AppPart*
    template<class TPartClass>
    inline TPartClass* findPart_() const
    {
        const string& staticPartIDStr = TPartClass::StaticClassNameID();
        assert(staticPartIDStr);
        ptr<TPartClass> pExistPart = findPartByName(staticPartIDStr);
        return pExistPart;
    }

    AppPart* getPartByInd(int Index) { return m_pParts[Index]; }



    // TPartClass base of AppPart*
    template<class TPartClass, typename ...TArgs>
    inline TPartClass* createPart_(qd::string name, TArgs&&... args)
    {
        TPartClass* pPart = new TPartClass(args...);
        qd::AppPart::OnCreate_t prm;
        prm.name = name;
        prm.typeInfo = &qd::typeof_<TPartClass>();
        prm.app = getApp();
        addPart(pPart);
        pPart->onPartCreate(prm);
        return pPart;
    }

    AppPart* findPartByName(const qd::string& strPartID) const;


    int findPartIndex(AppPart* pPart) const;
    bool addPartTry(ref_ptr<AppPart> pPart);
    void addPart(ref_ptr<AppPart> p_part);
    void addPart(ref_ptr<AppPart> p_part, const qd::string& part_name_id);

    template<class TPart>
    void addPart_(TPart* pPart)
    {
        addPart(pPart, pPart->StaticClassNameID());
    }

    template<class TPartClass>
    void destroyPart_()
    {
        ref_ptr<AppPart> pPart = findPart_<TPartClass>();
        destroyPart(pPart);
    }

    void destroyPart(ref_ptr<AppPart> pPart);
    void destroy();
    virtual ~AppPartsManager();

    void update(float dt, float time);
    void render();
    qd::EFlow onSdlEventProc(SDL_Event& event);

    Application* getApp() const { return m_pApp; }
    void setApp(Application* pApplication) { m_pApp = pApplication; }

    static inline bool _getZOrderSort(AppPart* pl, AppPart* pr)
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
