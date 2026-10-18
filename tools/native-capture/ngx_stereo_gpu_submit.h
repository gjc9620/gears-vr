#pragma once
#include "ngx_stereo_adapter.h"
#include <d3d12.h>
#include <wrl/client.h>

namespace gears_ngx_stereo {
// Direct host transport for the already-recorded adapter calls. Call under the
// same API-order lock as Reset/Execute. The context is pinned by ControlOwner;
// destroying it is legal only after the adapter's drained detach succeeds.
// This adds no Execute, Wait, barrier, guessed producer or acceptance receipt.
class NativeGpuSubmission {
public:
    using Original=void(STDMETHODCALLTYPE*)(ID3D12CommandQueue*,UINT,ID3D12CommandList* const*);
    using Read=bool(*)(void*,void const*,size_t);
    using Generation=uint64_t(*)(ID3D12CommandList*,uintptr_t);
private:
    template<class T> using Ptr=Microsoft::WRL::ComPtr<T>;
    struct Member {Ptr<ID3D12CommandList> object;uintptr_t id{};uint64_t generation{};};
    struct Seen {Ptr<IUnknown> identity;uint64_t generation{};};
    struct Flight {std::array<Member,64> members{};UINT count{};uint64_t value{};};
    Ptr<ID3D12CommandQueue> queue_;Ptr<IUnknown> queue_id_,device_id_,fence_id_;
    Ptr<ID3D12Device> device_;Ptr<ID3D12Fence> fence_;
    std::array<Seen,128> seen_{};std::array<Flight,8> flights_{};
    uint64_t value_{},order_{};bool failed_{};
    bool bind(ID3D12CommandQueue* queue){
        Ptr<IUnknown> qid,did;Ptr<ID3D12Device> device;
        if(!queue||queue->GetDesc().Type!=D3D12_COMMAND_LIST_TYPE_DIRECT||
           FAILED(queue->QueryInterface(IID_PPV_ARGS(&qid)))||FAILED(queue->GetDevice(IID_PPV_ARGS(&device)))||FAILED(device.As(&did)))return false;
        if(queue_)return qid.Get()==queue_id_.Get()&&did.Get()==device_id_.Get();
        Ptr<ID3D12Fence> fence;Ptr<IUnknown> fid;
        if(FAILED(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)))||FAILED(fence.As(&fid)))return false;
        queue_=queue;queue_id_=qid;device_=device;device_id_=did;fence_=fence;fence_id_=fid;return true;
    }
    void fail(StereoAdapter& adapter){failed_=true;auto entry=adapter.enter();if(entry)adapter.invalidate(entry);}
    bool seen(Member const& member){
        for(auto const& s:seen_)if(reinterpret_cast<uintptr_t>(s.identity.Get())==member.id)return member.generation<=s.generation;
        return false;
    }
    bool remember(Member const& member){
        Seen* empty{};for(auto& s:seen_){if(reinterpret_cast<uintptr_t>(s.identity.Get())==member.id){s.generation=member.generation;return true;}if(!s.identity&&!empty)empty=&s;}
        if(!empty||FAILED(member.object.As(&empty->identity)))return false;empty->generation=member.generation;return true;
    }
