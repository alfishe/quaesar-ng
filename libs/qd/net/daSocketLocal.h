#pragma once
#include "qd/base/base.h"
#include "daSocket.h"
#include "qd/stl/vector.h"
#include "qd/stl/vector_map.h"
#include "qd/thread/mutex.h"
#include "qd/stl/ref_ptr.h"
#include "qd/mem/memBuffer.h"
#include "qd/mem/memAlloc.h"
#include "qd/base/baseTypes.h"
#include "qd/base/eTrisult.h"
#include <EASTL/fixed_function.h>
#include "qd/math/fixedPoint.h"
#include "qd/base/eFlow.h"



namespace FTCommands {
class CBaseComand : public qd::RefCounted
{
    CBaseComand* m_pParent = nullptr;
    bool m_bIsInit;
    bool m_bIsDone;

public:
    CBaseComand(CBaseComand* pParent = nullptr)
    {
        if (pParent)
            SetParent(pParent);
    }
    virtual void Initialize() {}
    virtual EFlow Update(qd::Fixed32 Delta) { return EFlow::DONE; }
    virtual void done() {}
    virtual void destroy() {}

    inline CBaseComand* GetParent() const { return m_pParent; }
    void SetParent(CBaseComand* pParent)
    {
        if (pParent == m_pParent)
            return;
        m_pParent = pParent;
    }
    inline bool IsInit() const { return m_bIsInit; }
    void SetIsInit(bool bIsInit) { m_bIsInit = bIsInit; }
    inline bool IsDone() const { return m_bIsDone != 0; }
    void SetIsDone(bool IsDone) { m_bIsDone = IsDone; }
};

inline void InvokeCmdDone(CBaseComand* pCurCom)
{
    if (pCurCom && pCurCom->IsInit() && !pCurCom->IsDone())
    {
        pCurCom->SetIsDone(true);
        pCurCom->SetIsInit(false);
        pCurCom->done();
    }
}

//////////////////////////////////////////////////////////////////////////
class CCommandList : public FTCommands::CBaseComand
{
    typedef FTCommands::CBaseComand TSuper;
    typedef CCommandList TThis;

protected:
    qd::vector< ref_ptr<CBaseComand> > m_pCommands;
    typedef qd::vector< ref_ptr<CBaseComand> >::iterator CmdIter;

public:
    inline int getNumCommands() { return (int)m_pCommands.size(); }

    inline CBaseComand* GetCommand(int Index)
    {
        assert(Index >= 0 && Index < getNumCommands());
        return m_pCommands[Index];
    }
    template<class TPtr>
    inline TPtr* GetCommand(int Index)
    {
        ptr<TPtr> pCmd = TThis::GetCommand(Index);
        return pCmd;
    }

    inline CBaseComand* GetLastCmd() { return m_pCommands.back(); }

    CCommandList(FTCommands::CBaseComand* pParent = nullptr)
        : TSuper(pParent)
    {}

    virtual void Initialize() override
    {
        for (CmdIter Iter = m_pCommands.begin(); Iter != m_pCommands.end(); ++Iter)
        {
            CBaseComand* pChildCmd = *Iter;
            // InvokeCmdInitialize(pChildCmd);
        }
    }

    // CONTINUE ???
    EFlow Update(qd::Fixed32 Delta) override { return EFlow::UNDEF; }

    virtual void done() override {}

    virtual void destroy() override {}

    FTCommands::CBaseComand* AddCmd(const ref_ptr<FTCommands::CBaseComand>& pCommand)
    {
        assert(pCommand);
        pCommand->SetParent(this);
        m_pCommands.push_back(pCommand);
        return pCommand;
    }

    template<class TPattern>
    TPattern* Add_()
    {
        ref_ptr<TPattern> pPtr = new TPattern();
        AddCmd(pPtr);
        return pPtr;
    }

