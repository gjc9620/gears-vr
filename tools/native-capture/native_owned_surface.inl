#pragma once
#include "native_owned_surface.h"
#include "native_metadata_contract.h"
#include <intrin.h>

namespace gears_native_owned {
// Include inside the capture translation unit only after its safe reader exists.
// Explicit reader/module input: no global probe state or compile flag dependency.
struct NativeOps {
 using Reader=bool (*)(void*,const void*,size_t);
 uintptr_t module{};Reader reader{};bool image_identity_verified{};
 NativeOps(uintptr_t game_base,Reader safe_reader,bool exact_image_verified):
  module(game_base),reader(safe_reader),image_identity_verified(exact_image_verified){}
 bool read(void* out,uintptr_t address,size_t size){return reader&&reader(out,reinterpret_cast<void*>(address),size);}
 DWORD thread(){return GetCurrentThreadId();}
 bool tls(uintptr_t& block,uintptr_t& context){uintptr_t vector=__readgsqword(0x58);unsigned char ready{};
  block=0;context=0;return vector&&read(&block,vector,8)&&block&&read(&ready,block+0x10,1)&&ready&&read(&context,block+0x20,8)&&context;}
 bool certify(uintptr_t& failed){
  // RVA-0x1000 into frozen section_1000.bin, SHA256
  // 911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6.
  // Root must additionally admit the exact supported game/module image identity.
  struct Window{uintptr_t rva;const char* hex;};
  static constexpr Window windows[]={
   {0x15abc8,"40534883ec508b842490000000458bd189442430458bd88b842488000000488b"},
   {0x15ac2c,"4883ec58418bc1458bd1f7d88b84249000000089442440451bc98b8424880000"},
   {0x15ac94,"40555356574154415541564157488dac2468ffffff4881ec98010000488b0589"},
   {0x15ad07,"4b8d049b48894520488d0dfa669701418bd08b4cc11ce83e070000448bc8418b"},
   {0x15ad93,"4183e0040f8500050000418bc185d20f85b70400008bcb81e1000400000f85c6"},
   {0x15b460,"85d2745a83f94a7f300f84178075008bd183ea1b0f848900000083ea010f8480"},
   {0x1592a4,"40534883ec20488bd9488b094885d27407f0ff82ccfeffff4889134885c97509"},
   {0x1753a0,"40534883ec20bbfffffffff00fc1590883eb0174088bc34883c4205bc3e84e75"},
   {0x1ac910,"40534883ec2065488b042558000000488bd9b910000000488b10803c11007418"},
   {0x224db8,"65488b042558000000488b08488d05f50b3b014889442440c744243800000000"},
   {0x224e22,"65488b042558000000488b08ba10000000803c0a007505e8fa9b310065488b04"}
  };
  failed=0;if(!image_identity_verified||!module||!reader)return false;
  for(auto const& w:windows){unsigned char expected[32]{},actual[32]{};
   for(unsigned i=0;i<32;++i){const char a=w.hex[2*i],b=w.hex[2*i+1];
    const unsigned hi=a<='9'?unsigned(a-'0'):unsigned(a-'a'+10),lo=b<='9'?unsigned(b-'0'):unsigned(b-'a'+10);
    expected[i]=static_cast<unsigned char>((hi<<4)|lo);}
   if(!read(actual,module+w.rva,32)||memcmp(actual,expected,32)){failed=w.rva;return false;}}
  uint32_t format{};if(!read(&format,module+0x1ad147c,4)||format!=28){failed=0x1ad147c;return false;}
  return certify_metadata(failed);
 }
 bool certify_metadata(uintptr_t& failed); // Exact metadata code windows below.
 bool create(uintptr_t context,uint64_t& slot,unsigned width,unsigned height,uintptr_t& returned){
  using Fn=uint64_t* (__fastcall*)(void*,uint64_t*,uint32_t,uint32_t,uint8_t,uint32_t,uint32_t);
  __try{returned=reinterpret_cast<uintptr_t>(reinterpret_cast<Fn>(module+0x15abc8)(reinterpret_cast<void*>(context),&slot,width,height,2,1,2));return true;}
  __except(EXCEPTION_EXECUTE_HANDLER){return false;}}
 bool acquire(uintptr_t raw,uintptr_t& held){
  __try{return SUCCEEDED(reinterpret_cast<ID3D12Resource*>(raw)->QueryInterface(__uuidof(ID3D12Resource),reinterpret_cast<void**>(&held)));}
  __except(EXCEPTION_EXECUTE_HANDLER){return false;}}
 bool describe(uintptr_t held,D3D12_RESOURCE_DESC& d){
  __try{d=reinterpret_cast<ID3D12Resource*>(held)->GetDesc();return true;}
  __except(EXCEPTION_EXECUTE_HANDLER){return false;}}
 bool device(uintptr_t held,uintptr_t& result){
  __try{return SUCCEEDED(reinterpret_cast<ID3D12Resource*>(held)->GetDevice(__uuidof(ID3D12Device),reinterpret_cast<void**>(&result)));}
  __except(EXCEPTION_EXECUTE_HANDLER){return false;}}
 bool release_com(uintptr_t held){__try{reinterpret_cast<IUnknown*>(held)->Release();return true;}
  __except(EXCEPTION_EXECUTE_HANDLER){return false;}}
 bool clear(uint64_t& slot){using Fn=uint64_t* (__fastcall*)(uint64_t*,uint64_t);
  __try{return reinterpret_cast<Fn>(module+0x1592a4)(&slot,0)==&slot&&slot==0;}
  __except(EXCEPTION_EXECUTE_HANDLER){return false;}}
 bool gamma(float& value){using Fn=float (__fastcall*)();
  __try{value=reinterpret_cast<Fn>(module+0x117700)();return true;}
  __except(EXCEPTION_EXECUTE_HANDLER){return false;}}
 bool color(uintptr_t self,void* out16,uintptr_t& returned){using Fn=void* (__fastcall*)(void*,void*);
  __try{returned=reinterpret_cast<uintptr_t>(reinterpret_cast<Fn>(module+0x1161d8)(reinterpret_cast<void*>(self),out16));return true;}
  __except(EXCEPTION_EXECUTE_HANDLER){return false;}}
};
inline bool NativeOps::certify_metadata(uintptr_t& failed){
 return gears_native_metadata::certify([&](void* out,uintptr_t address,size_t size){return read(out,address,size);},
  module,reinterpret_cast<uintptr_t>(&GetCurrentThreadId),failed);
}
} // namespace gears_native_owned
