#pragma once

#include "fileBase.h"
#include "qd/base/base.h"
#include "qd/base/baseTypes.h" // SINGLETON
#include "qd/base/tribool.h"
#include "qd/debug/assert.h"
#include "qd/debug/exception.h"
#include "qd/stl/string.h"


#ifndef QD_FILE_ARCHIVE_INCLUDED
#define QD_FILE_ARCHIVE_INCLUDED
#endif


FORWARD_DECLARATION_4S(qd, Arc, Private, CArchiveNamedArgScope);


namespace qd {
class CArchive;
class IBaseFileIO;
class MemFile;

struct CSerializeContext {
    uint32_t m_nVer = 0;

public:
    CSerializeContext() {}
    CSerializeContext(qd::CArchive& ar) {}
}; // struct CSerializeContext
//////////////////////////////////////////////////////////////////////////




namespace Serializers {
struct OpShift {
    // typedef /*typename*/ TObject value_type;
    template<typename TObject, class TArchive>
    inline static void Store(TArchive& ar, const TObject& Obj)
    {
        ar << Obj;
    }
    template<typename TObject, class TArchive>
    inline static void Load(TArchive& ar, TObject& Obj)
    {
        ar >> Obj;
    }
}; // struct ShiftTypeSerialize


struct OpPointSer {
    template<typename TObject>
    inline static void Store(qd::CArchive& ar, /*typename*/ TObject& Obj)
    {
        Obj.serialize(ar);
    }

    template<typename TObject>
    inline static void Load(qd::CArchive& ar, /*typename*/ TObject& Obj)
    {
        Obj.serialize(ar);
    }
};
} // namespace Serializers
//////////////////////////////////////////////////////////////////////////



namespace Arc {
enum EChunkID : uint8_t {
    EChunkID_Undef = 0,
    EChunkID_U0, // zero byte ID
    EChunkID_U8,
    EChunkID_U16,
    EChunkID_U32,
    EChunkID_STRING,
    EChunkID_S1,
    EChunkID_S2,
    EChunkID_S3,
    EChunkID_S4,
};

enum EChunkVer : uint8_t {
    EChunkVer_Undef = 0,
    EChunkVer_U0,
    EChunkVer_U8,
    EChunkVer_U16,
    EChunkVer_U32,
};

enum EChunkLim : uint8_t {
    EChunkLim_Undef = 0,
    EChunkLim_Unlim, // unlimited for string format
    EChunkLim_U32, // max 4gb
    EChunkLim_U24, // max (16Mb)
    EChunkLim_U16, // max 64kb
    EChunkLim_U8, // 256 byte for named
};

enum EChunkType : uint8_t {
    // -ID-VER-BODY-
    U0_U0_U16 = 2, // ONLY ReadPos (16 bit) withoud ID and Version
    U8_U0_U16 = 3, // ChunkID = 8 Bit; nVer = 0 bit; ReadPos = 16bit;
    U8_U8_U16 = 4, // ChunkID = 8 Bit; nVer = 8 bit; ReadPos = 16bit;(64kb)
    U8_U8_U24 = 5, // ChunkID = 8 Bit; nVer = 8 bit; ReadPos = 24bit;(16Mb)
    U8_U8_U32 = 6, // ChunkID = 8 Bit; nVer = 8 bit; ReadPos = 32bit;(2Gb)
    U0_U8_U24 = 7, // ChunkID = 0 Bit; nVer = 8 bit; ReadPos = 24bit;(16Mb)
    U0_U8_U16 = 8, // ChunkID = 0 Bit; nVer = 8 bit; ReadPos = 16bit;(64Mb)
    U16_U0_U16 = 9, // ChunkID = 16 Bit; nVer = 0 bit; ReadPos = 16bit;(64Mb)
    U16_U1_U16 = 10, // ChunkID = 16 Bit; nVer = 0 bit; ReadPos = 16bit;(64Mb)
    U32_U0_U16 = 11, // ChunkID = 16 Bit; nVer = 0 bit; ReadPos = 16bit;(64Mb)
    ID32_V32_P32 = 12,
    STR_U8_U32 = 128, // ChunkID = string; nVer = 8 bit; ReadPos = 32bit; == qd::Arc::CChunkScopeS8
    STR_U32_UFF = 129, // { Arc::EChunkID_STRING, Arc::EChunkVer_U32, Arc::EChunkLim_Unlim, 0 };
    STR_U0_U8 = 130, // ChunkID = string; nVer = 0 bit; ReadPos = 32bit;
}; // enum EChunkType


struct ChunkInfo_t {
    qd::Arc::EChunkType m_ChunkType;
    qd::Arc::EChunkID m_IDType;
    qd::Arc::EChunkVer m_VerType;
    qd::Arc::EChunkLim m_LimitType;
    uint8_t m_SizeOf; // CHUNK STRUCT SIZE ON DISK

    uint8_t size_of() const { return m_SizeOf; }

