#include "ngx_stereo_gpu_submit.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <vector>
using namespace gears_ngx_stereo;
using Microsoft::WRL::ComPtr;
static unsigned checks{},executes{};
static void check(bool ok){++checks;if(!ok){std::fprintf(stderr,"check %u failed\n",checks);throw std::runtime_error("NGX GPU submission check failed");}}
static void hr(HRESULT result){check(SUCCEEDED(result));}
static uintptr_t identity(IUnknown* object){ComPtr<IUnknown> id;hr(object->QueryInterface(IID_PPV_ARGS(&id)));return reinterpret_cast<uintptr_t>(id.Get());}
static uint64_t current_generation=7;
static uint64_t generation(ID3D12CommandList*,uintptr_t){return current_generation;}
static bool read(void* out,void const* in,size_t size){if(!out||!in)return false;std::memcpy(out,in,size);return true;}
static void STDMETHODCALLTYPE execute(ID3D12CommandQueue* queue,UINT count,ID3D12CommandList* const* lists){++executes;queue->ExecuteCommandLists(count,lists);}
struct Backend final:AdapterBackend {
    unsigned creates{};
    NativeResult create(uint64_t,uint32_t,uint64_t,uint64_t* handle)override{*handle=100+creates++;return 1;}
    NativeResult evaluate(uint64_t,uint64_t,uint64_t,uint64_t)override{return 1;}
    NativeResult release(uint64_t)override{return 1;}
    void copy(CopyPlan const&)override{}
};
static Layout layout(){Layout l;for(auto& e:l.physical)e={128,64};l.full_output={128,64};l.input=l.eye_output={64,64};l.stride=64;l.input_base[1]={64,0};return l;}
static void wait(ID3D12Fence* fence,uint64_t value){const auto end=GetTickCount64()+5000;
    while(fence->GetCompletedValue()<value&&GetTickCount64()<end)Sleep(1);check(fence->GetCompletedValue()==value);}
static void stereo_submissions(ID3D12Device* device,ID3D12CommandQueue* queue,bool shared_copy){
    current_generation=11;ComPtr<ID3D12CommandAllocator> allocator;hr(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)));
    std::array<ComPtr<ID3D12GraphicsCommandList>,5> lists;std::array<uintptr_t,5> ids{};
    for(unsigned i=0;i<5;++i){hr(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&lists[i])));hr(lists[i]->Close());ids[i]=identity(lists[i].Get());}
    const auto did=identity(device);Identity id{1,2,did,4,5};Backend backend;
    StereoAdapter adapter(backend,{true,true,true,true,true,true,true,true,true,true,true,true});
    {auto entry=adapter.enter();check(adapter.begin_epoch(entry,id,layout()));check(adapter.create(entry,{reinterpret_cast<uintptr_t>(lists[0].Get()),ids[0],11,did,true},1,4).admitted);}
    for(unsigned eye=0;eye<2;++eye){const auto index=1+2*eye;auto entry=adapter.enter();ProducerObservation p;
        p.use={id,10,ids[index],11,eye};p.view=1000+eye;
        for(unsigned r=0;r<4;++r)p.resources[r]={2000+r,did,{128,64},26,0,1,0};
        for(auto& rect:p.rendered)rect={64*eye,0,64,64};check(adapter.observe_producer(entry,p));
        bool reset{};Command command{reinterpret_cast<uintptr_t>(lists[index].Get()),ids[index],11,did,true};
        check(adapter.prepare_evaluate(entry,p.use,command,100,4,false,reset));check(adapter.evaluate(entry,command.raw,100,4,0).admitted);
        CopyFacts f;f.identity=id;f.ticket=10;f.caller=0x1c6093;f.list=ids[index+(shared_copy?0:1)];f.generation=11;f.list_device=did;
        f.direct=f.copy_source=f.copy_dest=true;f.resources=p.resources;f.input_base=layout().input_base;f.feature_output=layout().eye_output;f.evaluated_ordinal=eye;
        check(adapter.copy(entry,f,eye));
    }
    {auto entry=adapter.enter();std::array<PendingRecording,5> pending;check(adapter.pending_recordings(entry,pending)==5);}
    NativeGpuSubmission gpu;
    for(unsigned i=0;i<5;++i)if(!shared_copy||i==0||i==1||i==3)check(gpu.retain_recording(adapter,lists[i].Get(),ids[i],11));
    ID3D12CommandList* first[]{lists[0].Get(),lists[1].Get()};
    check(gpu.execute(adapter,queue,2,first,execute,read,generation));
    {auto entry=adapter.enter();std::array<PendingRecording,5> pending;check(adapter.pending_recordings(entry,pending)==(shared_copy?2u:3u));}
    ID3D12CommandList* second[]{lists[2].Get(),lists[3].Get(),lists[4].Get()};ID3D12CommandList* shared[]{lists[3].Get()};
    check(gpu.execute(adapter,queue,shared_copy?1:3,shared_copy?shared:second,execute,read,generation));
    ComPtr<ID3D12Fence> end;hr(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&end)));hr(queue->Signal(end.Get(),1));wait(end.Get(),1);
    {auto entry=adapter.enter();std::array<PendingRecording,5> pending;check(adapter.pending_recordings(entry,pending)==0);adapter.invalidate(entry);
        auto complete=gpu.completion();check(complete.value==2);NativeResult result{};
        check(adapter.release_completed(entry,100,complete,result)&&adapter.release_completed(entry,101,complete,result)&&adapter.drained(entry));}
}

