#pragma once
#include <array>
#include <cstdint>

namespace gears_ngx_mv_census {
// Scalar, all-device cumulative creation counts. Not live heaps or VRAM usage.
// No COM ownership, descriptor provenance, pair validation or GPU authorization.
struct Category {uint64_t creations{},descriptors{},max_descriptors{};};
struct Counts {
    uint64_t calls{},creations{},failed{},descriptor_unreadable{},output_unreadable{},non_creation{},other_type{};
    Category visible{},nonvisible{};bool overflow{};
};
inline void add(uint64_t& value,uint64_t amount,bool& overflow){
    if(amount>UINT64_MAX-value){value=UINT64_MAX;overflow=true;}else value+=amount;
}
struct Event {
    bool descriptor_readable{},cbv_srv_uav{},failed{},output_readable=true,created{},visible{};
    uint32_t descriptors{};
};
inline void accumulate(Counts& c,Event e){
    add(c.calls,1,c.overflow);
    if(e.failed)add(c.failed,1,c.overflow);
    if(!e.descriptor_readable){add(c.descriptor_unreadable,1,c.overflow);return;}
    if(!e.cbv_srv_uav){add(c.other_type,1,c.overflow);return;}
    if(e.failed)return;
    if(!e.output_readable){add(c.output_unreadable,1,c.overflow);return;}
    if(!e.created){add(c.non_creation,1,c.overflow);return;}
    add(c.creations,1,c.overflow);auto& category=e.visible?c.visible:c.nonvisible;
    add(category.creations,1,c.overflow);add(category.descriptors,e.descriptors,c.overflow);
    if(e.descriptors>category.max_descriptors)category.max_descriptors=e.descriptors;
}
struct Use {uint64_t revision{},world{},ticket{};uintptr_t renderer{},handle{},parameters{};};
inline bool valid(Use u){return u.revision&&u.world&&u.ticket&&u.renderer&&u.handle&&u.parameters;}
inline bool same(Use a,Use b){return a.revision==b.revision&&a.world==b.world&&a.ticket==b.ticket&&
    a.renderer==b.renderer&&a.handle==b.handle&&a.parameters==b.parameters;}
struct Snapshot {Counts counts{};Use use{};bool present{};};
class Census {
    Counts counts_{};std::array<Snapshot,2> snapshots_{};bool armed_{},active_{};
public:
    void arm(){if(!armed_){armed_=true;active_=true;}}
    void close(){active_=false;}
    bool active()const{return active_;}
    void record(Event e){if(active_)accumulate(counts_,e);}
    // Caller supplies only the actual accepted probe ordinal. No inference from
    // SDK calls, counters or a later epoch; each snapshot is immutable.
    bool accepted(unsigned ordinal,Use use){
        if(!active_||ordinal>=snapshots_.size()||!valid(use)||snapshots_[ordinal].present)return false;
        if(ordinal&&(!snapshots_[0].present||!same(snapshots_[0].use,use)))return false;
        snapshots_[ordinal]={counts_,use,true};return true;
    }
    Counts const& counts()const{return counts_;}
    std::array<Snapshot,2> const& snapshots()const{return snapshots_;}
};
}
