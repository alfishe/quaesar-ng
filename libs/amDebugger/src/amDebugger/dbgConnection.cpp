#include "dbgConnection.h"
#include "amDebugger/generic/lockFreeQueue.h"


namespace amD
{

	class DbgSharedConnectionImp : public IDbgConnection
	{
        struct OpBuf
        {
            uint8_t m_buf[192];
        };
        LockFreeQueueCpp11<OpBuf> m_memPool;

	public:
        DbgSharedConnectionImp()
            : m_memPool(2048)
        {
        }

	    virtual void pushOperation(amD::operation::OperationArgs* pNewOp)
        {
            OpBuf* buf = m_memPool.push();
        }
	    virtual amD::operation::OperationArgs* popOperation()
        {
            return nullptr;
        }
	};


}; // namespace amD