    // TEST IS OPTIMIZED ?
    static constexpr ChunkInfo_t MakeBySizeOf(qd::Arc::EChunkType chSize)
    {
        switch (chSize)
        {
        case EChunkType::ID32_V32_P32:
            return {chSize, Arc::EChunkID_U32, Arc::EChunkVer_U32, Arc::EChunkLim_U32, 12};
        case EChunkType::U8_U8_U32:
            return {chSize, Arc::EChunkID_U8, Arc::EChunkVer_U8, Arc::EChunkLim_U32, 6};
        case EChunkType::U8_U8_U24:
            return {chSize, Arc::EChunkID_U8, Arc::EChunkVer_U8, Arc::EChunkLim_U24, 5};
        case EChunkType::U8_U8_U16:
            return {chSize, Arc::EChunkID_U8, Arc::EChunkVer_U8, Arc::EChunkLim_U16, 1 + 1 + 2};
        case EChunkType::U8_U0_U16:
            return {chSize, Arc::EChunkID_U8, Arc::EChunkVer_U0, Arc::EChunkLim_U16, 3};
        case EChunkType::U0_U0_U16:
            return {chSize, Arc::EChunkID_U0, Arc::EChunkVer_U0, Arc::EChunkLim_U16, 2};
        case EChunkType::U0_U8_U24:
            return {chSize, Arc::EChunkID_U0, Arc::EChunkVer_U8, Arc::EChunkLim_U24, 0 + 1 + 3};
        case EChunkType::U0_U8_U16:
            return {chSize, Arc::EChunkID_U0, Arc::EChunkVer_U8, Arc::EChunkLim_U16, 0 + 1 + 2};
        case EChunkType::U16_U0_U16:
            return {chSize, Arc::EChunkID_U16, Arc::EChunkVer_U0, Arc::EChunkLim_U16, 2 + 0 + 2};
        case EChunkType::U16_U1_U16:
            return {chSize, Arc::EChunkID_U16, Arc::EChunkVer_U8, Arc::EChunkLim_U16, 2 + 1 + 2};
        case EChunkType::U32_U0_U16:
            return {chSize, Arc::EChunkID_U32, Arc::EChunkVer_U0, Arc::EChunkLim_U16, 4 + 0 + 2};
        case EChunkType::STR_U8_U32:
            return {chSize, Arc::EChunkID_STRING, Arc::EChunkVer_U8, Arc::EChunkLim_U32, 1 + 1 + 4};
        case EChunkType::STR_U0_U8:
            return {chSize, Arc::EChunkID_STRING, Arc::EChunkVer_U0, Arc::EChunkLim_U8, 1 + 0 + 1};
        case EChunkType::STR_U32_UFF:
            return {chSize, Arc::EChunkID_STRING, Arc::EChunkVer_U32, Arc::EChunkLim_U32, 1 + 4 + 4};
        default:
            throw qd::Exception("unknown chunk type");
            break;
        }
    }
};

struct ChunkID_t {
    uint32_t m_ID32 = 0;
    qd::string m_IDStr;
    EChunkID m_Type = EChunkID_Undef;
}; // struct ChunkID_t

}; // namespace Arc
//////////////////////////////////////////////////////////////////////////


class IFileChunk
{
    friend class CArchive;

public:
    static constexpr uint8_t MAX_CHUNK_LEN = 16;
    uint32_t m_ID; // or HashFrom String
    uint32_t m_Version = 0;
    uint32_t m_Size = 0; // skip size of chunk
    uint32_t m_ReadPos = qd::_noPos; // start position of chunk
    uint32_t m_StackLevel = 0; // chunk hierarchy level
    qd::Arc::ChunkInfo_t m_ChunkInfo;
    char m_pIDStr[MAX_CHUNK_LEN]; // chunkID including Zero
protected:
    bool m_bWasException = false; // TO NOT DROP ASSERTS IN DESTRUCTOR
    uint8_t m_ChunkStrIDLen = 0;

public:
    inline IFileChunk(const IFileChunk& r) = default;

    explicit IFileChunk(qd::Arc::EChunkType _chunkType)
        : m_ID(0)
        , m_ChunkInfo(Arc::ChunkInfo_t::MakeBySizeOf(_chunkType))
    {}

    explicit IFileChunk(uint32_t ID, uint32_t nVersion, qd::Arc::EChunkType _chunkType)
        : m_ID(ID)
        , m_Version(nVersion)
        , m_ChunkInfo(Arc::ChunkInfo_t::MakeBySizeOf(_chunkType))
    {
        // assert( _chunkType > EChunkType::U8_U8_U32 || (m_ID <= 0xFF && m_Version <= 0xFF) ); // CHECK TINY CHUNK
    }

    explicit IFileChunk(const char* pIDStr, uint32_t nVersion, qd::Arc::EChunkType _chunkType);

    // ID:STRING, VER:8, MAX_SIZE:NONE
    static IFileChunk S_U1_N(const char* pIDStr, uint8_t nVersion = 0)
    {
        IFileChunk ch(pIDStr, nVersion, Arc::EChunkType::STR_U8_U32);
        return ch;
    }

    // ID:STRING, VER:8, MAX_SIZE:NONE
    static qd::IFileChunk U4_U4_U4(uint32_t ID, uint32_t nVersion = 0)
    {
        qd::IFileChunk ch(Arc::EChunkType::ID32_V32_P32);
        ch.m_ID = ID;
        ch.m_Version = nVersion;
        return ch;
    }


    void setChunk(uint32_t ID, uint32_t nVersion, qd::Arc::EChunkType _chunkType /*= qd::Arc::EChunkType::U8_U8_U16*/)
    {
        m_ID = ID;
        m_Version = nVersion;
        m_ChunkInfo = Arc::ChunkInfo_t::MakeBySizeOf(_chunkType);
        m_bWasException = false;
        m_ReadPos = qd::_noPos;
        m_Size = 0;
    }

    void setChunk(const qd::IFileChunk& _ch)
    {
        *this = _ch; // DEEP COPY
    }

    const qd::Arc::ChunkInfo_t& GetChunkInfo() const { return m_ChunkInfo; }

    // Loads chunk without testing
    inline explicit IFileChunk(qd::CArchive& ar, qd::Arc::EChunkType _SizeOf);

    // Loads chunk and test his ID, if ID not match throw IO_EXCEPTION
    explicit IFileChunk(qd::CArchive& ar, uint32_t ID, qd::Arc::EChunkType _SizeOf);

    /*virtual*/ ~IFileChunk() {}

    inline uint32_t getId() const { return m_ID; }
    inline void setId(uint32_t val) { m_ID = val; }

    inline uint32_t getSize() const { return m_Size; }
    inline uint32_t getVer() const { return m_Version; }
    inline uint8_t getVer8() const { return (uint8_t)m_Version; }
    inline uint32_t getVersion() const { return getVer(); }
    inline operator uint32_t () const { return getVersion(); }

    uint32_t getReadPos() const { return m_ReadPos; }

    inline void setVer(uint32_t val) { m_Version = val; }

    inline void skip(CArchive& ar) const;

    inline void undo(CArchive& ar) const;

    void rewind(CArchive& ar) const;

