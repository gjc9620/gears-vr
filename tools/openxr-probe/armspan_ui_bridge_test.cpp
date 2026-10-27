#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "armspan_ui_bridge.h"
#include <chrono>
#include <atomic>
#include <cwchar>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <thread>

namespace ui=gears_armspan_ui;
namespace bridge=armspan_ui_bridge;
namespace {
std::atomic<unsigned> checks{};
#define C(expression) do{++checks;if(!(expression))throw std::runtime_error(#expression);}while(false)
std::int64_t now(){return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();}
struct Fixture {
    ui::Binding binding{};ui::Packet packet{};HANDLE mutex{},mapping{};void* view{};
    Fixture(){
        binding.game_pid=binding.owner_pid=GetCurrentProcessId();
        binding.game_creation=binding.owner_creation=bridge::creation(GetCurrentProcess());
        LARGE_INTEGER counter{};C(QueryPerformanceCounter(&counter));
        for(unsigned i=0;i<8;++i){binding.nonce[i]=static_cast<unsigned char>(counter.QuadPart>>(8*i));binding.nonce[i+8]=static_cast<unsigned char>(binding.game_pid>>(4*i));}
        const auto name=bridge::name(binding.nonce);
        mutex=CreateMutexW(nullptr,FALSE,(name+L".Lock").c_str());C(mutex&&GetLastError()!=ERROR_ALREADY_EXISTS);
        mapping=CreateFileMappingW(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,0,sizeof(packet),name.c_str());C(mapping&&GetLastError()!=ERROR_ALREADY_EXISTS);
        view=MapViewOfFile(mapping,FILE_MAP_WRITE,0,0,sizeof(packet));C(view);
        std::memcpy(packet.magic,"GCALUI1",8);packet.version=1;packet.byte_size=sizeof(packet);
        packet.game_pid=binding.game_pid;packet.game_creation=binding.game_creation;
        packet.owner_pid=binding.owner_pid;packet.owner_creation=binding.owner_creation;packet.nonce=binding.nonce;
        packet.publisher_pid=binding.game_pid;packet.publisher_creation=binding.game_creation;
        packet.sequence=1;packet.updated_ns=now();publish();
    }
    void publish(){C(WaitForSingleObject(mutex,100)==WAIT_OBJECT_0);std::memcpy(view,&packet,sizeof(packet));C(ReleaseMutex(mutex));}
    ~Fixture(){if(view)UnmapViewOfFile(view);if(mapping)CloseHandle(mapping);if(mutex)CloseHandle(mutex);}
};
void content_contract(){
    ui::Nonce nonce{};C(!ui::decode_nonce(nullptr,nonce));C(!ui::decode_nonce("00000000000000000000000000000000",nonce));
    C(!ui::decode_nonce("ABCDEFabcdef01234567890123456789",nonce));C(!ui::decode_nonce("12",nonce));
    C(ui::decode_nonce("1234567890abcdef1234567890abcdef",nonce)&&nonce[0]==0x12&&nonce[15]==0xef);
    C(ui::valid({ui::Phase::holding,999,ui::Language::chinese}));C(!ui::valid({ui::Phase::holding,1000,ui::Language::english}));
    C(ui::valid({ui::Phase::saved,1000,ui::Language::english}));C(!ui::valid({ui::Phase::saved,999,ui::Language::english}));
    C(!ui::valid({ui::Phase::waiting,1,ui::Language::english}));C(!ui::valid({ui::Phase::waiting,0,static_cast<ui::Language>(2)}));
}
void current_and_stale(){
    Fixture f;bridge::Consumer reader;C(reader.open(f.binding));C(!reader.open(f.binding));ui::Packet out{};
    C(reader.read(f.packet.updated_ns,out)==bridge::Read::current&&out.sequence==1);
    f.packet.sequence++;f.packet.updated_ns=now();f.packet.content={ui::Phase::holding,750,ui::Language::chinese};f.publish();
    C(reader.read(f.packet.updated_ns,out)==bridge::Read::current&&ui::same(out.content,f.packet.content));
    const auto retained=out;
    C(reader.read(f.packet.updated_ns+750000001,out)==bridge::Read::stale&&ui::same_publication(out,retained));
    C(reader.read(f.packet.updated_ns-1,out)==bridge::Read::stale&&ui::same_publication(out,retained));
    const auto locked=CreateEventW(nullptr,TRUE,FALSE,nullptr),unlock=CreateEventW(nullptr,TRUE,FALSE,nullptr);C(locked&&unlock);
    std::thread busy([&]{C(WaitForSingleObject(f.mutex,100)==WAIT_OBJECT_0);C(SetEvent(locked));
        C(WaitForSingleObject(unlock,3000)==WAIT_OBJECT_0);C(ReleaseMutex(f.mutex));});
    C(WaitForSingleObject(locked,3000)==WAIT_OBJECT_0);C(reader.read(f.packet.updated_ns,out)==bridge::Read::current);
    C(reader.read(f.packet.updated_ns+750000001,out)==bridge::Read::stale);
    C(SetEvent(unlock));busy.join();C(CloseHandle(locked));C(CloseHandle(unlock));
    f.packet.sequence++;f.packet.updated_ns=now();f.packet.content={ui::Phase::saved,1000,ui::Language::chinese};f.publish();
    C(reader.read(f.packet.updated_ns,out)==bridge::Read::current&&out.content.phase==ui::Phase::saved);
    reader.close();C(reader.read(now(),out)==bridge::Read::unavailable);
}
void rejects(){
    for(unsigned variant=0;variant<10;++variant){Fixture f;bridge::Consumer reader;ui::Packet out{};C(reader.open(f.binding));
        C(reader.read(f.packet.updated_ns,out)==bridge::Read::current);const auto before=out;
        switch(variant){case 0:f.packet.sequence=0;break;case 1:f.packet.sequence++;f.packet.updated_ns--;break;
        case 2:f.packet.content.progress_permille=1;break;case 3:f.packet.sequence++;f.packet.publisher_creation++;break;
        case 4:f.packet.game_creation++;break;case 5:f.packet.owner_creation++;break;case 6:f.packet.nonce[0]^=1;break;
        case 7:f.packet.reserved=1;break;case 8:f.packet.byte_size--;break;case 9:f.packet.magic[0]='X';break;}
        f.publish();C(reader.read(now(),out)==bridge::Read::invalid);C(ui::same_publication(before,out));
        f.packet=before;f.publish();C(reader.read(now(),out)==bridge::Read::invalid);
    }
    Fixture f;bridge::Consumer wrong;auto binding=f.binding;binding.game_creation++;C(!wrong.open(binding));
    binding=f.binding;binding.owner_creation++;C(!wrong.open(binding));binding=f.binding;binding.nonce[0]^=1;C(!wrong.open(binding));
    bridge::Consumer abandoned;C(abandoned.open(f.binding));
    std::thread abandon([&]{C(WaitForSingleObject(f.mutex,100)==WAIT_OBJECT_0);});abandon.join();
    ui::Packet out{};C(abandoned.read(now(),out)==bridge::Read::invalid);
}
void publisher_lifetime(wchar_t const* executable){
    Fixture f;const auto event_name=bridge::name(f.binding.nonce)+L".Exit";
    const auto event=CreateEventW(nullptr,TRUE,FALSE,event_name.c_str());C(event);
    std::wstring command=L"\""+std::wstring(executable)+L"\" --event "+event_name;
    STARTUPINFOW startup{};startup.cb=sizeof(startup);PROCESS_INFORMATION child{};
    C(CreateProcessW(executable,command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,nullptr,&startup,&child));
    f.packet.publisher_pid=child.dwProcessId;f.packet.publisher_creation=bridge::creation(child.hProcess);f.publish();
    bridge::Consumer reader;C(reader.open(f.binding));ui::Packet out{};C(reader.read(now(),out)==bridge::Read::current);
    C(SetEvent(event));C(WaitForSingleObject(child.hProcess,3000)==WAIT_OBJECT_0);DWORD code{};C(GetExitCodeProcess(child.hProcess,&code)&&code==0);
    const auto before=out;C(reader.read(now(),out)==bridge::Read::publisher_stopped&&ui::same_publication(out,before));
    C(CloseHandle(child.hThread));C(CloseHandle(child.hProcess));C(CloseHandle(event));
}
} // namespace
int wmain(int argc,wchar_t** argv){try{
    if(argc==3&&std::wcscmp(argv[1],L"--event")==0){const auto event=OpenEventW(SYNCHRONIZE,FALSE,argv[2]);
        if(!event)return 3;const auto waited=WaitForSingleObject(event,5000);CloseHandle(event);return waited==WAIT_OBJECT_0?0:4;}
    if(argc==8&&std::wcscmp(argv[1],L"--python")==0){
        const std::wstring token=argv[2];std::string ascii;
        for(auto character:token){if(character>127)return 2;ascii.push_back(static_cast<char>(character));}
        ui::Nonce nonce{};C(ui::decode_nonce(ascii.c_str(),nonce));
        const auto pid=std::wcstoull(argv[3],nullptr,10),born=std::wcstoull(argv[4],nullptr,10);
        bridge::Consumer reader;C(reader.open({pid,born,pid,born,nonce}));ui::Packet out{};
        C(reader.read(now(),out)==bridge::Read::current);C(out.content.phase==ui::Phase::holding);
        C(out.content.progress_permille==std::wcstoul(argv[5],nullptr,10));
        C(static_cast<unsigned>(out.content.language)==std::wcstoul(argv[6],nullptr,10));C(out.sequence==std::wcstoull(argv[7],nullptr,10));
        std::printf("{\"python_cpp_ui\":true,\"packet_bytes\":%zu,\"sequence\":%llu,\"read_only_view\":true}\n",sizeof(out),out.sequence);return 0;
    }
    if(argc!=1)return 2;
    content_contract();current_and_stale();rejects();publisher_lifetime(argv[0]);
    std::printf("ArmspanUiBridgeTest: %u checks passed; owned IPC/processes only, no Game/XR/window\n",checks.load());return 0;
}catch(std::exception const& error){std::fprintf(stderr,"ArmspanUiBridgeTest failed after %u checks: %s\n",checks.load(),error.what());return 1;}}
