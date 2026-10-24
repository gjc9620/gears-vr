#pragma once
#include <Windows.h>
#include <atomic>
#include <cstdio>
#include <string>
#include <vector>
#include <cwchar>
#include <cstddef>
#include <cstring>
#include <cerrno>
#include <io.h>
#include <fcntl.h>
#include <share.h>
#include <sys/stat.h>

namespace gears_status {
// Requires Windows 10+ and filesystem support for FileRenameInfoEx/POSIX
// replacement. Unsupported operations fail; there is no alternate write path.
inline BOOL publish_closed(LPCWSTR temporary,LPCWSTR destination) {
    const size_t bytes=std::wcslen(destination)*sizeof(wchar_t);
    if(bytes>MAXDWORD-sizeof(FILE_RENAME_INFO)){SetLastError(ERROR_INVALID_PARAMETER);return FALSE;}
    std::vector<unsigned char> storage(sizeof(FILE_RENAME_INFO)+bytes);
    auto* info=reinterpret_cast<FILE_RENAME_INFO*>(storage.data());
    info->Flags=FILE_RENAME_FLAG_REPLACE_IF_EXISTS|FILE_RENAME_FLAG_POSIX_SEMANTICS;
    info->RootDirectory=nullptr;info->FileNameLength=static_cast<DWORD>(bytes);
    std::memcpy(info->FileName,destination,bytes);
    HANDLE handle=CreateFileW(temporary,DELETE,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                              nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(handle==INVALID_HANDLE_VALUE)return FALSE;
    const BOOL published=SetFileInformationByHandle(handle,FileRenameInfoEx,info,static_cast<DWORD>(storage.size()));
    const DWORD error=GetLastError();CloseHandle(handle);SetLastError(error);return published;
}

struct FileApi {
    int (*flush)(FILE*){std::fflush};
    int (*close)(FILE*){std::fclose};
    BOOL (*replace)(LPCWSTR,LPCWSTR){publish_closed};
    void (*observe)(const char*,bool,bool,int,DWORD){};
};
// Each final path is immutable until replacement with another CLOSED file.
// No retry, no fallback truncation, no changes to status/lease admission.
class AtomicFile {
    std::wstring destination_,temporary_;
    FILE* file_{};
    bool owned_{};
    FileApi api_;
    inline static std::atomic<unsigned long long> serial_{};
    void observe(const char* stage,bool entering,bool good=true,int crt_error=0,DWORD winerror=0) noexcept {
        if(!api_.observe)return;
        const DWORD saved=GetLastError();const int saved_errno=errno;
        api_.observe(stage,entering,good,crt_error,winerror);
        errno=saved_errno;SetLastError(saved);
    }
    void discard() noexcept {if(owned_){DeleteFileW(temporary_.c_str());owned_=false;}}
public:
    explicit AtomicFile(const wchar_t* destination,FileApi api={}) :destination_(destination),api_(api) {
        temporary_=destination_+L".tmp."+std::to_wstring(GetCurrentProcessId())+L"."+
            std::to_wstring(GetCurrentThreadId())+L"."+std::to_wstring(++serial_);
        int fd=-1;
        observe("status_open",true);
        const auto opened=_wsopen_s(&fd,temporary_.c_str(),_O_WRONLY|_O_CREAT|_O_EXCL|_O_TEXT,
                                    _SH_DENYWR,_S_IREAD|_S_IWRITE);
        observe("status_open",false,opened==0,opened);
        if(opened!=0)return;
        owned_=true;observe("status_fdopen",true);file_=_wfdopen(fd,L"w");
        observe("status_fdopen",false,file_!=nullptr,file_?0:errno);
        if(!file_){_close(fd);discard();}
    }
    AtomicFile(AtomicFile const&)=delete;
    AtomicFile& operator=(AtomicFile const&)=delete;
    ~AtomicFile(){if(file_)api_.close(file_);discard();}
    FILE* get() const noexcept {return file_;}
    bool commit() {
        if(!file_)return false;
        bool good=!std::ferror(file_);
        if(!good)observe("status_stream_error",false,false);
        observe("status_flush",true);
        const int flushed=api_.flush(file_);observe("status_flush",false,flushed==0,flushed?errno:0);
        if(flushed!=0)good=false;
        observe("status_close",true);
        const int closed=api_.close(file_);observe("status_close",false,closed==0,closed?errno:0);
        if(closed!=0)good=false;
        file_=nullptr;
        if(good){
            observe("status_replace",true);
            const BOOL replaced=api_.replace(temporary_.c_str(),destination_.c_str());
            observe("status_replace",false,replaced!=FALSE,0,replaced?0:GetLastError());
            if(replaced){owned_=false;return true;}
        }
        discard();return false;
    }
};
}
