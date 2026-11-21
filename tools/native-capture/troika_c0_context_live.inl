#include "troika_c0_hands.h"
#include "troika_c0_readback.h"
// Existing full_body_c0 State owns these objects. No separate Kernel/Backend,
// hook, ordinary Pawn binding, input frame or engine-action permission.
namespace troika_c0_context_live {
namespace meta=weapon_hand_scene_binding;namespace api=gears_troika_driver;
using Entry=gears_continuous_hand_entry::Entry;namespace tx=gears_full_body_c0_transaction;
#if defined(GEARS_TROIKA_MOUNTED_SOURCE) && GEARS_TROIKA_MOUNTED_SOURCE
bool resolve(gears_weapon_hand::ObjectIdentity const& x,gears_anchor::Identity& actual) {
    return meta::identity(x.address,actual,true)&&gears_weapon_hand::same(rig_tracking_live::object(actual),x);
}
gears_cover_pose_phase::Identity compact(gears_anchor::Identity const& x) noexcept {
    return {x.address,x.cls,x.outer,static_cast<std::int32_t>(x.index),static_cast<std::int32_t>(x.name_index),x.number};
}
struct Storage {
    gears_troika_geometry::Source source{};gears_anchor::Identity asset{};
    gears_anchor::Identity world{},driver{},mesh{},turret{},controller{};
    meta::Proof parts{};gears_cover_pose_phase::OwnerKey owner{};
    bool current_locked() const {
        auto const& f=source.frame;
        if(!meta::metadata_current()||f.world_generation!=committed_world_generation.load()||
           f.native.native_started_qpc!=uint64_t(session_started_qpc)||!meta::current(parts))return false;
        const gears_anchor::Identity* objects[]={&world,&driver,&mesh};
        for(auto x:objects){gears_anchor::Identity live{};if(!meta::identity(x->address,live,true)||!meta::same(live,*x))return false;}
        gears_anchor::Identity live_asset{};uintptr_t linked_mesh{};gears_cover_pose_phase::Metadata metadata{};
        if(!meta::identity(asset.address,live_asset,true)||!meta::same(live_asset,asset)||
           !meta::field(linked_mesh,f.driver.address,parts.fields[1].offset)||linked_mesh!=f.driver_mesh.address)return false;
        gears_cover_pose_phase::Request request{};request.owner=owner;
        auto read=[](void* to,void const* from,size_t bytes){return safe_copy(to,from,bytes);};
        return gears_cover_pose_phase::metadata(read,request,metadata)&&metadata.asset==asset.address&&meta::current(parts);
    }
    bool current() const noexcept {try{std::unique_lock<std::mutex> lock(meta::mutex,std::try_to_lock);
        return lock.owns_lock()&&current_locked();}catch(...){return false;}}
    bool capture(api::Snapshot const& input) noexcept {try{
        std::unique_lock<std::mutex> lock(meta::mutex,std::try_to_lock);if(!lock.owns_lock()||!meta::metadata_current())return false;
        auto const& f=input.source.frame;
        if(f.driver.registry_index>INT_MAX||f.driver_mesh.registry_index>INT_MAX||f.world.registry_index>INT_MAX||
           f.driver.name_index>INT_MAX||f.driver_mesh.name_index>INT_MAX||f.world.name_index>INT_MAX||
           !resolve(f.world,world)||!resolve(f.driver,driver)||!resolve(f.driver_mesh,mesh)||!resolve(f.turret,turret)||!resolve(f.controller,controller)||
           !troika_scene_binding::parts(turret,driver)||!meta::current(troika_scene_binding::parts_proof))return false;
        source=input.source;parts=troika_scene_binding::parts_proof;
        owner={compact(world),compact(driver),compact(mesh),f.world_generation,
            f.native.native_started_qpc,f.scene_invocation,f.scene_qpc};
        gears_cover_pose_phase::Request request{};request.owner=owner;gears_cover_pose_phase::Metadata metadata{};
        auto read=[](void* to,void const* from,size_t bytes){return safe_copy(to,from,bytes);};
        if(!gears_cover_pose_phase::metadata(read,request,metadata)||!meta::identity(metadata.asset,asset,true))return false;
        meta::ClassPath path{};return meta::derives(asset.cls,L"SkeletalMesh",0,path)&&current_locked();
    }catch(...){return false;}}
};
bool same_avatar(Storage const& s,meta::Binding const& ordinary) noexcept {
    return ordinary.context_valid&&ordinary.weapon_known&&s.source.frame.world_generation==ordinary.world_generation&&
        meta::same(s.world,ordinary.world)&&meta::same(s.controller,ordinary.controller)&&
        meta::same(s.driver,ordinary.pawn)&&meta::same(s.mesh,ordinary.mesh);
}
struct Context {
    bool active{};api::Snapshot source{};Entry const* entry{};Entry copy{};
    Entry const* outer{};Entry outer_copy{};::Scope* lease{};Storage storage{};std::int64_t began_ns{};
    mutable tx::Verdict verdict{tx::Verdict::denied};
    tx::Verdict rejection()const noexcept{return verdict;}
    bool capture(Entry const& e) noexcept {
        active=false;entry=nullptr;outer=nullptr;lease=nullptr;began_ns=rig_tracking_live::clock_ns();
        if(!troika_driver_scope_live::copy_for_compose(e,source)||!storage.capture(source))return false;
        api::Snapshot retained{};
        if(!troika_driver_scope_live::retained_for_compose(e,retained,outer,lease)||
           std::memcmp(&retained,&source,sizeof source)||!outer||!lease)return false;
        entry=&e;copy=e;outer_copy=*outer;active=true;return current(gears_full_body_c0_transaction::Stage::prepare);
    }
    bool current(tx::Stage stage) const noexcept {
        verdict=tx::Verdict::denied;
        if(!active||!entry||GetCurrentThreadId()!=copy.thread||std::memcmp(entry,&copy,sizeof copy)||
           !outer||std::memcmp(outer,&outer_copy,sizeof outer_copy)||!lease||!lease->world()||!storage.current())return false;
        const bool closing=stage==tx::Stage::rollback||stage==tx::Stage::consumer_entry||stage==tx::Stage::scope_end;
        api::Snapshot actual{};Entry const* held_outer{};::Scope* held_lease{};
        if(closing){
            if(!troika_driver_scope_live::retained_for_compose(*entry,actual,held_outer,held_lease)||held_outer!=outer||held_lease!=lease)return false;
        }else{
            const auto now=rig_tracking_live::clock_ns();
            // Prove the entire still-current 100ms source BEFORE classifying
            // only the stricter existing C0 50ms deadline as recoverable.
            if(now<began_ns||!troika_driver_scope_live::copy_for_compose(*entry,actual)||
               std::memcmp(&actual,&source,sizeof source)||!storage.current())return false;
            if(now-began_ns>50000000||!gears_troika_source::frame_valid(source.source.frame,now,50000000,simulator_operator)){
                verdict=tx::Verdict::source_age_expired;return false;
            }
        }
        if(std::memcmp(&actual,&source,sizeof source)||!storage.current())return false;
        verdict=tx::Verdict::verified;return true;
    }
};
#else
struct Storage {gears_anchor::Identity asset{};gears_cover_pose_phase::OwnerKey owner{};bool current()const noexcept{return false;}};
struct Context {bool active{};Storage storage{};api::Snapshot source{};Entry const* entry{};Entry copy{};Entry const* outer{};Entry outer_copy{};::Scope* lease{};std::int64_t began_ns{};
    bool capture(Entry const&)noexcept{return false;}bool current(tx::Stage)const noexcept{return false;}};
#endif
} // namespace troika_c0_context_live