    inline void check(CArchive& ar) const;

    inline void checkSafe(CArchive& ar) const;

    void _setChunkStrID(string_view src);

    inline uint8_t _getChunkStrLen() const
    {
        return m_ChunkStrIDLen; // (uint)strnlen_s(m_pIDStr, IFileChunk::MAX_CHUNK_LEN);
    }

    qd::string_view getChunkStrIDRef() const { return qd::string_view(&m_pIDStr[0], m_ChunkStrIDLen); }

    void _serializeChunkID32(qd::CArchive& ar, uint32_t ID, uint32_t nVersion = 0, bool bCheckId = true);
    void _serializeChunkStr(qd::CArchive& ar, string_view pStrID, uint32_t nVersion /*= 0*/);

}; // class IFileChunk
//////////////////////////////////////////////////////////////////////////






//////////////////////////////////////////////////////////////////////////
class IArchiveFormat
{
protected:
    qd::IBaseFileIO* m_pFile = nullptr;

public:
    qd::ESaveLoad m_StoreMode;

    IArchiveFormat(qd::IBaseFileIO* pFile, qd::ESaveLoad bStoreMode)
        : m_StoreMode(bStoreMode)
    {
        m_pFile = pFile;
    }

    virtual ~IArchiveFormat() {}

    qd::IBaseFileIO* getFile() const { return m_pFile; }

    void setFile(qd::IBaseFileIO* pFile);

    virtual qd::string_view _arGetFileName() { return "AR_UNKNOWN_FILE"; }

    virtual bool _arLoadChunkTryTest(IFileChunk& FileChunk, uint32_t ChunkID = 0)
    {
        _err_NotSupported();
        return false;
    }

    virtual void _arSaveChunkBegin(const qd::IFileChunk& Chunk) { _err_NotSupported(); }
    virtual void _arSaveChunkEnd() { _err_NotSupported(); }

    virtual void _arLoadChunkBegin(const qd::IFileChunk& FileChunk, qd::IFileChunk* pOutChunk) { _err_NotSupported(); }
    virtual bool _arLoadChunkEnd(const qd::IFileChunk* pChunk = nullptr)
    {
        _err_NotSupported();
        return false;
    }

    virtual void _arSkip(const IFileChunk& Chunk) { _err_NotSupported(); }
    virtual void _arUndo(const IFileChunk& Chunk) { _err_NotSupported(); }

    virtual uint32_t _arGetFilePos()
    {
        _err_NotSupported();
        return 0;
    }
    virtual void _arSetFilePos(uint32_t Pos) { _err_NotSupported(); };

    virtual void _arSkipBytes(uint32_t nBytes) { _err_NotSupported(); }

    // save raw buffer
    virtual void _arRead_Buf(void* pDest, uint32_t nBytes) { _err_NotSupported(); }
    virtual void _arWrite_Buf(const void* pSrc, uint32_t nBytes) { _err_NotSupported(); }

    // MAY BE SWAPPED DUE TO ENDIANS
    virtual void _arRead_Int(void* pDest, uint32_t nBytes) { _err_NotSupported(); }
    virtual void _arRead_UInt(void* pDest, uint32_t nBytes) { _err_NotSupported(); }
    virtual void _arRead_Float(void* pDest, uint32_t nBytes) { _err_NotSupported(); }

    // WRITERS FOR SWAP SRC DATA
    virtual void _arWrite_Int(const void* pSrc, uint32_t nBytes) { _err_NotSupported(); }
    virtual void _arWrite_UInt(const void* pSrc, uint32_t nBytes) { _err_NotSupported(); }
    virtual void _arWrite_Float(const void* pSrc, uint32_t nBytes) { _err_NotSupported(); }

    // strings
    virtual void _arRead_String(qd::string& Dest) { _err_NotSupported(); }
    virtual void _arWrite_String(const char* pSrc, uint32_t nBytes) { _err_NotSupported(); }
    virtual void _arWrite_StringW(const wchar_t* pSrc) { _err_NotSupported(); }

    virtual void _arFlush() {}
    virtual void _arReset() {}

    virtual uint32_t _arGetNumOpenChunks()
    {
        _err_NotSupported();
        return 0;
    }

protected:
    void _err_NotSupported() { assert(0 && "SERIALIZE OPERATOR NOT IMPLEMENTED"); }

}; // class IArchiveFormat
//////////////////////////////////////////////////////////////////////////




//////////////////////////////////////////////////////////////////////////
class CArchive
{
    typedef CArchive TThis;

protected:
    uint32_t m_UserFileVer;
    qd::ESaveLoad m_StoreMode;
    qd::IArchiveFormat* m_pAr; // ARCHIVE FILE FORMAT
public:
    CArchive(qd::IArchiveFormat* pAr, qd::IBaseFileIO* pFile, qd::ESaveLoad bStoring = ESaveLoad::Load)
    {
        _onConstruct(pAr, pFile, bStoring);
    }

    void _onConstruct(qd::IArchiveFormat* pAr, qd::IBaseFileIO* pFile, ESaveLoad bStoring);

    virtual ~CArchive();


    uint32_t GetNumOpenChunks()
    {
        return m_pAr->_arGetNumOpenChunks(); // m_ChunkStack.size();
    }

    uint32_t getUserFileVer() const { return m_UserFileVer; }

    uint32_t setUserFileVer(uint32_t UserFileVer)
    {
        uint32_t tmp = m_UserFileVer;
        m_UserFileVer = UserFileVer;
        return tmp;
    }

    uint32_t beginChunk12(uint32_t ID, uint32_t nVersion = 0);
    uint8_t BeginChunk6(uint8_t ID, uint8_t nVersion = 0); // ChunkID = 8 Bit; nVer = 8 bit; ReadPos = 32bit;(2Gb)
    uint8_t BeginChunk5(uint8_t ID, uint8_t nVersion = 0); // ChunkID = 8 Bit; nVer = 8 bit; ReadPos = 24bit;(16Mb)
    uint8_t BeginChunk4(uint8_t ID, uint8_t nVersion = 0); // return VERSION OF CHUNK

