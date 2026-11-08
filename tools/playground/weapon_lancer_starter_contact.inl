// Starter contact/pull and the normal-GT original input handoff. Actual motor,
// target/execution and visual recovery require separate game observations;
// submitted gesture/input alone never establishes attack or damage.
namespace lancersaw_starter {
namespace hand=gears_weapon_hand;namespace inv=lancerreload_inventory;
using Phase=gears_weapon::LancerSawPhase;using Event=gears_weapon::LancerSawEvent;
hand::Lease held{};inv::Identity identity{};
std::uint64_t granted_frame{};std::int64_t last_tick_ns{},completed_ns{};
gears_weapon::LancerChainsawInteraction interaction;
bool pulling{},pull_complete{};double shown_pull{};
bool locked_rail{},cosmetic_nearby{};inv::Identity cosmetic_identity{};
inv::Identity rail_identity{};std::uint64_t rail_hold{};
gears_hand_object_grasp::ReleasedSocket released_socket;
bool released_socket_preferred(hand::Frame const& f) noexcept {
    if(!locked_rail||!(rail_identity==inv::identity(f))) {released_socket.clear();return false;}
    return released_socket.retained(f,vrequip::model.hold_generation());
}
unsigned events{},sample_rows{},observed_gate{},observed_matrix_gate{};std::uint64_t previous_sample_ms{};
unsigned proposal_gate{},proposal_observed_gate{},proposal_matrix_gate{},ungranted_rows{};std::uint64_t proposal_frame{};
unsigned proposal_conditions{};
bool route_magazine_pending{},route_selected_socket{};std::uint64_t route_frame{};
void observe_route(hand::Frame const& f,bool magazine_pending_value,bool socket_selected_value) noexcept {
    route_frame=f.key.frame;route_magazine_pending=magazine_pending_value;route_selected_socket=socket_selected_value;
}
// Association/pin for an ALREADY acquired C grasp. A missing rendered Frame
// is not a physical release. This never provides a pose, new action or edge.
hand::Frame grasp_parent{};hand::ExistingActionLifetime grasp_lifetime{};
gears_lancer_saw_input::HoldInput grasp_input;
std::uint64_t grasp_hold{};bool grasp_pin_known{};
bool input_continuity_known{};std::uint64_t input_continuity_sequence{};unsigned continuity_rows{};

bool idle_reload(hand::Frame const& f) noexcept {
    return lancerreload_runtime::stage==lancerreload_runtime::Stage::idle&&
        !lancerreload_runtime::needs_priority()&&!lancerreload_contact::preparing&&
        !lancerreload_visual::active&&!lancerreload_state::lease.acquired&&!lancer_charging::required(f);
}
bool same_hold(hand::Frame const& f) noexcept {
    return granted_frame&&identity==inv::identity(f)&&held.id==f.press_token&&
        held.kind==hand::LeaseKind::magazine&&hand::lease_valid(held,f.key.lifecycle);
}
bool observed(hand::Frame const& f,gears_weapon::LancerSawSample& sample,bool current_grip) noexcept {
    sample={};observed_gate=1;observed_matrix_gate=0;
    if(!lancersaw_visual::ready||!lancersaw_visual::visible||!lancersaw_visual::same_binding(f))return false;
    lancersaw_visual::Observation handle{};
    observed_gate=2;if(!lancersaw_visual::observe(f,handle))return false;
    gears_committed_interaction::Frame actual{};
    observed_gate=4;if(!lancerreload_visual::interaction(f,actual))return false;
    const double units=actual.right.units_per_metre;
    observed_gate=5;if(!std::isfinite(units)||units<10||units>500)return false;
    const auto rest=handle.rest,visible_grasp=handle.grasp;
    const auto left=actual.physical_left_grip.position;
    // Acquisition uses the ACTUALLY visible crossbar. Motion then uses its
    // fixed socket/rest point, so moving the handle cannot reset pull travel.
    const bool attached=(cosmetic_nearby&&cosmetic_identity==inv::identity(f))||released_socket_preferred(f);
    const double acquire=attached?.22:gears_weapon::lancer_contact::starter_radius;
    observed_gate=7;if(!current_grip&&std::hypot(std::hypot(left.x-visible_grasp.x,left.y-visible_grasp.y),left.z-visible_grasp.z)>
        acquire*units)return false;
    sample.world=f.world.address;sample.pawn=f.pawn.address;sample.weapon=f.weapon.address;
    sample.session=f.stereo.session;sample.space_epoch=f.stereo.space_epoch;sample.input_epoch=f.controllers.input_epoch;
    sample.world_generation=f.key.world_generation;sample.sequence=actual.right.controllers.sequence;
    sample.producer_stream=actual.right.producer.stream;
    sample.tracking_epoch[0]=actual.right.producer.tracking[0];sample.tracking_epoch[1]=actual.right.producer.tracking[1];
    observed_gate=8;if(actual.right.controllers.sync_ns<=0)return false;
    sample.created_ms=std::uint64_t(actual.right.controllers.sync_ns/1000000);
    sample.left_palm_m={left.x/units,left.y/units,left.z/units};
    sample.starter_anchor_m={rest.x/units,rest.y/units,rest.z/units};
    sample.pull_axis_m=handle.axis;sample.original_handle=true;sample.starter_actual_angle=handle.angle;
    sample.original_slide=true;sample.require_full_slide=true;
    sample.starter_actual_pull_m=handle.pull_raw*handle.scale/units;
    sample.starter_travel_m=gears_lancer_original_handle::maximum_pull_raw*handle.scale/units;
    sample.starter_grasp_m={handle.grasp.x/units,handle.grasp.y/units,handle.grasp.z/units};
    const auto d=gears_vr::Vector{left.x-rest.x,left.y-rest.y,left.z-rest.z};
    const auto axis=[&](unsigned column){const unsigned i=column*4;
        const double n=std::hypot(std::hypot(double(handle.gun[i]),double(handle.gun[i+1])),double(handle.gun[i+2]));
        return (d.x*handle.gun[i]+d.y*handle.gun[i+1]+d.z*handle.gun[i+2])/(n*units);};
    sample.local_rail=true;sample.rail_hand_m={axis(0),axis(1),axis(2)};
    sample.starter_pivot_m={handle.pivot.x/units,handle.pivot.y/units,handle.pivot.z/units};
    sample.lancer_active=sample.right_owns_weapon=sample.right_tracked=true;
    sample.left_tracked=f.left_tracked==1;sample.left_grip_down=f.left_grip_known&&f.left_grip_down;
    // Gesture eligibility only. The original action adapter must re-run the
    // real CanDoSpecialMeleeAttack on a fresh frame before it presses anything.
    sample.game_can_melee=true;
    observed_gate=9;if(!sample.sequence||!sample.created_ms||!gears_vr::finite(sample.left_palm_m))return false;
    observed_gate=0;return true;
}
bool retain_incumbent(hand::Frame const& f,hand::GripGameState& game) noexcept {
    if(!game.main_hand_held||game.kind!=hand::WeaponKind::firearm||!idle_reload(f))return false;
    return gears_weapon::retain_lancer_part_incumbent(f,held,granted_frame,
        identity==inv::identity(f),game.object_incumbents);
}
bool propose(hand::Frame const& f,hand::GripGameState& game,double* distance_metres) noexcept {
    if(distance_metres)*distance_metres=0;
    proposal_frame=f.key.frame;proposal_gate=1;proposal_observed_gate=proposal_matrix_gate=0;
    proposal_conditions=unsigned(game.main_hand_held)|(unsigned(game.kind==hand::WeaponKind::firearm)<<1)|
        (unsigned(idle_reload(f))<<2)|(unsigned(hand::index_pinch(f))<<3)|(unsigned(f.left_tracked!=0)<<4)|
        (unsigned(f.left_grip_known!=0)<<5)|(unsigned(f.left_grip_down!=0)<<6)|
        (unsigned(lancersaw_original::active())<<7)|(unsigned(lancersaw_original::prepared())<<8)|
        (unsigned(game.busy)<<9)|(unsigned(game.animation_owns_hands)<<10);
    proposal_gate=2; // Large C manipulation belongs to Grip only.
    if(!game.main_hand_held||game.kind!=hand::WeaponKind::firearm||
       !idle_reload(f)||hand::index_pinch(f)||!f.left_tracked||!f.left_grip_known||!f.left_grip_down)return false;
    if(same_hold(f)){
        if(f.key.frame==granted_frame)return true; // immutable original grant proposal
        retain_incumbent(f,game);
        return false;
    }
    // An EXISTING C grasp remains the same incumbent through the original
    // ready hold and its stop transition. This adds no new grant or Grip
    // edge, never overwrites another incumbent and still requires the exact
    // current identity/lifecycle/tracked held grip above. Reject new grabs
    // while original animation or other busy work owns the weapon.
    if(lancersaw_original::active()||!lancersaw_original::prepared()||game.busy||game.animation_owns_hands)return false;
    proposal_gate=3; // A released slider can be regrasped at its locked position.
    gears_weapon::LancerSawSample sample{};const bool available=observed(f,sample,false);
    if(available){
        double support_distance{};
        const auto d=gears_vr::Vector{sample.left_palm_m.x-sample.starter_grasp_m.x,
            sample.left_palm_m.y-sample.starter_grasp_m.y,sample.left_palm_m.z-sample.starter_grasp_m.z};
        const double starter_distance=std::hypot(std::hypot(d.x,d.y),d.z);
        if(distance_metres)*distance_metres=starter_distance;
        // Overlapping generous volumes choose the nearest large part. An
        // existing support/C lease remains owned until its ordinary release.
        if(!lancer_grasp::selected_starter(f)&&
           hand::support_contact(f,game,false,&support_distance)&&support_distance<starter_distance)return false;
    }
    proposal_observed_gate=observed_gate;proposal_matrix_gate=observed_matrix_gate;proposal_gate=available?0:4;return available;
}
void committed(hand::Frame const& f,hand::Decision const& decision) noexcept {
    // Caller has already matched the exact frozen STARTER purpose.
    if(!idle_reload(f)||!lancerreload_contact::mechanism_route.grants(f,decision,gears_weapon::LancerContact::starter)||
       f.controllers.sync_ns<=0||f.controllers.sync_ns>INT64_MAX-250000000)return;
    held=decision.lease;identity=inv::identity(f);granted_frame=f.key.frame;
    rail_identity=identity;rail_hold=vrequip::model.hold_generation();
    locked_rail=false;released_socket.clear();
}
void clear_grab(unsigned reason=0,hand::Frame const* source=nullptr) noexcept {
    // Extend the existing bounded gesture receipt; do not sample another
    // source or renew an input/gesture clock merely to explain a withdrawal.
    if(granted_frame&&events++<24&&log_file){fwprintf(log_file,
        L"lancer_starter_grab_clear reason=%u phase=%u frame=%llu age_ns=%lld pulling=%u complete=%u original_active=%u\n",
        reason,unsigned(interaction.phase()),source?source->key.frame:0,
        source?weaponhand::clock_ns(nullptr)-source->controllers.sync_ns:-1,
        unsigned(pulling),unsigned(pull_complete),unsigned(lancersaw_original::active()));fflush(log_file);}
    held={};identity={};granted_frame=0;completed_ns=0;pulling=pull_complete=false;interaction.reset();
    grasp_parent={};grasp_lifetime={};grasp_input.reset();grasp_hold=0;grasp_pin_known=false;}
bool pin_existing_grasp(hand::Frame const& f) noexcept {
    if(grasp_pin_known)return true;
    if(!pulling||!same_hold(f)||!nativeequipment::current(nullptr,f))return false;
    gears_hand_continuity::TrackedInput tracked{};gears_hand_continuity::Publisher publisher{};
    if(!nativeequipment::read_input_continuity(f,tracked,publisher)||
       !grasp_input.bind(f.stereo,tracked,publisher,weaponhand::clock_ns(nullptr),
           f.stereo.source==gears_vr::StereoSource::MetaOperatorSimulated,hand::left_control(f))||
       !weaponhand::client.capture_existing_action_lifetime_on_gt(f.key,grasp_lifetime))return false;
    grasp_parent=f;grasp_hold=vrequip::model.hold_generation();grasp_pin_known=true;return true;
}
bool retain_grasp_during_pose_gap(uptr pc) noexcept {
    // Only preserve the existing logical equipment hold. Input suppression
    // stays active; no missing Frame/geometry is reconstructed or refreshed.
    if(!grasp_pin_known||!pulling||!granted_frame||lancersaw_original::active()||
       pc!=grasp_parent.controller.address||!nativeequipment::domain_ready()||
       !weaponhand::current_gt(nullptr)||!nativeequipment::restoration_quiet()||
       grasp_hold!=vrequip::model.hold_generation()||
       !vrequip::model.held_for(grasp_parent.world.address,grasp_parent.pawn.address,grasp_parent.weapon.address)||
       !weaponhand::client.existing_action_lifetime_on_gt(grasp_lifetime))return false;
    const auto begin=weaponhand::clock_ns(nullptr);hand::GripGameState game{};
    if(begin<=0||!weaponhand::observe_source(nullptr,grasp_parent,begin,game)||
       !game.known||!game.main_hand_held||game.busy||game.animation_owns_hands)return false;
    gears_lancer_saw_input::Observation actual{};
    if(!lancersaw_original::context(lancersaw_original::actors(grasp_parent),actual)||
       !actual.actors_current||!actual.current_lancer||actual.special_move||actual.doing_special||actual.doing_melee)return false;
    gears_vr::StereoSample stereo{};gears_vr::ControllerInputFrame input{};
    gears_hand_continuity::TrackedInput tracked{};gears_hand_continuity::Publisher publisher{};
    const bool sim=grasp_parent.stereo.source==gears_vr::StereoSource::MetaOperatorSimulated;
    if(nativeequipment::input_continuity_reader.read(GetCurrentProcessId(),[](){return weaponhand::clock_ns(nullptr);},
       sim,stereo,input,tracked,publisher)!=gears_current_tracking::Failure::none)return false;
    const auto end=weaponhand::clock_ns(nullptr);
    return end>=begin&&end-begin<=50000000&&grasp_input.refresh(stereo,tracked,publisher,end,sim)&&
        weaponhand::client.existing_action_lifetime_on_gt(grasp_lifetime);
}
bool blocks_fire(hand::ObjectIdentity const& weapon) noexcept {
    return lancersaw_original::blocks_fire(weapon)||(granted_frame&&hand::same(identity.weapon,weapon)&&
        interaction.phase()!=Phase::rail_off);
}
void observe_pull_input(hand::Frame const& f) noexcept {
    input_continuity_known=false;input_continuity_sequence=0;
    if((!pulling&&!lancersaw_original::active())||!same_hold(f))return;
    gears_hand_continuity::TrackedInput tracked{};gears_hand_continuity::Publisher publisher{};
    if(!nativeequipment::read_input_continuity(f,tracked,publisher)||
       !nativeequipment::current(nullptr,f))return;
    gears_weapon::LancerSawInputContinuity w{};
    w.world=f.world.address;w.pawn=f.pawn.address;w.weapon=f.weapon.address;
    w.session=f.stereo.session;w.space_epoch=f.stereo.space_epoch;w.input_epoch=f.controllers.input_epoch;
    w.world_generation=f.key.world_generation;w.producer_stream=tracked.stream_epoch;
    w.tracking_epoch[0]=tracked.tracking_epoch[0];w.tracking_epoch[1]=tracked.tracking_epoch[1];
    w.publisher_pid=publisher.pid;w.publisher_start=publisher.start;w.profile=f.controllers.profile_generation;
    w.sequence=f.controllers.sequence;w.created_ms=std::uint64_t(f.controllers.sync_ns/1000000);
    const auto observed_ns=weaponhand::clock_ns(nullptr);
    w.observed_ms=std::uint64_t(observed_ns/1000000);
    w.presses=f.controllers.hands[0].pressed[hand::left_control(f)];
    w.releases=f.controllers.hands[0].released[hand::left_control(f)];
    w.focused=f.controllers.focused==1;w.left_tracked=f.left_tracked==1;
    w.right_tracked=gears_vr::controller_pose_usable(f.controllers.hands[1].grip,
        f.controllers.hands[1].grip_flags,f.controllers.hands[1].grip_active);
    w.left_grip_down=f.left_grip_known&&f.left_grip_down;
    input_continuity_known=true;input_continuity_sequence=w.sequence;
    //13108: real XR kept publishing, but missing game/part callbacks left a
    //333ms gap between these observers. Reuse the SAME existing-action pin
    // which already retains this grasp on a missing Frame. Its producer
    // stream changes on a >100ms XR sync gap, and release/tracking counters
    // cannot be hidden by a slow game callback. Never renew the Frame/pose,
    // grant a new squeeze, or advance physical/mechanical travel here.
    const bool producer_continuity_verified=grasp_pin_known&&
        grasp_hold==vrequip::model.hold_generation()&&
        vrequip::model.held_for(f.world.address,f.pawn.address,f.weapon.address)&&
        weaponhand::client.existing_action_lifetime_on_gt(grasp_lifetime)&&
        grasp_input.refresh(f.stereo,tracked,publisher,observed_ns,
            f.stereo.source==gears_vr::StereoSource::MetaOperatorSimulated);
    const auto bridged_before=interaction.bridged_input_polls();
    const bool interrupted=interaction.observe_input(w,producer_continuity_verified).event==Event::interrupted;
    const bool bridged=!interrupted&&interaction.bridged_input_polls()!=bridged_before;
    if(interrupted||bridged){
        if(log_file&&continuity_rows++<12){fwprintf(log_file,
            L"%ls frame=%llu sequence=%llu age_ms=%llu stream=%llu tracking=%llu,%llu presses=%u releases=%u producer_verified=%u input_polls_bridged=%llu\n",
            interrupted?L"lancer_starter_continuity_interrupted":L"lancer_starter_continuity_bridged",
            f.key.frame,w.sequence,w.observed_ms>=w.created_ms?w.observed_ms-w.created_ms:~0ull,
            w.producer_stream,w.tracking_epoch[0],w.tracking_epoch[1],w.presses,w.releases,
            unsigned(producer_continuity_verified),interaction.bridged_input_polls());fflush(log_file);}
        if(interrupted){clear_grab(5,&f);shown_pull=0;}
    }
}
void reset_after_drain() noexcept {
    if(lancersaw_original::active())return;
    clear_grab();shown_pull=0;locked_rail=cosmetic_nearby=false;released_socket.clear();cosmetic_identity=rail_identity={};rail_hold=0;last_tick_ns=0;events=sample_rows=0;previous_sample_ms=0;lancersaw_visual::restore();lancersaw_original::reset_after_drain();
}
void tick(hand::Frame const& f,hand::Decision const& decision) noexcept {
    const auto now=weaponhand::clock_ns(nullptr);
    // A locked mechanical pose belongs only to this exact weapon hold. Do
    // not carry it to another Lancer, a reused address or a later stow lease.
    if(rail_hold&&(rail_hold!=vrequip::model.hold_generation()||!(rail_identity==inv::identity(f)))){
        locked_rail=false;released_socket.clear();shown_pull=0;rail_identity={};rail_hold=0;clear_grab(8,&f);
    }
    const bool nearby=cosmetic_nearby&&cosmetic_identity==inv::identity(f)&&f.left_tracked&&
        f.controllers.hands[0].squeeze_active==1;
    if(now<=0||!nativeequipment::current(nullptr,f)){
        // Original stop/reflection work can spend the immutable Frame's
        // remaining lifetime. Use the SAME existing input/lifecycle pin used
        // on a missing Frame to retain only its prior C grasp. No pose,
        // travel, new Grip edge or motor request can advance on this path.
        const bool keep=now>0&&same_hold(f)&&pulling&&!pull_complete&&
            (lancersaw_original::retain_hold_during_pose_gap(f.controller.address)||
             retain_grasp_during_pose_gap(f.controller.address));
        if(!keep)clear_grab(1,&f);
        lancersaw_visual::hide();return;
    }
    if(lancersaw_original::active()){
        // Keep the acquired part and fitting through the ready hold. The
        // The grasp and motor have separate lifetimes. A real release returns
        // the hand/handle now; the authenticated existing motor gets its one
        // three-second tail. Tracking/focus/weapon loss still stops at once.
        if(!f.left_tracked||!f.left_grip_known){
            locked_rail=false;shown_pull=0;
            lancersaw_original::stop_pending=true;lancersaw_original::service(f.controller.address);clear_grab(4,&f);
        }else if(!f.left_grip_down||lancersaw_original::hold_input.release_tail_started()){
            lancersaw_original::service(f.controller.address);
            if(granted_frame&&!f.left_grip_down){locked_rail=true;released_socket.bind(f,rail_hold);}
            if(granted_frame)clear_grab(4,&f);
            last_tick_ns=now;lancersaw_visual::tick(f,shown_pull,nearby,locked_rail);return;
        }else{
            observe_pull_input(f);
            gears_weapon::LancerSawSample sample{};gears_lancer_saw_input::Observation stock{};
            if(same_hold(f)&&observed(f,sample,true)&&
               lancersaw_original::context(lancersaw_original::button.owner(),stock)){
                sample.original_hold=gears_lancer_saw_input::ready_motion_context(stock);
                sample.original_button_held=stock.button_held;sample.original_execution=stock.special_move==34;
                const auto result=interaction.observe(sample);
                shown_pull=std::clamp(interaction.pull_metres()/sample.starter_travel_m,0.,1.)*gears_lancer_original_handle::maximum_pull_raw;
                if(result.release_original_melee){
                    lancersaw_original::stop_pending=true;lancersaw_original::service(f.controller.address);
                }
            }
        }
        last_tick_ns=now;lancersaw_visual::tick(f,shown_pull,f.left_tracked&&f.left_grip_known&&f.left_grip_down);return;
    }
    if(!idle_reload(f)){
        clear_grab(2,&f);
        // Only a mechanically acquired rail has a locked position. Reloading
        // before the first C grasp must not permanently lock cosmetic deploy.
        if(rail_hold)locked_rail=true;
        last_tick_ns=now;lancersaw_visual::tick(f,shown_pull,false,locked_rail);return;
    }
    // Journal a REAL known Grip release before the arbiter's released lease
    // clears the action. Keep the mechanical position, not an action permit.
    if(granted_frame&&f.left_tracked&&f.left_grip_known&&!f.left_grip_down){
        locked_rail=true;released_socket.bind(f,rail_hold);
    }
    if(granted_frame&&(!same_hold(f)||decision.release_required||!hand::same(held,decision.lease)||
       (decision.status!=hand::DecisionStatus::granted&&decision.status!=hand::DecisionStatus::retained))){clear_grab(3,&f);}
    if(!f.left_tracked||!f.left_grip_known||!f.left_grip_down){
        if(!f.left_tracked||!f.left_grip_known){locked_rail=false;shown_pull=0;}
        clear_grab(4,&f);
    }
    if(!same_hold(f)){
        if(!locked_rail)shown_pull=0;
    }
    // A current input witness can retain the already-owned pull while an
    // actual render/part matrix is temporarily unavailable. It cannot move
    // the handle or complete/start the motor; observed() below still requires
    // a NEW actual <=100ms geometry and the original hand lease.
    observe_pull_input(f);
    last_tick_ns=now;
    // A completed actual pull has a fixed250ms handoff deadline. Authenticate
    // and dispatch it before reflected visual setters consume this frame's
    // unchanged input/history lifetime. No deadline or input edge is renewed.
    if(pull_complete){
        if(completed_ns<=0||now<completed_ns||now-completed_ns>250000000){clear_grab(7,&f);return;}
        if(same_hold(f)&&lancersaw_original::begin(f)){
            pull_complete=false;pulling=true;completed_ns=0;
        }
        lancersaw_visual::tick(f,shown_pull,pulling||pull_complete||lancersaw_original::active());return;
    }
    lancersaw_visual::tick(f,shown_pull,pulling||pull_complete||nearby,locked_rail);
    if(!lancersaw_visual::ready){
        if(pulling&&sample_rows++<32&&log_file){fwprintf(log_file,
            L"lancer_starter_sample visual_ready=0 visual_gate=%u asset_gate=%u frame=%llu sync_ns=%lld previous_sample_ms=%llu\n",
            lancersaw_visual::failure,weapon_part_assets::failure,f.key.frame,f.controllers.sync_ns,previous_sample_ms);fflush(log_file);}
        return;
    }
    lancersaw_original::prepare(f);
    gears_weapon::LancerSawSample sample{};
    if(!observed(f,sample,true)){
        if(pulling&&sample_rows++<32&&log_file){fwprintf(log_file,
            L"lancer_starter_sample visual_ready=1 observed=0 frame=%llu sync_ns=%lld previous_sample_ms=%llu observed_gate=%u interaction_gate=%u input_age_ns=%lld tick_elapsed_ns=%lld continuity_known=%u continuity_sequence=%llu\n",
            f.key.frame,f.controllers.sync_ns,previous_sample_ms,observed_gate,
            observed_gate==4?lancerreload_visual::interaction_failure:0,
            weaponhand::clock_ns(nullptr)-f.controllers.sync_ns,weaponhand::clock_ns(nullptr)-now,
            unsigned(input_continuity_known),input_continuity_sequence);fflush(log_file);}
        return;
    }
    const auto gap=previous_sample_ms&&sample.created_ms>=previous_sample_ms?sample.created_ms-previous_sample_ms:0;
    previous_sample_ms=sample.created_ms;
    if(!sample.left_grip_down){interaction.observe(sample);return;}
    if(!same_hold(f)||pull_complete){
        if(!same_hold(f)&&ungranted_rows++<12&&log_file){
            fwprintf(log_file,L"lancer_starter_ungranted frame=%llu token=%llu proposal_frame=%llu proposal_gate=%u proposal_observed_gate=%u proposal_matrix_gate=%u proposal_conditions=%u route_frame=%llu route_magazine_pending=%u route_selected_socket=%u decision=%u new_grant=%u lease_kind=%u left=%.6g,%.6g,%.6g anchor=%.6g,%.6g,%.6g\n",
                f.key.frame,f.press_token,proposal_frame,proposal_gate,proposal_observed_gate,proposal_matrix_gate,proposal_conditions,route_frame,unsigned(route_magazine_pending),unsigned(route_selected_socket),unsigned(decision.status),decision.new_grant,unsigned(decision.lease.kind),
                sample.left_palm_m.x,sample.left_palm_m.y,sample.left_palm_m.z,sample.starter_anchor_m.x,sample.starter_anchor_m.y,sample.starter_anchor_m.z);fflush(log_file);}
        return;
    }
    const auto decision_model=interaction.observe(sample);
    const auto event=decision_model.event;
    const auto bridges=interaction.bridged_samples();
    if(event==Event::pull_began)pulling=true;
    if(pulling&&!grasp_pin_known)pin_existing_grasp(f);
    if(pulling)shown_pull=std::clamp(interaction.pull_metres()/sample.starter_travel_m,0.,1.)*gears_lancer_original_handle::maximum_pull_raw;
    if(event==Event::request_original_hold){
        pull_complete=true;pulling=false;completed_ns=std::int64_t(sample.created_ms)*1000000;
    }else if(event==Event::interrupted){clear_grab(6,&f);}
    if(((event!=Event::none&&events++<24)||(pulling&&sample_rows++<32))&&log_file){
        fwprintf(log_file,L"lancer_starter_gesture event=%u frame=%llu sequence=%llu token=%llu pull_raw=%.6g complete=%u original_input_owed=%u gap_ms=%llu left=%.6g,%.6g,%.6g anchor=%.6g,%.6g,%.6g axis=%.6g,%.6g,%.6g input_bridged_samples=%llu\n",
            unsigned(event),f.key.frame,sample.sequence,f.press_token,shown_pull,unsigned(pull_complete),unsigned(lancersaw_original::active()),gap,
            sample.left_palm_m.x,sample.left_palm_m.y,sample.left_palm_m.z,sample.starter_anchor_m.x,sample.starter_anchor_m.y,sample.starter_anchor_m.z,
            sample.pull_axis_m.x,sample.pull_axis_m.y,sample.pull_axis_m.z,bridges);fflush(log_file);
    }
}
} // namespace lancersaw_starter
