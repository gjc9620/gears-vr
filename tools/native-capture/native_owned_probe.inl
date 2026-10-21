#pragma once
#if defined(GEARS_NATIVE_OWNED_PROBE) && GEARS_NATIVE_OWNED_PROBE
#include "native_owned_surface.inl"
namespace gears_owned_probe {
using Owner=gears_native_owned::Owner<gears_native_owned::NativeOps>;
std::atomic<unsigned> phase{};bool image_verified{};
std::unique_ptr<gears_native_owned::NativeOps> ops;
Owner owners[2];
struct Sample {gears_native_owned::Snapshot created{},retired{};float gamma{};
 unsigned char color[13]{};bool dimensions{},surface{},metadata_equal{},released{};};
Sample samples[2];uint64_t ticket{};DWORD thread{};unsigned error{};uintptr_t original_target{};

bool run(uint64_t frame,void* renderer){
 ticket=frame;thread=GetCurrentThreadId();
 ops=std::make_unique<gears_native_owned::NativeOps>(base,safe_copy,image_verified);
 // Current renderer's target is borrowed ONLY during this callback comparison.
 // It is never stored in either Owner or used after original_consume returns.
 uintptr_t vt{},slots[6]{};
 constexpr uintptr_t expected[6]={0x857dc8,0x514980,0x515f40,0x514a80,0x117700,0x1161d8};
 if(!image_verified||!safe_copy(&original_target,static_cast<unsigned char*>(renderer)+0x18,8)||
    !safe_copy(&vt,reinterpret_cast<void*>(original_target),8)||vt!=base+0x1706880||
    !safe_copy(slots,reinterpret_cast<void*>(vt),sizeof(slots))){error=1;return false;}
 for(unsigned i=0;i<6;++i)if(slots[i]!=base+expected[i]){error=2;return false;}
 for(unsigned i=0;i<2;++i){auto& owner=owners[i];auto& sample=samples[i];
  const auto quality=i?gears_native_owned::Quality::high:gears_native_owned::Quality::low;
  const bool created=owner.create_on_rhi(*ops,quality,frame+i);
  sample.created=owner.snapshot();bool valid=created;
  if(created){auto* adapter=owner.borrow_adapter();
   using Dim=uint32_t (__fastcall*)(Owner::Adapter*);using Ref=const uint64_t* (__fastcall*)(Owner::Adapter*);
   using Gamma=float (__fastcall*)(Owner::Adapter*);using Color=void* (__fastcall*)(Owner::Adapter*,void*);
   sample.dimensions=reinterpret_cast<Dim>(adapter->vtable[2])(adapter)==(i?8192u:4096u)&&
       reinterpret_cast<Dim>(adapter->vtable[3])(adapter)==(i?4096u:2048u);
   const auto* ref=reinterpret_cast<Ref>(adapter->vtable[1])(adapter);
   sample.surface=ref&&*ref==sample.created.surface;
   sample.gamma=reinterpret_cast<Gamma>(adapter->vtable[4])(adapter);
   unsigned char expected_color[16]{},actual_color[16]{};uintptr_t returned{};
   const bool original_ok=ops->color(original_target,expected_color,returned)&&returned==reinterpret_cast<uintptr_t>(expected_color);
   const bool adapter_ok=reinterpret_cast<Color>(adapter->vtable[5])(adapter,actual_color)==actual_color;
   memcpy(sample.color,actual_color,13);
   sample.metadata_equal=original_ok&&adapter_ok&&!memcmp(expected_color,actual_color,13)&&std::isfinite(sample.gamma);
   valid=sample.dimensions&&sample.surface&&sample.metadata_equal&&!owner.snapshot().adapter_fault;
  }
  // No adapter was passed to a renderer/Canvas command and no resource was
  // submitted to the GPU. All probe-only synchronous metadata calls returned;
  // the queue/fence/borrower obligations are therefore empty for this probe.
  const gears_native_owned::DrainProof drained{frame+i,true,true,true,true,true};
  sample.released=owner.retire_on_rhi(drained);sample.retired=owner.snapshot();
  if(!valid||!sample.released||owner.retained()){error=3+i;return false;}
 }
 return true;
}
}
void initialize_native_owned_probe(bool verified_image){gears_owned_probe::image_verified=verified_image;}
bool native_owned_probe_tick(uint64_t ticket,void* renderer){using namespace gears_owned_probe;
 std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
 unsigned expected=0;if(!phase.compare_exchange_strong(expected,1))return phase.load()!=3;
 const bool ok=run(ticket,renderer);phase.store(ok?2:3,std::memory_order_release);return ok;}
bool native_owned_probe_retained(){using namespace gears_owned_probe;
 const auto current=phase.load(std::memory_order_acquire);if(current==1)return true;if(!current)return false;
 for(auto& owner:owners)if(owner.retained())return true;return false;}
void print_native_owned_probe(FILE* file){using namespace gears_owned_probe;
 if(phase.load(std::memory_order_acquire)<2){fprintf(file,"native_owned_probe phase=%u pending=1\n",phase.load());return;}
 fprintf(file,"native_owned_probe phase=%u image_verified=%u ticket=%llu thread=%lu error=%u original_target=%llx retained=%u target_redirected=0 gpu_target_rendered=0\n",
  phase.load(),image_verified?1:0,ticket,thread,error,original_target,native_owned_probe_retained()?1:0);
 for(unsigned i=0;i<2;++i){auto const& s=samples[i];
  fprintf(file,"native_owned_probe_profile index=%u width=%u height=%u surface=%llx resource=%llx device=%llx dimensions=%u surface_match=%u metadata_equal=%u gamma=%.9g created_error=%u failed_window=%llx released=%u retired_phase=%u retired_error=%u color13=",
   i,s.created.width,s.created.height,s.created.surface,s.created.resource,s.created.device,s.dimensions?1:0,s.surface?1:0,
   s.metadata_equal?1:0,s.gamma,unsigned(s.created.error),s.created.failed_window,s.released?1:0,unsigned(s.retired.phase),unsigned(s.retired.error));
  for(auto c:s.color)fprintf(file,"%02x",unsigned(c));fprintf(file,"\n");}
}
#else
void initialize_native_owned_probe(bool){}
bool native_owned_probe_tick(uint64_t,void*){return true;}
bool native_owned_probe_retained(){return false;}
void print_native_owned_probe(FILE*){}
#endif
