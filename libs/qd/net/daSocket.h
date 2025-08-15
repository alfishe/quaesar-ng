#pragma once
#include "qd/thread/mutex.h"
#include "qd/debug/exception.h"
#include "qd/file/archiveBase.h"
#include "qd/file/archiveSerializer.h"
#include "qd/file/memFile.h"
#include "qd/stl/vector.h"
#include "qd/thread/thread.h"
#include <deque>
#include "qd/base/tribool.h"
#include "qd/stl/ref_ptr.h"
#include "qd/base/eTrisult.h"
#include "qd/log/log.h"


namespace qd
{
class CListnerImpBase;
class CFTSocket;
typedef int FTSocketErrorCode;
class CFtTcpListner;

#define vlog_socket 0

	#define FTCHUNK_SOCKET_PACKET_HEADER      _MAKE4C('HEAD') // 1st SOCKET TRANSFER PACKET
	#define FTCHUNK_SOCKET_PACKET_HEADER_VER  1

	#define FTCHUNK_LOTS_BODYROOT        _MAKE4C('BODY')  // 2st SOCKET PACKET
	#define FTCHUNK_LOTS_BODYROOT_VER    0x0101

	#define FTCHUNK_LOT_REMOTE_CMD   0x444D4345 // ECMD

	// MAXIMUM CHUNK SIZE 10Mb
	#define FT_SOCKET_MAX_CHUNK_SIZE 10100200


	FORWARD_DECLARATION_2(DaSocket, CBaseDataProc);


    namespace ERemoteCmd
    {
        using Type = uint8_t;
    };


	class CSocketException : public qd::Exception
	{
		typedef Exception super;
	public:

		enum Err
		{
			CANT_CONNECT_TO_SERVER,
			CANT_AUTHORIZE_ON_SERVER,
			CONNECTION_LOST,
			WAIT_DATA_TIMEOUT,
			PLAYER_ALREADY_ONLINE,
		};

		CSocketException(CSocketException::Err ErrID,  const char* pError, ...)
			: super(false)
		{
			va_list arg_list;
			va_start(arg_list, pError);
			setErrorMessageVA(pError, arg_list);
		}

	};



	//////////////////////////////////////////////////////////////////////////
	///
	struct CInetAddr
	{
		qd::string m_Host;
		uint16_t m_Port;

		CInetAddr()
			: m_Port(0)
		{}

		CInetAddr(const qd::string& Host, uint16_t Port)
		{
			Set(Host, Port);
		}

		void Set(const qd::string& Host, uint16_t Port)
		{
			m_Host = Host;
			m_Port = Port;
		}

		const qd::string& GetHost() const { return m_Host; }
		void SetHost(const qd::string& Host) { m_Host = Host; }
		uint16_t GetPort() const { return m_Port; }
		void SetPort(uint16_t Port) { m_Port = Port; }

		CInetAddr(const qd::string& HostWithPort)
		{
			parseHostWithPort(HostWithPort);
		}

		inline qd::string ToStringFull() const
		{
			return qd::string_format("%s:%u", CC(m_Host), (uint32_t)m_Port);
		}

		bool isValid() const {
			return m_Host.size() && m_Port;
		}

 		void parseHostWithPort(const qd::string& HostWithPort);

		bool operator == (const CInetAddr& r) const {
			return m_Port == r.m_Port && _stricmp(m_Host.c_str(), m_Host.c_str()) == 0;
		}

	}; // struct CInetAddr
	//////////////////////////////////////////////////////////////////////////





	class CSocketService
	{
	protected:
		bool m_bStarted = false;
		bool m_bDone = false;

	protected:
		CSocketService() {}

		virtual void stopImp() {
			m_bDone = true;
		}

	public:

		virtual void startAsync()
		{
		}
		virtual void startSync() {}

		bool IsStarted() {
			return m_bStarted;
		}

		void Stop();

		virtual CListnerImpBase* CreateSocketListner( CFtTcpListner* pListner ) {
			return nullptr;
		}

		virtual ref_ptr<CFTSocket> CreateSocket(DaSocket::CBaseDataProc* pDataProc) {
			return nullptr;
		}

		virtual void destroySocket(CFTSocket* pSocket);

		static CSocketService* CreateInstance();

		virtual void destroy()
		{
		}

