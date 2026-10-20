#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <utility>

namespace gears_owned_retirement {
struct NativeQueueDrainOps {
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue;
    Microsoft::WRL::ComPtr<ID3D12Fence> fence;
    HRESULT initialize(ID3D12CommandQueue* input){
        queue=input;Microsoft::WRL::ComPtr<ID3D12Device> device;
        HRESULT hr=queue->GetDevice(IID_PPV_ARGS(&device));
        return FAILED(hr)?hr:device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence));
    }
    HRESULT signal(){return queue->Signal(fence.Get(),1);}
    UINT64 completed(){return fence->GetCompletedValue();}
    void close(){fence.Reset();queue.Reset();}
    void retain(){fence.Detach();queue.Detach();}
    bool retained()const{return queue||fence;}
};
// Signal only after CPU borrowers have retired and future owned submissions
// are excluded. This fence then covers the original scene and every diagnostic
// copy on the same observed DIRECT queue, rather than publisher copies alone.
template<class Ops=NativeQueueDrainOps> class QueueDrain {
    Ops ops_;
    ID3D12CommandQueue* input_{};
    unsigned state_{};HRESULT error_{S_OK};
public:
    explicit QueueDrain(Ops ops={}):ops_(std::move(ops)){}
    ~QueueDrain(){if(state_==1||state_==3)ops_.retain();}
    QueueDrain(QueueDrain const&)=delete;QueueDrain& operator=(QueueDrain const&)=delete;
    bool begin(ID3D12CommandQueue* queue){
        if(state_)return (state_==1||state_==2)&&queue==input_&&ops_.retained();
        if(!queue||queue->GetDesc().Type!=D3D12_COMMAND_LIST_TYPE_DIRECT){state_=3;error_=E_INVALIDARG;return false;}
        input_=queue;HRESULT hr=ops_.initialize(queue);
        if(SUCCEEDED(hr))hr=ops_.signal();
        if(FAILED(hr)){state_=3;error_=hr;return false;}
        state_=1;return true;
    }
    bool poll(){
        if(state_!=1)return state_==2;
        const auto completed=ops_.completed();
        if(completed==UINT64_MAX){state_=3;error_=DXGI_ERROR_DEVICE_REMOVED;return false;}
        if(completed<1)return false;state_=2;return true;
    }
    bool close(){if(state_!=2)return false;ops_.close();return true;}
    unsigned state()const{return state_;}
    HRESULT error()const{return error_;}
    bool retained()const{return ops_.retained();}
    bool rearm(){
        if(state_!=2||ops_.retained())return false;
        input_=nullptr;state_=0;error_=S_OK;return true;
    }
};
}
