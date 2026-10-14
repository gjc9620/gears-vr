#pragma once
#include <atomic>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <windows.h>
#include <d3d12.h>
#include <intrin.h>

#if defined(GEARS_NATIVE_SURFACE_PROBE) && GEARS_NATIVE_SURFACE_PROBE
namespace gears_native_surface_probe {
// Exact RVA windows from section_1000.bin, RVA - 0x1000 file offset.
// SHA256 911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6.
struct Window {uintptr_t rva;const char* hex;};
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
enum Error:unsigned {ok,not_ready,code_window,format_table,create_call,create_return,
 native_chain,com_acquire,description,device_query,context_changed,com_cleanup,native_cleanup};
struct Profile {unsigned width{},height{};uintptr_t surface{},resource{},device{};
 D3D12_RESOURCE_DESC desc{};unsigned error{},cleanup_error{};bool cleared{};
 uintptr_t retained_com_resource{},retained_com_device{};bool cleanup_uncertain{};};
struct Report {uint64_t ticket{};DWORD thread{};uintptr_t tls{},context{},failed_window{};
 unsigned attempts{},error{};Profile profiles[2]{};};
static std::atomic<unsigned> phase{}; // 0 idle,1 executing,2 pending,3 complete,4 error; terminal records immutable.
static Report report{};
static unsigned nibble(char c){return c<='9'?unsigned(c-'0'):unsigned(c-'a'+10);}
template<class Ops> bool certify(Ops& ops,Report& r){
 for(auto const& w:windows){unsigned char expected[32]{},actual[32]{};
  for(unsigned i=0;i<32;++i)expected[i]=static_cast<unsigned char>((nibble(w.hex[i*2])<<4)|nibble(w.hex[i*2+1]));
  if(!ops.read(actual,ops.module+w.rva,32)||memcmp(actual,expected,32)){
   r.failed_window=w.rva;r.error=code_window;return false;}}
 uint32_t format{};if(!ops.read(&format,ops.module+0x1ad147c,4)||format!=28){r.error=format_table;return false;}
 return true;
}
struct Chain {uintptr_t t{},l{},r{},d{};};
template<class Ops> bool chain(Ops& ops,uint64_t s,Chain& out){
 if(s<0x13c)return false;Chain c{};c.t=uintptr_t(s)-0x13c;uintptr_t vt{};uint32_t refs{};
 if(!ops.read(&vt,c.t,8)||vt!=ops.module+0x133a180||!ops.read(&refs,c.t+8,4)||!refs)return false;
 if(!ops.read(&c.l,c.t+0xc,8)||!c.l||!ops.read(&vt,c.l,8)||vt!=ops.module+0x133cc28||
    !ops.read(&c.r,c.l+0xc,8)||!c.r||!ops.read(&vt,c.r,8)||vt!=ops.module+0x133ecf8||
    !ops.read(&c.d,c.r+0xc,8)||!c.d)return false;
 out=c;return true;
}
inline bool valid_desc(D3D12_RESOURCE_DESC const& d,unsigned width,unsigned height){
 return d.Dimension==D3D12_RESOURCE_DIMENSION_TEXTURE2D&&d.Width==width&&d.Height==height&&
 d.DepthOrArraySize==1&&d.MipLevels==1&&d.Format==DXGI_FORMAT_R8G8B8A8_UNORM&&
 d.SampleDesc.Count==1&&d.SampleDesc.Quality==0&&d.Layout==D3D12_TEXTURE_LAYOUT_UNKNOWN&&
 d.Flags==D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
}
// Ops keeps primitive native/COM calls separately SEH guarded. Never infer that
// an exception rolled allocation back. A suspect native ref is retained/reported.
template<class Ops> bool run_profile(Ops& ops,Report const& r,Profile& p){
 uint64_t slot{};uintptr_t returned{},held{},device{};Chain owned{};bool held_trusted{},device_trusted{};
 uintptr_t start_block{},start_context{};
 if(!ops.tls(start_block,start_context)||start_block!=r.tls||start_context!=r.context){p.error=context_changed;return false;}
 if(!ops.create(r.context,slot,p.width,p.height,returned)){p.error=create_call;p.cleanup_uncertain=true;}
 else if(returned!=reinterpret_cast<uintptr_t>(&slot)||!slot)p.error=create_return;
 p.surface=slot;
 bool trusted=!p.error&&slot&&chain(ops,slot,owned);
 if(!p.error&&!trusted)p.error=native_chain;
 if(!p.error){p.resource=owned.d;
  held_trusted=ops.acquire(owned.d,held)&&held;
  if(!held_trusted){p.error=com_acquire;p.retained_com_resource=held;}
  else if(!ops.describe(held,p.desc)||!valid_desc(p.desc,p.width,p.height))p.error=description;
  else {device_trusted=ops.device(held,device)&&device;
   if(!device_trusted){p.error=device_query;p.retained_com_device=device;}
   else p.device=device;}}
 // Release only acquired COM references. Native S owns placed heap lifetime.
 if(device_trusted&&!ops.release_com(device)){p.cleanup_error=com_cleanup;p.retained_com_device=device;}
 if(held_trusted&&!ops.release_com(held)){p.cleanup_error=com_cleanup;p.retained_com_resource=held;}
 uintptr_t block{},context{};
 if(slot){
  if(p.retained_com_resource||p.retained_com_device){
   // Preserve the native placed-allocation owner while any COM cleanup outcome
   // is unknown. Do not trade an uncertain reference for uncertain heap lifetime.
   if(!p.cleanup_error)p.cleanup_error=com_cleanup;
  }
  else if(!ops.tls(block,context)||block!=r.tls||context!=r.context)p.cleanup_error=context_changed;
  else {Chain again{};
   // Unknown chain/identity is never passed into a native release function.
   if(!trusted||!chain(ops,slot,again)||memcmp(&again,&owned,sizeof(Chain)))p.cleanup_error=native_chain;
   else if(!ops.clear(slot)||slot){p.cleanup_error=native_cleanup;p.cleanup_uncertain=true;
    // An exception after clearing the slot is not proof that release completed.
    if(!slot)slot=p.surface;}
   else {p.cleared=true;uintptr_t after_block{},after_context{};
    if(!ops.tls(after_block,after_context)||after_block!=r.tls||after_context!=r.context)p.cleanup_error=context_changed;}}}
 p.surface=slot;return !p.error&&!p.cleanup_error&&p.cleared;
}
template<class Ops> bool tick(Ops& ops,uint64_t ticket){
 unsigned current=phase.load(std::memory_order_acquire);
 if(current==3)return true;if(current==4)return false;if(!ticket||current==1)return true;
 if(!phase.compare_exchange_strong(current,1,std::memory_order_acq_rel))return true;
 report.ticket=ticket;report.thread=ops.thread();++report.attempts;
 if(!ops.tls(report.tls,report.context)){
  if(report.attempts<60){phase.store(2,std::memory_order_release);return true;}
  report.error=not_ready;phase.store(4,std::memory_order_release);return false;}
 bool success=certify(ops,report);
 if(success){report.profiles[0].width=4096;report.profiles[0].height=2048;
  success=run_profile(ops,report,report.profiles[0]);
  if(success){report.profiles[1].width=8192;report.profiles[1].height=4096;
   success=run_profile(ops,report,report.profiles[1]);}}
 if(!success&&!report.error){for(auto const& p:report.profiles)if(p.error||p.cleanup_error){report.error=p.error?p.error:p.cleanup_error;break;}}
 phase.store(success?3u:4u,std::memory_order_release);return success;
}
struct NativeOps {
 uintptr_t module=base;
 bool read(void* out,uintptr_t address,size_t size){return safe_copy(out,reinterpret_cast<void*>(address),size);}
 DWORD thread(){return GetCurrentThreadId();}
 bool tls(uintptr_t& block,uintptr_t& context){uintptr_t vector=__readgsqword(0x58);unsigned char ready{};
  block=0;context=0;return vector&&read(&block,vector,8)&&block&&read(&ready,block+0x10,1)&&ready&&
   read(&context,block+0x20,8)&&context;}
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
};
} // namespace gears_native_surface_probe
bool native_surface_probe_tick(uint64_t frozen_ticket){gears_native_surface_probe::NativeOps ops;
 return gears_native_surface_probe::tick(ops,frozen_ticket);}
