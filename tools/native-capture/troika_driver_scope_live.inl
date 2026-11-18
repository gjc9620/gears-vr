#include "../vr-common/weapon_troika_driver_scope.h"
#include "native_action_context.h"
// Tagged source admission on the existing continuous Tick/C0 hooks, not a
// second writer. Ordinary CoreSnapshot/Binding/Frame are never constructed.
namespace troika_driver_scope_live {
namespace api=gears_troika_driver;using Entry=gears_continuous_hand_entry::Entry;
std::atomic<bool> stopped{},failed{};std::atomic<unsigned> pending{};
std::atomic<std::uint64_t> ticks{},composes{},rejected{},closed{};
#if defined(GEARS_TROIKA_MOUNTED_SOURCE) && GEARS_TROIKA_MOUNTED_SOURCE
struct Local {
    api::Scope source;
    alignas(::Scope) unsigned char lease_storage[sizeof(::Scope)]{};::Scope* lease{};bool reserved{};
};
inline thread_local Local local;
api::Entry view(Entry const& e) noexcept {
    return {reinterpret_cast<std::uintptr_t>(&e),e.rcx,e.ra,e.rdx,e.r8,e.r9,e.rsp,e.invocation,e.thread,e.kind};
}
bool mapping_same(gears_weapon::TroikaMountedMapping const& a,gears_weapon::TroikaMountedMapping const& b) noexcept {
    if(!(a.identity==b.identity)||a.publication_sequence!=b.publication_sequence||a.source_ticket!=b.source_ticket||
       a.units_per_metre!=b.units_per_metre||a.new_publication!=b.new_publication||a.continuity_changed!=b.continuity_changed||
       a.hands.input_sequence!=b.hands.input_sequence||a.hands.input_sync_ns!=b.hands.input_sync_ns||a.hands.input_received_ns!=b.hands.input_received_ns||
       a.hands.stereo_received_ns!=b.hands.stereo_received_ns||a.hands.tracked_mask!=b.hands.tracked_mask||!gears_vr::stereo_same_pose(a.world_head,b.world_head))return false;
    for(unsigned side=0;side<2;++side)if(!gears_vr::stereo_same_pose(a.hands.world[side],b.hands.world[side]))return false;
    return true;
}
bool current(api::Snapshot const& saved) noexcept {
    auto const& ledger=gears_native_context::thread_ledger;
    if(stopped.load()||failed.load()||ledger.poisoned||ledger.depth[unsigned(gears_native_context::Kind::stereo)]||
       !local.lease||!local.lease->world()||!api::valid(saved,rig_tracking_live::clock_ns()))return false;
    gears_troika_control::Decision actual{};gears_weapon::TroikaMountedMapping mapping{};
    return troika_control_live::read_for(saved.source,actual)&&!std::memcmp(&actual,&saved.control,sizeof actual)&&
        troika_scene_tracking::mapped_for(saved.source.frame,mapping)&&mapping_same(mapping,saved.mapping)&&
        troika_control_live::read_for(saved.source,actual)&&!std::memcmp(&actual,&saved.control,sizeof actual);
}
// true means this real mounted Driver event was routed here, including a
// rejected/nested event. It must not fall through to the ordinary Pawn lane.
bool begin_tick(Entry& e,bool (*certify_mesh)(std::uintptr_t),bool ordinary_active) noexcept {
    if(e.kind!=2)return false;
    if(local.source.active()&&e.rcx==local.source.tick().mesh){
        local.source.begin_tick(view(e),{},0,0,false,ordinary_active);++rejected;return true;
    }
    auto const& ledger=gears_native_context::thread_ledger;
    if(ledger.poisoned||ledger.depth[unsigned(gears_native_context::Kind::stereo)])return false;
    gears_troika_geometry::Source hint{};
    if(stopped.load()||failed.load()||!troika_control_live::driver_hint(e.rcx,hint))return false;
    if(ordinary_active||!certify_mesh){++rejected;return true;}
    try{
        if(local.lease||local.reserved){failed=true;++rejected;return true;}
        local.reserved=true;++pending; // Also covers construction/validation failures.
        local.lease=::new(static_cast<void*>(local.lease_storage)) ::Scope;
        bool accepted=false;
        if(local.lease->world()&&e.thread==GetCurrentThreadId()&&e.ra==base+0x12e537&&certify_mesh(e.rcx)){
            api::Snapshot next{};next.source=hint;
            if(troika_control_live::read_for(hint,next.control)&&troika_scene_tracking::mapped_for(hint.frame,next.mapping)&&current(next))
                accepted=local.source.begin_tick(view(e),next,rig_tracking_live::clock_ns(),base+0x12e537,true,false);
        }
        if(accepted){++ticks;return true;}
        local.lease->~Scope();local.lease=nullptr;local.reserved=false;--pending;++rejected;return true;
    }catch(...){failed=true;++rejected;return true;}
}
bool begin_compose(Entry& e,bool attested) noexcept {
    if(e.kind!=0||!local.source.active()||e.rcx!=local.source.tick().mesh)return false;
    const bool ready=current(local.source.source());
    if(local.source.begin_compose(view(e),rig_tracking_live::clock_ns(),base+gears_cover_pose_phase::compose_caller,attested&&ready))++composes;
    else ++rejected;
    return true;
}
// Future sole C0 writer consumes THIS exact tagged source/scope. A successful
// copy does not grant Backend commit, model geometry, fingers or engine calls.
bool copy_for_compose(Entry const& e,api::Snapshot& output) noexcept {
    output={};return !stopped.load()&&!failed.load()&&current(local.source.source())&&local.source.copy(view(e),rig_tracking_live::clock_ns(),output);
}
bool owns_compose(Entry const& e) noexcept {return local.source.matches_compose(view(e));}
bool copy_for_tick(api::Snapshot& output) noexcept {
    if(stopped.load()||failed.load()||!current(local.source.source()))return false;
    auto outer=reinterpret_cast<Entry const*>(local.source.tick().address);
    return outer&&local.source.copy_tick(view(*outer),rig_tracking_live::clock_ns(),output);
}
bool retained_for_compose(Entry const& e,api::Snapshot& output,Entry const*& outer,::Scope*& lease) noexcept {
    outer=nullptr;lease=nullptr;
    if(!local.lease||!local.lease->world()||!local.source.retained(view(e),output))return false;
    outer=reinterpret_cast<Entry const*>(local.source.tick().address);lease=local.lease;
    return outer&&api::same(view(*outer),local.source.tick());
}
bool end(Entry const& e,bool returned) noexcept {
    if(!local.source.active()||e.rcx!=local.source.tick().mesh)return false;
    try{
        if(e.kind==0){
            if(local.source.composing())local.source.end_compose(view(e),returned);
            return true;
        }
        if(e.kind!=2)return false;
        if(local.source.end_tick(view(e),returned)){
            if(!local.lease||!local.reserved){failed=true;return true;}
            local.lease->~Scope();local.lease=nullptr;local.reserved=false;--pending;++closed;
        }
        return true;
    }catch(...){failed=true;return true;}
}
#else
bool begin_tick(Entry&,bool(*)(std::uintptr_t),bool) noexcept {return false;}
bool begin_compose(Entry&,bool) noexcept {return false;}
bool copy_for_compose(Entry const&,api::Snapshot& output) noexcept {output={};return false;}
bool owns_compose(Entry const&) noexcept {return false;}
bool copy_for_tick(api::Snapshot&) noexcept {return false;}
bool retained_for_compose(Entry const&,api::Snapshot&,Entry const*& outer,::Scope*& lease) noexcept {outer=nullptr;lease=nullptr;return false;}
bool end(Entry const&,bool) noexcept {return false;}
#endif
void request_drain() noexcept {stopped=true;}
unsigned pending_count() noexcept {return pending.load();}
void summary(FILE* out){if(out)std::fprintf(out,"troika_driver_ticks=%llu troika_driver_c0_sources=%llu troika_driver_rejected=%llu troika_driver_closed=%llu troika_driver_pending=%u troika_driver_failed=%u troika_driver_source_only=1 ",ticks.load(),composes.load(),rejected.load(),closed.load(),pending.load(),unsigned(failed.load()));}
} // namespace troika_driver_scope_live
