#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "MinHook.h"
#include "cover_future_g_code_certificate.h"
// Real MinHook and actual saved getter bytes, executed in private VirtualAlloc.
// No process access, Game/XR, engine call, or deployment.
using Getter=float(__fastcall*)(void*);Getter trampoline{};
unsigned checks{},failures{},calls{};void* expected_self{};bool self_ok{};
void check(bool x,char const* msg){++checks;if(!x){++failures;fprintf(stderr,"FAIL %s\n",msg);}}
#define C(x) check(bool(x),#x)
extern "C" __declspec(noinline) float __fastcall Detour(void* self){++calls;self_ok=self_ok&&self==expected_self;return trampoline(self);}
DWORD invoke(Getter getter,void* object,uint32_t& bits){
    __try{const float result=getter(object);memcpy(&bits,&result,4);return 0;}
    __except(EXCEPTION_EXECUTE_HANDLER){return GetExceptionCode();}
}
unsigned char* buffer(std::array<unsigned char,32> const& bytes){
    auto* p=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));C(p!=nullptr);
    if(p){memset(p,0xcc,4096);memcpy(p+0x100,bytes.data(),32);C(FlushInstructionCache(GetCurrentProcess(),p,4096)!=FALSE);}return p;
}
void samples(Getter getter,bool hooked){
    std::array<unsigned char,256> object{};expected_self=object.data();self_ok=true;
    for(uint32_t value:{0x3f200000u,0xc0100000u,0x80000000u,0x7fc12345u}){
        memcpy(object.data()+0xac,&value,4);const auto before=calls;uint32_t got{};C(invoke(getter,object.data(),got)==0);C(got==value);C(calls-before==unsigned(hooked));C(self_ok);}
    expected_self=nullptr;uint32_t ignored{};const auto before=calls;C(invoke(getter,nullptr,ignored)==EXCEPTION_ACCESS_VIOLATION);C(calls-before==unsigned(hooked)&&self_ok);
}
void positive(std::array<unsigned char,32> bytes,bool on){
    auto* allocation=buffer(bytes);if(!allocation)return;auto* target=allocation+0x100;auto getter=reinterpret_cast<Getter>(target);
    samples(getter,false);void* original{};const auto created=MH_CreateHook(target,reinterpret_cast<void*>(&Detour),&original);
    C(created==(on?MH_OK:MH_ERROR_UNSUPPORTED_FUNCTION));C(!memcmp(target,bytes.data(),32));
    if(created==MH_OK){trampoline=reinterpret_cast<Getter>(original);C(original!=nullptr);auto* code=static_cast<unsigned char*>(original);
        uintptr_t destination{};memcpy(&destination,code+14,8);C(!memcmp(code,bytes.data(),8));C(code[8]==0xff&&code[9]==0x25);C(destination==reinterpret_cast<uintptr_t>(target+8));
        samples(trampoline,false);C(MH_EnableHook(target)==MH_OK);C(memcmp(target,bytes.data(),5)!=0);samples(getter,true);samples(trampoline,false);
        C(MH_DisableHook(target)==MH_OK);C(!memcmp(target,bytes.data(),32));samples(getter,false);C(MH_RemoveHook(target)==MH_OK);C(!memcmp(target,bytes.data(),32));
    }else {C(original==nullptr);C(MH_EnableHook(target)==MH_ERROR_NOT_CREATED);C(MH_RemoveHook(target)==MH_ERROR_NOT_CREATED);samples(getter,false);}
    C(VirtualFree(allocation,0,MEM_RELEASE)!=FALSE);trampoline=nullptr;
}
void negative(std::array<unsigned char,32> bytes){auto* allocation=buffer(bytes);if(!allocation)return;auto* target=allocation+0x100;void* original{};
    C(MH_CreateHook(target,reinterpret_cast<void*>(&Detour),&original)==MH_ERROR_UNSUPPORTED_FUNCTION);C(original==nullptr);C(!memcmp(target,bytes.data(),32));
    C(MH_EnableHook(target)==MH_ERROR_NOT_CREATED);C(MH_RemoveHook(target)==MH_ERROR_NOT_CREATED);C(VirtualFree(allocation,0,MEM_RELEASE)!=FALSE);
}
int main(int argc,char** argv){C(argc==3);if(argc!=3)return 1;const bool on=!strcmp(argv[1],"on");C(on||!strcmp(argv[1],"off"));
    C(IsProcessorFeaturePresent(PF_AVX_INSTRUCTIONS_AVAILABLE)!=FALSE);std::array<unsigned char,32> actual{};FILE* f{};C(fopen_s(&f,argv[2],"rb")==0);
    if(!f)return 1;C(fread(actual.data(),1,32,f)==32);C(fgetc(f)==EOF);C(fclose(f)==0);C(!memcmp(actual.data(),gears_cover_future_g_certificate::prefix.data(),9));
    C(MH_Initialize()==MH_OK);positive(actual,on);
    auto changed=actual;changed[4]=0xad;negative(changed);changed=actual;changed[3]=5;negative(changed);
    changed=actual;changed[8]=0x90;negative(changed);changed=actual;memmove(changed.data()+1,changed.data(),9);changed[0]=0x90;negative(changed);
    C(MH_Uninitialize()==MH_OK);printf("FutureG real MinHook %s: %u checks %u failures; actual_saved_32=1 own_VirtualAlloc=1 native_AV_passthrough=1 Game=0 XR=0\n",argv[1],checks,failures);return failures?1:0;
}
