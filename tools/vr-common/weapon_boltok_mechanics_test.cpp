#ifdef NDEBUG
#undef NDEBUG
#endif
#include "weapon_boltok_mechanics.h"
#include <cassert>
#include <cstdio>
using namespace gears_weapon;
namespace {
const BoltokReloadIdentity id{1,2,3,4,5,6,7,8,9};
BoltokReloadStamp stamp(unsigned seq){return {id,seq,1000000000+std::int64_t(seq)*20000000,1000,true,true,true};}
Vector at(double angle){return {.08*std::cos(angle),.08*std::sin(angle),0};}
HammerburstLatchGeometry geometry(){HammerburstLatchGeometry g{};g.axis={0,0,1};g.closed_contact=at(0);
    g.open_angle=1.4;g.current_instance_verified=true;return g;}
void swing(){
    for(double angle:{0.,.7,1.4})for(double sign:{-1.,1.}){
        const Quaternion q{0,sign*std::sin(angle/2),0,sign*std::cos(angle/2)};double measured{};
        assert(boltok_swing_angle(q,{},measured)&&std::abs(measured-angle)<1e-9);
    }
    {double measured{};assert(!boltok_swing_angle({.2,0,0,std::sqrt(.96)},{},measured));}
    auto g=geometry();BoltokSwingStroke stroke;auto f=stamp(1);
    assert(stroke.begin(g,f,at(0),7,true,0,false,f.observed_ns));
    BoltokReloadPlan p{};p.identity=id;p.generation=1;p.original_reload_serial=2;
    const BoltokReloadStep step{BoltokStepKind::move_original_parts,boltok_object_part(3),3,4,5,6,7,0};
    BoltokMotionReceipt r{};assert(!stroke.receipt(p,step,r));
    for(unsigned i=1;i<=10;++i){f=stamp(i+1);assert(stroke.move(f,at(.14*i),7,true,f.observed_ns));}
    assert(!stroke.acknowledge(f,0,f.observed_ns)); // Hand arrived, part did not.
    assert(!stroke.receipt(p,step,r));
    assert(stroke.acknowledge(f,1.4,f.observed_ns)&&stroke.receipt(p,step,r));
    assert(r.tracked_samples==11&&r.maximum_sample_gap_ns==20000000&&r.motion_begin_sequence==1);
    assert(stroke.begin(g,f,at(1.4),8,true,1.4,true,f.observed_ns));
    for(unsigned i=1;i<=10;++i){f=stamp(11+i);assert(stroke.move(f,at(1.4-.14*i),8,true,f.observed_ns));}
    assert(stroke.acknowledge(f,0,f.observed_ns));
    for(unsigned bad=0;bad<9;++bad){
        f=stamp(1);assert(stroke.begin(g,f,at(0),7,true,0,false,f.observed_ns));
        f=stamp(2);auto hand=at(.1);std::uint64_t token=7;bool held=true;auto now=f.observed_ns;
        switch(bad){case 0:f.left_tracked=false;break;case 1:f.right_tracked=false;break;
            case 2:f.identity.input_epoch++;break;case 3:token++;break;case 4:held=false;break;
            case 5:f.observed_ns+=100000001;now=f.observed_ns;break;
            case 6:hand=at(1.4);break;case 7:hand.z=.1;break;case 8:now+=100000001;break;}
        assert(!stroke.move(f,hand,token,held,now)&&!stroke.receipt(p,step,r));
    }
}
BoltokChamberGeometry chambers(){
    BoltokChamberGeometry g{};g.calibration=1;g.current_instance_verified=true;
    for(unsigned i=0;i<6;++i){auto& p=g.ports[i];const auto a=i*3.141592653589793/3;
        p.port_in_weapon.position={.0192*std::cos(a),.0192*std::sin(a),0};
        p.lateral_tolerance_metres=.006;p.maximum_sample_gap_ns=100000000;}
    return g;
}
FeedSample feed(BoltokReloadStamp const& f,BoltokChamberGeometry const& g,unsigned slot,
                double depth,Pose parent={}){
    auto local=g.ports[slot].port_in_weapon.position;local.z+=depth;
    return {id.weapon,88,id.world,id.space_epoch,f.sequence,f.observed_ns,parent,
        {sum(parent.position,rotate(parent.orientation,local)),parent.orientation},true,true};
}
void six_holes(){
    auto g=chambers();assert(g.valid());
    for(unsigned mask=0;mask<64;++mask)for(unsigned slot=0;slot<6;++slot)for(bool moved:{false,true}){
        BoltokChamberInsertion stroke;assert(stroke.arm(id,88,mask,g));
        const Pose parent=moved?Pose{{3,4,5},{0,0,std::sin(.4),std::cos(.4)}}:Pose{};
        auto f=stamp(1);auto s=feed(f,g,slot,.06,parent);
        if(mask&(1u<<slot)){assert(stroke.observe(f,s,f.observed_ns)==FeedResult::rejected);continue;}
        assert(stroke.observe(f,s,f.observed_ns)==FeedResult::approaching&&stroke.slot()==slot);
        f=stamp(2);s=feed(f,g,slot,.03,parent);
        assert(stroke.observe(f,s,f.observed_ns)==FeedResult::approaching);
        f=stamp(3);s=feed(f,g,slot,0,parent);
        assert(stroke.observe(f,s,f.observed_ns)==FeedResult::inserted&&stroke.slot()==slot);
        assert(stroke.observe(f,s,f.observed_ns)==FeedResult::rejected);
    }
    for(unsigned bad=0;bad<5;++bad){auto invalid=g;
        switch(bad){case 0:invalid.calibration=0;break;case 1:invalid.current_instance_verified=false;break;
            case 2:invalid.ports[0].lateral_tolerance_metres=.035;break;
            case 3:invalid.ports[1].port_in_weapon.position=invalid.ports[0].port_in_weapon.position;
                invalid.ports[1].port_in_weapon.position.z=.5;break;
            case 4:invalid.ports[1].item_in_left_grip.position.x=.02;break;}
        assert(!invalid.valid());}
}
void insertion_rejections(){
    const auto g=chambers();
    for(unsigned bad=0;bad<9;++bad){
        BoltokChamberInsertion stroke;assert(stroke.arm(id,88,0,g));auto f=stamp(1);auto s=feed(f,g,0,.06);
        assert(stroke.observe(f,s,f.observed_ns)==FeedResult::approaching);
        f=stamp(2);s=feed(f,g,0,0);auto now=f.observed_ns;
        switch(bad){case 0:s.item++;break;case 1:s.space_epoch++;break;case 2:s.sequence++;break;
            case 3:f.identity.weapon++;break;case 4:s.item_held=false;break;
            case 5:s.left_grip.position.z=-.05;break;case 6:f.left_tracked=false;break;
            case 7:s.left_grip.position.x=g.ports[1].port_in_weapon.position.x;
                s.left_grip.position.y=g.ports[1].port_in_weapon.position.y;break;
            case 8:now+=100000001;break;}
        assert(stroke.observe(f,s,now)==FeedResult::rejected);
    }
    // A >100ms gap invalidates the trajectory. The same still-held round can
    // only retry after a NEW outward approach; it cannot insert from the seat.
    BoltokChamberInsertion retry;assert(retry.arm(id,88,0,g));
    auto f=stamp(1);assert(retry.observe(f,feed(f,g,0,.06),f.observed_ns)==FeedResult::approaching);
    f=stamp(8);assert(retry.observe(f,feed(f,g,0,0),f.observed_ns)==FeedResult::rejected);
    f=stamp(9);assert(retry.observe(f,feed(f,g,0,0),f.observed_ns)==FeedResult::rejected);
    f=stamp(10);assert(retry.observe(f,feed(f,g,0,.06),f.observed_ns)==FeedResult::approaching);
    f=stamp(11);assert(retry.observe(f,feed(f,g,0,.03),f.observed_ns)==FeedResult::approaching);
    f=stamp(12);assert(retry.observe(f,feed(f,g,0,0),f.observed_ns)==FeedResult::inserted);
}
}
int main(){swing();six_holes();insertion_rejections();
    std::puts("Boltok mechanics: measured swing acknowledgement, six-hole selection and invalid trajectory checks passed (synthetic only).");}
