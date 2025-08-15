#pragma once
#include "qd/base/base.h"
#include "qd/debug/assert.h"
#include "qd/stl/string.h"
#include "qd/enum/enumBase.h"
#include "qd/stl/ref_ptr.h"


namespace qd {


struct ESaveLoad {
    enum eType : uint8_t {
        Load = 0x00, Save = 0x01, MaskClipboard = 0x02, PasteClipboard = MaskClipboard | Load,
        CopyClipboard = MaskClipboard | Save,
        // SaveLoad mode may be expanded (derived) for different targets
        // saves for whom (for client or for serverSave)
    };

    inline bool isLoad() const { return (mV & ESaveLoad::Save) == ESaveLoad::Load; }
    inline bool IsSave() const { return (mV & ESaveLoad::Save) == ESaveLoad::Save; }
    inline bool IsCopy() const { return (mV & ESaveLoad::CopyClipboard) == ESaveLoad::CopyClipboard; }
    inline bool IsPaste() const { return (mV & ESaveLoad::PasteClipboard) == ESaveLoad::PasteClipboard; }
    inline operator bool () const { return (mV & ESaveLoad::Save) == ESaveLoad::Save; } 

    ENUM_DECLARE_BASE(qd::, ESaveLoad, eType, 0);
}; // enum ESaveLoad
//////////////////////////////////////////////////////////////////////////



struct EIOErrorCode {
	enum eType {
		Success = 0,
		OperNotPerm = 1, // Operation not permitted
		NotExist = 2, // No such file or directory
		NoProcess = 3, // No such process
		InteruptedFunc = 4, // Interrupted function
		ErrorIO = 5, // I/O error
		NoSuchDevice = 6, // No such device or address
		E_E2BIG = 7, // Argument list too long
		E_ENOEXEC = 8, // Exec format error
		BadFileNumber = 9, // Bad file number
		E_ECHILD = 10, // No spawned processes
		E_EAGAIN = 11, // No more processes or not enough memory or maximum nesting level reached
		NoMemory = 12, // Not enough memory
		NoAccess = 13, // Permission bits of the file mode do not permit the requested access, or search permission is
		               // denied on a component of the path prefix.
		E_EBUSY = 16, // Device or resource busy
		AlreadyExists = 17, // File exists
		E_EXDEV = 18, // Cross-device link
		E_ENODEV = 19, // No such device
		NotDirectory = 20, // Not a directory
		NotFile = 21, // Is a directory
		BadArgument = 22, // The value of the amode argument is invalid.
		E_ENFILE = 23, // Too many files open in system
		E_EMFILE = 24, // Too many open files
		E_ENOTTY = 25, // Inappropriate I/O control operation
		E_EFBIG = 27, // File too large
		E_ENOSPC = 28, // No space left on device
		E_ESPIPE = 29, // Invalid seek
		ReadOnly = 30, // Write access is requested for a file on a read-only file system.
		E_EMLINK = 31, // Too many links
		E_EPIPE = 32, // Broken pipe
		E_EDOM = 33, // Math argument
		E_ERANGE = 34, // Result too large
		E_EDEADLOCK = 36, // Same as EDEADLK for compatibility with older Microsoft C versions
		NameTooLong =
			38, // The length of the path argument exceeds {PATH_MAX} or a pathname component is longer than {NAME_MAX}.
		E_ENOLCK = 39, // No locks available
		E_ENOSYS = 40, // Function not supported
		E_ENOTEMPTY = 41, // Directory not empty
		E_EILSEQ = 42, // Illegal byte sequence
		E_STRUNCATE = 80, // String was truncated
		DiskFull,
		EofReached,
		BadPath,
		ReadFault,
		WriteFault,
		DirNotEmpty,
		NegativeSeek,
		TooManyOpenFiles,
		NotImplemented,
		InvalidData,
	};
	ENUM_DECLARE_BASE(qd::, EIOErrorCode, eType, 0);

}; // enum EPathErrorCode
//////////////////////////////////////////////////////////////////////////


struct EOpenMode {
	enum eType {
		None = 0x00,
		Read = 0x01,
		Write = 0x02,
		ReadWrite = 0x03,
	};
	ENUM_DECLARE_BASE(qd::, EOpenMode, eType, 0);
	ENUM_DECLARE_FLAGS;

}; // enum EOpenMode
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
struct EFileSeek {
	enum eType {
		SET = 0, /* Seek from beginning of file.  */
		CUR = 1, /* Seek from current position.  */
		END = 2 /* Set file pointer to EOF plus "offset" */
	};
	ENUM_DECLARE_BASE(qd::, EFileSeek, eType, 0);
}; // enum EFileSeek
//////////////////////////////////////////////////////////////////////////



