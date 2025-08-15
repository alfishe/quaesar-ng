#include "daSocketLocal.h"
#include "daSocket.h"
#include "qd/mem/ptrMath.h"
#include "qd/math/mathBase.h"
#include "qd/math/fixedPoint.h"
#include "qd/base/eFlow.h"
#include "qd/log/log.h"
#include "qd/thread/mutex.h"
#include "qd/thread/thread.h"



namespace qd::DaSocket
{
namespace LocalImp
{
	//////////////////////////////////////////////////////////////////////////

	namespace Cmd
	{


		class SocketBaseCmd : public FTCommands::CBaseComand
		{
			typedef FTCommands::CBaseComand super;
		protected:
			ref_ptr<CSocketLocal> m_pSocket;

			SocketBaseCmd( ref_ptr<CSocketLocal> pSocket )
				: m_pSocket(pSocket)
			{
			}

			virtual void destroy() override
			{
				m_pSocket = nullptr;
				super::destroy();
			}

		public:
			const ref_ptr<CSocketLocal>& GetSocket() const {
				return m_pSocket;
			}

			inline const ref_ptr<HSocket>& _hSocket() const {
				if ( !m_pSocket )
					return ref_ptr<HSocket>::NullPtr();
				return m_pSocket->m_hSocket;
			}

		}; // class SocketBaseCmd
		//////////////////////////////////////////////////////////////////////////



		//////////////////////////////////////////////////////////////////////////
		class SendSocketBuffer : public SocketBaseCmd
		{
			typedef SocketBaseCmd super;
			ref_ptr<qd::MemData> m_pBuf;
			uint32_t m_nSentCount;
			// ref_ptr<CSocketLocal> m_pSocket; - Derived
		public:

			SendSocketBuffer( ref_ptr<CSocketLocal> pSocket, ref_ptr<qd::MemData> pBuf )
				: super(pSocket)
				, m_pBuf( pBuf )
				, m_nSentCount(0)
			{
				assert( m_pBuf->getSize() );
			}

			virtual void Initialize() override
			{
			}

			// SEND
			virtual EFlow Update(qd::Fixed32 Delta) override
			{
				if ( !m_pSocket->IsConnected() ) {
					log_debug( "Socket:%X LocalSocket:%X HSocketSend SendSocketBuffer - Socket not Connected", ptr2DW( m_pSocket ), ptr2DW(_hSocket() ) );
					return EFlow::STOP;
				}

				const ref_ptr<HSocket>& hSocket = m_pSocket->GetNative();

				uint32_t nFullLen = m_pBuf->getSize();
				uint32_t nRemLen = nFullLen - m_nSentCount;
				if ( nRemLen == 0 )
				{
					assert(0 && "Nothing to send");
					m_pBuf = nullptr;
					return EFlow::STOP;
				}

				uint32_t Sent = CNetLocal::get().SocketSend( hSocket, m_pBuf->getBuffer(m_nSentCount), nRemLen );

				if ( Sent == ~0u )
				{
					if ( vlog_socket >= 1 )
                        log_info( "Socket:%X LocalSocket:%X HSocketSend Error: - Close connection", ptr2DW(m_pSocket), ptr2DW(_hSocket()) );
					m_pSocket->SetConnectState( CFTSocket::EConSt::CLOSED );
					return EFlow::STOP;
				}

				m_nSentCount += (uint32_t)Sent;

				if ( vlog_socket >= 2 )
                    log_info( "Socket:%X LocalSocket:%X HSocketSend - Send bytes: %u from %u", ptr2DW(m_pSocket), ptr2DW(_hSocket()), (uint32_t)Sent, nFullLen );

				if ( (uint32_t)Sent < nRemLen )
					return EFlow::REPEAT; // CONTINUE

				assert((uint32_t)Sent == nRemLen);
				m_pBuf = nullptr;
				return EFlow::STOP;
			}

