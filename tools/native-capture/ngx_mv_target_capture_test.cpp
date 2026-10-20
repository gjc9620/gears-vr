// Compile the actual scheduling integration against a synthetic CPU boundary.
// No COM, native SDK, Windows calls, detours, game or GPU execution.
#include "ngx_mv_target_window.h"
#include "ngx_mv_sdk_receipt.h"
#include <array>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <utility>
#define GEARS_NGX_MV_TARGET_CAPTURE 1
namespace {
uint64_t tick=100;uint64_t GetTickCount64(){return tick;}
uintptr_t base=0x140000000;
using FrameHookMutex=std::recursive_mutex;
FrameHookMutex frame_hook_mutex;
std::atomic<uint64_t> committed_world_generation{3};
std::atomic<bool> recording{true};std::atomic<unsigned> active{};
bool safe_copy(void* out,void const* in,size_t size){if(!in)return false;std::memcpy(out,in,size);return true;}
}
namespace gears_submitted {struct FrameTicket {uint64_t world_generation{},id{};bool enabled{};};}
namespace gears_ngx_stereo {
struct TypedResourceLease {
    uintptr_t raw{},canonical{},device{};bool held{};
    TypedResourceLease()=default;
    TypedResourceLease(uintptr_t r,uintptr_t c,uintptr_t d,bool h):raw(r),canonical(c),device(d),held(h){}
    TypedResourceLease(TypedResourceLease const&)=default;
    TypedResourceLease& operator=(TypedResourceLease const&)=default;
    TypedResourceLease(TypedResourceLease&& other)noexcept{*this=std::move(other);}
    TypedResourceLease& operator=(TypedResourceLease&& other)noexcept{
        raw=other.raw;canonical=other.canonical;device=other.device;held=std::exchange(other.held,false);return *this;
    }
    bool retained()const{return held&&raw&&canonical&&device;}
    void release(){held=false;}
};
}
namespace gears_ngx_mv_census {struct Use {uint64_t revision{},world{},ticket{};uintptr_t renderer{},handle{},parameters{};};}
namespace {
struct Lifetime {
    bool retired{},stop{};
    bool stopping(){return stop;}
    bool frame(uintptr_t renderer,gears_submitted::FrameTicket& out){
        if(retired||!renderer)return false;out={3,41,true};
        if(renderer==0x800)out.id=40;return true;
    }
} submitted_lifetime;
gears_submitted::FrameTicket consuming_frozen_frame{3,40,true};
uint64_t consuming_frame_ticket=40;void* consuming_renderer=reinterpret_cast<void*>(0x800);
}
namespace ngx_stereo_host {
struct NativeCreateSource {uint64_t revision{},world{};uintptr_t handle{},parameters{};};
NativeCreateSource latest_native_create{2,3,0x100,0x200};uint64_t native_create_revision=2;unsigned in_wrappers{};
bool native_create_matches(NativeCreateSource const& s){
    auto const& c=latest_native_create;
    return s.revision&&s.revision==c.revision&&s.world==c.world&&s.handle==c.handle&&s.parameters==c.parameters;
}
struct WrapperScope {
    bool target_admitted{},target_sdk_returned{};uintptr_t target_structure{};
    unsigned target_ordinal{2};uint32_t target_sdk_result{};
    gears_ngx_mv_target::Context target_context{};gears_ngx_mv_target::SdkReceipt target_receipt;
};
}
namespace ngx_mv_descriptor_capture {
bool stopped{},seed_ready=true;
void stop_target(){stopped=true;}
bool snapshot_target_seed(uint64_t& observation,gears_ngx_mv_census::Use& use,gears_ngx_stereo::TypedResourceLease& mv){
    if(!seed_ready||stopped)return false;
    observation=2;use={2,3,40,0x800,0x100,0x200};mv={0x300,0x301,0xd00,true};return true;
}
}
namespace ngx_mv_compute_capture {void stop(){}void release_drained(){}void print(FILE*){}}
#include "ngx_mv_target_capture.inl"
namespace {
std::array<unsigned char,0x28> input{};
void reset(){
    using namespace ngx_mv_target_capture;
    window={};selected_source={};selected_mv.release();closed=leases_released=false;stop_cause=0;results={};
    tick=100;recording=true;active=0;committed_world_generation=3;submitted_lifetime={};
    ngx_stereo_host::latest_native_create={2,3,0x100,0x200};ngx_stereo_host::native_create_revision=2;ngx_stereo_host::in_wrappers=0;
    ngx_mv_descriptor_capture::stopped=false;ngx_mv_descriptor_capture::seed_ready=true;
    consuming_frozen_frame={3,40,true};consuming_frame_ticket=40;consuming_renderer=reinterpret_cast<void*>(0x800);
    const uintptr_t raw=0x300;input={};std::memcpy(input.data()+0x20,&raw,8);
}
void select(){
    ngx_stereo_host::WrapperScope selecting;
    ngx_mv_target_capture::eval_enter(selecting,true,base+0x9a35d3,0x500,input.data(),false);
    assert(!selecting.target_admitted&&ngx_mv_target_capture::window.snapshot().phase==gears_ngx_mv_target::Phase::waiting);
    consuming_frozen_frame={3,41,true};consuming_frame_ticket=41;consuming_renderer=reinterpret_cast<void*>(0xabc);++tick;
}
void enter(ngx_stereo_host::WrapperScope& e){
    ngx_mv_target_capture::eval_enter(e,true,base+0x9a35d3,0x500,input.data(),false);assert(e.target_admitted);++tick;
}
void eval(ngx_stereo_host::WrapperScope& e,uint32_t sdk=1,uint64_t wrapper=1){
    enter(e);ngx_mv_target_capture::sdk_enter(&e,0x500,0x100,0x200,0);
    ngx_mv_target_capture::sdk_leave(&e,sdk);++tick;ngx_mv_target_capture::eval_leave(e,true,false,wrapper);++tick;
}
void failed(){assert(ngx_mv_target_capture::window.snapshot().phase==gears_ngx_mv_target::Phase::failed&&ngx_mv_descriptor_capture::stopped);}
}
int main(){
    using ngx_mv_target_capture::ConsumeScope;
    using gears_ngx_mv_target::Phase;
    reset();select();
    {ConsumeScope consuming(consuming_frozen_frame,consuming_renderer,false);ngx_stereo_host::WrapperScope e;
        eval(e);submitted_lifetime.retired=true;consuming.finish();}
    assert(ngx_mv_target_capture::window.snapshot().phase==Phase::frozen&&ngx_mv_descriptor_capture::stopped);
    assert(ngx_mv_target_capture::window.snapshot().consume.renderer==0xabc&&ngx_mv_target_capture::results[0].sdk_returned);
    reset();select();
    {ConsumeScope consuming(consuming_frozen_frame,consuming_renderer,false);ngx_stereo_host::WrapperScope e;
        enter(e);ngx_mv_target_capture::eval_leave(e,true,false,1);consuming.finish();}failed(); // Wrapper success alone is insufficient.
    reset();select();
    {ConsumeScope consuming(consuming_frozen_frame,consuming_renderer,false);ngx_stereo_host::WrapperScope e;
        eval(e,0xbad00001u,0xbad00001u);consuming.finish();}failed();
    reset();select();
    {ConsumeScope consuming(consuming_frozen_frame,consuming_renderer,false);const uintptr_t changed=0x400;
        std::memcpy(input.data()+0x20,&changed,8);ngx_stereo_host::WrapperScope e;
        ngx_mv_target_capture::eval_enter(e,true,base+0x9a35d3,0x500,input.data(),false);consuming.finish();}failed();
    reset();select();committed_world_generation=4;ngx_mv_target_capture::poll();failed();
    reset();select();tick=5100;ngx_mv_target_capture::poll();failed();
    reset();select();
    {ConsumeScope consuming(consuming_frozen_frame,consuming_renderer,false);ngx_mv_target_capture::invalidate(1);consuming.finish();}failed();
    reset();select();{ConsumeScope consuming(consuming_frozen_frame,consuming_renderer,true);consuming.finish();}failed();
    reset();select();{ConsumeScope consuming(consuming_frozen_frame,consuming_renderer,false);}failed(); // Unwinding/absent normal return.
    reset();select();ngx_mv_target_capture::close();failed();
    active=1;recording=false;ngx_mv_target_capture::release_drained();assert(ngx_mv_target_capture::selected_mv.retained());
    active=0;ngx_mv_target_capture::release_drained();assert(!ngx_mv_target_capture::selected_mv.retained());
    std::puts("MV target actual scheduling CPU integration PASS");
}