	//////////////////////////////////////////////////////////////////////////
	class IBaseFileIO : public qd::RefCounted
	{
	public:
		virtual uint32_t read(void* pDest, uint32_t nBytes) { return 0; };
		virtual uint32_t write(const void* pSrc, uint32_t nBytes) { return 0; };
		virtual uint32_t seek(uint32_t Position, EFileSeek Where = EFileSeek::SET) { return 0; };
		virtual uint32_t tell() { return 0; };
		virtual void close(){};
	
		virtual uint32_t getSize()
		{
			uint32_t prevPosition = tell();
			seek(0, EFileSeek::END);
			uint32_t size = tell();
			seek(prevPosition, EFileSeek::SET);
			return size;
		};

        virtual qd::string_view getFileName() const { return ""; }
        virtual int getNumChunks() const { return 0; }
        virtual int addNumChunks(int add) { return 0; };

		inline uint32_t skip(uint32_t nBytes) { return seek(nBytes, EFileSeek::CUR); }
	}; // class IBaseFileIO
	//////////////////////////////////////////////////////////////////////////
	
	
		//////////////////////////////////////////////////////////////////////////
	// COMMON - FILE_IO INTERAFACE for all files
	class IFile : public IBaseFileIO
	{
	protected:
		int m_nChunks = 0;
		bool m_bAOpened = false;
	
	public:
		IFile() = default;
		virtual ~IFile() { assert(m_nChunks == 0); }
	
        virtual int getNumChunks() const override { return m_nChunks; }
		virtual int addNumChunks(int Chunks) override
		{
			m_nChunks += Chunks;
			assert(m_nChunks >= 0);
			return m_nChunks;
		}
		bool isOpened() const { return m_bAOpened; }
	
	protected:
		void _setOpened(bool Opened) { m_bAOpened = Opened; }
	}; // class IFile
	//////////////////////////////////////////////////////////////////////////
	
	
    typedef size_t HndFile;
	
	class StdFile : public qd::IFile
	{
		typedef StdFile TThis;
		typedef qd::IFile TSuper;
		qd::HndFile m_pFileHnd = {};
		uint32_t m_curPos = ~0u;
		uint32_t m_nFileSize = ~0u;
		qd::string m_Name;

	
	public:
		StdFile(const char* fileName, const char* pMode)
		{
			openSafe(fileName, pMode);
		}
	
		StdFile() = default;
	
		StdFile(qd::HndFile pFile)
			: m_pFileHnd(pFile)
		{
			if (!m_pFileHnd)
				return;
			m_curPos = tell();
			_setOpened(true);
		}

        virtual qd::string_view getFileName() const { return m_Name; }
        void setFileName(const qd::string& Name) { m_Name = Name; }

		// MODE can be: "rb", "wb", "rt", "wt"
		bool open(const char* path, const char* pMode, qd::EIOErrorCode* pErr = nullptr);
		virtual void close() override;
	
		void openSafe(const char* path, const char* mode);
	
		virtual uint32_t read(void* pDest, uint32_t nBytes) override;
		virtual uint32_t write(const void* pSrc, uint32_t nBytes) override;
		virtual uint32_t getSize() override;
		virtual uint32_t seek(uint32_t Position, qd::EFileSeek Where = EFileSeek::SET) override;
		virtual uint32_t tell() override;
	
		inline qd::HndFile getFile() const { return m_pFileHnd; }
	
		// COPY WITH MOVE
		StdFile& operator= (StdFile&& rf)
		{
			m_nFileSize = rf.m_nFileSize;
			m_curPos = rf.m_curPos;
			m_bAOpened = rf.m_bAOpened;
			//m_Name = rf.m_Name;
			m_nChunks = rf.m_nChunks;
			m_pFileHnd = rf.release();
			return (*this);
		}
	
		// COPY CONSTRUCTORS
		StdFile(StdFile&& r) { *this = eastl::move(r); }
	
		qd::HndFile release()
		{
			qd::HndFile pFile = m_pFileHnd;
			m_pFileHnd = {};
			m_nFileSize = ~0u;
			m_curPos = ~0u;
			_setOpened(false);
			return pFile;
		}
	
		virtual ~StdFile() { TThis::close(); }
	
	}; /// class CStdioFile
	//////////////////////////////////////////////////////////////////////////
	
	
}; // namespace qd
