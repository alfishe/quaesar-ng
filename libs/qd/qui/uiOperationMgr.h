#pragma once
//#include "qd/qui/uiOperation.h"
#include "qd/debug/assert.h"
//#include "qd/node/node.h"
//#include "qd/stl/span.h"
#include "qd/stl/string.h"
#include "qd/stl/vector.h"
#include "qd/stl/vector_map.h"
#include "qd/qui/shortcutHnd.h"


namespace qd {
class UiOperation;
struct UiOperationCreator;
class IOperationEnvironment;
FORWARD_DECLARATION_3S(operation, args, Base);


namespace operation::args {

class OpDesc
{
public:
    ShortcutsHnd* m_pShortcuts = nullptr;
    qd::string m_id;
    qd::string m_name;

    OpDesc() = default;
    OpDesc(OpDesc&& other) noexcept
        : m_pShortcuts(other.m_pShortcuts)
        , m_id(std::move(other.m_id))
        , m_name(std::move(other.m_name))
    {
        other.m_pShortcuts = nullptr;
    }

    void addShortcut(uint32_t sid);
    void getShortcutGuiStr(qd::InlineString& out) const;

    ~OpDesc();

}; // class OpDesc

}; // namespace operation::args



class UiOperationMgr : public qd::RefCounted
{
    friend class OperationMgrOperationsListImp;
    qd::vector<ref_ptr<UiOperation>> m_pOperations;
    using TOpList = qd::vector<ref_ptr<UiOperation>>;
    qd::vector_map<const qd::TypeInfo*, qd::UiOperation*> m_operationByOperationTypeMap;
    // qd::vector_map<const qd::TypeInfo*, qd::vector<qd::UiOperation*>> m_operationsByMsgTypeMap;

    qd::vector<qd::operation::args::OpDesc> m_OpDescList;
    qd::vector_map<THash32, uint32_t /*OpDescIndex*/> m_opsCidToDescIdx;

    bool mInit = false;

public:
    UiOperationMgr();
    virtual ~UiOperationMgr() override;

    static UiOperationMgr& get();

    void createOperations(qd::UiOperationCreator* ca);
    virtual void destroy();

    int getNumOps() const { return (int)m_pOperations.size(); }

    void addOperation(UiOperation* pNewOperation);

    UiOperation* findOperation(uint32_t class_id) const;
    UiOperation* findOperationByType(const qd::TypeInfo& type) const;

    virtual eastl::span<UiOperation* const> getOperationsList() const;

    template<typename TClass>
    TClass* getOperation_() const
    {
        qd::UiOperation* pOp = findOperationByType(TClass::getStaticTypeInfo());
        return static_cast<TClass*>(pOp);
    }

    const qd::operation::args::OpDesc* findOpDesc(THash32 cid) const
    {
        auto it = m_opsCidToDescIdx.find(cid);
        if (it == m_opsCidToDescIdx.end())
            return nullptr;
        uint32_t descIdx = it->second;
        return &m_OpDescList[descIdx];
    }

    template<typename TOp>
    const qd::operation::args::OpDesc& getOpDesc_(TOp* = nullptr) const
    {
        //const qd::TypeInfo& ti = TOp::getStaticTypeInfo();
        const qd::operation::args::OpDesc* pDesc = findOpDesc(TOp::CID);
        assert(pDesc);
        return *pDesc;
    }

    template<typename TClass>
    void regOperationDesc_()
    {
        const qd::TypeInfo& ti = TClass::getStaticTypeInfo();
        qd::operation::args::OpDesc desc;
        TClass::setup(desc);
        addOperationDesc(ti, std::move(desc));
    }

    void addOperationDesc(const qd::TypeInfo& ti, qd::operation::args::OpDesc&& desc);


}; ///////////////////////////////////////////////////////////////




}; // namespace qd