			virtual void destroy() override
			{
				m_pSocket = nullptr;
				m_pBuf = nullptr;
				super::destroy();
			}

		}; // class SendSocketBuffer
		/////////////////////////////////////////////////////////////////////



		class ReadSocketBuffer : public SocketBaseCmd
		{
			typedef SocketBaseCmd super;
			ref_ptr<qd::MemData> m_pBuf;

		public:

			ReadSocketBuffer( ref_ptr<CSocketLocal> pSocket, ref_ptr<qd::MemData> pBuf )
				: super(pSocket)
				, m_pBuf(pBuf)
			{
			}

			virtual void Initialize() override
			{
			}


			// READ BYTES
			virtual EFlow Update(qd::Fixed32 Delta) override
			{
				const ref_ptr<HSocket>& hSocket = m_pSocket->GetNative();

				uint32_t nRemLen = m_pBuf->getCapacity() - m_pBuf->getSize();
				if ( nRemLen == 0 )
				{
					if ( vlog_socket >= 1 )
						log_debug( "Socket:%X LocalSocket:%X LocalReadSocket Command found - nullptr READ Buffer - STOP", ptr2DW(m_pSocket), ptr2DW(_hSocket()) );
					assert(0 && "Read Buffer Null");
					return EFlow::STOP;
				}

				int nRecived = CNetLocal::get().SocketRecv( hSocket, (char*)m_pBuf->getBuffer() + m_pBuf->getSize(), nRemLen );

				if ( nRecived < 0 ) {
					// CLOSE SOCKET
					m_pSocket->SetConnectState( CFTSocket::EConSt::CLOSED );
					if ( vlog_socket >= 1 )
						log_debug( "Socket:%X LocalSocket:%X - Socket Read not connected while SocketRecv. SetConnectState(CLOSED)", ptr2DW(m_pSocket), ptr2DW(_hSocket()) );
					return EFlow::STOP;
				}

				if ( nRecived == 0 )
				{
					// IF REMAIN SOME UNRECEIVED DATA
					if ( !m_pSocket->IsConnected() ) {
						if ( vlog_socket >= 1 )
							log_debug( "Socket:%X LocalSocket:%X LocalReadSocket Command found - Disconnected Socket - STOP", ptr2DW(m_pSocket), ptr2DW(_hSocket()) );
						return EFlow::STOP;
					}
					return EFlow::REPEAT;
				}

				if ( vlog_socket >= 2 )
					log_debug( "Socket:%X LocalSocket:%X LocalReadSocket Command Read received: %u bytes", ptr2DW(m_pSocket), ptr2DW(_hSocket()), (uint32_t)nRecived );

				m_pBuf->setSize( (int)m_pBuf->getSize() + nRecived );

				m_pSocket->_onDataBlockReadedLock(m_pBuf); // create NEW AsyncRead Command with new buffer
				m_pBuf = nullptr;

				FTCommands::CCommandList* pReadList = (FTCommands::CCommandList*)GetParent();
				if (pReadList->getNumCommands() == 0) {
					if ( vlog_socket >= 3 )
						log_debug( "WARNING: LocalSocket Read Command found - NO READ COMMANDS ADDED - STOP" );
					c_def(0);
				}

				return EFlow::STOP; // BUFFER FILLED - STOP
			}


			virtual void destroy() override
			{
				m_pSocket = nullptr;
				m_pBuf = nullptr;

				super::destroy();
			}

		}; // class ReadSocketBuffer
		//////////////////////////////////////////////////////////////////////////



		class FlushSocketFuture_t : public qd::RefCounted
		{
		public:
			volatile ETrisult::Type m_Result;
			qd::ThreadEvent m_Event;

			inline qd::ETrisult GetResult() const {
				return m_Result;
			}
			void SetResult( qd::ETrisult Result ) {
				m_Result = Result.get();
				m_Event.set();
			}

