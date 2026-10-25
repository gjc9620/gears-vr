#pragma once
#include <Windows.h>
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <cwchar>
#include <mutex>

namespace gears_startup {
// A separate, bounded worker journal: it is readable before session-status is
// published, and never waits for a renderer/status mutex. No hook calls this.
// WriteFile completes each record; no CRT FILE lock or forced disk flush is used.
class Trace {
    HANDLE file_{INVALID_HANDLE_VALUE};
    SRWLOCK lock_=SRWLOCK_INIT;
    unsigned rows_{};
    unsigned long long creation_{};
    long long frequency_{};
    std::atomic<unsigned> skipped_{},io_error_{};
public:
    static constexpr unsigned capacity=1024;
    ~Trace(){if(file_!=INVALID_HANDLE_VALUE)CloseHandle(file_);}
    void initialize(const wchar_t* directory) noexcept {
        const DWORD saved=GetLastError();const int saved_errno=errno;
        wchar_t path[MAX_PATH]{};FILETIME born{},a{},b{},c{};LARGE_INTEGER frequency{};
        if(file_==INVALID_HANDLE_VALUE&&GetProcessTimes(GetCurrentProcess(),&born,&a,&b,&c)&&
           QueryPerformanceFrequency(&frequency)&&
           swprintf_s(path,L"%s\\support-startup-%lu.jsonl",directory,GetCurrentProcessId())>0){
            creation_=(static_cast<unsigned long long>(born.dwHighDateTime)<<32)|born.dwLowDateTime;
            frequency_=frequency.QuadPart;
            file_=CreateFileW(path,FILE_APPEND_DATA,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                              nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
            if(file_==INVALID_HANDLE_VALUE)io_error_=GetLastError();
        }
        errno=saved_errno;SetLastError(saved);
    }
    void record(const char* stage,const char* phase,bool succeeded=true,int crt_error=0,DWORD winerror=0) noexcept {
        const DWORD saved=GetLastError();const int saved_errno=errno;
        if(file_!=INVALID_HANDLE_VALUE){
            if(TryAcquireSRWLockExclusive(&lock_)){
                if(rows_<capacity){
                    if(rows_==capacity-1){stage="trace_budget";phase="complete";succeeded=true;crt_error=0;winerror=0;}
                    LARGE_INTEGER qpc{};QueryPerformanceCounter(&qpc);char row[768]{};
                    // Call sites pass static labels only, never paths or game data.
                    const int n=sprintf_s(row,"{\"support_diagnostics\":true,\"schema\":1,\"role\":\"native\",\"event\":\"startup_stage\",\"pid\":%lu,\"creation_time\":%llu,\"event_qpc\":%lld,\"qpc_frequency\":%lld,\"thread_id\":%lu,\"sequence\":%u,\"stage\":\"%s\",\"phase\":\"%s\",\"succeeded\":%s,\"crt_errno\":%d,\"winerror\":%lu,\"skipped\":%u,\"trace_io_error\":%u}\n",
                        GetCurrentProcessId(),creation_,qpc.QuadPart,frequency_,GetCurrentThreadId(),++rows_,stage,phase,
                        succeeded?"true":"false",crt_error,winerror,skipped_.load(),io_error_.load());
                    DWORD written{};
                    if(n<=0||!WriteFile(file_,row,static_cast<DWORD>(n),&written,nullptr)||written!=static_cast<DWORD>(n))
                        io_error_=n<=0?ERROR_INVALID_DATA:GetLastError();
                }
                ReleaseSRWLockExclusive(&lock_);
            }else ++skipped_;
        }
        errno=saved_errno;SetLastError(saved);
    }
    // Observe acquisition separately from the protected work. The returned
    // guard keeps exactly the original scope and underlying mutex semantics.
    template<class Mutex> std::unique_lock<Mutex> acquire(const char* stage,Mutex& mutex) {
        record(stage,"enter");
        std::unique_lock<Mutex> guard(mutex);
        record(stage,"exit");
        return guard;
    }
    template<class F> auto call(const char* stage,F&& function)->decltype(function()) {
        struct Exit {Trace& trace;const char* stage;~Exit(){trace.record(stage,"exit");}};
        record(stage,"enter");Exit exit{*this,stage};return function();
    }
};
}