    inline void _arSaveChunkBegin(uint32_t ID, uint32_t nVersion) { beginChunk12(ID, nVersion); }

    void beginChunk(const qd::IFileChunk& inChunk)
    {
        if (isStoring())
            m_pAr->_arSaveChunkBegin(inChunk);
        else
            m_pAr->_arLoadChunkBegin(inChunk, nullptr); // CONST CHUNK
    }

    void beginChunk(_Inout_ qd::IFileChunk& inChunk);

    bool endChunk(qd::IFileChunk& inChunk)
    { // RETURN FALSE IF IT WASN'T READED
        if (isStoring())
        {
            m_pAr->_arSaveChunkEnd();
            return true;
        }
        else
        {
            bool bReadedFullly = m_pAr->_arLoadChunkEnd(&inChunk);
            return bReadedFullly;
        }
    }

    qd::IFileChunk beginChunk(const char* pChunkID, uint8_t nVer = 0)
    {
        qd::IFileChunk strChunk(pChunkID, nVer, qd::Arc::EChunkType::STR_U8_U32);
        if (isStoring())
            m_pAr->_arSaveChunkBegin(strChunk);
        else
            m_pAr->_arLoadChunkBegin(strChunk, &strChunk);
        return strChunk;
    }

    bool endChunk()
    { // COMMON END CHUNK
        if (isStoring())
        {
            m_pAr->_arSaveChunkEnd();
            return true;
        }
        else
        {
            bool bReadedFullly = m_pAr->_arLoadChunkEnd(nullptr);
            return bReadedFullly;
        }
    }


    bool loadChunk(_Inout_ IFileChunk& FileChunk, uint32_t checkChunkID = 0, bool bThrow = true);

    inline bool LoadChunk_(const IFileChunk& FileChunk, bool bThrow = true)
    {
        IFileChunk tmpChunk(FileChunk);
        return loadChunk(tmpChunk, FileChunk.getId(), bThrow);
    }
    bool loadChunkTryTest(IFileChunk& FileChunk) { return m_pAr->_arLoadChunkTryTest(FileChunk); }
    bool loadMark8Test(uint8_t ID);
    bool loadMark8Test(uint8_t FromID, uint8_t ToID, uint8_t* pOut = nullptr);
    uint8_t loadMark8Throw(uint8_t FromID, uint8_t ToID, uint8_t* pOut = nullptr);

    uint32_t loadChunkVer(uint32_t ChunkID = 0);

    inline int LoadInt()
    {
        int iVal;
        (*this) >> iVal;
        return iVal;
    }

    inline const qd::ESaveLoad& getStoreMode() { return m_StoreMode; }
    inline bool isLoading() const { return m_StoreMode.isLoad(); }
    inline bool isStoring() const { return m_StoreMode.IsSave(); }

    // Read / Store
    template<typename CType>
    inline TThis& serializeSelf(CType& v)
    {
        CArchive& ar(*this);
        if (isStoring())
            ar << v;
        else
            ar >> v;
        return *this;
    }
    template<typename TType>
    inline CArchive& operator() (TType& v)
    {
        return serializeSelf(v);
    }

    inline CArchive& operator<< (unsigned char c)
    {
        m_pAr->_arWrite_Buf(&c, 1);
        return *this;
    }
    inline CArchive& operator<< (signed char c)
    {
        m_pAr->_arWrite_Buf(&c, 1);
        return *this;
    }
    inline CArchive& operator<< (char c)
    {
        m_pAr->_arWrite_Buf(&c, 1);
        return *this;
    }
    inline CArchive& operator<< (bool b)
    {
        uint8_t v = b ? 1u : 0u;
        m_pAr->_arWrite_Buf(&v, 1);
        return *this;
    }
    inline CArchive& operator<< (const qd::Tribool& b)
    {
        uint8_t v = b.getRaw();
        m_pAr->_arWrite_Buf(&v, 1);
        return *this;
    }
    // copy Value to stack and swap due to little/big endians
    inline CArchive& operator<< (uint16_t w)
    {
        m_pAr->_arWrite_UInt(&w, 2);
        return *this;
    }
    inline CArchive& operator<< (short s)
    {
        m_pAr->_arWrite_Int(&s, 2);
        return *this;
    }
    inline CArchive& operator<< (int i)
    {
        m_pAr->_arWrite_Int(&i, 4);
        return *this;
    }
    inline CArchive& operator<< (uint32_t i)
    {
        m_pAr->_arWrite_UInt(&i, 4);
        return *this;
    }
    inline CArchive& operator<< (float f)
    {
        m_pAr->_arWrite_Float(&f, 4);
        return *this;
    }
    inline CArchive& operator<< (double d)
    {
        m_pAr->_arWrite_Float(&d, 8);
        return *this;
    }
    inline CArchive& operator<< (int64_t ll)
    {
        m_pAr->_arWrite_Int(&ll, 8);
        static_assert(sizeof(int64_t) == 8);
        return *this;
    }
    inline CArchive& operator<< (uint64_t ll)
    {
        m_pAr->_arWrite_UInt(&ll, 8);
        static_assert(sizeof(uint64_t) == 8);
        return *this;
    }

    inline CArchive& operator<< (const IFileChunk& FileChunk);
    inline CArchive& operator<< (const char* pString)
    {
        qd::string String(pString);
        return operator<< (String);
    }
    CArchive& operator<< (const qd::string& String)
    {
        m_pAr->_arWrite_String(String.c_str(), (uint32_t)String.size());
        return *this;
    }
    CArchive& operator<< (/*const*/ qd::IBaseFileIO& File);


    inline CArchive& operator>> (qd::Tribool& b)
    {
        m_pAr->_arRead_Buf(&b, 1);
        return *this;
    }
    inline CArchive& operator>> (unsigned char& b)
    {
        m_pAr->_arRead_Buf(&b, 1);
        return *this;
    }
    inline CArchive& operator>> (signed char& b)
    {
        m_pAr->_arRead_Buf(&b, 1);
        return *this;
    }
    inline CArchive& operator>> (char& b)
    {
        m_pAr->_arRead_Buf(&b, 1);
        return *this;
    }
    inline CArchive& operator>> (bool& b)
    {
        uint8_t v;
        m_pAr->_arRead_Buf(&v, 1);
        b = (v != 0);
        return *this;
    }

