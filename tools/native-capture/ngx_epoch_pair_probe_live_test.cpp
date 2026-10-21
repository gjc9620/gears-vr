#define GEARS_NGX_EPOCH_PAIR_OBSERVER 1
#define GEARS_NGX_STEREO_HOST_OBSERVER 1
#include "ngx_epoch_pair_probe.h"
#include <atomic>
#include <cstring>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>
namespace gears_submitted {struct FrameTicket {uint64_t id{},world_generation{};bool enabled{};};}
namespace gears_ngx_route {constexpr uintptr_t handle_cell_rva=0x1c20bd0,parameter_cell_rva=0x1c20bd8;}
namespace ngx_stereo_host {struct NativeCreateSource {uint64_t revision{},world{};uintptr_t handle{},parameters{};std::array<uint32_t,7> fields{};};}
namespace {
using FrameHookMutex=std::recursive_mutex;
std::recursive_mutex frame_hook_mutex;std::atomic<bool> recording{true};std::atomic<unsigned> active{0};std::atomic<uint64_t> committed_world_generation{2};
std::vector<unsigned char> module(0x1c3b400),renderer_bytes(0x100),left_bytes(0x430),right_bytes(0x430);
uintptr_t base=reinterpret_cast<uintptr_t>(module.data());void* consuming_renderer=renderer_bytes.data();uint64_t consuming_frame_ticket=10;
gears_submitted::FrameTicket consuming_frozen_frame{10,2,true};
struct Lifetime {bool frame(uintptr_t ptr,gears_submitted::FrameTicket& f){if(ptr!=reinterpret_cast<uintptr_t>(consuming_renderer))return false;f=consuming_frozen_frame;return true;}} submitted_lifetime;
struct Record {int family_count=2,renderer_count=2;std::array<uintptr_t,2> family_views{reinterpret_cast<uintptr_t>(left_bytes.data()),reinterpret_cast<uintptr_t>(right_bytes.data())};};
void inspect_completed_renderer(Record& r,void*){r=Record{};}
struct Tracker {uint64_t generation=1;struct Ref {uintptr_t identity;uint64_t generation;};Ref current(uintptr_t id){return {id,generation};}struct Snapshot {uint64_t list_overflow{};bool capacity_bad{};};Snapshot stats{};Snapshot snapshot(){return stats;}} frame_link;
unsigned bounds_reads{},bounds_fault{};
bool safe_copy(void* out,void const* in,size_t n){if(!in||!out)return false;
    if(reinterpret_cast<uintptr_t>(in)==base+0x1ad1960){++bounds_reads;if(bounds_fault==1)return false;}
    std::memcpy(out,in,n);
    if(reinterpret_cast<uintptr_t>(in)==base+0x1ad1960&&bounds_reads==1){
        if(bounds_fault==2)++*reinterpret_cast<uint32_t*>(module.data()+0x1ad1968);
        if(bounds_fault==3)++*reinterpret_cast<uintptr_t*>(module.data()+gears_ngx_route::handle_cell_rva);
        if(bounds_fault==4)++committed_world_generation;
    }return true;}
uintptr_t canonical_identity(IUnknown* p){Microsoft::WRL::ComPtr<IUnknown> id;if(!p||FAILED(p->QueryInterface(IID_PPV_ARGS(&id))))return 0;return reinterpret_cast<uintptr_t>(id.Get());}
}
#include "ngx_epoch_pair_probe.inl"
using Microsoft::WRL::ComPtr;
static unsigned checks{};
static void check(bool value){++checks;if(!value)throw std::runtime_error("live epoch probe check failed");}
static void hr(HRESULT value){check(SUCCEEDED(value));}
template<class T> static void put(unsigned char* bytes,size_t offset,T const& value){std::memcpy(bytes+offset,&value,sizeof(value));}
static void bounds_cases(){
    const uintptr_t h=300,p=400;const std::array<uint32_t,4> bounds{3840,2160,1920,1080};
    auto arm=[&](unsigned fault){recording=false;ngx_epoch_pair::reset_capture(true);recording=true;committed_world_generation=2;
        put(module.data(),gears_ngx_route::handle_cell_rva,h);put(module.data(),gears_ngx_route::parameter_cell_rva,p);
        put(module.data(),0x1ad1960,bounds);bounds_reads=0;bounds_fault=fault;
        ngx_epoch_pair::created({1,2,h,p,{2560,1440,3840,2160,2,67,0}});};
    arm(0);check(ngx_epoch_pair::create_bounds[0].stable&&ngx_epoch_pair::create_bounds[0].first==bounds);
    auto observed=ngx_epoch_pair::sample_bounds(10,0x123,0x456);
    check(observed.stable&&observed.ticket==10&&observed.renderer==0x123&&observed.scope==0x456);
    for(unsigned fault=1;fault<=4;++fault){arm(fault);auto const& b=ngx_epoch_pair::create_bounds[0];check(!b.stable);
        if(fault==1)check(!b.readable&&b.identity);
        if(fault==2)check(b.readable&&b.identity&&b.first!=b.last);
        if(fault>=3)check(b.readable&&!b.identity);}
    arm(0);ngx_epoch_pair::released(h);put(module.data(),0x1ad1968,uint32_t(960));
    ngx_epoch_pair::created({2,2,h,p,{1280,1440,1920,2160,2,67,0}});
    check(ngx_epoch_pair::create_bounds[0].first[2]==1920&&ngx_epoch_pair::create_bounds[1].revision==2&&ngx_epoch_pair::create_bounds[1].first[2]==960);
    Record record;ngx_epoch_pair::built(consuming_renderer,consuming_frozen_frame,record,true,nullptr,nullptr);
    const auto before_reads=bounds_reads;consuming_frame_ticket=11;
    ngx_epoch_pair::eval_enter(99,true,base+0x9a35d3,1,nullptr,false);
    check(bounds_reads==before_reads&&!ngx_epoch_pair::eval_bounds[0].revision);consuming_frame_ticket=10;
    ngx_epoch_pair::created({3,2,h,p,{1280,1440,1920,2160,2,67,0}});
    check(ngx_epoch_pair::probe.revision==2&&ngx_epoch_pair::create_bounds[1].revision==2&&!ngx_epoch_pair::create_bounds[2].revision);
    recording=false;ngx_epoch_pair::reset_capture(false);check(!ngx_epoch_pair::create_bounds[0].revision&&!ngx_epoch_pair::eval_bounds[0].revision);recording=true;bounds_fault=0;
}
int main(int argc,char** argv){try{
    bounds_cases();if(argc==2&&std::strcmp(argv[1],"--bounds-only")==0){std::printf("Bounds CPU observer tests passed: %u checks; no GPU/game\n",checks);return 0;}

    // Inactive observation must not dereference even a synthetic caller's
    // list/arguments on return. The ordinary copy self-test uses such a list.
    ngx_epoch_pair::reset_capture(false);
    auto invalid_list=reinterpret_cast<ID3D12GraphicsCommandList*>(1);
    auto invalid_copy=reinterpret_cast<D3D12_TEXTURE_COPY_LOCATION const*>(1);
    {ngx_epoch_pair::CopyScope ignored(invalid_list,invalid_copy,0,0,0,invalid_copy,nullptr,0);
        check(ignored.index<0);ignored.returned(invalid_list,invalid_copy,invalid_copy,nullptr);}
    const uint32_t extent[]{2133,720},buffer[]{2560,1080};std::memcpy(renderer_bytes.data()+0xd0,extent,8);put(module.data(),0x1c3b3b8,buffer[0]);put(module.data(),0x1c3b3bc,buffer[1]);
    const uintptr_t h=300,p=400;put(module.data(),gears_ngx_route::handle_cell_rva,h);put(module.data(),gears_ngx_route::parameter_cell_rva,p);
    const std::array<float,4> sr0{0,0,853,720},sr1{1280,0,853,720};const std::array<int32_t,4> tr0{0,0,853,720},tr1{1280,0,853,720};
    put(left_bytes.data(),0x48,sr0);put(right_bytes.data(),0x48,sr1);put(left_bytes.data(),0x58,tr0);put(right_bytes.data(),0x58,tr1);put(left_bytes.data(),0x3d0,sr0);put(right_bytes.data(),0x3d0,sr1);
    ngx_epoch_pair::reset_capture(true);ngx_epoch_pair::created({1,2,h,p,{1706,720,2560,1080,1,0x12,1}});
    Record record;ngx_epoch_pair::built(consuming_renderer,consuming_frozen_frame,record,true,sr0.data(),sr1.data());ngx_epoch_pair::prepare(1,consuming_renderer,record);ngx_epoch_pair::prepare(2,consuming_renderer,record);
    check(ngx_epoch_pair::probe.selected&&ngx_epoch_pair::probe.geometry[0].constructor_valid==3);
    ComPtr<ID3D12Device> device;hr(D3D12CreateDevice(nullptr,D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)));
    ComPtr<ID3D12CommandQueue> queue;D3D12_COMMAND_QUEUE_DESC qd{};hr(device->CreateCommandQueue(&qd,IID_PPV_ARGS(&queue)));
    ComPtr<ID3D12CommandAllocator> allocator;hr(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)));
    ComPtr<ID3D12GraphicsCommandList> list;hr(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)));
    ngx_epoch_pair::created_list(list.Get(),true,true);check(ngx_epoch_pair::typed_lists.held()==1);
    ComPtr<ID3D12Fence> fence;hr(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)));
    std::array<ComPtr<ID3D12Resource>,4> resources;D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;D3D12_RESOURCE_DESC desc{};
    desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;desc.Width=2560;desc.Height=1080;desc.MipLevels=desc.DepthOrArraySize=1;desc.SampleDesc.Count=1;desc.Format=DXGI_FORMAT_R11G11B10_FLOAT;
    for(unsigned i=0;i<4;++i){if(i==2)desc.Format=DXGI_FORMAT_R32_FLOAT;if(i==3)desc.Format=DXGI_FORMAT_R16G16_FLOAT;
        hr(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(&resources[i])));}
    for(unsigned eye=0;eye<2;++eye){
        std::array<D3D12_RESOURCE_BARRIER,4> barriers{};const unsigned n=eye?1:4;
        for(unsigned i=0;i<n;++i){auto& b=barriers[i];b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition.pResource=resources[i].Get();b.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            b.Transition.StateBefore=eye?D3D12_RESOURCE_STATE_COPY_DEST:D3D12_RESOURCE_STATE_COMMON;b.Transition.StateAfter=i==1?D3D12_RESOURCE_STATE_COPY_SOURCE:D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;}
        {ngx_epoch_pair::BarrierScope observed(list.Get(),n,barriers.data(),base+11);list->ResourceBarrier(n,barriers.data());observed.returned(list.Get());}
        std::array<unsigned char,0x168> eval{};const unsigned offsets[]{0,8,0x18,0x20};for(unsigned i=0;i<4;++i)put(eval.data(),offsets[i],reinterpret_cast<uintptr_t>(resources[i].Get()));
        put(eval.data(),0x30,uint32_t(1280));put(eval.data(),0x34,uint32_t(1080));put(eval.data(),0x60,uint32_t(eye*1280));auto unchanged=eval;
        ngx_epoch_pair::eval_enter(10000+eye,true,base+0x9a35d3,reinterpret_cast<uintptr_t>(list.Get()),eval.data(),false);
        const int index=ngx_epoch_pair::dispatch(reinterpret_cast<uintptr_t>(list.Get()),h,p,0);check(index==int(eye));ngx_epoch_pair::dispatched(index,1);ngx_epoch_pair::eval_leave(10000+eye,true,1);check(eval==unchanged);
        auto b=barriers[0];b.Transition.StateBefore=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;b.Transition.StateAfter=D3D12_RESOURCE_STATE_COPY_DEST;
        {ngx_epoch_pair::BarrierScope observed(list.Get(),1,&b,base+12);list->ResourceBarrier(1,&b);observed.returned(list.Get());}
        D3D12_TEXTURE_COPY_LOCATION src{},dst{};src.pResource=resources[1].Get();dst.pResource=resources[0].Get();src.Type=dst.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        // Real color_copy_observer passes _ReturnAddress()-base through both
        // passthrough layers unchanged. Every call still executes its original
        // D3D Copy; observation alone rejects wrong units or a foreign RVA.
        unsigned originals{};
        auto copy_passthrough=[&](uintptr_t caller_rva){
            ngx_epoch_pair::CopyScope observed(list.Get(),&dst,0,0,0,&src,nullptr,caller_rva);
            list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);++originals;
            observed.returned(list.Get(),&dst,&src,nullptr);return observed.index;};
        const auto prior_events=ngx_epoch_pair::probe.event_count;
        check(copy_passthrough(base+0x1c6093)==-1&&ngx_epoch_pair::probe.evals[eye].copies==0);
        check(copy_passthrough(0x1c6094)==-1&&ngx_epoch_pair::probe.event_count==prior_events);
        const uintptr_t caller_rva=(base+0x1c6093)-base;
        check(copy_passthrough(caller_rva)==int(eye)&&ngx_epoch_pair::probe.evals[eye].copies==1&&originals==3);
        check(ngx_epoch_pair::probe.events[prior_events].caller==0x1c6093);
        hr(list->Close());ID3D12CommandList* batch[]{list.Get()};auto* saved_renderer=consuming_renderer;consuming_renderer=nullptr;{ngx_epoch_pair::ExecuteScope observed(queue.Get(),1,batch);queue->ExecuteCommandLists(1,batch);observed.returned();}consuming_renderer=saved_renderer;
        // Independent test fence only; observer never submits a fence or waits.
        hr(queue->Signal(fence.Get(),eye+1));const auto end=GetTickCount64()+5000;while(fence->GetCompletedValue()<eye+1&&GetTickCount64()<end)Sleep(1);check(fence->GetCompletedValue()==eye+1);
        if(!eye){const auto before=ngx_epoch_pair::list(list.Get());hr(allocator->Reset());hr(list->Reset(allocator.Get(),nullptr));++frame_link.generation;ngx_epoch_pair::reset(before,list.Get(),S_OK);}
    }
    check(ngx_epoch_pair::probe.complete());check(ngx_epoch_pair::eval_bounds[0].ticket==10&&ngx_epoch_pair::eval_bounds[1].ticket==10&&ngx_epoch_pair::eval_bounds[0].scope==10000&&ngx_epoch_pair::eval_bounds[1].scope==10001);recording=false;FILE* output{};check(tmpfile_s(&output)==0&&output);ngx_epoch_pair::print(output);rewind(output);
    std::string text;char buffer_line[2048];while(fgets(buffer_line,sizeof(buffer_line),output))text+=buffer_line;fclose(output);
    check(text.find("candidate_complete=1")!=std::string::npos&&text.find("producer_domain_verified=0")!=std::string::npos&&text.find("gpu_completion_verified=0")!=std::string::npos);
    check(text.find("stage=prepare_after")!=std::string::npos&&text.find("typed_before=f")!=std::string::npos&&text.find("format=41")!=std::string::npos);
    check(text.find("membership_frozen_at_execute_begin=1 earlier_history_complete=0")!=std::string::npos);
    check(!ngx_epoch_pair::probe.resources[0].lease.retained()&&!ngx_epoch_pair::typed_lists.held());
    recording=true;ngx_epoch_pair::reset_capture(true);ngx_epoch_pair::created({2,2,h,p,{1706,720,2560,1080,1,0x12,1}});ngx_epoch_pair::released(h);check(ngx_epoch_pair::probe.stop==2);
    ngx_epoch_pair::reset_capture(true);ngx_epoch_pair::created({3,2,h,p,{1706,720,2560,1080,1,0x12,1}});committed_world_generation=3;check(!ngx_epoch_pair::selected()&&ngx_epoch_pair::probe.stop==2);
    // A real successful Reset supplies identity without any selected Barrier.
    committed_world_generation=2;
    auto arm=[&](uint64_t revision){recording=false;active=0;ngx_epoch_pair::reset_capture(true);recording=true;
        ngx_epoch_pair::created({revision,2,h,p,{1706,720,2560,1080,1,0x12,1}});
        ngx_epoch_pair::built(consuming_renderer,consuming_frozen_frame,record,true,sr0.data(),sr1.data());};
    arm(4);auto before=ngx_epoch_pair::list(list.Get());hr(allocator->Reset());hr(list->Reset(allocator.Get(),nullptr));++frame_link.generation;ngx_epoch_pair::reset(before,list.Get(),S_OK);
    check(ngx_epoch_pair::probe.event_count==0&&ngx_epoch_pair::typed_lists.held()==1);
    std::array<unsigned char,0x168> empty_eval{};
    const auto raw_list=reinterpret_cast<uintptr_t>(list.Get());ngx_epoch_pair::eval_enter(20000,true,base+0x9a35d3,raw_list,empty_eval.data(),false);
    check(ngx_epoch_pair::probe.eval_count==1&&ngx_epoch_pair::typed_lists.pinned==1&&!ngx_epoch_pair::probe.rejection.present);
    ngx_epoch_pair::eval_leave(20000,true,1);
    // Synthetic failure HRESULT: the prior successful typed boundary and the
    // eval_begin row must not be usable as a fallback after invalidation.
    before=ngx_epoch_pair::list(list.Get());ngx_epoch_pair::reset(before,list.Get(),E_FAIL);
    ngx_epoch_pair::eval_enter(20001,true,base+0x9a35d3,raw_list,empty_eval.data(),false);
    check(ngx_epoch_pair::probe.stop==3&&ngx_epoch_pair::probe.rejection.lookup_reason==gears_ngx_epoch::TypedLists::boundary_failed&&ngx_epoch_pair::probe.eval_count==1);
    check(ngx_epoch_pair::probe.rejection.mask&gears_ngx_epoch::Probe::typed_lookup_failed);
    const auto frozen=ngx_epoch_pair::probe.rejection.input.scope;ngx_epoch_pair::eval_enter(20002,true,base+0x9a35d3,raw_list,empty_eval.data(),true);check(ngx_epoch_pair::probe.rejection.input.scope==frozen);
    const auto held=ngx_epoch_pair::typed_lists.held();ngx_epoch_pair::reset_capture(false);check(ngx_epoch_pair::typed_lists.held()==held&&ngx_epoch_pair::probe.rejection.present);
    check(tmpfile_s(&output)==0&&output);ngx_epoch_pair::print(output);check(ngx_epoch_pair::typed_lists.held()==held&&!ngx_epoch_pair::printed);
    recording=false;active=1;ngx_epoch_pair::print(output);check(ngx_epoch_pair::typed_lists.held()==held&&!ngx_epoch_pair::printed);
    active=0;ngx_epoch_pair::print(output);check(!ngx_epoch_pair::typed_lists.held()&&ngx_epoch_pair::printed);rewind(output);text.clear();while(fgets(buffer_line,sizeof(buffer_line),output))text+=buffer_line;fclose(output);
    check(text.find("ngx_epoch_reject mask=")!=std::string::npos&&text.find("lookup=3 world=2 ticket=10")!=std::string::npos);
    // No actual Create epoch means no retained candidates, even if enabled.
    ngx_epoch_pair::reset_capture(true);recording=true;ngx_epoch_pair::created_list(list.Get(),true,true);ngx_epoch_pair::creation_failed();check(!ngx_epoch_pair::typed_lists.held()&&!ngx_epoch_pair::typed_lists.create_failures);
    // Empty Create rearm drops the prior directory; no cross-epoch candidates.
    ngx_epoch_pair::created({5,2,h,p,{1706,720,2560,1080,1,0x12,1}});
    ComPtr<ID3D12Device4> device4;hr(device.As(&device4));ComPtr<ID3D12GraphicsCommandList> closed;
    hr(device4->CreateCommandList1(0,D3D12_COMMAND_LIST_TYPE_DIRECT,D3D12_COMMAND_LIST_FLAG_NONE,IID_PPV_ARGS(&closed)));
    ngx_epoch_pair::created_list(closed.Get(),false,true);check(ngx_epoch_pair::typed_lists.held()==1);
    ngx_epoch_pair::released(h);ngx_epoch_pair::created({6,2,h,p,{1706,720,2560,1080,1,0x12,1}});
    check(!ngx_epoch_pair::typed_lists.held()&&ngx_epoch_pair::probe.revision==6&&ngx_epoch_pair::probe.empty_epoch_count==1);
    arm(7);ngx_epoch_pair::created_list(closed.Get(),false,true);ngx_epoch_pair::eval_enter(30000,true,base+0x9a35d3,reinterpret_cast<uintptr_t>(closed.Get()),empty_eval.data(),false);
    check(ngx_epoch_pair::probe.rejection.lookup_reason==gears_ngx_epoch::TypedLists::closed);
    arm(8);ngx_epoch_pair::creation_failed();check(ngx_epoch_pair::typed_lists.create_failures==1&&!ngx_epoch_pair::typed_lists.held());
    // Existing global tracker capacity loss remains a conservative rejection.
    frame_link.stats={7,true};ngx_epoch_pair::created_list(list.Get(),true,false);ngx_epoch_pair::eval_enter(40000,true,base+0x9a35d3,raw_list,empty_eval.data(),false);
    check(ngx_epoch_pair::probe.rejection.lookup_reason==gears_ngx_epoch::TypedLists::generation_missing&&ngx_epoch_pair::probe.rejection.tracker_overflow==7&&ngx_epoch_pair::probe.rejection.tracker_capacity_bad);
    recording=false;ngx_epoch_pair::reset_capture(false);check(!ngx_epoch_pair::typed_lists.held());hr(list->Close());
    std::printf("NGX live epoch probe tests passed: %u checks (real D3D barriers/copies/Execute, synthetic SDK)\n",checks);return 0;
}catch(std::exception const& e){std::fprintf(stderr,"%s (%u)\n",e.what(),checks);return 1;}}
