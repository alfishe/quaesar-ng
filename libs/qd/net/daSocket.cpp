#include "daSocket.h"
#include "qd/stl/ref_ptr.h"
#include "qd/mem/memBuffer.h"
#include "qd/log/log.h"
#include "daSocketLocal.h"
#include "qd/math/mathBase.h"

#define QD_SOCKET_READ_BLOCK_SIZE   4096

namespace qd
{

	ref_ptr<MemData> DaSocket::CGameSlotDataProc::_ReadChunkSync( float TimeOut, bool bOneChunkOnly )
	{
		ref_ptr<MemData> pReadBuf;

		do // LOOP
		{
			if (bOneChunkOnly)
				pReadBuf = _PopReadOneBody();  // RARE NEED ONLY ONE CHUNK
			else
				pReadBuf = PopReadedMemFile();

			if (pReadBuf) {
				if ( vlog_socket >= 3 )
					log_debug( "ReadChunk - Received: %u bytes", pReadBuf->getSize() );
				return pReadBuf;
			}

		} while( m_pSocket->IsConnected() );

	 	if ( vlog_socket >= 5 )
	 		log_debug( "_ReadChunkSync() - wait data - TIMEOUT" );

		return pReadBuf;
	}




	CFTSocket::CFTSocket( DaSocket::CBaseDataProc* pDataProc )
		: m_pDataProc(pDataProc)
		, m_ConnectState(EConSt::CLOSED)
		, m_bInit(false)
	{
		m_pDataProc->SetSocket(this);
		m_pLotsPool = new SocketLots::CLotsPool(this);
	}

	CFTSocket::~CFTSocket()
	{
		if ( IsInit() || m_pLotsPool || m_pDataProc)
			destroy();

	//	SAFE_DELETE(m_pLotsPool);
	//	SAFE_DELETE(m_pDataProc);
	}


	void CFTSocket::Close()
	{
		if ( !IsInit() )
			return;

		qd::MutexLock ml( m_SocketMutex );

		if ( vlog_socket >= 1 )
			log_debug( "Socket:%X Close()", ptr2DW(this) );

		if ( m_pLotsPool ) {
			assert( !IsConnected() || m_pLotsPool->IsEmpty() );
			m_pLotsPool->Clear();
		}

		SetConnectState( CFTSocket::EConSt::CLOSED );
		SetInit(false);
	}



	void CFTSocket::destroy()
	{
		if (!c_def(this)) return;

		qd::MutexLock ml( m_SocketMutex );

		if ( IsInit() )
		{
			if ( vlog_socket >= 2 )
				log_debug( "Socket:%X Destroy()", ptr2DW( this ) );
			Close();
		}

		if( m_pLotsPool )
		{
			if ( !IsConnected() )
				m_pLotsPool->Clear(); // TO PREVENT CALL OF DEBUG ASSERT
			m_pLotsPool = nullptr; // SAFE_DELETE( m_pLotsPool );
		}
		SAFE_DELETE(m_pDataProc);
	}



	void CFTSocket::_onDataBlockReadedLock( ref_ptr<qd::MemData> pReadBuf )
	{
		if ( m_pDataProc )
		{
			// <MemData> pReadBuf - Can Change
			// Copys data to  CBaseDataProc::m_pOutReadBuffer  and returns NEW or resized buffer
			ETrisult res = m_pDataProc->OnDataBlockReceived( pReadBuf ); // DaSocket::CGameSlotDataProc::OnDataBlockReceived( _Inout_ pReadBuf )

			if ( res.isSuccess() ) {
				AsyncRead( pReadBuf ); // READ AGAIN
			}
			else {
				Close();
				//ft_assert2( 0, "Socket:%X CFTSocket::_onDataBlockReadedLock - return ERROR", ptr2DW(this) );
			}
		}
		else {
			if ( vlog_socket >= 2 )
				log_debug("Socket:%X _onDataBlockReadedLock() - m_pDataProc == nullptr", ptr2DW(this) );
			c_def( 0 );
		}
	}