			ETrisult WaitResult()
			{
				log_debug( "LocalSocket.FlushSocketFuture_t - WaitResult()" );
				ETrisult res = m_Result;
				if ( res.hasResult() )
					return res;

				bool bEvent = m_Event.wait( 15 * 1000 );

				if ( !bEvent ) {
                    log_debug( "LocalSocket.FlushSocketFuture_t - TimeOut" );
				}
				ETrisult res2 = m_Result;
				return m_Result;
			}
		}; // class FlushSocketFuture_t
		//////////////////////////////////////////////////////////////////////////




		class CFlushSocketCmds : public SocketBaseCmd
		{
			typedef SocketBaseCmd super;
		public:

		private:
			ref_ptr<Cmd::FlushSocketFuture_t> m_pFuture;

		public:

			CFlushSocketCmds( ref_ptr<CSocketLocal> pSocket )
				: super(pSocket)
			{
				m_pFuture = new Cmd::FlushSocketFuture_t();
			}

			virtual EFlow Update(qd::Fixed32 Delta) override
			{
                log_debug( "LocalSocket::Cmd::FlushSocketFuture - SUCCESS" );
				m_pFuture->SetResult( ETrisult::SUCCESS );
				return EFlow::STOP; // CONTINUE
			}

			virtual void destroy() override
			{
				ETrisult res = m_pFuture->GetResult();
				// if were no update invoked
				if ( !res.hasResult() ) {
                    log_debug( "LocalSocket::Cmd::FlushSocketFuture - ERROR" );
					m_pFuture->SetResult( ETrisult::ERROR );
				}
				m_pFuture = nullptr;
			}

			const ref_ptr<Cmd::FlushSocketFuture_t>& GetFuture() const {
				return m_pFuture;
			}
		}; // class CFlushSocketCmds
		/////////////////////////////////////////////////////////////////////

	}; // namespace Cmd






	//////////////////////////////////////////////////////////////////////////



	CNetLocal::CNetLocal()
	{
		qd::MutexLock ml(m_Mutex);
		c_def(0);
	}


	ref_ptr<HSocket> CNetLocal::SocketCreate()
	{
		qd::MutexLock ml(m_Mutex);

		ref_ptr<HSocket> pSocket = new HSocket( (uint32_t)m_pSockets.size() );
		m_pSockets.push_back(pSocket);

		return pSocket;
	}



	void CNetLocal::SocketClose( const ref_ptr<HSocket>& pSocket )
	{
		qd::MutexLock ml(m_Mutex);

		// WE NEED TO GET KNOW ON OTHER SIDE ABOUT CLOSE CONNECTION

		ref_ptr<HSocket> pLink = pSocket->m_pLink;
		pSocket->m_pLink = nullptr;
		if  ( pLink ) {
			pLink->m_pLink = nullptr;
		}

		TSockets::iterator It = std::find( m_pSockets.begin(), m_pSockets.end(), pSocket );
		m_pSockets.erase(It);
	}







	int CNetLocal::SocketSend( const ref_ptr<HSocket>& pClientSocket, void* pBuffer, uint32_t nSize )
	{
		ref_ptr<HSocket> pLinkedSocket = pClientSocket->m_pLink; // THREADED

		if ( !pLinkedSocket )
			return -1; // CONNECTION CLOSED

		// AFFRAID OF DEAD-LOCKS
		qd::MutexLock mls(pLinkedSocket->m_Mutex); // MUTEX LINED BUFFER

		if ( !pLinkedSocket->m_pLink )
			return -1; // BROKEN CONNECTION

		pLinkedSocket->m_Buffer.write( pBuffer, nSize ); // WRITE TO DESTINATION BUFFER

		return nSize;
	}



	int CNetLocal::SocketRecv( const ref_ptr<HSocket>& pSocket, void* pDestBuff, uint32_t nBufSize )
	{
		// READ SOCKET
		qd::MutexLock mls( pSocket->m_Mutex ); // MUTEX BUFFER

		qd::MemData& srcBuf = pSocket->m_Buffer;

		qd::MemBuf Dest(pDestBuff, nBufSize, false);
		uint32_t nBytes = qd::min( srcBuf.getSize(), nBufSize );
		if ( nBytes == 0 ) {
			if ( !pSocket->m_pLink )
				return -1; // CONNECTION LOST
			return nBytes;
		}

		Dest.write( 0, srcBuf.getBuffer(), nBytes );
		srcBuf.popFront( nBytes );

		return nBytes;
	}