    int FindCommandIndex(CBaseComand* pCommand)
    {
        for (CmdIter It = m_pCommands.begin(); It != m_pCommands.end(); ++It)
        {
            if (*It == pCommand)
                return (int)(It - m_pCommands.begin());
        }
        return -1;
    }

    virtual int removeCommand(CBaseComand* pCommand)
    {
        int Ind = FindCommandIndex(pCommand);
        if (Ind < 0)
        {
            assert(0 && "Command not found to Delete!");
            return -1;
        }

        if (pCommand->IsInit() && !pCommand->IsDone())
            InvokeCmdDone(pCommand);

        SAFE_DESTROY(pCommand);

        m_pCommands.erase(m_pCommands.begin() + Ind);
        Ind--;
        return Ind;
    }

    void RemoveOldCommands()
    {
        int numCommands = getNumCommands();
        for (int i = 0; i < numCommands;)
        {
            CBaseComand* pCommand = GetCommand(i);
            if (!pCommand->IsDone())
            {
                ++i;
                continue;
            }

            removeCommand(pCommand);
            --numCommands;
        }
    }

}; // class CCommandList
//////////////////////////////////////////////////////////////////////////



class CCommandSequence : public FTCommands::CCommandList
{
    typedef FTCommands::CCommandList TSuper;

protected:
    int m_nCurIter;

    inline void MoveIter() { m_nCurIter++; }

    inline bool IsEndIter() { return m_nCurIter >= getNumCommands() || m_nCurIter < 0; }

    inline void SetBeginIter() { m_nCurIter = 0; }

public:
    inline CBaseComand* GetCurCmd()
    {
        assert(m_nCurIter >= 0 && m_nCurIter < getNumCommands());
        return m_pCommands[m_nCurIter];
    }

    inline CBaseComand* GetCurCmdPtr()
    {
        if (m_nCurIter >= 0 && m_nCurIter < getNumCommands())
            return m_pCommands[m_nCurIter];
        return nullptr;
    }

    int GetCurIter() const { return m_nCurIter; }


    void SetCurIter(int nCommand)
    {
        assert(nCommand == 0 || (nCommand >= -1 && nCommand < getNumCommands()));
        m_nCurIter = nCommand;
    }

public:
    CCommandSequence(FTCommands::CBaseComand* pParent = nullptr)
        : TSuper(pParent)
        , m_nCurIter(0)
    {}

    CCommandSequence(FTCommands::CBaseComand* pParent, CBaseComand* pCom1, CBaseComand* pCom2 = nullptr,
        CBaseComand* pCom3 = nullptr, CBaseComand* pCom4 = nullptr, CBaseComand* pCom5 = nullptr)
        : TSuper(pParent)
        , m_nCurIter(0)
    {
        if (pCom1)
            AddCmd(pCom1);
        if (pCom2)
            AddCmd(pCom2);
        if (pCom3)
            AddCmd(pCom3);
        if (pCom4)
            AddCmd(pCom4);
        if (pCom5)
            AddCmd(pCom5);
    }


    virtual void Initialize() override
    {
        if (getNumCommands() > 0)
            SetCurIter(0);
    }

    // CONTINUE ???
    EFlow Update(qd::Fixed32 Delta) override;


    void InsertCmd(FTCommands::CBaseComand* pCommand, int Index)
    {
        assert(pCommand);
        pCommand->SetParent(GetParent());
        assert(Index >= 0 && Index < (int)m_pCommands.size());
        m_pCommands.insert(m_pCommands.begin() + Index, pCommand);
    }

    virtual int removeCommand(CBaseComand* pCommand) override
    {
        int i = TSuper::removeCommand(pCommand);
        if (i < 0)
            return -1;

        int nCurIter = GetCurIter();
        if (nCurIter >= 1 && nCurIter >= i)
        {
            int nNewCmd = nCurIter - 1;
            SetCurIter(nNewCmd);
        }

        return i;
    }

}; // class CCommandSequence

}; // namespace FTCommands
//////////////////////////////////////////////////////////////////////////