	// GAME DATA CHUNK PARSER
	ETrisult DaSocket::CGameSlotDataProc::OnDataBlockReceived( _Inout_ DaSocket::TBlockPtr& pInOutBuf ) /* OVERRIDE */
	{
		uint32_t nCurFilePos = 0;
		const uint32_t nInBufSize = pInOutBuf->getSize();

		while( nCurFilePos < nInBufSize ) // SPLIT ON CHUNKS
		{
			uint32_t bufLenRem = nInBufSize - nCurFilePos;

			if ( vlog_socket >= 3 )
				log_debug( "Socket:%X: CFTSocket::OnDataBlockReceived() Remain Size: %u", ptr2DW( m_pSocket ), bufLenRem );

			if ( !m_pCurChunkBuffer )
			{
				if ( bufLenRem < CFileChunk12::size_of() )
					break; // WAIT FULL CHUNK

				MemFile ReadBufFile( pInOutBuf->getBuffer( nCurFilePos ), bufLenRem, false );
				qd::CArchiveBin ar( &ReadBufFile, false );

				qd::CFileChunk12 ChunkBufRead(FTCHUNK_SOCKET_PACKET_HEADER, 0);
				if ( !ar.loadChunkTryTest( ChunkBufRead) || ChunkBufRead.getVer() < FTCHUNK_SOCKET_PACKET_HEADER_VER )
				{
					//ft_assert2( 0, "Socket:%x - Bad Data received", ptr2DW(m_pSocket) );
					if ( c_def(0) ) {
						ar.setFilePos(nCurFilePos); // to rewind serialize for debuf
					}
					log_debug( "socket:%X: CFTSocket::OnDataBlockReceived() WARNING: ERROR: - Bad ChunkBufRead IDs - SKIP BUFFER", ptr2DW( m_pSocket ) );
					pInOutBuf->setSize(0);
					return ETrisult::ERROR;
				}

				nCurFilePos += CFileChunk12::size_of();
				bufLenRem -= CFileChunk12::size_of();

				uint32_t FullChunkSize = ChunkBufRead.getSize();
				if (FullChunkSize >= FT_SOCKET_MAX_CHUNK_SIZE )
					throw qd::Exception( "ERROR: WARNING: FT-SOCKET Very Large ChunkBufRead Size!" );

				m_CurChunkLen = FullChunkSize;
				m_pCurChunkBuffer = new MemData(m_CurChunkLen);

				if ( vlog_socket >= 3 )
					log_debug( "socket:%X: CFTSocket::OnDataBlockReceived() - Creates new Chunk buffer size: %u", ptr2DW( m_pSocket ), m_CurChunkLen );
			}

			if ( m_pCurChunkBuffer->getSize() < m_CurChunkLen )
			{
				// NOT FULL CHUNK DOWNLOADED YET
				// APPEND BUFFER
				if ( vlog_socket >= 3 )
					log_debug("socket:%X: Expand buffer for full chunk size from %i to %i", ptr2DW( m_pSocket ), m_pCurChunkBuffer->getSize(), m_CurChunkLen );

				void* pStartBuf = pInOutBuf->getBuffer( nCurFilePos ); // chunk header already read
				uint32_t nBufLen = qd::min( (m_CurChunkLen - m_pCurChunkBuffer->getSize()), (nInBufSize - nCurFilePos) );
				m_pCurChunkBuffer->write( pStartBuf, nBufLen );
				nCurFilePos += nBufLen;

				ASSERT_F( m_pCurChunkBuffer->getSize() <= m_CurChunkLen, "Socket read once buffer ERROR", 0 );
			}

			if ( m_pCurChunkBuffer->getSize() == m_CurChunkLen )
			{
				// FULL CHUNK RECEIVED
				if ( vlog_socket >= 4 )
					log_debug( "Socket:%X: CFTSocket::OnDataBlockReceived() - Full chunk received: %u", ptr2DW( m_pSocket ), m_CurChunkLen );

				qd::MutexLock ml(m_OutBufMutex);
				m_pOutReadBuffer.push_back( m_pCurChunkBuffer );
				m_pCurChunkBuffer = nullptr;
				m_CurChunkLen = 0;
			}
		} // while( nInBufSize > nCurFilePos )


		// MOVE BUFFER TO BEGIN
		if ( nCurFilePos != 0 )
		{
			if ( pInOutBuf->getSize() == nCurFilePos ) {
				// FREE BUFFER
				pInOutBuf->setSize(0);
				pInOutBuf->getMemBuf()->setU32(0, MAKE4C('O','L','D','B') ); // spoil the buffer for checks "OLDB"
			}
			else {
				// MOVE CHUNK START TO BEGIN OF BUFFER
				uint32_t nRemSize = pInOutBuf->getSize() - nCurFilePos;
				pInOutBuf->getMemBuf()->memMove( nCurFilePos, 0, nRemSize ); // MOVE TO ZERO
				pInOutBuf->setSize(nRemSize); // SET POSITION

				if ( vlog_socket >= 1 )
					log_debug( "Socket:%X: Move Readed buffer to start from nCurFilePos: %u nRemSize: %u", ptr2DW( m_pSocket ), nCurFilePos, nRemSize );
			}
		}

		return ETrisult::SUCCESS;
	}



