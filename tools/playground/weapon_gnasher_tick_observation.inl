// Included inside gnasher_runtime. Existing local branch results only; no
// engine/memory query, input, hook, retained permission or alternate path.
struct TickObservation {
    hand::Frame const& source;gnasher_fire::Entry const* entry{};
    hand::Decision const& grant;unsigned stage{},flags{},event{};
    std::uint64_t sample_sequence{};double angle{};
    ~TickObservation() noexcept {
        if(!log_file||!entry||!entry->mechanical.mechanism_pending()||
           !source.left_grip_known||!source.left_grip_down)return;
        static unsigned rows{},previous_stage{},previous_flags{},previous_event{};
        if(rows>=24||(rows&&previous_stage==stage&&previous_flags==flags&&previous_event==event))return;
        ++rows;previous_stage=stage;previous_flags=flags;previous_event=event;
        fwprintf(log_file,L"gnasher_mechanical_tick stage=%u flags=%u event=%u frame=%llu sequence=%llu sample_sequence=%llu observed_ns=%lld sync_ns=%lld decision=%u lease=%u lease_id=%llu press=%llu mechanical=%u hold_suspended=%u cycle_pending=%u cycle_blocked=%u animation_owner=%u curve=%u down_seen=%u main_required=%u lever_angle=%.9g observation_only=1\n",
            stage,flags,event,source.key.frame,source.controllers.sequence,sample_sequence,
            weaponhand::clock_ns(nullptr),source.controllers.sync_ns,unsigned(grant.status),unsigned(grant.lease.kind),grant.lease.id,source.press_token,
            unsigned(entry->mechanical.stage()),unsigned(entry->mechanical.hold_suspended()),unsigned(entry->cycle.pending()),unsigned(entry->cycle.blocked()),
            unsigned(gnasher_animation::owner.active),unsigned(curve_known),unsigned(down_seen),unsigned(main_required),angle);
        fflush(log_file);
    }
};
