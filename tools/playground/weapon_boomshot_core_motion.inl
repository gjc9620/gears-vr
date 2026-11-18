// The normal-GT physical/core bridge. Raw sequence/time never become core
// event numbers. This is called by the single inspection/reload controller.
namespace boomshot_core_motion {
namespace hand=gears_weapon_hand;namespace core=gears_weapon;
namespace motion=core::boomshot_barrel;
class Bridge {
    core::BoomshotMechanismState expected_{};
    core::BoomshotOwnerEvents::InputEvent began_{};
    std::uint64_t raw_frame_{},press_{},lease_{},seen_frame_{},seen_raw_{},seen_event_{};motion::Direction direction_{};bool active_{};
    static bool same(core::BoomshotMechanismState const& a,core::BoomshotMechanismState const& b) noexcept {
        return a.identity==b.identity&&a.binding==b.binding&&a.operation_serial==b.operation_serial&&
            a.plan_id==b.plan_id&&a.phase_id==b.phase_id&&a.geometry_id==b.geometry_id&&a.step==b.step&&a.return_required==b.return_required;
    }
    static bool same(core::BoomshotStamp const& a,core::BoomshotStamp const& b) noexcept {
        return a.identity==b.identity&&a.binding==b.binding&&a.sequence==b.sequence&&a.motion_epoch==b.motion_epoch&&
            a.game_tick==b.game_tick&&a.monotonic_ns==b.monotonic_ns;
    }
    static bool input(hand::Frame const& f,core::BoomshotOwnerEvents::InputEvent const& event,
        core::BoomshotFrame const& core_frame,core::BoomshotInteraction const& owner) noexcept {
        const auto& stamp=event.stamp();const auto identity=owner.inventory().identity;
        return weaponhand::current_gt(nullptr)&&stamp.valid()&&stamp.identity==identity&&stamp.binding==owner.binding_token()&&
            identity.world==f.world.address&&identity.pawn==f.pawn.address&&identity.weapon==f.weapon.address&&
            event.raw_sequence()==f.controllers.sequence&&event.input_epoch()==f.controllers.input_epoch&&
            stamp.monotonic_ns==f.controllers.sync_ns&&same(stamp,core_frame.stamp)&&
            core_frame.left_tracked&&core_frame.right_tracked&&core_frame.right_hand_equipped&&core_frame.original_interaction_allowed&&
            !core_frame.original_owns_hands&&core_frame.observed.identity==identity&&
            core_frame.observed.revision==owner.inventory().revision&&core_frame.observed.loaded==owner.inventory().loaded&&
            core_frame.observed.reserve==owner.inventory().reserve&&core_frame.observed.infinite_reserve==owner.inventory().infinite_reserve;
    }
    static core::BoomshotContact purpose(motion::Direction direction) noexcept {
        return direction==motion::Direction::opening?core::BoomshotContact::opening:core::BoomshotContact::closing;
    }
public:
    Bridge()=default;Bridge(Bridge const&)=delete;Bridge& operator=(Bridge const&)=delete;
    bool active()const noexcept{return active_;}
    void release(core::BoomshotInteraction& owner) noexcept {
        if(active_){boomshot_barrel::release();owner.tracking_lost();}active_=false;expected_={};began_={};raw_frame_=press_=lease_=seen_frame_=seen_raw_=seen_event_=0;
    }
    bool begin(hand::Frame const& f,hand::Decision const& d,core::BoomshotMechanismGrant& grant,
        core::BoomshotInteraction& owner,core::BoomshotOwnerEvents::InputEvent const& event,
        core::BoomshotFrame const& core_frame,motion::Direction direction) noexcept {
        core::BoomshotMechanismState expected{};
        if(active_||!input(f,event,core_frame,owner)||!owner.current_motion_seed(event.stamp())||!owner.mechanism_state(expected)||
           (direction!=motion::Direction::opening&&direction!=motion::Direction::closing)||
           (expected.return_required!=(direction==motion::Direction::closing))||
           !boomshot_barrel::begin(f,d,expected.operation_serial,direction,grant))return false;
        // begin() authenticates/consumes the actual Native receipt before this
        // bridge records the core event corresponding to that raw first sample.
        if(!grant.retained(f,d,purpose(direction),weaponhand::clock_ns(nullptr))){boomshot_barrel::release();return false;}
        expected_=expected;began_=event;raw_frame_=f.key.frame;press_=f.press_token;lease_=d.lease.id;direction_=direction;seen_frame_=f.key.frame;seen_raw_=f.controllers.sequence;seen_event_=event.stamp().sequence;active_=true;return true;
    }
    struct Result {motion::Result physical{motion::Result::rejected};core::BoomshotEvent event{core::BoomshotEvent::rejected};};
    Result observe(hand::Frame const& f,hand::Decision const& d,core::BoomshotMechanismGrant& grant,
        core::BoomshotInteraction& owner,core::BoomshotOwnerEvents::InputEvent const& event,
        core::BoomshotFrame const& core_frame) noexcept {
        core::BoomshotMechanismState expected{};
        if(!active_||!input(f,event,core_frame,owner)||!owner.mechanism_state(expected)||!same(expected_,expected)||
           !grant.retained(f,d,purpose(direction_),weaponhand::clock_ns(nullptr))){release(owner);return {};}
        // A repeated raw sample is never sent to the core a second time.
        if(event.stamp().sequence<=seen_event_||f.controllers.sequence<=seen_raw_||f.key.frame<=seen_frame_)
            return {motion::Result::waiting,core::BoomshotEvent::none};
        // Reject a changed core continuity epoch BEFORE any original seek.
        if(!owner.next_motion_sample(event.stamp())){release(owner);return {motion::Result::interrupted,core::BoomshotEvent::motion_reset};}
        const auto physical=boomshot_barrel::observe(f,d,expected_.operation_serial);
        if(physical==motion::Result::interrupted||physical==motion::Result::rejected){release(owner);return {physical,core::BoomshotEvent::rejected};}
        if(physical!=motion::Result::complete){
            const auto accepted=owner.observe(core_frame);
            if(accepted!=core::BoomshotEvent::none){release(owner);return {motion::Result::interrupted,accepted};}
            seen_frame_=f.key.frame;seen_raw_=f.controllers.sequence;seen_event_=event.stamp().sequence;
            return {physical,accepted};
        }
        motion::Completion proof{};
        if(!boomshot_barrel::take_completion(f,d,expected_.operation_serial,proof)){release(owner);return {motion::Result::interrupted,core::BoomshotEvent::rejected};}
        const auto& first=proof.began();const auto& last=proof.ended();
        if(!proof.valid()||proof.direction()!=direction_||first.frame!=raw_frame_||first.stamp.sequence!=began_.raw_sequence()||
           first.stamp.monotonic_ns!=began_.stamp().monotonic_ns||first.lease!=lease_||first.press!=press_||
           last.frame!=f.key.frame||last.stamp.sequence!=event.raw_sequence()||last.stamp.monotonic_ns!=event.stamp().monotonic_ns||
           first.operation!=expected_.operation_serial||last.operation!=expected_.operation_serial||
           last.lease!=lease_||last.press!=press_||began_.stamp().motion_epoch!=event.stamp().motion_epoch){
            release(owner);return {motion::Result::interrupted,core::BoomshotEvent::rejected};
        }
        const core::BoomshotMotionReceipt receipt{expected_.binding,expected_.operation_serial,expected_.plan_id,
            expected_.phase_id,expected_.geometry_id,began_.stamp().motion_epoch,began_.stamp().sequence,
            began_.stamp().monotonic_ns,true,true,true,direction_==motion::Direction::closing,true};
        // Complete consumes this event itself. Sending owner.observe first would
        // replay/consume the exact endpoint input and lose the legitimate step.
        const auto accepted=owner.complete_mechanism(core_frame,receipt);
        active_=false;expected_={};began_={};raw_frame_=press_=lease_=seen_frame_=seen_raw_=seen_event_=0;
        if(accepted!=core::BoomshotEvent::step_completed&&accepted!=core::BoomshotEvent::request_completion)owner.tracking_lost();
        return {physical,accepted};
    }
};
}
