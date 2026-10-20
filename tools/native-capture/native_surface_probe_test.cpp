#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#ifndef GEARS_NATIVE_SURFACE_PROBE
#define GEARS_NATIVE_SURFACE_PROBE 1
#endif
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <vector>
[[maybe_unused]] static uintptr_t base=0x140000000;
[[maybe_unused]] static bool safe_copy(void*,const void*,size_t){return false;}
#include "native_surface_probe.inl"
#if GEARS_NATIVE_SURFACE_PROBE
using namespace gears_native_surface_probe;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"line %u: %s\n",__LINE__,#x);std::exit(1);}}while(0)
struct Mock {
 uintptr_t module=base,s=0x20013c,l=0x300000,r=0x400000,d=0x500000;
 unsigned creates{},tls_calls{},clears{};bool ready=true,bad_code{},bad_format{},bad_chain{},bad_desc{},bad_device{},bad_create{},bad_clear{},changed{},bad_release{};
 uintptr_t release_failure{};bool clear_return_mismatch{},create_zero_exception{};
 std::vector<unsigned> order;
 DWORD thread(){return 42;}
 bool tls(uintptr_t& b,uintptr_t& c){++tls_calls;b=0x600000;c=changed&&tls_calls>=3?0x700001:0x700000;return ready;}
 bool read(void* out,uintptr_t addr,size_t n){
  for(auto const& w:windows)if(addr==module+w.rva&&n==32){unsigned char bytes[32]{};
   for(unsigned i=0;i<32;++i)bytes[i]=static_cast<unsigned char>((nibble(w.hex[i*2])<<4)|nibble(w.hex[i*2+1]));
   if(bad_code)bytes[0]^=1;memcpy(out,bytes,n);return true;}
  uint64_t value{};
  if(addr==module+0x1ad147c)value=bad_format?29:28;
  else if(addr==s-0x13c)value=module+0x133a180;
  else if(addr==s-0x134)value=1;
  else if(addr==s-0x130)value=bad_chain?0:l;
  else if(addr==l)value=module+0x133cc28;
  else if(addr==l+0xc)value=r;
  else if(addr==r)value=module+0x133ecf8;
  else if(addr==r+0xc)value=d;
  else return false;
  memcpy(out,&value,n);return true;
 }
 unsigned width{},height{};
 bool create(uintptr_t context,uint64_t& slot,unsigned w,unsigned h,uintptr_t& result){
  CHECK(context==0x700000);width=w;height=h;++creates;order.push_back(w);
  slot=create_zero_exception?0:s;result=reinterpret_cast<uintptr_t>(&slot);return !bad_create&&!create_zero_exception;}
 bool acquire(uintptr_t raw,uintptr_t& held){CHECK(raw==d);held=d;order.push_back(1);return true;}
 bool describe(uintptr_t,D3D12_RESOURCE_DESC& desc){order.push_back(2);desc={};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  desc.Width=width;desc.Height=height;desc.DepthOrArraySize=1;desc.MipLevels=1;desc.Format=bad_desc?DXGI_FORMAT_UNKNOWN:DXGI_FORMAT_R8G8B8A8_UNORM;
  desc.SampleDesc.Count=1;desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;return true;}
 bool device(uintptr_t,uintptr_t& device){order.push_back(3);if(bad_device)return false;device=0x800000;return true;}
 bool release_com(uintptr_t value){order.push_back(value==0x800000?4:5);return !bad_release&&value!=release_failure;}
 bool clear(uint64_t& slot){++clears;order.push_back(6);CHECK(slot==s);if(bad_clear)return false;slot=0;return !clear_return_mismatch;}
};
static void reset(){phase.store(0);report={};}
int main(){
 {reset();Mock m;CHECK(tick(m,0));CHECK(report.attempts==0);m.ready=false;
  for(unsigned i=0;i<59;++i)CHECK(tick(m,1));CHECK(phase==2);CHECK(m.creates==0);CHECK(!tick(m,1));
  CHECK(report.error==not_ready&&phase==4);CHECK(!native_surface_probe_retained());CHECK(!tick(m,2));CHECK(report.attempts==60);}
 {reset();Mock m;m.ready=false;CHECK(tick(m,1));m.ready=true;CHECK(tick(m,2));CHECK(phase==3&&report.attempts==2);
  CHECK(m.creates==2&&m.clears==2);CHECK(!native_surface_probe_retained());
  const std::vector<unsigned> expected{4096,1,2,3,4,5,6,8192,1,2,3,4,5,6};CHECK(m.order==expected);
  CHECK(tick(m,3));CHECK(m.creates==2);}
 {reset();Mock m;m.bad_code=true;CHECK(!tick(m,1));CHECK(report.error==code_window&&report.failed_window==windows[0].rva);CHECK(!m.creates);}
 {reset();Mock m;m.bad_format=true;CHECK(!tick(m,1));CHECK(report.error==format_table);CHECK(!m.creates);}
 {reset();Mock m;m.bad_chain=true;CHECK(!tick(m,1));CHECK(m.creates==1&&!m.clears);CHECK(native_surface_probe_retained());CHECK(report.error==native_chain);}
 {reset();Mock m;m.bad_create=true;CHECK(!tick(m,1));CHECK(m.creates==1&&!m.clears);CHECK(native_surface_probe_retained());CHECK(report.error==create_call);}
 {reset();Mock m;m.create_zero_exception=true;CHECK(!tick(m,1));CHECK(m.creates==1&&!m.clears);
  CHECK(report.profiles[0].surface==0&&report.profiles[0].cleanup_uncertain);CHECK(native_surface_probe_retained());CHECK(report.error==create_call);}
 {reset();Mock m;m.bad_desc=true;CHECK(!tick(m,1));CHECK(m.creates==1&&m.clears==1);CHECK(!native_surface_probe_retained());CHECK(report.error==description);}
 {reset();Mock m;m.bad_device=true;CHECK(!tick(m,1));CHECK(m.clears==1);CHECK(!native_surface_probe_retained());CHECK(report.error==device_query);}
 {reset();Mock m;m.changed=true;CHECK(!tick(m,1));CHECK(m.clears==0);CHECK(native_surface_probe_retained());CHECK(report.error==context_changed);}
 {reset();Mock m;m.bad_clear=true;CHECK(!tick(m,1));CHECK(m.clears==1);CHECK(native_surface_probe_retained());CHECK(report.error==native_cleanup);}
 {reset();Mock m;m.clear_return_mismatch=true;CHECK(!tick(m,1));CHECK(m.clears==1);CHECK(native_surface_probe_retained());
  CHECK(report.profiles[0].surface==m.s&&!report.profiles[0].cleared&&report.profiles[0].cleanup_uncertain);CHECK(report.error==native_cleanup);}
 for(uintptr_t failed:{uintptr_t(0x800000),uintptr_t(0x500000)}){reset();Mock m;m.release_failure=failed;
  CHECK(!tick(m,1));CHECK(m.clears==0&&m.creates==1);CHECK(native_surface_probe_retained());
  CHECK(report.profiles[0].surface==m.s);CHECK(report.error==com_cleanup);
  CHECK((failed==m.d?report.profiles[0].retained_com_resource:report.profiles[0].retained_com_device)==failed);}
 {reset();Mock m;m.bad_release=true;CHECK(!tick(m,1));CHECK(m.clears==0);CHECK(native_surface_probe_retained());CHECK(report.error==com_cleanup);
  CHECK(report.profiles[0].surface==m.s&&!report.profiles[0].cleared);
  CHECK(report.profiles[0].retained_com_resource==m.d&&report.profiles[0].retained_com_device==0x800000);
  const std::vector<unsigned> expected{4096,1,2,3,4,5};CHECK(m.order==expected);}
 std::printf("native surface probe mocks: %u checks passed\n",checks);
}
#else
int main(){if(!native_surface_probe_tick(1)||native_surface_probe_retained())return 1;
 print_native_surface_probe(nullptr);std::puts("native surface probe disabled: no effect");}
#endif
