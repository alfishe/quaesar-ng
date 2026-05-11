#include "qd/base/base.h"
#if !QD_USE_SDL

#include "qd/debug/exception.h"
#include "qd/debug/assert.h"
#include "qd/file/fileBase.h"
#include "qd/stl/string.h"
#include <cstdio>


namespace qd {


//////////////////////////////////////////////////////////////////////////
uint32_t ImpStdFilePosix::getSize() {

    if (m_nFileSize != ~0u)
        return m_nFileSize;

    FILE* f = (FILE*)m_pFileHnd;
    if (!f) {
        ASSERT_F(0, "StdIOFile GetSize File is Closed: %s", CC(getFileName()));
        return 0;
    }

    uint32_t size = ~0u;
    long prevPosition = ftell(f);
    fseek(f, 0L, SEEK_END);
    size = (uint32_t)ftell(f);
    fseek(f, prevPosition, SEEK_SET);
    m_curPos = (uint32_t)prevPosition;
    m_nFileSize = size;
    return size;
}


uint32_t ImpStdFilePosix::read(void* pDest, uint32_t nBytes) {

    assert(m_pFileHnd);
    if (nBytes == 0)
        return 0;

    FILE* f = (FILE*)m_pFileHnd;
    uint32_t nReaded = (uint32_t)fread(pDest, 1, nBytes, f);
    if (nReaded == 0 && ferror(f)) {
        throw Exception(EException::IO_ERROR, "ImpStdFilePosix::fread - ERROR! NeedRead:%u Readed:%u", nBytes, nReaded);
    }
    m_curPos += nReaded;
    return nReaded;
}


uint32_t ImpStdFilePosix::write(const void* pSrc, uint32_t nBytes) {

    if (nBytes == 0)
        return 0;

    FILE* f = (FILE*)m_pFileHnd;
    uint32_t nWrited = (uint32_t)fwrite(pSrc, 1, nBytes, f);
    if (nWrited != nBytes) {
        throw Exception(EException::IO_ERROR, "ImpStdFilePosix::Write - ERROR! NeedWrite:%u Writed:%u", nBytes, nWrited);
    }
    m_curPos += nWrited;
    m_nFileSize += nWrited;
    return nWrited;
}



bool ImpStdFilePosix::open(const char* path, const char* pMode, qd::EIOErrorCode* pErr /*= nullptr*/) {

    if (m_pFileHnd)
        close();

    if (!path || !pMode)
        return false;

    FILE* f = nullptr;
#ifdef _MSC_VER
    errno_t err = fopen_s(&f, path, pMode);
    if (err != 0)
        f = nullptr;
#else
    f = fopen(path, pMode);
#endif

    m_pFileHnd = (HndFile)f;
    if (!m_pFileHnd)
        return false;

    if (m_pFileHnd) {
        if (pMode[0] == 'w' || pMode[0] == 'W') {
            m_curPos = 0;
            m_nFileSize = 0;
        }
        else if (pMode[0] == 'r' || pMode[0] == 'R') {
            m_curPos = 0;
            m_nFileSize = ~0u;
        }
        else if (pMode[0] == 'a' || pMode[0] == 'A') {
            m_nFileSize = getSize();
            m_curPos = m_nFileSize;
            if (m_nFileSize == (uint32_t)~0u) {
                close();
                if (pErr)
                    *pErr = EIOErrorCode::DiskFull; // FILE overflow more then 2Gb
                return false;
            }
        }

        if (m_curPos == ~0u) {
            close();
            throw qd::Exception(EException::OPERATION_ERR, "Bad std File Open Mode %s", pMode);
        }
    }

    _setOpened(m_pFileHnd != 0);
    return isOpened();
}


void ImpStdFilePosix::openSafe(const char* path, const char* mode) {
    if (!open(path, mode)) {
        ASSERT_F(0, "Can't open file: '%s'", path);
        return;
    }
}


void ImpStdFilePosix::close() {
    qd::HndFile pFile = m_pFileHnd;
    m_pFileHnd = {};
    m_curPos = ~0u;
    _setOpened(false);

    if (pFile) {
        fclose((FILE*)pFile);
    }
}


uint32_t ImpStdFilePosix::seek(uint32_t pos, qd::EFileSeek where /*= SEEK_SET*/) {
    ASSERT_AND_DO(m_pFileHnd, return ~0u, "Bad StdIo file handler!");

    FILE* f = (FILE*)m_pFileHnd;

    switch (where) {
    case qd::EFileSeek::SET:
        m_curPos = pos;
        break;
    case qd::EFileSeek::CUR:
        m_curPos += pos;
        break;
    case qd::EFileSeek::END:
    {
        if (m_nFileSize != ~0u)
            m_curPos = m_nFileSize;
        else {
            fseek(f, (long)pos, SEEK_END);
            m_nFileSize = (uint32_t)ftell(f);
            m_curPos = m_nFileSize;
            return m_curPos;
        }
    } break;
    default:
        assert(0 && "Bad Seek Parameter");
        break;
    }

    int nErr = fseek(f, (long)pos, where);
    if (nErr != 0)
        throw Exception(EException::IO_ERROR, "Bad StdFile Seek Command");

    return m_curPos;
}



uint32_t ImpStdFilePosix::tell() {
    ASSERT_AND_DO(m_pFileHnd, return ~0u, "Bad StdIo file handler!");

    if (m_curPos != ~0u)
        return m_curPos;

    m_curPos = (uint32_t)ftell((FILE*)m_pFileHnd);
    if (m_curPos == (uint32_t)-1L) {
        throw Exception(EException::IO_ERROR, "ImpStdFilePosix::Tell() - ERROR! errno=%d", errno);
    }
    return m_curPos;
}



}; // namespace qd

#endif // !QD_USE_SDL
