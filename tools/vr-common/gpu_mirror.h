#pragma once
#include "gpu_share.h"
#include "pose_bridge.h"
#include "mirror_geometry.h"
#include <d3dcompiler.h>
#include <atomic>
#include <chrono>
#include <algorithm>
#ifndef GEARS_DESKTOP_MIRROR_ASYNC
#define GEARS_DESKTOP_MIRROR_ASYNC 0
#endif

namespace gears_vr {
// Caller certifies source state, scene/UI submission, current backbuffer state,
// swapchain queue identity and API order. This primitive never selects a target.
// Wrap its API submissions in the capture integration's observer bypass.
class DesktopMirror {
public:
    static constexpr bool asynchronous=GEARS_DESKTOP_MIRROR_ASYNC!=0;
    static constexpr unsigned draw_slot_count=asynchronous?3:1;
    enum class Reason : unsigned { none,not_ready,lock_busy,copy_busy,source,generation,ticket,stale,queue,device,destination,api,timeout,closed,draw_busy };
    struct Status {
        unsigned state{},destination_format{}; // 0 empty, 1 bound, 2 ready, 3 failed, 4 closed, 5 retiring.
        uint64_t captures{},presents{},busy{},stale{},rejected{},wait_ns{},max_wait_ns{},timeouts{},ticket{};
        int64_t received_ns{};HRESULT error{S_OK};Reason reason{Reason::none};bool unsafe{},destination_held{};
        unsigned pending_draws{};
    };
#if defined(GEARS_DESKTOP_MIRROR_READBACK) && GEARS_DESKTOP_MIRROR_READBACK
    // Private acceptance evidence. The completed buffer is independent of the
    // swapchain/source lifetime; the worker owns mapping, conversion and I/O.
    enum class ReadbackState : unsigned { idle,armed,recorded,submitted,ready,taken,failed,unsafe };
    struct Readback {
        ComPtr<ID3D12Resource> buffer;
        D3D12_RESOURCE_DESC source{},destination{};
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
        MirrorRectangle rectangle{};
        uint64_t bytes{},generation{},ticket{},draw_serial{};
        int64_t received_ns{},present_ns{};
    };
    struct ReadbackStatus {ReadbackState state{};HRESULT error{S_OK};bool held{},signaled{};uint64_t draw_serial{};};
#endif
private:
    SRWLOCK lock_=SRWLOCK_INIT;
    std::atomic<uint64_t> lock_busy_{};
    Status stats_{};
    ComPtr<ID3D12CommandQueue> queue_;
    ComPtr<IUnknown> queue_identity_,source_identity_,device_identity_;
    ComPtr<ID3D12Device> device_;
    ComPtr<ID3D12Resource> snapshot_;
    D3D12_RESOURCE_DESC source_desc_{};uint64_t generation_{};
    ComPtr<ID3D12CommandAllocator> copy_allocator_;
    ComPtr<ID3D12GraphicsCommandList> copy_list_;
    ComPtr<ID3D12Fence> copy_fence_;
    struct DrawSlot {
        ComPtr<ID3D12CommandAllocator> allocator;
        ComPtr<ID3D12GraphicsCommandList> list;
        ComPtr<ID3D12DescriptorHeap> rtv;
        ComPtr<ID3D12Fence> fence;
        ComPtr<ID3D12Resource> destination;
        uint64_t serial{};bool inflight{},signaled{};
    };
    std::array<DrawSlot,draw_slot_count> draws_;
    uint64_t copy_serial_{},draw_serial_{};
    bool copy_inflight_{},copy_signaled_{},snapshot_valid_{};
    HANDLE event_{};
    ComPtr<ID3D12RootSignature> root_;
    std::array<ComPtr<ID3D12PipelineState>,5> pipelines_;
    ComPtr<ID3D12DescriptorHeap> srv_;
#if defined(GEARS_DESKTOP_MIRROR_READBACK) && GEARS_DESKTOP_MIRROR_READBACK
    Readback readback_;
    ReadbackState readback_state_{ReadbackState::idle};
    HRESULT readback_error_{S_OK};
    ComPtr<ID3D12Fence> readback_fence_;
    bool readback_signaled_{};
    void refresh_readback(){
        if(readback_state_!=ReadbackState::submitted||!readback_signaled_)return;
        const auto completed=readback_fence_->GetCompletedValue();
        if(completed==UINT64_MAX){readback_error_=DXGI_ERROR_DEVICE_REMOVED;return;}
        if(completed>=readback_.draw_serial){readback_state_=ReadbackState::ready;readback_fence_.Reset();}
    }
    bool prepare_readback(D3D12_RESOURCE_DESC const& destination){
        if(readback_state_!=ReadbackState::armed)return false;
        // Allocation is one-shot and bounded. Failure never suppresses a draw.
        readback_state_=ReadbackState::failed;
        UINT rows{};UINT64 row_bytes{};
        device_->GetCopyableFootprints(&destination,0,1,0,&readback_.footprint,&rows,&row_bytes,&readback_.bytes);
        if(!readback_.bytes||readback_.bytes>256ull*1024*1024||rows!=destination.Height||
           row_bytes!=destination.Width*4||readback_.footprint.Footprint.RowPitch<row_bytes){readback_error_=E_INVALIDARG;return false;}
        D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_READBACK;
        D3D12_RESOURCE_DESC buffer{};buffer.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;buffer.Width=readback_.bytes;
        buffer.Height=1;buffer.DepthOrArraySize=1;buffer.MipLevels=1;buffer.SampleDesc.Count=1;buffer.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        readback_error_=device_->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&readback_.buffer));
        return SUCCEEDED(readback_error_);
    }
