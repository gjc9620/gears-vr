#pragma once
#include "crosshair_assignment.h"
#include <optional>

namespace gears_crosshair_tick {
struct Binding {
    std::uintptr_t controller{},pawn{},weapon{};
    std::uint64_t world{};
    std::uintptr_t root_frame{};
};
struct Sealed {
    Binding binding{};
    std::array<float,3> point{};
    std::uint64_t tick{};
    std::int64_t begin_qpc{},assignment_qpc{},finish_qpc{};
    gears_crosshair_view::Sample view{};
};
struct Counters {
    std::uint64_t begins{},nested{},assignments{},sealed{},consumed{},
        discarded{},invalid_binding{},wrong_token{},wrong_owner{},
        duplicate_assignment{},missing_assignment{},owners_changed{},
        invalid_time{},invalid_point{},wrong_world{},empty_consume{};
};

// One caller-owned thread_local instance. No synchronization or thread-ID
// inference: caller proves thread, root ancestry, binding and receipt authority.
// A consumed result proves only ordered tick-local writer evidence. It does NOT
// certify a camera, scene identity, render ticket, or coordinate equivalence.
class Evidence {
    std::uint64_t sequence_{},token_{};
    unsigned depth_{},writes_{};
    bool valid_{};
    Sealed pending_{};
    std::optional<Sealed> sealed_;
    void discard() noexcept {if(sealed_){++counters.discarded;sealed_.reset();}}
    bool time_valid(std::int64_t value,std::int64_t minimum) noexcept {
        if(value<=0||value<minimum){++counters.invalid_time;valid_=false;return false;}
        return true;
    }
public:
    Counters counters{}; // Same thread only; root may snapshot after quiescence.
    // Revoke evidence without changing the outstanding begin/finish pairing.
    void invalidate() noexcept {valid_=false;discard();}
    std::uint64_t begin(Binding binding,std::int64_t qpc) noexcept {
        ++counters.begins;discard();
        if(depth_){
            ++counters.nested;valid_=false;
            if(depth_<(std::numeric_limits<unsigned>::max)())++depth_;
            return 0; // Finish this nested scope with token 0; never admit it.
        }
        if(sequence_==(std::numeric_limits<std::uint64_t>::max)())return 0;
        token_=++sequence_;depth_=1;writes_=0;valid_=true;
        pending_={binding,{},token_,qpc,0,0};
        if(!binding.controller||!binding.pawn||!binding.weapon||!binding.world||!binding.root_frame){
            ++counters.invalid_binding;valid_=false;
        }
        time_valid(qpc,1);
        return token_; // Even invalid bindings need a matching finish.
    }
    void assignment(std::uint64_t token,gears_crosshair_assignment::CompletedAssignmentReceipt const& receipt,
                    std::int64_t qpc) noexcept {
        ++counters.assignments;
        if(!depth_||depth_!=1||!token||token!=token_){
            ++counters.wrong_token;valid_=false;discard();return;
        }
        if(writes_){++counters.duplicate_assignment;valid_=false;return;}
        ++writes_;
        const auto& binding=pending_.binding;
        if(receipt.controller!=binding.controller||receipt.pawn!=binding.pawn||receipt.weapon!=binding.weapon){
            ++counters.wrong_owner;valid_=false;return;
        }
        if(!time_valid(qpc,pending_.begin_qpc))return;
        for(float value:receipt.point)if(!std::isfinite(value)){
            ++counters.invalid_point;valid_=false;return;
        }
        pending_.point=receipt.point;pending_.assignment_qpc=qpc;pending_.view=receipt.view;
    }
    bool finish(std::uint64_t token,bool owners_valid,std::int64_t qpc) noexcept {
        if(depth_>1&&token==0){--depth_;valid_=false;return false;}
        if(depth_!=1||!token||token!=token_){
            ++counters.wrong_token;valid_=false;discard();return false;
        }
        if(!owners_valid){++counters.owners_changed;valid_=false;}
        if(writes_!=1){++counters.missing_assignment;valid_=false;}
        time_valid(qpc,(std::max)(pending_.begin_qpc,pending_.assignment_qpc));
        depth_=0;token_=0;
        if(!valid_)return false;
        pending_.finish_qpc=qpc;sealed_=pending_;++counters.sealed;return true;
    }
    std::optional<Sealed> consume(std::uint64_t world,std::int64_t qpc) noexcept {
        if(!sealed_){++counters.empty_consume;return std::nullopt;}
        auto result=*sealed_;sealed_.reset(); // Wrong-world/time attempts burn it too.
        if(!world||world!=result.binding.world){++counters.wrong_world;return std::nullopt;}
        if(depth_||qpc<=result.finish_qpc){++counters.invalid_time;return std::nullopt;}
        ++counters.consumed;return result;
    }
};
} // namespace gears_crosshair_tick