	bool CNetLocal::SocketConnect( const ref_ptr<HSocket>& pClientSocket, CInetAddr& Addr )
	{
		qd::MutexLock ml(m_Mutex);

		uint16_t Port = Addr.GetPort();

		TPorts::iterator It = m_pListenPorts.find(Port);
		if ( It == m_pListenPorts.end() ) {
			assert2( 0, "Port not binded yet" );
			return false;
		}

		Listner_t& PortListner = It->second;

		const ref_ptr<HSocket>& pServerSocket = PortListner.m_pListenSocket;
		if (!pServerSocket) {
			assert(0);
			return false;
		}

		// SET LINKS
		pServerSocket->m_pLink = pClientSocket;
		pClientSocket->m_pLink = pServerSocket;
		pServerSocket->m_Port = Port;
		pClientSocket->m_Port = Port;

		PortListner.m_pListenSocket = nullptr;
		PortListner.m_pOnConnectCB(pServerSocket);

		return true;
	}



	void CNetLocal::bindListenPort( const CNetLocal::Listner_t& ServerPortListen )
	{
		qd::MutexLock ml(m_Mutex);

		assert( ServerPortListen.m_Port );
		TPorts::iterator It = m_pListenPorts.find(ServerPortListen.m_Port);
 		if ( It != m_pListenPorts.end() && It->second.m_pListenSocket ) {
 			assert2( 0, "Port already used");
 			return;
 		}

		assert( ServerPortListen.m_pListenSocket );

		m_pListenPorts[ServerPortListen.m_Port] = ServerPortListen;
	}



	void CNetLocal::UnBindListenPort( uint16_t Port )
	{
		qd::MutexLock ml(m_Mutex);
		assert(Port);
		m_pListenPorts.erase(Port);
	}


    void CNetLocal::RegisterService(CSocketServiceLocal* pService)
    {
        qd::MutexLock ml(m_Mutex);
        if (std::find(m_pServiceList.begin(), m_pServiceList.end(), pService) != m_pServiceList.end())
            return;
        m_pServiceList.push_back(pService);
    }


    void CNetLocal::UnregisterService(CSocketServiceLocal* pService)
    {
        qd::MutexLock ml(m_Mutex);
        auto Iter = std::find(m_pServiceList.begin(), m_pServiceList.end(), pService);
        if (Iter == m_pServiceList.end())
            return;
        m_pServiceList.erase(Iter);
    }


	void CSocketServiceLocal::_threadProcLoop()
	{
		for (;;)
		{
			if ( m_bThreadQuit ) {
				log_debug( "LocalSocket::CSocketServiceLocal:: ThreadLoop - EXIT" );
				break;
			}

			{
				qd::MutexLock ml(m_CmdMutex); // MUTEXED
				_doThreadedSocketCommands( 0 );
			}
			try {
				m_SendCmdEvent.wait( 1000 / 10 ); // 10 times per second
				m_SendCmdEvent.reset();
				// Thread::Sleep( 1 / 10.0f );
			} catch(std::exception& e) {
				log_debug( "LocalSvocket::ThreadLoop EXCEPTION - Thread Event Exception:\"%s\"", e.what() );
			}
		}
		c_def(0);
	}


	void CSocketServiceLocal::_doThreadedSocketCommands( qd::Fixed32 Delta )
	{
		// SEND - TO SOCKET
		if ( m_pSendCommands->Update( Delta ) == EFlow::DONE )
		{
			m_pSendCommands->RemoveOldCommands();
			m_pSendCommands->SetCurIter( 0 );
		}

		// READ COMMANDS
		if ( m_pReadCommands->Update( Delta ) == EFlow::DONE ) {
			//CLog3::get()->PrintLn("LocalSocket - No ReadCommands" ); // - many records in log, but works
		}
		m_pReadCommands->RemoveOldCommands();
	}



