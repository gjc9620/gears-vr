#include "../vr-common/hand_part_grasp.h"
#include "../vr-common/weapon_two_hand_hold.h"
#include <atomic>
#include <cassert>
#include <cstdio>
#include <cstring>
using DWORD=unsigned long;DWORD test_thread=7;
DWORD GetCurrentThreadId(){return test_thread;}
long long test_ns=1000000000;
namespace gears_vr {long long monotonic_ns(){return test_ns;}}
namespace hand=gears_weapon_hand;namespace api=gears_hand_object_grasp;
hand::Frame expected{};hand::LeaseKind left_owner=hand::LeaseKind::magazine;
bool enabled=true,loading_owner=true;
bool safe_copy(void* to,void const* from,std::size_t bytes){if(!from)return false;std::memcpy(to,from,bytes);return true;}
namespace lancer_handle_morph_live {bool grasp_owner_current(hand::Frame const&) noexcept {return loading_owner;}}
namespace lancer_original_handle_live {
std::atomic<bool> retained{};
struct {struct {bool restoring(){return false;}}pose;hand::ObjectIdentity weapon{},world{},pawn{};hand::FrameKey key{};}bound;
}
namespace gears_native_rig_tracking {
struct CoreSnapshot {
    struct {std::uint64_t game_start{},native_started_qpc{};}native;
    std::uint64_t world_generation{},reference_generation{};
    hand::ObjectIdentity world{},pawn{},mesh{},weapon{};gears_vr::StereoSample stereo{};
};
}
namespace weapon_hand_runtime {
struct Broker {
    bool current(hand::Frame const& f){return enabled&&test_thread==7&&
        test_ns>=f.controllers.sync_ns&&test_ns-f.controllers.sync_ns<=100000000&&
        !std::memcmp(&f,&expected,sizeof f);}
    bool admit_part_visibility(hand::Frame const& f){return current(f)&&f.left_grip_down&&
        (left_owner==hand::LeaseKind::magazine||left_owner==hand::LeaseKind::ammo);}
    bool admit_socket_visibility(hand::Frame const& f){return current(f)&&left_owner==hand::LeaseKind::none&&
        f.controllers.hands[0].squeeze_active==1&&!hand::control_held(f.controllers.hands[0],gears_vr::control_squeeze);}
    bool admit_support_visibility(hand::Frame const& f){return current(f)&&left_owner==hand::LeaseKind::support;}
}broker;
Broker* current(){return &broker;}
}
#define GEARS_NATIVE_ATOM_OVERWRITE_RECEIPT 1
#include "lancer_grasp_live.inl"
gears_native_rig_tracking::CoreSnapshot core(){
    gears_native_rig_tracking::CoreSnapshot c{};c.native={expected.key.game_start,expected.key.native_started_qpc};
    c.world_generation=expected.key.world_generation;c.reference_generation=expected.reference_generation;
    c.world=expected.world;c.pawn=expected.pawn;c.mesh=expected.mesh;c.weapon=expected.weapon;c.stereo=expected.stereo;return c;
}
api::Target selected(){api::Target out{};lancer_grasp_live::target(core(),expected.controllers,out);return out;}
int main(){
    expected.key={1,2,3,4,5,6,100,7};expected.controllers.sync_ns=test_ns;
    expected.controllers.input_epoch=2;expected.controllers.profile_generation=3;expected.controllers.sequence=4;
    expected.stereo.session=5;expected.stereo.space_epoch=6;expected.reference_generation=7;
    expected.world.address=16;expected.pawn.address=32;expected.mesh.address=48;expected.weapon.address=64;
    expected.left_grip_known=expected.left_tracked=expected.left_grip_down=1;expected.press_token=1;
    auto& left=expected.controllers.hands[0];left.squeeze_active=left.grip_active=1;left.grip_flags=15;left.squeeze=1;
    left.pressed[gears_vr::control_squeeze]=1;
    api::Request legacy{};legacy.source=expected;legacy.kind=api::Kind::magazine;
    assert(lancer_grasp_live::publish(&legacy));assert(selected().preset==api::Preset::automatic);
    gears_hand_part_grasp::Request extended{};extended.pose=legacy;extended.preset=api::Preset::precision;
    const auto physical=expected.controllers;
    assert(lancer_grasp_live::publish_part(&extended));auto chosen=selected();
    assert(chosen.kind==api::Kind::magazine&&chosen.preset==api::Preset::precision&&!chosen.passive_socket);
    assert(!std::memcmp(&physical,&expected.controllers,sizeof physical));
    assert(lancer_grasp_live::publish(&legacy)&&selected().preset==api::Preset::automatic); // V2 never inherits a V3 preset.
    for(auto invalid:{api::Preset::automatic,static_cast<api::Preset>(99)}){extended.preset=invalid;assert(!lancer_grasp_live::publish_part(&extended));}
    extended.preset=api::Preset::cylinder;extended.reserved=1;assert(!lancer_grasp_live::publish_part(&extended));extended.reserved=0;
    extended.version=2;assert(!lancer_grasp_live::publish_part(&extended));extended.version=3;
    extended.pose.version=3;assert(!lancer_grasp_live::publish_part(&extended));extended.pose.version=2;
    extended.pose.kind=api::Kind::support;assert(!lancer_grasp_live::publish_part(&extended));extended.pose.kind=api::Kind::magazine;
    extended.preset=api::Preset::support_surface;assert(!lancer_grasp_live::publish_part(&extended));
    extended.pose.kind=api::Kind::support;extended.pose.grip.position={0,.4,0};left_owner=hand::LeaseKind::support;
    assert(lancer_grasp_live::publish_part(&extended));chosen=selected();
    assert(chosen.kind==api::Kind::support&&chosen.preset==api::Preset::support_surface);
    left_owner=hand::LeaseKind::none;assert(!lancer_grasp_live::publish_part(&extended));
    left.squeeze=0;expected.left_grip_down=0;extended.pose.source=expected;extended.pose.proximity=1;
    assert(lancer_grasp_live::publish_part(&extended));assert(selected().passive_socket);
    left.squeeze=1;expected.left_grip_down=1;extended.pose.source=expected;extended.pose.proximity=0;
    left_owner=hand::LeaseKind::magazine;extended.pose.kind=api::Kind::magazine;extended.pose.grip={};extended.preset=api::Preset::cylinder;
    enabled=false;assert(!lancer_grasp_live::publish_part(&extended));enabled=true;
    ++extended.pose.source.key.frame;assert(!lancer_grasp_live::publish_part(&extended));extended.pose.source=expected;
    left_owner=hand::LeaseKind::none;assert(!lancer_grasp_live::publish_part(&extended));
    left.squeeze=0;left.released[gears_vr::control_squeeze]=1;expected.left_grip_down=0;
    extended.pose.source=expected;extended.pose.proximity=1;
    legacy=extended.pose;assert(!lancer_grasp_live::publish(&legacy)); // Old passive magazine behavior remains rejected.
    assert(lancer_grasp_live::publish_part(&extended));chosen=selected();
    assert(chosen.kind==api::Kind::magazine&&chosen.preset==api::Preset::cylinder&&chosen.passive_socket);
    left.squeeze=1;assert(selected().kind==api::Kind::none);left.squeeze=0;
    test_ns+=100000001;assert(selected().kind==api::Kind::none);test_ns=1000000000;
    auto wrong=core();++wrong.reference_generation;api::Target out{};
    lancer_grasp_live::target(wrong,expected.controllers,out);assert(out.kind==api::Kind::none);
    test_thread=8;assert(selected().kind==api::Kind::none);test_thread=7;
    expected.left_grip_down=1;expected.interaction_control=unsigned(gears_vr::LeftInteractionControl::pinch);
    left.trigger_active=1;left.trigger=1;left.pressed[gears_vr::control_trigger]=1;
    left_owner=hand::LeaseKind::ammo;extended.pose.source=expected;extended.pose.proximity=0;extended.preset=api::Preset::precision;
    assert(lancer_grasp_live::publish_part(&extended)&&selected().preset==api::Preset::precision);
    left.trigger=0;assert(selected().kind==api::Kind::none);
    std::puts("Production Native part grasp: V2 default/neutral rejection, V3 precision/cylinder, exact-source/lease/socket/age/reference/thread/control loss and no raw input mutation passed; Broker boundary injected, no game/skin proof");
}