public:
    bool failed()const{return failed_;}
    Completion completion()const{
        if(failed_||!fence_||!value_)return {};const auto value=fence_->GetCompletedValue();
        if(value==UINT64_MAX||value>value_)return {};
        return {reinterpret_cast<uintptr_t>(device_id_.Get()),reinterpret_cast<uintptr_t>(queue_id_.Get()),reinterpret_cast<uintptr_t>(fence_id_.Get()),value};
    }
    // Always forwards the exact game Execute arguments once, including on
    // rejection. Signal and adapter receipts happen only AFTER that returns.
    bool execute(StereoAdapter& adapter,ID3D12CommandQueue* queue,UINT count,ID3D12CommandList* const* lists,
        Original original,Read read,Generation generation){
        std::array<PendingRecording,5> pending{};unsigned n{};
        {auto entry=adapter.enter();if(!entry){failed_=true;original(queue,count,lists);return false;}n=adapter.pending_recordings(entry,pending);}
        std::array<ID3D12CommandList*,64> raw{};Flight batch;bool valid=!failed_&&read&&generation&&count<=raw.size()&&(!count||read(raw.data(),lists,count*sizeof(raw[0])));
        bool matched{};std::array<bool,64> owned{};
        for(UINT i=0;valid&&i<count;++i){
            auto& member=batch.members[i];member.object=raw[i];Ptr<IUnknown> identity;
            valid=raw[i]&&SUCCEEDED(raw[i]->QueryInterface(IID_PPV_ARGS(&identity)));
            if(!valid)break;member.id=reinterpret_cast<uintptr_t>(identity.Get());member.generation=generation(raw[i],member.id);
            if(seen(member)){valid=false;break;}
            for(unsigned j=0;j<n;++j)if(member.id==pending[j].list){
                owned[i]=matched=true;if(member.generation!=pending[j].generation)valid=false;
            }
            if(owned[i]){
                Ptr<ID3D12Device> device;Ptr<IUnknown> did;
                if(raw[i]->GetType()!=D3D12_COMMAND_LIST_TYPE_DIRECT||FAILED(raw[i]->GetDevice(IID_PPV_ARGS(&device)))||FAILED(device.As(&did)))valid=false;
                if(valid&&(!bind(queue)||did.Get()!=device_id_.Get()))valid=false;
                for(UINT j=0;j<i;++j)if(batch.members[j].id==member.id)valid=false;
            }
        }
        Flight* retained{};
        if(valid&&matched){
            const auto completed=fence_->GetCompletedValue();if(completed==UINT64_MAX)valid=false;
            for(auto& flight:flights_){if(flight.value&&flight.value<=completed)flight={};if(!flight.value&&!retained)retained=&flight;}
            if(!retained||value_>=UINT64_MAX-1||order_==UINT64_MAX)valid=false;
            for(UINT i=0;valid&&i<count;++i)if(owned[i]&&!remember(batch.members[i]))valid=false;
            if(valid){batch.count=count;batch.value=value_+1;*retained=std::move(batch);}
        }
        if(!valid)fail(adapter);
        try{original(queue,count,lists);}catch(...){fail(adapter);throw;}
        if(!valid)return false;if(!matched)return true;
        // A concurrent Reset changes generation; never let the old snapshot
        // certify whatever recording happens to occupy that pointer now.
        for(UINT i=0;i<count;++i)if(owned[i]&&generation(retained->members[i].object.Get(),retained->members[i].id)!=retained->members[i].generation){fail(adapter);return false;}
        ++value_;++order_;if(FAILED(queue_->Signal(fence_.Get(),value_))){fail(adapter);return false;}
        for(UINT member=0;member<count;++member)if(owned[member]){
            auto const& m=retained->members[member];
            ExecuteReceipt receipt{{reinterpret_cast<uintptr_t>(device_id_.Get()),m.id,m.generation,reinterpret_cast<uintptr_t>(queue_id_.Get()),reinterpret_cast<uintptr_t>(fence_id_.Get()),value_,true},order_,member};
            // Refresh after each receipt: same-list Eval may also discharge
            // Copy. Its actual receipt must not be submitted a second time.
            for(unsigned slot=0;slot<5;++slot){
                auto entry=adapter.enter();if(!entry){failed_=true;return false;}
                std::array<PendingRecording,5> fresh{};const auto fresh_count=adapter.pending_recordings(entry,fresh);bool found{};
                for(unsigned j=0;j<fresh_count;++j){auto const& r=fresh[j];if(r.list!=m.id||r.generation!=m.generation)continue;
                    found=true;bool ok{};
                    if(r.kind==RecordingKind::creation)ok=adapter.submitted_creation(entry,receipt);
                    else if(r.kind==RecordingKind::evaluate)ok=adapter.submitted_eye(entry,r.eye,receipt);
                    else ok=adapter.submitted_copy(entry,r.eye,receipt);
                    if(!ok){failed_=true;adapter.invalidate(entry);return false;}break;
                }
                if(!found)break;
            }
        }
        return true;
    }
};
}
