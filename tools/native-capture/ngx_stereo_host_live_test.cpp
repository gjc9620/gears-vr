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
unsigned dispatch_calls{},copy_calls{},execute_calls{},release_calls{};bool throw_dispatch{};
uint32_t __fastcall forward_eval(uintptr_t command,uintptr_t h,uintptr_t p,uintptr_t callback){unlocked();++dispatch_calls;
    assert(command==0x100&&h==handle&&p==parameters&&!callback);if(throw_dispatch)throw std::runtime_error("dispatcher");return 0xbad01234;}
uint32_t __fastcall forward_create(uintptr_t command,uint32_t feature,uintptr_t p,uintptr_t* output){unlocked();++dispatch_calls;
    assert(command==0x100&&feature==1&&p==parameters&&reinterpret_cast<uintptr_t>(output)==base+handle_cell_rva);handle=0x12345;return 0x12345678;}
uint32_t __fastcall forward_release(uintptr_t h){unlocked();++release_calls;assert(h==handle);return 0x87654321;}
void STDMETHODCALLTYPE forward_copy(ID3D12GraphicsCommandList* command,D3D12_TEXTURE_COPY_LOCATION const* dst,UINT x,UINT y,UINT z,D3D12_TEXTURE_COPY_LOCATION const* src,D3D12_BOX const* box){
    unlocked();++copy_calls;assert(command==reinterpret_cast<ID3D12GraphicsCommandList*>(0x100)&&!x&&!y&&!z&&!box);assert(src->pResource==reinterpret_cast<ID3D12Resource*>(0x501)&&dst->pResource==reinterpret_cast<ID3D12Resource*>(0x500));}
void STDMETHODCALLTYPE forward_execute(ID3D12CommandQueue* queue,UINT count,ID3D12CommandList* const* commands){unlocked();++execute_calls;assert(queue==reinterpret_cast<ID3D12CommandQueue*>(0x200)&&count==1&&commands[0]==reinterpret_cast<ID3D12CommandList*>(0x100));}
void host_reset(){reset();using namespace ngx_stereo_host;attested=true;printed=false;bank={};rows={};releases={};row_count=epoch_evals=epochs=in_wrappers=release_count=0;
    sampled_tickets=typed_observations=0;
    sequence=execute_sequence=limited=foreign_dispatches=read_failures=0;current_row=pending_row=-1;typed_calls=0;
    ngx_stereo_host::original_create=forward_create;ngx_stereo_host::original_evaluate=forward_eval;original_release=forward_release;frame_link.generation=1;
    dispatch_calls=copy_calls=execute_calls=release_calls=0;throw_dispatch=false;
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
void host_cases(){
    host_reset();observe_inputs();eval();copy_execute();auto const& row=ngx_stereo_host::rows[0];
    assert(dispatch_calls==1&&copy_calls==1&&execute_calls==1&&row.dispatcher_match&&row.dispatcher_returned&&row.returned&&row.wrapper_result==0xfedcba9876543210);
    assert(row.typed_mask==15&&row.producer_mask==1&&row.copies==1&&row.copy_returned&&row.execute_generation_match&&row.execute_returned);
    assert(ngx_stereo_host::typed_calls==6&&!ngx_stereo_host::in_wrappers&&!active.load());
    host_reset();eval();assert(!ngx_stereo_host::rows[0].typed_mask&&!ngx_stereo_host::typed_calls); // Raw NGX pointers never probed.
    host_reset();observe_inputs();ngx_route_observer::original_evaluate=wrapper_forward;test_caller=base+0x9a35d3;
    assert(ngx_route_observer::hook_evaluate(0x100,0,0,evaluation.data())==0xfedcba9876543210);
    assert(ngx_stereo_host::row_count==1&&dispatch_calls==1&&ngx_stereo_host::rows[0].dispatcher_match&&ngx_stereo_host::rows[0].returned);
    host_reset();cells_available=false;eval();assert(!ngx_stereo_host::rows[0].dispatcher_match&&dispatch_calls==1);
    host_reset();ngx_stereo_host::typed_observations=512;observe_inputs();eval();
    assert(!ngx_stereo_host::typed_calls&&!ngx_stereo_host::rows[0].typed_mask&&ngx_stereo_host::rows[0].bank_dropped==4);
    host_reset();ngx_stereo_host::sampled_tickets=8;observe_inputs();eval();assert(!ngx_stereo_host::typed_calls&&!ngx_stereo_host::rows[0].typed_mask);
    host_reset();observe_inputs();eval();frame_link.generation=2;auto* list=reinterpret_cast<ID3D12CommandList*>(0x100);
    ngx_stereo_host::execute_passthrough(reinterpret_cast<ID3D12CommandQueue*>(0x200),1,&list,forward_execute);
    assert(!ngx_stereo_host::rows[0].execute_generation_match&&execute_calls==1);
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
int main(){assert(route_observer_test_main()==0);host_cases();std::puts("NgxStereoHostLiveTest passed (forwarding/provenance only; no SDK/GPU completion claims)");return 0;}
