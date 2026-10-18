// Exercise actual opt-in host/wrapper code with synthetic memory and typed
// boundary injection. No detours, COM, SDK, GPU or game execution.
#define GEARS_NGX_STEREO_HOST_OBSERVER 1
#define GEARS_OWNED_OUTPUT_TRACKING 1
#define GEARS_NGX_STEREO_HOST_TEST 1
#define main route_observer_test_main
#include "ngx_route_observer_test.cpp"
#undef main
#include "gpu_viewport_stats.h"
namespace {
bool color_copy_diagnostics=true;
struct Frame {uint64_t generation=1;struct Ref {uintptr_t identity;uint64_t generation;};Ref current(uintptr_t id){return {id,generation};}} frame_link;
gears_viewport::Tracker frame_target_stats;
uintptr_t canonical_identity(IUnknown* object){return reinterpret_cast<uintptr_t>(object);}
bool install(void*,void*,void**){return true;}
}
#define _ReturnAddress test_return_address
#pragma warning(push)
#pragma warning(disable:4459) // Synthetic fixture globals named handle/parameters.
#include "ngx_stereo_host_live.inl"
#pragma warning(pop)
#undef _ReturnAddress
namespace ngx_stereo_host {
unsigned typed_calls{};
Typed typed(ID3D12Resource* resource){++typed_calls;Typed out;out.raw=out.canonical=reinterpret_cast<uintptr_t>(resource);out.device=0xd00;out.hr=S_OK;
    out.desc.Width=2560;out.desc.Height=1080;out.desc.Format=DXGI_FORMAT_R11G11B10_FLOAT;return out;}
bool vendor_owner(uintptr_t,uintptr_t&){return false;}
UINT descriptor_stride(ID3D12GraphicsCommandList*){return 32;}
}
namespace {
unsigned dispatch_calls{},copy_calls{},execute_calls{},release_calls{};bool throw_dispatch{};uint32_t create_raw=0x12345678;
uint32_t __fastcall forward_eval(uintptr_t command,uintptr_t h,uintptr_t p,uintptr_t callback){unlocked();++dispatch_calls;
    assert(command==0x100&&h==handle&&p==parameters&&!callback);if(throw_dispatch)throw std::runtime_error("dispatcher");return 0xbad01234;}
uint32_t __fastcall forward_create(uintptr_t command,uint32_t feature,uintptr_t p,uintptr_t* output){unlocked();++dispatch_calls;
    assert(command==0x100&&feature==1&&p==parameters&&reinterpret_cast<uintptr_t>(output)==base+handle_cell_rva);handle=0x12345;return create_raw;}
uint32_t __fastcall forward_release(uintptr_t h){unlocked();++release_calls;assert(h==handle);return 0x87654321;}
void STDMETHODCALLTYPE forward_copy(ID3D12GraphicsCommandList* command,D3D12_TEXTURE_COPY_LOCATION const* dst,UINT x,UINT y,UINT z,D3D12_TEXTURE_COPY_LOCATION const* src,D3D12_BOX const* box){
    unlocked();++copy_calls;assert(command==reinterpret_cast<ID3D12GraphicsCommandList*>(0x100)&&!x&&!y&&!z&&!box);assert(src->pResource==reinterpret_cast<ID3D12Resource*>(0x501)&&dst->pResource==reinterpret_cast<ID3D12Resource*>(0x500));}
void STDMETHODCALLTYPE forward_execute(ID3D12CommandQueue* queue,UINT count,ID3D12CommandList* const* commands){unlocked();++execute_calls;assert(queue==reinterpret_cast<ID3D12CommandQueue*>(0x200)&&count==1&&commands[0]==reinterpret_cast<ID3D12CommandList*>(0x100));}
void host_reset(){reset();using namespace ngx_stereo_host;attested=true;printed=false;bank={};rows={};releases={};row_count=epoch_evals=epochs=in_wrappers=release_count=0;
    sampled_tickets=typed_observations=total_sampled_tickets=total_typed_observations=0;
    latest_native_create={};native_create_revision=0;source_create_scope=nullptr;
    sequence=execute_sequence=limited=foreign_dispatches=read_failures=0;current_row=pending_row=-1;typed_calls=0;
    ngx_stereo_host::original_create=forward_create;ngx_stereo_host::original_evaluate=forward_eval;original_release=forward_release;frame_link.generation=1;
    dispatch_calls=copy_calls=execute_calls=release_calls=0;throw_dispatch=false;create_raw=0x12345678;
    for(unsigned i=0;i<4;++i)put(evaluation,resource_offsets[i],uintptr_t(0x500+i));}
void observe_inputs(){auto* command=reinterpret_cast<ID3D12GraphicsCommandList*>(0x100);
    std::array<D3D12_RESOURCE_BARRIER,4> barriers{};for(unsigned i=0;i<4;++i){auto& b=barriers[i];b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition.pResource=reinterpret_cast<ID3D12Resource*>(uintptr_t(0x500+i));b.Transition.StateAfter=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;}
    ngx_stereo_host::barriers(command,4,barriers.data(),base+0xabc);
    frame_target_stats.reset(0x100);frame_target_stats.create(0x1000,{0x500,2560,1080,26,0});
    D3D12_CPU_DESCRIPTOR_HANDLE descriptor{0x1000};ngx_stereo_host::targets(command,1,&descriptor,FALSE,nullptr);
    ngx_stereo_host::draw(command,base+0x123,6,1);
}
void eval(){ngx_stereo_host::WrapperScope scope(true,true,base+0x9a35d3,0x100,evaluation.data());
    const auto result=ngx_stereo_host::hook_evaluate(0x100,handle,parameters,0);assert(result==0xbad01234);scope.normal_return(0xfedcba9876543210);}
uint64_t __fastcall wrapper_forward(uintptr_t command,uintptr_t,uintptr_t,void const* structure){
    assert(structure==evaluation.data());assert(ngx_stereo_host::hook_evaluate(command,handle,parameters,0)==0xbad01234);return 0xfedcba9876543210;}
void copy_execute(){D3D12_TEXTURE_COPY_LOCATION src{},dst{};src.pResource=reinterpret_cast<ID3D12Resource*>(0x501);dst.pResource=reinterpret_cast<ID3D12Resource*>(0x500);
    src.Type=dst.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    ngx_stereo_host::copy_passthrough(reinterpret_cast<ID3D12GraphicsCommandList*>(0x100),&dst,0,0,0,&src,nullptr,base+0x1c6093,forward_copy);
    auto* list=reinterpret_cast<ID3D12CommandList*>(0x100);ngx_stereo_host::execute_passthrough(reinterpret_cast<ID3D12CommandQueue*>(0x200),1,&list,forward_execute);}
void source_cases(){
    using namespace ngx_stereo_host;
    auto dimensions=[](){const uint32_t values[]{1706,720,2560,1080,1,0x12};
        for(unsigned i=0;i<6;++i)put(creation,i*4,values[i]);put(creation,0x18,uint8_t(1));};
    auto dispatch=[](){return hook_create(0x100,1,parameters,reinterpret_cast<uintptr_t*>(base+handle_cell_rva));};
    auto complete=[&](){WrapperScope scope(false,true,base+0xbe66bc,0x100,creation.data());
        NativeCreateSource pending;assert(!native_create_source(9,pending));scope.normal_return(dispatch());};
    NativeCreateSource source;uint32_t w=7,h=9;
    host_reset();dimensions();assert(native_create_state(9,source)==NativeCreateAvailability::unavailable);
    handle=0;assert(native_create_state(9,source)==NativeCreateAvailability::inactive);
    cells_available=false;assert(native_create_state(9,source)==NativeCreateAvailability::unavailable);cells_available=true;
    complete();
    assert(native_create_source(9,source)&&source.fields[0]==1706&&source.fields[1]==720&&source.fields[4]==1&&source.fields[5]==0x12&&source.fields[6]==1);
    assert(source.split_input(2560,1080,w,h)&&w==853&&h==720);
    w=7;h=9;assert(!source.split_input(4096,2048,w,h)&&w==7&&h==9); // No ratio scaling to owned output.
    auto invalid=source;invalid.fields[0]=1707;assert(!invalid.split_input(2560,1080,w,h));
    invalid=source;invalid.fields[1]=1081;assert(!invalid.split_input(2560,1080,w,h));
    // Diagnostic quotas can exhaust; actual latest Create semantics remain fresh.
    const auto first=source.revision;for(unsigned i=0;i<5;++i)complete();
    assert(epochs>=4&&native_create_source(9,source)&&source.revision>first);
    assert(!native_create_source(8,source)&&!source.revision);
    ++handle;assert(!native_create_source(9,source));--handle;assert(!native_create_source(9,source));complete();
    ++parameters;assert(!native_create_source(9,source));--parameters;assert(!native_create_source(9,source));complete();
    committed_world_generation=10;assert(!native_create_source(9,source)&&!native_create_source(10,source));committed_world_generation=9;
    assert(!native_create_source(9,source));complete();
    cells_available=false;assert(!native_create_source(9,source));cells_available=true;assert(!native_create_source(9,source));complete();
    recording=false;assert(!native_create_source(9,source));recording=true;
    hook_release(handle);assert(!native_create_source(9,source));
    complete();assert(native_create_source(9,source));
    // Failure, missing/duplicate dispatcher, mutated fields, unrelated callsite
    // and nested/concurrent scope all invalidate rather than revive old fields.
    create_raw=0xbad00001;complete();assert(!native_create_source(9,source));create_raw=0x12345678;
    complete();{WrapperScope scope(false,true,base+0xbe66bc,0x100,creation.data());scope.normal_return(create_raw);}
    assert(!native_create_source(9,source));
    complete();{WrapperScope scope(false,true,base+0xbe66bc,0x100,creation.data());dispatch();scope.normal_return(dispatch());}
    assert(!native_create_source(9,source));
    complete();{WrapperScope scope(false,true,base+0xbe66bc,0x100,creation.data());scope.normal_return(dispatch());put(creation,0,uint32_t(1600));}
    assert(!native_create_source(9,source));dimensions();
    complete();{WrapperScope scope(false,true,base+0xbe66bd,0x100,creation.data());scope.normal_return(dispatch());}
    assert(!native_create_source(9,source));
    complete();{WrapperScope scope(false,true,base+0xbe66bc,0x100,creation.data());
        {WrapperScope nested(true,true,base+0x9a35d3,0x100,evaluation.data());}scope.normal_return(dispatch());}
    assert(!native_create_source(9,source));
    complete();{WrapperScope scope(false,true,base+0xbe66bc,0x100,creation.data());
        std::thread competing([&]{complete();});competing.join();scope.normal_return(dispatch());}
    assert(!native_create_source(9,source));
    complete();{WrapperScope scope(false,true,base+0xbe66bc,0x100,creation.data());scope.normal_return(dispatch());
        std::thread release_during_create([&]{hook_release(handle);});release_during_create.join();}
    assert(!native_create_source(9,source));
    complete();try{WrapperScope scope(false,true,base+0xbe66bc,0x100,creation.data());dispatch();throw std::runtime_error("Create wrapper");}
    catch(std::runtime_error const&){}assert(!native_create_source(9,source));
    complete();unreadable=reinterpret_cast<uintptr_t>(creation.data());complete();unreadable=0;
    assert(!native_create_source(9,source));
    complete();dispatch();assert(!native_create_source(9,source)); // Foreign dispatcher, no wrapper.
}
void host_cases(){
    host_reset();observe_inputs();eval();copy_execute();auto const& row=ngx_stereo_host::rows[0];
    assert(dispatch_calls==1&&copy_calls==1&&execute_calls==1&&row.dispatcher_match&&row.dispatcher_returned&&row.returned&&row.wrapper_result==0xfedcba9876543210);
    assert(row.typed_mask==15&&row.producer_mask==1&&row.copies==1&&row.copy_returned&&row.execute_generation_match&&row.execute_returned);
    assert(row.eval_execute_returned&&row.eval_execute_generation_match&&row.eval_execute_occurrences==1);
    assert(!row.retained_mask&&!row.typed_set_ready); // Synthetic raw metadata cannot become retained COM proof.
    assert(ngx_stereo_host::typed_calls==6&&!ngx_stereo_host::in_wrappers&&!active.load());
    host_reset();eval();assert(!ngx_stereo_host::rows[0].typed_mask&&!ngx_stereo_host::typed_calls); // Raw NGX pointers never probed.
    host_reset();observe_inputs();ngx_route_observer::original_evaluate=wrapper_forward;test_caller=base+0x9a35d3;
    assert(ngx_route_observer::hook_evaluate(0x100,0,0,evaluation.data())==0xfedcba9876543210);
    assert(ngx_stereo_host::row_count==1&&dispatch_calls==1&&ngx_stereo_host::rows[0].dispatcher_match&&ngx_stereo_host::rows[0].returned);
    host_reset();cells_available=false;eval();assert(!ngx_stereo_host::rows[0].dispatcher_match&&dispatch_calls==1);
    host_reset();ngx_stereo_host::typed_observations=512;observe_inputs();eval();
    assert(!ngx_stereo_host::typed_calls&&!ngx_stereo_host::rows[0].typed_mask&&ngx_stereo_host::rows[0].bank_dropped==4);
    host_reset();ngx_stereo_host::sampled_tickets=8;observe_inputs();eval();assert(!ngx_stereo_host::typed_calls&&!ngx_stereo_host::rows[0].typed_mask);
    // Pre-menu normal frames may exhaust a bank before any NGX feature exists.
    // Only a real observed Create epoch replenishes it, never arbitrary Eval.
    host_reset();
    for(unsigned epoch=0;epoch<4;++epoch){
        if(epoch){ngx_stereo_host::WrapperScope create(false,true,base+0xbe66bc,0x100,creation.data());create.normal_return(1);}
        assert(ngx_stereo_host::epochs==epoch);
        for(unsigned ticket=1;ticket<=8;++ticket){
            consuming_frame_ticket=consuming_frozen_frame.id=100*epoch+ticket;
            for(unsigned repeat=0;repeat<16;++repeat)observe_inputs();
        }
        assert(ngx_stereo_host::sampled_tickets==8&&ngx_stereo_host::typed_observations==512);
        assert(ngx_stereo_host::typed_calls==512*(epoch+1));
        observe_inputs(); // Same-ticket typed quota is exhausted.
        consuming_frame_ticket=consuming_frozen_frame.id=100*epoch+9;
        observe_inputs(); // A ninth ticket cannot bypass either quota.
        assert(ngx_stereo_host::typed_calls==512*(epoch+1));
        consuming_frame_ticket=consuming_frozen_frame.id=100*epoch+8;
        eval();assert(ngx_stereo_host::rows[ngx_stereo_host::row_count-1].typed_mask==15);
        observe_inputs();assert(ngx_stereo_host::typed_calls==512*(epoch+1)); // Eval never replenishes.
    }
    {ngx_stereo_host::WrapperScope create(false,true,base+0xbe66bc,0x100,creation.data());create.normal_return(1);}
    observe_inputs();assert(ngx_stereo_host::epochs==4&&ngx_stereo_host::typed_calls==2048);
    assert(ngx_stereo_host::total_sampled_tickets==32&&ngx_stereo_host::total_typed_observations==2048);
    host_reset();observe_inputs();eval();frame_link.generation=2;auto* list=reinterpret_cast<ID3D12CommandList*>(0x100);
    ngx_stereo_host::execute_passthrough(reinterpret_cast<ID3D12CommandQueue*>(0x200),1,&list,forward_execute);
    assert(!ngx_stereo_host::rows[0].execute_generation_match&&execute_calls==1);
    assert(!ngx_stereo_host::rows[0].eval_execute_generation_match);
    // Copy execution on another list is independent of Eval's actual recording.
    host_reset();observe_inputs();eval();ngx_stereo_host::rows[0].copy_list={0x300,0x300,1};
    auto* different=reinterpret_cast<ID3D12CommandList*>(0x300);
    const auto any_execute=[](ID3D12CommandQueue*,UINT,ID3D12CommandList* const*){unlocked();++execute_calls;};
    ngx_stereo_host::execute_passthrough(reinterpret_cast<ID3D12CommandQueue*>(0x200),1,&different,any_execute);
    assert(ngx_stereo_host::rows[0].execute_returned&&!ngx_stereo_host::rows[0].eval_execute_returned);
    list=reinterpret_cast<ID3D12CommandList*>(0x100);
    ngx_stereo_host::execute_passthrough(reinterpret_cast<ID3D12CommandQueue*>(0x200),1,&list,forward_execute);
    assert(ngx_stereo_host::rows[0].eval_execute_generation_match&&ngx_stereo_host::rows[0].eval_execute_order==2&&
        ngx_stereo_host::rows[0].execute_order==1&&execute_calls==2);
    ngx_stereo_host::execute_passthrough(reinterpret_cast<ID3D12CommandQueue*>(0x200),1,&list,forward_execute);
    assert(ngx_stereo_host::rows[0].eval_execute_occurrences==2&&!ngx_stereo_host::rows[0].eval_execute_generation_match);
    host_reset();throw_dispatch=true;try{eval();assert(false);}catch(std::runtime_error const&){}
    assert(dispatch_calls==1&&!ngx_stereo_host::rows[0].returned&&ngx_stereo_host::rows[0].unwinding&&!ngx_stereo_host::rows[0].dispatcher_returned&&!ngx_stereo_host::in_wrappers);
    host_reset();for(unsigned i=0;i<5;++i)eval();assert(dispatch_calls==5&&ngx_stereo_host::row_count==2&&!ngx_stereo_host::in_wrappers);
    host_reset();{ngx_stereo_host::WrapperScope outer(true,true,base+0x9a35d3,0x100,evaluation.data());recording=false;
        {ngx_stereo_host::WrapperScope skipped(true,true,base+0x9a35d3,0x100,evaluation.data());}assert(ngx_stereo_host::in_wrappers==1);recording=true;}
    assert(!ngx_stereo_host::in_wrappers);
    host_reset();{ngx_stereo_host::WrapperScope create(false,true,base+0xbe66bc,0x100,creation.data());
        auto result=ngx_stereo_host::hook_create(0x100,1,parameters,reinterpret_cast<uintptr_t*>(base+handle_cell_rva));assert(result==0x12345678);create.normal_return(result);}
    assert(ngx_stereo_host::rows[0].dispatcher_match&&ngx_stereo_host::rows[0].handle_output==handle);
    assert(ngx_stereo_host::hook_release(handle)==0x87654321&&release_calls==1&&ngx_stereo_host::releases[0].returned);
    host_reset();observe_inputs();eval();recording=false;FILE* report{};assert(!tmpfile_s(&report));ngx_stereo_host::print(report);
    assert(contains(report,"gpu_completion_verified=0")&&contains(report,"ngx_stereo_host_resource row=0 input=3")&&contains(report,"safe_retirement_verified=0")==false);fclose(report);
    host_reset();FILE* rejected{};assert(!tmpfile_s(&rejected));assert(!ngx_stereo_host::enable(rejected)&&!ngx_stereo_host::attested);fclose(rejected);
}
}
int main(){assert(route_observer_test_main()==0);source_cases();host_cases();std::puts("NgxStereoHostLiveTest passed (forwarding/provenance only; no SDK/GPU completion claims)");return 0;}
