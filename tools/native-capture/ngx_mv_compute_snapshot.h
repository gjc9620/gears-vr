#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
namespace gears_ngx_mv_compute {
constexpr std::size_t pso_blob_save_limit=4u*1024u*1024u;
constexpr bool pso_blob_save_size(std::size_t size){return size&&size<=pso_blob_save_limit;}
struct PsoWitnessKey {
    uintptr_t canonical{},pso{},renderer{};uint64_t generation{},world{},ticket{},serial{};
};
struct RootWitnessKey {
    uintptr_t canonical{},root{},device{},renderer{};
    uint64_t generation{},world{},ticket{},serial{};
    unsigned provenance{};std::size_t size{};
};
inline bool root_witness_matches(RootWitnessKey const& key,bool valid,uintptr_t canonical,uint64_t generation,
                                 uint64_t world,uint64_t ticket,uintptr_t renderer,uint64_t serial){
    return valid&&key.canonical&&key.root&&key.device&&key.generation&&key.world&&key.ticket&&key.renderer&&key.size&&
        key.canonical==canonical&&key.generation==generation&&key.world==world&&key.ticket==ticket&&key.renderer==renderer&&key.serial==serial;
}
struct PsoWitnessValidity {
    bool valid{},historical_stable{};
    void revoke(){valid=false;}
    void attest_stable_dispatch(bool stable){if(valid&&stable)historical_stable=true;}
};
inline bool pso_witness_matches(PsoWitnessKey const& key,bool valid,uintptr_t canonical,uint64_t generation,
                                uint64_t world,uint64_t ticket,uintptr_t renderer,uint64_t serial){
    return valid&&key.canonical&&key.pso&&key.generation&&key.world&&key.ticket&&key.renderer&&
        key.canonical==canonical&&key.generation==generation&&key.world==world&&key.ticket==ticket&&key.renderer==renderer&&key.serial==serial;
}
// Scalar snapshot only; native descriptor/resource ownership stays in its bank.
struct ExactTarget {
    bool ready{},match{};uintptr_t heap{},resource{};
    uint64_t gpu{},version{},origin{},source_handle{},source_version{};
};
inline bool same_exact(ExactTarget const& a,ExactTarget const& b){
    return a.ready==b.ready&&a.match==b.match&&a.heap==b.heap&&a.resource==b.resource&&a.gpu==b.gpu&&
        a.version==b.version&&a.origin==b.origin&&a.source_handle==b.source_handle&&a.source_version==b.source_version;
}
struct DescriptorCandidate {
    uint64_t cpu{},gpu{},version{},origin{},source_handle{},source_version{},offset{};
};
struct DescriptorLookup {
    bool ready{},exact_base_mv{},overflow{};unsigned count{};
    std::array<DescriptorCandidate,8> candidates{};
};
inline bool same(DescriptorLookup const& a,DescriptorLookup const& b){
    if(a.ready!=b.ready||a.exact_base_mv!=b.exact_base_mv||a.overflow!=b.overflow||a.count!=b.count)return false;
    for(unsigned i=0;i<a.count;++i){auto const& x=a.candidates[i];auto const& y=b.candidates[i];
        if(x.cpu!=y.cpu||x.gpu!=y.gpu||x.version!=y.version||x.origin!=y.origin||
           x.source_handle!=y.source_handle||x.source_version!=y.source_version||x.offset!=y.offset)return false;}
    return true;
}
}
