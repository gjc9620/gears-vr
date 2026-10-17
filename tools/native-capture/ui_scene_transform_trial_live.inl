// Compile-time opt-in private experiment. No configuration means no mutation.
namespace ui_scene_trial_live {
#if defined(GEARS_UI_SCENE_SOURCE_TRIAL) && GEARS_UI_SCENE_SOURCE_TRIAL
namespace trial=gears_ui_scene_trial;
struct Active {trial::Config config;ULONGLONG expires{};};
std::shared_ptr<Active const> selected;
std::atomic<uint64_t> loaded_nonce{},begun{},restored{},rejected{},failed{},nested{},load_rejected{};
std::atomic<unsigned> before_depth{},before_capacity{};
std::atomic<int> last_shift_y{};
thread_local uintptr_t active_canvas{};
ULONGLONG last_poll{};FILETIME seen_file{};
#include "ui_scene_transform_code_contract.h"
bool certify_code(){
    std::array<unsigned char,2048> bytes{},again{};
    for(auto const& span:gears_ui_scene_code::spans){
        if(span.size>bytes.size()||base>UINTPTR_MAX-span.rva-span.size||
           !safe_copy(bytes.data(),reinterpret_cast<void const*>(base+span.rva),span.size)||
           !safe_copy(again.data(),reinterpret_cast<void const*>(base+span.rva),span.size)||
           !std::equal(bytes.begin(),bytes.begin()+span.size,again.begin())||
           !gears_native_metadata::matches(bytes.data(),span.size,span.sha256))return false;
    }return true;
}
void disable(){std::atomic_store(&selected,std::shared_ptr<Active const>{});}
bool modified(){return begun.load()!=0;}
void entering(){if(active_canvas){++nested;++failed;disable();}}
bool current(trial::Config const& c){return c.pid==GetCurrentProcessId()&&c.base==base&&
    c.started_qpc==static_cast<uint64_t>(session_started_qpc)&&c.world_generation==committed_world_generation.load();}
void poll(wchar_t const* path,FILE* log){
    const auto tick=GetTickCount64();if(tick-last_poll<200)return;last_poll=tick;
    WIN32_FILE_ATTRIBUTE_DATA file{};
    if(!GetFileAttributesExW(path,GetFileExInfoStandard,&file)){
        if(GetLastError()==ERROR_FILE_NOT_FOUND){disable();seen_file={};}
        return;
    }
    if((file.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))||
       (file.ftLastWriteTime.dwLowDateTime==seen_file.dwLowDateTime&&file.ftLastWriteTime.dwHighDateTime==seen_file.dwHighDateTime))return;
    const FILETIME previous_file=seen_file;seen_file=file.ftLastWriteTime;
    try {
        if(file.nFileSizeHigh||!file.nFileSizeLow||file.nFileSizeLow>262144)throw 1;
        FILE* input{};if(_wfopen_s(&input,path,L"rb")||!input){seen_file=previous_file;return;}
        struct Close {FILE* f;~Close(){fclose(f);}} close{input};std::string text(file.nFileSizeLow,'\0');
        if(fread(text.data(),1,text.size(),input)!=text.size()||fgetc(input)!=EOF){seen_file=previous_file;return;}
        trial::Config c;if(!trial::parse(text,c)||c.nonce<=loaded_nonce.load()||!current(c))throw 4;
        FILETIME creation{},exit{},kernel{},user{};
        if(!GetProcessTimes(GetCurrentProcess(),&creation,&exit,&kernel,&user)||
           c.creation_time!=((uint64_t(creation.dwHighDateTime)<<32)|creation.dwLowDateTime))throw 5;
        if(!certify_code()||!trial::checks(c,safe_copy))throw 6;
        auto config=std::make_shared<Active>();config->config=std::move(c);config->expires=GetTickCount64()+config->config.duration_ms;
        loaded_nonce=config->config.nonce;last_shift_y=config->config.shift_y;
        std::atomic_store(&selected,std::shared_ptr<Active const>(config));
        fprintf(log,"ui_path_trial_loaded nonce=%llu checks=%zu shift_x=%d shift_y=%d duration_ms=%u code_attested=1\n",
            config->config.nonce,config->config.checks.size(),config->config.shift_x,config->config.shift_y,config->config.duration_ms);fflush(log);
    }catch(...){++load_rejected;disable();fprintf(log,"ui_path_trial_load_rejected count=%llu\n",load_rejected.load());fflush(log);}
}
using Push=void(__fastcall*)(void*,float const*);
using Pop=void(__fastcall*)(void*,int,int,unsigned,unsigned);
template<class Original> uintptr_t invoke(gears_ui_scene_root::Candidate const& candidate,Original original){
    auto request=std::atomic_load(&selected);
    if(!request||GetTickCount64()>=request->expires||!current(request->config)||
       request->config.client!=candidate.client||request->config.scene!=candidate.scene)return original();
    if(active_canvas){++nested;++failed;disable();return original();}
    if(!trial::checks(request->config,safe_copy)){++rejected;disable();return original();}
    trial::Lease lease;
    auto push=[](uintptr_t canvas,trial::Matrix const& m){reinterpret_cast<Push>(base+0x2c7650)(reinterpret_cast<void*>(canvas),m.data());};
    auto pop=[](uintptr_t canvas,int index){reinterpret_cast<Pop>(base+0x138d70)(reinterpret_cast<void*>(canvas+0x38),index,1,80,8);};
    bool accepted=trial::begin(safe_copy,push,candidate.fcanvas,trial::translation(request->config.shift_x,request->config.shift_y),lease);
    before_depth=lease.before.stack.count;before_capacity=lease.before.stack.capacity;
    if(!accepted){++failed;disable();if(lease.owned&&!trial::finish(safe_copy,pop,lease))request_session_stop(SessionStop::proof_failure);
        else if(lease.failed&&!lease.owned)request_session_stop(SessionStop::proof_failure);return original();}
    ++begun;active_canvas=candidate.fcanvas;
    struct Reset {~Reset(){active_canvas=0;}} reset;
    uintptr_t result{};
    try{result=trial::run(safe_copy,pop,lease,original);}
    catch(...){++failed;disable();if(!lease.restored)request_session_stop(SessionStop::proof_failure);throw;}
    if(lease.restored)++restored;
    else{++failed;disable();request_session_stop(SessionStop::proof_failure);}
    if(!trial::checks(request->config,safe_copy)){++rejected;disable();}
    return result;
}
void progress(FILE* out){auto request=std::atomic_load(&selected);
    fprintf(out,"path_trial_compiled=1 path_trial_nonce=%llu path_trial_active=%u path_trial_begun=%llu path_trial_restored=%llu path_trial_rejected=%llu path_trial_failed=%llu path_trial_nested=%llu path_trial_load_rejected=%llu path_trial_depth=%u path_trial_capacity=%u path_trial_shift_y=%d ",
        loaded_nonce.load(),request&&GetTickCount64()<request->expires?1u:0u,begun.load(),restored.load(),rejected.load(),failed.load(),nested.load(),load_rejected.load(),before_depth.load(),before_capacity.load(),last_shift_y.load());}
#else
void entering(){}
bool modified(){return false;}
void poll(wchar_t const*,FILE*){}
void progress(FILE*){}
template<class Candidate,class Original> uintptr_t invoke(Candidate const&,Original original){return original();}
#endif
}
