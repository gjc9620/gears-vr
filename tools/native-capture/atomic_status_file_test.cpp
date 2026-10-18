#include "atomic_status_file.h"
#ifdef NDEBUG
#error AtomicStatusFileTest requires assertions
#endif
#include <cassert>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <thread>

namespace fs=std::filesystem;
std::string logical(std::string value){value.erase(std::remove(value.begin(),value.end(),'\r'),value.end());return value;}
std::string read(fs::path const& p){std::ifstream in(p,std::ios::binary);return logical({std::istreambuf_iterator<char>(in),{}});}
void publish(fs::path const& p,const char* text){gears_status::AtomicFile file(p.c_str());assert(file.get());std::fputs(text,file.get());static unsigned sequence=0;++sequence;const bool committed=file.commit();if(!committed)std::fprintf(stderr,"publish %u failed error=%lu\n",sequence,GetLastError());assert(committed);}
int fail_flush(FILE*){return EOF;}
int fail_close(FILE* file){std::fclose(file);return EOF;}
BOOL fail_replace(LPCWSTR,LPCWSTR){SetLastError(ERROR_SHARING_VIOLATION);return FALSE;}
int wmain(int argc,wchar_t** argv){
    wchar_t root[MAX_PATH]{};assert(GetTempPathW(MAX_PATH,root));
    auto dir=(argc==2?fs::path(argv[1]):fs::path(root))/(L"gears-status-test-"+std::to_wstring(GetCurrentProcessId()));
    assert(fs::create_directory(dir));auto path=dir/L"session.txt";
    const char* a="generation=A complete=1\n";const char* b="generation=B complete=1\n";
    publish(path,a);
    {gears_status::AtomicFile file(path.c_str());assert(file.get());std::fputs("generation=B",file.get());
        assert(read(path)==a);std::fputs(" complete=1\n",file.get());assert(file.commit());assert(read(path)==b);}
    publish(path,a);
    {gears_status::AtomicFile missing((dir/L"missing"/L"session.txt").c_str());assert(!missing.get());assert(!missing.commit());}
    {gears_status::AtomicFile file(path.c_str());assert(file.get());assert(std::fgetc(file.get())==EOF);assert(std::ferror(file.get()));
        std::fputs(b,file.get());assert(!file.commit());assert(read(path)==a);}
    HANDLE old=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr);
    assert(old!=INVALID_HANDLE_VALUE);publish(path,b);
    char raw[128]{};DWORD count{};assert(ReadFile(old,raw,sizeof(raw),&count,nullptr));assert(logical(std::string(raw,count))==a);
    assert(CloseHandle(old));assert(read(path)==b);
    for(unsigned failure=0;failure<3;++failure){
        publish(path,a);gears_status::FileApi api{};
        if(failure==0)api.flush=fail_flush;if(failure==1)api.close=fail_close;if(failure==2)api.replace=fail_replace;
        gears_status::AtomicFile file(path.c_str(),api);assert(file.get());std::fputs(b,file.get());assert(!file.commit());assert(read(path)==a);
    }
    {gears_status::AtomicFile file(path.c_str());assert(file.get());std::fputs(b,file.get());}assert(read(path)==a);
    // A legacy reader without delete sharing may prevent replacement. It must
    // never cause truncation of the old final file or a fallback write.
    old=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr);
    assert(old!=INVALID_HANDLE_VALUE);
    {gears_status::AtomicFile file(path.c_str());assert(file.get());std::fputs(b,file.get());assert(!file.commit());}
    assert(read(path)==a);assert(CloseHandle(old));
    // Concurrent reads use share-delete handles and see only whole A or B.
    std::atomic<bool> done{};std::atomic<unsigned> reads{};
    std::thread reader([&]{while(!done.load()){
        HANDLE h=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr);
        assert(h!=INVALID_HANDLE_VALUE);char data[128]{};DWORD bytes{};assert(ReadFile(h,data,sizeof(data),&bytes,nullptr));assert(CloseHandle(h));
        auto value=logical(std::string(data,bytes));assert(value==a||value==b);++reads;
    }});
    while(!reads.load())std::this_thread::yield();
    for(unsigned i=0;i<100;++i)publish(path,i%2?a:b);
    done=true;reader.join();assert(reads.load());
    assert(std::distance(fs::directory_iterator(dir),fs::directory_iterator{})==1);
    assert(fs::remove(path));assert(fs::remove(dir));
    std::printf("AtomicStatusFileTest passed: published_generations=100 observed_whole_generations=%u\n",reads.load());
}
