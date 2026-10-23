#include "stereo_bridge.h"
#include "stereo_image_ticket.h"
#include <cstdio>
#include <limits>
#include <thread>
using namespace gears_vr;
static bool approximately(double a,double b,double tolerance=1e-4){return std::abs(a-b)<=tolerance;}
static Quaternion axis(unsigned i,double angle){Quaternion q{};q.w=std::cos(angle/2);if(i==0)q.x=std::sin(angle/2);if(i==1)q.y=std::sin(angle/2);if(i==2)q.z=std::sin(angle/2);return q;}
// Independent point oracle: quaternion sandwich instead of matrix composition.
static Vector rotate(Quaternion q,Vector v){auto p=multiply(multiply(q,{v.x,v.y,v.z,0}),inverse(q));return {p.x,p.y,p.z};}
static Vector point(StereoMatrix const& m,Vector p){return {p.x*m[0]+p.y*m[4]+p.z*m[8]+m[12],p.x*m[1]+p.y*m[5]+p.z*m[9]+m[13],p.x*m[2]+p.y*m[6]+p.z*m[10]+m[14]};}
static StereoSample sample(){StereoSample s{};s.session=21;s.sequence=1;s.received_ns=1000000000;s.predicted_time=123456;
    s.focused=s.should_render=1;s.head_flags=s.view_flags=15;s.head.orientation={};
    for(unsigned i=0;i<2;++i){s.eyes[i].pose.orientation={};s.eyes[i].pose.position.x=i?.032:-.032;s.eyes[i].fov=i?StereoFov{-.7,.85,.65,-.8}:StereoFov{-.8,.7,.75,-.65};}return s;}