		virtual ~CSocketService() {}
	}; // class CSocketServiceBase
	//////////////////////////////////////////////////////////////////////////



	//////////////////////////////////////////////////////////////////////////
	namespace SocketLots
	{
		enum eSendMethod
		{
			SYNC,
			ASYNC,
		};

		class CLotsPool;

		class CBaseLot : public qd::RefCounted
		{
			int m_nLocked = 0;
		public:

			virtual ~CBaseLot()
			{}

			virtual void FlushLot( qd::CArchive& ar, CFTSocket* pSocket ) //override
			{
			}

			virtual uint32_t RecommendBufferCapacity() {
				return 128;
			}

			int GetNumLocks() {
				return ( m_nLocked );
			}

			bool IsLocked() {
				return (m_nLocked > 0);
			}

			void Lock() {
				m_nLocked ++;
			}

			void Unlock() {
				m_nLocked --;
				assert(m_nLocked >= 0);
			}


		}; // class CBaseLot
		//////////////////////////////////////////////////////////////////////////




		class CRawBufLot : public SocketLots::CBaseLot
		{
		protected:
			ref_ptr<qd::MemFile> m_pMemFile;
		public:
			qd::CArchiveBin ar; // INNER ARCHIVE AND BUFFER

			CRawBufLot(uint32_t Capacity = 128)
				: ar(nullptr, true)
			{
				m_pMemFile = new qd::MemFile(Capacity);
				ar.setFile( m_pMemFile );
			}

			virtual ~CRawBufLot() {
				ar.reset();
				m_pMemFile.reset();
			}

			virtual void FlushLot( qd::CArchive& outAr, CFTSocket* pSocket ) override {
				assert( ar.GetNumOpenChunks() == 0 );
				_flushInternalBuffer(outAr);
			}

			void ExpandBuffer(uint32_t nCapacity, bool bExactSize = false) const {
				return m_pMemFile->expandBuffer(nCapacity, bExactSize);
			}

			virtual uint32_t RecommendBufferCapacity() override
			{
				uint32_t FinalSize = m_pMemFile->getSize();
				return FinalSize;
			}


			qd::MemFile* GetMemFile() const {
				return m_pMemFile;
			}

		protected:
			// virtual
			virtual void _flushInternalBuffer( qd::CArchive& arx ) {
				if ( m_pMemFile->getSize() ) {
					arx.write( m_pMemFile->getBuffer(), m_pMemFile->getSize() );
				}
				this->ar.setFile(nullptr);
				m_pMemFile = nullptr;
			}

		}; // class CRawBufLot
		//////////////////////////////////////////////////////////////////////////



		class CRemoteCmd : public CRawBufLot
		{
			typedef CRawBufLot TSuper;
			ERemoteCmd::Type m_CmdID;
			uint16_t m_CmdVer;
			// qd::CArchive& ar;
		public:

			CRemoteCmd( ERemoteCmd::Type eCmdID, uint32_t CmdVer = 0, uint32_t Capacity = 512 )
				: TSuper(Capacity)
				, m_CmdID(eCmdID)
				, m_CmdVer((uint16_t)CmdVer)
			{
			}
			virtual ~CRemoteCmd() {
			}

			inline qd::CArchive& GetArchive() {
				return this->ar;
			}

			virtual void FlushLot( qd::CArchive& arx, CFTSocket* pSocket ) override;

		}; // class CSendServerCmd
		//////////////////////////////////////////////////////////////////////////


		class CLotsPool : public qd::RefCounted
		{
			friend class CFTSocket;
			qd::vector< ref_ptr<CBaseLot> > m_pLots;
			typedef qd::vector< ref_ptr<CBaseLot> > TLotsList;
			CFTSocket* m_pSocket;

		public:
			CLotsPool(CFTSocket* pSocket)
				: m_pSocket(pSocket)
			{
				m_pLots.reserve(16);
			}

			~CLotsPool() {
				assert( m_pLots.empty() );
			}

			int GetNumLots() const {
				return (int)m_pLots.size();
			}

			bool IsEmpty() {
				return m_pLots.empty();
			}

			void Clear() {
				if ( m_pLots.empty() )
					return;
				m_pLots.clear();
			}

			inline void AddLot( ref_ptr<SocketLots::CBaseLot> pLot ) {
				// pLot->SetPool(this);
				m_pLots.push_back( std::move(pLot) );
				//return pLot;
			}

