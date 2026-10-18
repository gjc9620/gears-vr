#pragma once
#include "native_owned_surface.h"

namespace gears_owned_retirement {
enum class Phase:unsigned {live,cpu_drained,gpu_drained,released};
// These assertions come from the stopped session under its producer-order
// mutex, the renderer destructor/return observations, and the Canvas ledger.
// A request or a publication counter alone does not establish any of them.
class Gate {
    Phase phase_{Phase::live};gears_native_owned::DrainProof proof_{};
public:
    bool request(gears_native_owned::DrainProof proof){
        if(phase_!=Phase::live)return proof.generation==proof_.generation;
        if(!proof.generation||!proof.new_submissions_disabled||!proof.renderers_retired||
           !proof.canvas_commands_retired||!proof.producer_cpu_access_retired||proof.gpu_fence_completed)return false;
        proof_=proof;phase_=Phase::cpu_drained;return true;
    }
    bool gpu_drained(uint64_t generation,bool queue_completed,bool bridges_closed,bool diagnostics_closed){
        if(phase_!=Phase::cpu_drained||generation!=proof_.generation||!queue_completed||!bridges_closed||!diagnostics_closed)return false;
        proof_.gpu_fence_completed=true;phase_=Phase::gpu_drained;return true;
    }
    template<class Owner> bool retire_on_rhi(Owner& owner){
        if(phase_==Phase::released)return !owner.retained();
        if(phase_!=Phase::gpu_drained||!owner.retire_on_rhi(proof_)||owner.retained())return false;
        phase_=Phase::released;return true;
    }
    Phase phase()const{return phase_;}
    uint64_t generation()const{return proof_.generation;}
    bool stopping()const{return phase_!=Phase::live;}
};
}