bool native_surface_probe_retained(){using namespace gears_native_surface_probe;
 const unsigned state=phase.load(std::memory_order_acquire);if(state!=3&&state!=4)return false;
 for(auto const& p:report.profiles)if(p.surface||p.retained_com_resource||p.retained_com_device||p.cleanup_uncertain)return true;return false;}
void print_native_surface_probe(FILE* file){using namespace gears_native_surface_probe;
 const unsigned state=phase.load(std::memory_order_acquire);
 if(state!=3&&state!=4){fprintf(file,"native_surface_probe phase=%u error_code=0 complete=0\n",state);return;}
 if(state==3)fprintf(file,"native_surface_probe phase=complete error_code=0 ticket=%llu thread=%lu attempts=%u tls=0x%llx context=0x%llx\n",
  report.ticket,report.thread,report.attempts,static_cast<unsigned long long>(report.tls),static_cast<unsigned long long>(report.context));
 else fprintf(file,"native_surface_probe phase=error error_code=%u failed_window=0x%llx ticket=%llu thread=%lu attempts=%u\n",
  report.error,static_cast<unsigned long long>(report.failed_window),report.ticket,report.thread,report.attempts);
 for(unsigned i=0;i<2;++i){auto const& p=report.profiles[i];
  // resource/device addresses are historical evidence, not retained handles.
  fprintf(file,"native_surface_probe_profile tier=%s size=%ux%u format=%u dimension=%u mips=%u array=%u samples=%u flags=%u resource=0x%llx device=0x%llx native_ref_cleared=%u retained_ref=0x%llx error_code=%u cleanup_error=%u retained_com_resource=0x%llx retained_com_device=0x%llx cleanup_uncertain=%u gpu_bound=0\n",
   i?"high":"low",p.width,p.height,unsigned(p.desc.Format),unsigned(p.desc.Dimension),unsigned(p.desc.MipLevels),unsigned(p.desc.DepthOrArraySize),p.desc.SampleDesc.Count,unsigned(p.desc.Flags),
   static_cast<unsigned long long>(p.resource),static_cast<unsigned long long>(p.device),p.cleared?1:0,static_cast<unsigned long long>(p.surface),p.error,p.cleanup_error,
   static_cast<unsigned long long>(p.retained_com_resource),static_cast<unsigned long long>(p.retained_com_device),p.cleanup_uncertain?1:0);}
}
#else
bool native_surface_probe_tick(uint64_t){return true;}
bool native_surface_probe_retained(){return false;}
void print_native_surface_probe(FILE*){}
#endif
