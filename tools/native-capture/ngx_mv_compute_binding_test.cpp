#ifdef NDEBUG
#undef NDEBUG
#endif
#include "ngx_mv_compute_binding.h"
#include "ngx_mv_constant_snapshot.h"
#include "ngx_mv_compute_snapshot.h"
#include <cassert>
#include <cstdio>

namespace {
using namespace gears_ngx_mv_compute;
constexpr Recording first{100,1,1};
void complete(Bindings& bindings,Recording ref=first){
    assert(bindings.shader_heap(ref,200)&&bindings.compute_root_signature(ref,300)&&bindings.compute_root_table(ref,3,400));
}
void assert_complete(Candidate const& value,Recording ref=first){
    assert(value.binding_observed&&value.command==ref.canonical&&value.generation==ref.generation&&
        value.shader_visible_heap==200&&value.compute_root_signature==300&&value.root3_gpu==400);
}
void redundant_and_changed_bindings(){
    Bindings bindings;complete(bindings);assert_complete(bindings.dispatch(first));
    assert(bindings.shader_heap(first,200)&&bindings.compute_root_signature(first,300));
    assert_complete(bindings.dispatch(first));
    assert(bindings.compute_root_table(first,2,999));assert_complete(bindings.dispatch(first));
    assert(bindings.shader_heap(first,201)&&!bindings.dispatch(first).binding_observed);
    assert(bindings.compute_root_table(first,3,401));auto changed_heap=bindings.dispatch(first);
    assert(changed_heap.binding_observed&&changed_heap.shader_visible_heap==201&&changed_heap.compute_root_signature==300&&changed_heap.root3_gpu==401);
    assert(bindings.compute_root_signature(first,301)&&!bindings.dispatch(first).binding_observed);
    assert(bindings.compute_root_table(first,3,402)&&bindings.dispatch(first).binding_observed);
    assert(bindings.compute_root_signature(first,300)&&!bindings.dispatch(first).binding_observed); // Changing back cannot restore old args.
    assert(bindings.compute_root_table(first,3,403)&&bindings.dispatch(first).binding_observed);
    assert(bindings.shader_heap(first,0)&&!bindings.dispatch(first).binding_observed);
    assert(bindings.shader_heap(first,201)&&!bindings.dispatch(first).binding_observed);
    assert(bindings.compute_root_table(first,3,404)&&bindings.dispatch(first).binding_observed);
    assert(bindings.compute_root_signature(first,0)&&!bindings.dispatch(first).binding_observed);
}
void table_preconditions_and_no_extent_inference(){
    Bindings bindings;
    assert(bindings.compute_root_table(first,3,400)&&!bindings.dispatch(first).binding_observed);
    assert(bindings.shader_heap(first,200)&&!bindings.dispatch(first).binding_observed);
    assert(bindings.compute_root_table(first,3,400)&&!bindings.dispatch(first).binding_observed);
    assert(bindings.compute_root_signature(first,300)&&!bindings.dispatch(first).binding_observed);
    assert(bindings.compute_root_table(first,3,400));assert_complete(bindings.dispatch(first));
    assert(bindings.compute_root_table(first,3,0)&&!bindings.dispatch(first).binding_observed);
    // Even an arbitrary nonzero base is only an observed scalar candidate.
    // There is no descriptor extent/stride input or membership operation here.
    assert(bindings.compute_root_table(first,3,UINT64_MAX));auto arbitrary=bindings.dispatch(first);
    assert(arbitrary.binding_observed&&arbitrary.root3_gpu==UINT64_MAX);
    static_assert(!Candidate::table_extent_membership_verified&&!Candidate::descriptor_current_verified&&
        !Candidate::gpu_execution_verified&&!Candidate::producer_authorized,"binding observations cannot authorize descriptor/producer claims");
    Bindings root_first;assert(root_first.compute_root_signature(first,300)&&root_first.compute_root_table(first,3,400));
    assert(!root_first.dispatch(first).binding_observed&&root_first.shader_heap(first,200)&&!root_first.dispatch(first).binding_observed);
    assert(root_first.compute_root_table(first,3,400));assert_complete(root_first.dispatch(first));
}
void boundaries_and_new_generation(){
    Bindings bindings;complete(bindings);
    assert(bindings.reset(first)&&!bindings.dispatch(first).binding_observed);complete(bindings);
    assert(bindings.clear_state(first)&&!bindings.dispatch(first).binding_observed);complete(bindings);
    assert(bindings.execute_bundle(first)&&!bindings.dispatch(first).binding_observed);complete(bindings);
    const Recording next{100,2,2};
    assert(!bindings.dispatch(next).binding_observed&&bindings.lists()==1); // No Reset event needed to reject an old recording.
    assert(bindings.compute_root_table(next,3,400)&&!bindings.dispatch(next).binding_observed);
    complete(bindings,next);assert_complete(bindings.dispatch(next),next);
    assert(bindings.reset({100,3,3})&&bindings.lists()==1&&!bindings.dispatch({100,3,3}).binding_observed);
}
void invalid_generation_and_terminal_loss(){
    const Recording invalid[]{ {0,1,1},{100,0,1},{100,1,0},{100,1,2} };
    for(auto ref:invalid){Bindings bindings;complete(bindings);auto historical=bindings.dispatch(first);
        assert(!bindings.compute_root_table(ref,3,999)&&bindings.failed()&&bindings.loss()==Loss::invalid_recording);
        assert(!bindings.dispatch(first).binding_observed&&!bindings.reset(first)&&!bindings.shader_heap(first,200));
        bindings.uncertain();assert(bindings.loss()==Loss::invalid_recording&&historical.binding_observed);
    }
    Bindings bindings;complete(bindings);complete(bindings,{100,2,2});
    assert(!bindings.dispatch({100,1,2}).binding_observed&&bindings.failed()); // Stale observed generation cannot roll back the live one.
    Bindings unreadable;complete(unreadable);unreadable.uncertain();
    assert(unreadable.loss()==Loss::unreadable&&!unreadable.dispatch(first).binding_observed&&!unreadable.clear_state(first));
}
void list_isolation_and_capacity(){
    Bindings bindings;complete(bindings);const Recording second{101,7,7};
    assert(!bindings.dispatch(second).binding_observed);complete(bindings,second);
    assert(bindings.clear_state(first)&&!bindings.dispatch(first).binding_observed);assert_complete(bindings.dispatch(second),second);
    for(uintptr_t i=2;i<Bindings::capacity;++i)complete(bindings,{100+i,uint64_t(i+1),uint64_t(i+1)});
    assert(bindings.lists()==16);
    assert(bindings.reset({100,99,99})&&bindings.lists()==16);complete(bindings,{100,99,99});
    assert(!bindings.shader_heap({999,1,1},200)&&bindings.loss()==Loss::capacity&&bindings.lists()==16);
    assert(!bindings.dispatch(second).binding_observed&&!bindings.compute_root_signature(second,300)&&!bindings.execute_bundle(second));
}
void destructive_revocation(){
    Bindings bindings;complete(bindings);complete(bindings,{101,2,2});
    bindings.forget_binding(100);assert(!bindings.failed()&&bindings.lists()==2);
    assert(!bindings.dispatch(first).binding_observed);
    assert_complete(bindings.dispatch({101,2,2}),{101,2,2});
    bindings.forget_binding(999);assert(bindings.lists()==2&&!bindings.failed());
    // Erasure cannot certify any new or missing generation.
    assert(!bindings.dispatch({100,1,0}).binding_observed&&bindings.failed());
    bindings.forget_binding(100);assert(bindings.failed());
    assert(!bindings.shader_heap(first,200));
    Bindings changed;complete(changed);changed.forget_binding(100);
    assert(!changed.dispatch({100,2,2}).binding_observed&&!changed.failed());
    assert(!changed.dispatch({100,1,2}).binding_observed&&changed.failed());
}
void empty_retirement(){
    using gears_ngx_mv_compute::reclaimable;
    assert(reclaimable(true,false,false,true,0,0));
    assert(reclaimable(true,false,false,true,4,4));
    assert(!reclaimable(true,false,false,true,0,4));
    assert(!reclaimable(true,false,false,true,4,0));
    assert(!reclaimable(true,false,false,true,4,5));
    assert(!reclaimable(false,false,false,true,4,4));
    assert(!reclaimable(true,true,false,true,4,4));
    assert(!reclaimable(true,false,true,true,4,4));
    assert(!reclaimable(true,false,false,false,4,4));
    Bindings b;
    for(uintptr_t i=0;i<Bindings::capacity;++i)complete(b,{100+i,1,1});
    assert(!b.erase_empty(100)&&!b.erase_empty(999)&&b.lists()==16);
    b.forget_binding(100);assert(b.binding_empty(100)&&b.erase_empty(100)&&b.lists()==15);
    complete(b,{999,2,2});assert(b.lists()==16);assert_complete(b.dispatch({101,1,1}),{101,1,1});
    b.forget_binding(999);assert(b.erase_empty(999));
    assert(b.shader_heap({100,3,3},200));assert(!b.binding_empty(100)&&!b.erase_empty(100));
    b.forget_binding(100);assert(b.compute_root_signature({100,3,3},300));assert(!b.erase_empty(100));
    b.forget_binding(100);assert(b.erase_empty(100));
    assert(!b.dispatch({100,4,4}).binding_observed); // Re-admission cannot inherit bindings.
    b.forget_binding(100);b.uncertain();assert(!b.erase_empty(100)&&b.failed());
}
void pso_witness_identity_and_revocation(){
    assert(!pso_blob_save_size(0)&&pso_blob_save_size(1));
    assert(pso_blob_save_size(pso_blob_save_limit)&&!pso_blob_save_size(pso_blob_save_limit+1));
    PsoWitnessKey key{100,200,300,4,5,6,7};
    // Typed interface raw addresses are deliberately absent: a canonical alias
    // with the same genuine generation/context is the same witness candidate.
    assert(pso_witness_matches(key,true,100,4,5,6,300,7));
    assert(!pso_witness_matches(key,true,101,4,5,6,300,7));
    assert(!pso_witness_matches(key,true,100,8,5,6,300,7));
    assert(!pso_witness_matches(key,true,100,0,5,6,300,7));
    assert(!pso_witness_matches(key,true,100,4,8,6,300,7));
    assert(!pso_witness_matches(key,true,100,4,5,8,300,7));
    assert(!pso_witness_matches(key,true,100,4,5,6,301,7));
    assert(!pso_witness_matches(key,true,100,4,5,6,300,8)); // Any Set/revocation advances serial.
    assert(!pso_witness_matches(key,false,100,4,5,6,300,7));
    PsoWitnessValidity state;state.valid=true;state.attest_stable_dispatch(true);
    state.revoke();assert(!state.valid&&state.historical_stable);
    state.attest_stable_dispatch(false);assert(state.historical_stable);
    PsoWitnessValidity unknown;unknown.attest_stable_dispatch(true);assert(!unknown.historical_stable);
}
}
int main(){
    using namespace gears_ngx_mv_constants;
    assert(bounded_upload(256,160,0x1000,0x2000));
    assert(bounded_upload(ring_bytes-256,256,0x1000,0x2000));
    assert(!bounded_upload(1,160,0x1000,0x2000));
    assert(!bounded_upload(0,159,0x1000,0x2000));
    assert(!bounded_upload(ring_bytes,160,0x1000,0x2000));
    assert(!bounded_upload(256,65537,0x1000,0x2000));
    assert(!bounded_upload(256,160,UINT64_MAX-255,0x2000));
    assert(!bounded_upload(256,160,0x1000,UINT64_MAX-255));
    assert(copied_slot_zero(0x2040,0x2000,0x1000,32,0x1040,8));
    assert(!copied_slot_zero(0x2041,0x2000,0x1000,32,0x1040,8));
    assert(!copied_slot_zero(0x2040,0x2000,0x1000,32,0x1060,8));
    assert(!copied_slot_zero(0x2040,0x2000,0x1000,32,0x1040,9));
    assert(range_contains(0x1040,0x1000,32,3));
    assert(!range_contains(0x1060,0x1000,32,3));
    assert(!range_contains(0x1041,0x1000,32,3));
    assert(!range_contains(0x1000,0x1000,0,3));
    RootWitnessKey root{100,200,400,300,4,5,6,7,0,160};
    assert(root_witness_matches(root,true,100,4,5,6,300,7));
    assert(!root_witness_matches(root,false,100,4,5,6,300,7));
    assert(!root_witness_matches(root,true,101,4,5,6,300,7));
    assert(!root_witness_matches(root,true,100,8,5,6,300,7));
    assert(!root_witness_matches(root,true,100,4,8,6,300,7));
    assert(!root_witness_matches(root,true,100,4,5,8,300,7));
    assert(!root_witness_matches(root,true,100,4,5,6,301,7));
    assert(!root_witness_matches(root,true,100,4,5,6,300,8));
    root.device=0;assert(!root_witness_matches(root,true,100,4,5,6,300,7));
    root.device=400;root.size=0;assert(!root_witness_matches(root,true,100,4,5,6,300,7));
    redundant_and_changed_bindings();table_preconditions_and_no_extent_inference();boundaries_and_new_generation();
    invalid_generation_and_terminal_loss();list_isolation_and_capacity();destructive_revocation();empty_retirement();pso_witness_identity_and_revocation();
    std::puts("NGX MV compute binding CPU tests passed; scalar candidate only, no table extent/descriptor/GPU/producer proof");
}
