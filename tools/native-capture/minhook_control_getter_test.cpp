#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "MinHook.h"
#include "hde64.h"
#include "cover_control_code_certificate.h"

// Own executable buffers only. Never loads or opens the game or XR runtime.
using Getter=float(__fastcall*)(void*);
Getter trampoline{};unsigned detours{},checks{},failures{};void* expected_self{};bool same_self=true;
std::array<void*,16> allocations{};unsigned allocation_count{};
void check(bool value,char const* what){++checks;if(!value){++failures;std::fprintf(stderr,"FAIL %s\n",what);}}
#define C(x) check(bool(x),#x)
extern "C" __declspec(noinline) float __fastcall Detour(void* self){++detours;same_self=same_self&&self==expected_self;return trampoline(self);}
void status(char const* name,char const* operation,MH_STATUS value){std::printf("{\"case\":\"%s\",\"operation\":\"%s\",\"status\":%d,\"name\":\"%s\"}\n",name,operation,int(value),MH_StatusToString(value));}
void dump(char const* name,char const* phase,void const* address,unsigned n){
    std::printf("{\"case\":\"%s\",\"phase\":\"%s\",\"address\":\"%p\",\"bytes\":\"",name,phase,address);
    const auto* p=static_cast<unsigned char const*>(address);for(unsigned i=0;i<n;++i)std::printf("%02x",p[i]);std::printf("\"}\n");
}
unsigned char* buffer(unsigned char const* bytes,std::size_t count){
    auto* page=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));
    C(page!=nullptr);if(!page)return nullptr;C(allocation_count<allocations.size());allocations[allocation_count++]=page;
    std::memset(page,0xcc,4096);auto* entry=page+0x100;std::memcpy(entry,bytes,count);C(FlushInstructionCache(GetCurrentProcess(),page,4096)!=FALSE);return entry;
}
DWORD invoke(Getter function,void* self,uint32_t& bits) noexcept {
    __try{const float value=function(self);std::memcpy(&bits,&value,4);return 0;}
    __except(EXCEPTION_EXECUTE_HANDLER){return GetExceptionCode();}
}
void values(char const* name,char const* phase,Getter function,void* self,bool hooked){
    constexpr uint32_t samples[]={0x3f200000,0xc0100000,0x80000000,0x7fc12345};
    for(auto sample:samples){std::memcpy(static_cast<unsigned char*>(self)+0x70,&sample,4);uint32_t got{};const auto before=detours;
        const auto exception=invoke(function,self,got);C(exception==0);C(got==sample);C(detours-before==unsigned(hooked));C(same_self);
        std::printf("{\"case\":\"%s\",\"phase\":\"%s\",\"input_bits\":\"%08x\",\"returned_bits\":\"%08x\",\"exception\":%lu,\"detours\":%u,\"rcx_preserved\":%s}\n",name,phase,sample,got,exception,detours-before,same_self?"true":"false");}
}
void getter(char const* name,unsigned char const* bytes,bool vex,bool fixed){
    auto* target=buffer(bytes,6);if(!target)return;const auto original_bytes=std::array<unsigned char,32>{};auto before=original_bytes;std::memcpy(before.data(),target,32);
    dump(name,"original_entry",target,32);hde64s decoded{};const auto length=hde64_disasm(target,&decoded);
    std::printf("{\"case\":\"%s\",\"hde_length\":%u,\"hde_flags\":\"%08x\",\"hde_error\":%s}\n",name,length,decoded.flags,(decoded.flags&F_ERROR)?"true":"false");
    alignas(16) std::array<unsigned char,0x100> object{};expected_self=object.data();same_self=true;detours=0;trampoline=nullptr;
    const auto function=reinterpret_cast<Getter>(target);values(name,"baseline",function,object.data(),false);
    void* raw_trampoline{};const auto created=MH_CreateHook(target,reinterpret_cast<void*>(&Detour),&raw_trampoline);status(name,"create",created);
    C(created==(vex&&!fixed?MH_ERROR_UNSUPPORTED_FUNCTION:MH_OK));C(!std::memcmp(before.data(),target,32));
    if(created==MH_OK){C(raw_trampoline!=nullptr);trampoline=reinterpret_cast<Getter>(raw_trampoline);dump(name,"real_trampoline",raw_trampoline,48);
        if(vex&&fixed){const auto* code=static_cast<unsigned char const*>(raw_trampoline);uintptr_t destination{};std::memcpy(&destination,code+11,8);
            C(!std::memcmp(code,bytes,5));C(code[5]==0xff&&code[6]==0x25);C(destination==reinterpret_cast<uintptr_t>(target+5));C(target[5]==0xc3);}
        values(name,"trampoline_before_enable",trampoline,object.data(),false);}
    else C(raw_trampoline==nullptr);
    const auto enabled=MH_EnableHook(target);status(name,"enable",enabled);C(enabled==(created==MH_OK?MH_OK:MH_ERROR_NOT_CREATED));
    dump(name,"entry_after_enable_attempt",target,32);values(name,"entry_after_enable_attempt",function,object.data(),created==MH_OK);
    if(created==MH_OK)values(name,"trampoline_while_enabled",trampoline,object.data(),false);
    const auto disabled=MH_DisableHook(target);status(name,"disable",disabled);C(disabled==(created==MH_OK?MH_OK:MH_ERROR_NOT_CREATED));
    C(!std::memcmp(before.data(),target,32));dump(name,"disabled_restored_entry",target,32);values(name,"disabled_original",function,object.data(),false);
    const auto removed=MH_RemoveHook(target);status(name,"remove",removed);C(removed==(created==MH_OK?MH_OK:MH_ERROR_NOT_CREATED));C(!std::memcmp(before.data(),target,32));trampoline=nullptr;
}
void compute(char const* name,std::array<unsigned char,32> const& bytes){
    auto* target=buffer(bytes.data(),bytes.size());if(!target)return;dump(name,"original_entry",target,32);
    void* original{};const auto created=MH_CreateHook(target,reinterpret_cast<void*>(&Detour),&original);status(name,"create_only",created);C(created==MH_OK);
    C(!std::memcmp(target,bytes.data(),bytes.size()));if(created==MH_OK){C(original!=nullptr);dump(name,"unexecuted_trampoline",original,48);}
    const auto removed=MH_RemoveHook(target);status(name,"remove_without_enable",removed);C(removed==(created==MH_OK?MH_OK:MH_ERROR_NOT_CREATED));C(!std::memcmp(target,bytes.data(),32));
    std::printf("{\"case\":\"%s\",\"original_executed\":false,\"trampoline_executed\":false,\"enable_requested\":false}\n",name);
}
void rejected(char const* name,unsigned char const* bytes,std::size_t size){
    auto* target=buffer(bytes,size);if(!target)return;std::array<unsigned char,32> before{};std::memcpy(before.data(),target,32);dump(name,"original_entry",target,32);
    void* original{};const auto created=MH_CreateHook(target,reinterpret_cast<void*>(&Detour),&original);status(name,"create_rejected",created);
    C(created==MH_ERROR_UNSUPPORTED_FUNCTION);C(original==nullptr);C(!std::memcmp(before.data(),target,32));
    const auto removed=MH_RemoveHook(target);status(name,"remove_uncreated",removed);C(removed==MH_ERROR_NOT_CREATED);
    std::printf("{\"case\":\"%s\",\"executed\":false,\"bytes_unchanged\":true}\n",name);
}
int main(int argc,char** argv){
    C(argc==2);if(argc!=2)return 1;const bool fixed=!std::strcmp(argv[1],"fixed");C(fixed||!std::strcmp(argv[1],"baseline"));
    std::printf("{\"pid\":%lu,\"avx_available\":%s,\"own_VirtualAlloc_only\":true,\"Game\":false,\"XR\":false}\n",GetCurrentProcessId(),IsProcessorFeaturePresent(PF_AVX_INSTRUCTIONS_AVAILABLE)?"true":"false");
    const auto initialized=MH_Initialize();status("global","initialize",initialized);C(initialized==MH_OK);if(initialized!=MH_OK)return 1;
    constexpr unsigned char vex[]={0xc5,0xfa,0x10,0x41,0x70,0xc3};constexpr unsigned char classic[]={0xf3,0x0f,0x10,0x41,0x70,0xc3};
    getter("v278_exact_vex",vex,true,fixed);getter("classic_movss",classic,false,fixed);
    constexpr unsigned char disp[]={0xc5,0xfa,0x10,0x41,0x71,0xc3};
    constexpr unsigned char rip[]={0xc5,0xfa,0x10,0x05,0,0,0,0,0xc3};
    constexpr unsigned char next[]={0xc5,0xfa,0x10,0x41,0x70,0x90,0xc3};
    constexpr unsigned char later[]={0x90,0xc5,0xfa,0x10,0x41,0x70,0xc3};
    rejected("vex_disp71",disp,sizeof disp);rejected("vex_rip_relative",rip,sizeof rip);rejected("vex_nonret_successor",next,sizeof next);rejected("vex_not_first",later,sizeof later);
    compute("limb_1D2A04",gears_cover_control_certificate::hooks[0].prefix);compute("single_1D396C",gears_cover_control_certificate::hooks[1].prefix);
    const auto ended=MH_Uninitialize();status("global","uninitialize",ended);C(ended==MH_OK);
    if(ended==MH_OK)for(unsigned i=0;i<allocation_count;++i)C(VirtualFree(allocations[i],0,MEM_RELEASE)!=FALSE);
    std::printf("{\"checks\":%u,\"failures\":%u,\"allocations\":%u,\"scope\":\"own executable buffers; no conclusion about Game guard cause\"}\n",checks,failures,allocation_count);
    return failures?1:0;
}
