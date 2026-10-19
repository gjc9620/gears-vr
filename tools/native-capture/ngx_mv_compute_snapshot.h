#pragma once
#include <array>
#include <cstdint>
namespace gears_ngx_mv_compute {
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
