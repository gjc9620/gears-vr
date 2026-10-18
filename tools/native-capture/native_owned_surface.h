#pragma once
#include <atomic>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <windows.h>
#include <d3d12.h>
#include "../vr-common/render_size_plan.h"

namespace gears_native_owned {
enum class Quality:unsigned {low,high};
enum class Phase:unsigned {empty,creating,live,retiring,released,fault};
enum class Error:unsigned {none,invalid_config,tls_pending,code_identity,metadata_identity,
 create_call,create_return,native_chain,com_acquire,description,device_query,
 wrong_thread,missing_drain,metadata_call,unexpected_destructor,com_release,native_release};
struct DrainProof {
 uint64_t generation{};
 bool new_submissions_disabled{},renderers_retired{},canvas_commands_retired{},
      producer_cpu_access_retired{},gpu_fence_completed{};
};
struct Snapshot {
 Phase phase{};Error error{};uint64_t generation{},surface{};
 uintptr_t resource{},device{},failed_window{};unsigned width{},height{};
 DWORD creation_thread{};bool cleanup_uncertain{},adapter_fault{};
};
struct Chain {uintptr_t owner{},location{},resource{},d3d{};};
template<class Ops> bool read_chain(Ops& ops,uint64_t s,Chain& out){
 if(s<0x13c)return false;Chain c{};c.owner=uintptr_t(s)-0x13c;uintptr_t vt{};uint32_t refs{};
 if(!ops.read(&vt,c.owner,8)||vt!=ops.module+0x133a180||!ops.read(&refs,c.owner+8,4)||!refs||
    !ops.read(&c.location,c.owner+0xc,8)||!c.location||!ops.read(&vt,c.location,8)||vt!=ops.module+0x133cc28||
    !ops.read(&c.resource,c.location+0xc,8)||!c.resource||!ops.read(&vt,c.resource,8)||vt!=ops.module+0x133ecf8||
    !ops.read(&c.d3d,c.resource+0xc,8)||!c.d3d)return false;
 out=c;return true;
}
inline bool valid_desc(D3D12_RESOURCE_DESC const& d,unsigned w,unsigned h){
 return d.Dimension==D3D12_RESOURCE_DIMENSION_TEXTURE2D&&d.Width==w&&d.Height==h&&
 d.DepthOrArraySize==1&&d.MipLevels==1&&d.Format==DXGI_FORMAT_R8G8B8A8_UNORM&&
 d.SampleDesc.Count==1&&d.SampleDesc.Quality==0&&d.Layout==D3D12_TEXTURE_LAYOUT_UNKNOWN&&
 d.Flags==D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
}
// This prefix is not a full interface census. Only paths proved to use these six
// slots may borrow it. No RTTI or methods >= +0x30 are supplied.
// Integration owns a stable Ops and Owner address through process exit. All
// create/borrow/snapshot/retire operations require its SAME external API-order
// lock. After creation publication, getters may run concurrently if integration
// guarantees NO owner mutation/retirement throughout every borrow (the bounded
// scene probe retains until process exit). Retirement still requires exclusion
// of new borrowers and DrainProof, not merely a getter's duration.
// Destructor never performs COM/native release. Objects must not be automatic
// callback locals: a native command can keep the adapter pointer after return.
template<class Ops> class Owner {
public:
 struct Adapter {const uintptr_t* vtable;Owner* owner;};
private:
 Ops* ops_{};uint64_t surface_{};uintptr_t com_{},device_{},tls_{},context_{},failed_window_{};
 Chain chain_{};uint64_t generation_{},generation_floor_{};DWORD thread_{};unsigned width_{},height_{};
 bool trusted_surface_{},trusted_com_{},trusted_device_{},uncertain_{};
 std::atomic<Phase> phase_{Phase::empty};std::atomic<Error> error_{Error::none};
 std::atomic<bool> adapter_fault_{};std::atomic<unsigned> callbacks_{};
 static const uintptr_t* table(){static const uintptr_t entries[6]={
  reinterpret_cast<uintptr_t>(&destroy),reinterpret_cast<uintptr_t>(&surface),
  reinterpret_cast<uintptr_t>(&width),reinterpret_cast<uintptr_t>(&height),
  reinterpret_cast<uintptr_t>(&gamma),reinterpret_cast<uintptr_t>(&color)};return entries;}
 Adapter adapter_{table(),this};
 bool fail(Error error){error_.store(error);phase_.store(Phase::fault,std::memory_order_release);return false;}
 bool same_tls(){uintptr_t block{},context{};return ops_&&ops_->thread()==thread_&&ops_->tls(block,context)&&block==tls_&&context==context_;}
 bool callable(){return phase_.load(std::memory_order_acquire)==Phase::live&&!adapter_fault_.load();}
 void callback_fault(Error error){adapter_fault_.store(true);error_.store(error);phase_.store(Phase::fault,std::memory_order_release);}
 struct Callback {
  Owner& owner;explicit Callback(Owner& o):owner(o){owner.callbacks_.fetch_add(1);}
  ~Callback(){owner.callbacks_.fetch_sub(1);}
 };
 static void* __fastcall destroy(Adapter* self,unsigned){auto& o=*self->owner;Callback guard(o);
  // Never call the viewport's deleting destructor, free self, or release S.
  o.callback_fault(Error::unexpected_destructor);return self;}
 static const uint64_t* __fastcall surface(Adapter* self){auto& o=*self->owner;Callback guard(o);
  if(!o.callable())o.callback_fault(Error::metadata_call);
  // Persistent slot remains live even on faults until explicitly drained.
  return &o.surface_;}
 static uint32_t __fastcall width(Adapter* self){return self->owner->width_;}
 static uint32_t __fastcall height(Adapter* self){return self->owner->height_;}
 static float __fastcall gamma(Adapter* self){auto& o=*self->owner;Callback guard(o);float value=1.f;
  if(!o.callable()||!o.ops_->gamma(value))o.callback_fault(Error::metadata_call);
  return value;}
 static void* __fastcall color(Adapter* self,void* out16){auto& o=*self->owner;Callback guard(o);uintptr_t returned{};
  if(!out16){o.callback_fault(Error::metadata_call);return out16;}
  unsigned char temporary[16]{};
  // The certified native routine can call only this adapter's virtual+20.
  // It reads current engine settings without retaining the desktop target.
  const bool valid=o.callable()&&o.ops_->color(reinterpret_cast<uintptr_t>(self),temporary,returned)&&
   returned==reinterpret_cast<uintptr_t>(temporary)&&o.callable();
  if(!valid){
   // Retained 1161d8/1161a4 enum2 branch writes (1,1,1) + byte2.
   // A native exception can partially write temporary, never caller storage.
   const float fallback[3]={1.f,1.f,1.f};std::memcpy(temporary,fallback,12);temporary[12]=2;
   o.callback_fault(Error::metadata_call);
  }
  // Only 13 meaningful bytes are written, preserving caller padding 0d..0f.
  std::memcpy(out16,temporary,13);
  return out16;}
public:
 Owner()=default;~Owner()=default; // Deliberate retention; no worker cleanup.
 Owner(Owner const&)=delete;Owner& operator=(Owner const&)=delete;
 Owner(Owner&&)=delete;Owner& operator=(Owner&&)=delete;
 // Metadata executes against this persistent adapter. No original target lease
 // is invented from pointer readability; the owner stores no viewport pointer.
 bool create_on_rhi(Ops& ops,Quality quality,uint64_t generation){
  if(phase_.load()!=Phase::empty)return false;
  if(quality!=Quality::low&&quality!=Quality::high)return fail(Error::invalid_config);
  return create_on_rhi(ops,quality==Quality::low?4096u:8192u,quality==Quality::low?2048u:4096u,generation);
 }
 bool create_on_rhi(Ops& ops,unsigned width,unsigned height,uint64_t generation){
  if(phase_.load()!=Phase::empty)return false;
  if(!generation||generation<=generation_floor_||width<128||width>8192||width%2||height<64||height>8192)
   return fail(Error::invalid_config);
  uintptr_t block{},context{};if(!ops.tls(block,context)||!block||!context){error_=Error::tls_pending;return false;}
  ops_=&ops;tls_=block;context_=context;thread_=ops.thread();generation_=generation;
  width_=width;height_=height;
  phase_=Phase::creating;error_=Error::none;
  if(!ops.certify(failed_window_))return fail(Error::code_identity);
  if(!same_tls())return fail(Error::wrong_thread);
  uintptr_t returned{};
  if(!ops.create(context_,surface_,width_,height_,returned)){uncertain_=true;return fail(Error::create_call);}
  if(!surface_||returned!=reinterpret_cast<uintptr_t>(&surface_)){uncertain_=true;return fail(Error::create_return);}
  trusted_surface_=read_chain(ops,surface_,chain_);if(!trusted_surface_)return fail(Error::native_chain);
  trusted_com_=ops.acquire(chain_.d3d,com_)&&com_;if(!trusted_com_){uncertain_=true;return fail(Error::com_acquire);}
  D3D12_RESOURCE_DESC desc{};
  if(!ops.describe(com_,desc)||!valid_desc(desc,width_,height_))return fail(Error::description);
  trusted_device_=ops.device(com_,device_)&&device_;if(!trusted_device_){uncertain_=true;return fail(Error::device_query);}
  if(!same_tls())return fail(Error::wrong_thread);
  phase_.store(Phase::live,std::memory_order_release);return true;
 }
 Adapter* borrow_adapter(){return callable()?&adapter_:nullptr;}
 ID3D12Resource* borrow_resource(){return callable()?reinterpret_cast<ID3D12Resource*>(com_):nullptr;}
 ID3D12Device* borrow_device(){return callable()?reinterpret_cast<ID3D12Device*>(device_):nullptr;}
 bool retire_on_rhi(DrainProof const& proof){
  const auto state=phase_.load();if(state==Phase::released)return true;
  if(state==Phase::empty||state==Phase::creating||state==Phase::retiring||!ops_)return false;
  // Tokens/counts must be derived from actual retirement/fences by integration.
  if(proof.generation!=generation_||!proof.new_submissions_disabled||!proof.renderers_retired||
     !proof.canvas_commands_retired||!proof.producer_cpu_access_retired||!proof.gpu_fence_completed||
     callbacks_.load()){error_=Error::missing_drain;return false;}
  if(!same_tls()){error_=Error::wrong_thread;return false;}
  if(uncertain_||adapter_fault_.load())return false;
  Chain again{};
  if(surface_&&(!trusted_surface_||!read_chain(*ops_,surface_,again)||memcmp(&chain_,&again,sizeof(again))))return fail(Error::native_chain);
  // Revalidate the known cleanup code before executing; do not reallocate.
  if(!ops_->certify(failed_window_))return fail(Error::code_identity);
  phase_=Phase::retiring;
  if(device_){if(!trusted_device_||!ops_->release_com(device_)){uncertain_=true;return fail(Error::com_release);}device_=0;}
  if(com_){if(!trusted_com_||!ops_->release_com(com_)){uncertain_=true;return fail(Error::com_release);}com_=0;}
  if(!same_tls()){uncertain_=true;return fail(Error::wrong_thread);}
  if(surface_){const auto original_ref=surface_;
   if(!ops_->clear(surface_)||surface_){uncertain_=true;if(!surface_)surface_=original_ref;return fail(Error::native_release);}}
  if(!same_tls()){uncertain_=true;return fail(Error::wrong_thread);}
  error_=Error::none;phase_.store(Phase::released,std::memory_order_release);return true;
 }
 bool retained()const{return surface_||com_||device_||uncertain_;}
 // Only a fully released Owner can be reused, on its original RHI/TLS. Keep
 // the adapter and surface-slot addresses stable and never reuse a generation.
 bool recreate_on_rhi(Ops& ops,unsigned width,unsigned height,uint64_t generation){
  if(phase_.load()!=Phase::released||retained()||callbacks_.load()||generation<=generation_||!same_tls())return false;
  generation_floor_=generation_;chain_={};trusted_surface_=trusted_com_=trusted_device_=false;
  failed_window_=0;phase_=Phase::empty;return create_on_rhi(ops,width,height,generation);
 }
 Snapshot snapshot()const{return {phase_.load(),error_.load(),generation_,surface_,com_,device_,failed_window_,
  width_,height_,thread_,uncertain_,adapter_fault_.load()};}
};
static_assert(sizeof(void*)==8,"Certified Win64 ABI only");
} // namespace gears_native_owned
