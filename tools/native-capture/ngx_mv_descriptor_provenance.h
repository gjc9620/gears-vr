#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <utility>
#include "ngx_typed_resources.h"

namespace gears_ngx_mv {
// CPU descriptor provenance only. No producer, shader, GPU state/completion or
// logical-eye authorization. Caller serializes this bank and drains callbacks
// before destroying it. Snapshots retain the actual observed heap/resources.
struct HeapLease {
    uintptr_t raw{},canonical{},device{};uint64_t cpu{},gpu{};uint32_t count{},stride{};bool visible{};
private:
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap_;
    Microsoft::WRL::ComPtr<IUnknown> identity_,device_identity_;
    Microsoft::WRL::ComPtr<ID3D12Device> device_;
public:
    static HeapLease observe(ID3D12DescriptorHeap* heap){
        HeapLease out;out.raw=reinterpret_cast<uintptr_t>(heap);if(!heap)return out;
        out.heap_=heap;if(FAILED(heap->QueryInterface(IID_PPV_ARGS(&out.identity_)))||
           FAILED(heap->GetDevice(IID_PPV_ARGS(&out.device_)))||FAILED(out.device_.As(&out.device_identity_)))return out;
        const auto d=heap->GetDesc();if(d.Type!=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)return out;
        out.canonical=reinterpret_cast<uintptr_t>(out.identity_.Get());out.device=reinterpret_cast<uintptr_t>(out.device_identity_.Get());
        out.cpu=heap->GetCPUDescriptorHandleForHeapStart().ptr;out.count=d.NumDescriptors;
        out.stride=out.device_->GetDescriptorHandleIncrementSize(d.Type);out.visible=(d.Flags&D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)!=0;
        if(out.visible)out.gpu=heap->GetGPUDescriptorHandleForHeapStart().ptr;return out;
    }
    bool retained()const{return heap_&&identity_&&device_&&device_identity_&&
        raw==reinterpret_cast<uintptr_t>(heap_.Get())&&canonical==reinterpret_cast<uintptr_t>(identity_.Get())&&
        device==reinterpret_cast<uintptr_t>(device_identity_.Get())&&cpu&&count&&stride&&(!visible||gpu);}
};
enum class ScopeKind {selected_ticket,device_lifetime};
// In device_lifetime mode epoch is the capture generation, NOT an NGX Create
// epoch. Unknown world/ticket stay zero. A later Eval must carry its own use
// identity separately; it cannot relabel these original creation records.
struct Scope {uint64_t epoch{},world{},ticket{};uintptr_t device{};ScopeKind kind=ScopeKind::selected_ticket;};
inline Scope lifetime_scope(uint64_t capture_generation,uintptr_t device){
    return {capture_generation,0,0,device,ScopeKind::device_lifetime};
}
inline bool same(Scope a,Scope b){return a.kind==b.kind&&a.epoch==b.epoch&&a.world==b.world&&a.ticket==b.ticket&&a.device==b.device;}
inline bool valid(Scope s){return s.epoch&&s.device&&(
    (s.kind==ScopeKind::selected_ticket&&s.world&&s.ticket)||
    (s.kind==ScopeKind::device_lifetime&&!s.world&&!s.ticket));}
enum class Loss {none,bad_scope,unreadable,capacity,invalid_heap,overlap,unknown_heap,bad_range,device_mismatch};
// Hook must guarded-copy the original arrays before forwarding. Null native
// sizes means one descriptor per range; no unowned pointer enters this bank.
template<size_t N=128> struct Ranges {
    unsigned count{};bool readable=true,sizes_present{};
    std::array<uint64_t,N> starts{};std::array<unsigned,N> sizes{};
};
struct ViewDescription {bool readable=true,provided{};D3D12_UNORDERED_ACCESS_VIEW_DESC value{};};
// Audit of the source observation used to seed another bounded bank. Local
// target versions are never relabeled as source history. Copies share this
// immutable evidence; ordinary native writes clear it with the old cell.
struct SeedEvidence {
    Scope scope{};std::shared_ptr<unsigned const> owner;
    uint64_t version{},origin{},source_handle{},source_version{};
};

template<class ResourceLease=gears_ngx_stereo::TypedResourceLease,class DescriptorHeapLease=HeapLease,
         size_t HeapCapacity=8,size_t CellCapacity=128,bool ReuseNonUav=false,size_t CopyCapacity=CellCapacity,bool SourceLedger=false>
class DescriptorBank {
    static_assert(HeapCapacity&&CellCapacity&&CopyCapacity&&CopyCapacity<=CellCapacity,"nonempty bounded observation/copy limits");
    struct Cell {uintptr_t heap{};uint32_t index{};uint64_t version{},origin{},source_handle{},source_version{};
        ResourceLease resource{},counter{};ViewDescription view{};bool known_uav{};
        std::shared_ptr<SeedEvidence const> seed;};
    Scope scope_{};Loss loss_{};bool begun_{};uint64_t serial_{};size_t heap_count_{},cell_count_{};
    uint64_t cell_reuses_{};
    std::array<DescriptorHeapLease,HeapCapacity> heaps_{};std::array<Cell,CellCapacity> cells_{};
    // Snapshot owner identity survives bank destruction, preventing bank-address ABA.
    std::shared_ptr<unsigned> owner_{std::make_shared<unsigned>(0)};
    bool lose(Loss value){if(loss_==Loss::none)loss_=value;return false;}
    bool scope(Scope value){return begun_&&loss_==Loss::none&&same(scope_,value)?true:lose(Loss::bad_scope);}
    static bool span(uint64_t start,uint32_t count,uint32_t stride,uint64_t& last){
        if(!start||!count||!stride||uint64_t(count-1)>(UINT64_MAX-start)/stride)return false;
        const auto final_start=start+uint64_t(count-1)*stride;
        if(uint64_t(stride-1)>UINT64_MAX-final_start)return false;
        last=final_start+stride-1;return true; // Full inclusive byte extent, not only descriptor starts.
    }
    static bool index(uint64_t address,uint64_t start,uint32_t count,uint32_t stride,uint32_t& out){
        if(!start||!stride||address<start)return false;const auto delta=address-start;
        if(delta%stride||delta/stride>=count)return false;out=uint32_t(delta/stride);return true;
    }
    DescriptorHeapLease const* heap(uint64_t handle,bool gpu,uint32_t& slot)const{
        for(size_t i=0;i<heap_count_;++i){auto const& h=heaps_[i];if((!gpu||h.visible)&&index(handle,gpu?h.gpu:h.cpu,h.count,h.stride,slot))return &h;}return nullptr;
    }
    Cell const* find(uintptr_t id,uint32_t slot)const{for(size_t i=0;i<cell_count_;++i)if(cells_[i].heap==id&&cells_[i].index==slot)return &cells_[i];return nullptr;}
    Cell* destination(uintptr_t id,uint32_t slot){
        for(size_t i=0;i<cell_count_;++i)if(cells_[i].heap==id&&cells_[i].index==slot)return &cells_[i];
        if(cell_count_==CellCapacity){
            if constexpr(ReuseNonUav){for(auto& c:cells_)if(!c.known_uav){
                // Only a known-erased/non-UAV cell is reusable. No retained
                // UAV may be evicted. Global serials never restart, so neither
                // same-address recreation nor slot reuse revives old snapshots.
                c={};c.heap=id;c.index=slot;++cell_reuses_;return &c;}}
            capacity_loss(CapacitySite::cells);return nullptr;
        }auto& c=cells_[cell_count_++];c.heap=id;c.index=slot;return &c;
    }
    bool flatten(Ranges<CopyCapacity> const& ranges,bool source,std::array<uint64_t,CopyCapacity>& handles,unsigned& total){
        total=0;if(!ranges.readable)return lose(Loss::unreadable);if(ranges.count>CopyCapacity)return capacity_loss(CapacitySite::ranges);
        for(unsigned r=0;r<ranges.count;++r){const unsigned n=ranges.sizes_present?ranges.sizes[r]:1;if(!n)continue;
            uint32_t slot{};auto* h=heap(ranges.starts[r],false,slot);if(!h)return lose(Loss::unknown_heap);
            if(source&&h->visible)return lose(Loss::bad_range); // D3D source descriptors cannot be in shader-visible heaps.
            if(n>h->count-slot||n>CopyCapacity-total)return lose(Loss::bad_range);
            for(unsigned j=0;j<n;++j)handles[total++]=h->cpu+uint64_t(slot+j)*h->stride;
        }return true;
    }
public:
    static constexpr size_t heap_capacity=HeapCapacity,cell_capacity=CellCapacity,copy_capacity=CopyCapacity;
    using CopyRanges=Ranges<CopyCapacity>;
    // Copy scratch stays independent of persistent cell storage. The caller
    // serializes callbacks; sources are all captured before any destination.
    static constexpr size_t copy_scratch_bytes=sizeof(std::array<Cell,CopyCapacity>)+2*sizeof(std::array<uint64_t,CopyCapacity>);
    static constexpr bool reuse_non_uav=ReuseNonUav;
    static constexpr bool source_ledger=SourceLedger;
    enum class CapacitySite:unsigned {none,heaps,cells,ranges,serial};
    struct CellCounts {size_t known_uav{},non_uav{},unused{};uint64_t reuses{};};
    struct Saturation {CapacitySite site{};size_t heaps{};CellCounts cells{};};
private:
    Saturation saturation_{};
    bool capacity_loss(CapacitySite site){
        if(loss_==Loss::none)saturation_={site,heap_count_,cell_counts()};
        return lose(Loss::capacity);
    }
public:
    struct Snapshot {
        Scope scope{};std::shared_ptr<unsigned const> owner;
        DescriptorHeapLease heap{};ResourceLease resource{},counter{};ViewDescription view{};
        uint64_t cpu{},gpu{},version{},origin{},source_handle{},source_version{};bool known_uav{};
        std::shared_ptr<SeedEvidence const> seed;
        bool retained_uav()const{return known_uav&&heap.retained()&&resource.retained()&&version&&origin;}
        // Comparison ONLY with an already retained typed quartet MV lease.
        bool same_mv(ResourceLease const& mv)const{return retained_uav()&&mv.retained()&&
            resource.canonical==mv.canonical&&resource.device==mv.device;}
        static constexpr bool producer_authorized=false;
    };
    DescriptorBank()=default;DescriptorBank(DescriptorBank const&)=delete;DescriptorBank& operator=(DescriptorBank const&)=delete;
    bool begin(Scope value){if(begun_||!valid(value))return lose(Loss::bad_scope);scope_=value;begun_=true;return true;}
    Loss loss()const{return loss_;}size_t heaps()const{return heap_count_;}size_t cells()const{return cell_count_;}
    CellCounts cell_counts()const{CellCounts out;out.unused=CellCapacity-cell_count_;out.reuses=cell_reuses_;
        for(size_t i=0;i<cell_count_;++i)if(cells_[i].known_uav)++out.known_uav;else ++out.non_uav;return out;}
    Saturation saturation()const{return saturation_;}
    bool failed()const{return loss_!=Loss::none;}
    bool contains_cpu(uint64_t handle)const{uint32_t slot{};return begun_&&!failed()&&heap(handle,false,slot)!=nullptr;}
    void uncertain(){lose(Loss::unreadable);}
    bool observe_heap(Scope value,ID3D12DescriptorHeap* typed){
        if(!scope(value))return false;return observe_heap_lease(value,DescriptorHeapLease::observe(typed));
    }
    // Import ownership, never reconstruct an interface from a stored address.
    // Uses the same validation as a direct typed heap boundary.
    bool observe_heap_lease(Scope value,DescriptorHeapLease const& h){
        if(!scope(value))return false;
        if(!h.retained())return lose(Loss::invalid_heap);if(h.device!=scope_.device)return lose(Loss::device_mismatch);
        for(size_t i=0;i<heap_count_;++i)if(heaps_[i].canonical==h.canonical){auto const& old=heaps_[i];
            return old.cpu==h.cpu&&old.gpu==h.gpu&&old.count==h.count&&old.stride==h.stride&&old.visible==h.visible?true:lose(Loss::invalid_heap);}
        uint64_t end{},gpu_end{};if(!span(h.cpu,h.count,h.stride,end)||(h.visible&&!span(h.gpu,h.count,h.stride,gpu_end)))return lose(Loss::invalid_heap);
        for(size_t i=0;i<heap_count_;++i){auto const& old=heaps_[i];uint64_t last{},last_gpu{};span(old.cpu,old.count,old.stride,last);
            if(h.cpu<=last&&old.cpu<=end)return lose(Loss::overlap);
            if(h.visible&&old.visible){span(old.gpu,old.count,old.stride,last_gpu);if(h.gpu<=last_gpu&&old.gpu<=gpu_end)return lose(Loss::overlap);}}
        if(heap_count_==HeapCapacity)return capacity_loss(CapacitySite::heaps);heaps_[heap_count_++]=h;return true;
    }
    // Caller holds the common ordering lock across heap import and seeding.
    // A failed source cannot donate any lease, even if its historical snapshots
    // still own valid COM objects. Visitor returns false to stop a failed import.
    template<class Visit> bool visit_heap_leases(Scope value,Visit visit)const{
        if(!begun_||failed()||!same(scope_,value))return false;
        for(size_t i=0;i<heap_count_;++i)if(!visit(heaps_[i]))return false;return true;
    }
    // Requires the source heap to have been imported. Import all source heaps
    // first so later native alias copies cannot resolve through missing history.
    template<class Source> bool seed_current_mv(Scope value,Source const& source,typename Source::Snapshot const& captured,ResourceLease const& selected_mv){
        if(!scope(value))return false;
        if(!source.current(captured)||!captured.same_mv(selected_mv))return lose(Loss::unreadable);
        // Snapshot fields are public diagnostic values. Re-read the current
        // source cell rather than copying edited resource/view/heap fields.
        const auto live=source.lookup(captured.scope,captured.cpu);
        if(!live.same_mv(selected_mv)||!live.view.readable||live.owner==owner_)return lose(Loss::unreadable);
        if(captured.gpu!=live.gpu)return lose(Loss::bad_range);
        const bool counter_present=live.counter.raw||live.counter.canonical||live.counter.device;
        if(counter_present&&!live.counter.retained())return lose(Loss::unreadable);
        if(live.scope.device!=scope_.device||live.heap.device!=scope_.device||live.resource.device!=scope_.device||
           (counter_present&&live.counter.device!=scope_.device))return lose(Loss::device_mismatch);
        uint32_t slot{};const auto* h=heap(live.cpu,false,slot);
        if(!h||h->canonical!=live.heap.canonical)return lose(Loss::unknown_heap);
        if(h->cpu!=live.heap.cpu||h->gpu!=live.heap.gpu||h->count!=live.heap.count||h->stride!=live.heap.stride||h->visible!=live.heap.visible||
           live.gpu!=(h->visible?h->gpu+uint64_t(slot)*h->stride:0))return lose(Loss::invalid_heap);
        if(serial_==UINT64_MAX)return capacity_loss(CapacitySite::serial);
        std::shared_ptr<SeedEvidence const> evidence;
        try{evidence=std::make_shared<SeedEvidence const>(SeedEvidence{live.scope,live.owner,live.version,live.origin,live.source_handle,live.source_version});}
        catch(...){return lose(Loss::unreadable);}
        auto* c=destination(h->canonical,slot);if(!c)return false;
        *c={h->canonical,slot,++serial_};c->origin=c->version;c->resource=live.resource;c->counter=live.counter;
        c->view=live.view;c->known_uav=true;c->seed=std::move(evidence);return true;
    }
    // Call after the original typed CreateUAV returned, under the same ordering
    // lock as every descriptor writer. Desc is a guarded pre-call value copy.
    bool write_uav(Scope value,uint64_t cpu,ID3D12Resource* typed,ID3D12Resource* counter,ViewDescription description){
        if(!scope(value))return false;uint32_t slot{};auto* h=heap(cpu,false,slot);if(!h)return lose(Loss::unknown_heap);
        if(!description.readable)return lose(Loss::unreadable);auto* c=destination(h->canonical,slot);if(!c)return false;
        if(serial_==UINT64_MAX)return capacity_loss(CapacitySite::serial);*c={h->canonical,slot,++serial_};
        if(!typed)return true; // Null UAV descriptor: explicit unknown/nonmatching record, old provenance erased.
        auto resource=ResourceLease::observe(typed);ResourceLease counter_lease{};if(counter)counter_lease=ResourceLease::observe(counter);
        if(!resource.retained()||(counter&&!counter_lease.retained()))return lose(Loss::unreadable);
        if(resource.device!=scope_.device||(counter&&counter_lease.device!=scope_.device))return lose(Loss::device_mismatch);
        c->resource=std::move(resource);c->counter=std::move(counter_lease);c->view=description;c->origin=c->version;c->known_uav=true;return true;
    }
    // SRV/CBV, unsupported typed view and other known destination writes MUST
    // erase a previous UAV. This is not a missing-event recovery mechanism.
    bool unknown_write(Scope value,uint64_t cpu){return write_uav(value,cpu,nullptr,nullptr,{});}
    // Early SRV/CBV observation must invalidate a tracked UAV without filling
    // the bounded bank with unrelated descriptors. A heap we never observed
    // cannot contain any tracked cell; it remains unknown, never reconstructed.
    bool invalidate_if_tracked(Scope value,uint64_t cpu){
        if(!scope(value))return false;uint32_t slot{};auto* h=heap(cpu,false,slot);
        if(!h||!find(h->canonical,slot))return true;
        return unknown_write(value,cpu);
    }
private:
    bool copy_impl(Scope value,CopyRanges const& destinations,CopyRanges const& sources,bool sparse){
        if(!scope(value))return false;std::array<uint64_t,CopyCapacity> dst{},src{};unsigned dn{},sn{};
        if(!flatten(destinations,false,dst,dn)||!flatten(sources,true,src,sn))return false;if(dn!=sn)return lose(Loss::bad_range);
        // Snapshot all sources before any destination write. Reject unsupported
        // overlapping copies rather than impose a guessed native write order.
        std::array<Cell,CopyCapacity> captured{};
        for(unsigned i=0;i<dn;++i){for(unsigned j=0;j<dn;++j)if(dst[i]==src[j]||(j<i&&dst[i]==dst[j]))return lose(Loss::overlap);
            uint32_t slot{};auto* h=heap(src[i],false,slot);if(auto* c=find(h->canonical,slot))captured[i]=*c;}
        for(unsigned i=0;i<dn;++i){uint32_t slot{};auto* h=heap(dst[i],false,slot);
            const auto* tracked=find(h->canonical,slot);
            // A source ledger retains direct CreateUAVs (including visible
            // births) and CPU aliases, never ordinary visible copy fanout.
            // Existing tracked visible destinations still update or erase.
            // Every range/source/overlap check above also covers skipped rows.
            if constexpr(SourceLedger){if(!tracked&&(h->visible||!captured[i].known_uav))continue;}
            if(sparse&&!captured[i].known_uav&&!tracked)continue;
            auto* c=destination(h->canonical,slot);if(!c)return false;
            if(serial_==UINT64_MAX)return capacity_loss(CapacitySite::serial);auto value_copy=std::move(captured[i]);value_copy.heap=h->canonical;value_copy.index=slot;
            value_copy.source_handle=src[i];value_copy.source_version=value_copy.version;value_copy.version=++serial_;*c=std::move(value_copy);}
        return true;
    }
public:
    bool copy(Scope value,CopyRanges const& destinations,CopyRanges const& sources){return copy_impl(value,destinations,sources,false);}
    // Early observation: propagate known UAVs, erase tracked destinations on
    // unknown sources, but do not allocate cells for unrelated unknown copies.
    // All range/heap/overlap validation still applies, including skipped cells.
    bool copy_sparse(Scope value,CopyRanges const& destinations,CopyRanges const& sources){return copy_impl(value,destinations,sources,true);}
    bool copy_simple(Scope value,uint64_t dst,uint64_t src,unsigned count){CopyRanges d{},s{};
        d.count=s.count=1;d.sizes_present=s.sizes_present=true;d.starts[0]=dst;s.starts[0]=src;d.sizes[0]=s.sizes[0]=count;return copy(value,d,s);}
    bool copy_simple_sparse(Scope value,uint64_t dst,uint64_t src,unsigned count){CopyRanges d{},s{};
        d.count=s.count=1;d.sizes_present=s.sizes_present=true;d.starts[0]=dst;s.starts[0]=src;d.sizes[0]=s.sizes[0]=count;return copy_sparse(value,d,s);}
    Snapshot lookup(Scope value,uint64_t handle,bool gpu=false)const{
        Snapshot out;if(!begun_||failed()||!same(scope_,value))return out;uint32_t slot{};auto* h=heap(handle,gpu,slot);if(!h)return out;
        auto* c=find(h->canonical,slot);if(!c)return out;out.scope=scope_;out.owner=owner_;out.heap=*h;out.resource=c->resource;out.counter=c->counter;out.view=c->view;
        out.cpu=h->cpu+uint64_t(slot)*h->stride;out.gpu=h->visible?h->gpu+uint64_t(slot)*h->stride:0;out.version=c->version;out.origin=c->origin;
        out.source_handle=c->source_handle;out.source_version=c->source_version;out.known_uav=c->known_uav;out.seed=c->seed;return out;
    }
    bool current(Snapshot const& captured)const{auto live=lookup(captured.scope,captured.cpu);
        return captured.owner&&captured.owner==owner_&&captured.version&&live.version==captured.version&&
            live.heap.canonical==captured.heap.canonical;}
    // Diagnostic join at one serialized CPU observation point. Only an already
    // retained typed MV lease may select cells; an NGX integer is never a COM
    // argument. Visits are bounded by CellCapacity and include no unknown UAVs.
    template<class Visit> size_t visit_mv(Scope value,ResourceLease const& mv,Visit visit)const{
        if(!begun_||failed()||!same(scope_,value)||!mv.retained()||mv.device!=scope_.device)return 0;
        size_t count{};
        for(size_t i=0;i<cell_count_;++i){auto const& c=cells_[i];
            if(!c.known_uav||!c.resource.retained()||c.resource.canonical!=mv.canonical||c.resource.device!=mv.device)continue;
            for(size_t j=0;j<heap_count_;++j)if(heaps_[j].canonical==c.heap){
                auto captured=lookup(value,heaps_[j].cpu+uint64_t(c.index)*heaps_[j].stride);
                if(captured.same_mv(mv)){visit(captured);++count;}break;
            }
        }return count;
    }
};
// Production observation has a separate explicit bound: the first real census
// saw 60 cumulative CBV/SRV/UAV creations. Generic/small fixtures retain the
// conservative default of 8. Production may reuse non-UAV tombstones; retained
// known UAVs are never evicted, and a capacity failure never recovers/restarts.
// The next census saturated at 128 known UAV cells after eight safe tombstone
// reuses. Increase persistent storage to 512, retaining the old 128-descriptor
// per-copy bound so hooks do not quadruple their source-snapshot stack scratch.
inline constexpr size_t observer_heap_capacity=96,observer_cell_capacity=512,observer_copy_capacity=128;
template<class ResourceLease=gears_ngx_stereo::TypedResourceLease,class DescriptorHeapLease=HeapLease>
using ObserverDescriptorBank=DescriptorBank<ResourceLease,DescriptorHeapLease,observer_heap_capacity,observer_cell_capacity,true,observer_copy_capacity>;
// Independent observation scope; never repairs or restarts a failed whole-
// device bank. Original direct UAV cells and nonvisible aliases only, no eviction.
template<class ResourceLease=gears_ngx_stereo::TypedResourceLease,class DescriptorHeapLease=HeapLease>
using SourceDescriptorLedger=DescriptorBank<ResourceLease,DescriptorHeapLease,96,128,false,128,true>;
// Core storage for a later selected-MV observation scope. Validated heap import
// and resource seeding stay separate; no hook or admission policy is attached.
template<class ResourceLease=gears_ngx_stereo::TypedResourceLease,class DescriptorHeapLease=HeapLease>
using SelectedMvDescriptorBank=DescriptorBank<ResourceLease,DescriptorHeapLease,96,128,true,128>;
}
