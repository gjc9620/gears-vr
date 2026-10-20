#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "owned_queue_drain.h"
#include "owned_scene_readback.h"
#include "owned_scene_retirement.h"
#include <dxgi1_4.h>
#include <cstdio>
#include <stdexcept>
#include <string>
using Microsoft::WRL::ComPtr;
static unsigned checks;
static void require(bool value){++checks;if(!value)throw std::runtime_error("owned shutdown check failed");}
static void check(HRESULT hr){require(SUCCEEDED(hr));}
struct FaultOps:gears_owned_retirement::NativeQueueDrainOps {
    unsigned fault{};unsigned* closes{};
    HRESULT signal(){return fault==1?E_FAIL:NativeQueueDrainOps::signal();}
    UINT64 completed(){return fault==2?UINT64_MAX:NativeQueueDrainOps::completed();}
    void close(){++*closes;NativeQueueDrainOps::close();}
};
struct Owner {unsigned releases{};bool retire_on_rhi(gears_native_owned::DrainProof const&){++releases;return true;}
    bool retained()const{return releases==0;}};
int main(){try{
    ComPtr<IDXGIFactory4> factory;ComPtr<IDXGIAdapter> warp;ComPtr<ID3D12Device> device;
    check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));check(factory->EnumWarpAdapter(IID_PPV_ARGS(&warp)));
    check(D3D12CreateDevice(warp.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)));
    D3D12_COMMAND_QUEUE_DESC q{};q.Type=D3D12_COMMAND_LIST_TYPE_DIRECT;ComPtr<ID3D12CommandQueue> queue;
    check(device->CreateCommandQueue(&q,IID_PPV_ARGS(&queue)));
    D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;d.Width=16;d.Height=8;
    d.DepthOrArraySize=d.MipLevels=1;d.Format=DXGI_FORMAT_R8G8B8A8_UNORM;d.SampleDesc.Count=1;
    d.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;D3D12_HEAP_PROPERTIES h{};h.Type=D3D12_HEAP_TYPE_DEFAULT;
    ComPtr<ID3D12Resource> texture;check(device->CreateCommittedResource(&h,D3D12_HEAP_FLAG_NONE,&d,
        D3D12_RESOURCE_STATE_RENDER_TARGET,nullptr,IID_PPV_ARGS(&texture)));
    ComPtr<ID3D12Fence> blocked;check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&blocked)));
    // Deterministically keep the real GPU copy and both completion signals in
    // flight; close must keep all COM objects until the CPU opens this wait.
    check(queue->Wait(blocked.Get(),1));
    wchar_t temp[MAX_PATH]{};require(GetTempPathW(MAX_PATH,temp)>0);
    const auto path=std::wstring(temp)+L"gears_shutdown_"+std::to_wstring(GetCurrentProcessId())+L"_"+
        std::to_wstring(GetTickCount64())+L".ppm";
    owned_scene_readback::Probe probe;
    require(probe.queue(texture.Get(),queue.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,true,path.c_str()));
    gears_owned_retirement::QueueDrain<> drain;require(drain.begin(queue.Get()));
    require(!drain.poll()&&!drain.close()&&!drain.rearm()&&drain.retained());
    require(!probe.close()&&!probe.rearm()&&probe.retained()&&!probe.result().completed);
    gears_owned_retirement::Gate gate;Owner owner;
    require(gate.request({7,true,true,true,true,false}));
    require(!gate.gpu_drained(7,drain.poll(),true,false)&&!gate.retire_on_rhi(owner)&&!owner.releases);
    check(blocked->Signal(1));const auto deadline=GetTickCount64()+10000;
    while(!drain.poll()&&GetTickCount64()<deadline)Sleep(1);
    require(drain.poll());require(probe.close()&&!probe.retained()&&probe.result().completed&&probe.result().saved);
    require(drain.close()&&!drain.retained());
    require(!drain.begin(queue.Get())); // A closed fence cannot certify a second drain.
    require(gate.gpu_drained(7,true,true,true)&&gate.retire_on_rhi(owner)&&owner.releases==1);
    require(gate.retire_on_rhi(owner)&&owner.releases==1);require(DeleteFileW(path.c_str())!=0);
    // Reuse the completed components for a different rectangular allocation.
    // No in-flight/faulted instance may be reset to manufacture this boundary.
    require(drain.rearm()&&probe.rearm());
    d.Width=32;d.Height=9;ComPtr<ID3D12Resource> second_texture;
    check(device->CreateCommittedResource(&h,D3D12_HEAP_FLAG_NONE,&d,D3D12_RESOURCE_STATE_RENDER_TARGET,nullptr,IID_PPV_ARGS(&second_texture)));
    check(queue->Wait(blocked.Get(),2));const auto second_path=path+L".world2.ppm";
    require(probe.queue(second_texture.Get(),queue.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,true,second_path.c_str()));
    require(drain.begin(queue.Get())&&!drain.poll()&&!probe.close());
    check(blocked->Signal(2));const auto second_deadline=GetTickCount64()+10000;
    while(!drain.poll()&&GetTickCount64()<second_deadline)Sleep(1);
    require(drain.poll()&&probe.close()&&probe.result().width==32&&probe.result().height==9);
    require(drain.close()&&!drain.retained()&&!probe.retained());require(DeleteFileW(second_path.c_str())!=0);
    // Fault injection changes only one native operation, retaining real WARP
    // queue/fence COM leases. Neither failed Signal nor removed completion is
    // accepted as a drain receipt, even after all other CPU proofs are ready.
    for(unsigned fault:{1u,2u}){unsigned closes=0;FaultOps ops;ops.fault=fault;ops.closes=&closes;
        gears_owned_retirement::QueueDrain<FaultOps> failed(std::move(ops));
        const bool begun=failed.begin(queue.Get());require(begun==(fault==2));
        require(!failed.poll()&&!failed.close()&&!failed.rearm()&&failed.retained()&&!closes&&failed.state()==3);
        gears_owned_retirement::Gate denied;Owner held;require(denied.request({8,true,true,true,true,false}));
        require(!denied.gpu_drained(8,failed.poll(),true,true)&&!denied.retire_on_rhi(held)&&!held.releases);
    }
    gears_owned_retirement::QueueDrain<> invalid;require(!invalid.begin(nullptr)&&!invalid.close());
    std::printf("owned queue shutdown: %u checks passed; real WARP pending/copy retirement, failed Signal/removed fence deny release\n",checks);
    return 0;
}catch(std::exception const& e){std::fprintf(stderr,"FAIL: %s\n",e.what());return 1;}}
