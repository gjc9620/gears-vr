#pragma once
#include <array>
#include <cstdint>
#include <mutex>

// CPU-only inventory. Identities are canonical IUnknown addresses supplied by
// the caller. No ownership, GPU operations or inference from Execute/Present.
// The caller must invalidate on unobserved lifetime/resize transitions; raw COM
// addresses alone cannot prove lifetime uniqueness. Capacity faults are sticky.
namespace gears_swapchain_binding {
using Identity = uintptr_t;
struct Binding {Identity chain{}, queue{}, window{}; unsigned width{}, height{}, method{}; bool anchored{};};
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
    Comparison first_{},failure_{};
    std::mutex mutex_;
    Comparison compare(Identity chain,unsigned width,unsigned height,const Identity* resources,unsigned count,bool anchor) {
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
        if(stats_.rejected)detail.reason|=inventory_rejected;
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
                if (entry.queue != value.queue || entry.window != value.window) {++stats_.conflicts; stats_.rejected = true; return false;}
                return !stats_.rejected;
            }
            if (!entry.chain && !empty) empty = &entry;
        }
        if (!empty) {++stats_.overflow; stats_.rejected = true; return false;}
        *empty = value; ++stats_.accepted; return !stats_.rejected;
    }
    bool buffer(Identity chain, Identity resource, unsigned index) {
        std::lock_guard<std::mutex> lock(mutex_);
        bool found{}; for (const auto& entry : bindings_) if (entry.chain == chain) found = true;
        if (!found || !resource || stats_.rejected) return false;
        Buffer* empty{};
        for (auto& entry : buffers_) {
            if (entry.resource == resource) {
                if (entry.chain != chain || entry.index != index) {++stats_.conflicts; stats_.rejected = true; return false;}
                return true;
            }
            if (!entry.resource && !empty) empty = &entry;
        }
        if (!empty) {++stats_.overflow; stats_.rejected = true; return false;}
        *empty = {chain, resource, index}; ++stats_.buffers; return true;
    }
    bool unchanged(Identity chain, unsigned width, unsigned height, const Identity* resources, unsigned count) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto detail=compare(chain,width,height,resources,count,false);
        if(!first_.recorded)first_=detail;
        if(detail.reason){if(!failure_.recorded)failure_=detail;++stats_.changed;stats_.rejected=true;return false;}
        return true;
    }
    // One explicit trial-start boundary. Loading-time Resize may precede it.
    // This is never a refresh after trial starts; later differences are sticky.
    bool anchor(Identity chain,unsigned width,unsigned height,const Identity* resources,unsigned count) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto detail=compare(chain,width,height,resources,count,true);
        if(!first_.recorded)first_=detail;
        Binding* binding{};for(auto& entry:bindings_)if(entry.chain==chain&&chain)binding=&entry;
        if(!binding||binding->anchored||stats_.rejected||!width||!height||detail.reason&(invalid_input|missing_binding|inventory_rejected))return false;
        // Check available capacity before replacing the loading-time inventory.
        size_t available{};for(const auto& entry:buffers_)if(!entry.resource||entry.chain==chain)++available;
        if(count>available){++stats_.overflow;stats_.rejected=true;return false;}
        for(unsigned i=0;i<count;++i)for(const auto& entry:buffers_)
            if(entry.resource==resources[i]&&entry.chain!=chain){++stats_.conflicts;stats_.rejected=true;return false;}
        for(unsigned i=0;i<count;++i)for(unsigned j=0;j<i;++j)if(resources[i]==resources[j]){
            ++stats_.conflicts;stats_.rejected=true;return false;}
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
        if (!stats_.rejected) for (const auto& entry : bindings_) if (entry.chain == chain) return entry;
        return {};
    }
    bool known_buffer(Identity resource) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!stats_.rejected && resource) for (const auto& entry : buffers_) if (entry.resource == resource) return true;
        return false;
    }
    void invalidate() {std::lock_guard<std::mutex> lock(mutex_); stats_.rejected = true;}
    Snapshot snapshot() {std::lock_guard<std::mutex> lock(mutex_); return stats_;}
};
} // namespace gears_swapchain_binding
