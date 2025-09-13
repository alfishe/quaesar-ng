#pragma once
#include "qd/base/base.h"
#include "qd/file/archiveBase.h"
#include "qd/stl/ref_ptr.h"
#include "qd/stl/fixed_vector.h"
#include "qd/file/fileBase.h"
#include "qd/mem/memBuffer.h"
#include "memFile.h"



namespace qd
{


//////////////////////////////////////////////////////////////////////////
class CArchiveBin : public qd::CArchive, public qd::IArchiveFormat
{
	typedef IArchiveFormat TSuper;
	typedef CArchiveBin TThis;
	struct ChunkItem_t {
		IFileChunk m_FileChunk;
		uint32_t m_nChunkPos;
		ChunkItem_t(const IFileChunk& _ch, uint32_t filePos)
			: m_FileChunk(_ch), m_nChunkPos(filePos)
		{}
	};
	eastl::fixed_vector<ChunkItem_t, 8, true> m_ChunkStack;
	qd::CArchive& m_ar; // put it lower (due to it not very well looking in the VS Debug window)

public:

	CArchiveBin(qd::IBaseFileIO* pFile, qd::ESaveLoad bStoring = qd::ESaveLoad::Load);
	virtual ~CArchiveBin();

	void _onConstruct(qd::IBaseFileIO* pFile, qd::ESaveLoad bStoring);


	qd::CArchive& getAr() {
		return m_ar;
	}

	// Rewinds +Delta or -DeltaPos
	void rewindBytes(int iDeltaPos);

	virtual void _arSaveChunkBegin(const qd::IFileChunk& Chunk) override;
	virtual void _arSaveChunkEnd() override;

	virtual void _arLoadChunkBegin(const qd::IFileChunk& FileChunk, qd::IFileChunk* pOutChunk) override;
	virtual bool _arLoadChunkEnd(const qd::IFileChunk* pChunk = nullptr) override;

	virtual bool _arLoadChunkTryTest(IFileChunk& FileChunk, uint32_t ChunkID = 0) override;

	virtual void _arSkipBytes(uint32_t nBytes) override;

	virtual uint32_t _arGetFilePos() override;
	virtual void _arSetFilePos(uint32_t Pos) override;

	virtual void _arRead_Int(void* pDest, uint32_t nBytes) override;
	virtual void _arRead_UInt(void* pDest, uint32_t nBytes) override;
	virtual void _arRead_Float(void* pDest, uint32_t nBytes) override;
	virtual void _arWrite_Int(const void* pSrc, uint32_t nBytes) override;
	virtual void _arWrite_UInt(const void* pSrc, uint32_t nBytes) override;
	virtual void _arWrite_Float(const void* pSrc, uint32_t nBytes) override;

	virtual void _arFlush() override;

	void _binReadGenericValue(void* pDest, uint32_t nBytes);
	void _binWriteGenericValue(const void* pSrc, uint32_t nBytes);

protected:

	virtual void _arWrite_String(const char* pString, uint32_t Length) override;
	virtual void _arRead_String(qd::string& String) override;

	virtual qd::string_view _arGetFileName() override {
		return string_view(m_pFile->getFileName());
	}

	virtual void _arSkip(const qd::IFileChunk& Chunk) override;
	virtual void _arUndo(const qd::IFileChunk& Chunk) override;
	//virtual bool _arLoadChunkEnd(const qd::IFileChunk& Chunk) override;

	virtual void _arWrite_Buf(const void* pSrc, uint32_t nBytes) override final;
	virtual void _arRead_Buf(void* pDest, uint32_t nBytes) override final;

	virtual uint32_t _arGetNumOpenChunks() override final {
		return (uint32_t)m_ChunkStack.size();
	}

	virtual void _arReset() override final {
		m_ChunkStack.clear();
	}

}; // class CArchiveBin
   //////////////////////////////////////////////////////////////////////////



	//////////////////////////////////////////////////////////////////////////
	class CMimeEncoder
	{
		static char m_ToMimeChar[];

	public:

		static qd::string Encode(IBaseFileIO &File);
		static void Encode(CArchive& SourceBinFile, CArchive& DestMimeFile, uint32_t Size);
		static void Decode(const qd::string& MimeString, IBaseFileIO& ToFile);

	}; // class CMimeEncoder
	//////////////////////////////////////////////////////////////////////////



	// Saved binart to Memory - and can return MemoryData
	class CMemoryArchiveBin : public qd::CArchiveBin
	{
		typedef qd::CArchiveBin TSuper;
	protected:
		ref_ptr<qd::MemFile> m_pMemTmpFile;

	public:

		// grabs inner MemBuf - and read or save them
		CMemoryArchiveBin(/*Inout*/ qd::MemData& memData, ESaveLoad bStore /*= ESaveLoad::Load*/ );

		CMemoryArchiveBin( const ref_ptr<qd::MemData>& pMemData, ESaveLoad bStore /*= ESaveLoad::Load*/ );

		CMemoryArchiveBin( const ref_ptr<qd::MemBuf>& pMemBuffer, uint32_t nSize, ESaveLoad bStore /*= ESaveLoad::Load*/ ); // explicit does'nt work here

		explicit CMemoryArchiveBin( uint32_t outBuffer/* = 512*/, ESaveLoad bStore /*= ESaveLoad::Save*/ );

		void reset();

		virtual ~CMemoryArchiveBin() {
			TSuper::setFile( nullptr );
			m_pMemTmpFile = nullptr;
		}

		qd::MemData GetMemoryData();

		const ref_ptr<qd::MemData>& GetMemoryDataPtr();

		qd::MemFile* GetMemTmpFile() const {
			return m_pMemTmpFile;
		}

		uint8_t* GetBuffer() const;

		uint32_t GetBufSize() const;

	}; // class CMemoryArchiveBin
	//////////////////////////////////////////////////////////////////////////



	//////////////////////////////////////////////////////////////////////////
	//
	// Reads or writes to qd::string
	//
	class CBase64Archive : public qd::CMemoryArchiveBin
	{
		typedef qd::CMemoryArchiveBin TSuper;

	public:

		CBase64Archive( uint32_t Buffer = 256, ESaveLoad bStore = ESaveLoad::Save );

		CBase64Archive( const qd::string& MimeBufferStr, ESaveLoad bStore = ESaveLoad::Load );

		qd::string GetBase64String();

		virtual ~CBase64Archive();

	}; // class CBase64Archive
	//////////////////////////////////////////////////////////////////////////






	inline IFileChunk::IFileChunk(qd::CArchive& ar, qd::Arc::EChunkType _chunkType /*= false*/)
		: m_ID(0)
		, m_Version(0)
		, m_Size(0)
		, m_ReadPos(0)
		, m_ChunkInfo( Arc::ChunkInfo_t::MakeBySizeOf(_chunkType) )
		, m_bWasException(false)
	{
		assert( ar.isLoading() && "Simple FileChunk - can't stored" );
		ar >> *this;
	}


	inline void IFileChunk::skip(qd::CArchive &ar) const {
		ar.skip(*this);
	}

	inline void IFileChunk::undo( qd::CArchive& ar ) const {
		ar.undo(*this);
	}

	inline void IFileChunk::check( CArchive& ar ) const {
		ar.check( *this );
	}

	inline void IFileChunk::checkSafe( CArchive& ar ) const {
		return ar.checkSafe( *this );
	}

	//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////

	typedef CFileChunkEx12 CFileChunkEx;

}; // namespace qd
//////////////////////////////////////////////////////////////////////////

