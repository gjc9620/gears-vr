// Included inside gnasher_animation. Engine/schema boundaries are implemented
// by the original-animation adapter; this is its single journal, not a writer.
struct Selection {
    anim::Observation before{};anim::Controls controls{};
    hand::ObjectIdentity target{},selector{};Name name{};bool needed{};
};
struct Owner {
    inv::Identity identity{};anim::Observation last{};anim::Controls controls{};
    std::uint64_t serial{},profile{},reference{};float wanted{};
    bool active{},stop_dispatched{},seek_pending{},selection_pending{},call_in_flight{},stopped_observed{};
    hand::ObjectIdentity controller{};
    hand::ObjectIdentity requested_sequence{};Name requested_name{};
};
Owner owner;std::uint64_t last_serial{};
bool support_granted(hand::Frame const&) noexcept;
bool shot_admitted(hand::Frame const&) noexcept;
bool observe(hand::Frame const&,anim::Observation&,bool chamber_only=true) noexcept;
bool current(hand::Frame const&,anim::Observation&) noexcept;
bool prepare_selection(hand::Frame const&,Selection&) noexcept;
bool dispatch_selection(hand::Frame const&,Selection const&,bool& dispatched) noexcept;
bool observe_owned_target(anim::Observation&) noexcept;
bool dispatch_owned_stop(anim::Observation const&,anim::Controls const&,bool& dispatched) noexcept;

// Withdraw only the exact animation control already admitted and journaled.
// This cannot choose a clip, seek, move a hand, ACK a mechanism or refresh input.
bool service_owned() noexcept {
    if(!owner.active)return true;
    if(owner.call_in_flight)return false;
    anim::Observation selected{};
    if(!observe_owned_target(selected))return false;
    if(owner.selection_pending){
        owner.last=selected;owner.selection_pending=false;
    }
    if(!owner.stop_dispatched){
        owner.call_in_flight=true;
        (void)dispatch_owned_stop(selected,owner.controls,owner.stop_dispatched);
        owner.call_in_flight=false;
    }
    anim::Observation stopped{};
    if(!observe_owned_target(stopped)||stopped.playing)return false;
    // Once frozen, cleanup observations must not silently adopt a changed
    // rate/length/loop baseline and thereby grant a later physical seek.
    if(owner.stopped_observed&&(stopped.rate!=owner.last.rate||stopped.length!=owner.last.length||
       stopped.looping!=owner.last.looping))return false;
    owner.last=stopped;owner.stopped_observed=true;return true;
}

bool continue_begin(hand::Frame const& f,std::uint64_t serial) noexcept {
    if(!owner.active||owner.call_in_flight||serial!=owner.serial||!(owner.identity==inv::identity(f))||!hand::same(owner.controller,f.controller)||
       owner.profile!=f.controllers.profile_generation||owner.reference!=f.reference_generation||
       !support_granted(f))return false;
    if(!service_owned())return false;
    anim::Observation stopped{};
    if(!gnasher_animation::current(f,stopped))return false;
    owner.last=stopped;return true;
}

bool begin(hand::Frame const& f,std::uint64_t serial) noexcept {
    if(owner.active)return continue_begin(f,serial);
    if(!serial||serial<=last_serial||!hand::identity_valid(f.controller)||!support_granted(f))return false;
    Selection prepared{};
    if(!prepare_selection(f,prepared)||!support_granted(f))return false;
    Owner next{};next.identity=inv::identity(f);next.last=prepared.before;next.controls=prepared.controls;
    next.serial=serial;next.profile=f.controllers.profile_generation;next.reference=f.reference_generation;
    next.active=next.selection_pending=true;next.requested_sequence=prepared.target;next.requested_name=prepared.name;
    next.controller=f.controller;
    owner=next;last_serial=serial; // consumed before any original selection call
    if(prepared.needed){
        bool dispatched{};owner.call_in_flight=true;
        (void)dispatch_selection(f,prepared,dispatched);owner.call_in_flight=false;
        // Exact preflight rejection is different from an unknown original
        // result: there is no selected-node responsibility to retain. The
        // consumed serial still cannot replay this expired admission.
        if(!dispatched){owner={};return false;}
    }
    // A return code alone is never selection proof. Also reconcile an unknown
    // return immediately when the exact actual target is readable, so a known
    // selected node can be stopped before the next animation update.
    (void)service_owned();
    return continue_begin(f,serial);
}
// Stop only the original SG_Chamber already selected by this actual shot.
// No left grasp is invented, no clip can be selected here, and no seek is
// authorized. This prevents the stock cocking animation from doing the visual
// lever cycle while the player has not yet grasped the fore-end.
bool hold_after_original_shot(hand::Frame const& f,std::uint64_t serial) noexcept {
    if(!shot_admitted(f))return false;
    if(owner.active){
        return !owner.call_in_flight&&serial==owner.serial&&owner.identity==inv::identity(f)&&
            hand::same(owner.controller,f.controller)&&owner.profile==f.controllers.profile_generation&&
            owner.reference==f.reference_generation&&service_owned();
    }
    if(!serial||serial<=last_serial||!hand::identity_valid(f.controller))return false;
    Selection prepared{};
    if(!prepare_selection(f,prepared)||prepared.needed||!shot_admitted(f))return false;
    Owner next{};next.identity=inv::identity(f);next.last=prepared.before;next.controls=prepared.controls;
    next.serial=serial;next.profile=f.controllers.profile_generation;next.reference=f.reference_generation;
    next.active=next.selection_pending=true;next.requested_sequence=prepared.target;next.requested_name=prepared.name;
    next.controller=f.controller;owner=next;last_serial=serial;
    return service_owned();
}
