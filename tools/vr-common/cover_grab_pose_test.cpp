#include "cover_grab_pose.h"
#include "cover_grab_transition.h"
#include <cstdio>
#include <limits>
namespace cp=gears_cover_grab_pose;namespace vr=gears_vr;namespace mount=gears_hand_authored_mount;
unsigned checks{},failures{};
#define C(value) do{++checks;if(!(value)){++failures;std::printf("FAIL %u: %s\n",__LINE__,#value);}}while(0)
bool near(double a,double b){return std::abs(a-b)<1e-8;}
struct Fixture {
    cp::Frame f{};cp::Contact contact{};cp::Profile profile{};cp::Plan plan{};
    Fixture(vr::CoverEdgeKind kind=vr::CoverEdgeKind::left){
        profile.id=123;profile.kind=kind;profile.geometry_signature=mount::marcus115_signature;
        profile.wrist_in_edge.position={10,0,0};profile.acquire_metres=.1;profile.retain_metres=.16;
        profile.maximum_wrist_turn=1.;
        // Synthetic authored joint data, not an accepted cover hand preset.
        for(unsigned i=0;i<15;++i)profile.held[i]={0,0,std::sin(double(i+1)*.01),std::cos(double(i+1)*.01)};
        f.expected={{1,2,3,4,5,6,7},42,200,3,profile.id};contact.key=f.expected;
        contact.observed_ns=980000000;contact.edge_world.position={100,200,300};
        f.now_ns=1000000000;f.model_scale=1;f.units_per_metre=100;
        f.actual_geometry_signature=mount::marcus115_signature;f.mode=cp::Mode::approach;
        f.cover.sequence=3;f.cover.observed_ns=990000000;f.cover.epochs=f.expected.epochs;
        f.cover.cover_valid=f.cover.interaction_enabled=true;f.cover.edge={42,kind,true,true};
        f.hand.sequence=4;f.hand.observed_ns=990000000;f.hand.session_epoch=5;f.hand.space_epoch=6;
        f.hand.input_active=f.hand.position_valid=f.hand.position_tracked=true;
        f.interaction.phase=vr::CoverPhase::attached;f.interaction.offset_certified=true;
        wrist({{110,200,300},{}});
    }
    void wrist(vr::Pose desired){
        mount::Mount m{};C(mount::marcus115(mount::marcus115_signature,mount::Side::left,f.model_scale,f.units_per_metre,m));
        const auto q=vr::multiply(desired.orientation,vr::inverse(m.calibration.grip_to_wrist_rotation));
        const auto d=vr::transform(vr::rotation_matrix(q),m.calibration.grip_to_wrist_metres);
        f.physical_grip_world={{desired.position.x-d.x*f.units_per_metre,
            desired.position.y-d.y*f.units_per_metre,desired.position.z-d.z*f.units_per_metre},q};
    }
    void grab(){f.mode=cp::Mode::held;f.hand.lgrip_down=true;
        f.ownership={vr::CoverLeftHandOwner::cover,9};f.interaction.owns_left_hand=true;
        f.interaction.phase=vr::CoverPhase::grabbing;f.interaction.edge_id=42;f.interaction.request_id=9;}
    cp::Code run(){return cp::prepare(f,contact,profile,plan);}
};
int main(){
    namespace transition=gears_cover_grab_transition;
    {Fixture x;transition::State state{};transition::Joints free{};free[0].w=std::sqrt(.99999);
        x.wrist({{108,200,300},{}});
        const auto step=[&](){x.f.hand.observed_ns=x.f.cover.observed_ns=x.contact.observed_ns=x.f.now_ns;
            return transition::prepare(x.f,x.contact,x.profile,free,state,x.plan);};
        C(step()==transition::Code::prepared);C(near(x.plan.wrist_world.position.x,108));
        for(unsigned n=1;n<=3;++n){x.f.now_ns+=40000000;C(step()==transition::Code::prepared);}
        C(near(x.plan.wrist_world.position.x,110));C(near(state.weight,1));
        x.grab();x.f.now_ns+=20000000;C(step()==transition::Code::prepared);
        C(x.plan.joints[14].z!=0);
        // Release follows a newly moved/rotated physical wrist. An obsolete
        // contact1000 units away must never become the release target.
        x.f.mode=cp::Mode::release;x.f.hand.lgrip_down=false;x.f.ownership={};
        x.f.interaction.owns_left_hand=false;x.f.interaction.request_left_hand=false;
        x.contact.edge_world.position.x=1000;
        x.wrist({{200,300,400},{0,0,std::sqrt(.5),std::sqrt(.5)}});x.f.now_ns+=40000000;
        C(step()==transition::Code::prepared);C(x.plan.mode==cp::Mode::release);
        C(near(x.plan.wrist_world.position.x,200));C(x.plan.wrist_world.position.y>300&&x.plan.wrist_world.position.y<302);
        C(near(x.plan.wrist_world.position.z,400));
        for(unsigned n=0;n<8;++n){x.f.now_ns+=40000000;step();}
        C(!state.known);C(!x.plan.active);
    }
    for(unsigned failure=0;failure<6;++failure){Fixture x;transition::State state{};transition::Joints free{};
        C(transition::prepare(x.f,x.contact,x.profile,free,state,x.plan)==transition::Code::prepared);
        x.f.now_ns+=20000000;x.f.cover.observed_ns=x.f.hand.observed_ns=x.contact.observed_ns=x.f.now_ns;
        switch(failure){case 0:x.f.ownership.owner=vr::CoverLeftHandOwner::other;break;
        case 1:x.f.hand.observed_ns=0;break;case 2:x.f.physical_grip_world.orientation={0,0,0,0};break;
        case 3:free[5]={0,0,0,0};break;case 4:x.f.now_ns=1;break;
        default:x.contact.key.component++;break;}
        C(transition::prepare(x.f,x.contact,x.profile,free,state,x.plan)!=transition::Code::prepared);
        C(!state.known);C(!x.plan.active);
    }
    {Fixture x;transition::State state{};transition::Joints free{};
        C(transition::prepare(x.f,x.contact,x.profile,free,state,x.plan)==transition::Code::prepared);
        x.f.now_ns+=500000000;x.f.cover.observed_ns=x.f.hand.observed_ns=x.contact.observed_ns=x.f.now_ns;
        x.wrist({{108,200,300},{}});
        C(transition::prepare(x.f,x.contact,x.profile,free,state,x.plan)==transition::Code::prepared);
        C(near(state.weight,0));C(near(x.plan.wrist_world.position.x,108));
        ++x.f.expected.epochs.reference;x.f.cover.epochs=x.f.expected.epochs;x.contact.key=x.f.expected;
        x.f.now_ns+=40000000;x.f.cover.observed_ns=x.f.hand.observed_ns=x.contact.observed_ns=x.f.now_ns;
        C(transition::prepare(x.f,x.contact,x.profile,free,state,x.plan)==transition::Code::prepared);C(near(state.weight,0));
    }
    for(auto kind:{vr::CoverEdgeKind::left,vr::CoverEdgeKind::right,vr::CoverEdgeKind::top}){
        Fixture x(kind);C(x.run()==cp::Code::prepared);C(x.plan.active);
        C(x.plan.mode==cp::Mode::approach);C(near(x.plan.wrist_world.position.x,110));
        C(!x.f.interaction.owns_left_hand);C(x.f.ownership.owner==vr::CoverLeftHandOwner::free);
        x.grab();C(x.run()==cp::Code::prepared);C(x.plan.mode==cp::Mode::held);
        C(near(x.plan.joints[14].z,x.profile.held[14].z));
        x.f.mode=cp::Mode::release;C(x.run()==cp::Code::inactive);C(!x.plan.active);
    }
    {Fixture x;x.contact.edge_world.orientation={0,0,std::sqrt(.5),std::sqrt(.5)};
        x.wrist({{100,210,300},x.contact.edge_world.orientation});C(x.run()==cp::Code::prepared);
        C(near(x.plan.wrist_world.position.x,100));C(near(x.plan.wrist_world.position.y,210));}
    {Fixture x;x.f.model_scale=2;x.wrist({{120,200,300},{}});C(x.run()==cp::Code::prepared);
        C(near(x.plan.wrist_world.position.x,120));}
    for(unsigned defect=0;defect<8;++defect){Fixture x;x.grab();
        switch(defect){case 0:++x.contact.key.component;break;case 1:++x.contact.key.shape_generation;break;
        case 2:++x.contact.key.epochs.world;break;case 3:++x.contact.key.epochs.slot;break;
        case 4:++x.contact.key.epochs.reference;break;case 5:++x.contact.key.profile;break;
        case 6:++x.contact.key.edge;break;default:++x.f.hand.space_epoch;break;}
        C(x.run()==cp::Code::binding);C(!x.plan.active);}
    for(unsigned defect=0;defect<6;++defect){Fixture x;
        switch(defect){case 0:x.contact.observed_ns=x.f.now_ns-100000001;break;
        case 1:x.f.hand.observed_ns=x.f.now_ns+1;break;case 2:x.f.cover.observed_ns=0;break;
        case 3:x.f.now_ns=0;break;case 4:x.contact.observed_ns=0;break;
        default:x.f.hand.observed_ns=x.f.now_ns-100000001;break;}
        C(x.run()==cp::Code::stale);C(!x.plan.active);}
    {Fixture x;x.f.ownership.owner=vr::CoverLeftHandOwner::other;C(x.run()==cp::Code::conflict);}
    {Fixture x;x.f.hand.lgrip_down=true;C(x.run()==cp::Code::conflict);}
    {Fixture x;x.grab();++x.f.ownership.request_id;C(x.run()==cp::Code::conflict);}
    {Fixture x;x.grab();x.f.hand.lgrip_down=false;C(x.run()==cp::Code::inactive);C(!x.plan.active);}
    {Fixture x;x.grab();x.f.cover.edge.within_acquisition_reach=false;C(x.run()==cp::Code::prepared);}
    {Fixture x;x.f.cover.edge.within_acquisition_reach=false;C(x.run()==cp::Code::conflict);}
    {Fixture x;x.wrist({{95,200,300},{}});C(x.run()==cp::Code::distant);
        x.grab();C(x.run()==cp::Code::prepared);x.wrist({{93,200,300},{}});C(x.run()==cp::Code::distant);}
    {Fixture x;x.wrist({{110,200,300},{0,0,1,0}});C(x.run()==cp::Code::orientation);}
    {Fixture x;x.f.actual_geometry_signature[0]^=1;C(x.run()==cp::Code::profile);}
    {Fixture x;x.profile.held[4]={0,0,0,0};C(x.run()==cp::Code::profile);}
    {Fixture x;x.profile.kind=vr::CoverEdgeKind::right;C(x.run()==cp::Code::profile);}
    {Fixture x;x.profile.retain_metres=.5;C(x.run()==cp::Code::profile);}
    {Fixture x;x.contact.edge_world.position.x=std::numeric_limits<double>::quiet_NaN();C(x.run()==cp::Code::geometry);}
    {Fixture x;x.f.interaction.offset_certified=false;C(x.run()==cp::Code::inactive);}
    {Fixture x;C(x.run()==cp::Code::prepared);x.f.mode=cp::Mode::none;C(x.run()==cp::Code::inactive);C(!x.plan.active);}
    std::printf("CoverGrabPoseTest checks=%u failures=%u\n",checks,failures);return failures?1:0;
}
