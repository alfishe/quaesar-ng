#include "qd/file/archiveSerializer.h"
#include "qd/debug/assert.h"
#include "qd/base/color.h"
#include "qd/file/fileBase.h"
#include "qd/file/memFile.h"


namespace qd
{


	CMemoryArchiveBin::CMemoryArchiveBin( uint32_t Buffer /*= 256 */, ESaveLoad bStore /*= ESaveLoad::Save*/ )
		: TSuper( nullptr, bStore )
	{
		// assert(  bStore == ESaveLoad::Save && "WARNING: CMemoryArchiveBin() constructor with UInt Buffer size - must be have ESaveLoad::Save" );
		m_pMemTmpFile = new qd::MemFile( Buffer );
		TSuper::setFile( m_pMemTmpFile );
	}


	CMemoryArchiveBin::CMemoryArchiveBin( qd::MemData& memData, ESaveLoad bStore /*= ESaveLoad::Load*/ )
		: TSuper( nullptr, bStore )
	{
		m_pMemTmpFile = new qd::MemFile( memData );
		TSuper::setFile( m_pMemTmpFile );
	}


	CMemoryArchiveBin::CMemoryArchiveBin( const ref_ptr<qd::MemData>& pMemData, ESaveLoad bStore )
		: TSuper( nullptr, bStore )
	{
		//assert(  bStore != ESaveLoad::Save && "WARNING: MemoryData size will be Zero" );
		m_pMemTmpFile = new qd::MemFile( pMemData );
		TSuper::setFile( m_pMemTmpFile );
	}


	CMemoryArchiveBin::CMemoryArchiveBin( const ref_ptr<qd::MemBuf>& pMemBuffer, uint32_t nSize /*= 0*/, ESaveLoad bStore )
		: TSuper( nullptr, bStore )
	{
		//assert(  bStore != ESaveLoad::Save && "WARNING: MemBuf size will be Zero" );
		if ( nSize == 0 )
		{
			if ( bStore.isLoad() )
				nSize = pMemBuffer->getCapacity();
			else
				nSize = 0;
		}

		m_pMemTmpFile = new qd::MemFile( pMemBuffer, nSize );
		TSuper::setFile( m_pMemTmpFile );
	}



	void CMemoryArchiveBin::reset()
	{
		assert( m_pMemTmpFile && isStoring() );
		if ( m_pMemTmpFile && isStoring() ) {
			m_pMemTmpFile->reset();
		}
		m_pAr->_arReset();
	}



	qd::MemData CMemoryArchiveBin::GetMemoryData()
	{
		return m_pMemTmpFile->getMemoryData();
	}


	const ref_ptr<qd::MemData>& CMemoryArchiveBin::GetMemoryDataPtr()
	{
		return m_pMemTmpFile->getMemoryDataPtr();
	}




	uint8_t* CMemoryArchiveBin::GetBuffer() const
	{
		return m_pMemTmpFile->getBuffer();
	}




	uint32_t CMemoryArchiveBin::GetBufSize() const
	{
		return m_pMemTmpFile->getFileSize();
	}




	CBase64Archive::CBase64Archive(uint32_t Buffer /*= 256 */, ESaveLoad bStore /*= ESaveLoad::Save*/)
		: TSuper( Buffer, ESaveLoad::Save )
	{
	}


	CBase64Archive::CBase64Archive( const qd::string& MimeBufferStr, ESaveLoad bStore /*= ESaveLoad::Load*/ )
		: TSuper( (uint32_t)MimeBufferStr.size() * 4u / 3u, ESaveLoad::Load )
	{
		assert( bStore == ESaveLoad::Load && "We can only Loads from Base64 String" );

		qd::CMimeEncoder::Decode(MimeBufferStr, *m_pMemTmpFile);
		m_pMemTmpFile->seek(0);

		//TSuper::setFile(m_pTmpFile);
	}


	qd::string CBase64Archive::GetBase64String()
	{
		qd::string res;
		uint32_t curPos = m_pMemTmpFile->getPosition();
		res = qd::CMimeEncoder::Encode(*m_pMemTmpFile);
		m_pMemTmpFile->setPosition(curPos);
		return res;
	}


	CBase64Archive::~CBase64Archive()
	{
		//TSuper::setFile(nullptr);
		//m_pTmpFile = nullptr;
	}




	CSerialMark2::CSerialMark2( qd::CArchive& ar, uint8_t ID, uint8_t nVer /*= 0 */ )
		: m_Version(nVer)
	{
		assert( nVer <= 0x20 && "Maybe Wrong Version number" );

		if ( ar.isStoring() ) {
			ar << ID; ar << nVer;
		}
		else {
			if ( ar.isLoading() ) {
				uint8_t loadID, loadVer;
				ar.U8_L(loadID);
				ar.U8_L(loadVer);
				if ( loadID != ID ) {
					//CGASSERT_EX(0, "Wrong SerialMarkID Loaded. Expected '0x%2.X', Receive: '0x%2.X'", (uint)ID, (uint)loadID);
					throw Exception( EException::IO_ERROR, "Wrong SerialMarkID Loaded. Expected '0x%2.X', Received: '0x%2.X'", (uint32_t)ID, (uint32_t)loadID );
				}
				m_Version = loadVer;
			}
		}
	}



	CSerialMark1::CSerialMark1( qd::CArchive& ar, uint8_t ID )
		: m_Byte(ID)
		, m_FromID(ID)
	{
		if ( ar.isStoring() ) {
			ar << ID;
		} else {
			ar >> m_Byte;
			if ( m_Byte != ID )
				throw Exception( EException::IO_ERROR, "Wrong SerialByteID Loaded. Expected '0x%2.X', Received: '0x%2.X'", (uint32_t)ID, (uint32_t)m_Byte );
		}
	}



	CSerialMark1::CSerialMark1( qd::CArchive& ar, uint8_t FromID, uint8_t ToID )
		: m_Byte(ToID)
		, m_FromID(FromID)
	{
		uint8_t nVer = (uint8_t)(ToID - FromID); // OVERFLOW ALLOWED
		unused(nVer);
		assert( nVer <= 0x20 && "Maybe Wrong Interval declared" );
		if ( ar.isStoring() ) {
			ar << ToID;
		} else {
			ar >> m_Byte;
			if ( (m_Byte - FromID) > nVer )
				throw Exception( EException::IO_ERROR, "Wrong SerialByteID Loaded. Expected From:'0x%2.X' To:'0x%2.X' Received: '0x%2.X'", (uint32_t)FromID, (uint32_t)ToID, (uint32_t)m_Byte );
		}
	}




	void CArchiveBin::_arSetFilePos(uint32_t Pos) {
		assert(Pos <= m_pFile->getSize());
		m_pFile->seek(Pos, SEEK_SET);
	}

}; // namespace qd