FORWARD_DECLARATION_2(qd, CFTSocket);


namespace qd::DaSocket {
namespace LocalImp {
	FORWARD_DECLARATION_2(Cmd, ReadSocketBuffer);
	FORWARD_DECLARATION_2(Cmd, SendSocketBuffer);
	FORWARD_DECLARATION_2(Cmd, FlushSocketFuture_t);

	class CSocketLocalListner;
	class CSocketServiceLocal;
	class CSocketLocal;

	void _CancelAsyncSocketCommands(FTCommands::CCommandList* pCmdList, qd::CFTSocket* pSocket);

	struct HSocket : public qd::RefCounted
	{
		qd::Mutex m_Mutex;
		qd::MemData m_Buffer;
		ref_ptr<HSocket> m_pLink;
		uint16_t m_Port;

		HSocket( uint32_t nIndex )
			: m_Buffer( qd::MemAlloc::Kb(4) )
			, m_Port(0)
			, m_pLink(nullptr)
		{
		}

	}; // struct HSocket
	//////////////////////////////////////////////////////////////////////////


	//////////////////////////////////////////////////////////////////////////
	class CNetLocal
	{
		SINGLETON_DECLARE(CNetLocal);
		qd::Mutex m_Mutex;
		qd::vector< CSocketServiceLocal* > m_pServiceList;
		qd::vector< ref_ptr<HSocket> > m_pSockets;
		typedef qd::vector< ref_ptr<HSocket> > TSockets;

	public:
		struct Listner_t
		{
			uint16_t m_Port;
			eastl::fixed_function<16, void (HSocket*)> m_pOnConnectCB;
			ref_ptr<HSocket> m_pListenSocket;
		};

		qd::vector_map<uint16_t, CNetLocal::Listner_t> m_pListenPorts;
		typedef qd::vector_map<uint16_t, Listner_t> TPorts;


	public:
		CNetLocal();

		void RegisterService(CSocketServiceLocal* pService);

		void UnregisterService(CSocketServiceLocal* pService);

		ref_ptr<HSocket> SocketCreate();

		bool SocketConnect( const ref_ptr<HSocket>& hSocket, CInetAddr& Addr );

		void SocketClose(const ref_ptr<HSocket>& hSocket);

		int SocketSend( const ref_ptr<HSocket>& pClientSocket, void* pBuffer, uint32_t nSize );

		int SocketRecv( const ref_ptr<HSocket>& hSocket, void* pDestBuff, uint32_t nBufSize );

		void bindListenPort( const CNetLocal::Listner_t& PortListen );

		void UnBindListenPort(uint16_t Port);

	}; // class CNetLocal
	//////////////////////////////////////////////////////////////////////////



		//////////////////////////////////////////////////////////////////////////
		class CSocketServiceLocal : public CSocketService
		{
			typedef CSocketService super;
		public:
			CSocketLocalListner* m_pCreatedAcceptor = nullptr;
			qd::vector< wref_ptr<CSocketLocal> > m_pCreatedSockets;
			typedef qd::vector< wref_ptr<CSocketLocal> > TCreatedSockets;

			qd::Mutex m_CmdMutex;
			FTCommands::CCommandList* m_pReadCommands = nullptr;
            FTCommands::CCommandSequence* m_pSendCommands = nullptr;
			qd::ThreadEvent m_SendCmdEvent = {/*bAutoReset:*/true};
			volatile bool m_bThreadQuit = false;
			qd::Thread* m_pThread = nullptr;

			enum
			{
				READ_COMMANDS = 0,
				SEND_COMMANDS = 1,
			};

			void _threadProcLoop();

			void _doThreadedSocketCommands( qd::Fixed32 Delta );

		public:

