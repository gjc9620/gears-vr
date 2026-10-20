#pragma once
#include <cstdint>

// Display-only state. A missing/invalid metadata read cannot start or authorize
// this wait. It never changes the receiver's world/ticket admission rules.
class WorldHandoffDisplay {
public:
    struct State {bool valid{};std::uint64_t generation{};bool quiescing{};};
private:
    State latest_{};
    std::uint64_t baseline_{},required_{};
    bool pending_{};
public:
    void observe(State state) noexcept {
        latest_=state;
        if(!state.valid||!state.generation)return;
        if(baseline_&&state.generation<baseline_)return;
        if(state.quiescing){
            // A quiescing old generation is not the completed handoff.
            if(state.generation!=~std::uint64_t(0)){
                const auto next=state.generation+1;
                if(next>required_)required_=next;
                pending_=true;
            }
        }else if(baseline_&&state.generation>baseline_){
            if(state.generation>required_)required_=state.generation;
            pending_=true;
        }
        if(state.generation>baseline_)baseline_=state.generation;
    }
    State state()const noexcept{return latest_;}
    bool eligible()const noexcept {
        return pending_&&latest_.valid&&latest_.generation&&latest_.generation==baseline_;
    }
    bool projection_completed(bool success,std::uint64_t copied_generation) noexcept {
        if(!success||!eligible()||latest_.quiescing||latest_.generation<required_||
           copied_generation!=latest_.generation)return false;
        pending_=false;required_=0;return true;
    }
};
