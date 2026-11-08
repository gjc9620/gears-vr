#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "fresh_wide_name.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
unsigned checks{},calls{};
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);std::abort();}}while(false)
bool read(void* out,std::uintptr_t from,std::size_t bytes) noexcept {
    ++calls;
    __try{std::memcpy(out,reinterpret_cast<void const*>(from),bytes);return true;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
int main(){
    SYSTEM_INFO info{};GetSystemInfo(&info);const auto page=std::size_t(info.dwPageSize);
    auto* memory=static_cast<wchar_t*>(VirtualAlloc(nullptr,page*2,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));CHECK(memory);
    wchar_t out[128]{};auto check=[&](std::uintptr_t p,std::size_t n=128){calls=0;return fresh_wide_name::text(p,out,n,page,read);};
    for(unsigned length=0;length<128;++length){
        for(unsigned i=0;i<length;++i)memory[i]=wchar_t(L'a'+i%26);memory[length]=0;
        CHECK(check(reinterpret_cast<std::uintptr_t>(memory)));CHECK(!std::memcmp(out,memory,(length+1)*2));CHECK(calls==1);
    }
    for(unsigned i=0;i<128;++i)memory[i]=L'x';CHECK(!check(reinterpret_cast<std::uintptr_t>(memory)));CHECK(calls==1&&out[127]==0);
    DWORD old{};CHECK(VirtualProtect(reinterpret_cast<char*>(memory)+page,page,PAGE_NOACCESS,&old));
    auto* edge=reinterpret_cast<wchar_t*>(reinterpret_cast<char*>(memory)+page-6);edge[0]=L'a';edge[1]=L'b';edge[2]=0;
    CHECK(check(reinterpret_cast<std::uintptr_t>(edge)));CHECK(calls==1&&!wcscmp(out,L"ab"));
    edge[2]=L'c';CHECK(!check(reinterpret_cast<std::uintptr_t>(edge)));CHECK(calls==2);
    CHECK(VirtualProtect(memory,page,PAGE_READWRITE|PAGE_GUARD,&old));
    CHECK(!check(reinterpret_cast<std::uintptr_t>(memory)));CHECK(calls==1); // No retry after one-shot guard.
    CHECK(!check(0));CHECK(calls==0);CHECK(!check(std::numeric_limits<std::uintptr_t>::max()-1));CHECK(calls==0);
    CHECK(VirtualFree(memory,0,MEM_RELEASE));
    std::printf("FreshWideNameTest: %u checks passed; page/noaccess/guard;95-char=1 page read vs prior12/scalar96; no FPS claim\n",checks);
}
