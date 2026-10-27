#include "hand_rig_ticket_policy.h"
#include "rig_tracking_reader.h"
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <type_traits>
#include <utility>

using namespace gears_hand_rig;
namespace {
unsigned checks{},scenarios{};
#define CHECK(x) do {++checks;if(!(x)){std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);std::abort();}} while(false)
using LegacyPlan=gears_hand_stream::Plan;
static_assert(!std::is_invocable_v<decltype(&RigPolicy::commit_binding),RigPolicy&,LegacyPlan const&,unsigned>);
static_assert(!std::is_invocable_v<decltype(&RigPolicy::authorize),RigPolicy&,LegacyPlan const&,RigContext const&,Frame const*,std::uint64_t,std::int64_t>);
static_assert(noexcept(std::declval<RigPolicy&>().observe(RigContext{},nullptr,0,0)));
static_assert(noexcept(std::declval<RigPolicy&>().authorize(RigPlan{},RigContext{},nullptr,0,0)));
gears_weapon_hand::ObjectIdentity object(std::uint64_t address,std::uint32_t index) {
    return {address,0x8000,0x9000,index,index+10,0,1};
}
RigBinding binding(unsigned source=0) {return {{21,31,41,51,61},source,71,81};}
RigContext context(std::uint64_t ms=1000) {
    RigContext c{};c.objects={object(0x1000,1),object(0x1100,2),object(0x1200,3),object(0x1300,4),{}};
    c.game={0x1000,0x1100,0x1200,0x1300,0};c.expected_native_world_generation=7;
    c.action_epoch=1;c.observed_ms=ms;c.idle_ready=c.clean=true;c.game_thread=31;c.input_revision=1;return c;
}
Frame frame(RigContext const& c,std::int64_t ns=2000000000,unsigned source=0) {
    Frame f{};f.status=gears_rig_tracking::Status::ready;
    f.flags=gears_rig_tracking::controllers_matched|gears_rig_tracking::producer_continuity_available;
    f.native=binding(source).native;f.publication_sequence=1;f.scene_invocation=10;f.world_generation=7;
    f.binding_generation=1;f.reference_generation=1;f.published_ns=std::uint64_t(ns);
    f.scene_qpc=10000000;f.qpc_frequency=10000000;f.observed={1,1,1,{1,1}};f.producer={1,{1,1}};
    f.native_reference={1,gears_rig_tracking::reference_stable,0};
    f.controller_publisher_pid=71;f.controller_publisher_start=81;f.view_policy=gears_rig_tracking::first_person;
    f.world=c.objects.world;f.controller=c.objects.controller;f.pawn=c.objects.pawn;
    f.mesh=c.objects.mesh;f.weapon=c.objects.weapon;f.units_per_metre=100;
    f.source_view={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};f.base_view=f.source_view;
    auto& s=f.stereo;s.source=static_cast<gears_vr::StereoSource>(source);s.session=91;s.sequence=1;
    s.received_ns=ns-2000000;s.predicted_time=100000;s.head_flags=s.view_flags=15;
    s.focused=s.should_render=s.visible=1;s.head={};s.space_epoch=1;
    for(unsigned i=0;i<2;++i){s.eyes[i].pose={};s.eyes[i].pose.position.x=i?.032:-.032;
        s.eyes[i].fov={-.7,.7,.7,-.7};}
    auto& input=f.controllers;input.source=s.source;input.session=s.session;input.sequence=s.sequence;
    input.space_epoch=s.space_epoch;input.profile_generation=input.input_epoch=1;input.predicted_time=s.predicted_time;
    input.received_ns=ns-3000000;input.sync_ns=ns-4000000;input.focused=1;
    for(auto& h:input.hands){h.grip_active=h.aim_active=1;h.grip_flags=h.aim_flags=15;}
    f.world_pose_mask=7;
    CHECK(gears_vr::rig_tracking::pose(f.base_view,f.reference,s.head,f.units_per_metre,f.world_head));
    for(unsigned i=0;i<2;++i)CHECK(gears_vr::rig_tracking::pose(f.base_view,f.reference,input.hands[i].grip,f.units_per_metre,f.world_grip[i]));
    return f;
}
struct Live {
    RigPolicy policy{};RigContext ctx=context();Frame receipt{};RigPlan plan{};
    std::uint64_t ms=1000;std::int64_t ns=2000000000;
    explicit Live(bool bind=true,unsigned source=0) {
        CHECK(policy.enable(binding(source)));receipt=frame(ctx,ns,source);
        CHECK(gears_rig_tracking::frame_valid(receipt,ns,100000000,source==1));
        plan=policy.observe(ctx,&receipt,ms,ns);
        CHECK(plan.baseline_mask==3&&!plan.accept_mask&&plan.restore_mask==3);
        CHECK(!policy.commit_binding(plan,3));
        if(bind){advance();CHECK(plan.accept_mask==3&&plan.bind_mask==3);CHECK(policy.commit_binding(plan,3));CHECK(auth()==3);}
    }
    void next(bool raw=true) {
        ms+=10;ns+=10000000;ctx.observed_ms=ms;
        ++receipt.publication_sequence;++receipt.scene_invocation;receipt.scene_qpc+=100000;
        receipt.published_ns=std::uint64_t(ns);
        if(raw){++receipt.stereo.sequence;receipt.stereo.received_ns=ns-2000000;receipt.stereo.predicted_time+=10000000;
            receipt.controllers.sequence=receipt.stereo.sequence;receipt.controllers.predicted_time=receipt.stereo.predicted_time;
            receipt.controllers.received_ns=ns-3000000;receipt.controllers.sync_ns=ns-4000000;}
    }
    void observe() {plan=policy.observe(ctx,&receipt,ms,ns);}
    void advance(bool raw=true) {next(raw);observe();}
    unsigned auth() {return policy.authorize(plan,ctx,&receipt,ms,ns);}
};
void baseline_binding_and_type_barrier() {
    ++scenarios;Live l(false);CHECK(l.plan.has_frame&&l.plan.has_ticket&&l.plan.raw_fresh);
    CHECK(same_frame(l.plan.frame,l.receipt));CHECK(l.plan.source_age_ns[0]==2000000&&l.plan.source_age_ns[1]==3000000);
    CHECK(l.plan.ticket.created_ms==l.ms&&l.plan.observed_ns==l.ns);
    CHECK(!l.plan.ticket.certificate.native_started_qpc&&!l.plan.ticket.certificate.stream_epoch);
    CHECK(!l.policy.enable(binding()));CHECK(l.auth()==0);l.advance();
    CHECK(l.plan.accept_mask==3&&l.plan.bind_mask==3&&l.auth()==0);
    CHECK(!l.policy.commit_binding(l.plan,0)&&!l.policy.commit_binding(l.plan,4));
    CHECK(l.policy.commit_binding(l.plan,1)&&l.auth()==1);CHECK(!l.policy.commit_binding(l.plan,1));
    CHECK(l.policy.commit_binding(l.plan,2)&&l.auth()==3);
    CHECK(l.receipt.scene_invocation!=l.ms); // No current PlayerTick/Scene equivalence is required.
    CHECK(l.plan.frame.world_grip[0].orientation.x==l.receipt.world_grip[0].orientation.x);
    CHECK(l.plan.frame.world_grip[0].orientation.w==l.receipt.world_grip[0].orientation.w);
}
void no_read_and_old_plan_revocation() {
    ++scenarios;Live l;const auto old=l.plan;
    l.plan=l.policy.observe(l.ctx,nullptr,l.ms,l.ns);
    CHECK(!l.plan.has_frame&&!l.plan.has_ticket&&l.plan.restore_mask==3&&!l.plan.reset_binding_mask);
    CHECK(l.policy.bound_mask()==3&&l.auth()==0);CHECK(l.policy.authorize(old,l.ctx,&l.receipt,l.ms,l.ns)==0);
    CHECK(!l.policy.commit_binding(old,1));l.advance();CHECK(l.auth()==3);
    l.plan=l.policy.observe(l.ctx,&l.receipt,l.ms,l.ns);CHECK(!l.plan.raw_fresh&&l.plan.restore_mask==3);
    CHECK(!l.policy.failed()&&l.policy.bound_mask()==3);
}
void scene_duplicate_raw_and_boundary_first() {
    ++scenarios;Live l;const auto old_stamp=l.policy.continuity_snapshot(l.ms).source_stamp[0];
    const auto raw=l.receipt.stereo.sequence;l.advance(false);
    CHECK(l.policy.publication_cursor()==3&&l.policy.raw_sequence_cursor()==raw);
    CHECK(!l.plan.raw_fresh&&!l.plan.has_ticket&&!l.plan.accept_mask&&l.plan.restore_mask==3);
    CHECK(l.policy.bound_mask()==3&&!l.policy.failed()&&!l.plan.reset_binding_mask);
    CHECK(l.policy.continuity_snapshot(l.ms).source_stamp[0]==old_stamp);
    l.next(false);++l.receipt.native_reference.epoch;++l.receipt.reference_generation;
    l.receipt.reference.position.x=.04;l.observe();
    CHECK(!l.policy.failed()&&!l.plan.accept_mask&&l.plan.reset_binding_mask==3&&l.policy.bound_mask()==0);
    l.advance();CHECK(l.plan.baseline_mask==3&&!l.plan.accept_mask);
    l.advance();CHECK(l.plan.bind_mask==3&&l.policy.commit_binding(l.plan,3)&&l.auth()==3);
}
void duplicate_scene_tracks_per_hand_boundaries() {
    ++scenarios;Live l;l.next(false);++l.receipt.producer.tracking[0];l.observe();
    CHECK(!l.policy.failed()&&l.policy.bound_mask()==2&&l.plan.reset_binding_mask==1&&!l.plan.accept_mask);
    l.advance();CHECK(l.plan.baseline_mask==1&&l.plan.accept_mask==2&&l.auth()==2);
    l.advance();CHECK(l.plan.bind_mask==1&&l.policy.commit_binding(l.plan,1)&&l.auth()==3);
    l.next(false);++l.receipt.observed.hand_tracking[1];l.observe();
    CHECK(l.policy.bound_mask()==1&&l.plan.reset_binding_mask==2);
}
void raw_rollback_mutation_and_publication_replay() {
    ++scenarios;
    {Live l;l.next(false);--l.receipt.stereo.sequence;l.receipt.controllers.sequence=l.receipt.stereo.sequence;l.observe();CHECK(l.policy.failed()&&l.plan.restore_mask==3);}
    {Live l;l.next(false);l.receipt.controllers.hands[0].grip.position.x=.04;l.observe();CHECK(l.policy.failed());}
    {Live l;l.next(false);l.receipt.stereo.head.position.x=.01;l.observe();CHECK(l.policy.failed());}
    {Live l;l.receipt.base_view[12]=1;l.observe();CHECK(l.policy.failed());}
    {Live l;--l.receipt.publication_sequence;l.observe();CHECK(l.policy.failed());}
    {Live l;l.next();--l.receipt.scene_invocation;l.observe();CHECK(l.policy.failed());}
    {Live l;l.next(false);++l.receipt.producer.stream;l.receipt.stereo.sequence=1;l.receipt.controllers.sequence=1;l.observe();CHECK(l.policy.failed());}
}
void session_restart_requires_real_producer_boundary() {
    ++scenarios;
    {Live l;l.next();++l.receipt.stereo.session;l.receipt.controllers.session=l.receipt.stereo.session;
        l.receipt.stereo.sequence=l.receipt.controllers.sequence=1;l.observe();CHECK(l.policy.failed());}
    {Live l;l.next();++l.receipt.stereo.session;l.receipt.controllers.session=l.receipt.stereo.session;
        l.receipt.stereo.sequence=l.receipt.controllers.sequence=1;++l.receipt.producer.stream;l.observe();
        CHECK(!l.policy.failed()&&l.plan.baseline_mask==3&&!l.plan.accept_mask);l.advance();CHECK(l.plan.bind_mask==3);}
}
void actual_reference_and_observed_epochs() {
    ++scenarios;
    {Live l;l.next();++l.receipt.reference_generation;l.observe();CHECK(l.policy.failed());}
    {Live l;l.next();l.receipt.reference.position.x=.04;l.observe();CHECK(l.policy.failed());}
    {Live l;l.next();++l.receipt.native_reference.epoch;l.observe();CHECK(!l.policy.failed()&&l.plan.baseline_mask==3);}
    {Live l;l.next();++l.receipt.observed.reference;l.observe();CHECK(!l.policy.failed()&&l.plan.baseline_mask==3);}
    {Live l;l.next();++l.receipt.observed.head_tracking;l.observe();CHECK(l.plan.baseline_mask==3);}
    {Live l;l.next();++l.receipt.observed.stream;l.observe();CHECK(l.plan.baseline_mask==3);}
    {Live l;l.next();l.receipt.native_reference.flags=gears_rig_tracking::reference_pending;l.observe();CHECK(l.policy.failed());}
    {Live l;l.next();l.receipt.native_reference.flags=gears_rig_tracking::reference_stable|gears_rig_tracking::reference_exhausted;l.observe();CHECK(l.policy.failed());}
}
void tracking_loss_each_hand() {
    ++scenarios;
    for(unsigned hand=0;hand<2;++hand){Live l;const auto bit=1u<<hand;l.next();
        l.receipt.controllers.hands[hand].grip_flags=0;l.receipt.world_pose_mask&=~(2u<<hand);
        ++l.receipt.producer.tracking[hand];++l.receipt.observed.hand_tracking[hand];l.observe();
        CHECK(l.plan.reset_binding_mask==bit&&l.plan.accept_mask==(3u&~bit)&&l.auth()==(3u&~bit));
        l.next();l.receipt.controllers.hands[hand].grip_flags=15;l.receipt.world_pose_mask|=2u<<hand;l.observe();
        CHECK(l.plan.accept_mask==3&&l.plan.bind_mask==bit&&l.policy.commit_binding(l.plan,bit)&&l.auth()==3);
    }
}
void global_scale_binding_input_and_cohort_boundaries() {
    ++scenarios;
    {Live l;l.next();l.receipt.units_per_metre=99;l.observe();CHECK(l.plan.baseline_mask==3&&l.plan.reset_binding_mask==3);}
    {Live l;l.next();++l.receipt.binding_generation;l.observe();CHECK(l.plan.baseline_mask==3);}
    {Live l;l.next();++l.ctx.input_revision;l.observe();CHECK(l.plan.baseline_mask==3);}
    {Live l;l.next();++l.ctx.action_epoch;l.observe();CHECK(l.plan.baseline_mask==3);}
    {Live l;l.next();++l.ctx.objects.mesh.registry_index;l.receipt.mesh=l.ctx.objects.mesh;l.observe();CHECK(l.plan.baseline_mask==3);}
    {Live l;l.next();++l.receipt.controllers.profile_generation;++l.receipt.producer.stream;l.observe();CHECK(l.plan.baseline_mask==3);}
    {Live l;l.next();++l.receipt.controllers.input_epoch;++l.receipt.producer.stream;l.observe();CHECK(l.plan.baseline_mask==3);}
    {Live l;l.next();++l.receipt.stereo.space_epoch;l.receipt.controllers.space_epoch=l.receipt.stereo.space_epoch;
        ++l.receipt.producer.stream;l.observe();CHECK(l.plan.baseline_mask==3);}
}
void ownership_and_source_rejections() {
    ++scenarios;
    auto bad=[](auto mutate){Live l;l.next();mutate(l);l.observe();CHECK(l.policy.failed()&&l.plan.restore_mask==3&&!l.plan.accept_mask);};
    bad([](Live& l){++l.receipt.native.registration_cookie;});
    bad([](Live& l){++l.receipt.native.native_started_qpc;});
    bad([](Live& l){++l.receipt.native.game_pid;});bad([](Live& l){++l.receipt.native.game_start;});
    bad([](Live& l){++l.receipt.native.game_thread;});bad([](Live& l){++l.ctx.game_thread;});
    bad([](Live& l){++l.receipt.controller_publisher_pid;});bad([](Live& l){++l.receipt.controller_publisher_start;});
    bad([](Live& l){++l.receipt.mesh.registry_index;});bad([](Live& l){l.receipt.mesh.klass+=8;});
    bad([](Live& l){++l.receipt.world_generation;});
    bad([](Live& l){l.receipt.stereo.source=l.receipt.controllers.source=gears_vr::StereoSource::MetaOperatorSimulated;});
    bad([](Live& l){l.receipt.flags&=~gears_rig_tracking::producer_continuity_available;l.receipt.producer={};});
    {Live simulated(true,1);CHECK(simulated.auth()==3);}
}
void fresh_source_ns_boundaries() {
    ++scenarios;
    for(unsigned source=0;source<2;++source){
        for(std::int64_t age:{99999999LL,100000000LL,100000001LL}){
            Live l(false);l.next();
            if(source==0)l.receipt.stereo.received_ns=l.ns-age;
            else {l.receipt.controllers.received_ns=l.ns-age;l.receipt.controllers.sync_ns=l.ns-age;}
            // Use a new policy so these tests isolate absolute age from the
            // separate monotone source-time test.
            RigPolicy p;CHECK(p.enable(binding()));auto plan=p.observe(l.ctx,&l.receipt,l.ms,l.ns);
            CHECK(p.failed()==(age>100000000));CHECK(plan.accept_mask==0);
            if(age<=100000000)CHECK(plan.source_age_ns[source]==std::uint64_t(age));
        }
    }
    {Live l;l.receipt.stereo.received_ns=l.ns+1;l.observe();CHECK(l.policy.failed());}
    {Live l;l.receipt.controllers.received_ns=l.ns+1;l.observe();CHECK(l.policy.failed());}
    {Live l;const auto boundary=l.receipt.controllers.received_ns+100000000;l.ctx.observed_ms=l.ms+97;
        CHECK(l.policy.authorize(l.plan,l.ctx,&l.receipt,l.ms+97,boundary)==3);
        CHECK(l.policy.authorize(l.plan,l.ctx,&l.receipt,l.ms+97,boundary+1)==0);}
    {Live l;l.next();l.receipt.stereo.received_ns-=10000001;l.observe();CHECK(l.policy.failed());}
    {Live l;l.next();l.receipt.controllers.received_ns-=10000001;l.receipt.controllers.sync_ns=l.receipt.controllers.received_ns;
        l.observe();CHECK(l.policy.failed());}
}
void historical_age_preserves_geometry_without_reusing_input() {
    ++scenarios;Live l;l.ms+=200;l.ns+=200000000;l.ctx.observed_ms=l.ms;
    l.plan=l.policy.observe(l.ctx,nullptr,l.ms,l.ns);
    CHECK(l.policy.bound_mask()==3&&!l.plan.accept_mask&&l.plan.restore_mask==3);
    CHECK(l.policy.last_observe().mask(ObserveReason::binding_age)==3);
    l.advance();CHECK(!l.plan.baseline_mask&&!l.plan.bind_mask&&l.auth()==3);
    CHECK(l.policy.last_observe().mask(ObserveReason::binding_age)==3);
}
void context_readiness_and_cache_revoke() {
    ++scenarios;
    for(unsigned mode=0;mode<5;++mode){Live l;auto c=l.ctx;
        if(mode==0)c.clean=false;if(mode==1)c.idle_ready=false;if(mode==2)++c.action_epoch;
        if(mode==3)++c.input_revision;if(mode==4)++c.objects.pawn.name_number;
        CHECK(l.policy.authorize(l.plan,c,&l.receipt,l.ms,l.ns)==0);
        CHECK(l.auth()==0); // A cached context cannot resurrect the retired plan.
        CHECK(l.policy.bound_mask()==0);l.advance();CHECK(l.plan.baseline_mask==3);
    }
    {Live l;l.ctx.clean=false;l.plan=l.policy.observe(l.ctx,nullptr,l.ms,l.ns);CHECK(!l.policy.bound_mask()&&l.plan.reset_binding_mask==3);
        l.ctx.clean=true;l.advance();CHECK(l.plan.baseline_mask==3);}
    {Live l;l.next();l.ctx.observed_ms=l.ms-101;l.observe();CHECK(!l.plan.accept_mask);}
}
void stop_and_revoke_do_not_own_restore_journals() {
    ++scenarios;Live l;bool journal_dirty=true;const auto old=l.plan;l.policy.stop();
    CHECK(l.policy.stopped()&&!l.policy.bound_mask()&&l.auth()==0&&!l.policy.commit_binding(old,1));
    l.advance();CHECK(l.plan.restore_mask==3&&!l.plan.accept_mask&&journal_dirty);
    // The already-owned host journal is independently restorable after stop.
    // This demonstrates no policy dependency; it is not an engine test.
    journal_dirty=false;CHECK(!journal_dirty&&!l.policy.enable(binding()));
    {Live other;other.policy.revoke();CHECK(other.auth()==0);other.advance();CHECK(other.plan.baseline_mask==3);}
}
void plan_tampering_and_whole_receipt_equality() {
    ++scenarios;Live l;const auto original=l.plan;
    auto changed=[&](auto mutate){auto p=original;mutate(p);CHECK(l.policy.authorize(p,l.ctx,&l.receipt,l.ms,l.ns)==0);CHECK(!l.policy.commit_binding(p,1));};
    changed([](RigPlan& p){++p.revision;});changed([](RigPlan& p){++p.observed_now;});
    changed([](RigPlan& p){++p.observed_ns;});changed([](RigPlan& p){++p.source_age_ns[0];});
    changed([](RigPlan& p){++p.source_age_ns[1];});changed([](RigPlan& p){p.raw_fresh=false;});
    changed([](RigPlan& p){p.has_frame=false;});changed([](RigPlan& p){p.has_ticket=false;});
    changed([](RigPlan& p){p.accept_mask=1;});changed([](RigPlan& p){p.bind_mask=1;});
    changed([](RigPlan& p){p.reset_binding_mask=1;});changed([](RigPlan& p){p.restore_mask=1;});
    changed([](RigPlan& p){p.baseline_mask=1;});changed([](RigPlan& p){++p.context.action_epoch;});
    changed([](RigPlan& p){++p.rig_context.input_revision;});changed([](RigPlan& p){++p.ticket.serial;});
    changed([](RigPlan& p){++p.ticket.certificate.stream_epoch;});
    CHECK(l.auth()==3);
    auto deny_live=[](auto mutate){Live live;auto f=live.receipt;mutate(f);
        CHECK(!same_frame(f,live.receipt));CHECK(live.policy.authorize(live.plan,live.ctx,&f,live.ms,live.ns)==0);
        CHECK(live.auth()==0);};
    deny_live([](Frame& f){++f.publication_sequence;});deny_live([](Frame& f){++f.scene_invocation;});
    deny_live([](Frame& f){++f.published_ns;});deny_live([](Frame& f){++f.scene_qpc;});
    deny_live([](Frame& f){++f.qpc_frequency;});deny_live([](Frame& f){++f.native.registration_cookie;});
    deny_live([](Frame& f){++f.native_reference.epoch;});deny_live([](Frame& f){++f.observed.reference;});
    deny_live([](Frame& f){++f.producer.tracking[1];});deny_live([](Frame& f){++f.binding_generation;});
    deny_live([](Frame& f){++f.mesh.registry_index;});deny_live([](Frame& f){f.base_view[12]=1;});
    deny_live([](Frame& f){f.source_view[12]=1;});deny_live([](Frame& f){f.reference.position.x=.04;});
    deny_live([](Frame& f){f.world_grip[0].position.x=.04;});deny_live([](Frame& f){f.world_grip[1].orientation.w=-1;});
    deny_live([](Frame& f){f.world_head.position.y=1;});deny_live([](Frame& f){f.controllers.hands[0].grip.position.x=.04;});
    deny_live([](Frame& f){f.stereo.head.position.x=.04;});deny_live([](Frame& f){++f.view_policy;});
    deny_live([](Frame& f){++f.controller_publisher_start;});deny_live([](Frame& f){++f.units_per_metre;});
}
Frame provider_frame{};gears_rig_tracking::Reply provider_reply{};std::int64_t provider_now{};
gears_rig_tracking::Reply provider(gears_rig_tracking::ReadRequest const*,Frame* out) noexcept {
    *out=provider_frame;return provider_reply;
}
std::int64_t clock_ns() noexcept {return provider_now;}
void real_reader_failure_and_read_write_gate_change() {
    ++scenarios;Live l;gears_rig_tracking::ReaderBinding reader{binding().native,0,&provider,&clock_ns};
    provider_frame=l.receipt;provider_now=l.ns;provider_reply=gears_rig_tracking::Reply::unavailable;Frame out=l.receipt;
    auto r=gears_rig_tracking::read_frame(reader,l.policy.publication_cursor(),out);
    CHECK(r.failure==gears_rig_tracking::ReadFailure::provider&&out.status==gears_rig_tracking::Status::unavailable);
    const auto old=l.plan;l.plan=l.policy.observe(l.ctx,r.failure==gears_rig_tracking::ReadFailure::none?&out:nullptr,l.ms,l.ns);
    CHECK(l.plan.restore_mask==3&&!l.plan.has_ticket&&l.policy.authorize(old,l.ctx,&provider_frame,l.ms,l.ns)==0);
    l.advance();CHECK(l.auth()==3);provider_reply=gears_rig_tracking::Reply::accepted;
    l.next(false);++l.receipt.native_reference.epoch;++l.receipt.reference_generation;
    provider_frame=l.receipt;provider_now=l.ns;
    r=gears_rig_tracking::read_frame(reader,l.plan.frame.publication_sequence-1,out);
    CHECK(r.failure==gears_rig_tracking::ReadFailure::none); // minpub is only a lower bound.
    CHECK(l.policy.authorize(l.plan,l.ctx,&out,l.ms,l.ns)==0);
    l.observe();CHECK(!l.plan.accept_mask&&l.plan.reset_binding_mask==3&&!l.policy.failed());
    {Live live;CHECK(live.policy.authorize(live.plan,live.ctx,nullptr,live.ms,live.ns)==0);CHECK(live.auth()==0);}
}
void exhaustion_and_bad_initial_binding() {
    ++scenarios;
    {RigPolicy p;auto b=binding();b.native.registration_cookie=0;CHECK(!p.enable(b));b=binding();b.publisher_start=0;CHECK(!p.enable(b));
        b=binding();b.expected_source=2;CHECK(!p.enable(b));CHECK(p.enable(binding()));}
    {RigPolicy p;auto c=context();auto out=p.observe(c,nullptr,1000,2000000000);CHECK(out.restore_mask==3&&!p.enable(binding()));}
    {Live l;l.next();l.receipt.publication_sequence=(std::numeric_limits<std::uint64_t>::max)();l.observe();CHECK(!l.policy.failed());
        l.receipt.publication_sequence=1;l.observe();CHECK(l.policy.failed());}
    {Live l;l.next();l.receipt.observed.stream=0;l.observe();CHECK(l.policy.failed());l.next();l.receipt.observed.stream=1;
        l.observe();CHECK(l.policy.failed()&&!l.plan.accept_mask);}
}
void monotone_epochs_and_live_base_motion() {
    ++scenarios;
    auto regresses=[](auto advance,auto regress){Live l;l.next();advance(l.receipt);l.observe();
        CHECK(!l.policy.failed());l.next();regress(l.receipt);l.observe();CHECK(l.policy.failed());};
    regresses([](Frame& f){f.producer.stream=2;},[](Frame& f){f.producer.stream=1;});
    regresses([](Frame& f){f.producer.tracking[0]=2;},[](Frame& f){f.producer.tracking[0]=1;});
    regresses([](Frame& f){f.producer.tracking[1]=2;},[](Frame& f){f.producer.tracking[1]=1;});
    regresses([](Frame& f){f.native_reference.epoch=2;},[](Frame& f){f.native_reference.epoch=1;});
    regresses([](Frame& f){f.observed.reference=2;},[](Frame& f){f.observed.reference=1;});
    regresses([](Frame& f){f.binding_generation=2;},[](Frame& f){f.binding_generation=1;});
    {Live l;l.next();l.receipt.base_view[12]=4;l.receipt.source_view[12]=4;
        CHECK(gears_vr::rig_tracking::pose(l.receipt.base_view,l.receipt.reference,l.receipt.controllers.hands[0].grip,
            l.receipt.units_per_metre,l.receipt.world_grip[0]));
        l.observe();CHECK(!l.plan.reset_binding_mask&&l.auth()==3);
        CHECK(l.plan.frame.world_grip[0].position.x==-4); // Already UE world; no second transform.
    }
    {Live l;l.next(false);l.receipt.base_view[12]=4;l.observe();CHECK(!l.policy.failed()&&!l.plan.accept_mask);
        CHECK(l.policy.bound_mask()==3);}
}
}
int main() {
    baseline_binding_and_type_barrier();no_read_and_old_plan_revocation();scene_duplicate_raw_and_boundary_first();
    duplicate_scene_tracks_per_hand_boundaries();raw_rollback_mutation_and_publication_replay();
    session_restart_requires_real_producer_boundary();actual_reference_and_observed_epochs();tracking_loss_each_hand();
    global_scale_binding_input_and_cohort_boundaries();ownership_and_source_rejections();fresh_source_ns_boundaries();
    historical_age_preserves_geometry_without_reusing_input();context_readiness_and_cache_revoke();
    stop_and_revoke_do_not_own_restore_journals();plan_tampering_and_whole_receipt_equality();
    real_reader_failure_and_read_write_gate_change();exhaustion_and_bad_initial_binding();monotone_epochs_and_live_base_motion();
    std::printf("HandRigTicketPolicy: %u checks in %u scenarios passed (pure CPU; no engine/SDK/XR).\n",checks,scenarios);
}