			void FlushSync() {
				FlushLots(SocketLots::SYNC);
			}
			void FlushAsync() {
				FlushLots(SocketLots::ASYNC);
			}

			void FlushLots(SocketLots::eSendMethod SendMtd);

			template<class TLot>
			inline TLot* CreateLot(/* CBaseLot */) {
				TLot* pLot = new TLot();
				AddLot((SocketLots::CBaseLot*)pLot);
				return pLot;
			}

			template<class TLot, typename T1>
			inline TLot* CreateLot(T1 p1) {
				TLot* pLot = new TLot(p1);
				AddLot((SocketLots::CBaseLot*)pLot);
				return pLot;
			}

			template<class TLot, typename T1, typename T2>
			inline TLot* CreateLot(T1 p1, T2 p2) {
				TLot* pLot = new TLot(p1, p2);
				AddLot((SocketLots::CBaseLot*)pLot);
				return pLot;
			}

			CFTSocket* GetSocket() const { return m_pSocket; }
			void SetSocket(CFTSocket* pSocket) { m_pSocket = pSocket; }

		}; // class CLotsPool
		//////////////////////////////////////////////////////////////////////////

	}; // namespace SocketLots
	//////////////////////////////////////////////////////////////////////////




	/////////////////////////////////////////////////////////////////////////////////////////////
	namespace DaSocket
	{
		typedef ref_ptr<MemData> TBlockPtr;



		struct ReadCmd_t
		{
			bool m_Async;
			bool m_bOneCommandOnly;
			//float m_TimeOut;

			ReadCmd_t()
			{
				m_Async = true;
				m_bOneCommandOnly = false;
				//m_TimeOut = 5.0f;
			}
		}; // struct ReadCmd_t
		//////////////////////////////////////////////////////////////////////////



		// RAW DATA PROCESSOR FOR HTTP INCOMING DATA
		class CBaseDataProc
		{
		protected:
			CFTSocket* m_pSocket;
			qd::Mutex m_OutBufMutex;
			std::deque< ref_ptr<qd::MemData> > m_pOutReadBuffer;

		public:
			CFTSocket* GetSocket() const { return m_pSocket; }
			void SetSocket(CFTSocket* Socket) { m_pSocket = Socket; }

			ref_ptr<MemData> PopReadedMemFile();

			void _setDirectOutReadBuffer( const ref_ptr<qd::MemData>& pInMemFile ) {
				qd::MutexLock ml( m_OutBufMutex );
				m_pOutReadBuffer.push_back( pInMemFile );
			}

			virtual ~CBaseDataProc()
			{
				m_pOutReadBuffer.clear();
			}

			virtual ETrisult OnDataBlockReceived( _Inout_ DaSocket::TBlockPtr& pInOutBuf );

			virtual ref_ptr<qd::MemData> InsertPacketHeader( void* pBuf, uint32_t Size ) // COPY BUFFER
			{
				// EMPTY HEADER - JUST COPY MEM BUF
				ref_ptr<qd::MemData> pWriteBuf = new qd::MemData();
				pWriteBuf->write( pBuf, Size );
				return pWriteBuf;
			}

			virtual void HeaderData_Begin( qd::CArchive& ar )
			{
			}

			virtual void HeaderData_End( qd::CArchive& ar )
			{
			}


		}; // struct CBaseDataProc
		//////////////////////////////////////////////////////////////////////////



		// RAW SOCKET SLOTS format processor
		struct CRawDataProc : public CBaseDataProc
		{
			typedef CBaseDataProc super;
			virtual ETrisult OnDataBlockReceived( _Inout_ ref_ptr<qd::MemData>& pReadBuf) override
			{
				return CBaseDataProc::OnDataBlockReceived(pReadBuf);
			}
		}; // struct CRawDataProc
		//////////////////////////////////////////////////////////////////////////



		// FT3 SOCKET SLOTS format processor
		struct CGameSlotDataProc : public CBaseDataProc
		{
			typedef CBaseDataProc super;
			friend class CFTSocket;
			typedef CGameSlotDataProc TThis;
		private:
			ref_ptr<MemData> m_pCurChunkBuffer;
			uint32_t m_CurChunkLen;

			ref_ptr<MemData> _PopReadOneBody();
			ref_ptr<MemData> _ReadChunkSync(float TimeOut = 5.0f, bool bOneChunkOnly = false);

