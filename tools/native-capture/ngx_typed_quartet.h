#pragma once
#include "ngx_typed_resources.h"

namespace gears_ngx_stereo {
// Ownership only: no resource state, producer domain, submission or fence proof.
// Construct once per control owner/Create epoch. No clear/rebind operation: the
// host destroys this object only after its real drained-detach checks succeed.
// Lease is injectable for CPU reference-lifetime tests; production uses COM.
template<class Lease=TypedResourceLease,size_t Capacity=64>
class BasicTypedQuartet {
    Identity identity_{};uint64_t ticket_{};bool bound_{},failed_{};
    std::array<Lease,Capacity> candidates_{};size_t count_{};
    std::array<Lease,4> native_{};
    bool fail(){failed_=true;return false;}
    bool scope(Identity id,uint64_t ticket){
        if(failed_||!valid(identity_)||!same(id,identity_)||!ticket)return fail();
        if(ticket_!=ticket){
            if(ticket_&&ticket<ticket_)return fail();
            // Unbound leases are observation-only: no owned SDK call has used
            // them. Never combine candidate resources from different tickets.
            if(!bound_){candidates_={};count_=0;}ticket_=ticket;
        }
        return true;
    }
public:
    BasicTypedQuartet()=default;
    BasicTypedQuartet(BasicTypedQuartet const&)=delete;
    BasicTypedQuartet& operator=(BasicTypedQuartet const&)=delete;
    bool begin(Identity id){
        if(failed_||!valid(id)||valid(identity_))return fail();identity_=id;return true;
    }
    bool observe_resource(Identity id,uint64_t ticket,ID3D12Resource* resource){
        if(!scope(id,ticket))return false;
        if(!resource)return true; // Null global UAV barriers identify no object.
        const auto raw=reinterpret_cast<uintptr_t>(resource);
        // Retention prevents address reuse. Once bound, unrelated resources
        // need no lease; producer authentication remains a separate callback.
        if(bound_)return true;
        for(size_t i=0;i<count_;++i)if(candidates_[i].raw==raw)return true;
        auto lease=Lease::observe(resource);Resource value{};
        // Other valid D3D resources (buffers, arrays) are not SR candidates.
        if(!lease.retained())return fail();
        if(!lease.materialize(value)||value.device!=id.device)return true;
        if(count_==Capacity)return fail(); // Only an eligible unique SR candidate consumes capacity.
        candidates_[count_++]=std::move(lease);return true;
    }
    bool bind(Identity id,uint64_t ticket,std::array<uintptr_t,4> const& raw,std::array<Resource,4>& out){
        out={};if(!scope(id,ticket))return false;
        std::array<Lease,4> selected{};std::array<Resource,4> values{};
        constexpr unsigned order[]{0,2,3,1}; // native CODM -> adapter CDMO
        for(unsigned n=0;n<4;++n){
            if(!raw[n])return fail();
            if(bound_){if(native_[n].raw!=raw[n])return fail();selected[n]=native_[n];}
            else for(size_t j=0;j<count_;++j)if(candidates_[j].raw==raw[n]){selected[n]=candidates_[j];break;}
        }
        for(unsigned i=0;i<4;++i){
            if(!selected[order[i]].materialize(values[i])||values[i].device!=id.device)return fail();
            for(unsigned j=0;j<i;++j)if(values[i].identity==values[j].identity)return fail();
        }
        if(!bound_){native_=std::move(selected);bound_=true;candidates_={};count_=0;}
        out=values;return true;
    }
    bool failed()const{return failed_;}
    bool bound()const{return bound_;}
    size_t held()const{return bound_?4:count_;}
    // Acquisition failure invalidates admission, never destroys held leases.
    void uncertain(){failed_=true;}
};
using TypedQuartet=BasicTypedQuartet<>;
// Resource pointers originate only in the typed API arguments. These are
// acquisition events, not claims that a requested transition executed.
template<class Quartet>
bool acquire_quartet_barrier(Quartet& q,Identity id,uint64_t ticket,D3D12_RESOURCE_BARRIER const& b){
    switch(b.Type){
    case D3D12_RESOURCE_BARRIER_TYPE_TRANSITION:return q.observe_resource(id,ticket,b.Transition.pResource);
    case D3D12_RESOURCE_BARRIER_TYPE_UAV:return q.observe_resource(id,ticket,b.UAV.pResource);
    case D3D12_RESOURCE_BARRIER_TYPE_ALIASING:
        return q.observe_resource(id,ticket,b.Aliasing.pResourceBefore)&&q.observe_resource(id,ticket,b.Aliasing.pResourceAfter);
    default:q.uncertain();return false;
    }
}
}
