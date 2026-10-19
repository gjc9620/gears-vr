#ifdef NDEBUG
#undef NDEBUG
#endif
#include "ngx_mv_compute_binding.h"
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
}
int main(){
    redundant_and_changed_bindings();table_preconditions_and_no_extent_inference();boundaries_and_new_generation();
    invalid_generation_and_terminal_loss();list_isolation_and_capacity();
    std::puts("NGX MV compute binding CPU tests passed; scalar candidate only, no table extent/descriptor/GPU/producer proof");
}
