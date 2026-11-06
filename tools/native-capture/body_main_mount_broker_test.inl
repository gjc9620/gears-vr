// Exercise the actual broker seam: the V3 sender-only fixture cannot detect
// an obsolete fresh-Scene gate in this independent Native half.
namespace bm=gears_body_main_mount_provider;
unsigned body_mount_calls{},body_current_calls{},body_mount_mode{};
bool body_current_ok=true;
bool body_module(void*,bm::Registration const&,w::ModulePin p) noexcept{return p.handle==0x8000&&p.module==0x8000;}
bool body_current(void* context,bm::Source const& s) noexcept {
    ++body_current_calls;const auto now=static_cast<Fake*>(context)->stamp.load();
    const auto fresh=[&](std::int64_t stamp){return stamp>0&&stamp<=now&&std::uint64_t(now-stamp)<=bm::tracking_age_ns;};
    return body_current_ok&&fresh(s.tracking.begin_ns)&&fresh(s.tracking.end_ns)&&
        fresh(s.tracking.stereo.received_ns)&&fresh(s.tracking.controllers.received_ns)&&fresh(s.tracking.controllers.sync_ns)&&
        fresh(s.basis.capture_begin_ns);
}
std::uint32_t body_prepare(void* context,bm::Request const* q,bm::Response* out) noexcept {
    auto& f=*static_cast<Fake*>(context);++body_mount_calls;
    C(f.broker->pending_callbacks()&&!f.broker->pending_transactions());
    static_assert(bm::version==5); // Explicit visual kind, unchanged ownership.
    C(q->abi_version==bm::version&&q->query_id&&q->hand_lifecycle==f.observed_frame.key.lifecycle);
    C(q->maximum_tracking_age_ns==40000000&&q->maximum_scene_age_ns==50000000&&q->maximum_equipment_age_ns==50000000);
    *out={};out->query_id=q->query_id;out->hand_lifecycle=q->hand_lifecycle;out->action_generation=q->action_generation;
    out->hold_kind=bm::HoldKind::firearm;
    out->source=bm::source_key(q->source);out->equipment_begin_ns=f.stamp.load()-1000000;out->equipment_end_ns=f.stamp.load();
    auto& r=out->mount;auto const& b=q->source.basis.geometry;auto const& frame=f.observed_frame;
    r.query_id=q->query_id;r.key=frame.key;r.world=frame.world;r.controller=frame.controller;r.pawn=frame.pawn;r.mesh=frame.mesh;r.weapon=frame.weapon;
    r.component={0x7000,r.weapon.address,0x9000,7,8,0,1};r.asset={0x8000,0,0x9000,8,9,0,1};r.hold_generation=1;
    r.provider_revision=m::provider_revision;r.body_revision=b.revision;r.basis_revision=b.basis_revision;r.basis=b.basis;
    r.geometry=b.geometry;r.model_scale=b.model_scale;r.units_per_metre=b.units_per_metre;
    gears_weapon_lancer_wrist_mount::Profile profile{};C(gears_weapon_lancer_wrist_mount::marcus115(b.geometry,b.model_scale,b.units_per_metre,profile));
    r.calibration_known=1;r.grip_to_wrist_metres=profile.mount.calibration.grip_to_wrist_metres;r.grip_to_wrist_rotation=profile.mount.calibration.grip_to_wrist_rotation;
    if(body_mount_mode==1)out->equipment_begin_ns=f.stamp.load()-50000001;
    if(body_mount_mode==2)f.stamp+=2000001;
    if(body_mount_mode==3)body_current_ok=false;
    if(body_mount_mode==4)++const_cast<bm::Request*>(q)->source.scene_ticket;
    if(body_mount_mode==5)out->hold_kind=bm::HoldKind::frag;
    if(body_mount_mode==6)out->hold_kind=bm::HoldKind::unknown;
    if(body_mount_mode==7)out->reserved=1;
    return unsigned(bm::Status::ready);
}
void configure_body_mount_test_backend(w::BrokerBackend& b) noexcept {
    b.body_mount_module=body_module;b.body_mount_prepare=w::weapon_broker_win32::body_mount_prepare;b.body_mount_current=body_current;
}
bm::Source body_source(Fixture& f,MountInput const& x,std::int64_t elapsed=300000000){
    f.fake.stamp+=elapsed;const auto now=f.fake.stamp.load();bm::Source s{};s.original_scene=x.rig;s.scene_ticket=41;s.scene_cache_revision=51;
    s.metadata_begin_ns=now-9000000;s.metadata_end_ns=now-8000000;auto& t=s.tracking;t.tick=61;t.thread=gt;t.parent_cookie=f.result.cookie;
    t.begin_ns=now-7000000;t.end_ns=now-3000000;t.stereo=x.rig.stereo;t.stereo.sequence+=5;t.stereo.received_ns=now-4000000;
    t.controllers=x.rig.controllers;t.controllers.sequence=t.stereo.sequence;t.controllers.received_ns=now-5000000;t.controllers.sync_ns=now-6000000;
    s.basis.compose=71;s.basis.capture_begin_ns=now-2000000;s.basis.geometry=x.body;s.basis.component_to_world=x.rig.base_view;s.basis.base_view=x.rig.base_view;return s;
}
void body_attach(Fixture& f){bm::Registration r{};r.hand_cookie=f.result.cookie;r.native_started_qpc=500;r.context=&f.fake;r.prepare=body_prepare;
    C(f.broker.attach_body_mount_provider(&r,&f.result,true)==w::Reply::accepted);}
