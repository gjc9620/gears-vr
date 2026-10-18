#pragma once
#include <array>
#include <cstdint>
#include <mutex>
#include <limits>

// CPU-only inventory. Identities are canonical IUnknown addresses supplied by
// the caller. No ownership, GPU operations or inference from Execute/Present.
// The caller must invalidate on unobserved lifetime/resize transitions; raw COM
// addresses alone cannot prove lifetime uniqueness. Capacity faults are sticky.
namespace gears_swapchain_binding {
using Identity = uintptr_t;
struct Binding {Identity chain{}, queue{}, window{}; unsigned width{}, height{}, method{}; bool anchored{};
    uint64_t generation{1};bool resizing{},needs_revalidation{},rejected{};};
struct Inventory {Binding binding{};std::array<Identity,16> resources{};unsigned count{};bool valid{};};
enum Difference : unsigned {invalid_input=1, dimensions=2, buffer_count=4, buffer_identity=8, missing_binding=16, inventory_rejected=32};
struct Comparison {
    bool recorded{}, anchor{};
    Identity chain{};
    unsigned expected_width{},expected_height{},actual_width{},actual_height{},expected_count{},actual_count{},reason{};
    std::array<Identity,16> expected{},actual{};
};
struct Snapshot {uint64_t accepted{}, excluded{}, conflicts{}, overflow{}, buffers{}, changed{}, anchors{}; bool rejected{};};
class Observer {
    struct Buffer {Identity chain{}, resource{}; unsigned index{};};
    std::array<Binding, 8> bindings_{};
    std::array<Buffer, 32> buffers_{};
    Snapshot stats_{};
    bool hard_rejected_{};
    Comparison first_{},failure_{};
    std::mutex mutex_;
    Binding* binding(Identity chain){for(auto& entry:bindings_)if(chain&&entry.chain==chain)return &entry;return nullptr;}
    bool admitted(Binding const& entry)const{return !hard_rejected_&&!entry.rejected&&!entry.resizing&&!entry.needs_revalidation;}
    void refresh(){stats_.rejected=hard_rejected_;stats_.buffers=0;
        for(auto const& entry:bindings_)stats_.rejected=stats_.rejected||entry.rejected;
        for(auto const& entry:buffers_)if(entry.resource)++stats_.buffers;}
    bool hard_conflict(){++stats_.conflicts;hard_rejected_=true;refresh();return false;}
    bool hard_capacity(){++stats_.overflow;hard_rejected_=true;refresh();return false;}
    bool inventory_input(Binding& owner,unsigned width,unsigned height,const Identity* resources,unsigned count){
        if(count>16)return hard_capacity();
        if(!width||!height||!resources||!count)return false;
        size_t available{};for(auto const& entry:buffers_)if(!entry.resource||entry.chain==owner.chain)++available;
        if(count>available)return hard_capacity();
        for(unsigned i=0;i<count;++i){
            if(!resources[i])return false;
            for(unsigned j=0;j<i;++j)if(resources[i]==resources[j])return hard_conflict();
            for(auto const& entry:buffers_)if(entry.resource==resources[i]&&entry.chain!=owner.chain)return hard_conflict();
        }
        return true;
    }
    void replace_inventory(Binding& owner,unsigned width,unsigned height,const Identity* resources,unsigned count){
        for(auto& entry:buffers_)if(entry.chain==owner.chain)entry={};
        for(unsigned i=0;i<count;++i)for(auto& entry:buffers_)if(!entry.resource){entry={owner.chain,resources[i],i};break;}
        owner.width=width;owner.height=height;owner.rejected=false;owner.needs_revalidation=false;refresh();
    }
    Comparison compare(Identity chain,unsigned width,unsigned height,const Identity* resources,unsigned count,bool anchor,bool revalidate=false) {
        Comparison detail{};detail.recorded=true;detail.anchor=anchor;detail.chain=chain;
        detail.actual_width=width;detail.actual_height=height;detail.actual_count=count;
        if(!resources||!count||count>detail.actual.size())detail.reason|=invalid_input;
        if(resources)for(unsigned i=0;i<count&&i<detail.actual.size();++i){detail.actual[i]=resources[i];if(!resources[i])detail.reason|=invalid_input;}
        bool found{};
        for(const auto& entry:bindings_)if(entry.chain==chain&&chain){found=true;detail.expected_width=entry.width;detail.expected_height=entry.height;}
        if(!found)detail.reason|=missing_binding;
        if(detail.expected_width!=width||detail.expected_height!=height)detail.reason|=dimensions;
        for(const auto& entry:buffers_)if(entry.chain==chain&&chain){++detail.expected_count;
            if(entry.index<detail.expected.size())detail.expected[entry.index]=entry.resource;else detail.reason|=invalid_input;}
        if(detail.expected_count!=count)detail.reason|=buffer_count;
        for(unsigned i=0;i<detail.expected.size();++i)if(detail.expected[i]!=detail.actual[i])detail.reason|=buffer_identity;
        auto owner=binding(chain);
        if(hard_rejected_||(owner&&(owner->resizing||(!revalidate&&(owner->rejected||owner->needs_revalidation)))))detail.reason|=inventory_rejected;
        return detail;
    }
public:
    bool bind(Binding value, bool same_process, bool direct_queue) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!value.chain || !value.queue || !value.window || !same_process || !direct_queue || value.width < 640) {
            ++stats_.excluded; return false;
        }
        Binding* empty{};
        for (auto& entry : bindings_) {
            if (entry.chain == value.chain) {
                if (entry.queue != value.queue || entry.window != value.window)return hard_conflict();
                return admitted(entry);
            }
            if (!entry.chain && !empty) empty = &entry;
        }
        if (!empty)return hard_capacity();
        if(hard_rejected_)return false;
        value.generation=1;value.resizing=value.needs_revalidation=value.rejected=false;
        *empty = value; ++stats_.accepted; return true;
    }
    bool buffer(Identity chain, Identity resource, unsigned index) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto owner=binding(chain);
        if (!owner || !resource || !admitted(*owner)) return false;
        if(index>=16)return hard_capacity();
        Buffer* empty{};
        for (auto& entry : buffers_) {
            if (entry.resource == resource) {
                if (entry.chain != chain || entry.index != index)return hard_conflict();
                return true;
            }
            if(entry.resource&&entry.chain==chain&&entry.index==index)return hard_conflict();
            if (!entry.resource && !empty) empty = &entry;
        }
        if (!empty)return hard_capacity();
        *empty = {chain, resource, index}; ++stats_.buffers; return true;
    }
    bool unchanged(Identity chain, unsigned width, unsigned height, const Identity* resources, unsigned count) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto owner=binding(chain);
        if(owner&&owner->resizing)return false;
        auto detail=compare(chain,width,height,resources,count,false,owner&&owner->needs_revalidation);
        if(!first_.recorded)first_=detail;
        if(detail.reason){if(!failure_.recorded)failure_=detail;++stats_.changed;
            if(owner)owner->rejected=true;else hard_rejected_=true;refresh();return false;}
        if(owner&&owner->needs_revalidation){owner->needs_revalidation=false;owner->rejected=false;refresh();}
        return true;
    }
    // Only the caller's actual ResizeBuffers/ResizeBuffers1 boundary may use
    // these methods. Generation changes immediately, including failed resizes.
    // Existing queue/window certification is retained, never inferred anew.
    bool begin_resize(Identity chain){
        std::lock_guard<std::mutex> lock(mutex_);auto owner=binding(chain);
        if(!owner||hard_rejected_)return false;
        if(owner->resizing)return hard_conflict();
        if(owner->generation==(std::numeric_limits<uint64_t>::max)())return hard_capacity();
        ++owner->generation;owner->resizing=true;return true;
    }
    bool complete_resize(Identity chain,unsigned width,unsigned height,const Identity* resources,unsigned count,bool success){
        std::lock_guard<std::mutex> lock(mutex_);auto owner=binding(chain);
        if(hard_rejected_)return false;
        if(!owner||!owner->resizing)return hard_conflict();
        owner->resizing=false;
        if(!success){
            // A failed API call does not authorize a changed inventory. Keep
            // the old one blocked until it is fully compared, now or at Present.
            owner->needs_revalidation=true;
            auto detail=compare(chain,width,height,resources,count,false,true);
            if(detail.reason)return false;
            owner->needs_revalidation=false;owner->rejected=false;refresh();return true;
        }
        if(!inventory_input(*owner,width,height,resources,count)){owner->rejected=true;refresh();return false;}
        replace_inventory(*owner,width,height,resources,count);return true;
    }
    // One explicit trial-start boundary. Loading-time Resize may precede it.
    // This is never a refresh after trial starts; later differences are sticky.
    bool anchor(Identity chain,unsigned width,unsigned height,const Identity* resources,unsigned count) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto detail=compare(chain,width,height,resources,count,true);
        if(!first_.recorded)first_=detail;
        Binding* binding{};for(auto& entry:bindings_)if(entry.chain==chain&&chain)binding=&entry;
        if(!binding||binding->anchored||!admitted(*binding)||!width||!height||detail.reason&(invalid_input|missing_binding|inventory_rejected))return false;
        // Check available capacity before replacing the loading-time inventory.
        size_t available{};for(const auto& entry:buffers_)if(!entry.resource||entry.chain==chain)++available;
        if(count>available)return hard_capacity();
        for(unsigned i=0;i<count;++i)for(const auto& entry:buffers_)
            if(entry.resource==resources[i]&&entry.chain!=chain)return hard_conflict();
        for(unsigned i=0;i<count;++i)for(unsigned j=0;j<i;++j)if(resources[i]==resources[j]){
            return hard_conflict();}
        for(auto& entry:buffers_)if(entry.chain==chain)entry={};
        for(unsigned i=0;i<count;++i)for(auto& entry:buffers_)if(!entry.resource){entry={chain,resources[i],i};break;}
        stats_.buffers=0;for(const auto& entry:buffers_)if(entry.resource)++stats_.buffers;
        binding->width=width;binding->height=height;binding->anchored=true;++stats_.anchors;
        return true;
    }
    Binding registered(Identity chain) {
        std::lock_guard<std::mutex> lock(mutex_);
        for(const auto& entry:bindings_)if(entry.chain==chain&&chain)return entry;return {};
    }
    Comparison first_comparison(){std::lock_guard<std::mutex> lock(mutex_);return first_;}
    Comparison first_failure(){std::lock_guard<std::mutex> lock(mutex_);return failure_;}
    Binding lookup(Identity chain) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& entry : bindings_) if (entry.chain == chain&&chain&&admitted(entry)) return entry;
        return {};
    }
    // Read-only current-generation inventory, copied in one lock scope. This
    // does not refresh, revalidate or admit an earlier Present observation.
    Inventory inventory(Identity chain){
        std::lock_guard<std::mutex> lock(mutex_);Inventory out{};
        for(auto const& entry:bindings_)if(chain&&entry.chain==chain&&entry.anchored&&admitted(entry))out.binding=entry;
        if(!out.binding.chain)return {};
        unsigned mask{};
        for(auto const& entry:buffers_)if(entry.chain==chain){
            if(entry.index>=out.resources.size()||!entry.resource||(mask&(1u<<entry.index)))return {};
            mask|=1u<<entry.index;out.resources[entry.index]=entry.resource;++out.count;
        }
        out.valid=out.count&&out.count<=out.resources.size()&&mask==((1u<<out.count)-1u);
        return out.valid?out:Inventory{};
    }
    bool known_buffer(Identity resource) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (resource) for (const auto& entry : buffers_) if (entry.resource == resource){auto owner=binding(entry.chain);return owner&&admitted(*owner);}
        return false;
    }
    void invalidate() {std::lock_guard<std::mutex> lock(mutex_); hard_rejected_=true;refresh();}
    Snapshot snapshot() {std::lock_guard<std::mutex> lock(mutex_); return stats_;}
};
} // namespace gears_swapchain_binding
