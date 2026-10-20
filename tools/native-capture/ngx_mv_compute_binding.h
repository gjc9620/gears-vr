#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace gears_ngx_mv_compute {
// Pure CPU observation. The caller serializes events and supplies a fresh,
// independently attested current generation, not a copy of the observed one.
// Canonical identities are scalar keys, never reconstructed COM interfaces.
struct Recording {uintptr_t canonical{};uint64_t generation{},attested_current_generation{};};
constexpr bool reclaimable(bool revoked,bool busy,bool history_pinned,bool empty,
                           uint64_t held_generation,uint64_t current_generation){
    return revoked&&!busy&&!history_pinned&&empty&&held_generation==current_generation;
}
enum class Loss {none,invalid_recording,capacity,unreadable};
struct Candidate {
    uintptr_t command{},shader_visible_heap{},compute_root_signature{};
    uint64_t generation{},root3_gpu{};bool binding_observed{};
    static constexpr unsigned root_parameter=3;
    // A root-table base address supplies no table length/range semantics. The
    // caller must separately look up a current descriptor at Dispatch and must
    // not infer that another address belongs to this table by subtraction.
    static constexpr bool table_extent_membership_verified=false;
    static constexpr bool descriptor_current_verified=false;
    static constexpr bool gpu_execution_verified=false;
    static constexpr bool producer_authorized=false;
};
class Bindings {
public:
    static constexpr size_t capacity=16;
private:
    struct State {uintptr_t command{};uint64_t generation{};uintptr_t heap{},root{};uint64_t table{};};
    std::array<State,capacity> lists_{};size_t count_{};Loss loss_{};
    bool lose(Loss loss){if(loss_==Loss::none)loss_=loss;return false;}
    State* state(Recording ref){
        if(failed())return nullptr;
        if(!ref.canonical||!ref.generation||!ref.attested_current_generation||ref.generation!=ref.attested_current_generation){
            lose(Loss::invalid_recording);return nullptr;}
        for(size_t i=0;i<count_;++i)if(lists_[i].command==ref.canonical){auto& s=lists_[i];
            // A fresh attestation of another recording never carries prior
            // heap/root/table observations into the new generation.
            if(s.generation!=ref.generation)s={ref.canonical,ref.generation};return &s;}
        if(count_==capacity){lose(Loss::capacity);return nullptr;}
        auto& s=lists_[count_++];s={ref.canonical,ref.generation};return &s;
    }
    bool invalidate(Recording ref){auto* s=state(ref);if(!s)return false;s->heap=0;s->root=0;s->table=0;return true;}
public:
    bool failed()const{return loss_!=Loss::none;}Loss loss()const{return loss_;}size_t lists()const{return count_;}
    void uncertain(){lose(Loss::unreadable);}
    // Revocation needs only the retained identity, not proof that the old
    // recording is still current. Never allocate or recover a failed model.
    void forget_binding(uintptr_t canonical){
        for(size_t i=0;i<count_;++i)if(lists_[i].command==canonical){
            lists_[i].heap=0;lists_[i].root=0;lists_[i].table=0;return;
        }
    }
    bool binding_empty(uintptr_t canonical)const{
        if(failed())return false;
        for(size_t i=0;i<count_;++i)if(lists_[i].command==canonical)
            return !lists_[i].heap&&!lists_[i].root&&!lists_[i].table;
        return false;
    }
    bool erase_empty(uintptr_t canonical){
        if(!binding_empty(canonical))return false;
        for(size_t i=0;i<count_;++i)if(lists_[i].command==canonical){
            for(size_t j=i+1;j<count_;++j)lists_[j-1]=lists_[j];
            lists_[--count_]={};return true;
        }
        return false;
    }
    bool reset(Recording ref){return invalidate(ref);}
    bool clear_state(Recording ref){return invalidate(ref);}
    // Bundle state effects are deliberately not reconstructed by this model.
    bool execute_bundle(Recording ref){return invalidate(ref);}
    // Caller supplies the current shader-visible CBV/SRV/UAV heap identity;
    // sampler heaps are outside this one-table model. Zero means unbound.
    bool shader_heap(Recording ref,uintptr_t identity){auto* s=state(ref);if(!s)return false;
        if(s->heap!=identity){s->heap=identity;s->table=0;}return true;}
    bool compute_root_signature(Recording ref,uintptr_t identity){auto* s=state(ref);if(!s)return false;
        // Redundant binding of the same root signature retains root arguments.
        if(s->root!=identity){s->root=identity;s->table=0;}return true;}
    bool compute_root_table(Recording ref,unsigned root_parameter,uint64_t gpu){auto* s=state(ref);if(!s)return false;
        if(root_parameter==Candidate::root_parameter)s->table=s->heap&&s->root?gpu:0;return true;}
    Candidate dispatch(Recording ref){auto* s=state(ref);if(!s||!s->heap||!s->root||!s->table)return {};
        return {s->command,s->heap,s->root,s->generation,s->table,true};}
};
}