			CSocketServiceLocal()
			{
				CNetLocal& pLocale = CNetLocal::get(); // CREATE INSTANCE WHILE MAIN THREAD
				G_UNUSED(pLocale);
			}


			bool isActive() const
			{
				if (!m_pThread)
					return false;
				if (!m_pReadCommands)
					return false;
				return true;
			}


			virtual void destroy() override;

			virtual ~CSocketServiceLocal();

			virtual void startAsync() override;

			virtual void startSync() override;

			virtual void stopImp() override;

			FTCommands::CBaseComand* AddReadCmd( Cmd::ReadSocketBuffer* pCmd );

			FTCommands::CBaseComand* AddSendCmd( Cmd::SendSocketBuffer* pCmd );

			ref_ptr<Cmd::FlushSocketFuture_t> AddFlushCmd( CSocketLocal* pSocket );

			void CancelSocketIO(CFTSocket* pSocket)
			{
				qd::MutexLock ml(m_CmdMutex);

				_CancelAsyncSocketCommands(m_pReadCommands, pSocket);
				_CancelAsyncSocketCommands(m_pSendCommands, pSocket);
			}


			virtual ref_ptr<CFTSocket> CreateSocket(DaSocket::CBaseDataProc* pDataProc) override;

			CSocketLocalListner* GetCreatedAcceptor() const { return m_pCreatedAcceptor; }

			virtual CListnerImpBase* CreateSocketListner( CFtTcpListner* pTcpListner ) override;

			virtual void destroySocket(CFTSocket* pSocket) override;


		}; // class CSocketServiceLocal
		//////////////////////////////////////////////////////////////////////////



		//////////////////////////////////////////////////////////////////////////
		class CSocketLocal : public qd::CFTSocket
		{
			typedef CFTSocket super;
			typedef CSocketLocal TThis;
			CSocketServiceLocal* m_pIOService;

		public:
			ref_ptr<HSocket> m_hSocket;

		public:

			CSocketLocal(CSocketServiceLocal* pIoService, DaSocket::CBaseDataProc* pDataProc)
				: super(pDataProc)
				, m_pIOService(pIoService)
				, m_hSocket(nullptr)
			{
			}


			virtual bool ConnectSync(const qd::string& pServerHost, unsigned short Port) override;

			virtual void Close() override;

			virtual void destroy() override;

			virtual void _asyncReadInternal( ref_ptr<qd::MemData> pReadBuf ) override;

			virtual void _asyncWriteInternal(ref_ptr<qd::MemData> pSendData) override;

			virtual void _syncWriteInternal(ref_ptr<qd::MemData> pSendData) override {
				_asyncWriteInternal(pSendData);
				FlushSocketSync();
			}
			void FlushSocketSync() override final;

			const ref_ptr<HSocket>& GetNative() const {
				return m_hSocket;
			}

			virtual ~CSocketLocal();

		}; // class CSocketLocal
		//////////////////////////////////////////////////////////////////////////



		//////////////////////////////////////////////////////////////////////////
		class CSocketLocalListner : public CListnerImpBase
		{
			typedef CSocketLocalListner TThis;
			typedef CListnerImpBase TSuper;

			ref_ptr<CSocketLocal> m_pServerSocket;

		public:
			CSocketLocalListner(CSocketService* pIOService, CFtTcpListner* pTcpListner)
				: TSuper(pIOService, pTcpListner)
			{
			}

			void _onClientConnected( HSocket* pDestSocket );

			virtual void WaitingStartASync( ref_ptr<CFTSocket> pIncomeSocket ) override;

			virtual void destroy() override
			{
				uint16_t Port = GetPort();
				CNetLocal::get().UnBindListenPort(Port);
			}

			virtual ~CSocketLocalListner()
			{
				TThis::destroy();
			}

		}; // class CSocketLocalListner
		//////////////////////////////////////////////////////////////////////////






}; // namespace LocalImp
}; // namespace FTSocket