#endif
    struct Unlock {SRWLOCK* lock;~Unlock(){ReleaseSRWLockExclusive(lock);}};
    static int64_t clock_ns(){return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();}
    static int format_index(DXGI_FORMAT f){
        switch(f){case DXGI_FORMAT_R8G8B8A8_UNORM:return 0;case DXGI_FORMAT_B8G8R8A8_UNORM:return 1;
            case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:return 2;case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:return 3;default:return -1;}
    }
    static int destination_index(DXGI_FORMAT f){return f==DXGI_FORMAT_R10G10B10A2_UNORM?4:format_index(f);}
    static bool same_object(IUnknown* object,IUnknown* identity){ComPtr<IUnknown> actual;
        return object&&SUCCEEDED(object->QueryInterface(IID_PPV_ARGS(&actual)))&&actual.Get()==identity;}
    bool reject(Reason r){++stats_.rejected;stats_.reason=r;return false;}
    bool failed(HRESULT error){stats_.state=3;stats_.error=error;stats_.reason=Reason::api;return false;}
    bool fresh(int64_t received,int64_t now){
        if(received<=0||now<received||now-received>100000000){++stats_.stale;return reject(Reason::stale);}return true;
    }
    static void transition(ID3D12GraphicsCommandList* list,ID3D12Resource* r,D3D12_RESOURCE_STATES before,D3D12_RESOURCE_STATES after){
        if(before!=after)gpu_barrier(list,r,before,after);
    }
    bool complete(ComPtr<ID3D12Fence> const& fence,uint64_t serial,bool signaled,bool& inflight){
        if(!inflight)return true;if(!signaled)return false;
        const auto done=fence->GetCompletedValue();if(done==UINT64_MAX){failed(DXGI_ERROR_DEVICE_REMOVED);return false;}
        if(done>=serial){inflight=false;return true;}return false;
    }
    bool wait(ComPtr<ID3D12Fence> const& fence,uint64_t serial,bool signaled,bool& inflight,uint64_t deadline){
        if(complete(fence,serial,signaled,inflight))return true;
        if(!signaled)return false;
        auto hr=fence->SetEventOnCompletion(serial,event_);if(FAILED(hr))return failed(hr);
        const auto now=GetTickCount64();const DWORD milliseconds=static_cast<DWORD>(deadline>now?deadline-now:0);
        const auto start=clock_ns();const auto result=WaitForSingleObject(event_,milliseconds);
        const auto elapsed=static_cast<uint64_t>((std::max)(int64_t(0),clock_ns()-start));
        stats_.wait_ns+=elapsed;stats_.max_wait_ns=(std::max)(stats_.max_wait_ns,elapsed);
        if(result!=WAIT_OBJECT_0){if(result==WAIT_TIMEOUT){++stats_.timeouts;stats_.reason=Reason::timeout;stats_.error=HRESULT_FROM_WIN32(WAIT_TIMEOUT);stats_.state=3;return false;}
            return failed(HRESULT_FROM_WIN32(GetLastError()));}
        return complete(fence,serial,signaled,inflight);
    }
    bool drain_locked(unsigned timeout_ms){
        const auto deadline=GetTickCount64()+(std::min)(timeout_ms,1000u);
        bool safe=wait(copy_fence_,copy_serial_,copy_signaled_,copy_inflight_,deadline);
        for(auto& draw:draws_){
            if(wait(draw.fence,draw.serial,draw.signaled,draw.inflight,deadline))draw.destination.Reset();
            else safe=false;
        }
        return safe;
    }
    bool poll_draws(){
        bool done=true;
        for(auto& draw:draws_){
            if(complete(draw.fence,draw.serial,draw.signaled,draw.inflight))draw.destination.Reset();
            else done=false;
        }
        return done;
    }
    void release(bool unsafe){
#if defined(GEARS_DESKTOP_MIRROR_READBACK) && GEARS_DESKTOP_MIRROR_READBACK
        refresh_readback();
        if(readback_state_==ReadbackState::submitted){
            // An unconfirmed Execute/Signal must never free its GPU storage.
            readback_.buffer.Detach();readback_fence_.Detach();readback_state_=ReadbackState::unsafe;
        }
        // Ready evidence deliberately survives source retirement and close.
#endif
        auto clear=[&](auto& p){if(unsafe)p.Detach();else p.Reset();};
        clear(snapshot_);clear(copy_list_);clear(copy_allocator_);clear(copy_fence_);
        for(auto& draw:draws_){clear(draw.destination);clear(draw.list);clear(draw.allocator);clear(draw.fence);clear(draw.rtv);}
        clear(root_);for(auto& p:pipelines_)clear(p);clear(srv_);
        clear(source_identity_);clear(queue_identity_);clear(device_identity_);clear(queue_);clear(device_);
        if(event_&&!unsafe)CloseHandle(event_);event_=nullptr;
    }
    void create_pipeline(){
        const char* shader=R"(
Texture2D<float4> image:register(t0);SamplerState linearClamp:register(s0);
cbuffer Size:register(b0){uint2 sourceSize;};
struct V {float4 position:SV_Position;float2 uv:TEXCOORD0;};
V vs(uint id:SV_VertexID){V v;float2 p=float2((id<<1)&2,id&2);v.position=float4(p.x*2-1,1-p.y*2,0,1);v.uv=p;return v;}
float4 ps(V v):SV_Target {
 float2 uv=float2(clamp(v.uv.x*.5,.5/sourceSize.x,.5-.5/sourceSize.x),clamp(v.uv.y,.5/sourceSize.y,1-.5/sourceSize.y));
 float3 c=image.Sample(linearClamp,uv).rgb;
#ifdef SRGB_TARGET
 c=float3(c.r<=.04045?c.r/12.92:pow((c.r+.055)/1.055,2.4),c.g<=.04045?c.g/12.92:pow((c.g+.055)/1.055,2.4),c.b<=.04045?c.b/12.92:pow((c.b+.055)/1.055,2.4));
#endif
 return float4(c,1);
})";
        ComPtr<ID3DBlob> vs,ps[2],errors,serialized;
        gpu_check(D3DCompile(shader,strlen(shader),nullptr,nullptr,nullptr,"vs","vs_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&vs,&errors),"mirror VS");
        D3D_SHADER_MACRO srgb[]={{"SRGB_TARGET","1"},{nullptr,nullptr}};
        for(unsigned i=0;i<2;++i){errors.Reset();gpu_check(D3DCompile(shader,strlen(shader),nullptr,i?srgb:nullptr,nullptr,"ps","ps_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&ps[i],&errors),"mirror PS");}
        D3D12_DESCRIPTOR_RANGE range{};range.RangeType=D3D12_DESCRIPTOR_RANGE_TYPE_SRV;range.NumDescriptors=1;
        D3D12_ROOT_PARAMETER parameters[2]{};parameters[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        parameters[0].DescriptorTable.NumDescriptorRanges=1;parameters[0].DescriptorTable.pDescriptorRanges=&range;parameters[0].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
        parameters[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;parameters[1].Constants.Num32BitValues=2;parameters[1].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
        D3D12_STATIC_SAMPLER_DESC sampler{};sampler.Filter=D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        sampler.AddressU=sampler.AddressV=sampler.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;sampler.MaxAnisotropy=1;sampler.ComparisonFunc=D3D12_COMPARISON_FUNC_ALWAYS;sampler.MaxLOD=D3D12_FLOAT32_MAX;sampler.ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
        D3D12_ROOT_SIGNATURE_DESC signature{};signature.NumParameters=2;signature.pParameters=parameters;signature.NumStaticSamplers=1;signature.pStaticSamplers=&sampler;
        gpu_check(D3D12SerializeRootSignature(&signature,D3D_ROOT_SIGNATURE_VERSION_1,&serialized,&errors),"mirror root blob");
        gpu_check(device_->CreateRootSignature(0,serialized->GetBufferPointer(),serialized->GetBufferSize(),IID_PPV_ARGS(&root_)),"mirror root");
        const DXGI_FORMAT formats[]={DXGI_FORMAT_R8G8B8A8_UNORM,DXGI_FORMAT_B8G8R8A8_UNORM,DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,DXGI_FORMAT_R10G10B10A2_UNORM};
        for(unsigned i=0;i<5;++i){D3D12_GRAPHICS_PIPELINE_STATE_DESC d{};d.pRootSignature=root_.Get();d.VS={vs->GetBufferPointer(),vs->GetBufferSize()};const unsigned transfer=(i==2||i==3)?1:0;d.PS={ps[transfer]->GetBufferPointer(),ps[transfer]->GetBufferSize()};
            auto& blend=d.BlendState.RenderTarget[0];blend.SrcBlend=blend.SrcBlendAlpha=D3D12_BLEND_ONE;blend.DestBlend=blend.DestBlendAlpha=D3D12_BLEND_ZERO;blend.BlendOp=blend.BlendOpAlpha=D3D12_BLEND_OP_ADD;blend.LogicOp=D3D12_LOGIC_OP_NOOP;blend.RenderTargetWriteMask=D3D12_COLOR_WRITE_ENABLE_ALL;
            d.SampleMask=UINT_MAX;d.RasterizerState.FillMode=D3D12_FILL_MODE_SOLID;d.RasterizerState.CullMode=D3D12_CULL_MODE_NONE;d.RasterizerState.DepthClipEnable=TRUE;
            d.DepthStencilState.DepthWriteMask=D3D12_DEPTH_WRITE_MASK_ZERO;d.DepthStencilState.DepthFunc=D3D12_COMPARISON_FUNC_ALWAYS;
            d.PrimitiveTopologyType=D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;d.NumRenderTargets=1;d.RTVFormats[0]=formats[i];d.SampleDesc.Count=1;
            gpu_check(device_->CreateGraphicsPipelineState(&d,IID_PPV_ARGS(&pipelines_[i])),"mirror pipeline");}
    }
public:
    DesktopMirror()=default;DesktopMirror(DesktopMirror const&)=delete;
    ~DesktopMirror(){close(0);}
    Status status(){AcquireSRWLockExclusive(&lock_);Unlock unlock{&lock_};auto result=stats_;result.busy+=lock_busy_.load();
        for(auto const& draw:draws_){result.destination_held|=draw.destination!=nullptr;result.pending_draws+=draw.inflight?1u:0u;}return result;}
#if defined(GEARS_DESKTOP_MIRROR_READBACK) && GEARS_DESKTOP_MIRROR_READBACK
    bool arm_readback(){
        AcquireSRWLockExclusive(&lock_);Unlock unlock{&lock_};
        if(readback_state_!=ReadbackState::idle||stats_.state==3||stats_.state==4||stats_.state==5)return false;
        readback_state_=ReadbackState::armed;return true;
    }
    ReadbackStatus readback_status(){
        AcquireSRWLockExclusive(&lock_);Unlock unlock{&lock_};refresh_readback();
        return {readback_state_,readback_error_,readback_.buffer!=nullptr,readback_signaled_,readback_.draw_serial};
    }
    bool take_readback(Readback& result){
        if(!TryAcquireSRWLockExclusive(&lock_))return false;Unlock unlock{&lock_};refresh_readback();
        if(readback_state_!=ReadbackState::ready||result.buffer)return false;
        result=std::move(readback_);readback_state_=ReadbackState::taken;return true;
    }
#endif
    bool bind(ID3D12CommandQueue* queue,ID3D12Resource* source,uint64_t generation){
        AcquireSRWLockExclusive(&lock_);Unlock unlock{&lock_};if(stats_.state)return reject(Reason::not_ready);
        if(generation<=generation_)return reject(Reason::generation);
        if(!queue||!source||!generation||queue->GetDesc().Type!=D3D12_COMMAND_LIST_TYPE_DIRECT)return reject(Reason::source);
        const auto d=source->GetDesc();
        if(d.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D||!d.Width||d.Width>16384||d.Width%2||d.DepthOrArraySize!=1||d.MipLevels!=1||d.SampleDesc.Count!=1||d.SampleDesc.Quality||format_index(d.Format)<0)return reject(Reason::source);
        ComPtr<ID3D12Device> device,other;ComPtr<IUnknown> di,qi,si,oi;
        if(FAILED(source->GetDevice(IID_PPV_ARGS(&device)))||FAILED(queue->GetDevice(IID_PPV_ARGS(&other)))||FAILED(device.As(&di))||FAILED(other.As(&oi))||di.Get()!=oi.Get()||
           FAILED(queue->QueryInterface(IID_PPV_ARGS(&qi)))||FAILED(source->QueryInterface(IID_PPV_ARGS(&si))))return reject(Reason::device);
        queue_=queue;device_=device;queue_identity_=qi;source_identity_=si;device_identity_=di;source_desc_=d;generation_=generation;stats_.state=1;stats_.reason=Reason::none;return true;
    }
    bool prepare(){
        AcquireSRWLockExclusive(&lock_);Unlock unlock{&lock_};if(stats_.state==2)return true;if(stats_.state!=1)return reject(Reason::not_ready);
        try{
            auto desc=source_desc_;desc.Flags=D3D12_RESOURCE_FLAG_NONE;D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;
            gpu_check(device_->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(&snapshot_)),"mirror snapshot");
            gpu_check(device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&copy_allocator_)),"mirror copy allocator");
            gpu_check(device_->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,copy_allocator_.Get(),nullptr,IID_PPV_ARGS(&copy_list_)),"mirror copy list");gpu_check(copy_list_->Close(),"mirror copy close");
            gpu_check(device_->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&copy_fence_)),"mirror copy fence");
            for(auto& draw:draws_){
                gpu_check(device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&draw.allocator)),"mirror draw allocator");
                gpu_check(device_->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,draw.allocator.Get(),nullptr,IID_PPV_ARGS(&draw.list)),"mirror draw list");gpu_check(draw.list->Close(),"mirror draw close");
                gpu_check(device_->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&draw.fence)),"mirror draw fence");
                D3D12_DESCRIPTOR_HEAP_DESC rtv{};rtv.Type=D3D12_DESCRIPTOR_HEAP_TYPE_RTV;rtv.NumDescriptors=1;
                gpu_check(device_->CreateDescriptorHeap(&rtv,IID_PPV_ARGS(&draw.rtv)),"mirror RTV");
            }
            event_=CreateEventW(nullptr,FALSE,FALSE,nullptr);if(!event_)return failed(HRESULT_FROM_WIN32(GetLastError()));
            create_pipeline();D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=1;hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            gpu_check(device_->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&srv_)),"mirror SRV");D3D12_SHADER_RESOURCE_VIEW_DESC view{};
            view.Format=(format_index(desc.Format)%2)?DXGI_FORMAT_B8G8R8A8_UNORM:DXGI_FORMAT_R8G8B8A8_UNORM;view.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;view.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;view.Texture2D.MipLevels=1;
            device_->CreateShaderResourceView(snapshot_.Get(),&view,srv_->GetCPUDescriptorHandleForHeapStart());stats_.state=2;stats_.reason=Reason::none;return true;
        }catch(...){return failed(E_FAIL);}
    }
    bool capture(ID3D12Resource* source,uint64_t generation,uint64_t ticket,int64_t received_ns,D3D12_RESOURCE_STATES idle){
        if(!TryAcquireSRWLockExclusive(&lock_)){++lock_busy_;return false;}Unlock unlock{&lock_};
        if(stats_.state!=2)return reject(Reason::not_ready);if(generation!=generation_)return reject(Reason::generation);
        if(!same_object(source,source_identity_.Get())||(idle!=D3D12_RESOURCE_STATE_COMMON&&idle!=D3D12_RESOURCE_STATE_RENDER_TARGET&&idle!=D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE))return reject(Reason::source);
        if(!ticket||ticket<=stats_.ticket)return reject(Reason::ticket);if(!fresh(received_ns,monotonic_ns()))return false;
        if(!complete(copy_fence_,copy_serial_,copy_signaled_,copy_inflight_)){if(stats_.state==3)return false;++stats_.busy;stats_.reason=Reason::copy_busy;return false;}
        try{
            if(copy_serial_==UINT64_MAX)return failed(E_UNEXPECTED);
            gpu_check(copy_allocator_->Reset(),"mirror copy reset");gpu_check(copy_list_->Reset(copy_allocator_.Get(),nullptr),"mirror copy list reset");
            transition(copy_list_.Get(),source,idle,D3D12_RESOURCE_STATE_COPY_SOURCE);transition(copy_list_.Get(),snapshot_.Get(),D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_COPY_DEST);
            copy_list_->CopyResource(snapshot_.Get(),source);transition(copy_list_.Get(),snapshot_.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_COMMON);transition(copy_list_.Get(),source,D3D12_RESOURCE_STATE_COPY_SOURCE,idle);
            gpu_check(copy_list_->Close(),"mirror copy close");ID3D12CommandList* lists[]={copy_list_.Get()};copy_inflight_=true;copy_signaled_=false;queue_->ExecuteCommandLists(1,lists);
            gpu_check(queue_->Signal(copy_fence_.Get(),++copy_serial_),"mirror copy signal");copy_signaled_=true;snapshot_valid_=true;
            stats_.ticket=ticket;stats_.received_ns=received_ns;++stats_.captures;stats_.reason=Reason::none;return true;
        }catch(...){return failed(E_FAIL);}
    }
    bool present(ID3D12Resource* destination,ID3D12CommandQueue* queue,int64_t now_ns){
        if(!TryAcquireSRWLockExclusive(&lock_)){++lock_busy_;return false;}Unlock unlock{&lock_};
        if(stats_.state!=2||!snapshot_valid_)return reject(Reason::not_ready);
        if(!same_object(queue,queue_identity_.Get())||queue->GetDesc().Type!=D3D12_COMMAND_LIST_TYPE_DIRECT)return reject(Reason::queue);
        if(!fresh(stats_.received_ns,now_ns))return false;
        if(!destination)return reject(Reason::destination);const auto d=destination->GetDesc();stats_.destination_format=unsigned(d.Format);const int index=destination_index(d.Format);
        if(index<0||d.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D||!d.Width||!d.Height||d.Width>16384||d.Height>16384||d.DepthOrArraySize!=1||d.MipLevels!=1||d.SampleDesc.Count!=1||d.SampleDesc.Quality||!(d.Flags&D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)||same_object(destination,source_identity_.Get()))return reject(Reason::destination);
        ComPtr<ID3D12Device> device;ComPtr<IUnknown> identity;
        if(FAILED(destination->GetDevice(IID_PPV_ARGS(&device)))||FAILED(device.As(&identity))||identity.Get()!=device_identity_.Get())return reject(Reason::device);
        // Poll every slot, but never wait or rewrite a descriptor in flight.
        // Snapshot copies and all draws use the same certified queue: a later
        // capture is ordered after earlier reads, even when their CPU slots live.
        poll_draws();if(stats_.state==3)return false;
        DrawSlot* available{};for(auto& slot:draws_)if(!slot.inflight){available=&slot;break;}
        if(!available){++stats_.busy;stats_.reason=Reason::draw_busy;return false;}
        auto& draw=*available;
        try{
            if(draw_serial_==UINT64_MAX)return failed(E_UNEXPECTED);
#if defined(GEARS_DESKTOP_MIRROR_READBACK) && GEARS_DESKTOP_MIRROR_READBACK
            const bool read_pixels=prepare_readback(d);
#endif
            gpu_check(draw.allocator->Reset(),"mirror draw reset");gpu_check(draw.list->Reset(draw.allocator.Get(),pipelines_[index].Get()),"mirror draw list reset");
            D3D12_RENDER_TARGET_VIEW_DESC view{};view.Format=d.Format;view.ViewDimension=D3D12_RTV_DIMENSION_TEXTURE2D;
            const auto handle=draw.rtv->GetCPUDescriptorHandleForHeapStart();device_->CreateRenderTargetView(destination,&view,handle);
            transition(draw.list.Get(),destination,D3D12_RESOURCE_STATE_PRESENT,D3D12_RESOURCE_STATE_RENDER_TARGET);transition(draw.list.Get(),snapshot_.Get(),D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            const float black[]={0,0,0,1};draw.list->ClearRenderTargetView(handle,black,0,nullptr);draw.list->OMSetRenderTargets(1,&handle,FALSE,nullptr);
            const auto rectangle=mirror_rectangle(unsigned(d.Width),d.Height,unsigned(source_desc_.Width/2),source_desc_.Height);
            if(!rectangle.width||!rectangle.height)return reject(Reason::destination);
            D3D12_VIEWPORT viewport{float(rectangle.x),float(rectangle.y),float(rectangle.width),float(rectangle.height),0,1};
            D3D12_RECT scissor{LONG(rectangle.x),LONG(rectangle.y),LONG(rectangle.x+rectangle.width),LONG(rectangle.y+rectangle.height)};
            draw.list->RSSetViewports(1,&viewport);draw.list->RSSetScissorRects(1,&scissor);draw.list->SetGraphicsRootSignature(root_.Get());
            ID3D12DescriptorHeap* heaps[]={srv_.Get()};draw.list->SetDescriptorHeaps(1,heaps);draw.list->SetGraphicsRootDescriptorTable(0,srv_->GetGPUDescriptorHandleForHeapStart());
            const unsigned extent[]={static_cast<unsigned>(source_desc_.Width),source_desc_.Height};draw.list->SetGraphicsRoot32BitConstants(1,2,extent,0);
            draw.list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);draw.list->DrawInstanced(3,1,0,0);
            transition(draw.list.Get(),snapshot_.Get(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COMMON);
#if defined(GEARS_DESKTOP_MIRROR_READBACK) && GEARS_DESKTOP_MIRROR_READBACK
            if(read_pixels){
                transition(draw.list.Get(),destination,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COPY_SOURCE);
                D3D12_TEXTURE_COPY_LOCATION to{},from{};to.pResource=readback_.buffer.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;to.PlacedFootprint=readback_.footprint;
                from.pResource=destination;from.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;draw.list->CopyTextureRegion(&to,0,0,0,&from,nullptr);
                transition(draw.list.Get(),destination,D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_PRESENT);
                readback_.source=source_desc_;readback_.destination=d;readback_.rectangle=rectangle;
                readback_.generation=generation_;readback_.ticket=stats_.ticket;readback_.received_ns=stats_.received_ns;readback_.present_ns=now_ns;
                readback_.draw_serial=draw_serial_+1;readback_state_=ReadbackState::recorded;
            }else
#endif
                transition(draw.list.Get(),destination,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PRESENT);
            gpu_check(draw.list->Close(),"mirror draw close");draw.destination=destination;draw.serial=++draw_serial_;draw.inflight=true;draw.signaled=false;
#if defined(GEARS_DESKTOP_MIRROR_READBACK) && GEARS_DESKTOP_MIRROR_READBACK
            if(read_pixels){readback_fence_=draw.fence;readback_state_=ReadbackState::submitted;}
#endif
            ID3D12CommandList* lists[]={draw.list.Get()};queue_->ExecuteCommandLists(1,lists);gpu_check(queue_->Signal(draw.fence.Get(),draw.serial),"mirror draw signal");draw.signaled=true;
#if defined(GEARS_DESKTOP_MIRROR_READBACK) && GEARS_DESKTOP_MIRROR_READBACK
            if(read_pixels)readback_signaled_=true;
#endif
            if constexpr(!asynchronous){
                if(!wait(draw.fence,draw.serial,draw.signaled,draw.inflight,GetTickCount64()+1000))return false;
                draw.destination.Reset();
            }
#if defined(GEARS_DESKTOP_MIRROR_READBACK) && GEARS_DESKTOP_MIRROR_READBACK
            refresh_readback();
#endif
            ++stats_.presents;stats_.reason=Reason::none;return true;
        }catch(...){
#if defined(GEARS_DESKTOP_MIRROR_READBACK) && GEARS_DESKTOP_MIRROR_READBACK
            if(readback_state_==ReadbackState::recorded||
               (readback_state_==ReadbackState::failed&&SUCCEEDED(readback_error_)&&readback_.buffer)){
                readback_state_=ReadbackState::failed;readback_error_=E_FAIL;
            }
#endif
            return failed(E_FAIL);}
    }
    // A timed-out draw stays disabled, but a later completed fence must release
    // its backbuffer reference so subsequent ResizeBuffers can succeed. Polling
    // neither waits nor destroys resources whose submission is still unconfirmed.
    bool poll(){
        if(!TryAcquireSRWLockExclusive(&lock_))return false;Unlock unlock{&lock_};
        if(stats_.state==4)return !stats_.unsafe;
        const bool copied=complete(copy_fence_,copy_serial_,copy_signaled_,copy_inflight_);
        const bool drawn=poll_draws();
        return copied&&drawn;
    }
    // Nonterminal source retirement. The caller has stopped old-world producers.
    // Freeze both copy and presentation before polling either fence; a pending
    // GPU operation keeps every resource alive and may be retried without waits.
    // Keep generation and ticket high-water marks across the next bind.
    bool retire_source(uint64_t expected_generation){
        if(!TryAcquireSRWLockExclusive(&lock_))return false;Unlock unlock{&lock_};
        if(!expected_generation||expected_generation!=generation_)return reject(Reason::generation);
        if(stats_.state==0)return true;
        if(stats_.state!=1&&stats_.state!=2&&stats_.state!=5)return false;
        stats_.state=5;snapshot_valid_=false;
        const bool copied=complete(copy_fence_,copy_serial_,copy_signaled_,copy_inflight_);
        const bool drawn=poll_draws();
        if(!copied||!drawn||stats_.state==3)return false;
        release(false);source_desc_={};copy_serial_=draw_serial_=0;
        copy_signaled_=false;for(auto& draw:draws_)draw={};
        stats_.received_ns=0;stats_.state=0;stats_.reason=Reason::none;return true;
    }
    bool drain(unsigned timeout_ms=1000){AcquireSRWLockExclusive(&lock_);Unlock unlock{&lock_};if(stats_.state==4)return !stats_.unsafe;return drain_locked(timeout_ms);}
    bool close(unsigned timeout_ms=1000){
        AcquireSRWLockExclusive(&lock_);Unlock unlock{&lock_};if(stats_.state==4)return !stats_.unsafe;
        const bool safe=drain_locked(timeout_ms);stats_.unsafe=!safe;release(!safe);stats_.state=4;stats_.reason=Reason::closed;return safe;
    }
};
}
