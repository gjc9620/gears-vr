#pragma once
#include <array>
#include <cstdint>
#include <windows.h>
#include <bcrypt.h>

namespace gears_native_metadata {
// Full chunks from PID16812's historical code section, SHA256
// 911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6.
// 1161d8 uses self only to call virtual+20. Its helper 117700 ignores self.
// The cold chunks are mandatory: entry prefixes alone do not certify this ABI.
struct Span {uintptr_t rva;ULONG size;const char* sha256;};
inline constexpr Span spans[]={
 {0x1161d8,117,"ce1d8740f45705133dacb3a78cbdc427a2387d17257ffb76912e6e119d7654b8"},
 {0x8a52a4,15,"5d34e568f26c4ccf06fd5e4fcb5a6d76c21b6b48d776eb0b331db3117f08633d"},
 {0x117700,20,"deefe96cc9bf252275447d0092c02c3fb76fc42a06dff6d60945b2391ecb912b"},
 {0x1161a4,50,"c0c4855068a596142d60903f8b91c5e02424502a63ca64a356b5a209e81b3e98"},
 {0x8a528c,24,"95b9f48f9f7121675cd4b49fe3077117503e34b39205174c481682af7889afb6"},
 {0x11722c,56,"7f57a6712951fbec030ba68dfd62a14d4f39fb632b0a76b42fb54d2cfecabce7"},
 {0xfe83c,118,"79e44a9c9052cdf4bfd9a8ffa0c63dcb2581308176bb0e2f4debffbf1cd202b0"},
 {0x117744,43,"70b1fcd407a3a2aad4fb104a0b48dda6233deb2844c4e44a084b4edae756b5ff"}
};
inline bool matches(const unsigned char* bytes,ULONG size,const char* expected){
 BCRYPT_ALG_HANDLE algorithm{};BCRYPT_HASH_HANDLE hash{};unsigned char digest[32]{};
 bool ok=BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
 if(ok)ok=BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)>=0;
 if(ok)ok=BCryptHashData(hash,const_cast<PUCHAR>(bytes),size,0)>=0;
 if(ok)ok=BCryptFinishHash(hash,digest,32,0)>=0;
 if(hash)BCryptDestroyHash(hash);if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);
 constexpr char hex[]="0123456789abcdef";
 for(unsigned i=0;ok&&i<32;++i)ok=expected[2*i]==hex[digest[i]>>4]&&expected[2*i+1]==hex[digest[i]&15];
 return ok;
}
template<class Read> bool certify(Read&& read,uintptr_t module,uintptr_t thread_id_function,uintptr_t& failed){
 failed=0;if(!module||module>UINTPTR_MAX-0x131b4a8||!thread_id_function)return false;
 std::array<unsigned char,118> bytes{};
 for(auto const& span:spans)if(!read(bytes.data(),module+span.rva,span.size)||!matches(bytes.data(),span.size,span.sha256)){
  failed=span.rva;return false;}
 // Raw RIP displacement at 117754; IDA's displayed data symbol is off by 1000.
 uintptr_t imported{};
 if(!read(&imported,module+0x131b4a0,8)||imported!=thread_id_function){failed=0x131b4a0;return false;}
 return true;
}
} // namespace gears_native_metadata
