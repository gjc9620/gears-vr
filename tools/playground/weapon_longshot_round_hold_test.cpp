#include "../vr-common/hand_authored_mount_profile.h"
#include "../vr-common/weapon_longshot_grasp_pose.h"
#include "../vr-common/body_slots.h"
#include "../vr-common/rig_tracking_frame.h"
#include "../vr-common/hand_authored_grasp.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
FILE* log_file{};
// Exercise the production hold implementation unchanged, injecting only the
// resource/rig/current-source boundary. No game, graphics or inventory writes.
namespace hand=gears_weapon_hand;
bool fresh=true,rig_current=true,resource_present=true,hide_ok=true;
bool v4_present=false;
bool authored_stub(gears_hand_authored_grasp::Request const*)noexcept{return true;}
namespace lancersaw_visual {template<class T>T function(char const*){return v4_present?&authored_stub:nullptr;}}
unsigned publications{},withdrawals{},rig_reads{};
gears_vr::Viewpoint published{};
gears_rig_tracking::Frame current_rig{};
namespace weaponhand {std::int64_t clock_ns(void*){return 1000000000;}}
namespace nativelancerfollow {double bound_units=100.;}
namespace nativeequipment {gears_weapon::ArmspanCalibration calibration=gears_weapon::default_body_slot_layout();}
namespace gears_equipment_rig {
enum class Status {matched,rejected};
Status match(hand::Frame const&,gears_rig_tracking::Frame const&,std::int64_t,unsigned){return rig_current?Status::matched:Status::rejected;}
}
namespace handrig {
struct Result {gears_rig_tracking::Reply reply;};
Result read(gears_rig_tracking::Frame& out,unsigned,unsigned){++rig_reads;out=current_rig;return {gears_rig_tracking::Reply::accepted};}
}
namespace gears_weapon {
gears_vr::Vector rotate(gears_vr::Quaternion q,gears_vr::Vector v){return gears_vr::transform(gears_vr::rotation_matrix(q),v);}
}
namespace weapon_part_assets {
struct Handle {unsigned slot{},generation{};};struct Resource{};Resource resource;
Resource* find(Handle){return resource_present?&resource:nullptr;}
bool set_visible(Handle,std::uint64_t,bool visible){assert(!visible);++withdrawals;return hide_ok;}
bool set_pose(hand::Frame const&,Handle,std::uint64_t,hand::Decision const&,gears_vr::Viewpoint const& pose,bool visible){assert(visible);++publications;published=pose;return true;}
}
namespace longshot_round {
namespace asset=weapon_part_assets;
asset::Handle handle{1,1};std::uint64_t generation=1;
bool ready=true,authored_pinch=false;hand::Lease held_lease{};
std::uint64_t item_serial{},last_pose_sequence{};
gears_vr::Pose item_in_grip{};
bool current(hand::Frame const&){return fresh&&resource_present;}
}
#include "weapon_longshot_round_hold.inl"
struct Fixture {
    hand::Frame f;hand::Decision d;
    Fixture(){
        longshot_round::held_lease={};longshot_round::item_serial=longshot_round::last_pose_sequence=0;
        fresh=rig_current=resource_present=hide_ok=longshot_round::ready=true;v4_present=longshot_round::authored_pinch=false;publications=withdrawals=rig_reads=0;
        f.key.lifecycle=1;f.key.frame=2;f.press_token=3;f.left_tracked=f.left_grip_known=f.left_grip_down=1;
        f.controllers.sequence=4;f.controllers.hands[0].grip.position={-.2295,-.115,.05};
        d.key=f.key;d.press_token=3;d.lease={hand::LeaseKind::ammo,0,1,3};d.status=hand::DecisionStatus::granted;d.new_grant=1;d.release_required=0;
        current_rig={};current_rig.world_pose_mask=gears_rig_tracking::left_grip_pose;
        current_rig.world_grip[0].position={100,200,300};
    }
    bool capture(){return longshot_round::capture_shoulder(f,d,3);}
};
int main(){
#ifdef _WIN32
 _set_error_mode(_OUT_TO_STDERR);_set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
#endif
    {Fixture v;v4_present=true;assert(v.capture()&&longshot_round::authored_pinch);
        gears_hand_authored_mount::Mount a{};assert(gears_hand_authored_mount::marcus115(
            gears_hand_authored_mount::marcus115_signature,gears_hand_authored_mount::Side::left,1,100,a));
        gears_vr::Pose expected{};assert(gears_weapon_longshot_round_grasp::mount(
            a.calibration.grip_to_wrist_metres,a.calibration.grip_to_wrist_rotation,100,expected));
        assert(gears_weapon_longshot_round_presentation::same_pose(expected,longshot_round::item_in_grip));
        const auto latched=longshot_round::item_in_grip;v4_present=false;
        assert(longshot_round::show(v.f,v.d)&&longshot_round::authored_pinch&&
            gears_weapon_longshot_round_presentation::same_pose(latched,longshot_round::item_in_grip));
        assert(longshot_round::hide()&&!longshot_round::authored_pinch);}
    {Fixture v;assert(v.capture());v.f.controllers.sync_ns=1000000000;
        gears_vr::Pose raw{};assert(longshot_round::physical_pose(v.f,v.d,raw));
        const gears_vr::Quaternion turn{0,0,std::sin(3.14159265358979323846/4),std::cos(3.14159265358979323846/4)};
        gears_vr::Pose root{};root.orientation=gears_vr::normalized(gears_vr::multiply(raw.orientation,turn));
        const auto tip=gears_weapon::rotate(raw.orientation,{0,15,0});
        const auto delta=gears_weapon::rotate(root.orientation,{-3,4,0});
        root.position={raw.position.x+tip.x-delta.x,raw.position.y+tip.y-delta.y,raw.position.z+tip.z-delta.z};
        const auto before_reads=rig_reads;
        assert(longshot_round::prepare_presentation(v.f,v.d,root,{},1,7)&&rig_reads==before_reads+1);
        assert(longshot_round::show(v.f,v.d)&&rig_reads==before_reads+1);
        ++v.f.key.frame;++v.f.controllers.sequence;v.f.controllers.sync_ns+=60000000;v.d.key=v.f.key;
        assert(longshot_round::prepare_presentation(v.f,v.d,root,{},1,7)&&longshot_round::show(v.f,v.d));
        longshot_round::presentation::Sample sample{};assert(longshot_round::presentation_sample(v.f,v.d,sample));
        assert(std::abs(sample.weight-.5)<1e-12&&std::abs(sample.measurement.raw_depth_m-.03)<1e-12);
        assert(gears_weapon_longshot_round_presentation::same_pose(sample.raw,raw));
        assert(std::abs(published.position.x-sample.display.position.x)<1e-10&&std::abs(published.position.y-sample.display.position.y)<1e-10);
        gears_vr::Pose grip{};assert(longshot_round::presentation_grip(v.f,v.d,grip));
        assert(gears_weapon_longshot_round_presentation::same_pose(grip,sample.grip));
        const auto calls=publications;assert(longshot_round::show(v.f,v.d)&&publications==calls);
        ++v.f.key.frame;v.d.key=v.f.key;assert(!longshot_round::show(v.f,v.d)&&!longshot_round::presentation_grip(v.f,v.d,grip));
        assert(longshot_round::item_serial==3); // Display loss does not consume/return an inventory reservation.
    }
    {Fixture v;assert(v.capture());assert(longshot_round::item_serial==3);assert(!v.capture());
        assert(longshot_round::show(v.f,v.d)&&publications==1);auto before=published;
        assert(longshot_round::show(v.f,v.d)&&publications==1);
        ++v.f.controllers.sequence;current_rig.world_grip[0].position.x+=20;
        assert(longshot_round::show(v.f,v.d)&&publications==2);
        assert(std::abs(published.position.x-before.position.x-20)<1e-6);
        assert(std::abs(published.position.y-before.position.y)<1e-6);
        assert(longshot_round::hide()&&withdrawals==1&&!longshot_round::item_serial);
        assert(!longshot_round::show(v.f,v.d));}
    for(unsigned reason=0;reason<9;++reason){Fixture v;
        switch(reason){case 0:v.d.new_grant=0;break;case 1:v.d.release_required=1;break;
        case 2:v.f.left_grip_down=0;break;case 3:v.f.left_tracked=0;break;
        case 4:v.d.lease.kind=hand::LeaseKind::support;break;case 5:v.d.lease.lifecycle=2;break;
        case 6:v.f.controllers.hands[0].grip.position={0,0,0};break;
        case 7:v.d.press_token=9;break;case 8:fresh=false;break;}
        assert(!v.capture()&&!longshot_round::item_serial&&!publications);}
    for(unsigned reason=0;reason<7;++reason){Fixture v;assert(v.capture());
        switch(reason){case 0:rig_current=false;break;case 1:current_rig.world_pose_mask=0;break;
        case 2:fresh=false;break;case 3:v.d.lease.id=8;break;case 4:v.d.key.frame=99;break;
        case 5:v.f.left_grip_known=0;break;case 6:resource_present=false;break;}
        assert(!longshot_round::show(v.f,v.d)&&!publications);}
    {Fixture v;assert(v.capture());hide_ok=false;assert(!longshot_round::hide()&&longshot_round::item_serial==3);
        hide_ok=true;assert(longshot_round::hide()&&!longshot_round::item_serial);}
    {Fixture v;assert(v.capture());assert(longshot_round::show(v.f,v.d));
        ++v.f.controllers.sequence;rig_current=false;
        assert(!longshot_round::show(v.f,v.d)&&withdrawals==1&&longshot_round::item_serial==3);
        rig_current=true;assert(longshot_round::show(v.f,v.d)&&publications==2);}
    std::puts("Longshot cartridge hold: original shoulder, live lease/rig, pose follow, duplicate suppression and withdrawal checks passed");
}
