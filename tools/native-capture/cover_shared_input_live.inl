// One Scene-GT transaction for cover AND the registered weapon participant.
// Include after cover_input_live, scene_binding and weapon_hand_runtime. Caller
// holds stereo_mutex and the admitted, current Scene Scope/Receipt. The legacy
// cover-only path and this path use the SAME participant/decoder/hand owner.
// The broker/owner-adapter headers must already be included at global scope.

namespace cover_shared_input_live {
namespace input=cover_input_live;
namespace binding=weapon_hand_scene_binding;
namespace wire=gears_weapon_hand;
namespace runtime=weapon_hand_runtime;
namespace camera=cover_camera_live;
namespace neutral=cover_neutral_eye_live;
struct Snapshot {
    binding::Binding actor{};neutral::EyeSample eye{};
    uint64_t session{},space{},reference{},cookie{},revision{},action_generation{};bool cover{},action_blocked{};
} previous;
struct Diagnostics {
    unsigned long long attempts{},bindings{},outside_cover{},rejected{},prepared{},committed{},aborted{},absent{},blocked{},retries{},physical_fallback{};
    unsigned long long action_revoked{},action_blocked{},action_raced{};
    uint64_t lifecycle{1},frame{},cookie{},revision{};unsigned last_prepare{},last_commit{};
    binding::Reason binding_reason=binding::Reason::body_unavailable;
} diagnostics;
bool have_previous{},interrupted=true,cached_owned{},cached_action_fallback{};
gears_vr::StereoResult cached_stereo{};
gears_vr::StereoMatrix cached_view{};

wire::ObjectIdentity object(gears_anchor::Identity const& id){
    if(!id.address)return {};
    return {id.address,id.outer,id.cls,id.index,id.name_index,id.number,1};
}
bool same_global(Snapshot const& a,Snapshot const& b){return binding::same_key(a.actor,b.actor)&&
    a.session==b.session&&a.space==b.space&&a.reference==b.reference&&a.cookie==b.cookie&&a.revision==b.revision;}
bool same_eye(neutral::EyeSample const& a,neutral::EyeSample const& b){return camera::same_key(a,b)&&
    a.native_ct==b.native_ct&&a.native_ca==b.native_ca&&a.special_move==b.special_move&&a.last_cover_action==b.last_cover_action&&
    a.transition_word==b.transition_word&&a.transition_mask==b.transition_mask&&a.transition==b.transition&&
    a.pawn_location.x==b.pawn_location.x&&a.pawn_location.y==b.pawn_location.y&&a.pawn_location.z==b.pawn_location.z&&
    a.neutral_eye_world.x==b.neutral_eye_world.x&&a.neutral_eye_world.y==b.neutral_eye_world.y&&a.neutral_eye_world.z==b.neutral_eye_world.z;}
void invalidate(wire::InvalidateReason reason) noexcept {
    interrupted=true;cached_owned=cached_action_fallback=false;
    try{input::participant.invalidate(input::left_hand_owner);
        if(auto* broker=runtime::current();broker&&!broker->action_barrier())broker->invalidate(reason);
    }catch(...){runtime::request_stop();}
}
bool epochs(Snapshot const& next){
    auto value=input::epochs;const bool first=!have_previous;
    const bool world=first||!gears_anchor::equal_identity(previous.actor.world,next.actor.world)||previous.actor.world_generation!=next.actor.world_generation;
    const bool pawn=first||!gears_anchor::equal_identity(previous.actor.controller,next.actor.controller)||
        !gears_anchor::equal_identity(previous.actor.pawn,next.actor.pawn)||!gears_anchor::equal_identity(previous.actor.mesh,next.actor.mesh);
    const bool cover=first||previous.cover!=next.cover||(next.cover&&!input::native_key(previous.eye,next.eye));
    const bool slot=first||previous.cover!=next.cover||(next.cover&&(!gears_anchor::equal_identity(previous.eye.link,next.eye.link)||previous.eye.slot!=next.eye.slot));
    const bool session=first||previous.session!=next.session;
    const bool space=session||previous.space!=next.space;
    const bool reference=space||previous.reference!=next.reference;
    if((world&&!input::increment(value.world))||(pawn&&!input::increment(value.pawn))||
       (cover&&!input::increment(value.cover))||(slot&&!input::increment(value.slot))||
       (session&&!input::increment(value.session))||(space&&!input::increment(value.space))||
       (reference&&!input::increment(value.reference)))return false;
    const bool barrier=interrupted||(!first&&(!same_global(previous,next)||cover));
    if(barrier){
        input::participant.invalidate(input::left_hand_owner);
        if(!input::increment(diagnostics.lifecycle))return false;
        ++diagnostics.bindings;
    }
    input::epochs=value;previous=next;have_previous=true;interrupted=false;return true;
}
// The callback pin/transaction is an obligation even in the gap between the
// two callbacks. Every early return/exception aborts it on this same owner GT.
struct Pending {
    wire::Broker* broker{};wire::FrameKey key{};uint64_t nonce{};bool armed{};
    ~Pending(){if(armed&&broker&&broker->pending_transactions()){
        try{broker->abort_frame(key,nonce);}catch(...){/* Runtime still reports retained debt. */}
    }}
};
bool prepare(gears_anchor::Receipt const* scene,gears_vr::StereoMatrix& view,gears_vr::StereoResult const& stereo){
    if(!runtime::enabled)return input::enabled?input::prepare(scene,view,stereo):camera::prepare(scene,view);
    ++diagnostics.attempts;if(input::enabled)++input::diagnostics.attempts;bool owned{};
    try{
        neutral::EyeSample eye{};
        owned=camera::prepare_with(scene,view,[&](auto const* receipt,auto& out){
            if(!neutral::sample(receipt,eye))return false;out=eye;return true;
        });
        auto* broker=runtime::current();
        binding::Binding actor{};
        if(!broker||!scene||scene->thread!=GetCurrentThreadId()||!scene->invocation||!stereo||!stereo.reference_generation||
           !binding::sample(scene,actor)){
            diagnostics.binding_reason=binding::last_reason;++diagnostics.rejected;
            invalidate(wire::InvalidateReason::context_lost);return owned;
        }
        diagnostics.binding_reason=binding::Reason::ready;
        // Action notices can revoke between two uses of this same Scene. A
        // cache hit must never resurrect the old hand grant. Only touch the
        // local participant here: an action callback may already own a Scene
        // or stereo lock, so this path cannot call PG to invalidate again.
        const auto action_generation=broker->action_generation();
        const bool action_barrier=broker->action_barrier();
        const bool same_frame=scene->invocation==diagnostics.frame;
        const bool action_changed=have_previous&&previous.action_generation!=action_generation;
        const bool action_refresh=have_previous&&(action_changed||(action_barrier&&!previous.action_blocked));
        const bool action_blocked=action_barrier||(same_frame&&action_changed);
        const bool cover=owned&&input::enabled&&actor.context_valid&&
            gears_anchor::equal_identity(actor.pawn,eye.pawn)&&gears_anchor::equal_identity(actor.mesh,eye.mesh)&&
            actor.world.address==eye.current_world&&actor.world_generation==eye.world_generation;
        if(cover)++input::diagnostics.camera_owned;
        const bool exact_view=same_frame&&have_previous&&
            owned==cached_owned&&cover==previous.cover&&(!owned||same_eye(previous.eye,eye))&&
            binding::same_key(previous.actor,actor)&&previous.actor.special_move==actor.special_move&&
            previous.cookie==broker->cookie()&&
            !memcmp(&cached_stereo,&stereo,sizeof stereo)&&gears_vr::monotonic_ns()>=stereo.sample.received_ns&&
            gears_vr::monotonic_ns()-stereo.sample.received_ns<=100000000;
        const bool exact_cache=exact_view&&previous.revision==broker->lifecycle_revision();
        if(action_refresh){
            auto fallback=input::diagnostics.latest;
            input::participant.invalidate(input::left_hand_owner);
            interrupted=true;++diagnostics.action_revoked;
            if(exact_view&&action_generation==broker->action_generation()){
                // Reuse only this byte-identical, still-fresh certified view.
                // Do not run a second input/cover observation with the same
                // sequence, publish a new pose, or keep its old hand grant.
                fallback.accepted=false;fallback.left_hand={};
                fallback.interaction.request_left_hand=fallback.interaction.owns_left_hand=false;
                fallback.interaction.request_id=0;
                input::diagnostics.latest=fallback;
                previous.action_generation=action_generation;previous.action_blocked=true;
                previous.revision=broker->lifecycle_revision();
                cached_action_fallback=true;++diagnostics.action_blocked;
                if(cached_owned)view=cached_view;return cached_owned;
            }
            cached_owned=cached_action_fallback=false;
        }
        if(scene->invocation<=diagnostics.frame){
            // Byte equality is conservative: padding differences can reject a
            // retry, but cannot hide mutated pose/reference/status fields.
            if(exact_cache&&(!interrupted||cached_action_fallback)&&
               previous.action_generation==action_generation&&(!action_barrier||previous.action_blocked)&&
               action_generation==broker->action_generation()&&(!broker->action_barrier()||previous.action_blocked)){
                ++diagnostics.retries;if(cached_owned)view=cached_view;return cached_owned;
            }
            ++diagnostics.rejected;invalidate(wire::InvalidateReason::context_lost);return owned;
        }
        diagnostics.frame=scene->invocation;
        if(!cover)++diagnostics.outside_cover;
        Snapshot next{};next.actor=actor;next.eye=eye;next.cover=cover;
        next.session=stereo.sample.session;next.space=stereo.sample.space_epoch;next.reference=stereo.reference_generation;
        next.cookie=broker->cookie();next.revision=broker->lifecycle_revision();
        next.action_generation=action_generation;next.action_blocked=action_blocked;
        if(have_previous&&!same_global(previous,next)){
            if(!action_barrier)broker->invalidate(wire::InvalidateReason::context_lost);
            next.revision=broker->lifecycle_revision();
        }
        diagnostics.cookie=next.cookie;diagnostics.revision=next.revision;
        if(!epochs(next)){++diagnostics.rejected;invalidate(wire::InvalidateReason::context_lost);return owned;}
        const auto stamp=gears_vr::monotonic_ns();gears_vr::ControllerInputFrame hands{};
        const bool matched=input::matching_input(stereo,stamp,hands);
        if(matched)++input::diagnostics.matched;else ++input::diagnostics.missing;
        gears_vr::CoverTrackingBasis axes{};
        if(!gears_vr::cover_tracking_basis(stereo.reference,axes)){
            ++diagnostics.rejected;invalidate(wire::InvalidateReason::input_lost);return owned;
        }
        gears_vr::CoverInputOwnerFrame frame{};frame.lifecycle=diagnostics.lifecycle;frame.frame=scene->invocation;
        frame.cover.sequence=frame.frame;frame.cover.observed_ns=stamp;frame.cover.epochs=input::epochs;
        frame.cover.cover_valid=cover;frame.cover.interaction_enabled=cover&&!eye.special_move;
        frame.cover.right_axis_tracking=axes.right;frame.cover.up_axis_tracking=axes.up;
        // Candidate slot planes are NOT reach/collision certificates. Until
        // native surface queries pass, cover must submit no grab request.
        frame.cover.range={0,0,0,0};frame.cover.edge={};
        if(cover){frame.anchor={eye.neutral_eye_world,camera::facing.forward,camera::facing.right};
            frame.height_curve=input::height_curve.data();frame.height_key_count=unsigned(input::height_curve.size());}
        frame.stereo=stereo;frame.controllers=matched?&hands:nullptr;frame.now_ns=stamp;
        frame.units_per_metre=stereo_units_per_metre;frame.allow_sim=simulator_operator;frame.owner_context_valid=actor.context_valid;
        const auto decoded=input::participant.begin(frame,input::left_hand_owner);
        if(!decoded.accepted){++diagnostics.rejected;invalidate(wire::InvalidateReason::input_lost);return owned;}
        wire::Frame callback{};wire::Proposal proposal{};Pending pending{};
        gears_vr::CoverWeaponContribution weapon{};
        const auto action_current=[&](){return action_generation==broker->action_generation()&&!broker->action_barrier();};
        const bool action_allowed=!action_blocked&&action_current();
        bool absent=action_allowed&&broker->availability()==wire::Availability::absent_quiescent;
        if(absent){weapon.state=gears_vr::CoverWeaponState::absent_quiescent;++diagnostics.absent;}
        else if(action_allowed&&next.cookie&&!broker->draining()){
            const auto session=runtime::session();
            callback.key={session.game_start,session.native_started_qpc,next.cookie,frame.lifecycle,frame.frame,actor.world_generation,
                session.game_pid,GetCurrentThreadId()};
            callback.flags=wire::scene_owned|(matched?wire::input_matched:0u)|(actor.context_valid?wire::gameplay_interactable:0u)|(simulator_operator?wire::allow_sim:0u);
            callback.now_ns=stamp;callback.world=object(actor.world);callback.controller=object(actor.controller);
            callback.pawn=object(actor.pawn);callback.mesh=object(actor.mesh);callback.weapon=object(actor.weapon);
            callback.controllers=hands;callback.stereo=stereo.sample;callback.reference=stereo.reference;
            callback.reference_generation=stereo.reference_generation;callback.stereo_status=unsigned(stereo.status);
            callback.recentered=stereo.recentered?1u:0u;callback.heading_deferred=stereo.heading_deferred?1u:0u;
            callback.press_token=decoded.press_token;callback.left_grip_down=decoded.left_grip_down?1u:0u;
            callback.left_grip_known=decoded.left_grip_known?1u:0u;callback.left_tracked=decoded.left_tracked?1u:0u;
            const auto reply=broker->prepare(callback,proposal);diagnostics.last_prepare=unsigned(reply);
            if(reply==wire::Reply::accepted){
                pending.broker=broker;pending.key=callback.key;pending.nonce=proposal.nonce;pending.armed=true;
                ++diagnostics.prepared;weapon.state=gears_vr::CoverWeaponState::ready;
                for(unsigned i=0;i<weapon.incumbents.size();++i)weapon.incumbents[i]=wire::owner_lease(proposal.incumbents[i]);
                weapon.requests.support=proposal.support!=0;weapon.requests.ammo=proposal.ammo!=0;weapon.requests.magazine=proposal.magazine!=0;
            }else ++diagnostics.blocked;
        }else {++diagnostics.blocked;if(!action_allowed)++diagnostics.action_blocked;}
        if(!action_current())weapon={};
        const auto candidate=input::participant.update_with_decoded(decoded.ticket,weapon,input::left_hand_owner);
        bool committed{};
        if(candidate.accepted&&action_current()){
            if(pending.armed){
                const auto decision=wire::owner_decision(callback,proposal,candidate.left_hand,gears_vr::monotonic_ns());
                const auto reply=broker->commit(callback,proposal,decision);diagnostics.last_commit=unsigned(reply);
                committed=reply==wire::Reply::accepted;
            }else committed=absent&&broker->availability()==wire::Availability::absent_quiescent&&
                next.cookie==broker->cookie()&&next.revision==broker->lifecycle_revision();
        }
        if(committed&&!action_current()){committed=false;++diagnostics.action_raced;}
        const auto result=input::participant.complete(decoded.ticket,committed,input::left_hand_owner);
        // A failed weapon callback aborts this transaction and requires a real
        // release. It must not create a fresh global barrier on EVERY retry:
        // that would keep logical input unknown and prevent recovery forever.
        if(committed)++diagnostics.committed;else ++diagnostics.aborted;
        input::diagnostics.latest=result;input::diagnostics.sequence=stereo.sample.sequence;
        input::diagnostics.reference=stereo.reference_generation;input::diagnostics.frame=scene->invocation;
        const auto status=unsigned(result.controller_status);
        if(status<input::diagnostics.controller_status.size())++input::diagnostics.controller_status[status];
        input::diagnostics.hand_world_valid=cover&&matched&&result.view_valid&&
            gears_vr::controller_pose_usable(hands.hands[0].grip,hands.hands[0].grip_flags,hands.hands[0].grip_active)&&
            gears_vr::cover_tracking_point(frame.anchor,result.view,stereo.reference,hands.hands[0].grip.position,stereo_units_per_metre,input::diagnostics.hand_world);
        if(cover&&result.view_valid){
            // complete(false) can certify the OLD committed offset against
            // this fresh Scene. Physical HMD motion remains independent of PG
            // availability; tentative grip motion and its lease never escape.
            if(!result.accepted)++diagnostics.physical_fallback;
            view=result.view.base_view;
            if(result.body_weight_valid&&!eye.special_move&&input::publish(scene,eye,float(result.requested_up_weight)))++input::diagnostics.targets;
            else ++input::diagnostics.target_rejected;
        }
        cached_owned=owned;cached_action_fallback=false;cached_view=view;memcpy(&cached_stereo,&stereo,sizeof stereo);
        if(!action_current()&&!action_blocked){interrupted=true;cached_owned=false;}
        return owned;
    }catch(...){++diagnostics.rejected;invalidate(wire::InvalidateReason::context_lost);return owned;}
}
void summary(FILE* file){
    fprintf(file,"shared_hand_attempts=%llu shared_hand_bindings=%llu shared_hand_outside_cover=%llu shared_hand_rejected=%llu shared_hand_prepared=%llu shared_hand_committed=%llu shared_hand_aborted=%llu shared_hand_absent=%llu shared_hand_blocked=%llu shared_hand_retries=%llu shared_hand_lifecycle=%llu shared_hand_frame=%llu shared_hand_cookie=%llu shared_hand_revision=%llu shared_hand_prepare_reply=%u shared_hand_commit_reply=%u shared_hand_binding_reason=%s shared_hand_grip_geometry_ready=0 ",
        diagnostics.attempts,diagnostics.bindings,diagnostics.outside_cover,diagnostics.rejected,diagnostics.prepared,diagnostics.committed,diagnostics.aborted,diagnostics.absent,diagnostics.blocked,diagnostics.retries,
        static_cast<unsigned long long>(diagnostics.lifecycle),static_cast<unsigned long long>(diagnostics.frame),static_cast<unsigned long long>(diagnostics.cookie),static_cast<unsigned long long>(diagnostics.revision),
        diagnostics.last_prepare,diagnostics.last_commit,binding::reason_name(diagnostics.binding_reason));
    fprintf(file,"shared_hand_physical_fallback=%llu shared_hand_action_revoked=%llu shared_hand_action_blocked=%llu shared_hand_action_raced=%llu ",
        diagnostics.physical_fallback,diagnostics.action_revoked,diagnostics.action_blocked,diagnostics.action_raced);
}
void close(){std::lock_guard<decltype(stereo_mutex)> lock(stereo_mutex);invalidate(wire::InvalidateReason::native_stopping);}
} // namespace cover_shared_input_live
