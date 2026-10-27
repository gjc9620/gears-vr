#include "weapon_ammo_card_plan.h"
#include <cassert>
#include <cstdio>
#include <limits>

using namespace weapon_ammo_card;
namespace {
bool close(double a,double b){return std::abs(a-b)<1e-10;}
struct Fixture {
    gears_vr::StereoSample eyes{};
    AmmoFrame ammo{};
    WeaponFrame weapon{};
    Mount mount{};
    Options options{true,false};
    const std::int64_t now=1010000000;
    Fixture(){
        eyes.session=1;eyes.sequence=2;eyes.space_epoch=3;eyes.predicted_time=42;
        eyes.received_ns=1000000000;eyes.head_flags=eyes.view_flags=15;
        eyes.focused=eyes.visible=eyes.should_render=1;
        eyes.head.orientation.w=1;
        for(unsigned i=0;i<2;++i){eyes.eyes[i].pose.orientation.w=1;
            eyes.eyes[i].pose.position.x=i?.032:-.032;eyes.eyes[i].fov={-.8,.8,.8,-.8};}
        ammo.identity={4,5,6,7};ammo.frame=key(eyes);ammo.received_ns=eyes.received_ns;
        ammo.authenticated=true;assert(original_firearm_counts(60,13,240,false,ammo.counts));
        weapon.identity=ammo.identity;weapon.frame=ammo.frame;weapon.received_ns=ammo.received_ns;
        weapon.pose_in_local_m.position={.2,-.15,-.6};weapon.authenticated=weapon.held=true;
        mount.profile=7;mount.verified=true;mount.card_in_weapon_m.position={0,.08,0};
        mount.width_m=.15;mount.height_m=.05;
    }
    bool plan(Plan& out){return prepare(options,ammo,weapon,mount,eyes,now,out);}
};
template<class Change>void reject(Change change){
    Fixture f;Plan out{};out.identity.weapon=999;out.width_m=123;change(f);
    assert(!f.plan(out));assert(out.identity.weapon==999&&out.width_m==123);
}
}
int main(){
    Counts c{};assert(original_firearm_counts(60,0,240,false,c)&&c.in_gun==60&&c.spare==240);
    assert(original_firearm_counts(60,60,0,false,c)&&c.in_gun==0&&c.spare==0);
    assert(original_firearm_counts(60,13,0,true,c)&&c.in_gun==47&&c.infinite_spare);
    for(double size:{0.,-1.,1.5,10001.,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()}){
        const auto before=c;assert(!original_firearm_counts(size,0,0,false,c));assert(c.in_gun==before.in_gun&&c.infinite_spare==before.infinite_spare);}
    for(auto used:{-1,61})assert(!original_firearm_counts(60,used,0,false,c));
    for(auto spare:{-1,1000001})assert(!original_firearm_counts(60,0,spare,false,c));
    assert(original_throwable_counts(3,false,c)&&c.kind==AmmoKind::throwable&&c.in_gun==3&&!c.spare);
    assert(original_throwable_counts(0,true,c)&&c.infinite_spare);
    assert(!original_throwable_counts(-1,false,c)&&!original_throwable_counts(1000001,false,c));
    Fixture f;Plan p{};assert(f.plan(p));assert(p.counts.in_gun==47&&p.counts.spare==240);
    assert(close(p.pose_in_local_m.position.x,.2)&&close(p.pose_in_local_m.position.y,-.07)&&close(p.pose_in_local_m.position.z,-.6));
    assert(p.front_cosine[0]>0&&p.front_cosine[1]>0&&p.distance_m[0]!=p.distance_m[1]);
    // Looking elsewhere changes neither the gun-fixed position nor orientation.
    const auto old=p;const double angle=.4;const gears_vr::Quaternion q{0,std::sin(angle/2),0,std::cos(angle/2)};
    f.eyes.head.orientation=q;for(auto& eye:f.eyes.eyes)eye.pose.orientation=q;
    assert(f.plan(p));assert(gears_vr::stereo_same_pose(old.pose_in_local_m,p.pose_in_local_m));
    // Gun translation and rotation move the offset by the actual weapon frame.
    f.weapon.pose_in_local_m.orientation={0,0,std::sin(.1),std::cos(.1)};
    f.weapon.pose_in_local_m.position.x+=.1;assert(f.plan(p));
    assert(close(p.pose_in_local_m.position.x,.3-.08*std::sin(.2)));
    assert(close(p.pose_in_local_m.position.y,-.15+.08*std::cos(.2)));
    assert(close(p.pose_in_local_m.orientation.z,std::sin(.1)));
    reject([](auto& x){x.options={};}); // OFF is the default.
    reject([](auto& x){x.ammo.authenticated=false;});reject([](auto& x){x.weapon.authenticated=false;});
    reject([](auto& x){x.weapon.held=false;});reject([](auto& x){x.weapon.identity.weapon++;});
    reject([](auto& x){x.weapon.identity.world++;});reject([](auto& x){x.weapon.identity.pawn++;});
    reject([](auto& x){x.ammo.identity.profile=0;});reject([](auto& x){x.mount.profile++;});
    reject([](auto& x){x.weapon.frame.sequence++;});reject([](auto& x){x.weapon.frame.session++;});
    reject([](auto& x){x.weapon.frame.space_epoch++;});reject([](auto& x){x.weapon.frame.predicted_time++;});
    reject([](auto& x){x.eyes.sequence++;});reject([](auto& x){x.ammo.frame.predicted_time=0;});
    reject([](auto& x){x.ammo.received_ns=x.now+1;});reject([](auto& x){x.ammo.received_ns=x.now-100000001;});
    reject([](auto& x){x.weapon.received_ns=x.now-100000001;});reject([](auto& x){x.eyes.received_ns=x.now-100000001;});
    reject([](auto& x){x.ammo.counts.kind=AmmoKind::unknown;});reject([](auto& x){x.ammo.counts.in_gun=10001;});
    reject([](auto& x){x.ammo.counts={AmmoKind::throwable,3,2,false};});
    reject([](auto& x){x.mount.verified=false;});reject([](auto& x){x.mount.width_m=.16;});
    reject([](auto& x){x.mount.card_in_weapon_m.orientation.w=0;});
    reject([](auto& x){x.weapon.pose_in_local_m.position.z=std::numeric_limits<double>::quiet_NaN();});
    reject([](auto& x){x.weapon.pose_in_local_m.orientation={0,1,0,0};}); // Readable side faces away.
    reject([](auto& x){x.eyes.view_flags=3;});reject([](auto& x){x.eyes.should_render=0;});
    reject([](auto& x){x.eyes.source=gears_vr::StereoSource::MetaOperatorSimulated;});
    Fixture boundary;boundary.ammo.received_ns=boundary.now-100000000;assert(boundary.plan(p));
    Fixture sim;sim.options.allow_simulated=true;sim.eyes.source=gears_vr::StereoSource::MetaOperatorSimulated;
    sim.eyes.head_flags=3;assert(sim.plan(p));
    std::puts("WeaponAmmoCardPlanTest passed");
}