	void CSocketServiceLocal::stopImp()
	{
		//CLog1::CSection cs( "CSocketServiceLocal::StopImp()" );

		m_bStarted = false;
		m_bThreadQuit = true;
		m_SendCmdEvent.set();

		if (m_pThread) {
			m_pThread->join();
			SAFE_DELETE(m_pThread);
		}

		qd::MutexLock ml(m_CmdMutex); // MUTEXED
		SAFE_DESTROY(m_pReadCommands);
		SAFE_DESTROY(m_pSendCommands);

		while( !m_pCreatedSockets.empty() )
		{
			wref_ptr<CSocketLocal> pSocket = m_pCreatedSockets.back();
			m_pCreatedSockets.pop_back();
			if (pSocket)
			{
				pSocket->destroy();
			}
		}

	}



	FTCommands::CBaseComand* CSocketServiceLocal::AddReadCmd( Cmd::ReadSocketBuffer* pCmd )
	{
		m_CmdMutex.lock();
		m_pReadCommands->AddCmd( pCmd );
		m_CmdMutex.unlock();

		m_SendCmdEvent.set();
		return pCmd;
	}



	FTCommands::CBaseComand* CSocketServiceLocal::AddSendCmd( Cmd::SendSocketBuffer* pCmd )
	{
		m_CmdMutex.lock();
		m_pSendCommands->AddCmd( pCmd );
		m_CmdMutex.unlock();

		m_SendCmdEvent.set();
		return pCmd;
	}


	ref_ptr<Cmd::FlushSocketFuture_t> CSocketServiceLocal::AddFlushCmd( CSocketLocal* pSocket )
	{
		qd::MutexLock ml( m_CmdMutex ); // COMMAND MUTEX

		if ( !isActive() )
			return nullptr;

		if ( m_pSendCommands->getNumCommands() == 0 )
			return nullptr;

		ref_ptr<Cmd::FlushSocketFuture_t> pFuture;

		Cmd::CFlushSocketCmds* pFlushMark = new Cmd::CFlushSocketCmds( pSocket );
		pFuture = pFlushMark->GetFuture();
		m_pSendCommands->AddCmd( pFlushMark );

		// FLUSH ALL SEND COMMANDS NOW
		do
		{
			_doThreadedSocketCommands( 0 );
			log_debug( "CSocketServiceLocal::AddFlushCmd - Wait Flushed send buffer" );

		} while ( !pFuture->GetResult().hasResult() );

		return nullptr;

		//return pFuture;
	}




	CListnerImpBase* CSocketServiceLocal::CreateSocketListner( CFtTcpListner* pTcpListner )
	{
		assert(!m_pCreatedAcceptor);
		m_pCreatedAcceptor = new CSocketLocalListner( this, pTcpListner );
		return m_pCreatedAcceptor;
	}


	ref_ptr<CFTSocket> CSocketServiceLocal::CreateSocket(DaSocket::CBaseDataProc* pDataProc)
	{
		ref_ptr<CSocketLocal> pSocket = new CSocketLocal( this, pDataProc );

		m_pCreatedSockets.push_back(pSocket);
		return pSocket;
	}



	void CSocketServiceLocal::destroy()
	{
		Stop();
		SAFE_DESTROY(m_pReadCommands);
		SAFE_DESTROY(m_pSendCommands);

		assert( m_pCreatedSockets.empty() );

		super::destroy();
	}



	CSocketServiceLocal::~CSocketServiceLocal()
	{
		//Destroy();
		assert(!m_pThread);
		assert(!m_pReadCommands);
		assert(!m_pSendCommands);
		assert( m_pCreatedSockets.empty() );

		m_pCreatedSockets.clear();

		CNetLocal::get().UnregisterService(this);
	}



