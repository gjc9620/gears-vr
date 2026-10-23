#pragma once
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <io.h>
#include <share.h>
#include <string>

// Passive support evidence. Hooks only update bounded memory with a try-lock;
// the owner worker performs file/adapter queries. No pointers leave this sink.
namespace gears_support {
struct LastError {DWORD value{GetLastError()};~LastError(){SetLastError(value);}};
inline long long qpc(){LARGE_INTEGER v{};QueryPerformanceCounter(&v);return v.QuadPart;}
inline unsigned long long creation(){FILETIME a{},b{},c{},d{};if(!GetProcessTimes(GetCurrentProcess(),&a,&b,&c,&d))return 0;return (uint64_t(a.dwHighDateTime)<<32)|a.dwLowDateTime;}
inline void text(FILE* f,const char* s){
    fputc('"',f);for(const unsigned char* p=reinterpret_cast<const unsigned char*>(s?s:"");*p;++p){
        if(*p=='"'||*p=='\\'){fputc('\\',f);fputc(*p,f);}else if(*p<32)fprintf(f,"\\u%04x",unsigned(*p));else fputc(*p,f);
    }fputc('"',f);
}
inline std::string utf8(const wchar_t* s){
    if(!s)return {};int n=WideCharToMultiByte(CP_UTF8,0,s,-1,nullptr,0,nullptr,nullptr);
    if(n<=1)return {};std::string out(size_t(n),0);WideCharToMultiByte(CP_UTF8,0,s,-1,&out[0],n,nullptr,nullptr);out.resize(size_t(n-1));return out;
}
inline void begin(FILE* f,const char* role,const char* event){
    LARGE_INTEGER frequency{};QueryPerformanceFrequency(&frequency);
    fprintf(f,"{\"support_diagnostics\":true,\"schema\":1,\"role\":\"%s\",\"event\":\"%s\",\"pid\":%lu,\"creation_time\":%llu,\"observed_qpc\":%lld,\"qpc_frequency\":%lld,",role,event,GetCurrentProcessId(),creation(),qpc(),frequency.QuadPart);
}
inline void adapter(FILE* f,const char* role,LUID luid,const char* event="adapter",unsigned chain_id=0){
    begin(f,role,event);fprintf(f,"\"chain_id\":%u,\"adapter_luid_high\":%ld,\"adapter_luid_low\":%lu,",chain_id,luid.HighPart,luid.LowPart);
    IDXGIFactory4* factory{};IDXGIAdapter1* selected{};DXGI_ADAPTER_DESC1 desc{};
    HRESULT hr=CreateDXGIFactory1(IID_PPV_ARGS(&factory));
    if(SUCCEEDED(hr))hr=factory->EnumAdapterByLuid(luid,IID_PPV_ARGS(&selected));
    if(SUCCEEDED(hr))hr=selected->GetDesc1(&desc);
    fprintf(f,"\"available\":%s,\"hresult\":%ld,\"vendor_id\":%u,\"device_id\":%u,\"software_adapter\":%s,\"adapter_name\":",SUCCEEDED(hr)?"true":"false",hr,desc.VendorId,desc.DeviceId,(desc.Flags&DXGI_ADAPTER_FLAG_SOFTWARE)?"true":"false");
    text(f,utf8(desc.Description).c_str());fprintf(f,"}\n");fflush(f);
    if(selected)selected->Release();if(factory)factory->Release();
}
inline void system(FILE* f,const char* name,unsigned vendor,bool position,bool orientation,unsigned width,unsigned height){
    begin(f,"xr","system");fprintf(f,"\"system_name\":");text(f,name);
    fprintf(f,",\"vendor_id\":%u,\"position_tracking_supported\":%s,\"orientation_tracking_supported\":%s,\"maximum_width\":%u,\"maximum_height\":%u}\n",vendor,position?"true":"false",orientation?"true":"false",width,height);fflush(f);
}
struct Row {unsigned event{},chain{},generation{},width{},height{},format{},buffers{},flags{},accepted{};long hr{};long long stamp{};};
struct Chain {uintptr_t identity{};unsigned id{},generation{},width{},height{},format{},buffers{};LUID luid{};bool adapter_valid{},binding_accepted{};uint64_t presents{},tests{},unbound{},resizing{};};
class Recorder {
    SRWLOCK lock_=SRWLOCK_INIT;
    std::array<Chain,32> chains_{};std::array<Row,256> rows_{};
    size_t used_{},written_{};Row first_{};bool first_valid_{},first_saved_{};
    FILE* file_{};unsigned snapshots_{},rotations_{};std::array<bool,32> adapter_saved_{};
    Chain* chain(uintptr_t identity){
        if(!identity)return nullptr;
        for(auto& c:chains_)if(c.identity==identity&&identity)return &c;
        for(size_t i=0;i<chains_.size();++i)if(!chains_[i].identity&&identity){chains_[i].identity=identity;chains_[i].id=unsigned(i+1);return &chains_[i];}
        ++overflow;return nullptr;
    }
    void add(Row row){if(used_-written_>=rows_.size())++overflow;rows_[used_++%rows_.size()]=row;if(row.hr<0&&!first_valid_){first_=row;first_valid_=true;}}
    static void write(FILE* f,Row const& r){
        begin(f,"native","graphics_event");fprintf(f,"\"event_code\":%u,\"chain_id\":%u,\"generation\":%u,\"width\":%u,\"height\":%u,\"format\":%u,\"buffer_count\":%u,\"flags\":%u,\"accepted\":%u,\"source_line\":%u,\"hresult\":%ld,\"event_qpc\":%lld}\n",r.event,r.chain,r.generation,r.event==6?0:r.width,r.height,r.format,r.buffers,r.flags,r.accepted,r.event==6?r.width:0,r.hr,r.stamp);
    }
public:
    std::atomic<uint64_t> present_entries{},present1_entries{},present_failures{},overflow{},lock_skips{},write_failures{};
    void entry(bool present1){if(present1)++present1_entries;else ++present_entries;}
    void event(unsigned event,uintptr_t identity,unsigned width=0,unsigned height=0,unsigned format=0,unsigned buffers=0,long hr=0,unsigned accepted=0,LUID luid={},bool adapter_valid=false){
        LastError preserve;
        if(!TryAcquireSRWLockExclusive(&lock_)){++lock_skips;return;}
        auto* c=chain(identity);if(c){if(event==3)++c->generation;if(width)c->width=width;if(height)c->height=height;if(format)c->format=format;if(buffers)c->buffers=buffers;
            if(event==2)c->binding_accepted=accepted!=0;
            if(adapter_valid){c->luid=luid;c->adapter_valid=true;}
            add({event,c->id,c->generation,width,height,format,buffers,0,accepted,hr,qpc()});
        }else add({event,0,0,width,height,format,buffers,0,accepted,hr,qpc()});
        ReleaseSRWLockExclusive(&lock_);
    }
    void present(uintptr_t identity,unsigned flags,bool bound,bool resizing){
        LastError preserve;
        if(!TryAcquireSRWLockExclusive(&lock_)){++lock_skips;return;}
        if(auto* c=chain(identity)){++c->presents;if(flags&DXGI_PRESENT_TEST)++c->tests;if(!bound)++c->unbound;if(resizing)++c->resizing;}
        ReleaseSRWLockExclusive(&lock_);
    }
    void failure(uintptr_t identity,HRESULT hr){if(FAILED(hr)){++present_failures;event(5,identity,0,0,0,0,hr);}}
    void stop(unsigned reason,unsigned line){event(6,0,line,0,0,0,(reason==4||reason==6)?E_FAIL:S_OK,reason);}
    void flush(const wchar_t* path){
        std::array<Row,256> rows;std::array<Chain,32> chains;size_t start{},end{};Row first{};bool first_valid{};
        AcquireSRWLockExclusive(&lock_);end=used_;start=end-written_>rows_.size()?end-rows_.size():written_;rows=rows_;chains=chains_;first=first_;first_valid=first_valid_;ReleaseSRWLockExclusive(&lock_);
        if(!file_){file_=_wfsopen(path,L"w",_SH_DENYNO);if(!file_){++write_failures;return;}}
        if(std::ftell(file_)>=2*1024*1024){fclose(file_);file_=nullptr;std::wstring old=std::wstring(path)+L".1";const bool rotated=MoveFileExW(path,old.c_str(),MOVEFILE_REPLACE_EXISTING)!=0;file_=_wfsopen(path,rotated?L"w":L"a",_SH_DENYNO);if(rotated){++rotations_;adapter_saved_.fill(false);}else ++write_failures;if(!file_){++write_failures;return;}}
        if(std::ftell(file_)<4*1024*1024){
            for(size_t i=start;i<end;++i)write(file_,rows[i%rows.size()]);
            for(size_t i=0;i<chains.size();++i)if(chains[i].identity&&chains[i].adapter_valid&&!adapter_saved_[i]){adapter(file_,"native",chains[i].luid,"adapter",chains[i].id);adapter_saved_[i]=true;}
            begin(file_,"native","graphics_snapshot");fprintf(file_,"\"snapshot_index\":%u,\"present_entries\":%llu,\"present1_entries\":%llu,\"present_failures\":%llu,\"overflow\":%llu,\"lock_skips\":%llu,\"write_failures\":%llu,\"chains\":[",snapshots_,present_entries.load(),present1_entries.load(),present_failures.load(),overflow.load(),lock_skips.load(),write_failures.load());
            bool comma=false;for(auto const& c:chains)if(c.identity){fprintf(file_,"%s{\"chain_id\":%u,\"generation\":%u,\"binding_accepted\":%s,\"width\":%u,\"height\":%u,\"format\":%u,\"buffer_count\":%u,\"presents\":%llu,\"tests\":%llu,\"unbound\":%llu,\"resizing\":%llu}",comma?",":"",c.id,c.generation,c.binding_accepted?"true":"false",c.width,c.height,c.format,c.buffers,c.presents,c.tests,c.unbound,c.resizing);comma=true;}
            fprintf(file_,"],\"rotations\":%u}\n",rotations_);if(fflush(file_)==0){AcquireSRWLockExclusive(&lock_);written_=end;ReleaseSRWLockExclusive(&lock_);}else ++write_failures;++snapshots_;
        }else ++write_failures;
        if(first_valid&&!first_saved_){std::wstring destination=std::wstring(path)+L".first.json";FILE* f{};_wfopen_s(&f,destination.c_str(),L"w");if(f){write(f,first);fflush(f);_commit(_fileno(f));fclose(f);first_saved_=true;}else ++write_failures;}
    }
};
}
