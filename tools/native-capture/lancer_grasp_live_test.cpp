// Owned-memory adapter test: real production descriptor/target consumer.
// Broker and engine-owner authorities are explicit seams, never game/HMD proof.
#define GEARS_NATIVE_ATOM_OVERWRITE_RECEIPT 1
#include "../vr-common/hand_authored_grasp.h"
#include "../vr-common/hand_part_grasp.h"
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
using DWORD=unsigned long;
DWORD test_thread=7;DWORD GetCurrentThreadId(){return test_thread;}
long long test_ns=1000000000;
namespace gears_vr {long long monotonic_ns(){return test_ns;}}
namespace hand=gears_weapon_hand;
hand::Frame test_frame{};bool enabled=true,binding_owner=true,evaluated_owner=true;
hand::LeaseKind left_owner=hand::LeaseKind::none;
bool safe_copy(void* to,void const* from,std::size_t n){if(!from)return false;std::memcpy(to,from,n);return true;}
namespace gears_native_rig_tracking {
struct CoreSnapshot {struct {std::uint64_t game_start{},native_started_qpc{};}native;
    std::uint64_t world_generation{},reference_generation{};hand::ObjectIdentity world{},pawn{},mesh{},weapon{};
    gears_vr::StereoSample stereo{};};
}
namespace weapon_hand_runtime {
struct Broker {
    bool current(hand::Frame const& f){return enabled&&test_thread==7&&test_ns>=f.controllers.sync_ns&&
        test_ns-f.controllers.sync_ns<=100000000&&!std::memcmp(&f,&test_frame,sizeof f);}
    bool admit_socket_visibility(hand::Frame const& f){return current(f)&&left_owner==hand::LeaseKind::none&&
        !hand::control_held(f.controllers.hands[0],gears_vr::control_squeeze);}
    bool copy_socket_visibility_frame(hand::Frame& f){if(!admit_socket_visibility(test_frame))return false;f=test_frame;return true;}
    bool admit_support_visibility(hand::Frame const& f){return current(f)&&left_owner==hand::LeaseKind::support;}
    bool admit_part_visibility(hand::Frame const& f){return current(f)&&left_owner==hand::LeaseKind::magazine;}
}broker;
Broker* current(){return &broker;}
}
namespace lancer_original_handle_live {
std::atomic<unsigned> retained{1};
struct Pose {bool restoring() const{return false;}};
struct Bound {Pose pose{};hand::FrameKey key{};hand::ObjectIdentity world{},pawn{},weapon{};}bound;
}
#include "lancer_grasp_live.inl"
namespace lancer_handle_morph_live {bool grasp_owner_current(hand::Frame const&) noexcept{return true;}}
namespace lancer_grasp_live {
bool socket_owner_current(socket::Descriptor const&,hand::Frame const&) noexcept{return evaluated_owner;}
bool starter_socket_binding_current(socket::Descriptor const&,hand::Frame const&) noexcept{return binding_owner;}
bool starter_socket_owner_current(socket::Descriptor const&,hand::Frame const&) noexcept{return binding_owner&&evaluated_owner;}
}
unsigned checks{},failures{};
#define C(x) do{++checks;if(!(x)){++failures;std::printf("FAIL %d: %s\n",__LINE__,#x);}}while(0)
hand::ObjectIdentity object(std::uint64_t at){return {at,0x8000,0x9000,unsigned(at/8),7,0,1};}
gears_native_rig_tracking::CoreSnapshot core(){
    gears_native_rig_tracking::CoreSnapshot c{};c.native={test_frame.key.game_start,test_frame.key.native_started_qpc};
    c.world_generation=test_frame.key.world_generation;c.reference_generation=test_frame.reference_generation;
    c.world=test_frame.world;c.pawn=test_frame.pawn;c.mesh=test_frame.mesh;c.weapon=test_frame.weapon;c.stereo=test_frame.stereo;return c;
}
int main(){
    namespace live=lancer_grasp_live;namespace socket=gears_lancer_support_socket;namespace pose=gears_hand_object_grasp;
    test_frame.key={1,2,3,4,5,6,100,7};test_frame.now_ns=test_ns;test_frame.reference_generation=7;
    test_frame.world=object(0x1000);test_frame.controller=object(0x2000);test_frame.pawn=object(0x3000);
    test_frame.mesh=object(0x4000);test_frame.weapon=object(0x5000);
    test_frame.stereo.session=5;test_frame.stereo.space_epoch=6;
    auto& input=test_frame.controllers;input.sync_ns=input.received_ns=test_ns;input.sequence=4;
    input.session=5;input.space_epoch=6;input.input_epoch=2;input.profile_generation=3;
    for(auto& h:input.hands){h.grip_active=h.squeeze_active=1;h.grip_flags=15;}
    socket::Descriptor d{};d.binding=test_frame;d.component=object(0x6000);d.asset=object(0x7000);d.hold_generation=9;
    d.geometry=gears_hand_authored_mount::marcus115_signature;d.model_scale=1;d.units_per_metre=100;d.in_main_grip.position={.3,0,0};
    C(socket::structural(d));C(live::publish_starter_socket(&d));
    const auto registered=d;
    pose::Target out{};
    auto select=[&](){live::target(core(),test_frame.controllers,out);};
    select();C(out.kind==pose::Kind::starter&&out.preset==pose::Preset::authored&&out.passive_relative_socket&&out.has_authored_pose);
    for(unsigned n=0;n<15;++n)C(!std::memcmp(&out.authored_pose[n],&gears_lancer_part_hand::starter[n],sizeof(gears_vr::Quaternion)));
    // Registration ages, while a genuinely NEW Native frame/input is supplied.
    // Geometry survives; neither an old world pose nor its clock is renewed.
    test_ns+=1000000000;test_frame.now_ns=test_ns;test_frame.key.frame++;test_frame.controllers.sequence++;
    test_frame.controllers.sync_ns=test_frame.controllers.received_ns=test_ns;
    select();C(out.kind==pose::Kind::starter);C(!std::memcmp(&registered,&live::socket_descriptor,sizeof d));
    const auto baseline=test_frame;
    auto reset=[&](){test_frame=baseline;test_thread=7;enabled=binding_owner=evaluated_owner=true;left_owner=hand::LeaseKind::none;};
    for(unsigned variant=0;variant<16;++variant){
        reset();auto c=core();auto current=test_frame.controllers;
        switch(variant){
        case 0:enabled=false;break;
        case 1:binding_owner=false;break;
        case 2:evaluated_owner=false;break;
        case 3:test_thread=8;break;
        case 4:test_frame.key.lifecycle++;break;
        case 5:test_frame.reference_generation++;break;
        case 6:current.sync_ns=test_ns-100000001;break;
        case 7:current.sync_ns=test_ns+1;break;
        case 8:current.input_epoch++;break;
        case 9:current.profile_generation++;break;
        case 10:current.session++;break;
        case 11:current.space_epoch++;break;
        case 12:current.source=gears_vr::StereoSource::MetaOperatorSimulated;break;
        case 13:c.world_generation++;break;
        case 14:current.hands[0].squeeze=1;break;
        default:left_owner=hand::LeaseKind::magazine;break;
        }
        live::target(c,current,out);C(out.kind==pose::Kind::none);
    }
    reset();d.binding=test_frame;evaluated_owner=false;C(live::publish_starter_socket(&d));select();C(out.kind==pose::Kind::none);
    evaluated_owner=true;select();C(out.kind==pose::Kind::starter);
    binding_owner=false;C(!live::publish_starter_socket(&d));reset();d.units_per_metre=0;C(!live::publish_starter_socket(&d));
    std::printf("Lancer socket adapter: %u checks %u failures; owned memory, mocked broker/engine-owner seams; not runtime or headset acceptance\n",checks,failures);
    return failures?1:0;
}