		public:

			CGameSlotDataProc()
				: m_pCurChunkBuffer(nullptr)
				, m_CurChunkLen(0)
			{
			}

			ref_ptr<MemData> ReadCommands(const DaSocket::ReadCmd_t& ReadParam);

			virtual ETrisult OnDataBlockReceived( _Inout_ ref_ptr<qd::MemData>& pReadBuf ) override;

			virtual ref_ptr<qd::MemData> InsertPacketHeader( void* pBuf, uint32_t Size ) override; //

			virtual void HeaderData_Begin( qd::CArchive& ar ) override
			{
				ar.beginChunk12(FTCHUNK_SOCKET_PACKET_HEADER, FTCHUNK_SOCKET_PACKET_HEADER_VER);
			}

			virtual void HeaderData_End( qd::CArchive& ar ) override
			{
				ar.endChunk();
			}

		}; // struct CGameSlotDataProc
		//////////////////////////////////////////////////////////////////////////


		DaSocket::CBaseDataProc* MakeGameSlotProc();
		DaSocket::CBaseDataProc* MakeRawProc();

		inline void BeginSocketPacket( CArchive& ar )
		{
			ar.beginChunk12(FTCHUNK_SOCKET_PACKET_HEADER, FTCHUNK_SOCKET_PACKET_HEADER_VER);
		}

		inline void BeginSlotChunk( CArchive& ar, ERemoteCmd::Type rCmdID, uint8_t rCmdVer = 0 )
		{
			ar.beginChunk12(FTCHUNK_LOTS_BODYROOT, FTCHUNK_LOTS_BODYROOT_VER);
			uint32_t nLots = 1;
			ar << nLots;
			// LOT
			ar.beginChunk12(FTCHUNK_LOT_REMOTE_CMD);
			ar.U8_S(rCmdID); // Remote CMD_ID
			ar.U8_S(rCmdVer); // Remote CMD_VER
		};

		inline void EndSlotChunk( CArchive& ar )
		{
			ar.endChunk(); // FTCHUNK_LOT_REMOTE_CMD
			ar.endChunk(); // FTCHUNK_LOTS_BODYROOT, FTCHUNK_LOTS_BODYROOT_VER
		}

		inline void EndSocketPacket( CArchive& ar )
		{
			ar.endChunk(); // FTCHUNK_SOCKET_PACKET_HEADER, FTCHUNK_SOCKET_PACKET_HEADER_VER);
		}


		inline MemData WrapRemoteCmdDataAsLot( const MemData &md, ERemoteCmd::Type rCmdID, uint8_t rCmdVer = 0 ) {
			CMemoryArchiveBin ar( md.getSize() + 36, ESaveLoad::Save );
			DaSocket::BeginSlotChunk(ar, rCmdID, rCmdVer);
			ar.write( md.getBuffer(), md.getBufSize() );
			DaSocket::EndSlotChunk(ar);
			return ar.GetMemoryData();
		}


		template <class TLot>
		struct LotScope_ {
			CFTSocket* m_pSocket = nullptr;
			ref_ptr<TLot> m_pLot;

			TLot* operator->() const {
				return m_pLot.get();
			}
			LotScope_(CFTSocket* pSocket, ref_ptr<TLot> pLot )
				: m_pSocket(pSocket)
				, m_pLot(pLot)
			{}

			// as AUTO_PTR
			LotScope_(LotScope_<TLot>& r) {
				m_pSocket = r.m_pSocket;
				m_pLot = std::move(r.m_pLot);
				r.m_pSocket = nullptr;
			}

			inline ~LotScope_();

		}; // struct LotScope_

		}; // namespace DaSocket
	////////////////////////////////////////////////////////////



	//////////////////////////////////////////////////////////////////////////
	//
	class CFTSocket : public qd::RefCounted
	{
		friend class CFtTcpListner;
		friend struct DaSocket::CGameSlotDataProc;
		friend class SocketLots::CLotsPool;
	public:
		enum eConSt
		{
			CLOSED = 0,
			CONNECTING = 1, // ASYNC CONNECTING
			CONNECTED = 2,
		};
		typedef CFTSocket EConSt;

		qd::Mutex m_SocketMutex;

