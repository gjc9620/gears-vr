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
std::recursive_mutex frame_hook_mutex;std::atomic<bool> recording{true};std::atomic<unsigned> active{0};std::atomic<uint64_t> committed_world_generation{2};
std::vector<unsigned char> module(0x1c3b400),renderer_bytes(0x100),left_bytes(0x430),right_bytes(0x430);
uintptr_t base=reinterpret_cast<uintptr_t>(module.data());void* consuming_renderer=renderer_bytes.data();uint64_t consuming_frame_ticket=10;
gears_submitted::FrameTicket consuming_frozen_frame{10,2,true};
struct Lifetime {bool frame(uintptr_t ptr,gears_submitted::FrameTicket& f){if(ptr!=reinterpret_cast<uintptr_t>(consuming_renderer))return false;f=consuming_frozen_frame;return true;}} submitted_lifetime;
struct Record {int family_count=2,renderer_count=2;std::array<uintptr_t,2> family_views{reinterpret_cast<uintptr_t>(left_bytes.data()),reinterpret_cast<uintptr_t>(right_bytes.data())};};
void inspect_completed_renderer(Record& r,void*){r=Record{};}
struct Tracker {uint64_t generation=1;struct Ref {uintptr_t identity;uint64_t generation;};Ref current(uintptr_t id){return {id,generation};}} frame_link;
bool safe_copy(void* out,void const* in,size_t n){if(!in||!out)return false;std::memcpy(out,in,n);return true;}
uintptr_t canonical_identity(IUnknown* p){Microsoft::WRL::ComPtr<IUnknown> id;if(!p||FAILED(p->QueryInterface(IID_PPV_ARGS(&id))))return 0;return reinterpret_cast<uintptr_t>(id.Get());}
}
#include "ngx_epoch_pair_probe.inl"
using Microsoft::WRL::ComPtr;
static unsigned checks{};
static void check(bool value){++checks;if(!value)throw std::runtime_error("live epoch probe check failed");}
static void hr(HRESULT value){check(SUCCEEDED(value));}
template<class T> static void put(unsigned char* bytes,size_t offset,T const& value){std::memcpy(bytes+offset,&value,sizeof(value));}
int main(){try{
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
        {ngx_epoch_pair::CopyScope observed(list.Get(),&dst,0,0,0,&src,nullptr,base+0x1c6093);list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);observed.returned(list.Get(),&dst,&src,nullptr);}
        hr(list->Close());ID3D12CommandList* batch[]{list.Get()};auto* saved_renderer=consuming_renderer;consuming_renderer=nullptr;{ngx_epoch_pair::ExecuteScope observed(queue.Get(),1,batch);queue->ExecuteCommandLists(1,batch);observed.returned();}consuming_renderer=saved_renderer;
        // Independent test fence only; observer never submits a fence or waits.
        hr(queue->Signal(fence.Get(),eye+1));const auto end=GetTickCount64()+5000;while(fence->GetCompletedValue()<eye+1&&GetTickCount64()<end)Sleep(1);check(fence->GetCompletedValue()==eye+1);
        if(!eye){const auto before=ngx_epoch_pair::list(list.Get());hr(allocator->Reset());hr(list->Reset(allocator.Get(),nullptr));++frame_link.generation;ngx_epoch_pair::reset(before,list.Get(),S_OK);}
    }
    check(ngx_epoch_pair::probe.complete());recording=false;FILE* output{};check(tmpfile_s(&output)==0&&output);ngx_epoch_pair::print(output);rewind(output);
    std::string text;char buffer_line[2048];while(fgets(buffer_line,sizeof(buffer_line),output))text+=buffer_line;fclose(output);
    check(text.find("candidate_complete=1")!=std::string::npos&&text.find("producer_domain_verified=0")!=std::string::npos&&text.find("gpu_completion_verified=0")!=std::string::npos);
    check(text.find("stage=prepare_after")!=std::string::npos&&text.find("typed_before=f")!=std::string::npos&&text.find("format=41")!=std::string::npos);
    check(text.find("membership_frozen_at_execute_begin=1 earlier_history_complete=0")!=std::string::npos);
    check(!ngx_epoch_pair::probe.resources[0].lease.retained());
    recording=true;ngx_epoch_pair::reset_capture(true);ngx_epoch_pair::created({2,2,h,p,{1706,720,2560,1080,1,0x12,1}});ngx_epoch_pair::released(h);check(ngx_epoch_pair::probe.stop==2);
    ngx_epoch_pair::reset_capture(true);ngx_epoch_pair::created({3,2,h,p,{1706,720,2560,1080,1,0x12,1}});committed_world_generation=3;check(!ngx_epoch_pair::selected()&&ngx_epoch_pair::probe.stop==2);
    std::printf("NGX live epoch probe tests passed: %u checks (real D3D barriers/copies/Execute, synthetic SDK)\n",checks);return 0;
}catch(std::exception const& e){std::fprintf(stderr,"%s (%u)\n",e.what(),checks);return 1;}}
