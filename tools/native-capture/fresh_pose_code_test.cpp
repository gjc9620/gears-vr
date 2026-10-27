// Real BCrypt and MinHook on private saved bytes. Never executes game functions.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include <vector>
#include <array>
#include <cstdio>
#include <cstring>
#include "MinHook.h"
#include "fresh_pose_producer_certificate.h"
namespace cert=gears_fresh_pose_certificate;
unsigned checks{},failures{};void C(bool b){++checks;if(!b){++failures;printf("FAIL %u\n",checks);}}
bool hash(std::vector<unsigned char> const& b,char const* expected){BCRYPT_ALG_HANDLE a{};BCRYPT_HASH_HANDLE h{};unsigned char digest[32]{};
    bool okay=BCryptOpenAlgorithmProvider(&a,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
    if(okay)okay=BCryptCreateHash(a,&h,nullptr,0,nullptr,0,0)>=0;if(okay)okay=BCryptHashData(h,const_cast<unsigned char*>(b.data()),ULONG(b.size()),0)>=0;
    if(okay)okay=BCryptFinishHash(h,digest,32,0)>=0;if(h)BCryptDestroyHash(h);if(a)BCryptCloseAlgorithmProvider(a,0);
    constexpr char hex[]="0123456789abcdef";for(unsigned i=0;okay&&i<32;++i)okay=expected[i*2]==hex[digest[i]>>4]&&expected[i*2+1]==hex[digest[i]&15];return okay;}
extern "C" void __declspec(noinline) Detour(){++failures;}
int main(int argc,char** argv){C(argc==2);if(argc!=2)return 1;FILE* f{};C(!fopen_s(&f,argv[1],"rb"));if(!f)return 1;
    char magic[8]{};uint32_t count{};C(fread(magic,1,8,f)==8&&fread(&count,4,1,f)==1&&!memcmp(magic,"FRESH001",8)&&count==3);
    std::array<std::vector<unsigned char>,3> chunks;
    for(unsigned i=0;i<3;++i){uint64_t rva{};uint32_t n{};C(fread(&rva,8,1,f)==1&&fread(&n,4,1,f)==1);C(rva==cert::chunks[i].rva&&n==cert::chunks[i].size);if(n>65536)return 1;
        chunks[i].resize(n);C(fread(chunks[i].data(),1,n,f)==n);C(hash(chunks[i],cert::chunks[i].sha256));chunks[i][n/2]^=1;C(!hash(chunks[i],cert::chunks[i].sha256));chunks[i][n/2]^=1;}
    C(fgetc(f)==EOF);fclose(f);C(MH_Initialize()==MH_OK);
    for(unsigned i=0;i<2;++i){auto* block=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));C(block!=nullptr);if(!block)return 1;
        auto* target=block+0x100;auto const& bytes=chunks[i?2:0];memcpy(target,bytes.data(),32);C(!memcmp(target,cert::prefixes[i].data(),32));
        target[17]^=1;C(memcmp(target,cert::prefixes[i].data(),32)!=0);target[17]^=1; // Startup prefix would reject this change.
        C(FlushInstructionCache(GetCurrentProcess(),target,32)!=0);void* original{};
        C(MH_CreateHook(target,reinterpret_cast<void*>(Detour),&original)==MH_OK&&original);C(!memcmp(target,bytes.data(),32));
        C(MH_EnableHook(target)==MH_OK);C(memcmp(target,bytes.data(),5)!=0);C(MH_DisableHook(target)==MH_OK);C(!memcmp(target,bytes.data(),32));
        C(MH_RemoveHook(target)==MH_OK);C(!memcmp(target,bytes.data(),32));C(VirtualFree(block,0,MEM_RELEASE)!=0);
    }C(MH_Uninitialize()==MH_OK);printf("FreshPoseCode: %u checks %u failures; real BCrypt + MinHook saved entries create/enable/disable/restore; no original game execution\n",checks,failures);return failures?1:0;
}