bool body_empty(bm::Response const& r){bm::Response empty{};return std::memcmp(&r,&empty,sizeof r)==0;}
void body_mount_tests(){
    body_mount_mode=0;body_current_ok=true;
    for(unsigned mode:{5u,6u,7u}){Fixture f;f.attach();body_attach(f);auto x=mount_committed(f);auto s=body_source(f,x);bm::Response out{};
        body_mount_mode=mode;const auto result=f.broker.query_body_mount(s,out);
        if(mode==5)C(result.code==MC::ready&&out.hold_kind==bm::HoldKind::frag);
        else C(result.code==MC::fault&&body_empty(out));
        C(!f.broker.pending_callbacks()&&!f.broker.pending_transactions());body_mount_mode=0;f.finish();}
    {Fixture f;f.attach();mount_attach(f);body_attach(f);auto x=mount_committed(f);auto s=body_source(f,x);bm::Response out{};
        const auto old=s.original_scene;auto result=f.broker.query_body_mount(s,out);
        C(result.code==MC::ready&&result.branch==w::MainMountQueryResult::Branch::reply);
        C(out.mount.key.frame==old.scene_invocation&&out.source.scene_publication==old.publication_sequence);
        C(std::memcmp(&old,&s.original_scene,sizeof old)==0&&!f.broker.authorized());m::Response legacy{};
        C(f.broker.query_main_mount(s.original_scene,x.body,legacy).code==MC::stale&&mount_empty(legacy));
        f.finish();}
    // Old associations remain exact; current receipt, source and equipment
    // deadlines still apply independently, both before and after callbacks.
    for(unsigned bad=0;bad<9;++bad){Fixture f;f.attach();body_attach(f);auto x=mount_committed(f);auto s=body_source(f,x);bm::Response out{};
        const auto calls=body_mount_calls;
        switch(bad){case 0:s.metadata_begin_ns=f.fake.stamp.load()-50000001;break;
        case 1:++s.original_scene.scene_invocation;break;case 2:++s.original_scene.weapon.name_number;break;
        case 3:s.original_scene.scene_qpc=std::int64_t(s.original_scene.published_ns)+1;break;
        case 4:body_current_ok=false;break;case 5:body_mount_mode=1;break;
        case 6:s.metadata_begin_ns=f.fake.stamp.load()-49000000;body_mount_mode=2;break;
        case 7:body_mount_mode=3;break;case 8:body_mount_mode=4;break;}
        const auto result=f.broker.query_body_mount(s,out);C(result.code!=MC::ready&&body_empty(out));
        if(bad<4)C(result.code==MC::stale&&result.branch==w::MainMountQueryResult::Branch::source_before&&body_mount_calls==calls);
        if(bad==4)C(result.code==MC::context_rejected&&body_mount_calls==calls);
        if(bad==5||bad==6)C(result.code==MC::stale&&result.branch==w::MainMountQueryResult::Branch::source_after);
        if(bad==7)C(result.code==MC::context_rejected&&result.branch==w::MainMountQueryResult::Branch::source_after);
        if(bad==8)C(result.code==MC::fault);
        C(!f.broker.pending_callbacks()&&!f.broker.pending_transactions());body_mount_mode=0;body_current_ok=true;f.finish();}
    printf("body_mount V3 broker seam complete calls=%u native_current=%u\n",body_mount_calls,body_current_calls);
}