	bool CFTSocket::WaitConnection( float timeOut /*= 90.0f*/ )
	{
		float totalTime = 0;
		do {
			if ( IsConnected() )
				return true;

			float SleepTime = 0.25f;
			totalTime += SleepTime;
			qd::sleep(SleepTime);
		} while ( totalTime < timeOut ); // CONNECTION TIMEOUT

		return false;
	}



	void CFTSocket::SyncWrite( const qd::MemData& md )
	{
		if( !md.getSize() )
			return;

		ref_ptr<qd::MemData> pSendData = m_pDataProc->InsertPacketHeader( md.getBuffer(), md.getSize() );
		_syncWriteInternal(pSendData);
	}



	void CFTSocket::AsyncWrite( void* pBuf, uint32_t Size )
	{
		if(!pBuf || !Size)
			return;

		ref_ptr<qd::MemData> pSendData = m_pDataProc->InsertPacketHeader( pBuf, Size );
		assert(pSendData);

		_asyncWriteInternal(pSendData);
	}



	bool CFTSocket::AsyncRead( ref_ptr<qd::MemData> pReadBuf /*= nullptr*/ )
	{
		// APPEND FUTURE READ DATA TO THIS BUFFER

		if ( !IsConnected() ) {
			log_debug( "Socket:%X ASyncRead WARNING: Socket no more connected", ptr2DW(this) );
			// THIS EXCEPTION - DROP OFF FROM - SocketListner Service  ( CAsioSocketService.StartSync )
			// throw CSocketException( CSocketException::CONNECTION_LOST, "Socket:%X is closed - can't AsyncRead", ptr2DW( this ) );
			return false;
		}

		if ( pReadBuf )
		{
			uint32_t bufCapacity = pReadBuf->getCapacity();
			if ( vlog_socket >= 3 )
				log_debug( "Socket:%X START ASync Read - Pos=%u Capacity=%u bytes", ptr2DW(this), pReadBuf->getSize(), bufCapacity );
		}
		else
		{
			pReadBuf = new qd::MemData( QD_SOCKET_READ_BLOCK_SIZE );
			if ( vlog_socket >= 2 )
				log_debug( "Socket:%X ASyncRead() - Create NEW Buffer", ptr2DW(this) );
		}

		_asyncReadInternal(pReadBuf);

		c_def(0);
		return true;
	}





