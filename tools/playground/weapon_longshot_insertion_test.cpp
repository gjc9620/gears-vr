#include "../vr-common/weapon_hand_participant.h"
#include "../vr-common/weapon_reload_transaction.h"
#include "../vr-common/weapon_longshot_round_presentation.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
// The production insertion function below runs unchanged. Only current
// original-bone and unsnapped cartridge-pose reads are injected.
namespace hand=gears_weapon_hand;using P=gears_vr::Pose;using V=gears_vr::Vector;
P actual_item{},actual_root{};bool pose_available=true;
namespace nativelancerfollow {double bound_units=100.;}
namespace bolt {
V sub(V a,V b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
V mul(V a,double n){return {a.x*n,a.y*n,a.z*n};}
V point(P const& p,V v){auto d=gears_vr::transform(gears_vr::rotation_matrix(p.orientation),v);return {p.position.x+d.x,p.position.y+d.y,p.position.z+d.z};}
V local(P const& p,V v,double scale){auto d=gears_vr::transform(gears_vr::transpose(gears_vr::rotation_matrix(p.orientation)),sub(v,p.position));return {d.x/scale,d.y/scale,d.z/scale};}
bool bones(hand::Frame const&,P& root,P& part,double& scale){root=part=actual_root;scale=1;return pose_available;}
}
namespace original_round {
namespace presentation=gears_weapon_longshot_round_presentation;
presentation::State displayed;
hand::Lease lease{hand::LeaseKind::ammo,0,1,3};
bool presentation_sample(hand::Frame const& f,hand::Decision const&,presentation::Sample& out){
 if(!pose_available||!displayed.matches(f,lease,3,1,7))return false;out=displayed.sample();return true;}
}
namespace gears_weapon {V rotate(gears_vr::Quaternion q,V v){return gears_vr::transform(gears_vr::rotation_matrix(q),v);}}
namespace snub_visual {double length(V v){return std::hypot(std::hypot(v.x,v.y),v.z);}}
namespace longshot_fixture {
namespace round=original_round;
std::uint64_t feed_sequence{};std::int64_t last_feed_ns{};hand::Frame feed_source{};V last_tip{},port_local{};bool approached{};
#include "weapon_longshot_insertion.inl"
}
struct Fixture {
    hand::Frame f;hand::Decision d;
    Fixture(){longshot_fixture::feed_sequence=0;longshot_fixture::last_feed_ns=0;longshot_fixture::approached=false;
        longshot_fixture::port_local={};actual_root={};pose_available=true;f.controllers.sync_ns=1000000000;
        original_round::displayed.reset();f.key.frame=2;f.key.lifecycle=1;f.press_token=3;f.left_tracked=f.left_grip_known=f.left_grip_down=1;}
    bool at(double depth,double lateral=0,double angle=0,std::int64_t gap=20000000){
        ++f.key.frame;++f.controllers.sequence;f.controllers.sync_ns+=gap;
        // Asset +Y is cartridge axis. Rotate it into root +X, then yaw away.
        const auto a=-3.14159265358979323846/2+angle;
        actual_item.orientation={0,0,std::sin(a/2),std::cos(a/2)};
        const auto axis=gears_weapon::rotate(actual_item.orientation,{0,15,0});
        actual_item.position={-depth*100-axis.x,lateral*100-axis.y,-axis.z};
        if(pose_available)assert(original_round::displayed.prepare(f,original_round::lease,3,1,7,actual_item,{},actual_root,{},1,100));
        return longshot_fixture::inserted(f,d);
    }
};
int main(){
#ifdef _WIN32
    _set_error_mode(_OUT_TO_STDERR);_set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
#endif
    {Fixture f;assert(!f.at(.08));assert(!f.at(.06));assert(!f.at(.03));assert(f.at(.005));}
    {Fixture f;assert(!f.at(.005));assert(!f.at(.005));} // Spawning at seat is not a stroke.
    {Fixture f;for(unsigned i=0;i<20;++i)assert(!f.at(.005,.04,.2));
        assert(original_round::displayed.sample().inside&&original_round::displayed.sample().weight==1);}
    {Fixture f;assert(!f.at(.08));assert(!f.at(.06));assert(f.at(.005,.016));} // Actual axial stroke plus lateral assistance.
    {Fixture f;assert(!f.at(.08));assert(!f.at(.06));assert(f.at(.005,0,.20));} // Actual axial stroke plus angular assistance.
    {Fixture f;assert(!f.at(.08));assert(!f.at(.06));assert(!f.at(.005,0,0,100000001));assert(!f.at(.005));}
    {Fixture f;assert(!f.at(.08));assert(!f.at(.06));assert(!f.at(.005,0,0,1000000));assert(!f.at(.005));}
    {Fixture f;assert(!f.at(.08));assert(!f.at(.06));assert(!f.at(.09));assert(!f.at(.005));}
    {Fixture f;assert(!f.at(.08));assert(!f.at(.06));assert(!f.at(.03,.061));assert(!f.at(.005));}
    {Fixture f;assert(!f.at(.08));assert(!f.at(.06));assert(!f.at(.03,0,.53));assert(!f.at(.005));}
    {Fixture f;assert(!f.at(.08));assert(!f.at(.06));pose_available=false;assert(!f.at(.005));}
    {Fixture f;assert(!f.at(.08));assert(!f.at(.06));pose_available=false;assert(!f.at(.005));
        pose_available=true;assert(!f.at(.005));assert(!f.at(.005));assert(!f.at(.03));assert(f.at(.005));}
    for(unsigned change=0;change<3;++change){Fixture f;assert(!f.at(.08));assert(!f.at(.06));
        if(change==0)++f.f.reference_generation;
        if(change==1)++f.f.controllers.profile_generation;
        if(change==2){++f.f.controllers.input_epoch;f.f.controllers.sequence=0;}
        assert(!f.at(.005));assert(!f.at(.005));assert(!f.at(.03));assert(f.at(.005));}
    {Fixture f;assert(!f.at(.08));assert(!f.at(.06));const auto sequence=f.f.controllers.sequence;
        assert(!f.at(.005,0,0,1000000));f.f.controllers.sequence=sequence;assert(!longshot_fixture::inserted(f.f,f.d));}
    std::puts("Longshot production insertion: approach-before-seat, physical tip/axis, capture/seat sizes, speed and continuity checks passed");
}