	protected:
		eastl::atomic<int> /*eConSt*/ m_ConnectState;
		bool m_bInit;
		qd::string m_ConnectName;

		ref_ptr<SocketLots::CLotsPool> m_pLotsPool;


	protected:

		DaSocket::CBaseDataProc* m_pDataProc;
		CInetAddr m_ConnectAddr;

		void SetInit(volatile bool Init) {
			m_bInit = Init;
		}

		// CREATES FROM: m_pIOService->CreateSocket();
		CFTSocket( DaSocket::CBaseDataProc* pDataProc );

	public:

		bool IsConnected() const {
			if ( !c_def(this) ) return false;
			return m_ConnectState == CFTSocket::EConSt::CONNECTED;
		}

		// IS IN PROCESS OF CONNECTION OR ALREADY CONNECTED
		bool IsInConnection() const {
			if ( !c_def(this) ) return false;
			int _state = m_ConnectState;
			return _state == CFTSocket::EConSt::CONNECTED || _state == CFTSocket::EConSt::CONNECTING;
		}

		bool IsAlive() const {
			if (!c_def(this) || !m_bInit)
				return false;
			return IsConnected();
		}

		void SetConnectState(CFTSocket::eConSt bCon);

		inline CFTSocket::eConSt GetConnectState() const {
			return ( CFTSocket::eConSt )(uint32_t)m_ConnectState;
		}

		bool IsInit() const {
			return m_bInit;
		}

		virtual void OnSocketConnected();

		virtual bool ConnectSync(const qd::string& pServerHoset, unsigned short Port) {
			return false;
		}
		virtual void Close();
		virtual void CloseAsync() {
			if ( vlog_socket >= 1 )
				log_debug( "Socket:%X CloseAsync()", ptr2DW( this ) );
			Close();
		}

		bool WaitConnection(float TimeOut = 90.0f);

		void AsyncWrite( void* pBuf, uint32_t Size );

		inline void AsyncWrite( qd::MemFile* pFile ) {
			assert(pFile && pFile->getBuffer()); assert(pFile->getNumChunks() == 0);
			AsyncWrite( pFile->getBuffer(), pFile->getSize() );
		}

		inline void AsyncWrite( const qd::MemData& pData )  {
			assert( pData.getBuffer() );
			AsyncWrite( pData.getBuffer(), pData.getSize() );
		}

		void SyncWrite( const qd::MemData& md );

		SocketLots::CLotsPool* GetLots() const {
			return m_pLotsPool;
		}

		inline void AddLot( ref_ptr<SocketLots::CBaseLot>/*&&*/ pLot ) {
			qd::MutexLock ml( m_SocketMutex );
			if (m_pLotsPool) {
				m_pLotsPool->AddLot( std::move(pLot) );
			}
			//return pLot;
		}

		void SetSendPool( const ref_ptr<SocketLots::CLotsPool>& pSendPool) {
			qd::MutexLock ml( m_SocketMutex );
			m_pLotsPool = pSendPool;
			if ( m_pLotsPool )
				m_pLotsPool->SetSocket(this);
		}

		void ClearLots() {
			qd::MutexLock ml( m_SocketMutex );
			if ( m_pLotsPool )
				m_pLotsPool->Clear();
		}

		void FlushSync() {
			// NO MUTEX here - DEAD LOCK OCCURE  //qd::MutexLock ml( m_SocketMutex );
			if ( m_pLotsPool && !m_pLotsPool->IsEmpty() ) {
				m_pLotsPool->FlushLots(SocketLots::SYNC);
			}
		}

		void FlushAsync() {
			qd::MutexLock ml( m_SocketMutex );
			if ( m_pLotsPool && !m_pLotsPool->IsEmpty() ) {
				m_pLotsPool->FlushLots(SocketLots::ASYNC);
			}
		}

		// SENDS REMOTE ALL BUFFERS THEN RETURN
		virtual void FlushSocketSync() {
			FlushSync();
		}

		void SetConnectName(const qd::string& ConnectName) {
			m_ConnectName = ConnectName;
		}
		const qd::string& GetConnectName() {
			return m_ConnectName;
		}

		DaSocket::CBaseDataProc* GetDataProc() const {
			return m_pDataProc;
		}
		DaSocket::CGameSlotDataProc* GetSlotProc() const {
			return static_cast<DaSocket::CGameSlotDataProc *>(m_pDataProc);
		}