	void CFTSocket::OnSocketConnected()
	{
		if ( vlog_socket >= 0 )
			log_debug( "CFTSocket::OnSocketConnected to \"%s:%u\" (%s) socket:%X - Start ASYNC READ", CC(m_ConnectAddr.GetHost()), (uint32_t)m_ConnectAddr.m_Port, CC(m_ConnectName), ptr2DW(this) );

		ASSERT_F( !m_ConnectState, "CFTSocket::OnSocketConnected m_ConnectState(%i) != 0", (int)m_ConnectState );
		SetConnectState( CFTSocket::EConSt::CONNECTED );
		SetInit(true);
		AsyncRead();

		c_def(0);
	}



	void CFTSocket::SetConnectState( CFTSocket::eConSt bCon )
	{
		if ( m_ConnectState == (int)bCon )
			return;

		if ( vlog_socket >= 0 )
			log_debug( "Socket:%X: CFTSocket::SetConnectState(%i)", ptr2DW( this ), (int)bCon );

		m_ConnectState = bCon;

		if ( m_ConnectState == CFTSocket::CONNECTED && !m_pLotsPool->IsEmpty() ) {
			FlushAsync(); // DANGER HERE if it in different Threads
		}
	}


	ref_ptr<MemData> DaSocket::CGameSlotDataProc::_PopReadOneBody()
	{
		qd::MutexLock ml(m_OutBufMutex);

		assert( 0 );

		if ( m_pOutReadBuffer.empty() )
			return nullptr;

		ref_ptr<MemData> pCurBufFile = m_pOutReadBuffer.front();
		m_pOutReadBuffer.pop_front();

		if ( vlog_socket >= 2 )
			log_debug( "CFTSocket::_PopReadOneBody() %u bytes", pCurBufFile->getSize() );

		return pCurBufFile;
	}



	ref_ptr<qd::MemData> DaSocket::CGameSlotDataProc::ReadCommands( const DaSocket::ReadCmd_t& ReadParam )
	{
		ref_ptr<MemData> pMemChunk;
		if (ReadParam.m_Async)
		{
			if ( ReadParam.m_bOneCommandOnly )
				pMemChunk = _PopReadOneBody();
			else
				pMemChunk = PopReadedMemFile();
		}
		else {
			// READ SINC
			pMemChunk = _ReadChunkSync( 0/*ReadParam.m_TimeOut*/ );
		}
		return pMemChunk;
	}


	ref_ptr<qd::MemData> DaSocket::CGameSlotDataProc::InsertPacketHeader( void* pBuf, uint32_t Size )
	{
		uint32_t nNewSize = Size + 16;
		// WRITE TO SOCKET PACKET 1st HEADER
		qd::CMemoryArchiveBin ar( nNewSize, ESaveLoad::Save );
		TThis::HeaderData_Begin(ar);
		//ar.BeginChunk12(FTCHUNK_SOCKET_PACKET_HEADER, FTCHUNK_SOCKET_PACKET_HEADER_VER);
		ar.write(pBuf, (uint32_t)Size);
		//ar.EndChunk();
		TThis::HeaderData_End(ar);

		ref_ptr<MemData> pSendData = ar.GetMemoryDataPtr();

		if ( vlog_socket >= 2 )
			log_debug("Socket:%X InsertPacketHeader(): %u bytes", ptr2DW(m_pSocket), pSendData->getSize() );

		return pSendData;
	}



	void CFtTcpListner::OnServerConnectionAcceptedCallback( ref_ptr<CFTSocket> pSocket, FTSocketErrorCode error )
	{
		if ( !error )
		{
			if ( vlog_socket >= 1 )
				log_debug( "CFtTcpListner::OnServerConnectionAcceptedCallback() - socket:%X", ptr2DW(pSocket) );

			pSocket->OnSocketConnected();

			if ( m_OnConnectCB )
				m_OnConnectCB( pSocket );
		}
		else {
			if ( vlog_socket >= 1 )
				log_debug( "CFtTcpListner::OnServerConnectionAcceptedCallback() - Error occured code:%i socket:%X", (int)error, ptr2DW(pSocket) );
		}

		_onStartConnectionWaiting();
	}