static StereoMatrix identity(){return {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};}
static StereoMatrix projection(){return {1.2f,0,0,0,0,1.5f,0,0,.02f,-.01f,.0001f,1,0,0,5,0};}
#define CHECK(condition) do{if(!(condition)){std::printf("Failed line %d: %s\n",__LINE__,#condition);return __LINE__;}}while(0)
static StereoSample posed(Pose head,std::uint64_t sequence=1){
    auto s=sample();s.head=head;s.sequence=sequence;s.received_ns+=sequence*1000000;s.predicted_time+=sequence;
    for(unsigned eye=0;eye<2;++eye){auto offset=rotate(head.orientation,{eye?.032:-.032,0,0});
        s.eyes[eye].pose={{head.position.x+offset.x,head.position.y+offset.y,head.position.z+offset.z},head.orientation};}
    return s;
}
static int heading_tests(){
    constexpr double pi=3.14159265358979323846;
    static_assert(static_cast<unsigned>(StereoStatus::invalid)==7);
    static_assert(static_cast<unsigned>(StereoStatus::heading_unavailable)==10);
    Pose output{{9,8,7},axis(0,.3)},untouched=output;
    Pose invalid{};invalid.orientation.w=2;CHECK(!stereo_heading_reference(invalid,output)&&stereo_same_pose(output,untouched));
    invalid={};invalid.position.z=std::numeric_limits<double>::infinity();CHECK(!stereo_heading_reference(invalid,output)&&stereo_same_pose(output,untouched));
    invalid={};invalid.orientation.x=std::numeric_limits<double>::quiet_NaN();CHECK(!stereo_heading_reference(invalid,output)&&stereo_same_pose(output,untouched));
    for(double yaw:{0.,.7,-.8,pi,-pi}){
        Pose head{{1,2,3},axis(1,yaw)};CHECK(stereo_heading_reference(head,output));
        CHECK(output.position.x==1&&output.position.y==2&&output.position.z==3);
        CHECK(output.orientation.x==0&&output.orientation.z==0);
        auto forward=rotate(output.orientation,{0,0,-1});auto expected=rotate(head.orientation,{0,0,-1});
        CHECK(approximately(forward.x,expected.x,1e-10)&&approximately(forward.z,expected.z,1e-10));
        head.orientation={-head.orientation.x,-head.orientation.y,-head.orientation.z,-head.orientation.w};
        Pose negative;CHECK(stereo_heading_reference(head,negative));CHECK(stereo_same_pose(negative,output));
    }
    Pose tilted{{1,2,3},multiply(axis(1,.7),multiply(axis(0,.35),axis(2,-.4)))};
    CHECK(stereo_heading_reference(tilted,output));
    auto heading=axis(1,.7);CHECK(approximately(output.orientation.y,heading.y,1e-10)&&approximately(output.orientation.w,heading.w,1e-10));
    StereoGate g;auto tilted_sample=posed(tilted);auto frozen=tilted_sample;
    auto initial=g.update(tilted_sample,tilted_sample.received_ns);
    CHECK(initial&&initial.status==StereoStatus::initial&&initial.reference_generation==1&&!initial.recentered&&!initial.heading_deferred);
    CHECK(stereo_same(tilted_sample,frozen));CHECK(stereo_same_pose(initial.reference,output));
    // Initial tilt remains visible, but returning upright leaves no pitch/roll
    // in the view basis: world-up is LOCAL Y rather than the initial head up.
    StereoMatrices tilted_matrices;CHECK(stereo_matrices(identity(),projection(),initial.reference,tilted_sample,100,tilted_matrices));
    auto frozen_old_matrices=tilted_matrices;
    CHECK(std::abs(tilted_matrices.view[0][1])>.01||std::abs(tilted_matrices.view[0][6])>.01);
    auto upright=posed({tilted.position,heading},2);auto u=g.update(upright,upright.received_ns);
    StereoMatrices matrices;CHECK(stereo_matrices(identity(),projection(),u.reference,upright,100,matrices));
    for(unsigned eye=0;eye<2;++eye)for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)
        CHECK(approximately(matrices.view[eye][4*i+j],i==j?1:0,1e-6));
    // Recenter requests coalesce and cannot consume on rejected samples.
    g.request_recenter();g.request_recenter();
    auto stale=posed(tilted,3);CHECK(g.update(stale,stale.received_ns+100000001).status==StereoStatus::stale);
    auto future=stale;CHECK(g.update(future,future.received_ns-1).status==StereoStatus::stale);
    auto untracked=stale;untracked.head_flags=3;CHECK(g.update(untracked,untracked.received_ns).status==StereoStatus::untracked);
    auto inactive=stale;inactive.focused=0;CHECK(g.update(inactive,inactive.received_ns).status==StereoStatus::inactive);
    auto invalid_geometry=stale;invalid_geometry.head.orientation.w=2;
    CHECK(g.update(invalid_geometry,invalid_geometry.received_ns).status==StereoStatus::invalid);
    auto bad_source=stale;bad_source.source=StereoSource::MetaOperatorSimulated;CHECK(g.update(bad_source,bad_source.received_ns,true).status==StereoStatus::invalid);
    bad_source.source=static_cast<StereoSource>(55);CHECK(g.update(bad_source,bad_source.received_ns).status==StereoStatus::invalid);
    auto different_session=stale;++different_session.session;CHECK(g.update(different_session,different_session.received_ns).status==StereoStatus::session_changed);
    CHECK(g.update(tilted_sample,tilted_sample.received_ns).status==StereoStatus::replayed);
    auto changed_same=upright;changed_same.head.position.z+=.1;CHECK(g.update(changed_same,changed_same.received_ns).status==StereoStatus::replayed);
    auto backward=stale;backward.received_ns=upright.received_ns-1;CHECK(g.update(backward,stale.received_ns).status==StereoStatus::replayed);
    backward=stale;backward.predicted_time=upright.predicted_time-1;CHECK(g.update(backward,stale.received_ns).status==StereoStatus::replayed);
    std::uint64_t vertical_sequence=3;
    for(double vertical:{pi/2,-pi/2}){
        Pose vertical_head{{4,5,6},multiply(axis(1,.9),axis(0,vertical))};
        output=untouched;CHECK(!stereo_heading_reference(vertical_head,output)&&stereo_same_pose(output,untouched));
        auto v=posed(vertical_head,vertical_sequence++);auto raw=v;auto deferred=g.update(v,v.received_ns);
        CHECK(deferred&&deferred.heading_deferred&&!deferred.recentered&&deferred.reference_generation==1);
        CHECK(stereo_same_pose(deferred.reference,initial.reference)&&stereo_same(raw,deferred.sample));
        auto flipped=v;flipped.head.position.x+=.1;CHECK(g.update(flipped,flipped.received_ns).status==StereoStatus::replayed);
    }
    auto apply=posed({{4,5,6},multiply(axis(1,-.6),multiply(axis(0,.2),axis(2,.3)))},5);
    auto raw_apply=apply;auto recentered=g.update(apply,apply.received_ns);
    CHECK(recentered&&recentered.recentered&&!recentered.heading_deferred&&recentered.reference_generation==2);
    CHECK(recentered.reference.position.x==4&&recentered.reference.position.y==5&&recentered.reference.position.z==6);
    CHECK(approximately(recentered.reference.orientation.y,std::sin(-.3),1e-10)&&stereo_same(apply,raw_apply));
    CHECK(!g.update(apply,apply.received_ns).recentered);CHECK(g.update(apply,apply.received_ns).reference_generation==2);
    CHECK(g.update(upright,upright.received_ns).status==StereoStatus::replayed);
    auto recentered_upright=posed({recentered.reference.position,recentered.reference.orientation},6);
    auto ru=g.update(recentered_upright,recentered_upright.received_ns);
    CHECK(ru&&ru.reference_generation==2&&!ru.recentered);
    CHECK(stereo_matrices(identity(),projection(),ru.reference,recentered_upright,100,matrices));
    for(unsigned eye=0;eye<2;++eye)for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)
        CHECK(approximately(matrices.view[eye][4*i+j],i==j?1:0,1e-6));
    for(unsigned eye=0;eye<2;++eye)CHECK(frozen_old_matrices.view[eye]==tilted_matrices.view[eye]&&frozen_old_matrices.projection[eye]==tilted_matrices.projection[eye]);
    StereoGate pending_initial;pending_initial.request_recenter();pending_initial.request_recenter();
    auto vertical=posed({{1,2,3},axis(0,pi/2)});auto unavailable=pending_initial.update(vertical,vertical.received_ns);
    CHECK(!unavailable&&unavailable.status==StereoStatus::heading_unavailable&&unavailable.heading_deferred&&unavailable.reference_generation==0);
    CHECK(stereo_same(unavailable.sample,vertical));
    auto malformed_repeat=vertical;malformed_repeat.head.position.x+=1;
    CHECK(pending_initial.update(malformed_repeat,malformed_repeat.received_ns).status==StereoStatus::replayed);
    auto old_time=posed(tilted,2);old_time.received_ns=vertical.received_ns-1;
    CHECK(pending_initial.update(old_time,vertical.received_ns).status==StereoStatus::replayed);
    auto changed_source=vertical;changed_source.source=StereoSource::MetaOperatorSimulated;changed_source.head_flags=3;
    CHECK(pending_initial.update(changed_source,changed_source.received_ns,true).status==StereoStatus::invalid);
    different_session=vertical;++different_session.session;CHECK(pending_initial.update(different_session,different_session.received_ns).status==StereoStatus::session_changed);
    auto ready=posed(tilted,2);auto established=pending_initial.update(ready,ready.received_ns);
    CHECK(established&&established.status==StereoStatus::initial&&established.reference_generation==1&&!established.recentered&&!established.heading_deferred);
    pending_initial.reset();CHECK(pending_initial.update(tilted_sample,tilted_sample.received_ns).reference_generation==1);
    StereoGate simulated_gate;auto simulated_tilt=posed(tilted);
    simulated_tilt.source=StereoSource::MetaOperatorSimulated;simulated_tilt.head_flags=3;
    CHECK(simulated_gate.update(simulated_tilt,simulated_tilt.received_ns,true).reference_generation==1);
    simulated_gate.request_recenter();auto simulated_next=posed(apply.head,2);
    simulated_next.source=StereoSource::MetaOperatorSimulated;simulated_next.head_flags=3;
    auto simulated_result=simulated_gate.update(simulated_next,simulated_next.received_ns,true);
    CHECK(simulated_result&&simulated_result.recentered&&simulated_result.reference_generation==2&&simulated_result.sample.head_flags==3);
    CHECK(stereo_same(simulated_next,simulated_result.sample));
    // Boundary: .05 is accepted; the helper does not secretly normalize up to
    // the tilted head's up vector. Near-vertical both signs defer.
    for(double sign:{-1.,1.}){
        Pose near_vertical{{},axis(0,sign*std::acos(.049))};CHECK(!stereo_heading_reference(near_vertical,output));
        near_vertical.orientation=axis(0,sign*std::acos(.051));CHECK(stereo_heading_reference(near_vertical,output));
    }
    // New heading-origin reference, every rotation/translation axis, both eyes:
    // independent quaternion point oracle and asymmetric-FOV edge mapping.
    Pose reference=recentered.reference;
    for(unsigned motion=0;motion<2;++motion)for(unsigned a=0;a<3;++a)for(double sign:{-1.,1.}){
        Vector delta{};if(motion==1){if(a==0)delta.x=.06*sign;if(a==1)delta.y=.04*sign;if(a==2)delta.z=.05*sign;}
        auto offset=rotate(reference.orientation,delta);
        Pose current{{reference.position.x+offset.x,reference.position.y+offset.y,reference.position.z+offset.z},multiply(reference.orientation,axis(a,motion==0?.25*sign:0))};
        auto s=posed(current,10+a);auto original=s;
        CHECK(stereo_matrices(identity(),projection(),reference,s,100,matrices));
        CHECK(stereo_same(s,original));
        CHECK(approximately(stereo_distance(stereo_camera_position(matrices.view[0]),stereo_camera_position(matrices.view[1])),6.4,.001));
        for(unsigned eye=0;eye<2;++eye){
            for(Vector point_native:{Vector{2,5,7},Vector{-33,72,140}}){
                auto world=rotate(reference.orientation,{point_native.x/100,point_native.y/100,-point_native.z/100});
                world={world.x+reference.position.x,world.y+reference.position.y,world.z+reference.position.z};
                auto ep=s.eyes[eye].pose.position;auto local=rotate(inverse(s.eyes[eye].pose.orientation),{world.x-ep.x,world.y-ep.y,world.z-ep.z});
                auto got=point(matrices.view[eye],point_native);
                CHECK(approximately(got.x,100*local.x,.001)&&approximately(got.y,100*local.y,.001)&&approximately(got.z,-100*local.z,.001));
            }
            auto p=matrices.projection[eye];auto f=s.eyes[eye].fov;
            CHECK(approximately(std::tan(f.left)*p[0]+p[8],-1)&&approximately(std::tan(f.right)*p[0]+p[8],1));
            CHECK(approximately(std::tan(f.up)*p[5]+p[9],1)&&approximately(std::tan(f.down)*p[5]+p[9],-1));
        }
    }
    std::printf("heading-only initial/recenter/defer/guard/six-axis point-oracle tests passed\n");return 0;
}
static int world_recovery_tests(){
    StereoGate gate;auto current=posed({{1,2,3},axis(1,.2)});
    auto accepted=gate.update(current,current.received_ns);CHECK(accepted);
    CHECK(!gate.advance_world(0)&&!gate.advance_world(-1));
    for(unsigned world=0;world<64;++world){
        const auto minimum=current.received_ns+2000000;
        CHECK(gate.advance_world(minimum));
        CHECK(!gate.advance_world(minimum)&&!gate.advance_world(minimum-1));
        // An identical previous sample and a newer sequence at the exact
        // boundary cannot latch the new reference. Neither is a fatal proof.
        auto old=gate.update(current,minimum);
        CHECK(old.status==StereoStatus::stale&&old.reference_generation==accepted.reference_generation);
        auto boundary=posed({{99,98,97},axis(1,.9)},current.sequence+1);
        boundary.received_ns=minimum;
        auto rejected=gate.update(boundary,minimum);
        CHECK(rejected.status==StereoStatus::stale&&rejected.reference_generation==accepted.reference_generation);
        CHECK(stereo_same_pose(rejected.reference,accepted.reference));
        auto bad=boundary;bad.head.orientation.w=2;
        CHECK(gate.update(bad,minimum).status==StereoStatus::invalid);
        bad=boundary;++bad.session;
        CHECK(gate.update(bad,minimum).status==StereoStatus::session_changed);
        bad=boundary;bad.sequence=current.sequence-1;
        if(bad.sequence)CHECK(gate.update(bad,minimum).status==StereoStatus::replayed);
        // The first eligible sample consumes the recenter exactly once, with
        // the fresh head position rather than any skipped boundary sample.
        current=posed({{double(world+4),5,6},axis(1,-.4)},current.sequence+2);
        current.received_ns=minimum+1;
        auto resumed=gate.update(current,current.received_ns);
        CHECK(resumed&&resumed.recentered&&resumed.reference_generation==accepted.reference_generation+1);
        CHECK(resumed.reference.position.x==current.head.position.x&&resumed.reference.position.y==5);
        CHECK(gate.update(current,current.received_ns).reference_generation==resumed.reference_generation);
        accepted=resumed;
    }
    gate.reset();auto initial=sample();
    CHECK(gate.update(initial,initial.received_ns).status==StereoStatus::initial);
    puts("64 repeated world cutoffs preserve replay guards and recenter only on fresh samples");return 0;
}
int main(){
    if(auto failure=world_recovery_tests())return failure;
    if(auto failure=heading_tests())return failure;
    {
        StereoGate visibility_gate;auto visible=sample();visible.focused=0;visible.visible=1;
        CHECK(stereo_render_active(visible));
        auto first_visible=visibility_gate.update(visible,visible.received_ns);
        CHECK(first_visible&&first_visible.sample.focused==0&&first_visible.sample.visible==1);
        CHECK(visibility_gate.update(visible,visible.received_ns+100000000));
        CHECK(visibility_gate.update(visible,visible.received_ns+100000001).status==StereoStatus::stale);
        auto changed_visible=visible;changed_visible.visible=0;
        CHECK(!stereo_same(visible,changed_visible));
        CHECK(visibility_gate.update(changed_visible,changed_visible.received_ns).status==StereoStatus::replayed);
        auto focused=visible;++focused.sequence;++focused.received_ns;++focused.predicted_time;focused.focused=1;focused.visible=0;
        auto focused_result=visibility_gate.update(focused,focused.received_ns);
        CHECK(focused_result&&focused_result.reference_generation==first_visible.reference_generation);
        auto hidden=focused;++hidden.sequence;++hidden.received_ns;++hidden.predicted_time;hidden.focused=0;
        CHECK(!stereo_render_active(hidden)&&visibility_gate.update(hidden,hidden.received_ns).status==StereoStatus::inactive);
        auto resumed=hidden;++resumed.sequence;++resumed.received_ns;++resumed.predicted_time;resumed.visible=1;
        CHECK(visibility_gate.update(resumed,resumed.received_ns));
        auto no_render=resumed;++no_render.sequence;no_render.should_render=0;
        CHECK(!stereo_render_active(no_render)&&visibility_gate.update(no_render,no_render.received_ns).status==StereoStatus::inactive);
        StereoImageTicket ticket{};ticket.id=1;ticket.sample=visible;ticket.rect[0]={0,0,320,360};ticket.rect[1]={320,0,320,360};
        CHECK(stereo_image_ticket_valid(ticket,640,360));
        ticket.sample=no_render;CHECK(!stereo_image_ticket_valid(ticket,640,360));
        for(unsigned flag=0;flag<3;++flag)for(auto invalid:{2u,(std::numeric_limits<std::uint32_t>::max)()}){
            auto bad=visible;
            if(flag==0)bad.focused=invalid;if(flag==1)bad.visible=invalid;if(flag==2)bad.should_render=invalid;
            CHECK(!stereo_render_active(bad));StereoGate rejected;
            CHECK(rejected.update(bad,bad.received_ns).status==StereoStatus::inactive);
            ticket.sample=bad;CHECK(!stereo_image_ticket_valid(ticket,640,360));
        }
        CHECK(StereoBridgePacket{}.version==4);
    }
    StereoSample zero{};CHECK(zero.head.orientation.w==0&&zero.eyes[0].pose.orientation.w==0);
    CHECK(zero.source==StereoSource::RuntimeTracked);
    auto simulated=sample();simulated.source=StereoSource::MetaOperatorSimulated;simulated.head_flags=3;
    StereoGate strict_sim,allowed_sim;
    CHECK(strict_sim.update(simulated,simulated.received_ns).status==StereoStatus::invalid);
    CHECK(allowed_sim.update(simulated,simulated.received_ns,true));
    CHECK(simulated.head_flags==3&&simulated.view_flags==15);
    CHECK(!allowed_sim.update(simulated,simulated.received_ns)); // Every gate call opts in.
    CHECK(allowed_sim.update(simulated,simulated.received_ns+100000001,true).status==StereoStatus::stale);
    auto wrong_source=simulated;wrong_source.source=static_cast<StereoSource>(99);
    CHECK(!stereo_same(wrong_source,simulated));
    CHECK(allowed_sim.update(wrong_source,wrong_source.received_ns,true).status==StereoStatus::invalid);
    auto switched=simulated;switched.source=StereoSource::RuntimeTracked;switched.head_flags=15;++switched.sequence;
    CHECK(allowed_sim.update(switched,switched.received_ns,true).status==StereoStatus::invalid);
    for(unsigned failure=0;failure<5;++failure){auto bad=simulated;StereoGate validation;
        if(failure==0)bad.head_flags=1;if(failure==1)bad.view_flags=3;
        if(failure==2)bad.head.orientation.w=2;if(failure==3)bad.focused=0;if(failure==4)bad.should_render=0;
        CHECK(!validation.update(bad,bad.received_ns,true));
    }
    auto real_untracked=sample();real_untracked.head_flags=3;StereoGate real_policy;
    CHECK(real_policy.update(real_untracked,real_untracked.received_ns).status==StereoStatus::untracked);
    CHECK(real_policy.update(real_untracked,real_untracked.received_ns,true).status==StereoStatus::invalid);
    auto wrong_profile=sample();CHECK(real_policy.update(wrong_profile,wrong_profile.received_ns,true).status==StereoStatus::invalid);
    StereoMatrices simulated_matrices;
    CHECK(!stereo_matrices(identity(),projection(),Pose{},simulated,100,simulated_matrices));
    CHECK(stereo_matrices(identity(),projection(),Pose{},simulated,100,simulated_matrices,true));
    CHECK(!stereo_matrices(identity(),projection(),Pose{},wrong_source,100,simulated_matrices,true));
    StereoGate gate;auto s=sample();auto first=gate.update(s,s.received_ns);CHECK(first&&first.status==StereoStatus::initial);
    CHECK(gate.update(s,s.received_ns+100000000));CHECK(!gate.update(s,s.received_ns+100000001));
    auto changed=s;changed.sequence=2;changed.head.position.x=.2;
    CHECK(gate.update(changed,changed.received_ns).reference.position.x==0);
    CHECK(gate.update(s,s.received_ns).status==StereoStatus::replayed);
    auto replay=changed;replay.eyes[0].fov.left-=.01;CHECK(gate.update(replay,replay.received_ns).status==StereoStatus::replayed);
    auto backward=changed;backward.sequence=3;--backward.received_ns;CHECK(gate.update(backward,changed.received_ns).status==StereoStatus::replayed);
    backward=changed;backward.sequence=3;--backward.predicted_time;CHECK(gate.update(backward,changed.received_ns).status==StereoStatus::replayed);
    CHECK(!gate.update(changed,changed.received_ns-1));
    auto session=changed;session.session=22;CHECK(gate.update(session,session.received_ns).status==StereoStatus::session_changed);
    gate.reset();CHECK(gate.update(session,session.received_ns).status==StereoStatus::initial);
    // LOCAL-origin changes preserve session/replay guards, replacing the head reference.
    StereoGate origin_gate;CHECK(origin_gate.update(s,s.received_ns));
    auto origin=s;origin.space_epoch=2;origin.sequence=2;++origin.predicted_time;++origin.received_ns;
    origin.head.position={2,1,3};
    auto invalid_origin=origin;invalid_origin.head_flags=0;
    CHECK(origin_gate.update(invalid_origin,invalid_origin.received_ns).status==StereoStatus::untracked);
    auto origin_result=origin_gate.update(origin,origin.received_ns);
    CHECK(origin_result&&origin_result.recentered&&origin_result.reference_generation==2);
    CHECK(origin_result.reference.position.x==2&&origin_result.reference.position.y==1&&origin_result.reference.position.z==3);
    CHECK(origin_gate.update(origin,origin.received_ns).reference_generation==2);
    auto old_origin=origin;old_origin.space_epoch=1;old_origin.sequence=3;
    CHECK(origin_gate.update(old_origin,old_origin.received_ns).status==StereoStatus::replayed);
    old_origin=origin;old_origin.space_epoch=3; // New epoch cannot reuse a sequence.
    CHECK(origin_gate.update(old_origin,old_origin.received_ns).status==StereoStatus::replayed);
    old_origin.sequence=3;old_origin.space_epoch=0;
    CHECK(origin_gate.update(old_origin,old_origin.received_ns).status==StereoStatus::invalid);
    for(unsigned failure=0;failure<8;++failure){StereoGate g;auto bad=s;
        if(failure==0)bad.head.orientation.w=2;
        if(failure==1)bad.eyes[1].pose.position.x=std::numeric_limits<double>::quiet_NaN();
        if(failure==2)bad.eyes[1].pose.position.x=-.032;
        if(failure==3)bad.eyes[0].fov.left=bad.eyes[0].fov.right;
        if(failure==4)bad.focused=0;if(failure==5)bad.should_render=0;
        if(failure==6)bad.head_flags=7;if(failure==7)bad.view_flags=7;
        CHECK(!g.update(bad,bad.received_ns));CHECK(g.update(s,s.received_ns).status==StereoStatus::initial);
    }
    // Nonidentity native game camera including translation; arbitrary reference
    // and independent XR rotations/translation along every axis.
    auto game=identity();auto gr=rotation_matrix(multiply(axis(1,.7),axis(2,-.2)));
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)game[4*i+j]=float(gr[i][j]);
    game[12]=127;game[13]=-83;game[14]=209;
    Pose reference{{1,2,3},multiply(axis(1,-.4),axis(0,.3))};
    for(unsigned a=0;a<3;++a)for(double angle:{-.5,0.,.6}){
        auto current=sample();current.head=reference;
        auto orientation=multiply(reference.orientation,axis(a,angle));
        for(unsigned i=0;i<2;++i){current.eyes[i].pose.orientation=orientation;
            auto offset=rotate(reference.orientation,{(i?.032:-.032)+.12,-.08,.17});
            current.eyes[i].pose.position={reference.position.x+offset.x,reference.position.y+offset.y,reference.position.z+offset.z};}
        StereoMatrices output;auto native_projection=projection();CHECK(stereo_matrices(game,native_projection,reference,current,100,output));
        for(unsigned eye=0;eye<2;++eye){
            for(Vector world: {Vector{2,5,7},Vector{-33,72,140},Vector{100,-200,300}}){
                Vector base=point(game,world);Vector in_ref{base.x/100,base.y/100,-base.z/100};
                Vector world_xr=rotate(reference.orientation,in_ref);world_xr.x+=reference.position.x;world_xr.y+=reference.position.y;world_xr.z+=reference.position.z;
                auto pos=current.eyes[eye].pose.position;
                Vector local=rotate(inverse(current.eyes[eye].pose.orientation),{world_xr.x-pos.x,world_xr.y-pos.y,world_xr.z-pos.z});
                auto observed=point(output.view[eye],world);
                CHECK(approximately(observed.x,local.x*100,.001)&&approximately(observed.y,local.y*100,.001)&&approximately(observed.z,-local.z*100,.001));
            }
            auto p=output.projection[eye];auto f=current.eyes[eye].fov;
            CHECK(approximately(std::tan(f.left)*p[0]+p[8],-1));CHECK(approximately(std::tan(f.right)*p[0]+p[8],1));
            CHECK(approximately(std::tan(f.up)*p[5]+p[9],1));CHECK(approximately(std::tan(f.down)*p[5]+p[9],-1));
            for(unsigned index=0;index<16;++index)if(index!=0&&index!=5&&index!=8&&index!=9)CHECK(p[index]==native_projection[index]);
        }
    }
    StereoMatrices output;CHECK(stereo_matrices(identity(),projection(),Pose{},s,output));
    CHECK(approximately(output.view[0][12],3.2)&&approximately(output.view[1][12],-3.2));
    auto before=output;auto invalid=identity();invalid[0]=2;CHECK(!stereo_matrices(invalid,projection(),Pose{},s,100,output));CHECK(output.view[0]==before.view[0]);
    invalid=identity();invalid[0]=-1;CHECK(!stereo_matrices(invalid,projection(),Pose{},s,100,output));
    invalid=identity();invalid[12]=1e9f;CHECK(!stereo_matrices(invalid,projection(),Pose{},s,100,output));
    auto badp=projection();badp[11]=-1;CHECK(!stereo_matrices(identity(),badp,Pose{},s,100,output));
    CHECK(!stereo_matrices(identity(),projection(),Pose{},s,9,output));CHECK(!stereo_matrices(identity(),projection(),Pose{},s,501,output));
    // Independent PID transport, single producer, corruption and contention.
    DWORD pid=GetCurrentProcessId();StereoBridge producer,reader,duplicate;
    CHECK(producer.open(pid,true));CHECK(!duplicate.open(pid,true));CHECK(reader.open(pid,false));
    CHECK(producer.publish(s));StereoSample received;CHECK(reader.read(received)&&stereo_same(received,s));
    wchar_t map_name[100],mutex_name[100];swprintf_s(map_name,L"Local\\GearsVR.Stereo.%lu",pid);swprintf_s(mutex_name,L"Local\\GearsVR.StereoLock.%lu",pid);
    HANDLE mapping=OpenFileMappingW(FILE_MAP_WRITE,FALSE,map_name);CHECK(mapping);
    auto packet=static_cast<StereoBridgePacket*>(MapViewOfFile(mapping,FILE_MAP_WRITE,0,0,sizeof(StereoBridgePacket)));CHECK(packet);
    auto original=*packet;
    for(unsigned corrupt=0;corrupt<7;++corrupt){*packet=original;if(corrupt==0)packet->magic=0;if(corrupt==1)packet->version=1;if(corrupt==2)--packet->size;if(corrupt==3)++packet->target_pid;if(corrupt==4)++packet->frequency;
        if(corrupt==5)packet->sample.source=static_cast<StereoSource>(99);if(corrupt==6)packet->version=3;CHECK(!reader.read(received));}
    *packet=original;CHECK(producer.publish(simulated));CHECK(reader.read(received)&&stereo_same(received,simulated)&&received.head_flags==3);
    CHECK(!producer.publish(wrong_source));CHECK(producer.publish(s));
    *packet=original;HANDLE lock=OpenMutexW(SYNCHRONIZE|MUTEX_MODIFY_STATE,FALSE,mutex_name);CHECK(lock);
    HANDLE held=CreateEventW(nullptr,TRUE,FALSE,nullptr),release=CreateEventW(nullptr,TRUE,FALSE,nullptr);CHECK(held&&release);
    std::thread blocker([&]{WaitForSingleObject(lock,INFINITE);SetEvent(held);WaitForSingleObject(release,INFINITE);ReleaseMutex(lock);});
    CHECK(WaitForSingleObject(held,1000)==WAIT_OBJECT_0);bool blocked_read=!reader.read(received),blocked_write=!producer.publish(s);SetEvent(release);blocker.join();CHECK(blocked_read&&blocked_write);
    CloseHandle(held);CloseHandle(release);CloseHandle(lock);UnmapViewOfFile(packet);CloseHandle(mapping);
    reader.close();producer.close();CHECK(!reader.open(pid,false));
    // Mapping-only collision is rejected even when no named mutex exists.
    DWORD orphan_pid=pid+1000000;swprintf_s(map_name,L"Local\\GearsVR.Stereo.%lu",orphan_pid);
    HANDLE orphan=CreateFileMappingW(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,0,sizeof(StereoBridgePacket),map_name);CHECK(orphan);
    CHECK(!producer.open(orphan_pid,true));CloseHandle(orphan);
    std::printf("stereo frame/gate/matrix/bridge tests passed\n");return 0;
}
