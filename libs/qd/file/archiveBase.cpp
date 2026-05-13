#include "qd/file/archiveBase.h"
#include "qd/file/archiveSerializer.h"
#include "qd/base/Color.h"
#include "qd/base/endian.h"
#include "qd/debug/assert.h"
#include "qd/log/log.h"
#include "qd/mem/fnvHash.h"
#include "qd/stl/vector.h"


namespace qd {


CArchiveBin::CArchiveBin(qd::IBaseFileIO* pFile, qd::ESaveLoad bStoring /*= qd::ESaveLoad::Load*/)
    : CArchive(this, pFile, bStoring)
    , IArchiveFormat(pFile, bStoring)
    , m_ar(*this)
{
    TThis::_onConstruct(pFile, bStoring);
}


CArchiveBin::~CArchiveBin() {}


void CArchiveBin::_onConstruct(qd::IBaseFileIO* pFile, qd::ESaveLoad /*bStoring*/)
{
    if (pFile)
        setFile(pFile);
}


void IArchiveFormat::setFile(qd::IBaseFileIO* pFile)
{
    if (pFile == m_pFile)
        return;
    // Archive don't own pointer of m_pFile but keeps it from deletion further
    // 		if (m_pFile)
    // 			m_pFile->ref_ptr_release();
    m_pFile = pFile;
    // 		if (m_pFile)
    // 			pFile->ref_ptr_retain();
}



void CArchiveBin::_arWrite_String(const char* pString, uint32_t Length)
{
    if (Length < 0xff)
    {
        operator<< ((uint8_t)Length);
    }
    else
    {
        if (Length < 0xfffe)
        {
            operator<< ((uint8_t)0xff);
            operator<< ((uint16_t)Length);
        }
        else
        {
            operator<< ((uint8_t)0xff);
            operator<< ((uint16_t)0xffff);
            operator<< ((uint32_t)Length);
        }
    }
    _arWrite_Buf(pString, Length);
}




void CArchiveBin::_arRead_String(qtd::string& in_str)
{
    uint32_t nLen;
    uint8_t bLength;
    operator>> (bLength);

    if (bLength != 0xff)
    {
        nLen = (uint32_t)bLength;
    }
    else
    {
        uint16_t wLength;
        operator>> (wLength);

        if (wLength < 0xfffe)
        {
            nLen = (uint32_t)wLength;
        }
        else
        {
            uint32_t dwLength;
            operator>> (dwLength);
            nLen = (uint32_t)dwLength;
        }
    }

    if (nLen != 0)
    {
        in_str.resize(nLen);
        char* pBuffer = in_str.data();
        m_pAr->_arRead_Buf(pBuffer, nLen);
    }
    else
    {
        in_str.clear();
    }
}



CArchive& CArchive::operator<< (qd::IBaseFileIO& File)
{
    CArchive& ar = *this;
    uint32_t FileSize = File.getSize();

    ar << (uint32_t)_MAKE4C("ABFL"); // CFileChunk
    ar << (uint32_t)0;
    ar << FileSize + (uint32_t)sizeof(uint32_t); // CHUNK size

    ar << FileSize;

    if (!FileSize)
        return *this;

    qtd::vector<uint8_t> pBuf(1024);
    uint32_t curPos = File.tell();
    File.seek(0, SEEK_SET);

    do
    {
        uint32_t nReaded = File.read(&pBuf[0], 1024);
        if (nReaded == 0)
            break;
        m_pAr->_arWrite_Buf(&pBuf[0], nReaded);
        FileSize -= nReaded;
    } while (FileSize != 0);

    File.seek(curPos, SEEK_SET);

    return *this;
}



qd::CArchive& CArchive::operator>> (IBaseFileIO& File)
{
    CArchive& ar = *this;

    CFileChunkEx12 c1(ar, _MAKE4C("ABFL"));
    uint32_t nFileSize;
    ar >> nFileSize;

    qtd::vector<uint8_t> pBuf(1024);
    do
    {
        uint32_t nBytes = qd::min(nFileSize, (uint32_t)1024);
        m_pAr->_arRead_Buf(&pBuf[0], nBytes);
        if (File.write(&pBuf[0], nBytes) != nBytes)
            throw Exception(EException::IO_ERROR, "Acrhive::FAILED : memFile is too big!");
        nFileSize -= nBytes;
    } while (nFileSize > 0);

    return *this;
}


qd::CArchive& operator<< (qd::CArchive& ar, const qd::Color& Color)
{
    ar << ((uint32_t)Color.getU32());
    return ar;
}

qd::CArchive& operator>> (qd::CArchive& ar, qd::Color& Color)
{
    uint32_t clr;
    ar >> (clr);
    Color.set(clr);
    return ar;
}


void CArchive::writeFrom(IBaseFileIO& /*SrcFile*/, uint32_t /*nBytes*/)
{
    assert(0 && "FIXME");
    // PathTools::Get()->CopyFileAbs( SrcFile, *m_pFile, nBytes );
}



void CArchiveBin::_arSkipBytes(uint32_t nBytes)
{
    m_pFile->skip(nBytes);
}

uint32_t CArchiveBin::_arGetFilePos()
{
    return m_pFile->tell();
    // 		ArFilePos_t p;
    // 		p.m_nFilePos = m_pFile->Tell();
    // 		return p;
}


void CArchiveBin::_binReadGenericValue(void* pDest, uint32_t nBytes)
{
    _arRead_Buf(pDest, nBytes); // little endian default

    // swap if bigend
    if (qd::is_big_endian())
    {
        switch (nBytes)
        {
        case 2:
            qd::swapBytes_<2>(pDest);
            break;
        case 4:
            qd::swapBytes_<4>(pDest);
            break;
        case 8:
            qd::swapBytes_<8>(pDest);
            break;
        default:
            assert(0 && "NOT IMPLEMENTED");
            break;
        }
    }
}



void CArchiveBin::_binWriteGenericValue(const void* pSrc, uint32_t nBytes)
{
    // swap if bigend
    if (qd::is_big_endian())
    {
        switch (nBytes)
        {
            // 			case 2: qd::swapBytes_<2>(pSrc); break;
            // 			case 4: qd::swapBytes_<4>(pSrc); break;
            // 			case 8: qd::swapBytes_<8>(pSrc); break;
        case 0:
        default:
            assert(0 && "NOT IMPLEMENTED");
            break;
        }
    }

    _arWrite_Buf(pSrc, nBytes); // little endian default
}



void CArchiveBin::_arRead_Int(void* pDest, uint32_t nBytes)
{
    _binReadGenericValue(pDest, nBytes);
}



void CArchiveBin::_arRead_UInt(void* pDest, uint32_t nBytes)
{
    _binReadGenericValue(pDest, nBytes);
}



void CArchiveBin::_arRead_Float(void* pDest, uint32_t nBytes)
{
    _binReadGenericValue(pDest, nBytes);
}



void CArchiveBin::_arWrite_Int(const void* pSrc, uint32_t nBytes)
{
    _binWriteGenericValue(pSrc, nBytes);
}



void CArchiveBin::_arWrite_UInt(const void* pSrc, uint32_t nBytes)
{
    _binWriteGenericValue(pSrc, nBytes);
}



void CArchiveBin::_arWrite_Float(const void* pSrc, uint32_t nBytes)
{
    _binWriteGenericValue(pSrc, nBytes);
}



void CArchiveBin::_arFlush() {}






void CArchiveBin::_arSkip(const IFileChunk& Chunk)
{
    if (isLoading())
        m_pFile->seek(Chunk.m_ReadPos + Chunk.m_Size + Chunk.m_ChunkInfo.size_of(), SEEK_SET);
}

void CArchiveBin::_arUndo(const IFileChunk& Chunk)
{
    if (Chunk.m_ReadPos == qd::_noPos)
        throw Exception(EException::IO_ERROR, "Serialize::undo ERROR: Chunk Not Defined");

    m_pFile->seek(Chunk.m_ReadPos, SEEK_SET);
}



bool CArchiveBin::_arLoadChunkEnd(const qd::IFileChunk* pChunk /*= nullptr*/)
{
    if (pChunk)
    {
        uint32_t CurPos = m_pFile->tell();
        CurPos -= pChunk->m_ReadPos;
        CurPos -= pChunk->m_Size;
        CurPos -= pChunk->m_ChunkInfo.size_of();
        if (CurPos != 0)
        {
            if (c_def(0))
                assert(0 && "WARNING: CArchiveBin _arLoadChunkEnd() cas bad SIZE");
            return false; // throw qd::Exception(qd::EException::IO_ERROR, "bad chunk");
        }
    }
    return true;
}



void CArchive::checkSafe(const qd::IFileChunk& Chunk)
{
    check(Chunk);
    // 		if (!check(Chunk))
    // 			throw Exception( EException::IO_ERROR, "FileChunk::check : ERROR");
}



void CArchiveBin::rewindBytes(int iDeltaPos)
{
    uint32_t Pos = m_pFile->tell();

    if (iDeltaPos >= 0)
        Pos += (uint32_t)iDeltaPos;
    else
        Pos -= (uint32_t)(-iDeltaPos);

    m_pFile->seek(Pos, SEEK_SET);
}




void CArchiveBin::_arLoadChunkBegin(const qd::IFileChunk& inChunk, qd::IFileChunk* pOutChunk)
{
    CArchive& ar = getAr();
    qd::IFileChunk outChunk = inChunk; // DEEP COPY
    Arc::ChunkInfo_t tp = outChunk.m_ChunkInfo;
    outChunk.m_ReadPos = m_pFile->tell();

    // CHUNK ID
    if (tp.m_IDType == Arc::EChunkID_U32)
    {
        ar.U32_L(outChunk.m_ID); // 4
    }
    else if (tp.m_IDType == Arc::EChunkID_U16)
    {
        ar.U16_L(outChunk.m_ID); // 2
    }
    else if (tp.m_IDType == Arc::EChunkID_U8)
    {
        ar.U8_L(outChunk.m_ID); // 4
    }
    else if (tp.m_IDType == Arc::EChunkID_U0)
    {
    }
    else if (tp.m_IDType == Arc::EChunkID_STRING)
    {
        outChunk.m_ID = U8_L();
        //_arRead_Buf(&outChunk.m_pIDStr[0], strLen);
        // outChunk._setChunkStrLen(strLen);
    }
    else
    {
        assert(0 && "CHUNK ID NOT IMPLEMENTED");
    }

    // VERSION
    if (tp.m_VerType == Arc::EChunkVer_U32)
    {
        ar.U32_L(outChunk.m_Version); // 4
    }
    else if (tp.m_VerType == Arc::EChunkVer_U16)
    {
        ar.U16_L(outChunk.m_Version); // 4
    }
    else if (tp.m_VerType == Arc::EChunkVer_U8)
    {
        ar.U8_L(outChunk.m_Version); // 4
    }


    // RESERVE SPACE FOR later write Chunk's body size
    if (tp.m_LimitType == Arc::EChunkLim_U32 || tp.m_LimitType == Arc::EChunkLim_Unlim)
    {
        ar.U32_L(outChunk.m_Size); // 4
    }
    else if (tp.m_LimitType == Arc::EChunkLim_U24)
    {
        ar.U16_L(outChunk.m_Size);
        uint32_t bHi = ar.U8_L();
        outChunk.m_Size += (uint32_t)bHi << 16;
    }
    else if (tp.m_LimitType == Arc::EChunkLim_U16)
    {
        ar.U16_L(outChunk.m_Size);
    }
    else if (tp.m_LimitType == Arc::EChunkLim_U8)
    {
        ar.U8_L(outChunk.m_Size);
    }
    else
    {
        assert(0 && "NOT IMPLEMENTED");
        throw Exception(EException::OPERATION_ERR, "Unknown Chunk Type size:%u",
            (uint32_t)inChunk.m_ChunkInfo.size_of());
    }
    if (pOutChunk)
        *pOutChunk = outChunk; // DEEP COPY
}



void CArchiveBin::_arSaveChunkBegin(const qd::IFileChunk& FileChunk)
{
    if (!isStoring())
        throw qd::Exception(EException::IO_ERROR, "Archive::FAILED : memFile must be opened for writing");

    m_pFile->addNumChunks(+1);
    CArchive& ar = getAr();

    Arc::ChunkInfo_t tp = FileChunk.m_ChunkInfo;

    // SAVE CHUNK ID
    if (tp.m_IDType == Arc::EChunkID_U32)
    {
        ar << ((uint32_t)FileChunk.m_ID);
    }
    else if (tp.m_IDType == Arc::EChunkID_U16)
    {
        assert(FileChunk.m_ID < 0xffff);
        ar << ((uint16_t)FileChunk.m_ID);
    }
    else if (tp.m_IDType == Arc::EChunkID_U8)
    {
        assert(FileChunk.m_ID < 0xff);
        ar << ((uint8_t)FileChunk.m_ID);
    }
    else if (tp.m_IDType == Arc::EChunkID_U0)
    {
    }
    else if (tp.m_IDType == Arc::EChunkID_STRING)
    {
#if _DEBUG
// 		uint8_t strLen = FileChunk._getChunkStrLen();
// 		THash32 stHash = StrHelper::hashFunc32_2C(&FileChunk.m_pIDStr[0], strLen);
// 		assert((stHash & 255u) == FileChunk.m_ID);
#endif // 0
        U8_S((uint8_t)FileChunk.m_ID); // save only 1byte hash
        // U8_S(strLen);
        //_arWrite_Buf(&FileChunk.m_pIDStr[0], strLen);
    }
    else
    {
        ASSERT_AND_DO(0, return, "CHUNK ID NOT IMPLEMENTED");
    }

    // SAVE VERSION
    if (tp.m_VerType == Arc::EChunkVer_U32)
    {
        ar << ((uint32_t)FileChunk.m_Version);
    }
    else if (tp.m_VerType == Arc::EChunkVer_U16)
    {
        assert(FileChunk.m_Version < 0xffff);
        ar << ((uint16_t)FileChunk.m_Version);
    }
    else if (tp.m_VerType == Arc::EChunkVer_U8)
    {
        assert(FileChunk.m_Version < 0xff);
        ar << ((uint8_t)FileChunk.m_Version);
    }

    // CHUNK BODY SIZE
    if (tp.m_LimitType == Arc::EChunkLim_U32 || tp.m_LimitType == Arc::EChunkLim_Unlim)
    {
        ar.U32_S(0); // RESERVERD 4 BYTES FOR DATA_SIZE
    }
    else if (tp.m_LimitType == Arc::EChunkLim_U24)
    {
        ar.U16_S(0); // RESERVE 3 BYTES FOR DATA_SIZE
        ar.U8_S(0);
    }
    else if (tp.m_LimitType == Arc::EChunkLim_U16)
    {
        ar.U16_S(0); // RESERVE 2 BYTES FOR DATA_SIZE
    }
    else if (tp.m_LimitType == Arc::EChunkLim_U8)
    {
        ar.U8_S(0); // RESERVE 1 BYTES FOR DATA_SIZE
    }

    TThis::ChunkItem_t ChunkItem(FileChunk, m_pFile->tell());
    m_ChunkStack.push_back(ChunkItem);
}


void CArchiveBin::_arSaveChunkEnd()
{
    assert(c_def(this));
    if (!isStoring())
        throw Exception(EException::IO_ERROR, "Archive::FAILED : memFile must be opened for writing");

    if (m_ChunkStack.empty())
        throw Exception(EException::IO_ERROR, "FAILED : No opened chunks in stack");

    if (m_pFile->addNumChunks(-1) < 0)
    { // CHECK NUMBERS OF FILE CHUNKS
        assert(0 && "EXCEPTION: Number of memFile Chunks FAILED!");
        throw Exception(EException::IO_ERROR, "Number of memFile Chunks FAILED!");
    }

    uint32_t nCurPos = m_pFile->tell();
    CArchive& ar = *this;
    const TThis::ChunkItem_t& ChunkItem = m_ChunkStack.back();
    const Arc::ChunkInfo_t& tp = ChunkItem.m_FileChunk.m_ChunkInfo;

    uint32_t bodySize32 = nCurPos - ChunkItem.m_nChunkPos;
    // Substitute Real Body size while CLOSE CHUNK
    if (tp.m_LimitType == Arc::EChunkLim_U32 || tp.m_LimitType == Arc::EChunkLim_Unlim)
    {
        m_pFile->seek(ChunkItem.m_nChunkPos - (32 / 8), EFileSeek::SET);
        ar.U32_S(bodySize32); // 32 bit BodySize
    }
    else if (tp.m_LimitType == Arc::EChunkLim_U24)
    {
        if (bodySize32 > 0xFFFFFF)
        {
            logErr("FAILED: To save Tiny File ChunkID: '0x%4.X' ChunkSize=%u ", ChunkItem.m_FileChunk.getId(),
                bodySize32)
                ->ASSERT_DLG();
            bodySize32 = 0xFFFFFF;
        }
        m_pFile->seek(ChunkItem.m_nChunkPos - (24 / 8), EFileSeek::SET);
        ar.U16_S(bodySize32); // save BodySize
        ar.U8_S(bodySize32 >> 16);
    }
    else if (tp.m_LimitType == Arc::EChunkLim_U16)
    {
        if (bodySize32 > 0xFFFF)
        {
            logErr("FAILED: Chunk4 content is too long to save. ChunkID: '0x%4.X' ChunkSize=%u ",
                ChunkItem.m_FileChunk.getId(), bodySize32)
                ->ASSERT_DLG();
            bodySize32 = 0xFFFF;
        }
        m_pFile->seek(ChunkItem.m_nChunkPos - (16 / 8), EFileSeek::SET);
        ar.U16_S(bodySize32); // save BodySize
    }
    else if (tp.m_LimitType == Arc::EChunkLim_U8)
    {
        if (bodySize32 > 0xFF)
        {
            logErr("FAILED: Chunk4 content is too long to save. ChunkID: '0x%4.X' ChunkSize=%u ",
                ChunkItem.m_FileChunk.getId(), bodySize32)
                ->ASSERT_DLG();
            bodySize32 = 0xFF;
        }
        m_pFile->seek(ChunkItem.m_nChunkPos - (8 / 8), EFileSeek::SET);
        ar.U8_S(bodySize32); // save BodySize
    }
    else
    {
        assert(0);
    }

    m_ChunkStack.pop_back();

    m_pFile->seek(nCurPos, EFileSeek::SET);
    c_def(0);
}


/*
CArchive& CArchive::operator<< (const qd::CStringW& StringW) {
    return operator<< (convertToUTF8(StringW));
}


CArchive& CArchive::operator>> (qd::CStringW& StringW) {
    qtd::string String;
    this->operator>> (String);
    StringW = convertFromUTF8(String);
    return *this;
}
*/



void CArchiveBin::_arWrite_Buf(const void* pSrc, uint32_t nBytes)
{
    assert(isStoring());
    assert(ptr2DW(pSrc) > 0x1000 && "SERIALIZE: Source pointer FROM nullptr");

    uint32_t nBytesWrited;
    nBytesWrited = m_pFile->write(pSrc, nBytes);

    if (nBytesWrited != nBytes)
        throw Exception(EException::IO_ERROR, "Serialization ERROR: Cannot write %d bytes, %d written", nBytes,
            nBytesWrited);
}


void CArchiveBin::_arRead_Buf(void* pDest, uint32_t nBytes)
{
    assert(isLoading());
    assert(ptr2DW(pDest) > 0x1000);

    uint32_t nBytesReaded = m_pFile->read(pDest, nBytes);

    if (nBytesReaded != nBytes)
        throw Exception(EException::IO_ERROR, "Serialization ERROR: Cannot read %d bytes, %d read", nBytes,
            nBytesReaded);
}




bool CArchiveBin::_arLoadChunkTryTest(qd::IFileChunk& FileChunk, uint32_t ChunkID /*= 0*/)
{
    if (isStoring())
        return false;

    auto oldPos = _arGetFilePos();
    uint32_t nSize = m_pFile->getSize();
    if ((oldPos + FileChunk.m_ChunkInfo.size_of()) > nSize)
        return false;

    bool bRes;
    QD_TRY
    {
        // Exception::CAssertLock a(false);
        bRes = loadChunk(FileChunk, ChunkID, false);
        c_def(0);
    }
    QD_CATCH(...)
    {
        bRes = false;
    };

    if (!bRes)
        _arSetFilePos(oldPos);

    return bRes;
}



bool CArchive::loadMark8Test(uint8_t ID)
{
    if (isStoring())
        return false;

    uint32_t oldPos = getFilePos();
    bool bRes;
    QD_TRY
    {
        // Exception::CAssertLock a(false);
        uint8_t loadID = U8_L();
        bRes = (loadID == ID);
    }
    QD_CATCH(...)
    {
        bRes = false;
    };
    if (!bRes)
        setFilePos(oldPos);
    return bRes;
}



bool CArchive::loadMark8Test(uint8_t FromID, uint8_t ToID, uint8_t* pOut /*= nullptr*/)
{
    uint8_t Mark = U8_L();
    if (pOut)
        *pOut = Mark;
    uint8_t dt = (uint8_t)(ToID - FromID); // OVERFLOW ALLOWED
    if ((Mark - FromID) > dt)
        return false;
    return true;
}



uint8_t CArchive::loadMark8Throw(uint8_t FromID, uint8_t ToID, uint8_t* pOut /*= nullptr*/)
{
    uint8_t Mark;
    if (!loadMark8Test(FromID, ToID, &Mark))
        QD_THROW_OR_DO(Exception(EException::IO_ERROR,
                          "Wrong SerialByteID Loaded. Expected From:'0x%2.X' To:'0x%2.X' Received: '0x%2.X'",
                          (uint32_t)FromID, (uint32_t)ToID, (uint32_t)Mark),
            return 0);
    if (pOut)
        *pOut = Mark;
    return Mark;
}



uint32_t CArchive::loadChunkVer(uint32_t ChunkID /*= 0 */)
{
    CFileChunk12 FileChunk;
    loadChunk(FileChunk, ChunkID, /*Throw*/ true);
    return FileChunk.getVer();
}



void CArchive::_onConstruct(qd::IArchiveFormat* pAr, IBaseFileIO* /*pFile*/, qd::ESaveLoad bStoring)
{
    assert(pAr);
    m_pAr = pAr;
    m_StoreMode = bStoring;
    m_UserFileVer = 0;
    // m_pSerialDocList = nullptr;
}



CArchive::~CArchive()
{
    // m_pSerialDocList = nullptr;
}



void CArchive::beginChunk(/*Inout*/qd::IFileChunk& Chunk)
{
    if (isStoring())
    {
        m_pAr->_arSaveChunkBegin(Chunk);
    }
    else
    {
        m_pAr->_arLoadChunkBegin(Chunk, &Chunk);
    }
}


void IFileChunk::rewind(CArchive& ar) const
{
    if (!ar.isLoading())
        return;

    ar.undo(*this);

    IFileChunk loadedCh = *this; // DEEP COPY
    ar.LoadChunk_(loadedCh, true);
    if (getId() != loadedCh.getId())
        throw Exception(EException::IO_ERROR, "Wrong Chunk ID. Expected '0x%4.X', Receive: '0x%4.X'",
            (uint32_t)loadedCh.getId(), (uint32_t)getId());
}




void IFileChunk::_setChunkStrID(qtd::string_view src)
{
    // class IFileChunk
    uint32_t nLen = (uint32_t)strnlen(src.data(), IFileChunk::MAX_CHUNK_LEN);
    m_ChunkStrIDLen = (uint8_t)nLen;
    memcpy(&m_pIDStr[0], src.data(), nLen);
    if (nLen != IFileChunk::MAX_CHUNK_LEN)
        m_pIDStr[nLen] = '\0';

    THash32 stHash = qd::fnv1aHash2(src.data(), src.size());
    m_ID = stHash & (255u);
}



IFileChunk::IFileChunk(qd::CArchive& ar, uint32_t ID, qd::Arc::EChunkType _chunkType)
    : m_ID(0)
    , m_Version(0)
    , m_Size(0)
    , m_ReadPos(0)
    , m_ChunkInfo(Arc::ChunkInfo_t::MakeBySizeOf(_chunkType))
{
    // assert(_SizeOf > CHUNK_SIZE_6 || (ID <= 0xFF)); // CHECK TINY CHUNK

    assert(ar.isLoading() && "Simple FileChunk - can't stored. User CFileChunkEx4 instead");

    if (ar.isLoading())
    {
        ar >> *this;
        if (m_ID != ID)
        {
            m_bWasException = true;
            throw Exception(EException::IO_ERROR, "Wrong Chunk ID. Expected '0x%4.X', Receive: '0x%4.X'", (uint32_t)ID,
                (uint32_t)m_ID);
        }
    }
}



IFileChunk::IFileChunk(const char* pIDStr, uint32_t nVersion, qd::Arc::EChunkType _chunkType)
    : m_Version(nVersion)
    , m_ChunkInfo(Arc::ChunkInfo_t::MakeBySizeOf(_chunkType))
{
    assert(m_ChunkInfo.m_IDType == Arc::EChunkID::EChunkID_STRING); // CHECK TINY CHUNK
    _setChunkStrID(pIDStr);
}



void IFileChunkScope::endChunk()
{
    if (!m_pArchive)
        return;

    bool bOk = false;
    QD_TRY
    {
        bOk = m_pArchive->endChunk(*this);
    }
    QD_CATCH(...)
    {
        bOk = false;
    };
#if defined(_DEBUG)
    if (!bOk)
    {
        if (!m_bWasException && m_pArchive->isLoading())
        {
            throw Exception(EException::IO_ERROR, "WARNING: Chunk%u ID:0x%4X not read fully!",
                (uint32_t)m_ChunkInfo.m_SizeOf, m_ID);
            //->THROW_ASSERT();
        }
        m_pArchive->skip(*this);
    }
#endif // _DEBUG

    m_pArchive = nullptr;
    c_def(0);
}



void IFileChunk::_serializeChunkID32(qd::CArchive& ar, uint32_t ID, uint32_t nVersion /*= 0*/, bool bCheckId /*= true*/)
{
    m_ID = ID;
    m_Version = nVersion;

    if (ar.isStoring())
    {
        ar.getArFormat()->_arSaveChunkBegin(*this);
        return;
    }
    else
    {
        if (!ar.loadChunk(*this))
        {
            m_bWasException = true;
            throw Exception(EException::IO_ERROR, "Error: Can't load Chunk!");
        }
        // compare ID32 chunks
        if (bCheckId && (m_ID != ID) && m_ChunkInfo.m_IDType != Arc::EChunkID_U0)
        {
            m_bWasException = true;
            throw Exception(EException::IO_ERROR, "Wrong Chunk ID Loaded. Expected '0x%4.X', Receive: '0x%4.X'",
                (uint32_t)ID, (uint32_t)m_ID);
        }
    }
}





void IFileChunk::_serializeChunkStr(qd::CArchive& ar, qtd::string_view pStrID, uint32_t nVersion /*= 0*/)
{

    _setChunkStrID(pStrID);
    // check ID by HashValue
    _serializeChunkID32(ar, m_ID, nVersion);

    // 	m_Version = nVersion;
    // 	if (ar.IsStoring()) {
    // 		ar.GetArFormat()->_arSaveChunkBegin(*this);
    // 		return;
    // 	}
    // 	else if (!ar.LoadChunk(*this)) {
    // 		m_bWasException = true;
    // 		throw qd::Exception(qd::EException::IO_ERROR, "Error: Can't load Chunk!");
    // 	}
    // 	// COMPARE String Chunks
    // 	if (std::strncmp(m_pIDStr, pStrID.getBuffer(), MAX_CHUNK_LEN) != 0) {
    // 		m_bWasException = true;
    // 		throw qd::Exception(qd::EException::IO_ERROR, "Wrong Chunk ID Loaded. Expected '%s', Receive: '%s'",
    // CC(pStrID), CC(m_pIDStr));
    // 	}
}






// 4 - bytes CHUNK
// ChunkID = 8 Bit; nVer = 8 bit; BodySize = 16bit( 65535 bytes max)
uint8_t CArchive::BeginChunk4(uint8_t ID, uint8_t nVersion /*= 0*/)
{
    CFileChunk4 c(ID, nVersion);
    if (isStoring())
    {
        m_pAr->_arSaveChunkBegin(c);
    }
    else
    {
        loadChunk(c, c.getId(), /*bThrow:*/ true);
    }
    return c.getVer8();
}


uint32_t CArchive::beginChunk12(uint32_t ID, uint32_t nVersion /*= 0*/)
{
    CFileChunk12 c(ID, nVersion);
    if (isStoring())
    {
        m_pAr->_arSaveChunkBegin(c);
    }
    else
    {
        loadChunk(c, c.getId(), /*bThrow:*/ true);
    }
    return c.getVer();
}


uint8_t CArchive::BeginChunk6(uint8_t ID, uint8_t nVersion /*= 0*/)
{
    CFileChunk6 c(ID, nVersion);
    if (isStoring())
    {
        m_pAr->_arSaveChunkBegin(c);
    }
    else
    {
        loadChunk(c, c.getId(), /*bThrow:*/ true);
    }
    return c.getVer8();
}


// 5 - bytes CHUNK
// ChunkID = 8 Bit; nVer = 8 bit; ReadPos = 24bit;
uint8_t CArchive::BeginChunk5(uint8_t ID, uint8_t nVersion /*= 0*/)
{
    CFileChunk5 c(ID, nVersion);
    if (isStoring())
    {
        m_pAr->_arSaveChunkBegin(c);
    }
    else
    {
        loadChunk(c, c.getId(), /*bThrow:*/ true);
    }
    return c.getVer8();
}



bool CArchive::loadChunk(IFileChunk& outFileChunk, uint32_t checkChunkID /*= 0*/, bool bThrow /*= true*/)
{
    assert(isLoading());
    // throw Exception(EException::OPERATION_ERR, "Archive is not in Loading mode");

    IFileChunk origFileChunk = outFileChunk; // DEEP COPY ORIGINAL
    QD_TRY
    {
        m_pAr->_arLoadChunkBegin(origFileChunk, &outFileChunk);
    }
    QD_CATCH(std::exception&)
    {
        outFileChunk.m_bWasException = true;
        if (bThrow)
            throw;
        return false;
    };

    if (origFileChunk.m_ID != 0 && origFileChunk.m_ID != outFileChunk.m_ID)
    {
        outFileChunk.m_bWasException = true;
        if (bThrow)
            throw Exception(EException::IO_ERROR, "Wrong Chunk ID Loaded. Expected '0x%4.X', Receive: '0x%4.X'",
                checkChunkID, outFileChunk.m_ID);
        return false;
    }
    return true;
}





}; // namespace qd