	void CSocketServiceLocal::startAsync()
	{
		assert(!m_pThread);
		assert(!m_pReadCommands);
		m_pReadCommands = new FTCommands::CCommandList();
		m_pSendCommands = new FTCommands::CCommandSequence();

		m_pThread = new qd::Thread();
        m_pThread->create([this]() { this->_threadProcLoop(); });
		m_bStarted = true;
	}



	void CSocketServiceLocal::startSync()
	{
		assert(!m_pThread);
		assert(!m_pReadCommands);
		m_pReadCommands = new FTCommands::CCommandList();
		m_pSendCommands = new FTCommands::CCommandSequence();

		m_bStarted = true;

		_threadProcLoop(); // DO WORK while SIGNAL

		log_debug( "CSocketServiceLocal::StartSync() - EXIT" );
		c_def(0);
	}



	void CSocketServiceLocal::destroySocket( CFTSocket* pSocket )
	{
		TCreatedSockets::iterator It = std::find( m_pCreatedSockets.begin(), m_pCreatedSockets.end(), pSocket );
		if (It != m_pCreatedSockets.end())
			m_pCreatedSockets.erase(It);

		if (pSocket)
			pSocket->destroy();
	}




	void CSocketLocalListner::_onClientConnected( HSocket* pDestSocket )
	{
		ref_ptr<CSocketLocal> pSocket = m_pServerSocket;
		m_pServerSocket = nullptr;

		if ( m_pTcpListner) {
			m_pTcpListner->OnServerConnectionAcceptedCallback( pSocket, /*err:*/0 );
		}
	}



	void CSocketLocalListner::WaitingStartASync( ref_ptr<CFTSocket> pIncomeSocket )
	{
		assert(!m_pServerSocket);
		m_pServerSocket = pIncomeSocket;

		m_pServerSocket->m_hSocket = CNetLocal::get().SocketCreate(); // CREATE NATIVE SOCKET

		CNetLocal::Listner_t ls;
		ls.m_Port = GetPort();
		ls.m_pListenSocket = m_pServerSocket->GetNative();
        ls.m_pOnConnectCB = [this](HSocket* s) {
            this->_onClientConnected(s);
        };
		CNetLocal::get().bindListenPort(ls);

		c_def(0);
		// pTcpListner->OnServerConnectionAcceptedCallback(pIncomeSocket, 0);
	}




	bool CSocketLocal::ConnectSync( const qd::string& pServerHost, unsigned short Port )
	{
        CSocketServiceLocal* pIOServ = static_cast<CSocketServiceLocal*>(m_pIOService);
		qd::MutexLock ml( pIOServ->m_CmdMutex );

		m_ConnectAddr.Set(pServerHost, Port);

		if( m_hSocket || IsConnected() )
			Close();

		if ( vlog_socket >= 1 )
			log_info( "LocalSocket ConnectSync" );

		if (m_hSocket) {
			CNetLocal::get().SocketClose( m_hSocket );
			m_hSocket = nullptr;
		}

		m_hSocket = CNetLocal::get().SocketCreate();
		if ( !m_hSocket ) {
            log_info( "Can't create HSocketCreate: socketDomain:%X", ptr2DW(m_hSocket) );
			return false;
		}

		bool conRes = CNetLocal::get().SocketConnect( m_hSocket, m_ConnectAddr );

		if ( !conRes ) {
			return false;
		}
/*
		assert( !IsConnected() );

		if ( !CNetLocal::getSingleton()->ConnectToLocalServer(this, Port) ) {
			assert( 0 && "Not connected" );
			return false;
		}
*/
		OnSocketConnected();

		if ( vlog_socket >= 1 )
            log_info( "Socket:%X LocalSocket:%X - CONNECTED SUCCESS", ptr2DW( this ), ptr2DW(m_hSocket) );

		return true;
	}