    inline CArchive& operator>> (uint16_t& w)
    {
        m_pAr->_arRead_UInt(&w, 2);
        return *this;
    }
    inline CArchive& operator>> (short& s)
    {
        m_pAr->_arRead_Int(&s, 2);
        return *this;
    }
    inline CArchive& operator>> (int& i)
    {
        m_pAr->_arRead_Int(&i, 4);
        return *this;
    }
    inline CArchive& operator>> (uint32_t& i)
    {
        m_pAr->_arRead_UInt(&i, 4);
        return *this;
    }
    inline CArchive& operator>> (float& f)
    {
        m_pAr->_arRead_Float(&f, 4);
        return *this;
    }
    inline CArchive& operator>> (double& d)
    {
        m_pAr->_arRead_Float(&d, 8);
        return *this;
    }
    inline CArchive& operator>> (int64_t& ll)
    {
        m_pAr->_arRead_Int(&ll, 8);
        return *this;
    }
    inline CArchive& operator>> (uint64_t& ll)
    {
        m_pAr->_arRead_UInt(&ll, 8);
        return *this;
    }
    inline CArchive& operator>> (IFileChunk& FileChunk);
    CArchive& operator>> (qd::string& String)
    {
        m_pAr->_arRead_String(String);
        return *this;
    }

    CArchive& operator>> (qd::IBaseFileIO& File);

    template<typename CType>
    inline void TT_S(const CType& v)
    {
        typedef typename std::underlying_type<CType>::type TInnerType;
        TInnerType tv = TInnerType(v);
        this->operator<< (tv);
    }

    template<typename CType>
    inline void TT_L(CType& v)
    {
        typedef typename std::underlying_type<CType>::type TInnerType;
        TInnerType tv = TInnerType(v);
        this->operator<< (tv);
        v = (CType)tv;
    }


    // names not supported yet
#if 1
    // 		inline CArchiveNamedArgScope operator [] (const qd::string_view& attrName) {
    // 			return *this;
    // 		}
    template<std::size_t N>
    Arc::Private::CArchiveNamedArgScope operator[] (const char (&pAttrName)[N]);
#endif // 0


#define _QD_SERIAL_MTD(pAR, TName, TType)                                            \
    inline TType TName##_L()                                                         \
    {                                                                                \
        TType Val;                                                                   \
        TName##_L(Val);                                                              \
        return Val;                                                                  \
    }                                                                                \
    template<typename TVal>                                                          \
    inline TVal TName##_L(TVal& Val)                                                 \
    {                                                                                \
        TType iVal;                                                                  \
        pAR->operator>> (iVal);                                                      \
        Val = (TVal)iVal;                                                            \
        return Val;                                                                  \
    }                                                                                \
    template<typename TVal>                                                          \
    inline TVal TName##_S(const TVal& Val)                                           \
    {                                                                                \
        pAR->operator<< ((TType)Val);                                                \
        return Val;                                                                  \
    }                                                                                \
    template<typename TVal>                                                          \
    inline TVal TName##Self(TVal& Val, qd::Tribool bStoring = qd::Tribool::Unknown)        \
    {                                                                                \
        bool bbStoring = (bStoring.isUnknown()) ? isStoring() : (bStoring.isTrue()); \
        if (bbStoring)                                                               \
        {                                                                            \
            return TName##_S(Val);                                                   \
        }                                                                            \
        else                                                                         \
        {                                                                            \
            return TName##_L(Val);                                                   \
        }                                                                            \
    }

    // I32_L / I32_S - expand
    _QD_SERIAL_MTD(this, Int, int);
    _QD_SERIAL_MTD(this, I32, int);
    _QD_SERIAL_MTD(this, Byte, uint8_t);
    _QD_SERIAL_MTD(this, U8, uint8_t);
    _QD_SERIAL_MTD(this, U16, uint16_t);
    _QD_SERIAL_MTD(this, U32, uint32_t);

    template<typename TInt>
    inline bool bool_L(TInt& Val)
    {
        bool tmpVal;
        operator>> (tmpVal);
        Val = (TInt)tmpVal;
        return Val;
    }
    inline bool bool_L()
    {
        bool Val;
        bool_L(Val);
        return Val;
    }

    template<typename TInt>
    inline bool bool_S(const TInt& Val)
    {
        bool b = !isPtrNull(Val);
        operator<< (b);
        return b;
    }

    template<typename TInt>
    inline bool boolSelf(TInt& Val, qd::Tribool bStoring = qd::Tribool::Unknown)
    {
        bool bbStoring = (bStoring.isUnknown()) ? isStoring() : bStoring.getBool();
        if (bbStoring)
        {
            return bool_S(Val);
        }
        else
        {
            return bool_L(Val);
        }
    }

    inline void write(const void* pBuffer, uint32_t nBytes) { m_pAr->_arWrite_Buf(pBuffer, nBytes); }
    inline void read(void* pBuffer, uint32_t nBytes) { m_pAr->_arRead_Buf(pBuffer, nBytes); }

    void writeFrom(qd::IBaseFileIO& SrcFile, uint32_t nBytes = UINT_MAX);

    // void SkipBytes(uint nBytes);

    void flush() { m_pAr->_arFlush(); }

    // raw file pos bytes
    uint32_t getFilePos() { return m_pAr->_arGetFilePos(); }
    void setFilePos(uint32_t Pos) { m_pAr->_arSetFilePos(Pos); }

    void skipBytes(uint32_t nBytes) { m_pAr->_arSkipBytes(nBytes); }

    qd::string_view getFileName() const { return m_pAr->_arGetFileName(); }

