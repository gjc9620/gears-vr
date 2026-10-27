#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstdio>
#include <cstring>
#include "MinHook.h"
#include "native_atom_overwrite_certificate.h"
unsigned checks{},failures{};
#define C(x)do{++checks;if(!(x)){++failures;printf("FAIL %u %s\n",__LINE__,#x);}}while(0)
extern "C" void Detour(){}
int main(){auto const&prefix=gears_atom_overwrite_certificate::prefix;
    auto*p=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));C(p!=nullptr);if(!p)return 1;
    std::memset(p,0xcc,4096);auto*target=p+0x100;std::memcpy(target,prefix.data(),32);C(FlushInstructionCache(GetCurrentProcess(),p,4096)!=FALSE);
    C(MH_Initialize()==MH_OK);void*original{};C(MH_CreateHook(target,reinterpret_cast<void*>(Detour),&original)==MH_OK);C(original!=nullptr);C(!std::memcmp(target,prefix.data(),32));
    C(!std::memcmp(original,prefix.data(),7)); // 3B MOV RAX,RSP + 4B original stack save
    C(MH_EnableHook(target)==MH_OK);C(std::memcmp(target,prefix.data(),7)!=0);
    C(MH_DisableHook(target)==MH_OK);C(!std::memcmp(target,prefix.data(),32));C(MH_RemoveHook(target)==MH_OK);C(!std::memcmp(target,prefix.data(),32));
    target[6]^=1;const bool certificate_matches=!std::memcmp(target,prefix.data(),32);C(!certificate_matches);
    C(MH_EnableHook(target)==MH_ERROR_NOT_CREATED); // certificate rejection never creates a replacement baseline
    C(MH_Uninitialize()==MH_OK);C(VirtualFree(p,0,MEM_RELEASE)!=FALSE);
    printf("atom_minhook: %u checks %u failures; actual_saved_prefix=1 original_executed=0 trampoline_executed=0 own_memory=1\n",checks,failures);return failures?1:0;
}