	void CSocketLocal::Close()
	{
		super::Close();

		if (m_hSocket)
		{
			if ( vlog_socket )
                log_info( "Socket:%X LocalSocket:%X Close()...", ptr2DW(this), ptr2DW(m_hSocket) );
			CNetLocal::get().SocketClose(m_hSocket);
			m_hSocket = nullptr;
		}

		SetConnectState( CFTSocket::EConSt::CLOSED );
		SetInit(false);

		if (m_pIOService)
		{
            CSocketServiceLocal* pIOServ = static_cast<CSocketServiceLocal*>(m_pIOService);
			assert(pIOServ);
			pIOServ->CancelSocketIO( this );
		}

	}



	void CSocketLocal::destroy()
	{
		if ( vlog_socket )
			log_info( "Socket:%X LocalSocket:%X Destroy()...", ptr2DW( this ), ptr2DW(m_hSocket) );

		Close();

		if (m_pIOService)
		{
            CSocketServiceLocal* pIOServ = static_cast<CSocketServiceLocal*>(m_pIOService);
			m_pIOService = nullptr;
			pIOServ->destroySocket( this );
		}
		super::destroy();
	}



	void CSocketLocal::_asyncReadInternal( ref_ptr<qd::MemData> pReadBuf )
	{
		if ( !IsConnected() )
			throw CSocketException( CSocketException::CONNECTION_LOST, "Socket:%X is closed - can't Receive Data", ptr2DW( this ) );

        CSocketServiceLocal* pIOServ = static_cast<CSocketServiceLocal*>(m_pIOService);
		pIOServ->AddReadCmd( new Cmd::ReadSocketBuffer(this, pReadBuf) );
	}



	void CSocketLocal::_asyncWriteInternal(ref_ptr<qd::MemData> pSendData)
	{
		if ( !IsConnected() )
			throw CSocketException( CSocketException::CONNECTION_LOST, "Socket:%X is closed - can't Send Data", ptr2DW( this ) );

        CSocketServiceLocal* pIOServ = static_cast<CSocketServiceLocal*>(m_pIOService);
		pIOServ->AddSendCmd( new Cmd::SendSocketBuffer(this, pSendData) );
	}




	void CSocketLocal::FlushSocketSync()
	{
		if ( !IsConnected() )
			return;

		ref_ptr<Cmd::FlushSocketFuture_t> pFuture;

		CSocketServiceLocal* pIOServ = static_cast<CSocketServiceLocal*>(m_pIOService);
		pFuture = pIOServ->AddFlushCmd( this );
		if ( pFuture )
			pFuture->WaitResult(); // WAIT ASYNC SENDING ALL BUFFERS

		return;
	}



	CSocketLocal::~CSocketLocal()
	{
		destroy();
		c_def(0);
	}



	/*static*/
	void _CancelAsyncSocketCommands(FTCommands::CCommandList* pCmdList, CFTSocket* pSocket)
	{
		if ( !pCmdList )
			return;

		int numCommands = pCmdList->getNumCommands();

		CSocketLocal* pMSocket = (CSocketLocal*)pSocket;
		ref_ptr<HSocket> _hSocket = pMSocket ? pMSocket->GetNative() : nullptr;
		log_info( "Socket:%X LocalSocket:%X - LocalSocket.CancelAsyncSocketCommand - NumCommands:%i", ptr2DW( pMSocket ), ptr2DW( _hSocket ), numCommands );

		for ( int i = 0; i < numCommands; i ++ )
		{
			Cmd::SocketBaseCmd* pSocketCmd = (Cmd::SocketBaseCmd*)pCmdList->GetCommand(i);
			if ( pSocketCmd->GetSocket() == pSocket )
			{
				log_debug( "Socket:%X LocalSocket:%X - LocalSocket.CancelAsyncSocketCommand - RemoveCommand" );
				pCmdList->removeCommand(pSocketCmd);
				-- i;
				-- numCommands;
			}
		}
	}


}; // namespace LocalImp
}; // namespace qd::DaSocket

