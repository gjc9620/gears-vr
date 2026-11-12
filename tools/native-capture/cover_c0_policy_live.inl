// Included INSIDE body_c0_goal_policy_live after the shared admission helpers.
// Uses the existing Native transaction and finally; this is not another writer.
namespace cc=gears_cover_c0_committed;
namespace cg=gears_cover_committed;
uint64_t cover_fits{},cover_plans{},cover_applied{},cover_completed{},cover_rejected{};
unsigned cover_reason{};
a::Status cover_measure_status{a::Status::okay};
struct CoverCache {
    cc::Frozen frozen{};gears_cover_neutral_goal::Workspace work{};
    a::marker::Marker marker{};a::marker::Workspace marker_work{};
    neutral::Stored calibration{};Key calibration_key{};
    bool fitted{},calibrated{};
} cover_cache;
full_body_c0_native_live::native::ConsumedPose cover_completion{};
bool current_geometry(Publication const&,b::Matrix64&);
Key cover_current_key(){auto const& s=n::state;Key k{};
    k.actor=s.context.actor;k.asset=s.context.asset;k.source=s.context.source;k.reference=s.backend.metadata().reference;
    k.ref=s.source.reference;k.units=s.source.units_per_metre;k.model_scale=s.source.model_scale;return k;
}
bool cover_action(cover_context_live::CurrentKey const& key){
    return !key.transitioning&&(key.action==0||key.action==3||key.action==4||key.action==7);
}
bool cover_binding(bind::Binding const& actor,core::CoreSnapshot const& source,cover_context_live::CurrentKey& key){
    return actor.special_move==0&&bind::current_actor(actor)&&cover_context_live::sample_current(actor,key)&&
        (key.type==1||key.type==2)&&key.type==source.cover_type&&key.slot==source.cover_slot&&
        gears_weapon_hand::same(rig::object(key.link),source.cover_link)&&cover_action(key);
}
bool cover_drag_domain_equal(cg::State const& x,cg::State const& y){
    return cg::epochs(x.key.epochs,y.key.epochs)&&gears_weapon_hand::same(x.pawn,y.pawn)&&
        gears_weapon_hand::same(x.mesh,y.mesh)&&gears_weapon_hand::same(x.link,y.link)&&
        x.key.world==y.key.world&&x.key.world_generation==y.key.world_generation&&x.type==y.type&&x.key.slot==y.key.slot&&
        x.key.xr_session==y.key.xr_session&&x.key.xr_space==y.key.xr_space&&x.key.reference==y.key.reference&&
        gears_vr::stereo_same_pose(x.reference,y.reference)&&x.tracking_source==y.tracking_source&&
        x.units_per_metre==y.units_per_metre&&gears_cover_neutral_goal::same(x.anchor,y.anchor)&&
        gears_cover_neutral_goal::same(x.view_anchor,y.view_anchor);
}
bool cover_publication_current(Publication const& p){
    cg::State current{};cover_context_live::CurrentKey key{};
    return cover_enabled&&cover_full_goal_live::read_committed(current)&&cover_drag_domain_equal(p.cover_drag,current)&&
        cover_binding(n::state.context.actor,n::state.context.source,key)&&cover_context_live::same(key,p.cover_key);
}
template<class Source>bool prepare_cover_source(Entry const& e,Source const& source){
    auto& s=n::state;cg::State drag{};cover_context_live::CurrentKey before{},after{};
    a::Policy policy{};policy.now_ns=rig::clock_ns();policy.allow_sim=simulator_operator;
    policy.scope=b::Scope::fixed_cover;policy.posture=b::Posture::preserve_c0;
    if(!cover_full_goal_live::read_committed(drag)||!cg::domain(drag,source,policy)||
       !cover_binding(s.context.actor,s.context.source,before)){cover_reason=1;return false;}
    if(a::marker::prepare(s.model,s.source,a::marker::Basis::marcus_mesh_face_forward_y,
       cover_cache.marker_work,cover_cache.marker)!=a::marker::Status::okay){cover_reason=3;return false;}
    a::CoverPlacement placement{};
    const bool reuse=cover_cache.fitted&&cg::epochs(cover_cache.frozen.epochs,drag.key.epochs)&&
        cc::placement(drag,cover_cache.frozen.neutral.binding)&&
        gears_cover_neutral_goal::same(cover_cache.frozen.neutral.basis,gears_cover_neutral_goal::basis(source,s.model,s.source));
    if(reuse)placement=cover_cache.frozen.neutral.binding;
    else{
        cover_cache.fitted=false;
        if(!observed_rebind_idle()){cover_reason=2;return false;}
        // Calibration never recaptures a driven pose. Reuse the standing
        // owner's immutable mesh-local neutral, or capture before FIRST write.
        a::Neutral calibrated{};
        if(origin.initialized&&!origin.poisoned&&same_calibration_key(origin.key)){
            if(!neutral::move(origin.stored,s.source.component_to_world,calibrated)){cover_reason=4;return false;}
        }else{
            if(!cover_cache.calibrated){
                if(n::commits.load()||before.action||before.last_action||before.transitioning||
                   !neutral::capture(s.model,s.source,cover_cache.marker,s.workspace,cover_cache.calibration)){
                    cover_reason=5;return false;}
                cover_cache.calibration_key=cover_current_key();cover_cache.calibrated=true;
            }
            if(!same_calibration_key(cover_cache.calibration_key)||
               !neutral::move(cover_cache.calibration,s.source.component_to_world,calibrated)){cover_reason=6;return false;}
        }
        if(before.action||before.last_action||before.transitioning){cover_reason=7;return false;}
        a::CoverPoseMeasurement measured{};
        cover_measure_status=a::measure_cover_pose(source,s.model,s.source,cover_cache.marker,policy,
            cover_cache.work.fitting.body_work,measured);
        if(cover_measure_status!=a::Status::okay){cover_reason=8;return false;}
        placement=measured.binding;placement.anchor=drag.anchor;placement.view_anchor=drag.view_anchor;
        for(unsigned i=0;i<2;++i)placement.feet_world[i]=measured.feet[i];
        const auto fitted=cc::freeze(drag,calibrated,placement,source,s.model,s.source,cover_cache.marker,
            policy,{.25},cover_cache.work,cover_cache.frozen);
        if(fitted!=cc::Code::prepared){cover_reason=100+unsigned(fitted);return false;}
        cover_cache.fitted=true;++cover_fits;
    }
    const auto& parent=gears_cover_neutral_goal::parent(source);
    placement.scene=parent.scene_invocation;placement.publication=parent.publication_sequence;
    if constexpr(std::is_same_v<Source,gears_body_tick_source::Input>)placement.observed_ns=source.metadata_end_ns;
    else placement.observed_ns=std::int64_t(parent.published_ns);
    cc::Prepared prepared{};const auto code=cc::prepare(cover_cache.frozen,drag,placement,source,
        s.model,s.source,cover_cache.marker,policy,prepared);
    if(code!=cc::Code::prepared){cover_reason=200+unsigned(code);return false;}
    ++cover_plans;++plans;
    if(!receipt_ready()||!s.context.current(n::tx::Stage::prepare)||
       !cover_binding(s.context.actor,s.context.source,after)||!cover_context_live::same(before,after)){
        cover_reason=9;return false;}
    pending={};pending.entry=&e;pending.copy=e;pending.revision=revision.load();open_ends=1;
    pending.next.key=cover_current_key();pending.next.render=prepared.body.render;
    pending.next.component=s.source.component_to_world;pending.next.tick=s.context.tick;pending.next.compose=e.invocation;
    pending.next.revision=pending.revision;pending.next.cover=true;pending.next.cover_drag=drag;pending.next.cover_key=before;
    pending.committed=n::apply_with_view_marker(e,prepared.body.goal,s.context.source,prepared.body.render.base_view,
        cover_cache.marker,&prepared.body.desired_eye);
    if(pending.committed){pending.kernel_serial=s.kernel.serial();++applied;++cover_applied;cover_reason=0;}
    else{++rejected;++cover_rejected;cover_reason=10;}
    return true; // The shared finally owns this attempt even after apply fails.
}
bool prepare_cover(Entry const& e){
    if(body_tick_source_live::enabled)return prepare_cover_source(e,n::state.context.body_source);
    if(body_tick_input_live::enabled){cover_reason=11;return false;} // No V1-as-V3 substitution.
    return prepare_cover_source(e,n::state.context.source);
}
bool cover_consumed(Entry const& e,Publication const& p){
    if(!n::read_completed_cover(e,p.key.source,cover_completion))return false;
    ++cover_completed;return true;
}
bool read_cover_view(Publication const& p,gears_anchor::Receipt const* scene,int64_t now,gears_vr::CoverViewResult& out){
    // A newer input intent may be waiting for the next C0. Display THIS
    // consumed pose's offset, never the newer requested offset. Domain changes
    // still invalidate it; input sequence/offset changes do not renew its TTL.
    if(!cover_enabled||!rig::source_fresh(p.ended_ns,now,a::maximum_age_ns))return false;
    bind::Binding actor{},closing{};cover_context_live::CurrentKey key{},after{};cg::State drag{};b::Matrix64 component{};
    if(!bind::sample(scene,actor)||!same_neutral_actor(actor,p.key.actor)||
       !cover_binding(actor,p.key.source,key)||!cover_context_live::same(key,p.cover_key)||
       !cover_full_goal_live::read_committed(drag)||!cover_drag_domain_equal(p.cover_drag,drag)||
       !current_geometry(p,component)||std::memcmp(&component,&p.component,sizeof component)||
       !bind::sample(scene,closing)||!same_neutral_actor(closing,p.key.actor)||
       !cover_binding(closing,p.key.source,after)||!cover_context_live::same(key,after))return false;
    std::lock_guard<std::mutex> lock(view_mutex);
    if(stopping.load()||!view_valid.load()||p.version!=view.version||p.revision!=revision.load())return false;
    out=p.render;return true;
}
