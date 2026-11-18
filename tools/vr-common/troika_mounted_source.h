#pragma once
#include "rig_tracking_frame.h"

// Distinct mounted data ABI. No normal hand Frame, lease, C0 plan, game call,
// grip grant or action token can be obtained from this read-only receipt.
namespace gears_troika_source {
namespace rig=gears_rig_tracking;namespace hand=gears_weapon_hand;
constexpr std::uint32_t version=2;
constexpr std::uint32_t mounted_look_suppressed=1;
struct Request {
    std::uint32_t size{sizeof(Request)},abi_version{version};
    rig::NativeKey native{};
    hand::ObjectIdentity world{},controller{},turret{},driver{};
    std::uint64_t minimum_publication_sequence{},maximum_source_age_ns{100000000};
};
struct Frame {
    std::uint32_t size{sizeof(Frame)},abi_version{version};
    rig::NativeKey native{};
    hand::ObjectIdentity world{},controller{},turret{},driver{},turret_mesh{},driver_mesh{},weapon{};
    std::uint64_t publication_sequence{},source_ticket{},scene_invocation{},world_generation{};
    std::int64_t scene_qpc{},qpc_frequency{},published_ns{};
    std::uint64_t reference_generation{},native_reference_revision{};
    rig::NativeReference native_reference{};rig::ProducerContinuity producer{};
    std::uint32_t publisher_pid{},control_policy{};std::uint64_t publisher_start{};
    gears_vr::StereoSample stereo{};gears_vr::ControllerInputFrame controllers{};
    gears_vr::Pose reference{};
};
inline bool request_valid(Request const& r) noexcept {
    return r.size==sizeof(r)&&r.abi_version==version&&r.native.game_pid&&r.native.game_thread&&
        r.native.game_start&&r.native.native_started_qpc&&r.native.registration_cookie&&
        hand::identity_valid(r.world)&&hand::identity_valid(r.controller)&&hand::identity_valid(r.turret)&&hand::identity_valid(r.driver)&&
        r.turret.address!=r.driver.address&&r.maximum_source_age_ns&&r.maximum_source_age_ns<=rig::maximum_age_ns;
}
inline bool frame_valid(Frame const& f,std::int64_t now,std::uint64_t age,bool allow_sim=false) noexcept {
    if(f.size!=sizeof(f)||f.abi_version!=version||f.control_policy!=mounted_look_suppressed||!f.native.game_pid||!f.native.game_thread||
       !f.native.game_start||!f.native.native_started_qpc||!f.native.registration_cookie||
       !f.publication_sequence||!f.source_ticket||!f.scene_invocation||!f.world_generation||!f.reference_generation||
       !f.native_reference_revision||(f.native_reference_revision&1)||!f.native_reference.epoch||
       f.native_reference.flags!=rig::reference_stable||f.native_reference.reserved||
       !f.producer.stream||!f.producer.tracking[0]||!f.producer.tracking[1]||!f.publisher_pid||!f.publisher_start||
       !age||age>rig::maximum_age_ns||now<=0||f.published_ns<=0||f.published_ns>now||
       std::uint64_t(now-f.published_ns)>age||f.scene_qpc<=0||f.qpc_frequency<=0)return false;
    for(auto const* id:{&f.world,&f.controller,&f.turret,&f.driver,&f.turret_mesh,&f.driver_mesh,&f.weapon})
        if(!hand::identity_valid(*id))return false;
    if(f.turret.address==f.driver.address||f.turret_mesh.address==f.driver_mesh.address||
       !gears_vr::stereo_pose_valid(f.reference)||!gears_vr::stereo_render_active(f.stereo)||
       f.stereo.focused!=1||!gears_vr::stereo_tracking_valid(f.stereo,allow_sim)||!gears_vr::stereo_geometry_valid(f.stereo)||
       !gears_vr::controller_frame_matches(f.controllers,f.stereo,now,allow_sim))return false;
    for(auto stamp:{f.stereo.received_ns,f.controllers.received_ns,f.controllers.sync_ns})
        if(stamp<=0||stamp>f.published_ns||now<stamp||std::uint64_t(now-stamp)>age)return false;
    const auto ticks=std::uint64_t(f.scene_qpc),frequency=std::uint64_t(f.qpc_frequency);
    const auto seconds=ticks/frequency,remainder=ticks%frequency;
    if(seconds>std::uint64_t(INT64_MAX)/1000000000||remainder>std::uint64_t(INT64_MAX)/1000000000)return false;
    const auto ns=seconds*1000000000+remainder*1000000000/frequency;
    return ns<=std::uint64_t(now)&&std::uint64_t(now)-ns<=age;
}
inline bool matches(Request const& r,Frame const& f,std::int64_t now,bool allow_sim=false) noexcept {
    return request_valid(r)&&frame_valid(f,now,r.maximum_source_age_ns,allow_sim)&&rig::same(r.native,f.native)&&
        hand::same(r.world,f.world)&&hand::same(r.controller,f.controller)&&hand::same(r.turret,f.turret)&&hand::same(r.driver,f.driver)&&
        f.publication_sequence>=r.minimum_publication_sequence;
}
// Per-consumer publication progress, independent of the shared geometric
// mapper's first-reader flag. This grants no engine/input/write permission.
class ConsumerCursor {
    rig::NativeKey native_{};std::uint64_t publication_{};bool have_{};
public:
    bool take(Frame const& source,std::int64_t now,bool allow_sim=false) noexcept {
        if(!frame_valid(source,now,rig::maximum_age_ns,allow_sim))return false;
        if(have_&&rig::same(native_,source.native)&&source.publication_sequence<=publication_)return false;
        native_=source.native;publication_=source.publication_sequence;have_=true;return true;
    }
};
using ReadFn=rig::Reply(__cdecl*)(Request const*,Frame*) noexcept;
} // namespace gears_troika_source
