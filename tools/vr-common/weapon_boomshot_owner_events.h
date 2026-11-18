#pragma once
#include "weapon_boomshot_types.h"

namespace gears_weapon {
// One GT owner's event order. Controller time stays the ORIGINAL sync time;
// an engine observation has its own timestamp and cannot become input.
class BoomshotOwnerEvents {
public:
    struct Input {
        std::uint64_t sequence{},input_epoch{},motion_epoch{},profile_generation{};
        std::int64_t sync_ns{},game_tick{};
    };
    enum class Result {accepted,unbound,invalid,stale,replayed,awaiting_new_input,exhausted};
    class InputEvent {
        friend class BoomshotOwnerEvents;
        BoomshotStamp stamp_{};std::uint64_t raw_sequence_{},input_epoch_{};
    public:
        BoomshotStamp const& stamp()const noexcept{return stamp_;}
        std::uint64_t raw_sequence()const noexcept{return raw_sequence_;}
        std::uint64_t input_epoch()const noexcept{return input_epoch_;}
    };
    class OriginalEvent {
        friend class BoomshotOwnerEvents;
        BoomshotStamp stamp_{};std::uint64_t ordinal_{};
    public:
        BoomshotStamp const& stamp()const noexcept{return stamp_;}
        std::uint64_t ordinal()const noexcept{return ordinal_;}
    };
private:
    BoomshotIdentity identity_{};std::uint64_t binding_{},event_{},original_{},raw_sequence_{},raw_epoch_{},raw_profile_{},native_motion_{},motion_epoch_{};
    std::int64_t ns_{},raw_ns_{},tick_{};
    BoomshotOwnerEvents(BoomshotOwnerEvents const&)=delete;
    BoomshotOwnerEvents& operator=(BoomshotOwnerEvents const&)=delete;
public:
    BoomshotOwnerEvents()=default;
    bool bind(BoomshotIdentity const& identity,std::uint64_t binding) noexcept {
        if(!identity.valid()||!binding||binding<=binding_)return false;
        // Rebinding is NOT authority to clear a debt. The mechanical owner
        // must separately prove closed/ready before rebinding its core.
        if(!(identity_==identity))tick_=0;
        identity_=identity;binding_=binding;return true;
    }
    Result input(Input const& source,std::int64_t now,InputEvent& out) noexcept {
        out={};if(!binding_)return Result::unbound;
        if(!source.sequence||!source.input_epoch||!source.motion_epoch||!source.profile_generation||source.sync_ns<=0||source.game_tick<tick_||
           now<source.sync_ns||source.sync_ns<raw_ns_)return Result::invalid;
        if(now-source.sync_ns>100000000)return Result::stale;
        if(raw_epoch_==source.input_epoch&&raw_profile_==source.profile_generation&&
           (source.sequence<=raw_sequence_||source.sync_ns<=raw_ns_))return Result::replayed;
        // A late original receipt may be observed after this physical sample
        // was acquired. Wait for genuinely newer input, never rewrite its time.
        if(source.sync_ns<=ns_)return Result::awaiting_new_input;
        const bool changed=raw_epoch_!=source.input_epoch||raw_profile_!=source.profile_generation||native_motion_!=source.motion_epoch;
        if(event_==(std::numeric_limits<std::uint64_t>::max)()||
           (changed&&motion_epoch_==(std::numeric_limits<std::uint64_t>::max)()))return Result::exhausted;
        // Private core continuity marker only; never a Native tracking ticket.
        if(changed)++motion_epoch_;
        const auto event=++event_;raw_sequence_=source.sequence;raw_epoch_=source.input_epoch;
        raw_profile_=source.profile_generation;native_motion_=source.motion_epoch;
        raw_ns_=ns_=source.sync_ns;tick_=source.game_tick;
        out.stamp_={identity_,binding_,event,motion_epoch_,tick_,ns_};
        out.raw_sequence_=source.sequence;out.input_epoch_=source.input_epoch;return Result::accepted;
    }
    Result original(std::int64_t observed_ns,std::int64_t game_tick,OriginalEvent& out) noexcept {
        out={};if(!binding_||!motion_epoch_)return Result::unbound;
        if(observed_ns<=0||game_tick<tick_)return Result::invalid;
        if(observed_ns<=ns_)return Result::replayed;
        if(event_==(std::numeric_limits<std::uint64_t>::max)()||original_==(std::numeric_limits<std::uint64_t>::max)())return Result::exhausted;
        ns_=observed_ns;tick_=game_tick;
        out.stamp_={identity_,binding_,++event_,motion_epoch_,tick_,ns_};out.ordinal_=++original_;
        return Result::accepted;
    }
};
}
