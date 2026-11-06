#include "weapon_lancer_insertion_assist.h"
#include "weapon_lancer_reload_policy.h"
#include <cstdio>
#include <cstdlib>

namespace {
unsigned checks{};
void check(bool value,char const* name){++checks;if(!value){std::fprintf(stderr,"FAIL %s\n",name);std::exit(1);}}
gears_weapon::LancerInsertionSample pose(double depth,double side=5.5,double twist=30.,unsigned seq=1){
    using namespace gears_weapon;
    LancerInsertionSample s{};s.weapon=1;s.component=2;s.world=3;s.session=4;s.space_epoch=5;
    s.input_epoch=6;s.world_generation=7;s.sequence=seq;s.created_ms=1000+seq*50;
    s.tracked=s.grip_down=true;s.gun_mesh={1,0,0,0,0,1,0,0,0,0,1,0,100,200,300,1};
    s.magazine.position={100+lancer_magazine_upper_anchor_raw.x+side,200+lancer_magazine_upper_anchor_raw.y,
        300+lancer_magazine_upper_anchor_raw.z-depth};
    s.magazine.rotation={0,3.14159265358979323846/2+twist*3.14159265358979323846/180,
        3.14159265358979323846};return s;
}
}
int main(){
    using namespace gears_weapon;
    LancerReloadPolicy policy{};
    check(LancerReloadPolicy::unpack(4000ULL|(6000ULL<<16)|(2ULL<<32),policy)&&
        policy.deadline(false,800)==4000&&policy.deadline(true,800)==6000,"Python/native VR policy encoding");
    check(LancerReloadPolicy::unpack(4125ULL|(7500ULL<<16)|(1ULL<<32),policy)&&
        policy.deadline(false,800)==800&&policy.deadline(true,0)==0,"original mode uses authentic window only");
    check(LancerReloadPolicy::unpack(4000ULL|(6000ULL<<16),policy)&&policy.deadline(true,800)==0,"off mode disables rewards");
    check(!LancerReloadPolicy::unpack(4000ULL|(6000ULL<<16)|(4ULL<<32),policy),"reject unsupported policy bits");
    check(!LancerReloadPolicy::unpack(0,policy),"reject zero timing");
    LancerMagazineInsertionAssist guide;LancerMagazineInsertionStroke stroke;
    auto s=pose(7.5);check(stroke.arm(s),"arm ordinary held item");
    check(guide.apply(s)&&guide.captured()&&!s.assisted,"entry has no instant snap");
    bool inserted{};
    for(unsigned i=2;i<=13;++i){
        s=pose(7.5-(i-1)*.5,5.5,30,i);const auto original=s;
        check(guide.apply(s),"fresh continuous guide");
        LancerInsertionGeometry raw{},help{};check(lancer_insertion_geometry(original,raw)&&lancer_insertion_geometry(s,help),"finite geometry");
        check(std::abs(raw.depth_raw-help.depth_raw)<=(s.snap_active?6.000001:1e-8),"only qualified final6cm seating changes visible depth");
        check(length(difference(s.magazine.position,original.magazine.position))<=std::hypot(14.,6.)+.000001,"bounded side/final-seat fitting");
        check(s.grip_down&&s.unassisted_magazine.position.z==original.magazine.position.z,"held input and raw motion remain unchanged");
        if(stroke.observe(s)==LancerInsertResult::inserted)inserted=true;
    }
    check(inserted,"5.5cm lateral /30degree continuous approach now inserts");
    guide.reset();s=pose(6);guide.apply(s);s=pose(3,5.5,30,5);guide.apply(s);
    check(s.assisted,"guide acts near port");
    s=pose(3,6.5,30,6);guide.apply(s);check(guide.captured(),"capture hysteresis survives small retreat");
    s=pose(3,18.1,30,7);guide.apply(s);check(!guide.captured()&&!s.assisted,"side withdrawal releases guide");
    s=pose(3,16,30,8);guide.apply(s);check(!guide.captured(),"cannot recapture in exit-only band");
    for(auto bad:{pose(3,15),pose(30),pose(3,1.8,180),pose(-5),pose(3,1.8,120)}){
        guide.reset();check(guide.apply(bad)&&!bad.assisted&&!guide.captured(),"no remote/backwards/wrong-way capture");
    }
    guide.reset();s=pose(6);guide.apply(s);s=pose(3,5.5,30,5);s.grip_down=false;
    check(!guide.apply(s)&&!guide.captured()&&!s.assisted,"release clears assistance");
    guide.reset();s=pose(6);guide.apply(s);s=pose(3,5.5,30,5);s.tracked=false;
    check(!guide.apply(s)&&!guide.captured(),"tracking loss clears assistance");
    guide.reset();s=pose(6);guide.apply(s);s=pose(3,5.5,30,7);
    check(guide.apply(s)&&!s.assisted,"long gap cannot retain ramp");
    guide.reset();s=pose(6);guide.apply(s);s=pose(3,5.5,30,5);++s.weapon;
    check(guide.apply(s)&&!s.assisted,"different item cannot retain guide");
    // Standing near the socket never creates insertion motion, even after
    // guidance settles. All samples remain at the same raw depth.
    guide.reset();s=pose(1.5);stroke.arm(s);
    for(unsigned i=1;i<10;++i){s=pose(1.5,5.5,30,i);guide.apply(s);
        check(stroke.observe(s)!=LancerInsertResult::inserted,"overlap without approach cannot insert");}
    // A forged assisted axial jump cannot use the guide as an inventory gate.
    s=pose(8,1.8,0);stroke.arm(s);stroke.observe(s);s=pose(1.5,1.8,0,5);
    s.assisted=true;s.unassisted_magazine=pose(8,1.8,0,5).magazine;
    check(stroke.observe(s)==LancerInsertResult::rejected,"assistance cannot invent axial crossing");
    // World transforms commute with the guide; do not guide toward world origin.
    guide.reset();auto turn=[](LancerInsertionSample a){
        const auto r=gears_vr::ue_matrix({.2,.8,-.3});
        auto p=gears_vr::transform(r,a.magazine.position);a.magazine.position={p.x+73,p.y-42,p.z+11};
        a.magazine.rotation=gears_vr::ue_angles(gears_vr::multiply(r,gears_vr::ue_matrix(a.magazine.rotation)));
        const auto gun=gears_vr::transform(r,{100,200,300});
        for(unsigned axis=0;axis<3;++axis)for(unsigned world=0;world<3;++world)a.gun_mesh[axis*4+world]=float(r[world][axis]);
        a.gun_mesh[12]=float(gun.x+73);a.gun_mesh[13]=float(gun.y-42);a.gun_mesh[14]=float(gun.z+11);return a;};
    auto a=turn(pose(7.5));stroke.arm(a);guide.apply(a);inserted=false;
    for(unsigned i=2;i<=13;++i){a=turn(pose(7.5-(i-1)*.5,5.5,30,i));guide.apply(a);
        if(stroke.observe(a)==LancerInsertResult::inserted)inserted=true;}
    check(inserted,"rotated translated gun uses its own port");
    guide.reset();s=pose(8,7.5,45);stroke.arm(s);guide.apply(s);inserted=false;
    for(unsigned i=2;i<=20;++i){s=pose(8-(i-1)*.5,7.5,45,i);const auto raw=s;
        check(guide.apply(s),"wide natural approach retained");
        check(std::abs(s.unassisted_magazine.position.z-raw.magazine.position.z)<1e-8,"wide snap preserves physical inward witness");
        if(stroke.observe(s)==LancerInsertResult::inserted)inserted=true;}
    check(inserted,"7.5cm lateral /45degree approach with slight overshoot inserts");
    // Shoulder-to-port acquisition may be diagonal with little additional
    // axial travel. Real decreasing distance counts; no second push/regrasp.
    guide.reset();s=pose(4.5,12.,0);stroke.arm(s);guide.apply(s);inserted=false;
    for(unsigned i=2;i<=16;++i){s=pose(4.5,(std::max)(0.,12.-double(i-1)*1.5),0,i);guide.apply(s);
        inserted=inserted||stroke.observe(s)==LancerInsertResult::inserted;}
    check(inserted&&s.grip_down,"diagonal held shoulder approach seats without a second axial push");
    // A human broad/quick approach has only150ms from18cm away to seating.
    // Earlier capture must settle the held item before its physical crossing,
    // while a stationary already-overlapping item still cannot insert above.
    guide.reset();s=pose(18,11,70);stroke.arm(s);guide.apply(s);inserted=false;
    unsigned seq=2;
    for(double depth:{11.,5.,2.}){s=pose(depth,11,70,seq++);const auto raw=s;
        check(guide.apply(s),"quick wide approach has valid assistance");
        check(std::abs(s.unassisted_magazine.position.z-raw.magazine.position.z)<1e-8,"quick snap keeps the unsnapped witness");
        inserted=inserted||stroke.observe(s)==LancerInsertResult::inserted;}
    check(!inserted,"quick physical approach waits for visible seating rather than teleporting");
    for(unsigned i=0;i<5;++i){s=pose(2.,11,70,seq++);guide.apply(s);
        inserted=inserted||stroke.observe(s)==LancerInsertResult::inserted;}
    check(inserted&&s.grip_down,"11cm/70degree approach smoothly locks while index or Grip remains held");
    check(std::abs(s.magazine.position.z-(300+lancer_magazine_upper_anchor_raw.z))<1e-8,"completed snap reaches actual port");
    check(stroke.observe(pose(2.,11,70,seq++))!=LancerInsertResult::inserted,"continued hold cannot insert the same item twice");
    // Holding inside the near volume without a real approach cannot qualify,
    // even when an internal caller falsely supplies a completed blend flag.
    guide.reset();s=pose(5.,1.8,0);stroke.arm(s);guide.apply(s);stroke.observe(s);
    s=pose(5.,1.8,0,6);s.assisted=s.snap_active=true;s.snap_progress=1;
    s.unassisted_magazine=s.magazine;s.magazine.position.z=300+lancer_magazine_upper_anchor_raw.z;
    check(stroke.observe(s)==LancerInsertResult::rejected,"completed cosmetic flag without real approach cannot lock");
    std::printf("Lancer insertion assistance: %u checks passed\n",checks);
}
