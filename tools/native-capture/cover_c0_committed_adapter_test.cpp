// Retail skeletal bytes with explicit synthetic Core/V3 admission. No game,
// input injection, collision check or Native writer is executed by this test.
#define main PriorBodyGoalAdapterMain
#include "body_goal_view_adapter_test.cpp"
#undef main
#include "cover_c0_committed_adapter.h"
namespace ng=gears_cover_neutral_goal;
namespace cc=gears_cover_c0_committed;
namespace committed=gears_cover_committed;
int main(int argc,char** argv){
    if(argc!=2)return 2;const std::string folder=argv[1];
    auto frame=std::make_unique<b::Frame>();auto model=std::make_unique<b::Model>();
    auto marker_work=std::make_unique<mount::Workspace>();auto work=std::make_unique<ng::Workspace>();
    mount::Marker marker{};a::core::CoreSnapshot source{};a::Neutral calibrated{};
    C(read(folder+"/source-CT0.bin",&source,sizeof source));
    C(read(folder+"/calibrated-neutral.bin",&calibrated,sizeof calibrated));
    C(read(folder+"/reference.bin",frame->reference.data(),sizeof frame->reference));
    C(read(folder+"/C0-local.bin",frame->local.data(),sizeof frame->local));
    C(read(folder+"/C0-component.bin",&frame->component_to_world,sizeof frame->component_to_world));
    frame->units_per_metre=source.units_per_metre;
    C(b::prepare_model(frame->reference,digest,*model));
    C(mount::prepare(*model,*frame,mount::Basis::marcus_mesh_face_forward_y,*marker_work,marker)==mount::Status::okay);
    source.flags|=a::core::rig::cover_key_known;source.cover_type=2;source.cover_slot=0;source.cover_link=source.world;
    a::Policy policy{};policy.now_ns=int64_t(source.published_ns);policy.allow_sim=true;
    policy.scope=b::Scope::fixed_cover;policy.posture=b::Posture::preserve_c0;
    a::CoverPoseMeasurement measured{};
    C(a::measure_cover_pose(source,*model,*frame,marker,policy,work->fitting.body_work,measured)==a::Status::okay);
    auto placement=measured.binding;
    const double theta=-.638;
    placement.anchor={measured.eye.position,{std::cos(theta),std::sin(theta),0},{-std::sin(theta),std::cos(theta),0}};
    placement.view_anchor={measured.eye.position,{std::cos(-.45),std::sin(-.45),0},{-std::sin(-.45),std::cos(-.45),0}};
    for(unsigned i=0;i<2;++i)placement.feet_world[i]=measured.feet[i];

    committed::State held{};
    held.key.epochs={1,2,3,4,5,6,7};held.key.world=source.world.address;
    held.key.pawn=source.pawn.address;held.key.mesh=source.mesh.address;held.key.link=source.cover_link.address;
    held.key.world_generation=source.world_generation;held.key.slot=source.cover_slot;held.key.thread=source.native.game_thread;
    held.key.scene=source.scene_invocation;held.key.xr_session=source.stereo.session;held.key.xr_space=source.stereo.space_epoch;
    held.key.reference=source.reference_generation;held.key.sequence=source.stereo.sequence;
    held.pawn=source.pawn;held.mesh=source.mesh;held.link=source.cover_link;
    held.anchor=placement.anchor;held.view_anchor=placement.view_anchor;
    held.range={-.7,.7,0,1};held.offset={.02,.01};held.reference=source.reference;
    held.tracking_source=source.stereo.source;held.units_per_metre=source.units_per_metre;held.type=2;
    held.observed_ns=std::int64_t(source.published_ns);
    cc::Frozen frozen{};
    C(cc::freeze(held,calibrated,placement,source,*model,*frame,marker,policy,{.25},*work,frozen)==cc::Code::prepared);
    cc::Prepared out{};C(cc::prepare(frozen,held,placement,source,*model,*frame,marker,policy,out)==cc::Code::prepared);
    a::Output expected{};C(ng::convert(frozen.neutral,placement,source,*model,*frame,marker,held.offset,policy,expected)==ng::Status::okay);
    compare(out.body,expected);C(out.interaction_observed_ns==held.observed_ns&&out.tracking_received_ns==source.stereo.received_ns);
    namespace src=gears_body_tick_source;
    src::Input input{};input.scene_cache_revision=2;
    const auto now=std::int64_t(source.published_ns)+1000000000;
    input.metadata_begin_ns=now-10000000;input.metadata_end_ns=now-8000000;
    auto& tick=input.observed;tick.parent_scene=source;tick.tick=1;tick.thread=source.native.game_thread;tick.parent_cookie=123;
    tick.begin_ns=now-7000000;tick.end_ns=now-3000000;
    tick.stereo=source.stereo;tick.stereo.sequence+=5;tick.stereo.received_ns=now-4000000;tick.stereo.predicted_time+=1000000000;
    tick.controllers=source.controllers;tick.controllers.sequence=tick.stereo.sequence;
    tick.controllers.predicted_time=tick.stereo.predicted_time;
    tick.controllers.received_ns=now-5000000;tick.controllers.sync_ns=now-6000000;
    tick.continuity.input=tick.controllers;tick.continuity.stream_epoch=source.producer.stream;
    for(unsigned i=0;i<2;++i)tick.continuity.tracking_epoch[i]=source.producer.tracking[i];
    tick.reference_revision=source.native_reference_revision;tick.native_reference=source.native_reference;
    tick.publisher_pid=source.controller_publisher_pid;tick.publisher_start=source.controller_publisher_start;
    auto current_policy=policy;current_policy.now_ns=now;
    auto current_placement=placement;current_placement.observed_ns=input.metadata_end_ns;
    C(src::classify(input,source,now,true)==src::Verdict::okay);
    C(!a::core::core_snapshot_valid(source,now,a::maximum_age_ns,true));

    const v::Vector physical{.04,.03,-.02};
    tick.stereo.head.position=plus(source.reference.position,rotate(source.reference.orientation,physical));
    C(src::classify(input,source,now,true)==src::Verdict::okay);
    const auto state_before=bytes(held);const auto input_before=bytes(input);const auto frozen_before=bytes(frozen);
    C(cc::prepare(frozen,held,current_placement,input,*model,*frame,marker,current_policy,out)==cc::Code::prepared);
    C(out.tracking_sequence==tick.stereo.sequence&&out.tracking_sequence!=held.key.sequence);
    C(out.tracking_received_ns==tick.stereo.received_ns&&out.interaction_observed_ns==held.observed_ns);
    const auto physical_world=map(frozen.neutral.neutral,physical,source.units_per_metre);
    const auto virtual_world=plus(times(held.anchor.right_world,held.offset.right*source.units_per_metre),
        {0,0,held.offset.up*source.units_per_metre});
    C(distance(out.body.goal.physical_world,physical_world)<1e-8);
    C(distance(out.body.goal.committed_grip_world,virtual_world)<1e-8);
    C(distance(out.body.desired_eye.position,plus(frozen.neutral.neutral.eye_anchor.neutral_eye_world,
        plus(physical_world,virtual_world)))<.002); // Current physical motion applied exactly once.
    C(now-held.observed_ns>=1000000000); // Persistent offset is not a tracking sample.
    C(ng::convert(frozen.neutral,current_placement,input,*model,*frame,marker,held.offset,current_policy,expected)==ng::Status::okay);
    compare(out.body,expected);
    C(bytes(held)==state_before&&bytes(input)==input_before&&bytes(frozen)==frozen_before);
    cc::Prepared sentinel{};sentinel.interaction_observed_ns=123;
    for(unsigned reason=0;reason<12;++reason){
        auto bad=held;
        if(reason==0)++bad.pawn.name_index;
        if(reason==1)++bad.mesh.name_index;
        if(reason==2)++bad.link.name_index;
        if(reason==3)++bad.key.world_generation;
        if(reason==4)++bad.key.slot;
        if(reason==5)++bad.key.reference;
        if(reason==6)++bad.key.xr_space;
        if(reason==7)++bad.key.epochs.cover;
        if(reason==8)bad.offset.right=1;
        if(reason==9)bad.observed_ns=now+1;
        if(reason==10)bad.reference.position.x+=1;
        if(reason==11)bad.type=1; // high cover cannot carry this popup range/offset.
        out=sentinel;
        C(cc::prepare(frozen,bad,current_placement,input,*model,*frame,marker,current_policy,out)!=cc::Code::prepared);
        C(bytes(out)==bytes(sentinel));
    }
    auto stale=current_policy;stale.now_ns+=50000001;out=sentinel;
    C(cc::prepare(frozen,held,current_placement,input,*model,*frame,marker,stale,out)!=cc::Code::prepared);
    C(bytes(out)==bytes(sentinel));
    auto lost=input;lost.observed.stereo.head_flags=0;out=sentinel;
    C(cc::prepare(frozen,held,current_placement,lost,*model,*frame,marker,current_policy,out)!=cc::Code::prepared);
    C(bytes(out)==bytes(sentinel));
    // Stale Core never becomes current merely because the held offset is valid.
    C(cc::prepare(frozen,held,placement,source,*model,*frame,marker,current_policy,out)!=cc::Code::prepared);
    C(bytes(out)==bytes(sentinel));
    std::printf("CoverC0CommittedAdapter checks=%u failures=%u; persistent virtual offset with current admitted tracking only\n",checks,failures);
    return failures?1:0;
}
