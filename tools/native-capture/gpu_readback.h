#pragma once
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <atomic>
#include <vector>
#include <cstdio>
#include <cstring>

// One-shot diagnostic copy only. A creation-time queue binding is required.
// Setup/pixel conversion/disk I/O are worker-side. Present submits a single
// copy list without waiting for the GPU; source returns to PRESENT in the list.
class OneShotReadback {
    ID3D12CommandQueue* queue{};
    IDXGISwapChain3* chain{};
    ID3D12Device* device{};
    ID3D12Resource* buffer{};
    ID3D12Resource* source{};
    ID3D12CommandAllocator* allocator{};
    ID3D12GraphicsCommandList* list{};
    ID3D12Fence* fence{};
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
    UINT64 bytes{};
    bool submitted{};
public:
    std::atomic<unsigned> state{0}; // 0 absent, 1 bound, 2 ready, 3 recording, 4 mapped, 5 failed, 6 submitted
    unsigned width{},height{},format{},index{};
    HRESULT failure=S_OK;
    ULONGLONG bound_at{};
    SRWLOCK binding_lock=SRWLOCK_INIT;
    bool bind(ID3D12CommandQueue* q,IDXGISwapChain* swapchain){
        if(!TryAcquireSRWLockExclusive(&binding_lock))return false;
        bool ok=false;
        if(state==0){
            if(SUCCEEDED(swapchain->QueryInterface(IID_PPV_ARGS(&chain))) && SUCCEEDED(q->GetDevice(IID_PPV_ARGS(&device)))){
                queue=q;queue->AddRef();bound_at=GetTickCount64();state=1;ok=true;
            }else{if(chain){chain->Release();chain=nullptr;}if(device){device->Release();device=nullptr;}}
        }
        ReleaseSRWLockExclusive(&binding_lock);return ok;
    }
    bool prepare(){
        if(state!=1)return false;
        ID3D12Resource* sample{};
        failure=chain->GetBuffer(0,IID_PPV_ARGS(&sample));
        if(FAILED(failure)){state=5;return false;}
        auto desc=sample->GetDesc();sample->Release();
        width=static_cast<unsigned>(desc.Width);height=desc.Height;format=desc.Format;
        if(desc.Width>8192 || !width || !height || height>8192 || desc.SampleDesc.Count!=1 ||
           (format!=DXGI_FORMAT_R10G10B10A2_UNORM && format!=DXGI_FORMAT_R8G8B8A8_UNORM && format!=DXGI_FORMAT_B8G8R8A8_UNORM)){
            failure=E_INVALIDARG;state=5;return false;
        }
        device->GetCopyableFootprints(&desc,0,1,0,&footprint,nullptr,nullptr,&bytes);
        if(!bytes || bytes>256ull*1024*1024){failure=E_INVALIDARG;state=5;return false;}
        D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_READBACK;
        D3D12_RESOURCE_DESC read_desc{};read_desc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;read_desc.Width=bytes;read_desc.Height=1;
        read_desc.DepthOrArraySize=1;read_desc.MipLevels=1;read_desc.SampleDesc.Count=1;read_desc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        failure=device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&read_desc,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&buffer));
        if(SUCCEEDED(failure))failure=device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator));
        if(SUCCEEDED(failure))failure=device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator,nullptr,IID_PPV_ARGS(&list));
        if(SUCCEEDED(failure))failure=device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence));
        if(FAILED(failure)){state=5;return false;}
        state=2;return true;
    }
    void capture(IDXGISwapChain* swapchain,ULONGLONG delay_ms){
        if(state!=2 || GetTickCount64()-bound_at<delay_ms)return;
        IUnknown* a{};IUnknown* b{};
        bool same=SUCCEEDED(chain->QueryInterface(IID_PPV_ARGS(&a))) && SUCCEEDED(swapchain->QueryInterface(IID_PPV_ARGS(&b))) && a==b;
        if(a)a->Release();if(b)b->Release();if(!same)return;
        unsigned expected=2;if(!state.compare_exchange_strong(expected,3))return;
        index=chain->GetCurrentBackBufferIndex();failure=chain->GetBuffer(index,IID_PPV_ARGS(&source));
        if(FAILED(failure)){state=5;return;}
        auto desc=source->GetDesc();
        if(desc.Width!=width || desc.Height!=height || static_cast<unsigned>(desc.Format)!=format || desc.SampleDesc.Count!=1){failure=E_INVALIDARG;state=5;return;}
        D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;barrier.Transition.pResource=source;
        barrier.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore=D3D12_RESOURCE_STATE_PRESENT;barrier.Transition.StateAfter=D3D12_RESOURCE_STATE_COPY_SOURCE;
        list->ResourceBarrier(1,&barrier);
        D3D12_TEXTURE_COPY_LOCATION destination{};destination.pResource=buffer;destination.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;destination.PlacedFootprint=footprint;
        D3D12_TEXTURE_COPY_LOCATION origin{};origin.pResource=source;origin.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        list->CopyTextureRegion(&destination,0,0,0,&origin,nullptr);
        barrier.Transition.StateBefore=D3D12_RESOURCE_STATE_COPY_SOURCE;barrier.Transition.StateAfter=D3D12_RESOURCE_STATE_PRESENT;
        list->ResourceBarrier(1,&barrier);failure=list->Close();
        if(FAILED(failure)){state=5;return;}
        ID3D12CommandList* commands[]={list};submitted=true;queue->ExecuteCommandLists(1,commands);
        failure=queue->Signal(fence,1);state=FAILED(failure)?5:6;
    }
    bool pixels(std::vector<unsigned char>& rgb,unsigned out_width,unsigned out_height){
        if(state!=6 || !fence || fence->GetCompletedValue()<1)return false;
        if(fence->GetCompletedValue()==~UINT64(0)){failure=DXGI_ERROR_DEVICE_REMOVED;state=5;return false;}
        unsigned char* data{};D3D12_RANGE range{0,static_cast<SIZE_T>(bytes)};
        failure=buffer->Map(0,&range,reinterpret_cast<void**>(&data));if(FAILED(failure)){state=5;return false;}
        rgb.resize(static_cast<size_t>(out_width)*out_height*3);
        for(unsigned y=0;y<out_height;++y)for(unsigned x=0;x<out_width;++x){
            const unsigned char* pixel=data+footprint.Offset+static_cast<size_t>(y)*height/out_height*footprint.Footprint.RowPitch+static_cast<size_t>(x)*width/out_width*4;
            auto output=&rgb[(static_cast<size_t>(y)*out_width+x)*3];
            if(format==DXGI_FORMAT_R10G10B10A2_UNORM){unsigned packed{};memcpy(&packed,pixel,4);for(unsigned c=0;c<3;++c)output[c]=static_cast<unsigned char>((((packed>>(c*10))&1023)*255+511)/1023);}
            else if(format==DXGI_FORMAT_B8G8R8A8_UNORM){output[0]=pixel[2];output[1]=pixel[1];output[2]=pixel[0];}
            else{output[0]=pixel[0];output[1]=pixel[1];output[2]=pixel[2];}
        }
        D3D12_RANGE empty{};buffer->Unmap(0,&empty);
        // Do not hold a game back buffer after GPU completion; that would
        // interfere with a later ResizeBuffers even though the copy is done.
        if(source){source->Release();source=nullptr;}
        state=4;return true;
    }
    bool save(const wchar_t* path){
        unsigned w=width<640?width:640;unsigned h=static_cast<unsigned>(static_cast<UINT64>(height)*w/width);
        std::vector<unsigned char> rgb;if(!pixels(rgb,w,h))return false;
        FILE* file{};_wfopen_s(&file,path,L"wb");if(!file){failure=E_FAIL;state=5;return false;}
        fprintf(file,"P6\n%u %u\n255\n",w,h);bool ok=fwrite(rgb.data(),1,rgb.size(),file)==rgb.size();if(fclose(file))ok=false;
        if(!ok){failure=E_FAIL;state=5;}return ok;
    }
    bool release(){
        // A failed Signal may follow successful GPU submission. Retain all
        // objects in that case rather than free possibly in-flight resources.
        if(submitted && (!fence || FAILED(failure) || fence->GetCompletedValue()<1))return false;
        if(source)source->Release();if(buffer)buffer->Release();if(list)list->Release();if(allocator)allocator->Release();
        if(fence)fence->Release();if(queue)queue->Release();if(device)device->Release();if(chain)chain->Release();
        source=nullptr;buffer=nullptr;list=nullptr;allocator=nullptr;fence=nullptr;queue=nullptr;device=nullptr;chain=nullptr;
        submitted=false;state=0;return true;
    }
};
