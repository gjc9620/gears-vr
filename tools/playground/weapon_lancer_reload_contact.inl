// Read-only candidate producer inside the existing hand participant. Never
// calls the client (it already holds its gate), setters or inventory functions.
namespace lancerreload_contact {
namespace hand=gears_weapon_hand;
bool authored_profile_current(hand::Frame const& f) noexcept {
    // Association only; the caller still proves its fresh visual/input frame.
    // A shared reload adapter must not apply this Lancer export to another gun.
    return nativelancerfollow::bound_key_known&&nativelancerfollow::bound_is_lancer&&
        hand::same(nativelancerfollow::bound_key.weapon,f.weapon)&&
        nativelancerfollow::bound_key.hold_generation==vrequip::model.hold_generation();
}
gears_weapon::LancerContactRoute mechanism_route;
bool dispatch_enabled{true}; // Same production consumer; package promotion follows runtime evidence.
bool waiting_ammo{};
// A known empty shoulder attempt retires only that real press. It must not
// keep publishing an object candidate/Grab Pose for a nonexistent magazine.
gears_weapon::ReloadSupplyRejection empty_supply;
bool resume_original{};
unsigned sample_failure{},match_status{};
// Existing once-per-press receipt only: already-read POD, no extra sampler.
struct CohortFailure {unsigned why{},control{},old_active{},new_active{},old_press{},new_press{},old_release{},new_release{},old_down{},new_down{};
    std::uint64_t old_sequence{},new_sequence{};std::int64_t old_sync{},new_sync{};};
CohortFailure cohort_failure;
std::int64_t last_geometry_ns{};
gears_committed_interaction::Frame last_actual{};
gears_vr::Vector last_magazine_position{};
gears_weapon::LancerInsertionGeometry last_geometry{};
hand::Lease held{};
lancerreload_inventory::Identity held_identity{};
std::uint64_t held_after_frame{};
bool preparing{};std::int64_t preparation_until{};
// A successful seat retires the ammo transaction, not the player's still-held
// grasp. Keep only its SAME incumbent for contextual skin until real release.
// This cannot grant another extraction, replacement or inventory operation.
bool seated_hold{};
bool retain(hand::Frame const& source,hand::Decision const& decision) noexcept {
    if(!lancerreload_visual::active||!hand::same(source.weapon,lancerreload_visual::saved.weapon)||
       !hand::same(source.pawn,lancerreload_visual::saved.pawn)||
       !hand::same(source.key,decision.key)||decision.release_required||
       (decision.status!=hand::DecisionStatus::granted&&decision.status!=hand::DecisionStatus::retained)||
       (decision.lease.kind!=hand::LeaseKind::magazine&&decision.lease.kind!=hand::LeaseKind::ammo)||
       !hand::lease_valid(decision.lease,source.key.lifecycle)||decision.lease.id!=source.press_token)return false;
    const auto identity=lancerreload_inventory::identity(source);
    if(!identity.valid())return false;
    preparing=false;preparation_until=0;
    if(hand::same(held,decision.lease)&&held_identity==identity)return true;
    held=decision.lease;held_identity=identity;held_after_frame=source.key.frame;return true;
}
void release() noexcept {held={};held_identity={};held_after_frame=0;preparing=false;preparation_until=0;seated_hold=false;}
bool retained(hand::Frame const& source,hand::Decision const& decision) noexcept {
    return held_after_frame&&source.key.frame>held_after_frame&&hand::same(held,decision.lease)&&
        held.id==source.press_token&&held_identity==lancerreload_inventory::identity(source)&&
        decision.status==hand::DecisionStatus::retained&&!decision.release_required;
}
void committed(hand::Frame const& source,hand::Decision const& decision) noexcept {
    // A magazine-kind grant may belong to a receiver or starter instead.
    // Never dispatch a new mechanism grant without its exact contact purpose.
    if(decision.new_grant&&decision.lease.kind==hand::LeaseKind::magazine){
        if(mechanism_route.grants(source,decision,gears_weapon::LancerContact::starter)){
            lancersaw_starter::committed(source,decision);return;
        }
        if(mechanism_route.grants(source,decision,gears_weapon::LancerContact::charging)){
            lancer_charging::committed(source,decision);return;
        }
        if(!mechanism_route.grants(source,decision,gears_weapon::LancerContact::magazine))return;
    }
    // Preserve only a newly arbitrated magazine grant while the ordinary GT
    // preflight prepares it. A later current sample must still be at contact;
    // no component, ammunition or old pose is owned by this preparation.
    if(!lancer_charging::enabled&&!waiting_ammo&&!resume_original&&!lancerreload_visual::active&&
       decision.status==hand::DecisionStatus::granted&&decision.new_grant&&!decision.release_required&&
       decision.lease.kind==hand::LeaseKind::magazine&&hand::same(decision.key,source.key)&&
       decision.lease.id==source.press_token&&hand::lease_valid(decision.lease,source.key.lifecycle)&&
       source.controllers.sync_ns>0&&source.controllers.sync_ns<=INT64_MAX-250000000){
        held=decision.lease;held_identity=lancerreload_inventory::identity(source);held_after_frame=source.key.frame;
        preparing=true;preparation_until=source.controllers.sync_ns+250000000;
        mechanism_route.remember_grant(source,decision,gears_weapon::LancerContact::magazine);
    }
    // A grant can precede PlayerTick by an entire owner frame. Record its
    // existing lease now, without acquiring a component or consuming ammo.
    // The same frozen frame must still observe its original shoulder request.
    if(waiting_ammo&&decision.status==hand::DecisionStatus::granted&&decision.new_grant&&
       decision.lease.kind==hand::LeaseKind::ammo)retain(source,decision);
    // The same early-commit handoff is required when a tracked release/new
    // contact regrabs the retained ORIGINAL magazine after an interruption.
    // Otherwise Native can retire the unreported new lease before PlayerTick
    // reaches the suspended consumer. This only preserves this exact already
    // arbitrated grant; regrasp_original still authenticates the actual item,
    // fresh physical pair and insertion observer before any mechanical work.
    if(resume_original&&decision.status==hand::DecisionStatus::granted&&decision.new_grant&&
       decision.lease.kind==hand::LeaseKind::magazine&&retain(source,decision))
        mechanism_route.remember_grant(source,decision,gears_weapon::LancerContact::magazine);
}
bool completed_sample(gears_committed_interaction::Frame const& actual,bool grip_down,
    gears_weapon::LancerInsertionSample& out) noexcept {
    out={};
    sample_failure=1;
    const auto& r=actual.right;
    if(!nativelancerfollow::bound_key_known||!nativelancerfollow::bound_is_lancer||!nativelancerfollow::mount.bound()||
       !hand::same(r.weapon,nativelancerfollow::bound_key.weapon)||
       nativelancerfollow::bound_key.hold_generation!=vrequip::model.hold_generation()||
       r.hold_generation!=vrequip::model.hold_generation()||
       nativelancerfollow::bound_session!=r.stereo.session||
       nativelancerfollow::bound_space!=r.stereo.space_epoch||
       nativelancerfollow::bound_input!=r.controllers.input_epoch||
       nativelancerfollow::bound_reference!=r.reference_generation||
       nativelancerfollow::bound_world!=r.parent.world_generation)return false;
    // Fixed-length IK can project either palm away from its raw controller.
    // Use the actual, still-current pair also used by the visible held gun.
    // Input buttons remain owned by the caller's authenticated association;
    // geometry keeps the completed pose's original acquisition/sync time.
    sample_failure=2;
    for(const auto& grip:r.controllers.hands)
        if(!gears_vr::controller_pose_usable(grip.grip,grip.grip_flags,grip.grip_active))return false;
    if(!lancerreload_visual::lancer_sample(actual,grip_down,out))return false;
    sample_failure=0;
    last_geometry_ns=actual.right.acquisition_begin_ns;
    last_actual=actual;last_magazine_position=out.magazine.position;
    return out.created_ms&&out.sequence;
}
bool sample(hand::Frame const& f,gears_weapon::LancerInsertionSample& out,bool* observation_pending=nullptr) noexcept {
    out={};sample_failure=2;cohort_failure={};
    if(observation_pending)*observation_pending=false;
    gears_committed_interaction::Frame actual{};
    if(!lancerreload_visual::interaction(f,actual)){
        if(observation_pending)*observation_pending=lancerreload_visual::interaction_failure==2||lancerreload_visual::interaction_failure==3;
        return false;
    }
    if(!gears_committed_interaction::same_left_control_cohort(actual,f)){
        auto const& a=actual.right.controllers;auto const& b=f.controllers;
        const auto control=hand::left_control(f);auto const& old=a.hands[0];auto const& now=b.hands[0];
        auto& d=cohort_failure;d.control=control;d.old_sequence=a.sequence;d.new_sequence=b.sequence;
        d.old_sync=a.sync_ns;d.new_sync=b.sync_ns;
        d.old_active=control==gears_vr::control_trigger?old.trigger_active:old.squeeze_active;
        d.new_active=control==gears_vr::control_trigger?now.trigger_active:now.squeeze_active;
        d.old_press=old.pressed[control];d.new_press=now.pressed[control];
        d.old_release=old.released[control];d.new_release=now.released[control];
        d.old_down=hand::control_held(old,control);d.new_down=hand::control_held(now,control);
        // Classify the existing failed predicate; do not alter its decision.
        d.why=f.interaction_control>unsigned(gears_vr::LeftInteractionControl::pinch)?1u:
            a.source!=b.source||a.session!=b.session||a.space_epoch!=b.space_epoch||
            a.input_epoch!=b.input_epoch||a.profile_generation!=b.profile_generation?2u:
            a.sequence>b.sequence?3u:a.sync_ns>b.sync_ns?4u:
            d.old_active!=1||d.new_active!=1?5u:d.old_press!=d.new_press?6u:
            d.old_release!=d.new_release?7u:d.old_down!=d.new_down?8u:9u;
        sample_failure=3;if(observation_pending)*observation_pending=true;return false;
    }
    return completed_sample(actual,f.left_tracked&&f.left_grip_known&&f.left_grip_down,out);
}
bool retained_observation(gears_committed_interaction::Frame const& actual,hand::RetainedItemInput const& input,
    lancerreload_inventory::Identity const& inventory) noexcept {
    return held_after_frame&&actual.right.parent.frame>held_after_frame&&
        hand::same(held,input.lease)&&held.id==input.press_token&&held_identity==inventory&&
        inventory==lancerreload_inventory::identity(actual)&&
        gears_committed_interaction::matches_retained_item(actual,input,vrequip::model.hold_generation(),
            weaponhand::clock_ns(nullptr),actual.right.stereo.source==gears_vr::StereoSource::MetaOperatorSimulated);
}
void propose(hand::Frame const& f,hand::GripGameState& game) noexcept {
    // First-frame handoff: the real grant already passed prepare/commit.
    // Keep its original proposal while our own display reparent/move occurs;
    // never promote an incumbent until a later owner frame. Actual current
    // identity/game/input/freshness checks still run in the caller, and this
    // exact-source receipt cannot acquire a different object or another press.
    if(dispatch_enabled&&game.known&&game.main_hand_held&&!game.busy&&!game.animation_owns_hands&&
       game.kind==hand::WeaponKind::firearm&&held_after_frame==f.key.frame&&
       held_identity==lancerreload_inventory::identity(f)&&
       (preparing||lancerreload_visual::active)&&
       mechanism_route.preserves_granted_proposal(f,held,gears_weapon::LancerContact::magazine)){
        game.request_magazine=true;game.magazine_observation_pending=false;return;
    }
    struct Tag {
        hand::Frame const& frame;hand::GripGameState& game;
        gears_weapon::LancerContact purpose{gears_weapon::LancerContact::magazine};
        double magazine_distance{1e9};
        ~Tag(){
            // Owner auto-socket contract: a neutral hand already selected a
            // known support socket. On the real Grip edge, its CURRENT contact
            // may drive that socket even if the unrelated magazine pair has
            // not completed. No known magazine/ammo request, existing C socket, incumbent,
            // changed hold/basis, stale input or cold held press is bypassed.
            // Affinity returns no pose or lease; Native still arbitrates and
            // revalidates the exact current support proposal at commit.
            const bool support_selected=lancer_grasp::support_socket_preferred(frame,game);
            if(game.magazine_observation_pending&&support_selected){
                game.magazine_observation_pending=false;
                mechanism_route.propose(frame,0);return;
            }
            lancersaw_starter::observe_route(frame,game.magazine_observation_pending,lancer_grasp::selected_starter(frame));
            if(seated_hold)return; // No new mechanism proposals on this consumed press.
            if(game.magazine_observation_pending){
                // A selected C socket has its OWN current geometry/contact
                //proposal. An unavailable unrelated magazine pair cannot
                //consume that real Grip before this known part is considered.
                //Selection alone never grants anything: the unchanged C
                //proposal and Native commit still prove the actual contact.
                if(lancer_grasp::selected_starter(frame)&&lancersaw_starter::propose(frame,game,nullptr)){
                    const auto part=mechanism_route.propose(frame,unsigned(gears_weapon::LancerContact::starter));
                    game.request_magazine=part==gears_weapon::LancerContact::starter;
                    if(game.request_magazine)game.magazine_observation_pending=false;
                    return;
                }
                // Preserve only this press's bounded intent. Do not turn an
                // unknown magazine pair/unknown C contact into a new action.
                // An already owned C grasp is independent of that magazine
                // observation. Omission here would manufacture a lease loss
                // while the authenticated initiating control remains held.
                lancersaw_starter::retain_incumbent(frame,game);
                mechanism_route.propose(frame,0);return;
            }
            unsigned contacts=game.request_magazine?unsigned(purpose):0u;
            double starter_distance{};
            const bool starter_available=lancersaw_starter::propose(frame,game,&starter_distance);
            const auto resolved=gears_weapon::resolve_lancer_starter_proposal(contacts,
                lancer_grasp::selected_starter(frame),starter_available,magazine_distance,starter_distance);
            contacts=resolved.candidates;
            if(resolved.observation_pending){
                game.request_magazine=false;game.magazine_observation_pending=true;
                mechanism_route.propose(frame,0);return;
            }
            const auto selected=mechanism_route.propose(frame,contacts);
            game.request_magazine=selected!=gears_weapon::LancerContact::none&&selected!=gears_weapon::LancerContact::conflict;
        }
    } tag{f,game};
    if(seated_hold){
        if(!dispatch_enabled||!game.main_hand_held||game.animation_owns_hands||
           !f.left_tracked||!f.left_grip_known||!f.left_grip_down||
           !held_after_frame||held.id!=f.press_token||!hand::lease_valid(held,f.key.lifecycle)||
           !(held_identity==lancerreload_inventory::identity(f))){release();return;}
        for(auto& incumbent:game.object_incumbents){
            if(hand::same(incumbent,held))return;
            if(incumbent.kind==hand::LeaseKind::none){incumbent=held;return;}
        }
        return; // Never replace another hand owner.
    }
    // Owner correction: magazine/replacement accept either logical control;
    // the existing Native adapter retains the initiating control and token.
    // Contact selects the part's hand pose, independently of either button.
    if(lancer_charging::enabled&&lancer_charging::required(f)){
        tag.purpose=gears_weapon::LancerContact::charging;
        lancer_charging::propose(f,game);return;
    }
    if(!dispatch_enabled||!game.main_hand_held||game.busy||game.animation_owns_hands||
       game.kind!=hand::WeaponKind::firearm||!f.left_tracked||!f.left_grip_down)return;
    // Once an actual component is in hand, contact with the gun is no longer
    // required. Only retain the exact grant, never create one from proximity.
    const auto now=weaponhand::clock_ns(nullptr);
    const bool preparation_live=preparing&&now>0&&now<=preparation_until;
    if((lancerreload_visual::active||preparation_live)&&(held.kind==hand::LeaseKind::magazine||held.kind==hand::LeaseKind::ammo)&&
       held_after_frame&&f.key.frame>held_after_frame&&
       hand::lease_valid(held,f.key.lifecycle)&&held.id==f.press_token&&
       held_identity==lancerreload_inventory::identity(f)){
        for(auto& incumbent:game.object_incumbents){
            if(hand::same(incumbent,held))return;
            if(incumbent.kind==hand::LeaseKind::none){incumbent=held;return;}
        }
        return; // Preserve existing conflict, never overwrite another holder.
    }
    if(waiting_ammo){
        if(empty_supply.rejected(f.key.lifecycle,f.press_token))return;
        if(!lancerreload_visual::active||!hand::same(f.weapon,lancerreload_visual::saved.weapon)||
           !hand::same(f.pawn,lancerreload_visual::saved.pawn))return;
        const gears_vr::Pose body{f.stereo.head.position,f.reference.orientation};
        const auto& left=f.controllers.hands[0];
        if(gears_vr::controller_pose_usable(left.grip,left.grip_flags,left.grip_active))
            game.request_ammo=gears_lancer_workbench_profile::shoulder_supply_enabled&&authored_profile_current(f)?
                gears_lancer_shoulder_supply::contains(
                    {gears_lancer_workbench_profile::shoulder_adjust,gears_lancer_workbench_profile::shoulder_scale},
                    body,left.grip.position):
                gears_weapon::body_slot(nativeequipment::calibration,body,left.grip.position,0)==gears_weapon::BodySlot::shoulder_ammo;
        return;
    }
    if(resume_original){
        // Read-only prepared publication: do not call the hand client while
        // its arbitration gate is held. Regrab the frozen visible old magazine,
        // not the now empty port, and never carry an old tracking pose forward.
        if(!lancerreload_visual::active||!lancerreload_visual::last_grab_known||
           !hand::same(f.weapon,lancerreload_visual::saved.weapon)||
           !hand::same(f.pawn,lancerreload_visual::saved.pawn))return;
        gears_committed_interaction::Frame actual{};
        if(!lancerreload_visual::interaction(f,actual))return;
        const auto p=actual.physical_left_grip.position,q=lancerreload_visual::last_grab_world;
        const double dx=p.x-q.x,dy=p.y-q.y,dz=p.z-q.z,
            radius=gears_weapon::lancer_contact::retained_magazine_radius*actual.right.units_per_metre;
        tag.magazine_distance=std::hypot(std::hypot(dx,dy),dz)/actual.right.units_per_metre;
        game.request_magazine=dx*dx+dy*dy+dz*dz<=radius*radius;
        return;
    }
    gears_weapon::LancerInsertionSample s{};gears_weapon::LancerInsertionGeometry geometry{};
    bool observation_pending{};
    if(!sample(f,s,&observation_pending)){
        game.magazine_observation_pending=observation_pending&&f.left_grip_down&&f.left_grip_known&&f.left_tracked;return;
    }
    if(!gears_weapon::lancer_insertion_geometry(s,geometry))return;
    last_geometry=geometry;
    game.request_magazine=gears_weapon::lancer_magazine_grab(s.gun_mesh,
        last_actual.physical_left_grip.position,last_actual.right.units_per_metre,&tag.magazine_distance,
        authored_profile_current(f)?gears_lancer_workbench_profile::magazine():nullptr);
    if(game.request_magazine){
        double support_distance{};
        if(hand::support_contact(f,game,false,&support_distance)&&support_distance<tag.magazine_distance)
            game.request_magazine=false;
    }
}
}
