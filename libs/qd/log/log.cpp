#include "qtdDefines.h"
#include "log.h"
#include <qd/base/base.h>
#if QD_USE_SDL
#include <SDL_log.h>
#endif // 

#ifdef _WINDOWS
#include <windows.h>
#endif // _WINDOWS


namespace qd {


Log::~Log() {
    done();
}


void Log::registerWriter(LogWriter_ptr p_writer) {
    mpLogWriters.push_back(qtd::move(p_writer));
}


void Log::destroyWriter(ILogWriter* p_ptr) {
    if (!p_ptr)
        return;
    for (auto it = mpLogWriters.begin(); it != mpLogWriters.end(); ++it) {
        ILogWriter* pCurWriter = *it;
        if (pCurWriter == p_ptr) {
            mpLogWriters.erase(it);
            SAFE_DESTROY_AND_DELETE(pCurWriter);
            return;
        }
    }
}


void Log::done() {
    while (!mpLogWriters.empty()) {
        ILogWriter* pCurWriter = mpLogWriters.back();
        mpLogWriters.pop_back();
        SAFE_DESTROY_AND_DELETE(pCurWriter);
    }
}


void Log::logV(LogEntry::ELevel level, const char* message, va_list arguments) {
    LogEntry rec;
    rec.level = level;
    rec.message = qd::string_format_v(message, arguments);
    rec.timeStamp = std::time(nullptr);

    for (LogWriter_ptr& curWriter : mpLogWriters) {
        curWriter->addLogEntry(rec);
    }
}


qd::Log& logConsole() {
    static Log instance;
    return instance;
}

TermMsg logInfo(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    TermMsg r(TermMsg::W_INFO);
    r.setMsgV(fmt, args);
    va_end(args);
    return r; // std::move(r);
}


TermMsg logDbg(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    TermMsg r(TermMsg::W_DEBUG);
    r.setMsgV(fmt, args);
    va_end(args);
    return r; // std::move(r);
}


TermMsg logWarn(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    TermMsg r(TermMsg::W_WARNING);
    r.setMsgV(fmt, args);
    va_end(args);
    return r; // std::move(r);
}

TermMsg logErr(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    TermMsg r(TermMsg::W_ERROR);
    r.setMsgV(fmt, args);
    va_end(args);
    return r; // std::move(r);
}


qd::TermMsg::TThis* TermMsg::setMsgV(const char* pFormat, va_list arguments) {
    m_logStr = qd::string_format_v(pFormat, arguments);
    return this;
}


void TermMsg::_flushLogMsg() {
#if QD_USE_SDL
    SDL_LogMessage(0, (SDL_LogPriority)m_nMsgType, "%s", m_logStr.c_str());
#endif // 

#ifdef _WINDOWS
    OutputDebugStringA(m_logStr.c_str());
    OutputDebugStringA("\n");
#endif // _WINDOWS
}


}; // namespace qd