	DaSocket::CBaseDataProc* DaSocket::MakeGameSlotProc()
	{
		return new DaSocket::CGameSlotDataProc();
	}

	DaSocket::CBaseDataProc* DaSocket::MakeRawProc()
	{
		return new DaSocket::CRawDataProc();
	}


	CFtTcpListner::CFtTcpListner( CSocketService* pIOService, unsigned short nPort, const CFtTcpListner::TOnConnectCB& OnConnect, qd::string Name )
        : m_pIOService(pIOService)
        , m_OnConnectCB(OnConnect)
        , m_Name(Name)
        , m_nListenPort(nPort)
    {
		log_debug( "CFtTcpListner - starts listen port: %u Name:\"%s\"", (uint32_t)nPort, CC(m_Name) );
		m_CreateDataProc = &DaSocket::MakeGameSlotProc;
	}



	void CFtTcpListner::startAsync()
	{
		if ( vlog_socket >= 1 )
			log_debug("CFtTcpListner::startAsync() : Port: %i Name:\"%s\"", (int)m_nListenPort, CC(m_Name) );

		// START TCP listener
		if ( m_ListnerThread.isActive() ) {
			ASSERT_F( 0, "Socket port:%i Listner Already Active", (int)m_nListenPort, CC(m_Name) );
			return;
		}

		if (!m_nListenPort)
			throw Exception( "CFtTcpListner::Start - listner port == %u", (uint32_t)m_nListenPort );

		m_ListnerThread.setThreadName(m_Name);
        m_ListnerThread.create([this]() { this->_listnerThreadProc(); });
	}


	void CFtTcpListner::destroy()
	{
		if (vlog_socket >= 1 )
			log_debug("CFtTcpListner::Destroy() : Port: %i Name:\"%s\"", (int)m_nListenPort, CC(m_Name) );

		m_OnConnectCB = nullptr;

		if (m_pListnerImp) {
			m_pListnerImp->destroy();
		}
		SAFE_DELETE(m_pListnerImp);

		if (m_pIOService) {
			m_pIOService->Stop();
		}

		m_ListnerThread.join();
		// WAIT EXIT FROM THREAD
		c_def(0);

		if ( vlog_socket )
			log_debug("CFtTcpListner::Stopped() success: Port: %i  Name:\"%s\"", m_nListenPort, CC(m_Name) );
	}






	// RUN IO SERVICE IN THREAD
	void CFtTcpListner::_listnerThreadProc()
	{
		assert2( !m_pListnerImp, "CFtTcpListner::!m_pListnerImp", 0 );
		if ( m_pListnerImp )
			return;

		// EXECUTES IN THREAD
		assert(m_nListenPort);
		m_pListnerImp = m_pIOService->CreateSocketListner(this); // CREATE IMPLEMENTATION

		if ( vlog_socket >= 1 )
			log_debug( "CFtTcpListner::_ListnerThreadProc() port:%i  Name:\"%s\" - START WAITING THREAD", (int)m_nListenPort, CC(m_Name) );

		_onStartConnectionWaiting();

		m_bRun = true;

		if ( !m_pIOService->IsStarted() )
		{
			// INFINITE LOOP OF IO_SERVICE proactor
			m_pIOService->startSync(); // STARTS SYNC WAITNING IN HIS OWN THREAD
		}

		if ( vlog_socket >= 1 )
			log_debug( "CFtTcpListner::_ListnerThreadProc() port:%i Name:\"%s\" WARNING: CANCEL LISTENING  - EXIT THREAD", (int)m_nListenPort, CC(m_Name) );

		m_bRun = false;
		c_def(0);
		return;
	}


