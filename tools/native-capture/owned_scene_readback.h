#pragma once
#include <windows.h>
#include <d3d12.h>
#include <dxgi.h>
#include <wrl/client.h>
#include <array>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <string>
#include <vector>

namespace owned_scene_readback {
enum class Status { idle, pending, saved, failed };
enum class Error { none, repeated_queue, invalid_argument, invalid_state, wrong_queue,
    wrong_device, unsupported_texture, footprint, create, close_list, signal,
    removed_fence, map, file_open, file_write };
struct EyeStats {
    uint64_t nonblack{}, hash{14695981039346656037ull};
    std::array<unsigned char,3> min_rgb{{255,255,255}}, max_rgb{};
};
struct Result {
    Status status{Status::idle}; Error error{Error::none}; HRESULT hr{S_OK};
    bool queued{}, completed{}, saved{}; UINT width{}, height{};
    std::array<EyeStats,2> eyes{}; uint64_t differing_pixels{};
};

// A serialized, one-shot receipt of RGB pixels, not a geometry correctness test.
// Caller certifies an explicit legacy idle state and serializes source access on
// this DIRECT queue. Hold capture's existing publish bypass around queue(): its
// ExecuteCommandLists must not be interpreted as a scene submission.
class Probe {
    template<class T> using Ptr = Microsoft::WRL::ComPtr<T>;
    Ptr<ID3D12Resource> source_, readback_;
    Ptr<ID3D12CommandQueue> queue_;
    Ptr<ID3D12Device> device_;
    Ptr<ID3D12CommandAllocator> allocator_;
    Ptr<ID3D12GraphicsCommandList> list_;
    Ptr<ID3D12Fence> fence_;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint_{};
    UINT64 bytes_{}; std::wstring path_; Result result_{};
    bool attempted_{}, executed_{}, signaled_{};
    bool fail(Error error, HRESULT hr) {
        result_.status=Status::failed; result_.error=error; result_.hr=hr; return false;
    }
    static bool absolute(const wchar_t* path) {
        if(!path || !*path) return false;
        return (path[0] && path[1]==L':' && (path[2]==L'\\'||path[2]==L'/') &&
            ((path[0]>=L'A'&&path[0]<=L'Z')||(path[0]>=L'a'&&path[0]<=L'z'))) ||
            (path[0]==L'\\'&&path[1]==L'\\'&&path[2]!=0);
    }
public:
    Probe()=default;
    Probe(const Probe&)=delete; Probe& operator=(const Probe&)=delete;
    ~Probe() {
        // Execute has no HRESULT. An unsuccessful Signal or removed fence leaves
        // completion uncertain; deliberately retain all submitted objects until
        // process exit rather than risk destroying GPU-visible storage.
        if(executed_ && !result_.completed) {
            source_.Detach(); readback_.Detach(); queue_.Detach(); device_.Detach();
            allocator_.Detach(); list_.Detach(); fence_.Detach();
        }
    }
    const Result& result() const { return result_; }
    bool retained() const {return source_||readback_||queue_||device_||allocator_||list_||fence_;}
    // Keep the immutable result for reporting; release only our actual COM
    // leases after this probe's own submission fence proves completion.
    bool close() {
        if(executed_&&!result_.completed)poll();
        if(executed_&&!result_.completed)return false;
        source_.Reset();readback_.Reset();queue_.Reset();device_.Reset();
        allocator_.Reset();list_.Reset();fence_.Reset();return true;
    }
    bool rearm() {
        if(retained()||(executed_&&!result_.completed))return false;
        footprint_={};bytes_=0;path_.clear();result_={};attempted_=executed_=signaled_=false;return true;
    }
    bool queue(ID3D12Resource* source, ID3D12CommandQueue* queue,
               D3D12_RESOURCE_STATES idle_state, bool certified_idle_state,
               const wchar_t* absolute_ppm_path) {
        if(attempted_) return fail(Error::repeated_queue,E_UNEXPECTED);
        attempted_=true;
        if(!source || !queue || !absolute(absolute_ppm_path))
            return fail(Error::invalid_argument,E_INVALIDARG);
        const unsigned state=static_cast<unsigned>(idle_state);
        if(!certified_idle_state || (state!=0 && state!=4 && state!=128))
            return fail(Error::invalid_state,E_INVALIDARG);
        if(queue->GetDesc().Type!=D3D12_COMMAND_LIST_TYPE_DIRECT)
            return fail(Error::wrong_queue,E_INVALIDARG);
        source_=source; queue_=queue; path_=absolute_ppm_path;
        HRESULT hr=source_->GetDevice(IID_PPV_ARGS(&device_));
        if(FAILED(hr)) return fail(Error::wrong_device,hr);
        Ptr<ID3D12Device> queue_device;
        hr=queue_->GetDevice(IID_PPV_ARGS(&queue_device));
        if(FAILED(hr)) return fail(Error::wrong_device,hr);
        Ptr<IUnknown> source_identity, queue_identity;
        hr=device_.As(&source_identity);
        if(FAILED(hr)) return fail(Error::wrong_device,hr);
        hr=queue_device.As(&queue_identity);
        if(FAILED(hr)) return fail(Error::wrong_device,hr);
        if(source_identity.Get()!=queue_identity.Get()) return fail(Error::wrong_device,E_INVALIDARG);
        const auto desc=source_->GetDesc();
        if(desc.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D ||
           desc.Format!=DXGI_FORMAT_R8G8B8A8_UNORM || desc.Width<2 ||
           desc.Width>(std::numeric_limits<UINT>::max)() || !desc.Height ||
           desc.Width%2 || desc.DepthOrArraySize!=1 ||
           desc.MipLevels!=1 || desc.SampleDesc.Count!=1 || desc.SampleDesc.Quality!=0)
            return fail(Error::unsupported_texture,E_INVALIDARG);
        result_.width=static_cast<UINT>(desc.Width); result_.height=desc.Height;
        UINT rows{}; UINT64 row_bytes{};
        device_->GetCopyableFootprints(&desc,0,1,0,&footprint_,&rows,&row_bytes,&bytes_);
        if(bytes_==UINT64_MAX || !bytes_ || bytes_>SIZE_MAX || rows!=desc.Height ||
           row_bytes!=desc.Width*4 || footprint_.Footprint.RowPitch<row_bytes)
            return fail(Error::footprint,E_INVALIDARG);
        D3D12_HEAP_PROPERTIES heap{}; heap.Type=D3D12_HEAP_TYPE_READBACK;
        D3D12_RESOURCE_DESC buffer{}; buffer.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;
        buffer.Width=bytes_; buffer.Height=1; buffer.DepthOrArraySize=1; buffer.MipLevels=1;
        buffer.SampleDesc.Count=1; buffer.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        hr=device_->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&buffer,
            D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&readback_));
        if(FAILED(hr)) return fail(Error::create,hr);
        hr=device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator_));
        if(FAILED(hr)) return fail(Error::create,hr);
        hr=device_->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator_.Get(),nullptr,IID_PPV_ARGS(&list_));
        if(FAILED(hr)) return fail(Error::create,hr);
        hr=device_->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence_));
        if(FAILED(hr)) return fail(Error::create,hr);
        D3D12_RESOURCE_BARRIER barrier{}; barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource=source_.Get(); barrier.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore=idle_state; barrier.Transition.StateAfter=D3D12_RESOURCE_STATE_COPY_SOURCE;
        list_->ResourceBarrier(1,&barrier);
        D3D12_TEXTURE_COPY_LOCATION destination{}, origin{};
        destination.pResource=readback_.Get(); destination.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        destination.PlacedFootprint=footprint_;
        origin.pResource=source_.Get(); origin.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        list_->CopyTextureRegion(&destination,0,0,0,&origin,nullptr);
        barrier.Transition.StateBefore=D3D12_RESOURCE_STATE_COPY_SOURCE; barrier.Transition.StateAfter=idle_state;
        list_->ResourceBarrier(1,&barrier);
        hr=list_->Close(); if(FAILED(hr)) return fail(Error::close_list,hr);
        ID3D12CommandList* lists[]={list_.Get()};
        executed_=true; result_.queued=true;
        queue_->ExecuteCommandLists(1,lists);
        hr=queue_->Signal(fence_.Get(),1);
        if(FAILED(hr)) return fail(Error::signal,hr);
        signaled_=true; result_.status=Status::pending; return true;
    }
    // No GPU waits. File serialization occurs only after fence completion.
    bool poll() {
        if(result_.status!=Status::pending || !signaled_) return result_.saved;
        const UINT64 completed=fence_->GetCompletedValue();
        if(completed==UINT64_MAX) return fail(Error::removed_fence,DXGI_ERROR_DEVICE_REMOVED);
        if(completed<1) return false;
        result_.completed=true;
        void* mapped{}; D3D12_RANGE read_range{0,static_cast<SIZE_T>(bytes_)};
        HRESULT hr=readback_->Map(0,&read_range,&mapped);
        if(FAILED(hr)) return fail(Error::map,hr);
        const auto* pixels=static_cast<const unsigned char*>(mapped)+footprint_.Offset;
        HANDLE file=CreateFileW(path_.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(file==INVALID_HANDLE_VALUE) {
            const auto error=GetLastError(); D3D12_RANGE written{0,0}; readback_->Unmap(0,&written);
            return fail(Error::file_open,HRESULT_FROM_WIN32(error));
        }
        bool ok=true; DWORD write_error=ERROR_WRITE_FAULT;
        auto write=[&](const void* data,DWORD size) {
            DWORD actual{};
            if(!WriteFile(file,data,size,&actual,nullptr)) {write_error=GetLastError();return false;}
            return actual==size;
        };
        char header[80]{}; const int header_size=std::snprintf(header,sizeof(header),"P6\n%u %u\n255\n",result_.width,result_.height);
        ok=header_size>0 && write(header,static_cast<DWORD>(header_size));
        // A valid D3D12 RGBA8 texture's dimensions keep this row within DWORD.
        std::vector<unsigned char> rgb;
        try { rgb.resize(static_cast<size_t>(result_.width)*3); }
        catch(...) { ok=false; write_error=ERROR_NOT_ENOUGH_MEMORY; }
        const UINT half=result_.width/2;
        for(UINT y=0;ok && y<result_.height;++y) {
            const auto* row=pixels+static_cast<size_t>(y)*footprint_.Footprint.RowPitch;
            for(UINT x=0;x<result_.width;++x) {
                const auto* p=row+static_cast<size_t>(x)*4; auto& eye=result_.eyes[x>=half];
                if(p[0]||p[1]||p[2]) ++eye.nonblack;
                for(unsigned c=0;c<3;++c) {
                    rgb[static_cast<size_t>(x)*3+c]=p[c];
                    if(p[c]<eye.min_rgb[c]) eye.min_rgb[c]=p[c];
                    if(p[c]>eye.max_rgb[c]) eye.max_rgb[c]=p[c];
                    eye.hash=(eye.hash^p[c])*1099511628211ull;
                }
                if(x<half) { const auto* other=p+static_cast<size_t>(half)*4;
                    if(p[0]!=other[0]||p[1]!=other[1]||p[2]!=other[2]) ++result_.differing_pixels; }
            }
            ok=write(rgb.data(),static_cast<DWORD>(rgb.size()));
        }
        if(ok && !FlushFileBuffers(file)) {ok=false;write_error=GetLastError();}
        if(!CloseHandle(file)) {if(ok)write_error=GetLastError();ok=false;}
        D3D12_RANGE written{0,0}; readback_->Unmap(0,&written);
        if(!ok) {DeleteFileW(path_.c_str());return fail(Error::file_write,HRESULT_FROM_WIN32(write_error));}
        result_.saved=true; result_.status=Status::saved; return true;
    }
};
} // namespace owned_scene_readback