    void skip(const qd::IFileChunk& Chunk) { m_pAr->_arSkip(Chunk); }
    void undo(const qd::IFileChunk& Chunk) { m_pAr->_arUndo(Chunk); }
    bool check(const qd::IFileChunk& Chunk) { return m_pAr->_arLoadChunkEnd(&Chunk); }
    void checkSafe(const qd::IFileChunk& Chunk);

    void endAllChunks()
    {
        while (this->GetNumOpenChunks() > 0)
            this->endChunk();
    }

    qd::IArchiveFormat* getArFormat() const { return m_pAr; }

    void reset()
    {
        m_pAr->_arReset();
        m_pAr->setFile(nullptr);
    }

protected:
    CArchive(const CArchive& Other) = delete;

private:
    CArchive& operator= (const CArchive& a) = delete; // { return *this; }

}; // class CArchive
//////////////////////////////////////////////////////////////////////////



qd::CArchive& CArchive::operator<< (const qd::IFileChunk& FileChunk)
{
    m_pAr->_arSaveChunkBegin(FileChunk);
    return *this;
}


qd::CArchive& CArchive::operator>> (qd::IFileChunk& FileChunk)
{
    if (!loadChunk(FileChunk))
        throw Exception(EException::IO_ERROR, "Error: Can't load Chunk!");
    return *this;
}






//////////////////////////////////////////////////////////////////////////
class IFileChunkScope : public qd::IFileChunk
{
    typedef IFileChunk TSuper;

protected:
    qd::CArchive* m_pArchive;

protected:
    IFileChunkScope(qd::CArchive* pAR, qd::Arc::EChunkType _ChunkSizeOf)
        : TSuper(_ChunkSizeOf)
        , m_pArchive(pAR)
    {}

public:
    bool check() { return m_pArchive->check(*this); }

    void checkSafe() { return m_pArchive->checkSafe(*this); }

    void skip() { m_pArchive->skip(*this); }

    // REWIND TO BEFORE CHUNK IS READED
    void undo() { m_pArchive->undo(*this); }

    // REWIND TO START OF THE CHUNK
    void rewind() { TSuper::rewind(*m_pArchive); }

    inline void reset()
    {
        if (m_pArchive && m_pArchive->isLoading())
            m_pArchive = nullptr;
    }

    void endChunk();

    ~IFileChunkScope() { endChunk(); }

}; // class IFileChunkScope
   //////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////
class CSerialMark2
{
    uint8_t m_Version;

public:
    CSerialMark2(qd::CArchive& ar, uint8_t ID, uint8_t nVer = 0);

    inline uint8_t GetVer() const { return m_Version; }

    inline operator uint8_t () const { return GetVer(); }

}; // class CSerialMark2
//////////////////////////////////////////////////////////////////////////



// SAVE ONE BYTE MARKER
class CSerialMark1
{
    uint8_t m_Byte;
    uint8_t m_FromID;

public:
    CSerialMark1(qd::CArchive& ar, uint8_t ID);

    // INTERVAL
    CSerialMark1(qd::CArchive& ar, uint8_t FromID, uint8_t ToID);

    inline operator uint8_t () const { return m_Byte; }

    inline uint8_t GetVer() const
    { // VERSION
        return (uint8_t)(m_Byte - m_FromID);
    }

}; // class CSerialMark1
//////////////////////////////////////////////////////////////////////////






//////////////////////////////////////////////////////////////////////////
#if 1
class CFileChunk12 : public IFileChunk
{
    typedef IFileChunk TSuper;
    friend class CArchive;

public:
    static constexpr uint8_t SIZE = Arc::EChunkType::ID32_V32_P32;

    static constexpr uint32_t size_of() { return Arc::EChunkType::ID32_V32_P32; }

public:
    CFileChunk12()
        : TSuper(Arc::EChunkType::ID32_V32_P32)
    {}

    explicit CFileChunk12(uint32_t ID, uint32_t nVersion)
        : TSuper(ID, nVersion, Arc::EChunkType::ID32_V32_P32)
    {}

    // Loads chunk without testing
    inline explicit CFileChunk12(qd::CArchive& ar)
        : TSuper(ar, Arc::EChunkType::ID32_V32_P32)
    {}

    // Loads chunk and test his ID, if ID not match throw IO_EXCEPTION
    explicit CFileChunk12(qd::CArchive& ar, uint32_t ID)
        : TSuper(ar, ID, Arc::EChunkType::ID32_V32_P32)
    {}

    ~CFileChunk12() {}


}; // class CFileChunk
#endif // 0
//////////////////////////////////////////////////////////////////////////



class CFileChunkEx12 : public IFileChunkScope
{
    typedef IFileChunkScope TSuper;

public:
    CFileChunkEx12(qd::CArchive& ar, uint32_t ID, uint32_t nVersion = 0)
        : TSuper(&ar, Arc::EChunkType::ID32_V32_P32)
    {
        TSuper::_serializeChunkID32(ar, ID, nVersion);
    }

    inline ~CFileChunkEx12() { c_def(0); }

}; // class CFileChunkEx
//////////////////////////////////////////////////////////////////////////



// 6 - bytes CHUNK
// ChunkID = 8 Bit; nVer = 8 bit; ReadPos = 32bit ( MAX = 2 Gb )
class CFileChunk6 : public IFileChunk
{
    typedef IFileChunk TSuper;

public:
    CFileChunk6(uint8_t ID = 0, uint8_t nVersion = 0)
        : TSuper(ID, nVersion, Arc::EChunkType::U8_U8_U32)
    {}

    CFileChunk6(qd::CArchive& ar, uint8_t ID)
        : TSuper(ar, ID, Arc::EChunkType::U8_U8_U32)
    {}

    static constexpr uint8_t SIZE = Arc::EChunkType::U8_U8_U32;
    static constexpr uint32_t size_of() { return Arc::EChunkType::U8_U8_U32; }

}; // class CFileChunk6
   //////////////////////////////////////////////////////////////////////////



// 6 - bytes CHUNK
// ChunkID = 8 Bit; nVer = 8 bit; ReadPos = 32bit ( MAX = 2 Gb )
class CFileChunkEx6 : public IFileChunkScope
{
    typedef IFileChunkScope TSuper;

