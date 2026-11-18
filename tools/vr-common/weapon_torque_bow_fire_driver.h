#pragma once
#include "weapon_torque_bow_original.h"
#include "weapon_torque_bow_controls.h"
#include "weapon_equipment_observation.h"
#include <atomic>
#include <cstring>

// One Bow coordinator on the existing normal GT/R2 dispatch. The backend is
// the existing journal and original-state adapter, never another input owner.
namespace gears_torque_bow_fire_driver {
namespace hand=gears_weapon_hand;namespace w=gears_weapon;namespace controls=gears_torque_bow_controls;
struct Snapshot {
    w::TorqueBowOriginalState original{};
    std::uint32_t loaded{},reserve{};
    std::int64_t game_tick{};
    bool fully_stopped{};
};
enum class Shot {none,waiting,recorded,unavailable,conflict};
enum class Status {idle,unavailable,baseline,observed,owned,cleaning,conflict};
struct ReloadAcceptance {
    Snapshot actual{};
    std::uint64_t original_serial{},arrow_serial{},left_grip_edge{};
    std::int64_t original_start_tick{-1};
    bool automatic_fill_suppressed{},fresh_left_grip{},left_shoulder_contact{};
};
struct ReloadMotion {
    std::uint64_t instance_config_serial{},original_reload_serial{},component{},arrow_serial{},begin_input_sequence{};
    bool geometry_verified{},aligned{},continuous{},complete{},tracked_left_hand{};
};
struct Diagnostic {
    std::uint64_t losses{},sequence{},same_frame_begin_waits{},history_verified_decodes{};unsigned reason{},decoded{};
    std::int64_t decode_now{},sync_ns{},received_ns{};
    unsigned decoded_count{},model_observation{},model_edge{};
    std::int64_t model_edge_ns{};
    bool model_can_fire{},decoder_previous_known{};
    unsigned decoder_rejection{};std::uint64_t decoder_previous_sequence{};std::int64_t decoder_previous_sync_ns{};
};
class Driver {
    Diagnostic diagnostic_{};
    w::TorqueBowInteraction model_;
    controls::Decoder decoder_;
    hand::Frame ancestry_{},decoded_source_{};
    gears_vr::ControllerInputFrame decoded_input_{};gears_vr::StereoSample decoded_stereo_{};
    w::TorqueBowRequest dispatched_{};
    w::TorqueBowCallResult call_result_{w::TorqueBowCallResult::rejected};
    Snapshot latest_{};
    std::uint64_t sequence_{},event_{},edge_{},hold_{};
    std::int64_t monotonic_{},game_tick_{};
    bool bound_{},cleanup_{},conflict_{},decoded_known_{};
    bool reload_begin_pending_{},reload_request_sent_{},reload_acknowledged_{};
    std::uint64_t reload_input_floor_{},reload_model_floor_{};
    w::TorqueBowCancelRoute route_{};
    std::atomic<bool> outstanding_{};

