#include "qd/file/memFile.h"
#include "qd/debug/exception.h"


namespace qd {


MemFile::MemFile(uint32_t nReserved) {
    m_pMemData = new MemData(nReserved);
}


MemFile::MemFile(qd::IFile* pFile, uint32_t nSize) {

    if (nSize) {
        uint32_t nMaxBytes = nSize;
        m_pMemData = new MemData(nMaxBytes);

        if (pFile->read(m_pMemData->getBuffer(), nSize) != nSize)
            QD_THROW_OR_DO(Exception(EException::IO_ERROR, "CMemoryFile: Can't Read \"%d\" bytes from AbstractFile:%s", nMaxBytes,
                CC(pFile->getFileName())), return);
    }
    _setOpened(true);
}


void MemFile::expandBuffer(uint32_t nSize, bool bExactSize) {

    if (getCapacity() > nSize)
        return;
    uint32_t nMaxSize = nSize;
    if (bExactSize == false) {
        nMaxSize = 128;
        while (nSize > nMaxSize)
            nMaxSize = nMaxSize + nMaxSize / 2;
    }

    m_pMemData->expandBuffer(nMaxSize);
}


uint32_t MemFile::read(void* pDest, uint32_t nBytes) {

    assert(m_pMemData && getCapacity() >= getFileSize());
    assert(m_Position <= getFileSize() && "Memory File out of Bounds!");

    uint32_t maxBytes = getFileSize() - m_Position;
    if (nBytes > maxBytes)
        nBytes = maxBytes;

    if (nBytes > 0) {
        uint8_t* pSrc = (uint8_t*)m_pMemData->getBuffer(m_Position);
        memcpy(pDest, pSrc, nBytes);
        m_Position += nBytes;
    }

    return nBytes;
}


uint32_t MemFile::write(const void* pSrc, uint32_t nBytes) {

    uint32_t lastByte = m_Position + nBytes;

    if (lastByte >= getCapacity())
        expandBuffer(lastByte, /*bExactSize*/ false);

    uint8_t* pDest = (uint8_t*)m_pMemData->getBuffer(m_Position);
    memcpy(pDest, pSrc, nBytes);

    m_Position += nBytes;

    if (m_Position > m_pMemData->getSize())
        m_pMemData->setSize(m_Position); // expand filesize

    return nBytes;
}


void MemFile::compact() {
    if (!m_pMemData)
        return;

    // REDUCE BUFFER
    m_pMemData->expandBuffer(getFileSize() /*,bExactSize*/);
}


uint32_t MemFile::seek(uint32_t Position, EFileSeek Where /*= SEEK_SET*/) {
    switch (Where) {
    case EFileSeek::SET:
        m_Position = Position;
        break;
    case EFileSeek::CUR:
        m_Position += Position;
        break;
    case EFileSeek::END:
        m_Position = getFileSize() - Position;
        break;
    default:
        assert(0 && "Bad Seek Parameter");
        break;
    }

    return m_Position;
}



void MemData::expandBuffer(uint32_t nCapacity) {
    m_pMemBuf->expandBuffer(nCapacity);
}



void MemData::write(const void* pSrc, uint32_t nBytes) {
    uint32_t nOffset = m_nUsedSize;
    uint32_t nNewSize = nOffset + nBytes;
    if (nNewSize > getCapacity()) {
        expandBuffer(nNewSize);
    }
    m_pMemBuf->copyFrom(pSrc, nBytes, nOffset);
    m_nUsedSize = nNewSize;
}


void MemBuf::copyFrom(const void* pSrc, uint32_t nBytes, uint32_t nToOffset /*= 0 */) {

    if ((nToOffset + nBytes) <= m_bufSize) {
        assert(0 && "MemBuf - Out of buffer!");
        return;
    }
    ASSERT_F(m_pBuffer, "MemBuf - buffer is Null");

    memcpy(m_pBuffer + nToOffset, pSrc, (size_t)nBytes);
}
} // namespace qd
