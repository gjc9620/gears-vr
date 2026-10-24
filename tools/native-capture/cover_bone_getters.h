#pragma once
#include <windows.h>
#include <bcrypt.h>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>

// The two read-only implementations were exported from historical artifact 1,
// then matched chunk-for-chunk in the purchased runtime (V and ab). This file
// contains hashes/addresses only, not the game's function bodies. A matching
// RVA alone is never permission to call. The owner must additionally certify
// the current controlled mesh, world and game thread at both call boundaries.
namespace gears_cover_bones {
struct Chunk {std::uintptr_t rva;unsigned size;const char* sha256;};
inline constexpr std::array<Chunk,8> chunks{{
    {0x3fe340,124,"0a0bb13f00ef00809736aa90c09cf49668f4dfc7686db1b93089321b56b4762e"},
    {0x931652,32,"6f57c5f5fa47519c48f01cb926d914b2ac75ad56d474b60d3e7fde500b074637"},
    {0x1d38d4,151,"19cab55eb3c9196f1ce021759c80f8636769c1d7eea6e1693a1f914ff9ef5d45"},
    {0x8c980e,20,"8156eba0efbf8118f6f12762934ce485473d9a9848b8e0c946249ba892cd95ef"},
    {0x473224,150,"f98d81f5f3fd90b44e82ff934c428140911b311508fbb624d9abeb74b799c0d4"},
    {0x94223c,32,"f4d52613bc18e76b83622770357e22a39e5fdce6bbaf374ff710379e4618c034"},
    {0x1d40b8,480,"e280f69cf3bfafc1134605dc8a714201f1555d525aef1e57caf0dff1e7b244c4"},
    {0x8c98d6,395,"7b0e6ae7032b0eaa481ee1af9de61fe0ceb17db41cec4bdc06d35a10e182d749"}
}};
inline bool digest_matches(void const* bytes,unsigned size,const char* expected){
    if(!bytes||!size||!expected||std::strlen(expected)!=64)return false;
    BCRYPT_ALG_HANDLE algorithm{};BCRYPT_HASH_HANDLE hash{};
    bool okay=BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
    if(okay)okay=BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)>=0;
    if(okay)okay=BCryptHashData(hash,static_cast<PUCHAR>(const_cast<void*>(bytes)),size,0)>=0;
    unsigned char digest[32]{};if(okay)okay=BCryptFinishHash(hash,digest,sizeof(digest),0)>=0;
    if(hash)BCryptDestroyHash(hash);if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);
    static constexpr char hex[]="0123456789abcdef";
    if(okay)for(unsigned i=0;i<32;++i)if(expected[2*i]!=hex[digest[i]>>4]||expected[2*i+1]!=hex[digest[i]&15])return false;
    return okay;
}
template<class Read> bool certify(std::uintptr_t base,Read read){
    if(!base)return false;
    for(auto const& chunk:chunks){
        if(chunk.size>512||base>UINTPTR_MAX-chunk.rva||base+chunk.rva>UINTPTR_MAX-chunk.size)return false;
        std::array<unsigned char,512> first{},last{};
        const auto source=reinterpret_cast<void const*>(base+chunk.rva);
        if(!read(first.data(),source,chunk.size)||!digest_matches(first.data(),chunk.size,chunk.sha256)||
           !read(last.data(),source,chunk.size)||std::memcmp(first.data(),last.data(),chunk.size))return false;
    }
    return true;
}
struct alignas(16) Matrix {float values[16]{};};
struct Result {int index=-1;unsigned reserved[3]{};Matrix world;};
struct BoneArray {std::uintptr_t data{};int count{},capacity{};};
static_assert(sizeof(BoneArray)==16);
inline bool same(BoneArray const& a,BoneArray const& b){return a.data==b.data&&a.count==b.count&&a.capacity==b.capacity;}
inline bool valid(Matrix const& m){
    for(float value:m.values)if(!std::isfinite(value))return false;
    if(m.values[3]!=0||m.values[7]!=0||m.values[11]!=0||m.values[15]!=1)return false;
    for(unsigned row=0;row<3;++row){
        const double norm=std::hypot(std::hypot(m.values[row*4],m.values[row*4+1]),m.values[row*4+2]);
        if(!std::isfinite(norm)||norm<.01||norm>100)return false;
    }
    return true;
}
// ASM-certified Windows x64 register ABIs: RCX=mesh, RDX=FName by value;
// RCX=mesh, RDX=caller-owned aligned FMatrix output, R8D=bone index.
// Each helper contains only POD locals so its SEH guard does not bypass C++
// destruction. Engine faults reject this read; no writer/rollback is involved.
inline bool call_match(std::uintptr_t entry,std::uintptr_t mesh,std::uint64_t name,int& index){
    using Function=int(*)(std::uintptr_t,std::uint64_t);
    __try{index=reinterpret_cast<Function>(entry)(mesh,name);return true;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
inline bool call_matrix(std::uintptr_t entry,std::uintptr_t mesh,int index,Matrix& output){
    using Function=void*(*)(std::uintptr_t,void*,int);
    __try{return reinterpret_cast<Function>(entry)(mesh,&output,index)==&output;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
template<class Read,class Guard> bool query(std::uintptr_t base,std::uintptr_t mesh,std::uint64_t bone_name,
                                         unsigned thread,Read read,Guard guard,Result& out){
    if(!mesh||!bone_name||!thread||GetCurrentThreadId()!=thread||!guard()||!certify(base,read))return false;
    // These offsets come from the fully certified implementation above. Limit
    // this first publisher to its own SpaceBases, with no MasterPose remap or
    // the native invalid-index identity-matrix fallback. Recheck the storage
    // header after both reads; the owner still supplies the UObject guard.
    BoneArray bones{},after{};std::uintptr_t master{};
    if(mesh>UINTPTR_MAX-0x2a4||!read(&bones,reinterpret_cast<void const*>(mesh+0x26c),sizeof(bones))||
       !bones.data||bones.count<=0||bones.count>1024||bones.capacity<bones.count||bones.capacity>2048||
       bones.data>UINTPTR_MAX-std::uintptr_t(bones.capacity)*64||
       !read(&master,reinterpret_cast<void const*>(mesh+0x29c),sizeof(master))||master)return false;
    Result candidate{};
    if(!call_match(base+0x1d38d4,mesh,bone_name,candidate.index)||candidate.index<0||candidate.index>=bones.count||!guard())return false;
    if(!call_matrix(base+0x1d40b8,mesh,candidate.index,candidate.world)||!valid(candidate.world)||!guard()||!certify(base,read))return false;
    if(!read(&after,reinterpret_cast<void const*>(mesh+0x26c),sizeof(after))||!same(bones,after)||
       !read(&master,reinterpret_cast<void const*>(mesh+0x29c),sizeof(master))||master)return false;
    out=candidate;return true;
}
} // namespace gears_cover_bones
