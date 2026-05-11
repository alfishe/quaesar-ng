#pragma once
#include "qd/base/base.h"
#include "qd/file/fileBase.h"
#include "qd/mem/memBuffer.h"


namespace qd {

//////////////////////////////////////////////////////////////////////////
// INHERITS - REGULAR FILE API
// STORES DATA IN MEMORY
class MemFile final : public qd::IFile
{
    typedef MemFile TThis;
    ref_ptr<MemData> m_pMemData;
    uint32_t m_Position = 0;
    bool m_bMemDataWasRetained = false;

public:
    explicit MemFile(qd::IFile* pFile, uint32_t nBytes);
    explicit MemFile(uint32_t nReserved);

    MemFile(MemFile&& r) { // REFERENCED COPY
        m_pMemData = r.m_pMemData;
        m_bMemDataWasRetained = r.m_bMemDataWasRetained;
        m_Position = r.m_Position;
        r.m_pMemData = nullptr;
    }

    explicit MemFile(void* pData, uint32_t nBytes, bool bFreeBuffer) {
        m_pMemData = new MemData(new MemBuf(pData, nBytes, bFreeBuffer), nBytes);
        m_Position = 0;
    }

    explicit MemFile(const MemData& MemData) {
        setMemBuf(MemData.getMemBuf(), MemData.getSize()); // get referenced only inner buffer
    }
    explicit MemFile(MemData& MemData) {
        MemData.ref_ptr_retain();
        m_bMemDataWasRetained = true; // RETAIN LIFE COUNTER to not delete
        setMemData(&MemData);
    }

    explicit MemFile(const ref_ptr<MemBuf>& pBuf, uint32_t nSize) { setMemData(new MemData(pBuf, nSize)); }

    MemFile() { freeBuf(); }

    // COPY OPERATOR
    // const MemFile& operator = (const MemFile& r) {
    //	SetMemData(r.m_pMemData, r.m_Position);
    //	return *this;
    //}

    void setMemBuf(const ref_ptr<MemBuf>& pBuf, uint32_t nSize = 0) {
        m_Position = 0;
        if (!m_pMemData) {
            m_pMemData = new MemData(pBuf, nSize);
            return;
        }
        m_pMemData->setMemBuf(pBuf, nSize);
        assert(getSize() <= getCapacity());
    }

    void setMemData(ref_ptr<MemData> pNewMemData, uint32_t nPos = 0) {
        if (m_pMemData) {
            MemData* pOldMemData = m_pMemData.get();
            m_pMemData.reset();
            if (m_bMemDataWasRetained) {
                pOldMemData->ref_ptr_release();
                m_bMemDataWasRetained = false;
            }
        }
        m_pMemData = qtd::move(pNewMemData);
        m_Position = nPos;
    }

    virtual ~MemFile() { setMemData(nullptr); }

    void freeBuf() { setMemData(nullptr); }

    TThis* reset() {
        m_Position = 0;
        return this;
    }

    void expandBuffer(uint32_t nSize, bool bExactSize = false);

    virtual uint32_t seek(uint32_t Position, qd::EFileSeek Where = EFileSeek::SET) override;
    virtual inline uint32_t tell() override { return m_Position; }
    virtual uint32_t read(void* pDest, uint32_t nBytes) override;
    virtual uint32_t write(const void* pSrc, uint32_t nBytes) override;

    void compact();

    inline uint8_t* getData() const {
        if (!m_pMemData)
            return (uint8_t*)nullptr;
        return (uint8_t*)m_pMemData->getBuffer();
    }
    inline uint8_t* getBuffer() const { return getData(); }

    inline uint8_t* getCurBuffer() const {
        if (!m_pMemData)
            return (uint8_t*)nullptr;
        return (uint8_t*)m_pMemData->getBuffer(m_Position);
    }

    const ref_ptr<MemBuf>& getMemBuf() const { return m_pMemData->getMemBuf(); }

    inline MemData getMemoryData() const { return MemData(m_pMemData->getMemBuf(), m_pMemData->getSize()); }

    inline const ref_ptr<MemData>& getMemoryDataPtr() const { return m_pMemData; }

    inline uint32_t getFileSize() const {
        if (!m_pMemData)
            return 0;
        return m_pMemData->getSize();
    }

    virtual inline uint32_t getSize() override { return getFileSize(); }

    uint32_t getRemFileSize() const {
        assert(m_pMemData);
        uint32_t nFileSize = getFileSize();
        if (m_Position >= nFileSize)
            return 0;
        return nFileSize - m_Position;
    }

    inline uint32_t getCapacity() const {
        if (!m_pMemData)
            return 0;
        return m_pMemData->getCapacity();
    }

    uint32_t getRemCapacity() const {
        if (!m_pMemData)
            return 0;
        uint32_t nCapacity = m_pMemData->getCapacity();
        if (m_Position > nCapacity) {
            assert(0 && "Bad size");
            return 0;
        }
        return nCapacity - m_Position;
    }


    TThis* setSize(uint32_t nFileSize) {
        assert(nFileSize <= getCapacity());
        m_pMemData->setSize(nFileSize);
        return this;
    }


    inline uint32_t getPosition() const { return m_Position; }
    inline void setPosition(uint32_t Position) { m_Position = Position; }

}; // class MemFile
//////////////////////////////////////////////////////////////////////////


}; // namespace qd
