// Retail reference/C0 fixture; target/source ownership below is an explicit
// CPU seam. This does not exercise the game, collision or a Native write.
#define main PriorBodyGoalAdapterMain
#include "body_goal_view_adapter_test.cpp"
#undef main
#include "cover_c0_target_adapter.h"
namespace ng=gears_cover_neutral_goal;
namespace c0=gears_cover_c0_target;
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
    ng::Result fitted{};
    C(ng::prepare(calibrated,placement,source,*model,*frame,marker,policy,{.25},*work,fitted)==ng::Status::okay);
    if(failures)return 1;
    v::CoverBodyTargetKey key{};key.epochs={1,2,3,4,5,6,7};
    key.world=source.world.address;key.pawn=source.pawn.address;key.mesh=source.mesh.address;key.link=source.cover_link.address;
    key.world_generation=source.world_generation;key.slot=source.cover_slot;key.thread=source.native.game_thread;
    key.scene=source.scene_invocation;key.xr_session=source.stereo.session;key.xr_space=source.stereo.space_epoch;
    key.reference=source.reference_generation;key.sequence=source.stereo.sequence;
    c0::Prepared out{};out.target_observed_ns=777;const auto sentinel=out;
    for(v::CoverOffset held:{v::CoverOffset{},v::CoverOffset{.02,.01}}){
        v::CoverViewResult view{};v::StereoResult stereo{};stereo.status=v::StereoStatus::accepted;
        stereo.sample=source.stereo;stereo.reference=source.reference;stereo.reference_generation=source.reference_generation;
        C(v::prepare_cover_view(placement.anchor,placement.view_anchor,held,source.units_per_metre,view));
        v::CoverBodyTarget target{};
        C(v::prepare_cover_body_target(key,key,placement.anchor,placement.view_anchor,view,held,stereo,policy.now_ns,source.units_per_metre,target,true));
        const auto frozen_before=bytes(fitted);
        const auto source_before=bytes(source);
        const auto frame_before=bytes(*frame);
        const auto target_before=bytes(target);
        const auto code=c0::prepare(fitted,placement,source,key,target,*model,*frame,marker,policy,out);
        if(code!=c0::Code::prepared)printf("C0 target code=%u\n",unsigned(code));
        C(code==c0::Code::prepared);if(code!=c0::Code::prepared)continue;
        a::Output expected{};C(ng::convert(fitted,placement,source,*model,*frame,marker,held,policy,expected)==ng::Status::okay);
        compare(out.body,expected);C(v::same(out.key,key));
        C(out.target_observed_ns==target.observed_ns&&out.tracking_received_ns==source.stereo.received_ns);
        C(bytes(fitted)==frozen_before&&bytes(source)==source_before&&bytes(*frame)==frame_before&&bytes(target)==target_before);
        auto fail=[&](auto const& current,auto const& value,auto const& selected,auto const& where,auto const& rules){
            out=sentinel;
            C(c0::prepare(fitted,where,selected,current,value,*model,*frame,marker,rules,out)!=c0::Code::prepared);
            C(bytes(out)==bytes(sentinel));
        };
        for(unsigned bad=0;bad<13;++bad){auto k=key;auto t=target;
            if(bad==0)++k.world;if(bad==1)++k.pawn;if(bad==2)++k.mesh;if(bad==3)++k.link;
            if(bad==4)++k.world_generation;if(bad==5)++k.slot;if(bad==6)++k.thread;if(bad==7)++k.scene;
            if(bad==8)++k.xr_session;if(bad==9)++k.xr_space;if(bad==10)++k.reference;if(bad==11)++k.sequence;
            if(bad==12)++t.key.epochs.slot;else t.key=k;
            fail(k,t,source,placement,policy);
        }
        for(unsigned bad=0;bad<12;++bad){auto t=target;
            if(bad==0)t.pose_applied=true;if(bad==1)t.head_bone_goal_valid=true;
            if(bad==2)t.same_pose_epoch_verified=true;if(bad==3)t.collision_verified=true;
            if(bad==4)++t.desired_eye_world.x;if(bad==5)++t.physical.up;if(bad==6)++t.desired_right_metres;
            if(bad==7)t.tracking.sample.head.position.x+=.1f;if(bad==8)++t.tracking.reference.position.x;
            if(bad==9)t.observed_ns=t.tracking.sample.received_ns-1;
            if(bad==10)t.committed_view.base_view[12]+=.1f;if(bad==11)t.virtual_offset.up+=.1;
            fail(key,t,source,placement,policy);
        }
        auto stale=policy;stale.now_ns+=100000001;fail(key,target,source,placement,stale);
        auto changed=placement;changed.anchor.neutral_eye_world.x+=1;fail(key,target,source,changed,policy);
        auto later=policy;++later.now_ns;
        C(c0::prepare(fitted,placement,source,key,target,*model,*frame,marker,later,out)==c0::Code::prepared);
        C(out.target_observed_ns==target.observed_ns&&out.tracking_received_ns==source.stereo.received_ns);
        auto& alias=*reinterpret_cast<c0::Prepared*>(&fitted);
        C(c0::prepare(fitted,placement,source,key,target,*model,*frame,marker,policy,alias)==c0::Code::alias);
        C(bytes(fitted)==frozen_before);
    }
    // Independent current Tick tracking must not be replaced by the older
    // immutable Scene sample, even when that Scene remains the identity parent.
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
    ng::Result current_neutral{};
    C(ng::prepare(calibrated,current_placement,input,*model,*frame,marker,current_policy,{.25},*work,current_neutral)==ng::Status::okay);
    auto current_key=key;current_key.sequence=tick.stereo.sequence;
    v::CoverViewResult view{};v::StereoResult stereo{};stereo.status=v::StereoStatus::accepted;
    stereo.sample=tick.stereo;stereo.reference=source.reference;stereo.reference_generation=source.reference_generation;
    C(v::prepare_cover_view(placement.anchor,placement.view_anchor,{},source.units_per_metre,view));
    v::CoverBodyTarget target{};
    C(v::prepare_cover_body_target(current_key,current_key,placement.anchor,placement.view_anchor,view,{},stereo,now,source.units_per_metre,target,true));
    const auto input_before=bytes(input);
    C(c0::prepare(current_neutral,current_placement,input,current_key,target,*model,*frame,marker,current_policy,out)==c0::Code::prepared);
    C(out.tracking_received_ns==tick.stereo.received_ns&&out.key.sequence==tick.stereo.sequence);
    C(bytes(input)==input_before);
    auto wrong=target;wrong.tracking.sample=source.stereo;out=sentinel;
    C(c0::prepare(current_neutral,current_placement,input,current_key,wrong,*model,*frame,marker,current_policy,out)==c0::Code::key);
    C(bytes(out)==bytes(sentinel));
    C(c0::prepare(current_neutral,current_placement,source,current_key,target,*model,*frame,marker,current_policy,out)==c0::Code::source);
    C(bytes(out)==bytes(sentinel));
    printf("CoverC0Target: %u checks, %u failures; math only, no current placement/Native/collision permission\n",checks,failures);
    return failures?1:0;
}
