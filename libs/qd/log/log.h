#pragma once
#include <EASTL/fixed_vector.h>
#include <EASTL/string.h>
#include <cstdarg>
#include <ctime>
#include "qd/stl/string.h"
#include "qd/debug/assert.h"
#include "qd/debug/exception.h"


namespace qd {


struct LogEntry {
    enum ELevel {
        E_DEBUG = 0x01,
        E_INFO = 0x02,
        E_WARNING = 0x04,
        E_ERROR = 0x08,
    };

    std::time_t timeStamp = 0;
    ELevel level = E_DEBUG;
    eastl::string message;
};  // struct LogEntry
//////////////////////////////////////////////////////////////////////////


class ILogWriter {
public:
    virtual void addLogEntry(const LogEntry& entry) = 0;
    virtual void destroy() {
    }
    virtual ~ILogWriter() = default;
};  // class ILogWriter


//////////////////////////////////////////////////////////////////////////
class Log {
    using LogWriter_ptr = ILogWriter*;
    eastl::fixed_vector<LogWriter_ptr, 4, false> mpLogWriters;

public:
    Log() = default;
    ~Log();

    void registerWriter(LogWriter_ptr p_writer);
    void destroyWriter(ILogWriter* p_ptr);
    void done();

    template <class TWriter>
    TWriter* createWriter_() {
        TWriter* pInst = new TWriter();
        registerWriter(eastl::move(pInst));
        return pInst;
    }

public:
    void logV(LogEntry::ELevel level, const char* message, va_list arguments);

#undef debug
#undef info
#undef warn
#undef error

    void debug(const char* message, ...) {
        va_list args;
        va_start(args, message);
        logV(LogEntry::E_DEBUG, message, args);
        va_end(args);
    }

    void info(const char* message, ...) {
        va_list args;
        va_start(args, message);
        logV(LogEntry::E_INFO, message, args);
        va_end(args);
    }

    void warn(const char* message, ...) {
        va_list args;
        va_start(args, message);
        logV(LogEntry::E_WARNING, message, args);
        va_end(args);
    }

    void error(const char* message, ...) {
        va_list args;
        va_start(args, message);
        logV(LogEntry::E_ERROR, message, args);
        va_end(args);
    }

};  // class Log
//////////////////////////////////////////////////////////////////////////



class TermMsg
{
    typedef TermMsg TThis;

public:
    enum eLogMsgType : uint8_t {
        W_VERBOSE = 1,
        W_DEBUG,
        W_INFO,
        W_WARNING,
        W_ERROR,
        W_FATAL,
    };
    qd::string m_logStr;
    eLogMsgType m_nMsgType = TThis::W_INFO;

public:
    TThis* operator->() { return this; }

    TermMsg(TermMsg::eLogMsgType nType)
        : m_nMsgType(nType)
    {}

    ~TermMsg()
    {
        _flushLogMsg();
    }

    TThis* setMsgV(const char* pFormat, va_list arguments);

    TThis* ASSERT_DLG()
    {
        ASSERT_F(0, "%s", m_logStr.c_str());
        return this;
    }

    qd::Exception GET_EXCEPTION(qd::EException excType = qd::EException::DEFAULT)
    {
        _flushLogMsg();
        return qd::Exception(excType, m_logStr);
    }

    inline void Throw_Exception(qd::EException excType = qd::EException::DEFAULT) { throw GET_EXCEPTION(excType); }

    inline void THROW_ASSERT(qd::EException excType = qd::EException::DEFAULT)
    {
        this->ASSERT_DLG();
        throw this->GET_EXCEPTION(excType);
    }

    const qd::string& getLogStr() const { return m_logStr; }

    void _flushLogMsg();
}; // class TermMsg


extern qd::Log& logConsole();
TermMsg logInfo(const char* msg, ...);
TermMsg logDbg(const char* msg, ...);
TermMsg logWarn(const char* msg, ...);
TermMsg logErr(const char* msg, ...);


};  // namespace qd
//////////////////////////////////////////////////////////////////////////

using qd::logDbg;
using qd::logWarn;
using qd::logErr;
