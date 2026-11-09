#include "weapon_hammerburst_latch_mount.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
namespace h=gears_hammerburst_latch_mount;namespace v=gears_vr;
static unsigned checks{};
static void check(bool b){++checks;if(!b){std::fprintf(stderr,"mount check%u failed\n",checks);std::abort();}}
static bool near(v::Vector a,v::Vector b,double e=1e-8){return std::hypot(std::hypot(a.x-b.x,a.y-b.y),a.z-b.z)<e;}
static bool same(v::Quaternion a,v::Quaternion b){return std::abs(std::abs(a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w)-1)<1e-9;}
static v::Pose compose(v::Pose p,v::Pose q,double size=1){return {h::detail::add(p.position,h::detail::rotate(p.orientation,h::detail::scale(q.position,size))),v::normalized(v::multiply(p.orientation,q.orientation))};}
int main(){
    // Unpublished offline motion fixture. These data exercise coordinate math,
    // not its rejected self-surface fit or any production publication/admission.
    const h::Profile profile{{{2.245603417759895,5.953499850993114,-15.948195547770123},
        {-.25440361097169667,-.784420369023758,-.08816882605089933,.5587394254015006}}};
    const v::Pose closed{{-2.228458981790027e-7,31.3649845123291,2.125365734100342},{0,0,.7071067811865475,.7071067811865475}};
    h::Target first{};check(h::target(profile,closed,1,{},1,h::mount::marcus115_signature,1,100,first));
    check(near(first.grip_in_weapon_metres.position,{-.08060267156267484,.32334205872900235,-.08962938890321736},1e-7));
    const v::Vector old_contact{-.021006133428458935,.24271409642329097,.04013061504100349};
    check(!near(first.grip_in_weapon_metres.position,old_contact,.035));
    const auto old_distance=std::hypot(std::hypot(first.grip_in_weapon_metres.position.x-old_contact.x,
        first.grip_in_weapon_metres.position.y-old_contact.y),first.grip_in_weapon_metres.position.z-old_contact.z);
    check(std::abs(old_distance-.16398254262804464)<1e-7);
    for(double angle:{0.,.217196513409997,.4188790697425776,.6205614850618819,.8377581041151458}){
        for(double units:{10.,100.,500.})for(double size:{.4,1.,2.})for(double body:{.7,1.,1.3}){
            const v::Quaternion turn{0,std::sin(angle/2),0,std::cos(angle/2)};
            auto part=closed;part.orientation=v::normalized(v::multiply(closed.orientation,turn));
            const v::Pose gun{{230,-75,42},v::normalized(v::multiply(v::Quaternion{.2,0,0,.9797958971132712},v::Quaternion{0,0,.3,.9539392014169457}))};
            part=compose(gun,part,size);h::Target target{};
            check(h::target(profile,part,size,gun,size,h::mount::marcus115_signature,body,units,target));
            h::mount::Mount calibration{};check(h::mount::marcus115(h::mount::marcus115_signature,h::mount::Side::left,body,units,calibration));
            v::Pose wrist{};check(gears_hand_wrist::native_wrist_target(target.grip_world_raw,calibration.calibration,units,wrist)==gears_hand_wrist::Status::okay);
            check(near(wrist.position,target.wrist_world_raw.position));check(same(wrist.orientation,target.wrist_world_raw.orientation));
            const auto decoded=compose(gun,{{target.grip_in_weapon_metres.position.x*units,target.grip_in_weapon_metres.position.y*units,target.grip_in_weapon_metres.position.z*units},target.grip_in_weapon_metres.orientation},size);
            check(near(decoded.position,target.grip_world_raw.position));check(same(decoded.orientation,target.grip_world_raw.orientation));
            check(near(target.wrist_world_raw.position,compose(part,profile.wrist_in_latch_reference_raw,size).position));
        }
    }
    h::Target sentinel=first,copy=sentinel;
    const auto reject=[&](h::Profile p,v::Pose part,double ps,v::Pose gun,double gs,h::mount::Signature signature,double body,double units){
        check(!h::target(p,part,ps,gun,gs,signature,body,units,sentinel));check(std::memcmp(&copy,&sentinel,sizeof sentinel)==0);
    };
    auto bad_signature=h::mount::marcus115_signature;bad_signature[31]^=1;
    reject(profile,closed,1,{},1,bad_signature,1,100);
    reject(profile,closed,1,{},1,h::mount::marcus115_signature,1,0);
    reject(profile,closed,1,{},1,h::mount::marcus115_signature,0,100);
    reject(profile,closed,1.01,{},1,h::mount::marcus115_signature,1,100);
    auto bad=profile;bad.wrist_in_latch_reference_raw.position.x=std::numeric_limits<double>::quiet_NaN();reject(bad,closed,1,{},1,h::mount::marcus115_signature,1,100);
    bad=profile;bad.wrist_in_latch_reference_raw.orientation.w=0;reject(bad,closed,1,{},1,h::mount::marcus115_signature,1,100);
    std::printf("Hammerburst latch mount:%u coordinate checks passed; no skin/runtime acceptance\n",checks);
}