    bool belongs(hand::Frame const& f) const noexcept {
        auto a=ancestry_.key,b=f.key;a.frame=b.frame=0;
        return hand::same(a,b)&&hand::same(ancestry_.world,f.world)&&
            hand::same(ancestry_.controller,f.controller)&&hand::same(ancestry_.pawn,f.pawn)&&
            hand::same(ancestry_.mesh,f.mesh)&&hand::same(ancestry_.weapon,f.weapon)&&
            ancestry_.reference_generation==f.reference_generation&&
            gears_vr::stereo_same_pose(ancestry_.reference,f.reference)&&
            ancestry_.stereo.source==f.stereo.source&&ancestry_.controllers.source==f.controllers.source&&
            ancestry_.controllers.session==f.controllers.session&&ancestry_.controllers.space_epoch==f.controllers.space_epoch&&
            ancestry_.controllers.profile_generation==f.controllers.profile_generation&&
            ancestry_.controllers.input_epoch==f.controllers.input_epoch;
    }
    bool unchanged_released_controls(hand::Frame const& f) const noexcept {
        const auto& a=f.controllers;const auto& b=decoded_input_;
        if(!decoded_known_||f.left_grip_down||a.source!=b.source||a.session!=b.session||
           a.space_epoch!=b.space_epoch||a.profile_generation!=b.profile_generation||a.input_epoch!=b.input_epoch)return false;
        const auto& right=a.hands[1];const auto& before=b.hands[1];
        if(right.trigger_active!=1||before.trigger_active!=1||right.trigger>.35f||before.trigger>.35f)return false;
        for(unsigned i=0;i<2;++i){
            const auto& x=a.hands[i];const auto& y=b.hands[i];
            if(x.buttons!=y.buttons||x.buttons_active!=y.buttons_active||
               std::memcmp(x.pressed,y.pressed,sizeof x.pressed)||std::memcmp(x.released,y.released,sizeof x.released))return false;
        }
        return true;
    }
    bool accepted_begin_wait(Snapshot const& actual) const noexcept {
        const auto inventory=model_.inventory();
        return dispatched_.token&&call_result_==w::TorqueBowCallResult::accepted&&
            dispatched_.action==w::TorqueBowAction::begin_original_charge&&
            !actual.original.ready&&!actual.original.charging&&actual.original.r2_held&&
            !actual.original.pending_end_fire&&!actual.original.pending_fire&&!actual.original.firing&&
            !actual.original.animation_owns_hands&&!actual.fully_stopped&&
            !model_.original_shot_recorded()&&actual.loaded==1&&inventory.loaded==1&&
            actual.reserve==inventory.reserve;
    }
    template<class Backend>static auto read_companion(Backend& backend,hand::Frame const& source,
        controls::Continuity& out,int) noexcept -> decltype(backend.read_continuity(source,out)) {
        return backend.read_continuity(source,out);
    }
    template<class Backend>static bool read_companion(Backend&,hand::Frame const&,controls::Continuity&,long) noexcept {
        return false;
    }
    template<class Backend>bool stamp(Backend& backend,std::int64_t tick,w::TorqueBowStamp& out) noexcept {
        const auto now=backend.now();
        if(!bound_||now<=monotonic_||tick<game_tick_||tick<0||sequence_==UINT64_MAX)return false;
        monotonic_=now;game_tick_=tick;
        const auto& f=ancestry_;
        out={{f.world.address,f.pawn.address,f.controller.address,f.weapon.address,f.key.world_generation,
            f.controllers.session,f.controllers.space_epoch,f.controllers.input_epoch},++sequence_,now,tick};
        return true;
    }
    bool event(std::uint64_t& out) noexcept {
        if(event_==UINT64_MAX)return false;out=++event_;return true;
    }
    template<class Backend>void publish(Backend& backend) noexcept {
        outstanding_.store(backend.owned()||backend.has_spawn()||backend.reload_owned()||reload_begin_pending_||dispatched_.token||
            (bound_&&(model_.has_pending_request()||model_.has_fire_attempt())),std::memory_order_release);
    }
    template<class Backend>void lose(Backend& backend,bool fault=false,unsigned reason=1) noexcept {
        ++diagnostic_.losses;diagnostic_.reason=reason;
        decoder_.lost();decoded_known_=false;backend.fence();
        reload_input_floor_=reload_model_floor_=0;
        if(bound_){if(fault||backend.reload_owned())model_.interrupt();else model_.tracking_lost();}
        cleanup_=true;conflict_=conflict_||fault;
    }
    template<class Backend>bool account(Backend& backend) noexcept {
        if(!backend.has_spawn())return true;
        w::TorqueBowStamp s{};std::uint64_t serial{};w::TorqueBowEvent received{};
        if(!stamp(backend,latest_.game_tick,s)||!event(serial))return false;
        const auto result=backend.account(model_,s,serial,received);
        if(received==w::TorqueBowEvent::safety_violation)conflict_=true;
        if(result==Shot::conflict){conflict_=true;lose(backend,true);return false;}
        return result==Shot::recorded||result==Shot::none;
    }
    template<class Backend>bool acknowledge(Backend& backend,Snapshot const& actual,bool accepted) noexcept {
        if(!dispatched_.token)return true;
        const auto inventory=model_.inventory();
        if(actual.loaded!=inventory.loaded||actual.reserve!=inventory.reserve)return false;
        w::TorqueBowStamp s{};std::uint64_t serial{};
        if(!stamp(backend,actual.game_tick,s)||!event(serial))return false;
        const auto result=model_.acknowledge({s,dispatched_.token,serial,accepted,inventory,actual.original,{}});
        if(result==w::TorqueBowEvent::safety_violation||
           (accepted&&dispatched_.action==w::TorqueBowAction::begin_original_charge&&!actual.original.charging)||
           (accepted&&dispatched_.action==w::TorqueBowAction::end_original_fire&&actual.original.r2_held))conflict_=true;
        if(model_.has_pending_request()&&result==w::TorqueBowEvent::interrupted)return false;
        dispatched_={};latest_=actual;
        return true;
    }
    template<class Backend>void service_reload(Backend& backend,hand::Frame const* source,
        hand::GripGameState const* game) noexcept {
        if(reload_begin_pending_&&!cleanup_&&source&&game){
            ReloadAcceptance accepted{};
            const auto result=backend.reload_accept(*source,*game,accepted);
            if(result==w::TorqueBowCallResult::rejected){lose(backend);}
            else if(result==w::TorqueBowCallResult::accepted){
                const auto inventory=model_.inventory();w::TorqueBowStamp s{};
                const auto& right=source->controllers.hands[1];
                const bool tracked=gears_vr::controller_pose_usable(right.grip,right.grip_flags,right.grip_active);
                if(accepted.actual.loaded!=inventory.loaded||accepted.actual.reserve!=inventory.reserve||
                   !stamp(backend,accepted.actual.game_tick,s)||
                   model_.observe({s,inventory,accepted.actual.original,tracked,source->left_tracked!=0,
                       game->main_hand_held,right.trigger>.35f,source->controllers.sync_ns})!=w::TorqueBowEvent::observed||
                   !stamp(backend,accepted.actual.game_tick,s)||
                   model_.begin_left_shoulder({s,accepted.left_grip_edge,accepted.original_serial,true,
                       accepted.automatic_fill_suppressed,accepted.fresh_left_grip,accepted.left_shoulder_contact,accepted.original_start_tick},
                       {accepted.arrow_serial,w::TorqueBowAmmoSource::left_shoulder,true,true,accepted.left_grip_edge,
                        accepted.fresh_left_grip,accepted.left_shoulder_contact})!=w::TorqueBowEvent::reload_started){lose(backend,true);}
                else{
                    latest_=accepted.actual;reload_begin_pending_=false;
                    reload_input_floor_=source->controllers.sequence;reload_model_floor_=sequence_;
                }
            }
        }
        if(!dispatched_.token&&model_.stage()==w::TorqueBowStage::reload_commit){
            w::TorqueBowRequest request{};
            if(model_.take_request(request,backend.now()))dispatched_=request;
        }
        const bool completion=dispatched_.token&&dispatched_.action==w::TorqueBowAction::complete_original_reload;
        if(completion&&!reload_request_sent_){
            reload_request_sent_=true;
            if(cleanup_||!source||!game||!backend.reload_complete(*source,*game,dispatched_)){
                // Preserve even an undispatched closure token until an actual
                // unchanged-inventory/original-exit receipt rejects it.
                if(!backend.reload_reject(dispatched_)){conflict_=true;return;}
                lose(backend);
            }
        }
        if(cleanup_)backend.reload_stop();else backend.reload_service(source);
        if(completion){
            if(!reload_acknowledged_){
                w::TorqueBowActionReceipt receipt{};Snapshot actual{};
                if(!backend.reload_receipt(source,receipt,actual)||receipt.request_token!=dispatched_.token)return;
                if(!stamp(backend,actual.game_tick,receipt.stamp)||!event(receipt.original_event_serial))return;
                const auto result=model_.acknowledge(receipt);
                if(model_.has_pending_request()||
                   (receipt.accepted&&result!=w::TorqueBowEvent::perfect&&result!=w::TorqueBowEvent::ordinary)){
                    conflict_=true;return;
                }
                latest_=actual;reload_acknowledged_=true;
            }
            if(!backend.reload_retire(dispatched_.token,model_))return;
            dispatched_={};reload_request_sent_=reload_acknowledged_=false;
            reload_input_floor_=reload_model_floor_=0;
        }
        if(!backend.reload_owned()&&!dispatched_.token){
            if(cleanup_||reload_begin_pending_)model_.interrupt();
            reload_begin_pending_=false;cleanup_=false;
        }
    }
    // Never polls in a loop awaiting an original state transition. At most
    // four already queued steps are handled on this normal GT dispatch.
    template<class Backend>void service(Backend& backend,hand::Frame const* source,
        hand::GripGameState const* game) noexcept {
        for(unsigned step=0;step<4;++step){
            if(backend.reload_owned()||reload_begin_pending_||
               model_.stage()==w::TorqueBowStage::reload_commit||
               (dispatched_.token&&dispatched_.action==w::TorqueBowAction::complete_original_reload)){
                service_reload(backend,source,game);return;
            }
            if(backend.owned()){
                Snapshot actual{};
                const bool owned_known=backend.read_owned(actual);
                if(owned_known)latest_=actual;
                if(!account(backend)){
                    // A real spawn may precede its ammo update. Stop further
                    // injection now and retain the original fact and journal.
                    lose(backend);Snapshot ignored{};backend.cancel(route_,ignored);return;
                }
                // Waiting grants no action or successful acknowledgement. The
                // first complete owned snapshot already has two fresh typed
                // reads/fingerprints. Positive acknowledgements below retain
                // their original additional read/account/read checks.
                if(owned_known&&!cleanup_&&dispatched_.token){
                    // Original ButtonPress can return with its R2 bit set before
                    // the normal original tick enters Charge. This is a pending
                    // result, never a second press or proof of a completed charge.
                    // Keep the exact journal and original request deadline. Every
                    // loss/cancel/read failure still takes the non-firing cleanup.
                    if(accepted_begin_wait(actual)){
                        const auto now=backend.now();
                        if(!model_.pose_current(now)||now>dispatched_.valid_until_ns){lose(backend);continue;}
                        return; // No polling loop; only a later normal dispatch observes again.
                    }
                }
            }
            if(cleanup_){
                // A dispatched begin/release must first receive its final
                // reconciliation. No unknown original call is replayed.
                if(!dispatched_.token&&model_.has_pending_request()){
                    w::TorqueBowRequest request{};
                    if(model_.take_request(request,backend.now()))dispatched_=request;
                }
                Snapshot stopped{};
                if(!backend.cancel(route_,stopped))return;
                latest_=stopped;
                if(dispatched_.token&&!acknowledge(backend,stopped,
                    dispatched_.action==w::TorqueBowAction::abort_original_charge))return;
                if(model_.has_pending_request())continue;
                cleanup_=false;return;
            }
            if(dispatched_.token){
                Snapshot actual{};
                if(!backend.read_owned(actual)&&!(source&&game&&backend.observe(*source,*game,actual))){lose(backend);continue;}
                latest_=actual;
                if(!account(backend)){lose(backend);continue;}
                // Accounting can change the model's inventory. Read again
                // instead of feeding its earlier pre-spawn snapshot back.
                if(backend.owned()&&!backend.read_owned(actual)){lose(backend);continue;}
                if(!acknowledge(backend,actual,call_result_==w::TorqueBowCallResult::accepted)){lose(backend,true);continue;}
                continue;
            }
            w::TorqueBowRequest request{};
            if(!model_.take_request(request,backend.now()))return;
            dispatched_=request;
            if(request.action==w::TorqueBowAction::abort_original_charge){cleanup_=true;continue;}
            if(request.action!=w::TorqueBowAction::begin_original_charge&&request.action!=w::TorqueBowAction::end_original_fire){
                lose(backend,true);continue;
            }
            if(!source||!game||!backend.prepare(*source,*game)){
                call_result_=w::TorqueBowCallResult::rejected;
                // A refused release leaves a real original key down. Use the
                // non-firing cleanup owner, never a later ordinary release.
                if(request.action==w::TorqueBowAction::end_original_fire){lose(backend);continue;}
            }else call_result_=request.action==w::TorqueBowAction::begin_original_charge?
                backend.press(*source):backend.release(*source);
            if(call_result_==w::TorqueBowCallResult::unknown){lose(backend);continue;}
        }
    }
public:
    Diagnostic diagnostic() const noexcept {return diagnostic_;}
    bool pending() const noexcept {return outstanding_.load(std::memory_order_acquire);}
    bool bound() const noexcept {return bound_;}
    std::uint64_t hold_generation() const noexcept {return bound_?hold_:0;}
    w::TorqueBowInteraction const& interaction() const noexcept {return model_;}
    w::TorqueBowStage stage() const noexcept {return model_.stage();}
    w::TorqueBowInventory inventory() const noexcept {return model_.inventory();}
    bool matches_source(hand::Frame const& source,std::uint64_t hold) const noexcept {return bound_&&hold==hold_&&belongs(source);}
    std::uint64_t controller() const noexcept {return bound_?ancestry_.controller.address:0;}
    template<class Backend>bool reset_drained(Backend& backend) noexcept {
        if(backend.owned()||backend.has_spawn()||backend.reload_owned()||reload_begin_pending_||dispatched_.token||
           (bound_&&(model_.has_pending_request()||model_.has_fire_attempt())))return false;
        if(bound_&&model_.stage()!=w::TorqueBowStage::ready&&model_.stage()!=w::TorqueBowStage::interrupted)return false;
        model_=w::TorqueBowInteraction{};decoder_.lost();ancestry_={};latest_={};
        reload_begin_pending_=reload_request_sent_=reload_acknowledged_=false;reload_input_floor_=reload_model_floor_=0;
        hold_=0;game_tick_=0;bound_=cleanup_=conflict_=decoded_known_=false;publish(backend);return true;
    }
    template<class Backend>Status begin_reload(Backend& backend,hand::Frame const& source,
        hand::GripGameState const& game,hand::Decision const& decision) noexcept {
        if(!bound_||!belongs(source)||backend.hold(source)!=hold_||cleanup_||conflict_||pending()||
           backend.owned()||backend.has_spawn()||backend.reload_owned()||!model_.can_begin_reload(backend.now()))return Status::unavailable;
        // Publish responsibility BEFORE the original call, just as R2's
        // journal does. No recursive call can see this owner as already free.
        reload_begin_pending_=true;decoder_.lost();decoded_known_=false;
        outstanding_.store(true,std::memory_order_release);
        if(!backend.reload_begin(source,game,decision,model_.inventory())){
            if(backend.reload_owned())lose(backend);else reload_begin_pending_=false;
        }
        service_reload(backend,&source,&game);publish(backend);
        return cleanup_?Status::cleaning:pending()?Status::owned:Status::unavailable;
    }
    template<class Backend>bool release_reload_arrow(Backend& backend,hand::Frame const& source) noexcept {
        if(!bound_||!belongs(source)||backend.hold(source)!=hold_||cleanup_||model_.stage()!=w::TorqueBowStage::reload_steps)return false;
        const auto arrow=model_.held_arrow_serial();Snapshot actual{};w::TorqueBowStamp s{};
        return arrow&&backend.reload_sample(source,actual)&&stamp(backend,actual.game_tick,s)&&
            model_.release_arrow(s,arrow)==w::TorqueBowEvent::arrow_released;
    }
    template<class Backend>bool regrab_reload_arrow(Backend& backend,hand::Frame const& source,
        hand::Decision const& decision) noexcept {
        if(!bound_||!belongs(source)||backend.hold(source)!=hold_||cleanup_||
           model_.stage()!=w::TorqueBowStage::reload_steps||model_.held_arrow_serial())return false;
        w::TorqueBowArrowReceipt arrow{};Snapshot actual{};w::TorqueBowStamp s{};
        const auto inventory=model_.inventory();
        if(!backend.reload_sample(source,actual)||actual.loaded!=inventory.loaded||actual.reserve!=inventory.reserve||
           !backend.reload_regrab(source,decision,arrow)||
           !stamp(backend,actual.game_tick,s)||model_.grab_left_shoulder(s,arrow)!=w::TorqueBowEvent::arrow_grabbed)return false;
        reload_input_floor_=source.controllers.sequence;reload_model_floor_=sequence_;return true;
    }
    template<class Backend>bool complete_reload_motion(Backend& backend,hand::Frame const& source,
        hand::GripGameState const& game,ReloadMotion const& motion) noexcept {
        if(!bound_||!belongs(source)||backend.hold(source)!=hold_||cleanup_||!reload_input_floor_||
           motion.begin_input_sequence<reload_input_floor_||motion.begin_input_sequence>=source.controllers.sequence)return false;
        Snapshot actual{};w::TorqueBowStamp s{};
        if(!backend.reload_sample(source,actual)||!stamp(backend,actual.game_tick,s))return false;
        const auto result=model_.complete_step(s,{motion.instance_config_serial,motion.original_reload_serial,motion.component,
            reload_model_floor_,motion.arrow_serial,motion.geometry_verified,motion.aligned,motion.continuous,motion.complete,motion.tracked_left_hand});
        if(result!=w::TorqueBowEvent::request_ready)return false;
        service(backend,&source,&game);publish(backend);return true;
    }
    template<class Backend>Status stop(Backend& backend,w::TorqueBowCancelRoute route) noexcept {
        route_=route;if(!bound_){backend.fence();publish(backend);return pending()?Status::cleaning:Status::idle;}
        lose(backend);service(backend,nullptr,nullptr);publish(backend);
        return pending()?Status::cleaning:conflict_?Status::conflict:Status::idle;
    }
    template<class Backend>Status tick(Backend& backend,hand::Frame const& source,hand::GripGameState const& game) noexcept {
        // An unrelated PC cannot service, replace or invalidate this owner.
        if(bound_&&source.controller.address!=ancestry_.controller.address)return Status::unavailable;
        if(bound_&&(!belongs(source)||backend.hold(source)!=hold_)){
            const auto stopped=stop(backend,w::TorqueBowCancelRoute::active);
            if(pending()||!reset_drained(backend))return stopped;
        }
        if(bound_&&(reload_begin_pending_||cleanup_||
           (dispatched_.token&&dispatched_.action==w::TorqueBowAction::complete_original_reload))){
            service(backend,&source,&game);publish(backend);
            if(reload_begin_pending_||cleanup_||dispatched_.token)return Status::cleaning;
        }
        const auto previous=latest_;
        Snapshot actual{};
        if(!backend.observe(source,game,actual)){
            if(bound_){lose(backend,false,2);service(backend,nullptr,nullptr);}publish(backend);return Status::unavailable;
        }
        if(!bound_){
            w::TorqueBowConfig config{};
            const auto hold=backend.hold(source);
            if(!hold||!actual.fully_stopped||!backend.configure(source,game,actual,config)||
               !model_.bind(config,{1,actual.loaded,actual.reserve}))return Status::unavailable;
            ancestry_=source;hold_=hold;bound_=true;
        }
        // Consume THIS input packet's original R3 before interpreting its
        // cancel edge. The previous packet's veto may already have expired;
        // cleanup can finish immediately and therefore skip muzzle prepare.
        if(backend.owned()&&!backend.guard(source)){
            lose(backend);service(backend,nullptr,nullptr);publish(backend);return Status::cleaning;
        }
        latest_=actual;
        if(!account(backend)){lose(backend);service(backend,nullptr,nullptr);publish(backend);return Status::cleaning;}
        // This exact Frame already supplied its edge. One new observe still
        // performs the backend's full typed two-read/currentness snapshot, and
        // guard/account above remain fresh. An unchanged accepted begin can
        // only wait here: no stamp, decoder, action, ACK or renewed pose clock.
        // Any Frame/input/state/game-clock progress retains the complete path.
        if(decoded_known_&&hand::same_frame(decoded_source_,source)&&
           game.known&&game.main_hand_held&&!game.busy&&!game.animation_owns_hands&&!source.left_grip_down&&
           bound_&&!cleanup_&&!conflict_&&!reload_begin_pending_&&backend.owned()&&
           !backend.has_spawn()&&!backend.reload_owned()&&
           accepted_begin_wait(previous)&&accepted_begin_wait(actual)&&actual.game_tick==previous.game_tick){
            const auto now=backend.now();
            if(model_.pose_current(now)&&now<=dispatched_.valid_until_ns){
                if(diagnostic_.same_frame_begin_waits<UINT64_MAX)++diagnostic_.same_frame_begin_waits;publish(backend);return Status::owned;
            }
            lose(backend);service(backend,nullptr,nullptr);publish(backend);return Status::cleaning;
        }
        // An unchanged, released, debt-free idle packet cannot submit an R2,
        // R3 or shoulder reload. Its first observe is still a full typed
        // two-read/fingerprint snapshot. Every new control/cohort, left grip,
        // other fire/reload/cleanup obligation keeps the second full observation.
        const bool idle=bound_&&!cleanup_&&!conflict_&&!dispatched_.token&&!reload_begin_pending_&&
            !backend.owned()&&!backend.has_spawn()&&!backend.reload_owned()&&
            !model_.has_pending_request()&&!model_.has_fire_attempt()&&model_.stage()==w::TorqueBowStage::ready&&
            unchanged_released_controls(source);
        if(!idle&&!backend.observe(source,game,actual)){lose(backend,false,3);service(backend,nullptr,nullptr);publish(backend);return Status::unavailable;}
        latest_=actual;
        // The pre-original and post-equipment boundaries may see exactly the
        // same immutable packet. It can service existing work, never another
        // edge or release observation; fresh source/time checks above remain.
        const bool repeat=decoded_known_&&
            !std::memcmp(&decoded_input_,&source.controllers,sizeof decoded_input_)&&
            !std::memcmp(&decoded_stereo_,&source.stereo,sizeof decoded_stereo_);
        controls::Result decoded{};
        if(repeat)decoded.status=controls::Status::observed;
        else {
            controls::Continuity companion{};
            const bool companion_known=read_companion(backend,source,companion,0);
            const auto decode_now=backend.now();
            diagnostic_.sequence=source.controllers.sequence;diagnostic_.decode_now=decode_now;
            diagnostic_.sync_ns=source.controllers.sync_ns;diagnostic_.received_ns=source.controllers.received_ns;
            decoded=decoder_.advance(source.controllers,source.stereo,decode_now,
            backend.owned()&&model_.has_fire_attempt()&&!model_.original_shot_recorded()&&!actual.original.firing,
            (source.flags&hand::allow_sim)!=0,companion_known?&companion:nullptr);
            if(decoded.history_verified&&diagnostic_.history_verified_decodes<UINT64_MAX)
                ++diagnostic_.history_verified_decodes;
            diagnostic_.decoder_rejection=unsigned(decoded.rejection);
            diagnostic_.decoder_previous_known=decoded.previous_known;
            diagnostic_.decoder_previous_sequence=decoded.previous_sequence;
            diagnostic_.decoder_previous_sync_ns=decoded.previous_sync_ns;
        }
        diagnostic_.decoded=unsigned(decoded.status);
        diagnostic_.decoded_count=decoded.count;
        if(decoded.status==controls::Status::invalid||decoded.status==controls::Status::ambiguous){
            lose(backend,false,4);service(backend,nullptr,nullptr);publish(backend);return Status::cleaning;
        }
        decoded_source_=source;decoded_input_=source.controllers;decoded_stereo_=source.stereo;decoded_known_=true;
        const auto expected=model_.inventory();
        if(actual.loaded!=expected.loaded||actual.reserve!=expected.reserve){
            lose(backend,true);service(backend,nullptr,nullptr);publish(backend);return Status::conflict;
        }
        w::TorqueBowStamp s{};
        if(!stamp(backend,actual.game_tick,s)){lose(backend);service(backend,nullptr,nullptr);publish(backend);return Status::unavailable;}
        const auto& right=source.controllers.hands[1];
        const bool right_tracked=gears_vr::controller_pose_usable(right.grip,right.grip_flags,right.grip_active);
        const auto observation=model_.observe({s,expected,actual.original,right_tracked,source.left_tracked!=0,
            game.main_hand_held,right.trigger>.35f,source.controllers.sync_ns,
            decoded.history_verified?decoded.previous_sync_ns:0});
        diagnostic_.model_observation=unsigned(observation);
        if(observation==w::TorqueBowEvent::abort_required||observation==w::TorqueBowEvent::interrupted)cleanup_=true;
        if(backend.reload_owned()&&!source.left_tracked)lose(backend);
        service(backend,&source,&game);
        for(unsigned i=0;i<decoded.count&&!cleanup_;++i){
            if(edge_==UINT64_MAX||!stamp(backend,actual.game_tick,s)){lose(backend,true);break;}
            const auto action=decoded.actions[i];const auto edge=++edge_;
            s.original_control_ns=decoded.original_action_ns[i];
            diagnostic_.model_edge_ns=s.monotonic_ns;
            diagnostic_.model_can_fire=model_.can_request_fire(s.monotonic_ns,model_.inventory());
            if(action==controls::Action::cancel){
                const auto cancelled=model_.cancel_charge(s,{edge,right_tracked,true,true});
                diagnostic_.model_edge=unsigned(cancelled);
                // A fresh R3 while an accepted begin awaits its original tick
                // must drain that begin, rather than keep waiting for Charge.
                if(cancelled==w::TorqueBowEvent::abort_required&&dispatched_.token&&
                   dispatched_.action==w::TorqueBowAction::begin_original_charge)cleanup_=true;
            }else diagnostic_.model_edge=unsigned(model_.trigger(s,{edge,right_tracked,true,action==controls::Action::press}));
            service(backend,&source,&game);
        }
        if(decoded.release_observed&&!cleanup_&&stamp(backend,actual.game_tick,s))model_.observe_trigger_up(s,true);
        // Full original neutrality and drained projectile receipts are required
        // even after normal EndFire; release acceptance alone retires nothing.
        if(!dispatched_.token&&!cleanup_&&backend.owned()){
            Snapshot neutral{};
            if(backend.read_owned(neutral)&&neutral.fully_stopped){
                const auto inventory=model_.inventory();
                if(neutral.loaded==inventory.loaded&&neutral.reserve==inventory.reserve&&backend.settle(neutral)){
                    latest_=neutral;
                    if(stamp(backend,neutral.game_tick,s)){
                        const auto neutral_event=model_.observe({s,inventory,neutral.original,right_tracked,source.left_tracked!=0,
                            game.main_hand_held,right.trigger>.35f,source.controllers.sync_ns});
                        if(neutral_event==w::TorqueBowEvent::abort_required)cleanup_=true;
                    }
                }
            }
        }
        publish(backend);
        return conflict_?Status::conflict:pending()?Status::owned:
            decoded.status==controls::Status::baseline?Status::baseline:Status::observed;
    }
    template<class Backend>Status poll(Backend& backend) noexcept {
        if(!bound_){publish(backend);return pending()?Status::cleaning:Status::idle;}
        const auto now=backend.now();const auto expired=model_.expire_pose(now);
        if(expired==w::TorqueBowEvent::abort_required||(backend.reload_owned()&&!model_.pose_current(now,true))){lose(backend,false,5);}
        service(backend,nullptr,nullptr);publish(backend);
        return conflict_?Status::conflict:pending()?Status::owned:Status::idle;
    }
    template<class Backend>Status before_original_tick(Backend& backend,hand::Frame const& source,
        hand::GripGameState const& game) noexcept {
        const auto result=tick(backend,source,game);
        // Original Charge.Tick/Pawn-deferred release can emit an arrow on a
        // later game tick. Keep its current held source, original sync clock
        // and exact journal; a failure stops injection before the stock tick.
        if(backend.owned()&&!cleanup_&&(!belongs(source)||!backend.prepare(source,game))){
            lose(backend);service(backend,nullptr,nullptr);publish(backend);return Status::cleaning;
        }
        return result;
    }
};
}
