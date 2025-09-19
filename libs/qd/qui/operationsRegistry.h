#pragma once
#include "qd/debug/assert.h"
#include "qd/stl/string.h"
#include "qd/stl/vector.h"
#include "qd/stl/vector_map.h"
#include "qd/qui/shortcutHnd.h"
#include "qd/stl/span.h"


namespace qd {
class UiOperation;
struct UiOperationCreator;
class IOperationEnvironment;
FORWARD_DECLARATION_2S(operation, BaseOpArgs);


namespace operation {

class OpDesc
{
public:
    ShortcutsHnd* m_pShortcuts = nullptr;
    qd::string m_id;
    qd::string m_name;
    qd::operation::BaseOpArgs* m_pOpTemplate = nullptr; // owner

    OpDesc() = default;
    OpDesc(OpDesc&& rh) noexcept
        : m_pShortcuts(rh.m_pShortcuts)
        , m_id(std::move(rh.m_id))
        , m_name(std::move(rh.m_name))
        , m_pOpTemplate(rh.m_pOpTemplate)
    {
        rh.m_pShortcuts = nullptr;
        rh.m_pOpTemplate = nullptr;
    }

    void addShortcut(uint32_t sid);
    const char* getShortcutGuiStr() const;
    ~OpDesc();

}; // class OpDesc

}; // namespace operation



class OperationsRegistry : public qd::RefCounted
{
    friend class OperationMgrOperationsListImp;
    //qd::vector<ref_ptr<UiOperation>> m_pOperations;
    using TOpList = qd::vector<ref_ptr<UiOperation>>;
    qd::vector_map<const qd::TypeInfo*, qd::UiOperation*> m_operationByOperationTypeMap;
    // qd::vector_map<const qd::TypeInfo*, qd::vector<qd::UiOperation*>> m_operationsByMsgTypeMap;

    qd::vector<qd::operation::OpDesc> m_OpDescList;
    qd::vector_map<THash32, uint32_t /*OpDescIndex*/> m_opsCidToDescIdx;

    bool mInit = false;

public:
    OperationsRegistry();
    virtual ~OperationsRegistry() override;

    static OperationsRegistry& get();

    void createOperations(qd::UiOperationCreator* ca);
    virtual void destroy();


    qd::span<qd::operation::OpDesc const> getOperationsList() const;

    template<typename TClass>
    TClass* getOperation_() const
    {
        qd::UiOperation* pOp = findOperationByType(TClass::getStaticTypeInfo());
        return static_cast<TClass*>(pOp);
    }

    const qd::operation::OpDesc* findOpDesc(THash32 cid) const
    {
        auto it = m_opsCidToDescIdx.find(cid);
        if (it == m_opsCidToDescIdx.end())
            return nullptr;
        uint32_t descIdx = it->second;
        return &m_OpDescList[descIdx];
    }

    template<typename TOpArg>
    const qd::operation::OpDesc& getOpDesc_(TOpArg* = nullptr) const
    {
        //const qd::TypeInfo& ti = TOpArg::getStaticTypeInfo();
        const qd::operation::OpDesc* pDesc = findOpDesc(TOpArg::CID);
        assert(pDesc);
        return *pDesc;
    }

    template<typename TClass>
    void regOperationDesc_()
    {
        const qd::TypeInfo& ti = TClass::getStaticTypeInfo();
        qd::operation::OpDesc desc;
        desc.m_pOpTemplate = new TClass();
        TClass::setup(desc);
        addOperationDesc(ti, std::move(desc));
    }

    void addOperationDesc(const qd::TypeInfo& ti, qd::operation::OpDesc&& desc);

    void testOperationsShortcuts(qd::IOperationEnvironment* pEnv, qd::span<qd::operation::OpDesc* const> opDescs);

    template<typename ...TOpClass>
    void testOperationsShortcuts_(qd::IOperationEnvironment* pEnv)
    {
        const qd::operation::OpDesc* opDescs[] = {(&getOpDesc_<TOpClass>())...};
        qd::span<qd::operation::OpDesc* const> spanOpDescs((qd::operation::OpDesc**)opDescs,
            EAArrayCount(opDescs));
        testOperationsShortcuts(pEnv, spanOpDescs);
    }

}; ///////////////////////////////////////////////////////////////




}; // namespace qd