    // PRIVATE COPY CONSTRUCTOR
    // 		CFileChunkEx16(const CFileChunkEx16&) {}
    // 		CFileChunkEx16& operator = ( const CFileChunkEx16& ) { return *this; }

public:
    CFileChunkEx6(qd::CArchive& ar, uint8_t ID, uint8_t nVersion = 0)
        : TSuper(&ar, Arc::EChunkType::U8_U8_U32)
    {
        _serializeChunkID32(ar, ID, nVersion);
    }

    inline ~CFileChunkEx6() { c_def(0); }

}; // class CFileChunkEx6
   //////////////////////////////////////////////////////////////////////////




// 5 - bytes CHUNK
// ChunkID = 8 Bit; nVer = 8 bit; ReadPos = 24bit;
class CFileChunk5 : public IFileChunk
{
    typedef IFileChunk TSuper;

public:
    CFileChunk5(uint8_t ID = 0, uint8_t nVersion = 0)
        : TSuper(ID, nVersion, Arc::EChunkType::U8_U8_U24)
    {}

    CFileChunk5(qd::CArchive& ar, uint8_t ID)
        : TSuper(ar, ID, Arc::EChunkType::U8_U8_U24)
    {}

    static const uint8_t SIZE = Arc::EChunkType::U8_U8_U24;
    static uint32_t size_of() { return Arc::EChunkType::U8_U8_U24; }

}; // class CFileChunk5
//////////////////////////////////////////////////////////////////////////



// 5 - bytes CHUNK
// ChunkID = 8 Bit; nVer = 8 bit; ReadPos = 24bit ( MAX = 16 Mb )
class CFileChunkEx5 : public IFileChunkScope
{
    typedef IFileChunkScope TSuper;

    // PRIVATE COPY CONSTRUCTOR
    // 		CFileChunkEx16(const CFileChunkEx16&) {}
    // 		CFileChunkEx16& operator = ( const CFileChunkEx16& ) { return *this; }

public:
    CFileChunkEx5(qd::CArchive& ar, uint8_t ID, uint8_t nVersion = 0)
        : TSuper(&ar, Arc::EChunkType::U8_U8_U24)
    {
        _serializeChunkID32(ar, ID, nVersion);
    }

    inline ~CFileChunkEx5() { c_def(0); }

}; // class CFileChunkEx5
//////////////////////////////////////////////////////////////////////////




// 4 - bytes CHUNK
// ChunkID = 8 Bit; nVer = 8 bit; BodySize = 16bit( 65535 bytes max)
class CFileChunk4 : public IFileChunk
{
    typedef IFileChunk TSuper;

public:
    CFileChunk4(uint8_t ID = 0, uint8_t nVersion = 0)
        : TSuper(ID, nVersion, Arc::EChunkType::U8_U8_U16)
    {}

    CFileChunk4(qd::CArchive& ar, uint8_t ID)
        : TSuper(ar, ID, Arc::EChunkType::U8_U8_U16)
    {}

    static const uint8_t SIZE = Arc::EChunkType::U8_U8_U16;
    static uint32_t size_of() { return Arc::EChunkType::U8_U8_U16; }

}; // class CFileChunk4
//////////////////////////////////////////////////////////////////////////



// 4 - bytes CHUNK
// ChunkID = 8 Bit; nVer = 8 bit; BodySize = 16bit( 65535 bytes max)
class CFileChunkEx4 : public IFileChunkScope
{
    typedef IFileChunkScope TSuper;

public:
    CFileChunkEx4(qd::CArchive& ar, uint8_t ID, uint8_t nVersion = 0)
        : TSuper(&ar, Arc::EChunkType::U8_U8_U16)
    {
        _serializeChunkID32(ar, ID, nVersion);
    }

    inline ~CFileChunkEx4() {}
}; // class CFileChunkEx4
//////////////////////////////////////////////////////////////////////////



// 3 - bytes CHUNK
// ChunkID = 8 Bit; nVer = 0 bit; BodySize = 16bit( 65535 bytes max)
class CFileChunk3 : public IFileChunk
{
    typedef IFileChunk TSuper;

public:
    CFileChunk3()
        : TSuper(Arc::EChunkType::U8_U0_U16)
    {}

    CFileChunk3(qd::CArchive& ar, uint8_t ID)
        : TSuper(ar, ID, Arc::EChunkType::U8_U0_U16)
    {}

    static const uint8_t SIZE = Arc::EChunkType::U8_U0_U16;
    static uint32_t size_of() { return Arc::EChunkType::U8_U0_U16; }

}; // class CFileChunk3
//////////////////////////////////////////////////////////////////////////





// 2 - bytes CHUNK
// ChunkID = 0 Bit; nVer = 0 bit; ReadPos = 16bit; ( MAX = 64 Kb )
class CFileChunk2 : public qd::IFileChunk
{
    typedef qd::IFileChunk TSuper;

public:
    CFileChunk2()
        : TSuper(Arc::EChunkType::U0_U0_U16)
    {}

    CFileChunk2(qd::CArchive& ar)
        : TSuper(ar, 0, Arc::EChunkType::U0_U0_U16)
    {}

    static const uint8_t SIZE = Arc::EChunkType::U0_U0_U16;
    static uint32_t size_of() { return Arc::EChunkType::U0_U0_U16; }

}; // class CFileChunk2
//////////////////////////////////////////////////////////////////////////



// 2 - bytes CHUNK
// ChunkID = 0(ZERO) Bit; nVer = 0(ZERO) bit; ReadPos = 16bit;
class CFileChunkEx2 : public IFileChunkScope
{
    typedef IFileChunkScope TSuper;

    // PRIVATE COPY CONSTRUCTOR
    // 		CFileChunkEx16(const CFileChunkEx16&) {}
    // 		CFileChunkEx16& operator = ( const CFileChunkEx16& ) { return *this; }

public:
    CFileChunkEx2(qd::CArchive& ar) // NO VERION or ID - HERE
        : TSuper(&ar, Arc::EChunkType::U0_U0_U16)
    {
        _serializeChunkID32(ar, 0, 0);
    }