	void CFtTcpListner::_onStartConnectionWaiting()
	{
		if ( vlog_socket >= 1 )
			log_debug( "CFtTcpListner::_onStartConnectionWaiting() port:%i Name:\"%s\"", (int)m_nListenPort, CC(m_Name) );

		if (!m_pListnerImp) {
			log_debug("ERROR - CFtTcpListner::_onStartConnectionWaiting() m_pListnerImp is NULL (port:%i Name:\"%s\")", (int)m_nListenPort, CC(m_Name));
			return;
		}

		// CREATE SOCKECT FROM ONE IO_SERVICE
		DaSocket::CBaseDataProc* pDataProc = m_CreateDataProc(); // DEFAULT DATA CONVERTER
		ref_ptr<CFTSocket> pSocket = m_pIOService->CreateSocket( pDataProc ); // CREATES NEW SOCKET
		m_pListnerImp->WaitingStartASync(pSocket);
	}



	CFtTcpListner::~CFtTcpListner()
	{
		if (vlog_socket >= 1 )
			log_debug( "CFtTcpListner::~CFtTcpListner() port:%i - DESTROYED", (int)m_nListenPort );
	}



	void CSocketService::Stop()
	{
		if ( vlog_socket >= 1 )
			log_debug( "CSocketService::Stop()" );

		stopImp();
	}

	void CSocketService::destroySocket( CFTSocket* pSocket )
	{
		if (pSocket)
		{
			if (vlog_socket >= 1 )
				log_debug( "CSocketService::DestroySocket( socket:%X)", ptr2DW(pSocket) );

			pSocket->destroy();
		}
	}


	CSocketService* CSocketService::CreateInstance()
	{
		CSocketService* pInstance = nullptr;

		pInstance = new DaSocket::LocalImp::CSocketServiceLocal();
		return pInstance;
	}



	void SocketLots::CLotsPool::FlushLots( SocketLots::eSendMethod SendMtd)
	{
		if ( m_pLots.empty() || m_pSocket->GetConnectState() == CFTSocket::CONNECTING ) // WAIT CONNECTING
			return;

		if ( !m_pSocket->IsConnected() ) {
			if ( vlog_socket >= 1 )
				log_debug( "SocketLots::CLotsPool::FlushLots - Socket:%X is already not connected!", ptr2DW(m_pSocket) );
			m_pLots.clear();
			return;
		}

		// PRE-CALC BUFFER SIZE
		uint32_t BufferSize = 500;
		for (TLotsList::iterator It = m_pLots.begin(); It != m_pLots.end(); ++ It) {
			CBaseLot* pCurLot = *It;
			BufferSize += pCurLot->RecommendBufferCapacity();
		}

		qd::CMemoryArchiveBin ar( BufferSize, ESaveLoad::Save );
		uint32_t nLotsCount = (uint32_t)m_pLots.size();

		m_pSocket->m_pDataProc->HeaderData_Begin(ar);
		// COMBINE ALL LOTS TO FINAL_FILE BUFFER
		{
			ar.beginChunk12(FTCHUNK_LOTS_BODYROOT, FTCHUNK_LOTS_BODYROOT_VER);
			{
				ar << nLotsCount;
				for (TLotsList::iterator It = m_pLots.begin(); It != m_pLots.end(); ++ It) { // for All Lots
					SocketLots::CBaseLot* pCurLot = *It;
					if ( pCurLot->IsLocked() ) {
						if ( vlog_socket >= 1 )
							log_warn( "CLotsPool::FlushLots - CurLot is Locked! (%i) Socket:%X", pCurLot->GetNumLocks(), ptr2DW(m_pSocket) )
							->ASSERT_DLG();
					}
					pCurLot->FlushLot( ar, m_pSocket );
				}
			}
			ar.endChunk();
		}
		m_pSocket->m_pDataProc->HeaderData_End(ar);

		m_pLots.clear();

		// SEND FINAL FRAMES
		try {
			ref_ptr<MemData> pMD = ar.GetMemoryDataPtr();
			ar.reset();

			if (vlog_socket >= 3)
                log_warn("CLotsPool::FlushLots() - Socket:%X nLotsCount:(%u) size:%u", ptr2DW(m_pSocket), (uint32_t)nLotsCount, (uint32_t)pMD->getSize());

			if (SendMtd == SocketLots::ASYNC) {
				m_pSocket->_asyncWriteInternal( std::move(pMD) );
			}
			else {
				m_pSocket->_syncWriteInternal( std::move(pMD));
			}
		}
		catch (std::exception& e) {
			// TODO: LOST CONNECTIONS
			log_debug( "Socket:%X Pool Write EXCEPTION - Can't Write to socket! MSG=\"%s\"", ptr2DW(m_pSocket), CC(e.what()) );
		}
	}




