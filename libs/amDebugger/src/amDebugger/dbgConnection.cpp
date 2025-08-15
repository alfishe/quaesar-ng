#include "dbgConnection.h"
#include "amDebugger/generic/lockFreeQueue.h"


namespace amD
{

	class DbgSharedConnectionImp : public IDbgConnection
	{
        struct OpBuf
        {
            uint8_t m_buf[operation::MAX_OP_SIZE];
        };
        LockFreeQueueCpp11<OpBuf> m_memPool;

	public:
        DbgSharedConnectionImp()
            : m_memPool(2048)
        {
        }

        struct OpAllocator : public qd::operation::args::IOpArgAllocator
        {
            OpBuf* m_pBuf = nullptr;
            OpAllocator(OpBuf* pBuf) : m_pBuf(pBuf) {}

            virtual void* alloc(size_t mem_size) override
            {
                assert(mem_size < operation::MAX_OP_SIZE);
                return m_pBuf->m_buf;
            }

            virtual void dealloc(void* pPtr) override
            {
            }
        }; // struct OpAllocator


	    virtual void pushOperation(amD::operation::OperationArgs* pNewOp)
        {
            OpBuf* pOpBuf = m_memPool.push_back();
            OpAllocator alloc(pOpBuf);
            pNewOp->clone(alloc);
        }

        virtual amD::operation::OperationArgs* getFrontOperation() override
        {
            OpBuf* pOpBuf = m_memPool.front();
            if (!pOpBuf)
                return nullptr;
            auto pOp = reinterpret_cast<amD::operation::OperationArgs*>(&pOpBuf->m_buf[0]);
            return pOp;
        }

        virtual void popFrontOperation()
        {
            m_memPool.pop_front();
        }

	}; // class DbgSharedConnectionImp
    //////////////////////////////////////////////////////////////////////////


    ref_ptr<amD::IDbgConnection> create_shared_connection(const char* name)
    {
        ref_ptr<DbgSharedConnectionImp> pInst;
        pInst.makeSelf();
        pInst->m_name = name;
        return pInst;
    }

    ref_ptr<amD::IDbgConnection> create_dummy_connection()
    {
        ref_ptr<DbgSharedConnectionImp> pInst;
        pInst.makeSelf();
        pInst->m_name = "dummy connection";
        return pInst;
    }


}; // namespace amD