static AdapterPrerequisites prerequisites(){return {true,true,true,true,true,true,true,true,true,true,true,true};}
static void ordinary_lists(ID3D12Device* device,ID3D12CommandQueue* queue){
    Backend backend;StereoAdapter adapter(backend,prerequisites());NativeGpuSubmission gpu;
    ComPtr<ID3D12CommandAllocator> allocator;hr(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)));
    // Keep all 600 alive simultaneously, proving 600 distinct actual identities.
    std::vector<ComPtr<ID3D12GraphicsCommandList>> lists(600);std::vector<ID3D12CommandList*> raw;
    for(auto& list:lists){hr(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)));hr(list->Close());raw.push_back(list.Get());}
    for(auto* list:raw)check(gpu.execute(adapter,queue,1,&list,execute,read,generation));
    // A large unrelated batch must not acquire a NGX slot or bounded-array limit.
    check(gpu.execute(adapter,queue,static_cast<UINT>(raw.size()),raw.data(),execute,read,generation));
    check(!gpu.failed()&&!gpu.retained_recordings()&&!gpu.submitted_generations()&&!gpu.inflight_batches()&&!gpu.completion().fence);
    ComPtr<ID3D12Fence> end;hr(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&end)));hr(queue->Signal(end.Get(),1));wait(end.Get(),1);
}
static void reset_lifetime(ID3D12Device* device,ID3D12CommandQueue* queue){
    current_generation=30;const auto did=identity(device);Identity id{1,2,did,4,5};Backend backend;StereoAdapter adapter(backend,prerequisites());NativeGpuSubmission gpu;
    ComPtr<ID3D12CommandAllocator> first,second;hr(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&first)));hr(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&second)));
    ComPtr<ID3D12GraphicsCommandList> list;hr(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,first.Get(),nullptr,IID_PPV_ARGS(&list)));
    const auto lid=identity(list.Get());check(gpu.retain_recording(adapter,list.Get(),lid,current_generation));
    {auto entry=adapter.enter();check(adapter.begin_epoch(entry,id,layout()));check(adapter.create(entry,{reinterpret_cast<uintptr_t>(list.Get()),lid,current_generation,did,true},1,4).admitted);}
    hr(list->Close());ComPtr<ID3D12Fence> gate,end;hr(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&gate)));hr(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&end)));
    hr(queue->Wait(gate.Get(),1));ID3D12CommandList* batch[]{list.Get()};check(gpu.execute(adapter,queue,1,batch,execute,read,generation));
    check(!gpu.retained_recordings()&&gpu.submitted_generations()==1&&gpu.inflight_batches()==1&&gpu.completion().value==0);
    // Real Reset with a different allocator is legal while the prior execution
    // is GPU-pending. It authorizes a fresh generation, not early retirement.
    hr(list->Reset(second.Get(),nullptr));++current_generation;check(gpu.reset_success(adapter,list.Get(),lid,current_generation));
    check(gpu.submitted_generations()==1&&gpu.inflight_batches()==1);
    check(gpu.retain_recording(adapter,list.Get(),lid,current_generation));
    {auto entry=adapter.enter();ProducerObservation p;p.use={id,10,lid,current_generation,0};p.view=1000;
        for(unsigned r=0;r<4;++r)p.resources[r]={2000+r,did,{128,64},26,0,1,0};for(auto& rect:p.rendered)rect={0,0,64,64};
        check(adapter.observe_producer(entry,p));bool reset{};Command command{reinterpret_cast<uintptr_t>(list.Get()),lid,current_generation,did,true};
        check(adapter.prepare_evaluate(entry,p.use,command,100,4,false,reset));check(adapter.evaluate(entry,command.raw,100,4,0).admitted);}
    hr(list->Close());check(gpu.execute(adapter,queue,1,batch,execute,read,generation));
    check(gpu.submitted_generations()==2&&gpu.inflight_batches()==2&&gpu.completion().value==0);
    hr(gate->Signal(1));hr(queue->Signal(end.Get(),1));wait(end.Get(),1);
    auto completion=gpu.completion();check(completion.value==2&&gpu.submitted_generations()==1&&!gpu.inflight_batches());
    {auto entry=adapter.enter();adapter.invalidate(entry);NativeResult result{};check(adapter.release_completed(entry,100,completion,result)&&adapter.release_completed(entry,101,completion,result)&&adapter.drained(entry));}
    // Completion of the newer generation did not erase its duplicate guard.
    hr(list->Reset(first.Get(),nullptr));++current_generation;check(gpu.reset_success(adapter,list.Get(),lid,current_generation));check(!gpu.submitted_generations());hr(list->Close());
    // More than the entire Seen capacity of real NGX generations can pass with
    // real Reset + completed fences, without overwriting any live generation.
    for(unsigned cycle=0;cycle<140;++cycle){
        hr(list->Reset(first.Get(),nullptr));++current_generation;check(gpu.reset_success(adapter,list.Get(),lid,current_generation));
        check(!gpu.submitted_generations());check(gpu.retain_recording(adapter,list.Get(),lid,current_generation));
        const auto handle=100+backend.creates;
        {auto entry=adapter.enter();++id.epoch;check(adapter.begin_epoch(entry,id,layout()));check(adapter.create(entry,{reinterpret_cast<uintptr_t>(list.Get()),lid,current_generation,did,true},1,4).admitted);}
        hr(list->Close());check(gpu.execute(adapter,queue,1,batch,execute,read,generation));hr(queue->Signal(end.Get(),cycle+2));wait(end.Get(),cycle+2);
        completion=gpu.completion();check(completion.value==cycle+3&&gpu.submitted_generations()==1&&!gpu.inflight_batches());
        {auto entry=adapter.enter();adapter.invalidate(entry);NativeResult result{};check(adapter.release_completed(entry,handle,completion,result)&&adapter.release_completed(entry,handle+1,completion,result)&&adapter.drained(entry));}
    }
    // Changing the numerical tracker generation alone must not erase Seen.
    // Even after actual Reset, omitting its hook notification stays rejected.
    hr(list->Reset(first.Get(),nullptr));++current_generation;
    check(!gpu.retain_recording(adapter,list.Get(),lid,current_generation));
    check(gpu.failed()&&gpu.submitted_generations()==1&&!gpu.completion().fence);hr(list->Close());
}
static void unsubmitted_reset(ID3D12Device* device){
    const auto did=identity(device);Backend backend;StereoAdapter adapter(backend,prerequisites());NativeGpuSubmission gpu;
    ComPtr<ID3D12CommandAllocator> allocator;hr(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)));
    ComPtr<ID3D12GraphicsCommandList> list;hr(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)));const auto lid=identity(list.Get());
    check(gpu.retain_recording(adapter,list.Get(),lid,77));
    {auto entry=adapter.enter();check(adapter.begin_epoch(entry,{1,2,did,4,5},layout()));check(adapter.create(entry,{reinterpret_cast<uintptr_t>(list.Get()),lid,77,did,true},1,4).admitted);}
    hr(list->Close());hr(list->Reset(allocator.Get(),nullptr));check(!gpu.reset_success(adapter,list.Get(),lid,78));
    check(gpu.failed()&&gpu.retained_recordings()==1&&!gpu.completion().fence);hr(list->Close());
    {auto entry=adapter.enter();std::array<PendingRecording,5> pending;check(adapter.pending_recordings(entry,pending)==1&&!adapter.drained(entry));}
}
int main(){try{
    ComPtr<ID3D12Device> device;hr(D3D12CreateDevice(nullptr,D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)));
    ComPtr<ID3D12CommandQueue> queue;D3D12_COMMAND_QUEUE_DESC desc{};hr(device->CreateCommandQueue(&desc,IID_PPV_ARGS(&queue)));
    ComPtr<ID3D12CommandAllocator> allocator;hr(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)));
    ComPtr<ID3D12GraphicsCommandList> list;hr(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)));hr(list->Close());
    ComPtr<ID3D12Fence> gate,end;hr(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&gate)));hr(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&end)));
    Backend backend;StereoAdapter adapter(backend,{true,true,true,true,true,true,true,true,true,true,true,true});
    const auto did=identity(device.Get()),lid=identity(list.Get());Identity id{1,2,did,4,5};
    {auto entry=adapter.enter();check(adapter.begin_epoch(entry,id,layout()));auto result=adapter.create(entry,{reinterpret_cast<uintptr_t>(list.Get()),lid,7,did,true},1,4);check(result.admitted);}
    NativeGpuSubmission gpu;check(gpu.retain_recording(adapter,list.Get(),lid,7));ID3D12CommandList* batch[]{list.Get()};
    // Deterministic real GPU pending interval. The helper never queues Wait;
    // this independent test gate prevents its actual Signal completing early.
    hr(queue->Wait(gate.Get(),1));check(gpu.execute(adapter,queue.Get(),1,batch,execute,read,generation));
    const auto pending=gpu.completion();check(executes==1&&pending.device==did&&pending.queue==identity(queue.Get())&&pending.fence&&pending.value==0);
    {auto entry=adapter.enter();adapter.invalidate(entry);NativeResult result{};check(!adapter.release_completed(entry,100,pending,result));std::array<PendingRecording,5> records;check(adapter.pending_recordings(entry,records)==0);}
    hr(gate->Signal(1));hr(queue->Signal(end.Get(),1));wait(end.Get(),1);
    const auto complete=gpu.completion();check(complete.value==1&&complete.fence==pending.fence);
    {auto entry=adapter.enter();NativeResult result{};check(adapter.release_completed(entry,100,complete,result)&&adapter.release_completed(entry,101,complete,result)&&adapter.drained(entry));}
    // A duplicate Execute is still forwarded, but cannot get a replacement
    // receipt or permit further release using the earlier completed fence.
    check(!gpu.execute(adapter,queue.Get(),1,batch,execute,read,generation));check(executes==2&&gpu.failed()&&!gpu.completion().fence);
    hr(queue->Signal(end.Get(),2));wait(end.Get(),2);
    // A stale Reset generation cannot match the adapter's real recording.
    Backend b2;StereoAdapter a2(b2,{true,true,true,true,true,true,true,true,true,true,true,true});
    {auto entry=a2.enter();check(a2.begin_epoch(entry,id,layout()));check(a2.create(entry,{reinterpret_cast<uintptr_t>(list.Get()),lid,7,did,true},1,4).admitted);}
    NativeGpuSubmission wrong;check(wrong.retain_recording(a2,list.Get(),lid,7));current_generation=8;check(!wrong.execute(a2,queue.Get(),1,batch,execute,read,generation)&&wrong.failed());
    check(executes==3&&!wrong.completion().fence);hr(queue->Signal(end.Get(),3));wait(end.Get(),3);
    stereo_submissions(device.Get(),queue.Get(),false);stereo_submissions(device.Get(),queue.Get(),true);
    ordinary_lists(device.Get(),queue.Get());reset_lifetime(device.Get(),queue.Get());unsubmitted_reset(device.Get());
    std::printf("NGX real queue submission tests passed: %u checks (SDK calls synthetic)\n",checks);return 0;
}catch(std::exception const& e){std::fprintf(stderr,"%s\n",e.what());return 1;}}
