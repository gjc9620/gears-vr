#pragma once
#include "native_owned_surface.h"

namespace gears_owned_retirement {
enum class Phase:unsigned {live,cpu_drained,gpu_drained,released};
// These assertions come from the stopped session under its producer-order
// mutex, the renderer destructor/return observations, and the Canvas ledger.
// A request or a publication counter alone does not establish any of them.
class Gate {
    friend class Handoff;
    Phase phase_{Phase::live};gears_native_owned::DrainProof proof_{};
    bool reopen(uint64_t old_generation,uint64_t next_generation){
        if(phase_!=Phase::released||proof_.generation!=old_generation||next_generation<=old_generation)return false;
        proof_={};phase_=Phase::live;return true;
    }
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
struct ResourceIdentity {
    uint64_t world{},generation{},ticket_cutoff{};uintptr_t resource{},queue{};
    bool operator==(ResourceIdentity const& b)const{return world==b.world&&generation==b.generation&&
        ticket_cutoff==b.ticket_cutoff&&resource==b.resource&&queue==b.queue;}
};
// A reusable handoff is separate from terminal shutdown. A receipt is issued
// only after the real Owner and its exact tracker binding have both retired.
// Its issuer/serial are checked again before arming one later RHI recreation.
class Handoff {
public:
    class Receipt {
        friend class Handoff;
        Handoff const* issuer_{};uint64_t serial_{};ResourceIdentity identity_{};
    public:
        ResourceIdentity identity()const{return identity_;}
        uint64_t serial()const{return serial_;}
    };
private:
    ResourceIdentity identity_{};uint64_t serial_{},next_world_{};
    bool requested_{},armed_{},bootstrap_claimed_{};
public:
    bool begin(Gate& gate,ResourceIdentity identity,gears_native_owned::DrainProof proof){
        if(requested_)return identity==identity_&&proof.generation==identity_.generation;
        if(armed_||gate.phase()!=Phase::live||serial_==UINT64_MAX||!identity.world||!identity.generation||!identity.resource||!identity.queue||
           identity.ticket_cutoff<identity.generation||proof.generation!=identity.generation||
           (serial_&&(identity.world<=identity_.world||identity.generation<=identity_.generation||
                     identity.ticket_cutoff<=identity_.ticket_cutoff))||!gate.request(proof))return false;
        identity_=identity;++serial_;requested_=true;bootstrap_claimed_=false;return true;
    }
    bool receipt(Gate const& gate,bool owner_retained,bool tracker_retired,Receipt& out)const{
        if(!requested_||gate.phase()!=Phase::released||gate.generation()!=identity_.generation||owner_retained||!tracker_retired)return false;
        Receipt result;result.issuer_=this;result.serial_=serial_;result.identity_=identity_;out=result;return true;
    }
    bool current(Gate const& gate,Receipt const& receipt)const{
        return requested_&&gate.phase()==Phase::released&&receipt.issuer_==this&&receipt.serial_==serial_&&receipt.identity_==identity_;
    }
    bool arm(Gate const& gate,Receipt const& receipt,uint64_t next_world,bool stopped){
        if(stopped||!current(gate,receipt)||identity_.world==UINT64_MAX||next_world!=identity_.world+1||
           identity_.ticket_cutoff==UINT64_MAX)return false;
        if(armed_)return next_world==next_world_;
        next_world_=next_world;armed_=true;return true;
    }
    bool recreated(Gate& gate,uint64_t world,uint64_t generation){
        if(!can_recreate(world,generation,false)||
           !gate.reopen(identity_.generation,generation))return false;
        requested_=armed_=bootstrap_claimed_=false;return true;
    }
    bool requested()const{return requested_;}
    bool armed()const{return armed_;}
    // Arm authorizes one ordinary stereo renderer to reach the RHI creation
    // callback. This claim does not discard the old trial or reopen Owner.
    bool claim_bootstrap(Gate const& gate,uint64_t world,bool stopped){
        if(stopped||!requested_||!armed_||bootstrap_claimed_||world!=next_world_||
           gate.phase()!=Phase::released||gate.generation()!=identity_.generation)return false;
        bootstrap_claimed_=true;return true;
    }
    bool bootstrap_claimed()const{return bootstrap_claimed_;}
    bool can_recreate(uint64_t world,uint64_t generation,bool stopped)const{
        return armed_&&bootstrap_claimed_&&!stopped&&world==next_world_&&generation>identity_.ticket_cutoff;
    }
    uint64_t next_world()const{return next_world_;}
    uint64_t minimum_generation()const{return identity_.ticket_cutoff==UINT64_MAX?0:identity_.ticket_cutoff+1;}
    ResourceIdentity identity()const{return identity_;}
};
}
