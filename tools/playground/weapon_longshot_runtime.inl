// Longshot normal-GT manual reload. Shared Native hand arbitration, original
// Reloading lease, stopped gun clip and revisioned inventory are the owners.
namespace longshot_runtime {
namespace hand=gears_weapon_hand;namespace inv=lancerreload_inventory;
namespace bolt=longshot_bolt;namespace round=longshot_round;
using P=gears_vr::Pose;using V=gears_vr::Vector;
enum class Stage {idle,acquiring,opening,waiting_round,holding_round,waiting_close,closing,completing,recovering};
Stage stage{};inv::ObservedInventory observer;inv::Transaction transaction;inv::InsertionWriter writer;
// Lifecycle aggregation only; never a veto for another weapon's new action.
// Hidden prepared assets have their own resource-owner drain accounting.
bool pending_work() noexcept {return stage!=Stage::idle;}
inv::Inventory opening{};inv::Request completion{};lancerreload_state::RewardReceipt reward{};
inv::Identity proposal_identity{},accepted_identity{},last_intent_identity{};P root_in_right{};double root_scale{};
V knob_local{},port_local{};bool proposal_known{},switch_requested{},standalone{},native_exited{},ui_pending{},shot_timed{};
hand::Lease held{},accepted{};std::uint64_t serial{},animation_serial{},last_intent{},accepted_intent{},proposal_reference{};
std::int64_t started_us{},last_feed_ns{};std::uint64_t feed_sequence{};hand::Frame feed_source{};V last_tip{};bool approached{};
std::uint64_t ui_prepare_frame{};
std::uint64_t contact_frame{};std::int64_t contact_sync_ns{};P contact_root_world{};
V bolt_pivot_local{}; // Existing measured bone POD, cosmetic stem direction only.
#include "weapon_longshot_acquiring_trace.inl"
// Cosmetic lifecycle facts only. Never clear or replace the authoritative
// shot clock, action lease, inventory transaction or completed result.
struct GuideShot {inv::Identity identity{};std::uint64_t event{};};
GuideShot cycle_guide_shot{},completed_guide_shot{};
// A retrospective cosmetic cue for one actually refused empty-reserve
// attempt. Its display lifetime never extends an input or action lifetime.
struct NoAmmoCue {hand::Frame source{};std::uint64_t hold{},token{};bool known{},active{};};
NoAmmoCue no_ammo_cue;
void note_no_ammo(hand::Frame const& f) noexcept {
    const auto hold=nativelancerfollow::bound_key.hold_generation;
    const bool same=no_ammo_cue.known&&no_ammo_cue.hold==hold&&
        gears_weapon_longshot_round_presentation::same_context(no_ammo_cue.source,f)&&
        no_ammo_cue.source.stereo.source==f.stereo.source&&no_ammo_cue.source.controllers.source==f.controllers.source;
    if(!f.press_token||f.controllers.sync_ns<=0||(same&&f.press_token<=no_ammo_cue.token))return;
    no_ammo_cue={f,hold,f.press_token,true,true};
}
bool no_ammo_hint(hand::Frame const& f) noexcept {
    return no_ammo_cue.active&&no_ammo_cue.hold==nativelancerfollow::bound_key.hold_generation&&
        gears_weapon_longshot_round_presentation::same_context(no_ammo_cue.source,f)&&
        no_ammo_cue.source.stereo.source==f.stereo.source&&
        no_ammo_cue.source.controllers.source==f.controllers.source&&
        f.key.frame>=no_ammo_cue.source.key.frame&&f.controllers.sequence>=no_ammo_cue.source.controllers.sequence&&
        f.controllers.sync_ns>=no_ammo_cue.source.controllers.sync_ns&&
        f.controllers.sync_ns-no_ammo_cue.source.controllers.sync_ns<750000000;
}
struct Debt {
    inv::Identity identity{};bool active{},saved{};
    inv::ObservedInventory observer{};inv::Transaction transaction{};inv::Inventory opening{};
    lancerreload_state::RewardReceipt reward{};std::uint64_t serial{};std::int64_t started_us{};bool shot_timed{};
    GuideShot guide_shot{};
};
std::array<Debt,16> debts{};int debt_slot{-1};
// Bounded observations of this state machine only. No extra object reads,
// retries, permissions or state setters; a driver must exercise real input.
struct TransitionReport {
    Stage before{stage};inv::Identity before_identity{opening.identity};std::uint64_t before_serial{serial};
    ~TransitionReport() noexcept {
        static unsigned rows{};
        if(!log_file||before==stage||rows>=96)return;
        const auto& id=opening.identity.weapon.address?opening.identity:before_identity;
        fwprintf(log_file,L"longshot_reload_transition from=%u to=%u serial=%llu weapon=%llx hold=%llu chamber_debt=%d\n",
            unsigned(before),unsigned(stage),serial?serial:before_serial,
            static_cast<unsigned long long>(id.weapon.address),id.hold_generation,debt_slot);
        fflush(log_file);++rows;
    }
};
void report_contact(hand::Frame const& f,bool opening_ready) noexcept {
    static std::array<unsigned,2> rows{};auto& count=rows[opening_ready?1:0];
    if(!log_file||count>=2)return;
    const auto p=root_in_right.position;const auto q=root_in_right.orientation;
    fwprintf(log_file,L"%ls serial=%llu weapon=%llx frame=%llu root_in_right=%.9g,%.9g,%.9g root_rotation=%.9g,%.9g,%.9g,%.9g scale=%.9g units=%.9g knob=%.9g,%.9g,%.9g port=%.9g,%.9g,%.9g\n",
        opening_ready?L"longshot_reload_geometry":L"longshot_reload_contact",serial,
        static_cast<unsigned long long>(f.weapon.address),f.key.frame,p.x,p.y,p.z,q.x,q.y,q.z,q.w,
        root_scale,nativelancerfollow::bound_units,knob_local.x,knob_local.y,knob_local.z,port_local.x,port_local.y,port_local.z);
    fflush(log_file);++count;
}
bool same_weapon(inv::Identity a,inv::Identity b) noexcept {
    return hand::same(a.world,b.world)&&hand::same(a.pawn,b.pawn)&&hand::same(a.weapon,b.weapon)&&a.game_start==b.game_start&&a.native_start==b.native_start;
}
void report_live_contact(hand::Frame const& f) noexcept {
    // Existing bind_contact already authenticated these exact POD values.
    // Initial-only pre-fire markers are insufficient to plan a later reach.
    static unsigned rows{};static std::int64_t previous{};
    const auto current_identity=inv::identity(f);
    const bool exact_shot=longshot_fire::confirmed.identity==current_identity;
    const bool same_shot_instance=inv::same_instance_across_hold(longshot_fire::confirmed.identity,current_identity);
    if(!log_file||rows>=48||stage!=Stage::idle||!longshot_fire::confirmed.active||
       !same_shot_instance||f.controllers.sync_ns<=previous||
       (previous&&f.controllers.sync_ns-previous<250000000))return;
    // This is planning data, not a revived lease or input witness. Record
    // normalization only after all original current contact checks succeeded.
    if(!exact_shot){
        static unsigned rebound_rows{};
        if(rebound_rows<8){++rebound_rows;const auto& before_identity=longshot_fire::confirmed.identity;
            fwprintf(log_file,L"longshot_contact_plan_rebind frame=%llu old_lifecycle=%llu lifecycle=%llu old_hold=%llu hold=%llu session=%llu space=%llu input=%llu original_shot_clock_unchanged=1 action_authority=0\n",
                f.key.frame,before_identity.lifecycle,current_identity.lifecycle,before_identity.hold_generation,current_identity.hold_generation,
                current_identity.session,current_identity.space,current_identity.input);
        }
    }
    const auto p=root_in_right.position;const auto q=root_in_right.orientation;
    fwprintf(log_file,L"longshot_reload_contact_current serial=%llu weapon=%llx frame=%llu source_sync_ns=%lld root_in_right=%.9g,%.9g,%.9g root_rotation=%.9g,%.9g,%.9g,%.9g scale=%.9g units=%.9g knob=%.9g,%.9g,%.9g port=%.9g,%.9g,%.9g\n",
        serial,static_cast<unsigned long long>(f.weapon.address),f.key.frame,f.controllers.sync_ns,
        p.x,p.y,p.z,q.x,q.y,q.z,q.w,root_scale,nativelancerfollow::bound_units,
        knob_local.x,knob_local.y,knob_local.z,port_local.x,port_local.y,port_local.z);
    fflush(log_file);++rows;previous=f.controllers.sync_ns;
}
int debt_for(inv::Identity const& id,bool create=false) noexcept {
    for(unsigned i=0;i<debts.size();++i)if(debts[i].active&&same_weapon(debts[i].identity,id))return int(i);
    if(create)for(unsigned i=0;i<debts.size();++i){auto& d=debts[i];if(d.active&&lancerreload_runtime::weapon_retired(d.identity.weapon))d={};
        if(!d.active){d={id,true};return int(i);}}
    return -1;
}
bool guide_needed(hand::Frame const& f) noexcept {
    if(stage==Stage::recovering||stage==Stage::completing)return false;
    if(stage!=Stage::idle)return true;
    if(no_ammo_hint(f))return true;
    const auto id=inv::identity(f);
    // An inserted round still owes physical closure after interruption.
    if(debt_for(id)>=0)return true;
    const auto& shot=longshot_fire::confirmed;
    if(!shot.active||!shot.shot.confirmed||!shot.shot.original_event_sequence||
       !inv::same_instance_across_hold(shot.identity,id))return false;
    return !(shot.shot.original_event_sequence==completed_guide_shot.event&&
        inv::same_instance_across_hold(completed_guide_shot.identity,id));
}
bool enabled() noexcept {return weapon_part_inputs::original_source_enabled.load(std::memory_order_acquire);}
bool selected(hand::Frame const& f) noexcept {
    return enabled()&&nativelancerfollow::bound_key_known&&nativelancerfollow::bound_is_longshot&&
        hand::same(f.weapon,nativelancerfollow::bound_key.weapon)&&nativelancerfollow::bound_key.hold_generation==vrequip::model.hold_generation();
}
bool blocks_fire(hand::ObjectIdentity const& weapon) noexcept {
    if(stage!=Stage::idle&&hand::same(opening.identity.weapon,weapon))return true;
    for(auto const& d:debts)if(d.active&&hand::same(d.identity.weapon,weapon))return true;return false;
}
void switch_dispatched(hand::Frame const& f) noexcept {if(stage!=Stage::idle&&same_weapon(opening.identity,inv::identity(f)))switch_requested=true;}
bool clock(hand::Frame const& f,std::int64_t& tick) noexcept {
    float seconds{};if(!nativeequipment::current(nullptr,f)||!lancerreload_state::time_field(f.world.address,L"TimeSeconds",seconds))return false;
    const double value=double(seconds)*1000000.;if(value<0||value>9e15||!std::isfinite(value))return false;tick=static_cast<std::int64_t>(value);return true;
}
bool bind_contact(hand::Frame const& f,bolt::BonesReceipt* receipt=nullptr) noexcept {
    contact_observation_gate=1;
    if(!selected(f))return false;
    contact_observation_gate=2;
    P root{},part{};double size{};if(!bolt::bones(f,root,part,size))return false;
    gears_rig_tracking::Frame rig{};const auto source=static_cast<std::uint32_t>(f.stereo.source);
    contact_observation_gate=3;
    if(handrig::read(rig,0,source).reply!=gears_rig_tracking::Reply::accepted||
       gears_equipment_rig::match(f,rig,weaponhand::clock_ns(nullptr),source)!=gears_equipment_rig::Status::matched||
       !(rig.world_pose_mask&gears_rig_tracking::right_grip_pose))return false;
    vrequip::MeshPoseProbe mesh{};P gun{};double gun_scale{};std::array<float,16> raw{};
    contact_observation_gate=4;
    if(!vrequip::mesh_pose_probe(f.weapon.address,mesh)||mesh.mesh!=nativelancerfollow::bound_key.component.address||mesh.asset!=nativelancerfollow::bound_key.asset.address)return false;
    std::memcpy(raw.data(),mesh.local_to_world,sizeof mesh.local_to_world);
    contact_observation_gate=5;
    if(!hammerburst_runtime::matrix(raw,gun,gun_scale)||std::abs(size-gun_scale)>.001)return false;
    const double units=nativelancerfollow::bound_units;
    const auto knob=bolt::prepared?bolt::point(part,bolt::handle_in_bone,size):
        bolt::point(gun,gears_weapon_longshot_bolt_contact::knob_in_mesh_units,size);
    // Initial chamber point under the rear scope, rear-to-front insertion.
    // Exact surface and snap-display fitting remain runtime acceptance work.
    const auto port=bolt::point(gun,{0,26,10.5},size);
    const auto& right=rig.world_grip[1];
    P relative{bolt::local(right,root.position),gears_vr::normalized(gears_vr::multiply(gears_vr::inverse(right.orientation),root.orientation))};
    contact_observation_gate=6;
    if(!gears_vr::stereo_pose_valid(relative)||!nativeequipment::current(nullptr,f))return false;
    root_in_right=relative;root_scale=size;knob_local=bolt::local(root,knob,size*units);port_local=bolt::local(root,port,size*units);
    proposal_identity=inv::identity(f);proposal_reference=f.reference_generation;proposal_known=true;
    contact_frame=f.key.frame;contact_sync_ns=f.controllers.sync_ns;contact_root_world=root;
    bolt_pivot_local=bolt::local(root,part.position,size*units);
    report_live_contact(f);
    report_contact(f,false);
    contact_observation_gate=7;
    if(receipt)*receipt=bolt::capture_bones(f,root,part,size);
    return true;
}
bool proposal_left(hand::Frame const& f,V& out) noexcept {
    if(!proposal_known||!(proposal_identity==inv::identity(f))||proposal_reference!=f.reference_generation)return false;
    for(auto const& h:f.controllers.hands)if(!gears_vr::controller_pose_usable(h.grip,h.grip_flags,h.grip_active))return false;
    const double units=nativelancerfollow::bound_units;
    const auto convert=[&](P const& p){const auto basis=gears_vr::xr_to_ue(gears_vr::rotation_matrix(p.orientation));gears_vr::Quaternion q{};
        if(!gears_vr::rig_tracking::detail::quaternion(basis,q))return P{};
        return P{bolt::mul(gears_vr::xr_to_ue(p.position),units),q};};
    const auto left=convert(f.controllers.hands[0].grip),right=convert(f.controllers.hands[1].grip);
    const P root{bolt::point(right,root_in_right.position),gears_vr::normalized(gears_vr::multiply(right.orientation,root_in_right.orientation))};
    out=bolt::local(root,left.position,root_scale*units);return gears_vr::finite(out);
}
// Observe a real left press at the two existing boundaries. No extra object
// reads or action attempts. Final-state transitions alone miss an acquire
// and recovery that both finish inside the same tick.
void report_input(hand::Frame const& f,hand::GripGameState const& game,hand::Decision const* decision) noexcept {
    if(!log_file||!selected(f))return;
    const auto& input=f.controllers.hands[0];
    if(input.squeeze<.5f&&input.trigger<.5f)return;
    const unsigned phase=decision?1:0;static unsigned rows{};
    static std::array<std::array<std::uint64_t,5>,2> previous{};
    const std::array<std::uint64_t,5> key{f.weapon.address,f.key.lifecycle,f.controllers.input_epoch,
        input.pressed[gears_vr::control_squeeze],input.pressed[gears_vr::control_trigger]};
    if(rows>=16||previous[phase]==key)return;previous[phase]=key;
    V physical{};const bool local_known=proposal_left(f,physical);
    const double distance=local_known?snub_visual::length(bolt::sub(physical,knob_local)):-1.;
    fwprintf(log_file,L"longshot_reload_input phase=%u frame=%llu token=%llu stage=%u control=%u left=%u,%u,%u raw=%.9g,%.9g requests=%u,%u,%u busy=%u animation=%u main=%u proposal=%u local_known=%u local=%.9g,%.9g,%.9g knob_distance=%.9g round_ready=%u decision=%u lease=%u lease_id=%llu new_grant=%u\n",
        phase,f.key.frame,f.press_token,unsigned(stage),f.interaction_control,f.left_tracked,f.left_grip_known,f.left_grip_down,
        double(input.squeeze),double(input.trigger),unsigned(game.request_magazine),unsigned(game.request_ammo),unsigned(game.magazine_observation_pending),
        unsigned(game.busy),unsigned(game.animation_owns_hands),unsigned(game.main_hand_held),unsigned(proposal_known),unsigned(local_known),
        physical.x,physical.y,physical.z,distance,unsigned(round::ready),decision?unsigned(decision->status):~0u,
        decision?unsigned(decision->lease.kind):0u,decision?decision->lease.id:0,decision?decision->new_grant:0);
    const auto& left=input.grip;const auto& right=f.controllers.hands[1].grip;
    const auto p=root_in_right.position;const auto q=root_in_right.orientation;
    fwprintf(log_file,L"longshot_reload_pose phase=%u frame=%llu token=%llu source_sync_ns=%lld contact_frame=%llu contact_sync_ns=%lld left_grip=%.9g,%.9g,%.9g left_quat=%.9g,%.9g,%.9g,%.9g right_grip=%.9g,%.9g,%.9g right_quat=%.9g,%.9g,%.9g,%.9g root_in_right=%.9g,%.9g,%.9g root_rotation=%.9g,%.9g,%.9g,%.9g knob=%.9g,%.9g,%.9g scale=%.9g units=%.9g\n",
        phase,f.key.frame,f.press_token,f.controllers.sync_ns,contact_frame,contact_sync_ns,
        left.position.x,left.position.y,left.position.z,left.orientation.x,left.orientation.y,left.orientation.z,left.orientation.w,
        right.position.x,right.position.y,right.position.z,right.orientation.x,right.orientation.y,right.orientation.z,right.orientation.w,
        p.x,p.y,p.z,q.x,q.y,q.z,q.w,knob_local.x,knob_local.y,knob_local.z,root_scale,nativelancerfollow::bound_units);
    fflush(log_file);++rows;
}
void entry_gate(hand::Frame const& f,unsigned gate) noexcept {
    static unsigned rows{},last_gate{~0u};static std::uint64_t last_token{},last_weapon{};
    if(!log_file||rows>=32||!f.left_grip_down||
       (last_gate==gate&&last_token==f.press_token&&last_weapon==f.weapon.address))return;
    fwprintf(log_file,L"longshot_reload_entry gate=%u frame=%llu token=%llu weapon=%llx stage=%u serial=%llu\n",
        gate,f.key.frame,f.press_token,static_cast<unsigned long long>(f.weapon.address),unsigned(stage),serial);
    fflush(log_file);++rows;last_gate=gate;last_token=f.press_token;last_weapon=f.weapon.address;
}
void propose_live(hand::Frame const& f,hand::GripGameState& game) noexcept {
    if(!selected(f)||!game.main_hand_held||game.kind!=hand::WeaponKind::firearm)return;
    report_input(f,game,nullptr);
    game.magazine_observation_pending=false;
    if(!f.left_tracked||!f.left_grip_known||!f.left_grip_down||game.busy||game.animation_owns_hands)return;
    if(stage==Stage::recovering||stage==Stage::completing)return;
    if(held.id==f.press_token&&hand::lease_valid(held,f.key.lifecycle)){
        for(auto& slot:game.object_incumbents){if(hand::same(slot,held))return;if(slot.kind==hand::LeaseKind::none){slot=held;return;}}return;
    }
    if(stage==Stage::waiting_round){const P body{f.stereo.head.position,f.reference.orientation};
        game.request_ammo=transaction.target()>0&&gears_weapon::body_slot(nativeequipment::calibration,body,f.controllers.hands[0].grip.position,0)==gears_weapon::BodySlot::shoulder_ammo;return;}
    if(stage!=Stage::idle&&stage!=Stage::opening&&stage!=Stage::waiting_close&&stage!=Stage::closing)return;
    V left{};if(proposal_left(f,left))game.request_magazine=snub_visual::length(bolt::sub(left,knob_local))<=.05;
}
// A completed arbitration publication is immutable for its exact owner Frame.
// Mechanic state can advance later in that GT callback, but its new incumbent
// is a NEXT-frame proposal. Fresh actor/input/busy/mount checks stay in the
// shared HandClient and Native; only this producer's route fields are kept.
struct RoutePublication {
    hand::Frame source{};hand::GripGameState before{},after{};bool known{};
};
RoutePublication route_proposal{},route_committed{};
bool same_route(hand::GripGameState const& a,hand::GripGameState const& b) noexcept {
    return a.request_ammo==b.request_ammo&&a.request_magazine==b.request_magazine&&
        a.magazine_observation_pending==b.magazine_observation_pending&&
        hand::same(a.object_incumbents[0],b.object_incumbents[0])&&hand::same(a.object_incumbents[1],b.object_incumbents[1]);
}
void propose(hand::Frame const& f,hand::GripGameState& game) noexcept {
    if(!selected(f)||!game.main_hand_held||game.kind!=hand::WeaponKind::firearm)return;
    if(route_committed.known&&hand::same_frame(route_committed.source,f)&&same_route(game,route_committed.before)){
        const auto& saved=route_committed.after;
        game.request_ammo=saved.request_ammo;game.request_magazine=saved.request_magazine;
        game.magazine_observation_pending=saved.magazine_observation_pending;game.object_incumbents=saved.object_incumbents;
        return;
    }
    const auto before=game;propose_live(f,game);route_proposal={f,before,game,true};
}
void committed(hand::Frame const& f,hand::Decision const& d) noexcept {
    if(selected(f)&&hand::same(f.key,d.key)&&route_proposal.known&&hand::same_frame(route_proposal.source,f))
        route_committed=route_proposal;
    if(!selected(f)||!d.new_grant||!bolt::granted(f,d)||!proposal_known||!(proposal_identity==inv::identity(f)))return;
    accepted_intent=f.press_token;accepted_identity=inv::identity(f);accepted=d.lease;
}
void reset() noexcept {
    stage=Stage::idle;observer={};transaction={};writer={};opening={};completion={};reward={};held=accepted={};
    serial=animation_serial=accepted_intent=0;proposal_known=switch_requested=standalone=native_exited=ui_pending=shot_timed=false;
    last_feed_ns=started_us=0;feed_sequence=ui_prepare_frame=0;feed_source={};approached=false;debt_slot=-1;bolt::reset();
    route_proposal={};route_committed={};cycle_guide_shot={};no_ammo_cue.active=false;
    lancerreload_runtime::outstanding.store(false,std::memory_order_release);
}
bool cleanup() noexcept {
    stage=Stage::recovering;proposal_known=false;held={};bolt::stroke.release();
    inv::Request owed_request{};
    if(transaction.dispatched_request(owed_request)){
        if(!lancerreload_state::restore_partial_insert(owed_request,observer))return false;
        inv::Inventory counts{};if(!observer.sample_owned(owed_request.identity,counts))return false;
        const bool applied=inv::counts_equal(counts,owed_request.expected);
        if(!applied&&!inv::counts_equal(counts,owed_request.before))return false;
        // This debt was reserved BEFORE dispatch so uncertain partial writes
        // cannot unblock firing. A reconciled unchanged pre-insertion count
        // proves no round entered the chamber, so no false chamber debt may
        // survive a definitively rejected insertion.
        const bool rejected_insert=!applied&&owed_request.call==gears_weapon::ReloadTxnCall::insert;
        if(owed_request.call!=gears_weapon::ReloadTxnCall::insert&&applied&&owed_request.result==gears_weapon::ReloadTxnResult::perfect&&!reward.applied)return false;
        const auto result=applied&&owed_request.call!=gears_weapon::ReloadTxnCall::insert&&owed_request.result!=gears_weapon::ReloadTxnResult::inspection?owed_request.result:gears_weapon::ReloadTxnResult::none;
        if(!transaction.acknowledge({owed_request.token,applied,counts,result}))return false;
        if(rejected_insert&&debt_slot>=0)debts[unsigned(debt_slot)]={};
    }
    if(round::item_serial&&!round::hide())return false;
    if(serial&&!lancerreload_state::recover_serial(serial))return false;
    if(!longshot_clip::restore())return false;
    if(!round::restore())return false;
    if(debt_slot>=0&&debts[unsigned(debt_slot)].active&&transaction.inserted()&&!transaction.pending()){
        const auto state=transaction.stage();
        if(state==gears_weapon::ReloadTxnStage::closing||state==gears_weapon::ReloadTxnStage::interrupted){
            if(state==gears_weapon::ReloadTxnStage::closing&&!transaction.interrupt())return false;
            auto& saved=debts[unsigned(debt_slot)];saved.identity=transaction.inventory().identity;saved.saved=true;
            saved.observer=observer;saved.transaction=transaction;saved.opening=opening;saved.reward=reward;
            saved.serial=serial;saved.started_us=started_us;saved.shot_timed=shot_timed;
            saved.guide_shot=cycle_guide_shot;
        }
    }
    reset();return true;
}
bool begin_stroke(hand::Frame const& f,hand::Decision const& d,gears_weapon::longshot_tracks::BoltDirection direction) noexcept {
    if(!bolt::granted(f,d))return false;
    const bool accepted_here=accepted_intent==f.press_token&&accepted_identity==inv::identity(f)&&hand::same(accepted,d.lease);
    if(!d.new_grant&&!accepted_here)return false;
    if(!bolt::begin(f,d,direction,accepted_here?accepted:hand::Lease{}))return false;
    held=d.lease;accepted={};accepted_intent=0;return true;
}
#include "weapon_longshot_insertion.inl"
void guide(hand::Frame const& f) noexcept;
bool tick(uptr pc) noexcept {
    if(!weaponhand::current_gt(nullptr))return false;
    const TransitionReport transition{};
    if(stage==Stage::recovering){cleanup();return true;}
    if(stage==Stage::completing){
        if(!native_exited){
            // Closure and inventory are already committed. A temporarily
            // unavailable owned exit owes the SAME completion, not a rollback
            // whose perfect receipt cannot yet have been awarded. Retry only
            // these idempotent cleanup steps before dispatching the reward.
            if(!round::hide()||(serial&&!lancerreload_state::recover_serial(serial))||!longshot_clip::restore())return true;
            native_exited=true;return true;
        }
        {
            if(completion.result==gears_weapon::ReloadTxnResult::perfect&&!lancerreload_state::perfect_reward(completion,opening,observer,reward))return true;
            inv::Inventory counts{};if(!observer.sample_owned(completion.identity,counts))return true;
            if(!transaction.acknowledge({completion.token,true,counts,completion.result})){cleanup();return true;}
        }
        if(!round::restore())return true;
        if(debt_slot>=0)debts[unsigned(debt_slot)]={};
        if(cycle_guide_shot.event)completed_guide_shot=cycle_guide_shot;
        if(log_file){fwprintf(log_file,L"longshot_reload_complete serial=%llu result=%u standalone=%u\n",serial,unsigned(completion.result),unsigned(standalone));fflush(log_file);}
        reset();return true;
    }
    // A physically committed closure owns its remaining exit/reward even
    // during stop/switch. Neither path may begin another physical action.
    if(lancerreload_runtime::stop_requested.load(std::memory_order_acquire)||switch_requested){
        if(stage!=Stage::idle){cleanup();return true;}return false;}
    const unsigned shared_busy=unsigned(lancerreload_state::lease.acquired)|
        (unsigned(hammerburst_animation::ownership.active)<<1)|(unsigned(lancerreload_visual::active)<<2);
    if(stage==Stage::idle&&shared_busy){
        static unsigned rows{},previous{~0u};
        if(log_file&&nativelancerfollow::bound_is_longshot&&rows<8&&previous!=shared_busy){
            fwprintf(log_file,L"longshot_reload_blocked mask=%u at_ns=%lld\n",shared_busy,weaponhand::clock_ns(nullptr));
            fflush(log_file);++rows;previous=shared_busy;
        }return false;
    }
    hand::Frame f{};hand::GripGameState game{};hand::Decision d{};
    AcquiringTrace acquisition_trace(f);
    const bool copied=weaponhand::client.copy_frame_on_gt(f,game,d);acquisition_trace.copied=copied;static bool unavailable_logged{};
    if(!copied||f.controller.address!=pc){
        static unsigned rows{};
        if(log_file&&nativelancerfollow::bound_is_longshot&&!unavailable_logged&&rows<16){
            fwprintf(log_file,L"longshot_reload_frame_unavailable copied=%u stage=%u at_ns=%lld\n",
                unsigned(copied),unsigned(stage),weaponhand::clock_ns(nullptr));fflush(log_file);++rows;
        }unavailable_logged=true;return stage!=Stage::idle;
    }
    unavailable_logged=false;
#if __has_include("weapon_source_validation_scope.inl")
    std::optional<bolt::Validation> active_source_validation;
    if(selected(f))active_source_validation.emplace(f);
#endif
    acquisition_trace.point=2;
    report_input(f,game,&d);
    if(!selected(f)||!game.known||!game.main_hand_held||game.busy||game.animation_owns_hands||
       (stage!=Stage::idle&&!(opening.identity==inv::identity(f)))){
        if(stage!=Stage::idle){cleanup();return true;}proposal_known=false;return false;}
    // While an actual stroke is active, run the existing original HUD query
    // before the fresh contact/bone observation. The immediately following
    // bolt sample can then consume it once, with no intervening Engine call.
    const bool borrow_bones=(stage==Stage::opening||stage==Stage::closing)&&bolt::stroke.active();
    bool ui_checked{};
    if(borrow_bones&&serial&&!standalone){
        if(!lancerreload_state::tick_ui_timing()){cleanup();return true;}
        ui_checked=true;
    }
    bolt::BonesReceipt bone_receipt{};
    acquisition_trace.point=3;
    if(!bind_contact(f,borrow_bones?&bone_receipt:nullptr))return true;
    acquisition_trace.point=4;
    // Optional cues run after this action and the current follow plan in the
    // equipment backend. Do not spend this physical intent's lifetime drawing.
    if(stage==Stage::idle){
        // Opening an empty bolt does not acquire or display a cartridge.
        if(!bolt::granted(f,d)||
           (last_intent_identity==inv::identity(f)&&f.press_token<=last_intent)||
           !(d.new_grant||(accepted_intent==f.press_token&&accepted_identity==inv::identity(f)))){entry_gate(f,1);return true;}
        if(!longshot_scope_owner::yield(f))return true;
        inv::Inventory counts{},cycle_opening{};inv::ObservedInventory cycle_observer=observer;
        inv::Transaction resumed_transaction;lancerreload_state::RewardReceipt qualified_reward{};
        std::int64_t cycle_start{};bool timed{};std::uint64_t retained_serial{};
        GuideShot cycle_cue{};
        debt_slot=debt_for(inv::identity(f));standalone=debt_slot>=0;
        if(standalone){
            // This is still the SAME one-round transaction. Rebind only a
            // positively matching instance/hold with unchanged actual counts,
            // then require this new real activation to resume its ledger.
            const auto& saved=debts[unsigned(debt_slot)];if(!saved.saved)return true;
            cycle_observer=saved.observer;resumed_transaction=saved.transaction;
            if(!cycle_observer.rebind_hold(f,saved.identity,counts)||counts.capacity!=1||
               !resumed_transaction.rebind_suspended(saved.identity,counts)||
               !resumed_transaction.resume(counts,f.press_token,saved.serial))return true;
            cycle_opening=saved.opening;cycle_opening.identity=counts.identity;
            qualified_reward=saved.reward;cycle_start=saved.started_us;timed=saved.shot_timed;retained_serial=saved.serial;
            cycle_cue=saved.guide_shot;
        }else{
            const bool sampled=cycle_observer.sample(f,counts);
            if(sampled&&counts.capacity==1&&!counts.loaded&&!counts.infinite_reserve&&!counts.reserve)note_no_ammo(f);
            else if(sampled)no_ammo_cue.active=false;
            if(!sampled||counts.capacity!=1||counts.loaded||(!counts.infinite_reserve&&!counts.reserve)){entry_gate(f,2);return true;}
            if(!lancerreload_state::capture_reward_eligibility(counts,qualified_reward)||!clock(f,cycle_start)){entry_gate(f,3);return true;}
            const auto& shot=longshot_fire::confirmed;
            // The confirmed shot clock is an immutable timing fact. A new
            // hold of the same fully matching instance may retain it; this
            // never revives the old hand lease, input or geometry witness.
            timed=shot.active&&inv::same_instance_across_hold(shot.identity,counts.identity)&&
                shot.shot.confirmed&&shot.shot.game_tick<=cycle_start;
            if(timed)cycle_start=shot.shot.game_tick;
            if(timed)cycle_cue={shot.identity,shot.shot.original_event_sequence};
            cycle_opening=counts;
        }
        observer=cycle_observer;opening=cycle_opening;reward=qualified_reward;started_us=cycle_start;shot_timed=timed;
        cycle_guide_shot=cycle_cue;
        held=d.lease;accepted=d.lease;accepted_intent=f.press_token;accepted_identity=counts.identity;
        last_intent=f.press_token;last_intent_identity=counts.identity;
        stage=Stage::acquiring;ui_prepare_frame=0;serial=retained_serial;animation_serial=f.press_token;lancerreload_runtime::outstanding.store(true,std::memory_order_release);
        if(standalone){transaction=resumed_transaction;}
        else{
            if(!transaction.bind(counts)||lancerreload_state::begin(f,serial)!=lancerreload_state::BeginResult::acquired){entry_gate(f,4);cleanup();return true;}
            animation_serial=serial;ui_pending=true;
            if(timed)lancerreload_state::lease.reward_start=float(double(started_us)/1000000.);
            if(!transaction.begin(counts,f.press_token,gears_weapon::ReloadIntentSource::mechanism_open,serial,gears_weapon::ReloadTxnMode::round,1,started_us)){entry_gate(f,5);cleanup();return true;}
        }
        acquisition_trace.point=5;
        const auto result=longshot_clip::start(f,animation_serial,!standalone);
        if(result==longshot_clip::Result::rejected||result==longshot_clip::Result::recovery_required){entry_gate(f,6);cleanup();}return true;
    }
    if(!f.left_tracked){cleanup();return true;}
    if(stage==Stage::acquiring){
        if(!f.left_grip_down||!bolt::granted(f,d)||!hand::same(held,d.lease)){cleanup();return true;}
        acquisition_trace.point=6;
        const auto clip=longshot_clip::service(f);
        if(clip==longshot_clip::Result::rejected||clip==longshot_clip::Result::recovery_required){entry_gate(f,7);cleanup();return true;}
        acquisition_trace.point=7;
        if(clip!=longshot_clip::Result::ready||!bolt::prepare_serviced(f)){entry_gate(f,7);return true;}
        acquisition_trace.point=8;
        if(ui_pending){
            // First curve decode/closed-bone binding can consume this input's
            // lifetime. Defer HUD work to a later REAL Frame, which takes the
            // normal fresh source, held lease and clip checks above again.
            // Repeated calls in the preparation Frame cannot advance it.
            if(!ui_prepare_frame)ui_prepare_frame=f.key.frame;
            if(f.key.frame<=ui_prepare_frame){entry_gate(f,9);return true;}
            if(!lancerreload_state::set_ui_timing(f,serial,4000,4000,true)||!longshot_clip::acknowledge_ui_start(serial)){entry_gate(f,10);cleanup();return true;}
            ui_pending=false;return true;
        }
        acquisition_trace.point=9;
        if(begin_stroke(f,d,gears_weapon::longshot_tracks::BoltDirection::opening)){
            stage=Stage::opening;
            report_contact(f,true);
        }else entry_gate(f,8);return true;
    }
    if(!ui_checked&&serial&&!standalone&&!lancerreload_state::tick_ui_timing()){cleanup();return true;}
    if(stage==Stage::opening||stage==Stage::closing){
        if(!f.left_grip_down){held={};bolt::stroke.release();return true;}
        if(!bolt::stroke.active()){
            begin_stroke(f,d,stage==Stage::opening?gears_weapon::longshot_tracks::BoltDirection::opening:gears_weapon::longshot_tracks::BoltDirection::closing);return true;}
        const auto result=bolt::tick(f,d,borrow_bones?&bone_receipt:nullptr);
        if(result==gears_weapon::longshot_tracks::BoltResult::interrupted){held={};return true;}
        if(result!=gears_weapon::longshot_tracks::BoltResult::complete)return true;
        held={};
        if(stage==Stage::opening){stage=standalone?Stage::waiting_close:Stage::waiting_round;return true;}
        std::int64_t closed{};if(!clock(f,closed)){cleanup();return true;}
        if(!transaction.confirm_closed(closed,shot_timed?gears_weapon::longshot_perfect_duration_us-1:0)||!transaction.take_request(completion)){cleanup();return true;}
        if(log_file){
            fwprintf(log_file,L"longshot_reload_closed serial=%llu weapon=%llx timed=%u shot_tick_us=%lld closed_tick_us=%lld elapsed_us=%lld request_result=%u shot_event=%llu\n",
                serial,static_cast<unsigned long long>(f.weapon.address),unsigned(shot_timed),started_us,closed,
                closed-started_us,unsigned(completion.result),cycle_guide_shot.event);fflush(log_file);
        }
        stage=Stage::completing;return true;
    }
    if(stage==Stage::waiting_round){
        // A real shoulder pickup still needs the current prepared resource.
        // Readiness recovery cannot replay a press that was already rejected.
        if(!round::ready||!round::current(f)||!d.new_grant||!round::granted(f,d))return true;
        if(!transaction.reserve_item(d.lease.id)){cleanup();return true;}
        if(!round::capture_shoulder(f,d,d.lease.id)){transaction.release_item(d.lease.id);return true;}
        held=d.lease;stage=Stage::holding_round;last_feed_ns=0;feed_sequence=0;approached=false;return true;
    }
    if(stage==Stage::holding_round){
        if(!f.left_grip_down){
            const auto item=round::item_serial;
            if(!transaction.release_item(item)||!round::hide()){cleanup();return true;}held={};stage=Stage::waiting_round;return true;}
        // The same actual Frame supplies round rendering, hand target and raw
        // tip measurement. Loss cannot retain the old approach stroke.
        if(contact_frame!=f.key.frame||contact_sync_ns!=f.controllers.sync_ns||
           !round::prepare_presentation(f,d,contact_root_world,port_local,root_scale,animation_serial)||!round::show(f,d)){
            last_feed_ns=0;feed_sequence=0;approached=false;return true;
        }
        if(!inserted(f,d))return true;
        debt_slot=debt_for(opening.identity,true);if(debt_slot<0){cleanup();return true;}
        inv::Request request{};inv::Receipt receipt{};
        if(!transaction.request_insertion(round::item_serial)||!transaction.take_request(request)){cleanup();return true;}
        const auto result=writer.commit(f,request,observer,lancerreload_state::owns,receipt);
        if(!receipt.token){receipt.token=request.token;observer.sample(f,receipt.observed);}
        if(result!=inv::CommitResult::committed||!transaction.acknowledge(receipt)){cleanup();return true;}
        if(!round::hide()){cleanup();return true;}held={};stage=Stage::waiting_close;return true;
    }
    if(stage==Stage::waiting_close&&begin_stroke(f,d,gears_weapon::longshot_tracks::BoltDirection::closing))stage=Stage::closing;
    return true;
}
// Service an existing old owner's completion/restoration only. Its pending
// result must not select or block another weapon's current candidate.
void service_unselected(uptr pc,uptr selected) noexcept {
    if(stage!=Stage::idle&&opening.identity.weapon.address!=selected)tick(pc);
}
} // namespace longshot_runtime