	// RAW DATA PROCESSOR
    ETrisult DaSocket::CBaseDataProc::OnDataBlockReceived( _Inout_ DaSocket::TBlockPtr& pInOutBuf )
	{
		if ( pInOutBuf->isEmpty() ) {
			assert(0); return ETrisult::FAIL;
		}

		qd::MutexLock ml(m_OutBufMutex);

		// RAW DATA READED for HTTP protocol

		// APPEND RAW HTTP DATA
		m_pOutReadBuffer.push_back(pInOutBuf);

		pInOutBuf = nullptr;

		// 	m_pOutReadBuffer->Write( pInOutBuf->GetBuffer(), pInOutBuf->GetSize() );
	// 	pInOutBuf->SetSize(0);

		return ETrisult::SUCCESS;
	}


	ref_ptr<MemData> DaSocket::CBaseDataProc::PopReadedMemFile()
	{
		qd::MutexLock ml(m_OutBufMutex);

	#ifdef _DEBUG
	// 	if ( m_DebugDelay.IsStarted() ) {
	// 		if ( m_DebugDelay.getElapsedTimeF() < 10.0f )
	// 			return nullptr;
	// 	}
	// 	m_DebugDelay.reStart();
	#endif // _DEBUG

		if ( m_pOutReadBuffer.empty() )
			return nullptr;

		ref_ptr<MemData> pCmdFile = std::move(m_pOutReadBuffer.front());
		m_pOutReadBuffer.pop_front();

		if ( vlog_socket >= 3 )
			log_debug( "CFTSocket:%X PopReadedMemFile() %u bytes bufQue:%u", ptr2DW(m_pSocket), pCmdFile->getSize(), (uint32_t)m_pOutReadBuffer.size() );

		return pCmdFile;
	}




	void SocketLots::CRemoteCmd::FlushLot( qd::CArchive& arx, CFTSocket* pSocket )
	{
		// this->ar - is used
		arx.beginChunk12( FTCHUNK_LOT_REMOTE_CMD );
		{
			arx.U8_S(m_CmdID);
			arx.U8_S(m_CmdVer);

			_flushInternalBuffer(arx);
		}
		arx.endChunk();
	}



	CListnerImpBase::CListnerImpBase( CSocketService* pIOService, CFtTcpListner* pTcpListner )
		: m_pIOService(pIOService)
		, m_pTcpListner(pTcpListner)
		, m_nListenPort (pTcpListner->getPort() )
	{
	}




void CInetAddr::parseHostWithPort(const qd::string& HostWithPort)
    {
        // 			m_Host = HostWithPort;
        // 			m_Port = 0;
        // 			uint32_t pos = m_Host.findRev(':');
        // 			if (pos == qd::string::npos)
        // 				return;
        // 			uint32_t port_pos = pos + 1;
        // 			int Port;
        // 			if ( m_Host.parseInt(Port, &port_pos) )
        // 			{
        // 				assert( (uint32_t)Port <= 0xFFFF );
        // 				m_Port = (uint16_t)Port;
        // 				m_Host.Delete( pos );
        // 			}
    }


    }; // namespace qd
