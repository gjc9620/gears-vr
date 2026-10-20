#pragma once
#include "ngx_stereo_adapter.h"
#include <d3d12.h>
#include <wrl/client.h>

namespace gears_ngx_stereo {
// Call under the same API-order lock as actual Create/Eval/Copy, Reset and
// Execute. Only actual NGX recordings acquire a lease. Non-NGX lists never
// occupy this transport's bounded lifetime tables.
class NativeGpuSubmission {
public:
    using Original=void(STDMETHODCALLTYPE*)(ID3D12CommandQueue*,UINT,ID3D12CommandList* const*);
    using Read=bool(*)(void*,void const*,size_t);
    using Generation=uint64_t(*)(ID3D12CommandList*,uintptr_t);
private:
    template<class T> using Ptr=Microsoft::WRL::ComPtr<T>;
    struct Member {Ptr<ID3D12CommandList> object;uintptr_t id{};uint64_t generation{};UINT index{};};
    struct Seen {Ptr<IUnknown> identity;uint64_t generation{},value{};bool reset{};};
    struct Flight {std::array<Member,5> members{};UINT count{};uint64_t value{};};
    Ptr<ID3D12CommandQueue> queue_;Ptr<IUnknown> queue_id_,device_id_,fence_id_;
    Ptr<ID3D12Device> device_;Ptr<ID3D12Fence> fence_;
    std::array<Member,6> recordings_{};
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
    bool seen(Member const& member)const{
        for(auto const& s:seen_)if(reinterpret_cast<uintptr_t>(s.identity.Get())==member.id&&
            (member.generation<=s.generation||!s.reset))return true;
        return false;
    }
    bool remember(Member const& member,uint64_t value){
        for(auto& s:seen_)if(!s.identity){
            if(FAILED(member.object.As(&s.identity)))return false;
            s.generation=member.generation;s.value=value;s.reset=false;return true;
        }
        return false;
    }
    bool collect(){
        if(failed_)return false;if(!fence_)return true;
        const auto completed=fence_->GetCompletedValue();if(completed==UINT64_MAX||completed>value_)return false;
        for(auto& flight:flights_)if(flight.value&&flight.value<=completed)flight={};
        // GPU completion alone must not forget a submitted recording: a
        // repeated Execute of that unchanged generation is still forbidden.
        for(auto& s:seen_)if(s.identity&&s.reset&&s.value&&s.value<=completed)s={};
        return true;
    }
    bool pending(StereoAdapter& adapter,std::array<PendingRecording,5>& out,unsigned& count){
        auto entry=adapter.enter();if(!entry){failed_=true;return false;}count=adapter.pending_recordings(entry,out);return true;
    }
    void prune(std::array<PendingRecording,5> const& pending,unsigned n){
        for(auto& r:recordings_)if(r.object){bool needed{};
            for(unsigned i=0;i<n;++i)if(pending[i].list==r.id&&pending[i].generation==r.generation)needed=true;
            if(!needed)r={}; // Provisional call rejected before recording, or now owned by a Flight.
        }
    }
public:
    bool failed()const{return failed_;}
    size_t retained_recordings()const{size_t n{};for(auto const& r:recordings_)if(r.object)++n;return n;}
    size_t submitted_generations()const{size_t n{};for(auto const& s:seen_)if(s.identity)++n;return n;}
    size_t inflight_batches()const{size_t n{};for(auto const& f:flights_)if(f.value)++n;return n;}
    // Borrow supplied by the actual SDK command argument (or typed Copy hook),
    // BEFORE recording. QueryInterface never uses an untyped NGX resource ID.
    // At most five actual pending obligations plus this provisional call exist.
    bool retain_recording(StereoAdapter& adapter,ID3D12CommandList* list,uintptr_t id,uint64_t gen){
        std::array<PendingRecording,5> records{};unsigned n{};Ptr<IUnknown> identity;
        if(failed_||!list||!id||!gen||list->GetType()!=D3D12_COMMAND_LIST_TYPE_DIRECT||
           FAILED(list->QueryInterface(IID_PPV_ARGS(&identity)))||reinterpret_cast<uintptr_t>(identity.Get())!=id||
           !pending(adapter,records,n)||!collect()){fail(adapter);return false;}
        Member next;next.object=list;next.id=id;next.generation=gen;
        if(seen(next)){fail(adapter);return false;}
        for(unsigned i=0;i<n;++i)if(records[i].list==id&&records[i].generation!=gen){fail(adapter);return false;}
        prune(records,n);
        for(auto const& r:recordings_)if(r.id==id&&r.generation==gen)return true;
        for(auto& r:recordings_)if(!r.object){r=std::move(next);return true;}
        fail(adapter);return false;
    }
    // Only the hook AFTER successful native Reset may call this. A new tracker
    // generation without this event cannot bypass duplicate submission checks.
    // Reset does not imply GPU completion; old generation leases survive until
    // our real same-queue fence completes. Unsubmitted owned work cannot vanish.
    bool reset_success(StereoAdapter& adapter,ID3D12CommandList* list,uintptr_t id,uint64_t gen){
        if(failed_)return false;bool tracked{};
        for(auto const& r:recordings_)if(r.object&&r.id==id)tracked=true;
        for(auto const& s:seen_)if(reinterpret_cast<uintptr_t>(s.identity.Get())==id&&s.identity)tracked=true;
        if(!tracked)return true;
        Ptr<IUnknown> identity;std::array<PendingRecording,5> records{};unsigned n{};
        if(!list||!gen||FAILED(list->QueryInterface(IID_PPV_ARGS(&identity)))||reinterpret_cast<uintptr_t>(identity.Get())!=id||
           !pending(adapter,records,n)){fail(adapter);return false;}
        for(unsigned i=0;i<n;++i)if(records[i].list==id){fail(adapter);return false;}
        for(auto const& s:seen_)if(reinterpret_cast<uintptr_t>(s.identity.Get())==id&&s.identity&&s.generation>=gen){fail(adapter);return false;}
        for(auto& s:seen_)if(reinterpret_cast<uintptr_t>(s.identity.Get())==id&&s.identity)s.reset=true;
        for(auto& r:recordings_)if(r.object&&r.id==id)r={};
        if(!collect()){fail(adapter);return false;}return true;
    }
    Completion completion(){
        if(!collect()||!fence_||!value_)return {};const auto value=fence_->GetCompletedValue();
        if(value==UINT64_MAX||value>value_)return {};
        return {reinterpret_cast<uintptr_t>(device_id_.Get()),reinterpret_cast<uintptr_t>(queue_id_.Get()),reinterpret_cast<uintptr_t>(fence_id_.Get()),value};
    }
    // Always forwards the exact game Execute arguments once, including on
    // rejection. Signal and adapter receipts happen only AFTER that returns.
    bool execute(StereoAdapter& adapter,ID3D12CommandQueue* queue,UINT count,ID3D12CommandList* const* lists,
        Original original,Read read,Generation generation){
        std::array<PendingRecording,5> records{};unsigned n{};
        if(!pending(adapter,records,n)){original(queue,count,lists);return false;}
        Flight batch;bool valid=!failed_&&read&&generation&&collect();
        for(UINT i=0;valid&&i<count;++i){
            ID3D12CommandList* raw{};Ptr<IUnknown> identity;
            valid=lists&&read(&raw,lists+i,sizeof(raw))&&raw&&SUCCEEDED(raw->QueryInterface(IID_PPV_ARGS(&identity)));
            if(!valid)break;
            Member member;member.id=reinterpret_cast<uintptr_t>(identity.Get());member.generation=generation(raw,member.id);member.index=i;
            if(seen(member)){valid=false;break;}
            bool owned{};for(unsigned j=0;j<n;++j)if(member.id==records[j].list){
                owned=true;if(member.generation!=records[j].generation)valid=false;
            }
            if(!owned)continue;
            Ptr<ID3D12Device> device;Ptr<IUnknown> did;
            if(raw->GetType()!=D3D12_COMMAND_LIST_TYPE_DIRECT||FAILED(raw->GetDevice(IID_PPV_ARGS(&device)))||FAILED(device.As(&did)))valid=false;
            if(valid&&(!bind(queue)||did.Get()!=device_id_.Get()))valid=false;
            bool leased{};for(auto const& r:recordings_)if(r.object&&r.id==member.id&&r.generation==member.generation){member.object=r.object;leased=true;}
            if(!leased||batch.count==batch.members.size())valid=false;
            for(UINT j=0;j<batch.count;++j)if(batch.members[j].id==member.id)valid=false;
            if(valid)batch.members[batch.count++]=std::move(member);
        }
        Flight* retained{};
        if(valid&&batch.count){
            for(auto& flight:flights_)if(!flight.value&&!retained)retained=&flight;
            if(!retained||value_>=UINT64_MAX-1||order_==UINT64_MAX)valid=false;
            for(UINT i=0;valid&&i<batch.count;++i)if(!remember(batch.members[i],value_+1))valid=false;
            if(valid){batch.value=value_+1;*retained=std::move(batch);}
        }
        if(!valid)fail(adapter);
        try{original(queue,count,lists);}catch(...){fail(adapter);throw;}
        if(!valid)return false;if(!retained)return true;
        // A concurrent Reset must not certify a replacement recording.
        for(UINT i=0;i<retained->count;++i)if(generation(retained->members[i].object.Get(),retained->members[i].id)!=retained->members[i].generation){fail(adapter);return false;}
        ++value_;++order_;if(FAILED(queue_->Signal(fence_.Get(),value_))){fail(adapter);return false;}
        for(UINT member=0;member<retained->count;++member){
            auto const& m=retained->members[member];
            ExecuteReceipt receipt{{reinterpret_cast<uintptr_t>(device_id_.Get()),m.id,m.generation,reinterpret_cast<uintptr_t>(queue_id_.Get()),reinterpret_cast<uintptr_t>(fence_id_.Get()),value_,true},order_,m.index};
            // Same-list Eval may discharge Copy too; refresh after each receipt.
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
        if(!pending(adapter,records,n))return false;prune(records,n);return true;
    }
};
}
