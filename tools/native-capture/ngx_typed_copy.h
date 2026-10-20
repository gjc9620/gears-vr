#pragma once
#include "ngx_typed_resources.h"
#include <stdexcept>

namespace gears_ngx_stereo {
using TypedCopyOriginal=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,D3D12_TEXTURE_COPY_LOCATION const*,UINT,UINT,UINT,
    D3D12_TEXTURE_COPY_LOCATION const*,D3D12_BOX const*);
// CPU shape check at the authenticated native return RVA. This preserves the
// original helper's COPY_SOURCE/COPY_DEST setup, flush, and post-call restore:
// only the copy box/destination may change. It does not observe GPU state.
inline bool preserves_native_copy(bool attested,uintptr_t caller,
    D3D12_TEXTURE_COPY_LOCATION const& dst,UINT x,UINT y,UINT z,
    D3D12_TEXTURE_COPY_LOCATION const& src,D3D12_BOX const* box,
    uintptr_t retained_output,uintptr_t retained_color){
    return attested&&caller==0x1c6093&&!x&&!y&&!z&&!box&&retained_output&&retained_color&&
        retained_output!=retained_color&&reinterpret_cast<uintptr_t>(src.pResource)==retained_output&&
        reinterpret_cast<uintptr_t>(dst.pResource)==retained_color&&
        src.Type==D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX&&dst.Type==D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX&&
        !src.SubresourceIndex&&!dst.SubresourceIndex;
}
// One admitted native Output->Color boundary. All objects come from its typed
// arguments, never from integer IDs in CopyPlan. The caller owns state/producer
// and ordering proof; this adapter adds no barrier, Execute, Signal or wait.
// The current callback verifies the host's actual canonical list/Reset generation.
class TypedCopyBoundary {
public:
    using Current=bool(*)(void*,ID3D12GraphicsCommandList*,uintptr_t,uint64_t);
private:
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_;
    TypedResourceLease source_,target_;
    D3D12_TEXTURE_COPY_LOCATION src_{},dst_{};
    uintptr_t canonical_{};uint64_t generation_{},ticket_{};unsigned ordinal_{};
    void* context_{};Current current_{};TypedCopyOriginal original_{};bool consumed_{},valid_{};
public:
    TypedCopyBoundary(ID3D12GraphicsCommandList* command,uintptr_t canonical,uint64_t generation,uint64_t ticket,unsigned ordinal,
        D3D12_TEXTURE_COPY_LOCATION const& destination,D3D12_TEXTURE_COPY_LOCATION const& source,
        void* context,Current current,TypedCopyOriginal original)
        :command_(command),source_(TypedResourceLease::observe(source.pResource)),target_(TypedResourceLease::observe(destination.pResource)),
         src_(source),dst_(destination),canonical_(canonical),generation_(generation),ticket_(ticket),ordinal_(ordinal),
         context_(context),current_(current),original_(original){
        Resource s{},d{};Microsoft::WRL::ComPtr<IUnknown> identity,device_identity;Microsoft::WRL::ComPtr<ID3D12Device> device;
        valid_=command&&canonical&&generation&&ticket&&ordinal<2&&current&&original&&
            src_.Type==D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX&&dst_.Type==D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX&&
            !src_.SubresourceIndex&&!dst_.SubresourceIndex&&source_.materialize(s)&&target_.materialize(d)&&
            s.identity!=d.identity&&s.device==d.device&&s.format==d.format&&command->GetType()==D3D12_COMMAND_LIST_TYPE_DIRECT&&
            SUCCEEDED(command->QueryInterface(IID_PPV_ARGS(&identity)))&&reinterpret_cast<uintptr_t>(identity.Get())==canonical&&
            SUCCEEDED(command->GetDevice(IID_PPV_ARGS(&device)))&&SUCCEEDED(device.As(&device_identity))&&
            reinterpret_cast<uintptr_t>(device_identity.Get())==s.device;
    }
    bool copy(CopyPlan const& plan){
        if(!valid_||consumed_||plan.source!=source_.canonical||plan.target!=target_.canonical||plan.list!=canonical_||
           plan.generation!=generation_||plan.ticket!=ticket_||plan.ordinal!=ordinal_||!current_(context_,command_.Get(),canonical_,generation_))return false;
        const auto& b=plan.source_box;const auto& d=plan.destination;
        if(b[0]||b[1]||b[2]||b[5]!=1||!b[3]||!b[4]||b[3]>source_.desc.Width||b[4]>source_.desc.Height||d[2]||
           d[0]>target_.desc.Width||d[1]>target_.desc.Height||b[3]>target_.desc.Width-d[0]||b[4]>target_.desc.Height-d[1])return false;
        D3D12_BOX box{b[0],b[1],b[2],b[3],b[4],b[5]};consumed_=true;
        original_(command_.Get(),&dst_,d[0],d[1],d[2],&src_,&box);
        return current_(context_,command_.Get(),canonical_,generation_);
    }
    // Directly usable as NativeDispatchFunctions::copy. Failure is not reported
    // as a successful boxed native copy, including drift during the original.
    static void dispatch(void* context,CopyPlan const& plan){
        if(!context||!static_cast<TypedCopyBoundary*>(context)->copy(plan))throw std::runtime_error("NGX typed copy boundary rejected");
    }
};
}
