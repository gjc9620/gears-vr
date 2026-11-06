#pragma once
#include "weapon_lancer_insertion.h"

namespace gears_weapon {
// Game-thread history of authenticated, completed bilateral poses. It supplies
// gesture observations only; the caller still needs a fresh current pose and
// the same grant before dispatching any gameplay or inventory action.
class LancerMotionSamples {
    std::array<LancerInsertionSample,32> samples_{};
    unsigned count_{},stage_{};
    std::uint64_t token_{},consumed_{};
    bool failed_{};
public:
    void clear() noexcept {*this={};}
    void begin(unsigned stage,std::uint64_t token,std::uint64_t sequence) noexcept {
        clear();stage_=stage;token_=token;consumed_=sequence;
    }
    bool matches(unsigned stage,std::uint64_t token) const noexcept {
        return token&&token_==token&&stage_==stage;
    }
    bool push(unsigned stage,std::uint64_t token,LancerInsertionSample const& sample) noexcept {
        if(!matches(stage,token)||failed_)return false;
        if(sample.sequence<=consumed_||(count_&&sample.sequence<=samples_[count_-1].sequence))return true;
        if(count_==samples_.size()){failed_=true;return false;}
        samples_[count_++]=sample;return true;
    }
    template<class Observe>
    bool consume(unsigned stage,std::uint64_t token,LancerInsertionSample const& current,Observe observe) noexcept {
        if(!matches(stage,token)||failed_||current.sequence<consumed_)return false;
        unsigned used{};
        while(used<count_&&samples_[used].sequence<=current.sequence){
            const auto& sample=samples_[used++];
            if(sample.sequence>consumed_){observe(sample);consumed_=sample.sequence;}
        }
        for(unsigned i=used;i<count_;++i)samples_[i-used]=samples_[i];
        count_-=used;
        if(current.sequence>consumed_){observe(current);consumed_=current.sequence;}
        return true;
    }
};
}
