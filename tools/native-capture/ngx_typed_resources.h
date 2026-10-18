#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include "ngx_stereo_layout.h"

namespace gears_ngx_stereo {
// Construct only at a typed D3D12 API boundary. Never cast an NGX parameter
// integer to COM. Copies share actual ownership, so bank rotation cannot make
// a sampled row's address refer to a different resource lifetime.
struct TypedResourceLease {
    uintptr_t raw{},canonical{},device{};
    D3D12_RESOURCE_DESC desc{};
    HRESULT hr{E_POINTER};
private:
    Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
    Microsoft::WRL::ComPtr<IUnknown> identity_;
    Microsoft::WRL::ComPtr<ID3D12Device> device_;
    Microsoft::WRL::ComPtr<IUnknown> device_identity_;
public:
    static TypedResourceLease observe(ID3D12Resource* resource){
        TypedResourceLease out;out.raw=reinterpret_cast<uintptr_t>(resource);if(!resource)return out;
        out.resource_=resource;
        out.hr=resource->QueryInterface(IID_PPV_ARGS(&out.identity_));if(FAILED(out.hr))return out;
        out.canonical=reinterpret_cast<uintptr_t>(out.identity_.Get());out.desc=resource->GetDesc();
        out.hr=resource->GetDevice(IID_PPV_ARGS(&out.device_));if(FAILED(out.hr))return out;
        out.hr=out.device_.As(&out.device_identity_);if(SUCCEEDED(out.hr))out.device=reinterpret_cast<uintptr_t>(out.device_identity_.Get());
        return out;
    }
    bool retained() const {return SUCCEEDED(hr)&&resource_&&identity_&&device_&&device_identity_&&
        raw==reinterpret_cast<uintptr_t>(resource_.Get())&&canonical==reinterpret_cast<uintptr_t>(identity_.Get())&&
        device==reinterpret_cast<uintptr_t>(device_identity_.Get());}
    ID3D12Resource* get() const {return retained()?resource_.Get():nullptr;}
    bool materialize(Resource& out) const {
        if(!retained()||desc.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D||!desc.Width||desc.Width>8192||
           !desc.Height||desc.Height>8192||desc.MipLevels!=1||desc.DepthOrArraySize!=1||desc.SampleDesc.Count!=1)return false;
        out={canonical,device,{uint32_t(desc.Width),desc.Height},uint32_t(desc.Format),0,1,0};return true;
    }
    // No GPU work is added by observing. Release only after all CPU observer
    // callbacks drain; this is not retirement of SDK features or native owners.
    void release(){resource_.Reset();identity_.Reset();device_.Reset();device_identity_.Reset();}
};
// Native wrapper order is Color,Output,Depth,MV; adapter order is Color,Depth,MV,
// Output. Resolve only exact already-retained pointers, never dimensions/RTV rank.
// Success proves typed allocation identity/topology, NOT producer pixels, state,
// logical eye, execution dependency or GPU completion.
inline bool resolve_typed_resources(std::array<TypedResourceLease,4> const& native,
    std::array<uintptr_t,4> const& raw,std::array<Resource,4>& out){
    std::array<Resource,4> candidate{};constexpr unsigned order[]{0,2,3,1};
    for(unsigned i=0;i<4;++i){auto const& r=native[order[i]];
        if(!raw[order[i]]||r.raw!=raw[order[i]]||!r.materialize(candidate[i]))return false;
        if(i&&candidate[i].device!=candidate[0].device)return false;
        for(unsigned j=0;j<i;++j)if(candidate[i].identity==candidate[j].identity)return false;
    }
    out=candidate;return true;
}
}