    inline ~CFileChunkEx2() { c_def(0); }

}; // class CFileChunkEx2
//////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////
namespace Arc {

// 4 - bytes CHUNK
// ChunkID = 0 Bit; nVer = 8 bit; BodySize = 24bit( 16Mb max)
class ChunkScope_U0_V8_P24 : public qd::IFileChunkScope
{
public:
    ChunkScope_U0_V8_P24(qd::CArchive& ar, uint8_t nVersion)
        : IFileChunkScope(&ar, Arc::EChunkType::U0_U8_U24)
    {
        _serializeChunkID32(ar, 0, nVersion);
    }

    inline ~ChunkScope_U0_V8_P24() {}
}; // class ChunkScope_U0_V8_P24
   //////////////////////////////////////////////////////////////////////////

// 4 - bytes CHUNK
// ChunkID = 0 Bit; nVer = 8 bit; BodySize = 16bit( 64Kb max)
class ChunkScope_U0_V8_P16 : public qd::IFileChunkScope
{
public:
    ChunkScope_U0_V8_P16(qd::CArchive& ar, uint8_t nVersion)
        : IFileChunkScope(&ar, Arc::EChunkType::U0_U8_U16)
    {
        _serializeChunkID32(ar, 0, nVersion);
    }

    inline ~ChunkScope_U0_V8_P16() {}
}; // class ChunkScope_U0_V8_P16
   //////////////////////////////////////////////////////////////////////////


// 3 - bytes CHUNK
// ChunkID = 8 Bit; nVer = 0 bit; BodySize = 16bit( 65535 bytes max)
class ChunkScope_U8_V0_P16 : public IFileChunkScope
{
    typedef IFileChunkScope TSuper;

public:
    ChunkScope_U8_V0_P16(qd::CArchive& ar, uint8_t ID)
        : TSuper(&ar, Arc::EChunkType::U8_U0_U16)
    {
        _serializeChunkID32(ar, ID, 0);
    }

    // INTERVAL-ID CONSTRUCTOR
    ChunkScope_U8_V0_P16(qd::CArchive& ar, uint8_t FromID, uint8_t ToID)
        : TSuper(&ar, Arc::EChunkType::U8_U0_U16)
    {
        uint8_t nVer = (uint8_t)(ToID - FromID); // OVERFLOW ALLOWED
        assert(nVer <= 0x20 && "WARNING: POSSIBLE - BAD ToID Chunk3 parameter");
        _serializeChunkID32(ar, FromID, nVer, /*checkID:*/ false); // set Chunk ID as FROM_ID
        m_Version = (uint8_t)m_ID; // (byte)(m_ID - FromID); // USE ID as Ver instead _V0_
    }


}; // class ChunkScope_U8_V0_P16
//////////////////////////////////////////////////////////////////////////


////////////
// CHUNK SCOPE - STRING WITH VERSION
// ID:char[16] Ver:8 BodySize:32(unlim)
class ChunkScope_S_V8 : public qd::IFileChunkScope
{
    typedef qd::IFileChunkScope TSuper;

public:
    ChunkScope_S_V8(qd::CArchive& ar, string_view pStrID, uint8_t nVersion = 0)
        : TSuper(&ar, qd::Arc::EChunkType::STR_U8_U32)
    {
        _serializeChunkStr(ar, pStrID, nVersion);
    }

    inline ~ChunkScope_S_V8() {}

}; // class ChunkScope_
   //////////////////////////////////////////////////////////////////////////



namespace Private {

struct CArchiveNamedArgScope {
    typedef CArchiveNamedArgScope TThis;
    CArchive* m_pAR;
    qd::IFileChunk m_Chunk;

public:
    CArchiveNamedArgScope(CArchive* pAR, string_view attrName)
        : m_pAR(pAR)
        , m_Chunk(qd::Arc::EChunkType::STR_U0_U8)
    {
        m_Chunk._serializeChunkStr(*m_pAR, attrName, 0);
    }

    template<class T>
    inline TThis& operator<< (const T& arg)
    {
        *m_pAR << arg;
        return *this;
    }

    template<class T>
    inline TThis& operator>> (T& arg)
    {
        *m_pAR >> arg;
        return *this;
    }

    // Read / Store
    template<typename CType>
    inline TThis& serializeSelf(CType& v)
    {
        m_pAR->serializeSelf(v);
        return *this;
    }
    template<typename TType>
    inline TThis& operator() (TType& v)
    {
        m_pAR->serializeSelf(v);
        return *this;
    }

    inline bool isStoring() const { return m_pAR->isStoring(); }

    _QD_SERIAL_MTD(m_pAR, Int, int);
    _QD_SERIAL_MTD(m_pAR, I32, int);
    _QD_SERIAL_MTD(m_pAR, Byte, uint8_t);
    _QD_SERIAL_MTD(m_pAR, U8, uint8_t);
    _QD_SERIAL_MTD(m_pAR, U16, uint16_t);
    _QD_SERIAL_MTD(m_pAR, U32, uint32_t);

    ~CArchiveNamedArgScope() { m_pAR->endChunk(m_Chunk); }
}; // struct CArchiveNamedArgScope
//////////////////////////////////////////////////////////////////////////
}; // namespace Private



// 4 - bytes CHUNK
// ChunkID = 8 Bit; nVer = 8 bit; BodySize = 16bit( 65535 bytes max)
using ChunkScope_U8_V8_P16 = qd::CFileChunkEx4;
using ChunkScope_U8_V8_P24 = qd::CFileChunkEx5;
using ChunkScope_U8_V8_P32 = qd::CFileChunkEx6;


}; // namespace Arc

template<std::size_t N>
Arc::Private::CArchiveNamedArgScope inline CArchive::operator[] (const char (&pAttrName)[N])
{
    return Arc::Private::CArchiveNamedArgScope(this, string_view(pAttrName, (uint32_t)(N - 1u)));
}

}; // namespace qd

#undef _QD_SERIAL_MTD
