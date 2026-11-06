#pragma once

// Source-side commit order only. Samples/sequence/timestamps are supplied by
// the owning XR loop. A skipped dependency must not expose an incomplete eye
// cohort to receivers. No retries, waits, fallback poses or clock renewal.
namespace gears_controller_cohort {
struct Result {
    bool controllers{},continuity{},eyes_attempted{},eyes{};
};
template<class Controllers,class Continuity,class Eyes>
inline Result publish(bool eyes_allowed,bool hands_required,bool continuity_required,
    Controllers controllers,Continuity continuity,Eyes eyes) {
    Result result{};
    if(hands_required){
        // Observe continuity exactly once even when another mapping is busy;
        // a skipped publication cannot erase source loss/epoch history.
        if(continuity_required)result.continuity=continuity();
        result.controllers=controllers();
    }
    if(eyes_allowed&&(!hands_required||(result.controllers&&
       (!continuity_required||result.continuity)))){
        result.eyes_attempted=true;result.eyes=eyes();
    }
    return result;
}
}
