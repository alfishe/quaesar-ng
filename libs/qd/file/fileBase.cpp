#include <qd/file/fileBase.h>
#include <qd/debug/assert.h>
#include <qd/stl/string.h>
#include "SDL_rwops.h"
#include "qd/debug/exception.h"


namespace qd {


//////////////////////////////////////////////////////////////////////////
uint32_t StdFile::getSize()
{
	if (m_nFileSize != ~0u)
		return m_nFileSize;

	if (!m_pFileHnd)
	{
		ASSERT_F(0, "StdIOFile GetSize File is Closed: %s", CC(getFileName()));
		return 0;
	}

	uint32_t Size = ~0u;

	uint32_t PrevPosition = (uint32_t)ftell((FILE*)m_pFileHnd);
	fseek((FILE*)m_pFileHnd, 0L, SEEK_END);
	Size = (uint32_t)ftell((FILE*)m_pFileHnd);
	fseek((FILE*)m_pFileHnd, PrevPosition, SEEK_SET);
	m_curPos = PrevPosition;

	m_nFileSize = Size;
	return Size;
}


uint32_t StdFile::read(void* pDest, uint32_t nBytes)
{
	assert(m_pFileHnd);
	if (nBytes == 0)
		return 0;

    uint32_t nReaded = (uint32_t)SDL_RWread((SDL_RWops*)m_pFileHnd, pDest, 1, nBytes);
    if (nReaded == 0)
    {
        const char* pErr = SDL_GetError();
        if (pErr)
            throw Exception(EException::IO_ERROR, "StdFile::SDL_RWread - ERROR! NeedRead:%u Readed:%u err:'%s'",
                nBytes, nReaded, pErr);
    }
	m_curPos += nReaded;
	return nReaded;
}


uint32_t StdFile::write(const void* pSrc, uint32_t nBytes)
{
	if (nBytes == 0)
		return 0;

	uint32_t nWrited;
    nWrited = (uint32_t)SDL_RWwrite((SDL_RWops*)m_pFileHnd, pSrc, 1, nBytes);
    if (nWrited != nBytes)
    {
        const char* pErr = SDL_GetError();
        throw Exception(EException::IO_ERROR, "StdFile::Write - ERROR! NeedWrite:%u Writed:%u err:%s", nBytes, nWrited,
            pErr);
    }
	m_curPos += nWrited;
	m_nFileSize += nWrited;
	return nWrited;
}



bool StdFile::open(const char* path, const char* pMode, qd::EIOErrorCode* pErr /*= nullptr*/)
{
	if (m_pFileHnd)
		close();

	if (!path || !pMode)
		return false;

	//setFileName(InFileName);

    qd::string realFilePath(path);

    m_pFileHnd = (HndFile)SDL_RWFromFile(CC(realFilePath), pMode);
    if (!m_pFileHnd)
        return false;

    if (m_pFileHnd)
    {
        if (pMode[0] == 'w' || pMode[0] == 'W')
        {
            m_curPos = 0;
            m_nFileSize = 0;
        }
        else if (pMode[0] == 'r' || pMode[0] == 'R')
        {
            m_curPos = 0;
            m_nFileSize = ~0u;
        }
        else if (pMode[0] == 'a' || pMode[0] == 'A')
        {
            m_nFileSize = getSize();
            m_curPos = m_nFileSize;
            if (m_nFileSize == (uint32_t)~0u)
            {
                close();
                if (pErr)
                    *pErr = EIOErrorCode::DiskFull; // FILE overflow more then 2Gb
                return false;
            }
        }

        if (m_curPos == ~0u)
        {
            close();
            throw qd::Exception(EException::OPERATION_ERR, "Bad std File Open Mode %s", pMode);
        }
    }

    _setOpened(m_pFileHnd != 0);

	return isOpened();
}



void StdFile::openSafe(const char* path, const char* mode)
{
	if (!open(path, mode))
	{
		assert2(0, "Can't open file: '%s'", CC(path));
		return;
	}
}


void StdFile::close()
{
	// a little thread safe
	qd::HndFile pFile = m_pFileHnd;
	m_pFileHnd = {};
	m_curPos = ~0u;
	_setOpened(false);

	if (pFile)
	{
        SDL_RWclose((SDL_RWops*)pFile);
	}
}



uint32_t StdFile::seek(uint32_t pos, qd::EFileSeek where /*= SEEK_SET*/)
{
    ASSERT_AND_DO(m_pFileHnd, return ~0u, "Bad StdIo file handler!");

    switch (where)
    {
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
        else
        {
            // IF FILE SIZE IS UNKNOWN
            SDL_RWseek((SDL_RWops*)m_pFileHnd, pos, EFileSeek::END);
            m_nFileSize = (uint32_t)SDL_RWtell((SDL_RWops*)m_pFileHnd);
            m_curPos = m_nFileSize;
            return m_curPos;
        }
    }
    break;
    default:
        assert(0 && "Bad Seek Parameter");
        break;
    }

    int nErr = (int)SDL_RWseek((SDL_RWops*)m_pFileHnd, pos, where);
    if (nErr < 0)
        throw Exception(EException::IO_ERROR, "Bad StdFile Seek Command");

    return m_curPos;
}



uint32_t StdFile::tell()
{
    ASSERT_AND_DO(m_pFileHnd, return ~0u, "Bad StdIo file handler!");

	if (m_curPos != ~0u)
		return m_curPos;

    m_curPos = (uint32_t)SDL_RWtell((SDL_RWops*)m_pFileHnd);
    if (m_curPos == (uint32_t)-1L)
    {
        const char* nErr = SDL_GetError();
        throw Exception(EException::IO_ERROR, "StdioFile::Tell() - ERROR! errno='%s'", nErr);
    }
	return m_curPos;
}



}; // namespace qd