		void SetDataProc(DaSocket::CBaseDataProc* DataProc) {
			m_pDataProc = DataProc;
		}

		const CInetAddr& GetConnectAddr() const {
			return m_ConnectAddr;
		}
		void SetConnectAddr(const CInetAddr& ConnectAddr) {
			m_ConnectAddr = ConnectAddr;
		}

	public:

		virtual void destroy();

		virtual ~CFTSocket();

		bool AsyncRead( ref_ptr<qd::MemData> pReadBuf = nullptr );



		template<class TLot, typename ...TArgs>
		inline DaSocket::LotScope_<TLot> MakeLot_(TArgs&& ...args) { // SocketLots::CBaseLot*
			DaSocket::LotScope_<TLot> sc(this, new TLot(args...));
			return sc;
		}


		virtual qd::string GetEndPointIP() {
			return "UNKNOWN";
		}

	protected:

		virtual void _asyncReadInternal( ref_ptr<qd::MemData> pReadBuf ) = 0;
		virtual void _asyncWriteInternal( ref_ptr<qd::MemData> pSendData) = 0;
		virtual void _syncWriteInternal( ref_ptr<qd::MemData> pSendData) = 0;

	public:
		void _onDataBlockReadedLock( ref_ptr<qd::MemData> pReadBuf );

		ref_ptr<MemData> PopReadedMemFile() {
			return m_pDataProc->PopReadedMemFile();
		}


	}; // class CFTSocket
	//////////////////////////////////////////////////////////////////////////


	class CListnerImpBase
	{
	protected:
		CSocketService* m_pIOService;
		CFtTcpListner* m_pTcpListner;
		unsigned short m_nListenPort;

	public:

		CListnerImpBase( CSocketService* pIOService, CFtTcpListner* pTcpListner );
		unsigned short GetPort() const {
			return m_nListenPort;
		}

		CSocketService* GetIOService() const {
			return m_pIOService;
		}
		template<class T>
		T* GetIOService() const {
			return static_cast<T*>(m_pIOService);
		}

		// SYNC WAITING CONNECTION IN THREAD
		virtual void WaitingStartASync( ref_ptr<CFTSocket> pIncomeSocket ) {};

		virtual void destroy()
		{
		}

		virtual ~CListnerImpBase()
		{
			m_pIOService = nullptr;
		}

	}; // class CSocketAcceptorBase
	//////////////////////////////////////////////////////////////////////////



	class CFTServerPart;



	//////////////////////////////////////////////////////////////////////////
	//
	class CFtTcpListner
	{
		CSocketService* m_pIOService;
        using TOnConnectCB = void(*)(ref_ptr<CFTSocket>);
        TOnConnectCB m_OnConnectCB;
        using TCreateDataProc = DaSocket::CBaseDataProc* (*)();
        TCreateDataProc m_CreateDataProc;
		CListnerImpBase* m_pListnerImp;
		qd::Thread m_ListnerThread;
		qd::string m_Name;
		unsigned short m_nListenPort;
		volatile bool m_bRun;

	public:

		CFtTcpListner( CSocketService* pIOService, unsigned short nPort, const CFtTcpListner::TOnConnectCB& OnConnect, qd::string Name );

		void SetSocketDataProcCallback(const CFtTcpListner::TCreateDataProc& CreateDataProc) {
			m_CreateDataProc = CreateDataProc;
		}

		unsigned short GetPort() const {
			return m_nListenPort;
		}
		void SetPort(unsigned short Port) { m_nListenPort = Port; }

		CSocketService* GetIOService() const {
			return m_pIOService;
		}

		bool IsRun() const {
			return m_bRun;
		}

	public:

		void StartAsync();

		void destroy();

		void OnServerConnectionAcceptedCallback(ref_ptr<CFTSocket> pSocket, FTSocketErrorCode error);

		~CFtTcpListner();

	private:
		void _OnStartConnectionWaiting();

		void _ListnerThreadProc();

	}; // class CFTcpListner
	//////////////////////////////////////////////////////////////////////////



	template <class TLot>
	inline DaSocket::LotScope_<TLot>::~LotScope_() {
		if (m_pSocket) {
			assert(m_pLot);
			m_pSocket->AddLot(std::move(m_pLot));
		}
		c_def(0);
	}


}; // namespace qd
