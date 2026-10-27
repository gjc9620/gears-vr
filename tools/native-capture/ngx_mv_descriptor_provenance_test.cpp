// CPU provenance/lease fixtures only. No COM methods, shaders or GPU work.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "ngx_mv_descriptor_provenance.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <memory>

namespace {
struct ResourceFixture {uintptr_t canonical=101,device=7;bool valid=true;};
struct ResourceLease {
    uintptr_t raw{},canonical{},device{};std::shared_ptr<unsigned> hold;
    static ResourceLease observe(ID3D12Resource* typed){
        if(!typed)return {};auto& f=*reinterpret_cast<ResourceFixture*>(typed);
        return {reinterpret_cast<uintptr_t>(typed),f.canonical,f.device,f.valid?std::make_shared<unsigned>(1):nullptr};
    }
    bool retained()const{return bool(hold)&&canonical&&device;}
};
struct HeapFixture {uintptr_t canonical=201,device=7;uint64_t cpu=1000,gpu=0;uint32_t count=8,stride=16;bool visible=false,valid=true;};
struct HeapLease {
    uintptr_t raw{},canonical{},device{};uint64_t cpu{},gpu{};uint32_t count{},stride{};bool visible{};std::shared_ptr<unsigned> hold;
    static HeapLease observe(ID3D12DescriptorHeap* typed){
        if(!typed)return {};auto& f=*reinterpret_cast<HeapFixture*>(typed);
        return {reinterpret_cast<uintptr_t>(typed),f.canonical,f.device,f.cpu,f.gpu,f.count,f.stride,f.visible,f.valid?std::make_shared<unsigned>(1):nullptr};
    }
    bool retained()const{return bool(hold)&&canonical&&device;}
};
using Bank=gears_ngx_mv::DescriptorBank<ResourceLease,HeapLease,3,8>;
using Loss=gears_ngx_mv::Loss;using Ranges=gears_ngx_mv::Ranges<8>;
constexpr gears_ngx_mv::Scope scope{1,2,3,7};
ID3D12Resource* typed(ResourceFixture& f){return reinterpret_cast<ID3D12Resource*>(&f);}
ID3D12DescriptorHeap* typed(HeapFixture& f){return reinterpret_cast<ID3D12DescriptorHeap*>(&f);}
struct Setup {
    HeapFixture cpu{},gpu{202,7,2000,10000,8,32,true,true};ResourceFixture mv{},other{102,7,true};Bank bank;
    Setup(){assert(bank.begin(scope));assert(bank.observe_heap(scope,typed(cpu)));assert(bank.observe_heap(scope,typed(gpu)));}
    void write(uint64_t address=1000,ResourceFixture* resource=nullptr){assert(bank.write_uav(scope,address,typed(resource?*resource:mv),nullptr,{}));}
};
void rewrite_and_lifetime(){
    Setup s;s.write();assert(s.bank.contains_cpu(1000)&&!s.bank.contains_cpu(1001)&&!s.bank.contains_cpu(1128));
    auto original=s.bank.lookup(scope,1000);assert(original.retained_uav()&&original.version==original.origin&&s.bank.current(original));
    auto mv=ResourceLease::observe(typed(s.mv));assert(original.same_mv(mv));assert(!original.same_mv(ResourceLease{}));
    assert(s.bank.copy_simple(scope,2000,1000,1));auto gpu=s.bank.lookup(scope,10000,true);
    assert(gpu.retained_uav()&&gpu.heap.canonical==202&&gpu.cpu==2000&&gpu.gpu==10000&&gpu.same_mv(mv));
    assert(gpu.origin==original.origin&&gpu.source_version==original.version&&gpu.source_handle==1000&&gpu.version>original.version);
    std::weak_ptr<unsigned> retained=original.resource.hold;
    s.write(1000,&s.other);assert(!s.bank.current(original)&&s.bank.current(gpu)&&!retained.expired());
    assert(!s.bank.lookup(scope,1000).same_mv(mv));assert(s.bank.copy_simple(scope,2000,1000,1));
    assert(!s.bank.current(gpu)&&!s.bank.lookup(scope,10000,true).same_mv(mv));
    original={};gpu={};assert(retained.expired());
    auto current=s.bank.lookup(scope,2000);assert(s.bank.unknown_write(scope,2000));
    assert(!s.bank.current(current)&&!s.bank.lookup(scope,2000).retained_uav());
    assert(s.bank.lookup(scope,2000).version>current.version);
    // Source without captured CreateUAV remains unknown after a valid copy.
    assert(s.bank.copy_simple(scope,2032,1016,1));assert(!s.bank.lookup(scope,10032,true).retained_uav());
    assert(s.bank.lookup(scope,10032,true).version&&!s.bank.lookup(scope,10032,true).origin);
}
void ranges(){
    Setup s;s.write();s.write(1016,&s.other);s.write(1032);
    Ranges d{},r{};d.count=1;d.starts[0]=2000;d.sizes_present=true;d.sizes[0]=3;
    r.count=2;r.starts[0]=1000;r.starts[1]=1032;r.sizes_present=true;r.sizes[0]=2;r.sizes[1]=1;
    assert(s.bank.copy(scope,d,r));assert(s.bank.lookup(scope,10000,true).resource.canonical==101);
    assert(s.bank.lookup(scope,10032,true).resource.canonical==102&&s.bank.lookup(scope,10064,true).resource.canonical==101);
    // Native null size arrays mean one descriptor per range, not one whole heap.
    d={};r={};d.count=r.count=2;d.starts[0]=2096;d.starts[1]=2128;r.starts[0]=1016;r.starts[1]=1000;
    assert(s.bank.copy(scope,d,r));assert(s.bank.lookup(scope,10096,true).resource.canonical==102);
    assert(s.bank.lookup(scope,10128,true).resource.canonical==101);
    assert(!s.bank.lookup(scope,10001,true).version&&!s.bank.lookup(scope,10256,true).version);
    assert(!s.bank.lookup(scope,0,true).version&&!s.bank.lookup(scope,1000,true).version);
}
void invalid_ranges(){
    for(unsigned bad=0;bad<7;++bad){Setup s;s.write();auto old=s.bank.lookup(scope,1000);Ranges d{},r{};d.count=r.count=1;d.starts[0]=2000;r.starts[0]=1000;
        if(bad==0)d.readable=false;if(bad==1)d.count=9;if(bad==2)d.starts[0]=2001;
        if(bad==3){d.sizes_present=true;d.sizes[0]=2;}if(bad==4)r.starts[0]=2000;
        if(bad==5){d.starts[0]=1000;}if(bad==6){d.count=r.count=2;d.starts[1]=2000;r.starts[1]=1016;}
        assert(!s.bank.copy(scope,d,r)&&s.bank.failed()&&!s.bank.current(old)&&!s.bank.lookup(scope,1000).retained_uav());
    }
    Setup s;s.write();assert(!s.bank.copy_simple(scope,2224,1000,2)&&s.bank.loss()==Loss::bad_range);
    Setup empty;assert(empty.bank.copy_simple(scope,0,0,0)&&!empty.bank.failed());
}
void scope_capacity_and_heap_aba(){
    Setup s;s.write();auto captured=s.bank.lookup(scope,1000);auto wrong=scope;++wrong.ticket;
    assert(!s.bank.lookup(wrong,1000).version&&!s.bank.copy_simple(wrong,2000,1000,1));assert(!s.bank.current(captured));
    Setup fresh;fresh.write();assert(!fresh.bank.current(captured)); // Distinct bank owner, identical handles/serial.
    Setup twice;assert(!twice.bank.begin(scope));
    Setup capacity;for(unsigned i=0;i<8;++i)capacity.write(1000+16*i);assert(capacity.bank.cells()==8);
    auto old=capacity.bank.lookup(scope,1000);capacity.write(1000);assert(!capacity.bank.current(old)&&capacity.bank.cells()==8);
    assert(!capacity.bank.unknown_write(scope,2000)&&capacity.bank.loss()==Loss::capacity);
    Setup heaps;HeapFixture third{203,7,3000,0,8,8,false,true},fourth{204,7,4000,0,8,8,false,true};
    assert(heaps.bank.observe_heap(scope,typed(third)));assert(heaps.bank.observe_heap(scope,typed(third))&&heaps.bank.heaps()==3);
    assert(!heaps.bank.observe_heap(scope,typed(fourth))&&heaps.bank.loss()==Loss::capacity);
    Setup alias;HeapFixture collision{999,7,1000,0,8,16,false,true};assert(!alias.bank.observe_heap(scope,typed(collision))&&alias.bank.loss()==Loss::overlap);
    Setup inside_padding;HeapFixture padding{999,7,1113,0,1,16,false,true};
    assert(!inside_padding.bank.observe_heap(scope,typed(padding))&&inside_padding.bank.loss()==Loss::overlap);
    Setup adjacent;HeapFixture next{999,7,1128,0,1,16,false,true};assert(adjacent.bank.observe_heap(scope,typed(next)));
    Setup gpu_padding;HeapFixture gpu_overlap{999,7,3000,10225,1,32,true,true};
    assert(!gpu_padding.bank.observe_heap(scope,typed(gpu_overlap))&&gpu_padding.bank.loss()==Loss::overlap);
    Setup final_byte;HeapFixture fits{999,7,UINT64_MAX-15,0,1,16,false,true};assert(final_byte.bank.observe_heap(scope,typed(fits)));
    Setup crossing;HeapFixture crosses{999,7,UINT64_MAX-14,0,1,16,false,true};assert(!crossing.bank.observe_heap(scope,typed(crosses)));
    Setup mutated;mutated.cpu.stride=32;assert(!mutated.bank.observe_heap(scope,typed(mutated.cpu))&&mutated.bank.loss()==Loss::invalid_heap);
    Setup overflow;HeapFixture huge{999,7,UINT64_MAX-1,0,8,16,false,true};assert(!overflow.bank.observe_heap(scope,typed(huge)));
    Setup foreign;HeapFixture other_device{999,8,3000,0,8,16,false,true};assert(!foreign.bank.observe_heap(scope,typed(other_device)));
}
void typed_boundaries(){
    Setup s;s.write();auto old=s.bank.lookup(scope,1000);assert(s.bank.write_uav(scope,1000,nullptr,nullptr,{}));
    assert(!s.bank.current(old)&&!s.bank.lookup(scope,1000).known_uav);
    Setup unreadable;gears_ngx_mv::ViewDescription desc;desc.readable=false;assert(!unreadable.bank.write_uav(scope,1000,typed(unreadable.mv),nullptr,desc));
    Setup bad;bad.mv.valid=false;assert(!bad.bank.write_uav(scope,1000,typed(bad.mv),nullptr,{}));
    Setup device;device.mv.device=99;assert(!device.bank.write_uav(scope,1000,typed(device.mv),nullptr,{}));
    Setup counter;gears_ngx_mv::ViewDescription specified;specified.provided=true;specified.value.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;
    assert(counter.bank.write_uav(scope,1000,typed(counter.mv),typed(counter.other),specified));
    auto snapshot=counter.bank.lookup(scope,1000);assert(snapshot.counter.retained()&&snapshot.view.provided);
    std::weak_ptr<unsigned> held_heap;Bank::Snapshot retained;
    {Setup scoped;scoped.write();retained=scoped.bank.lookup(scope,1000);held_heap=retained.heap.hold;}
    assert(!held_heap.expired()&&retained.retained_uav());retained={};assert(held_heap.expired());
}
void early_lifetime_and_sparse_invalidations(){
    const auto early=gears_ngx_mv::lifetime_scope(9,7);
    Bank bank;HeapFixture heap;ResourceFixture mv;
    assert(bank.begin(early)&&bank.observe_heap(early,typed(heap)));
    for(unsigned i=0;i<8;++i)assert(bank.invalidate_if_tracked(early,1000+16*i));
    assert(bank.invalidate_if_tracked(early,9000)&&bank.cells()==0);
    assert(bank.write_uav(early,1000,typed(mv),nullptr,{}));
    auto birth=bank.lookup(early,1000);
    assert(birth.retained_uav()&&birth.scope.world==0&&birth.scope.ticket==0);
    // Consuming a different ticket cannot manufacture a creation in that scope.
    assert(!bank.lookup(scope,1000).version&&bank.current(birth));
    auto another=early;++another.epoch;assert(!bank.lookup(another,1000).version);
    assert(bank.invalidate_if_tracked(early,1000)&&bank.cells()==1);
    assert(!bank.current(birth)&&!bank.lookup(early,1000).retained_uav());
    assert(birth.retained_uav()); // Retained historical observation, not current.
    auto forged=early;forged.world=2;forged.ticket=3;
    Bank invalid;assert(!invalid.begin(forged));
    Bank missing;assert(!missing.begin(gears_ngx_mv::lifetime_scope(0,7)));
    auto wrong_kind=early;wrong_kind.kind=gears_ngx_mv::ScopeKind::selected_ticket;
    assert(!gears_ngx_mv::same(early,wrong_kind)&&!gears_ngx_mv::valid(wrong_kind));
    assert(!bank.begin(scope)); // No in-place relabeling or restart.
}
void sparse_ranges_and_rewrites(){
    Setup s;s.write();s.write(1032,&s.other);
    auto birth=s.bank.lookup(scope,1000);
    Ranges d{},r{};d.count=2;d.sizes_present=true;d.starts[0]=2000;d.sizes[0]=2;d.starts[1]=2128;d.sizes[1]=2;
    r.count=2;r.sizes_present=true;r.starts[0]=1000;r.sizes[0]=3;r.starts[1]=1064;r.sizes[1]=1;
    assert(s.bank.copy_sparse(scope,d,r)&&s.bank.cells()==4);
    auto first=s.bank.lookup(scope,2000),second=s.bank.lookup(scope,2128);
    assert(first.retained_uav()&&first.origin==birth.origin&&first.source_version==birth.version);
    assert(second.resource.canonical==102&&second.source_handle==1032);
    assert(!s.bank.lookup(scope,2032).version&&!s.bank.lookup(scope,2160).version);
    s.write(1000,&s.other);assert(s.bank.current(first));
    assert(s.bank.copy_simple_sparse(scope,2000,1000,1)&&s.bank.cells()==4);
    auto rewritten=s.bank.lookup(scope,2000);
    assert(!s.bank.current(first)&&rewritten.version>first.version&&rewritten.resource.canonical==102);
    // Missing UAV creation in an observed heap clears a tracked destination.
    assert(s.bank.copy_simple_sparse(scope,2000,1016,1)&&s.bank.cells()==4);
    auto erased=s.bank.lookup(scope,2000);
    assert(!s.bank.current(rewritten)&&!erased.known_uav&&!erased.resource.retained()&&!erased.origin);
    assert(erased.version>rewritten.version&&erased.source_handle==1016&&!erased.source_version);
    // Explicitly invalidated sources also remain unknown, without new cells.
    assert(s.bank.invalidate_if_tracked(scope,1032));auto invalid=s.bank.lookup(scope,1032);
    d={};r={};d.count=r.count=2;d.starts[0]=2128;d.starts[1]=2160;r.starts[0]=1032;r.starts[1]=1016;
    assert(s.bank.copy_sparse(scope,d,r)&&s.bank.cells()==4);
    assert(!s.bank.current(second)&&!s.bank.lookup(scope,2128).retained_uav());
    assert(s.bank.lookup(scope,2128).source_version==invalid.version&&!s.bank.lookup(scope,2160).version);
    assert(s.bank.copy_simple_sparse(scope,0,0,0));
}
void sparse_capacity_and_lifetime_scope(){
    const auto early=gears_ngx_mv::lifetime_scope(9,7);
    Bank bank;HeapFixture cpu{},gpu{202,7,2000,10000,8,32,true,true};ResourceFixture mv;
    assert(bank.begin(early)&&bank.observe_heap(early,typed(cpu))&&bank.observe_heap(early,typed(gpu)));
    assert(bank.copy_simple_sparse(early,2000,1000,8)&&bank.cells()==0);
    assert(bank.write_uav(early,1000,typed(mv),nullptr,{}));
    for(unsigned i=0;i<7;++i)assert(bank.copy_simple_sparse(early,2000+32*i,1000,1));
    assert(bank.cells()==8);auto old=bank.lookup(early,2000);
    assert(old.retained_uav()&&gears_ngx_mv::same(old.scope,early)&&!old.scope.world&&!old.scope.ticket);
    assert(bank.copy_simple_sparse(early,2224,1016,1)&&bank.cells()==8&&!bank.lookup(early,2224).version);
    assert(bank.copy_simple_sparse(early,2000,1016,1)&&bank.cells()==8&&!bank.current(old));
    assert(!bank.lookup(early,2000).retained_uav());
    assert(bank.copy_simple_sparse(early,2000,1000,1)&&bank.cells()==8&&bank.lookup(early,2000).retained_uav());
    // Eligible new provenance must not be dropped/evicted when full.
    assert(!bank.copy_simple_sparse(early,2224,1000,1)&&bank.loss()==Loss::capacity);
    assert(!bank.lookup(early,2000).version);
}
void sparse_rejections(){
    // Validate even wholly unknown copies that would allocate no cells.
    for(unsigned bad=0;bad<9;++bad){Setup s;Ranges d{},r{};d.count=r.count=1;d.starts[0]=2000;r.starts[0]=1000;
        if(bad==0)d.readable=false;if(bad==1)d.count=9;if(bad==2)d.starts[0]=2001;
        if(bad==3){d.sizes_present=true;d.sizes[0]=2;}if(bad==4)r.starts[0]=2000;
        if(bad==5)d.starts[0]=1000;if(bad==6){d.count=r.count=2;d.starts[1]=2000;r.starts[1]=1016;}
        if(bad==7){d.starts[0]=2224;d.sizes_present=r.sizes_present=true;d.sizes[0]=r.sizes[0]=2;}
        if(bad==8)r.starts[0]=9000;
        assert(!s.bank.copy_sparse(scope,d,r)&&s.bank.failed()&&s.bank.cells()==0);
    }
    Setup unknown;unknown.write(2000);auto old=unknown.bank.lookup(scope,2000);
    assert(!unknown.bank.copy_simple_sparse(scope,2000,9000,1)&&unknown.bank.loss()==Loss::unknown_heap);
    assert(!unknown.bank.current(old)&&!unknown.bank.lookup(scope,2000).version);
    Setup wrong;auto changed=scope;++changed.ticket;
    assert(!wrong.bank.copy_simple_sparse(changed,2000,1000,1)&&wrong.bank.loss()==Loss::bad_scope);
}
void production_heap_boundary(){
    using DefaultBank=gears_ngx_mv::DescriptorBank<ResourceLease,HeapLease>;
    using ObserverBank=gears_ngx_mv::ObserverDescriptorBank<ResourceLease,HeapLease>;
    static_assert(DefaultBank::heap_capacity==8&&DefaultBank::cell_capacity==128&&DefaultBank::copy_capacity==128,"generic fixture defaults unchanged");
    static_assert(ObserverBank::heap_capacity==96&&ObserverBank::cell_capacity==512&&ObserverBank::copy_capacity==128,"explicit production bounds");
    static_assert(gears_ngx_mv::ObserverDescriptorBank<>::copy_scratch_bytes==gears_ngx_mv::DescriptorBank<>::copy_scratch_bytes,"production copy array scratch stays at the previous 128-cell size");
    const auto early=gears_ngx_mv::lifetime_scope(1,7);
    ObserverBank bank;assert(bank.begin(early));
    std::array<HeapFixture,97> heaps{};ResourceFixture mv;
    for(size_t i=0;i<96;++i){auto& h=heaps[i];h.canonical=1000+i;h.cpu=10000+1024*i;
        h.visible=i%2==0;h.gpu=h.visible?1000000+1024*i:0;
        assert(bank.observe_heap(early,typed(h))&&bank.heaps()==i+1&&!bank.failed());
        if(i==0)assert(bank.write_uav(early,h.cpu,typed(mv),nullptr,{}));
        // The observed real-game census of 60 must remain readable, as must 96.
        if(i==59||i==95)assert(bank.lookup(early,heaps[0].cpu).retained_uav());
    }
    auto captured=bank.lookup(early,heaps[0].cpu);assert(bank.current(captured));
    assert(bank.observe_heap(early,typed(heaps[95]))&&bank.heaps()==96); // Existing identity at capacity.
    auto& extra=heaps[96];extra.canonical=1096;extra.cpu=10000+1024*96;
    assert(!bank.observe_heap(early,typed(extra))&&bank.loss()==Loss::capacity&&bank.heaps()==96);
    assert(!bank.current(captured)&&!bank.lookup(early,heaps[0].cpu).version&&captured.retained_uav());
    assert(!bank.observe_heap(early,typed(heaps[0]))&&!bank.begin(early)); // Sticky, no eviction/rearm.
}
void retained_mv_join(){
    Setup s;s.write();s.write(1016,&s.other);s.write(1032);
    assert(s.bank.copy_simple(scope,2000,1000,1));
    auto mv=ResourceLease::observe(typed(s.mv));size_t calls{};
    auto visit=[&](Bank::Snapshot const& snapshot){++calls;assert(snapshot.same_mv(mv)&&s.bank.current(snapshot));};
    assert(s.bank.visit_mv(scope,mv,visit)==3&&calls==3);
    auto wrong=scope;++wrong.ticket;calls=0;
    assert(!s.bank.visit_mv(wrong,mv,visit)&&!calls);
    assert(!s.bank.visit_mv(scope,ResourceLease{},visit)&&!calls);
    auto foreign=mv;foreign.device=8;assert(!s.bank.visit_mv(scope,foreign,visit)&&!calls);
    // Same resource through another retained interface address is an identity
    // match, whereas another resource with identical dimensions is excluded.
    auto alias=mv;++alias.raw;assert(s.bank.visit_mv(scope,alias,visit)==3&&calls==3);
    assert(s.bank.invalidate_if_tracked(scope,1000));calls=0;
    assert(s.bank.visit_mv(scope,mv,visit)==2&&calls==2);
    assert(s.bank.copy_simple_sparse(scope,2000,1016,1));calls=0;
    assert(s.bank.visit_mv(scope,mv,visit)==1&&calls==1);
    s.bank.uncertain();calls=0;assert(!s.bank.visit_mv(scope,mv,visit)&&!calls);

    // A scalar collector can cap saved rows without silently undercounting
    // matches. All live cells remain bounded by the production cell bank.
    using ObserverBank=gears_ngx_mv::ObserverDescriptorBank<ResourceLease,HeapLease>;
    ObserverBank many;HeapFixture heap;heap.count=16;
    assert(many.begin(scope)&&many.observe_heap(scope,typed(heap)));
    for(unsigned i=0;i<10;++i)assert(many.write_uav(scope,1000+16*i,typed(s.mv),nullptr,{}));
    size_t saved{},total=many.visit_mv(scope,mv,[&](ObserverBank::Snapshot const& snapshot){
        assert(snapshot.same_mv(mv));if(saved<8)++saved;});
    assert(total==10&&saved==8);
}
void erased_cell_reuse_and_snapshot_aba(){
    using ReusingBank=gears_ngx_mv::DescriptorBank<ResourceLease,HeapLease,3,8,true>;
    static_assert(!Bank::reuse_non_uav,"generic copies retain existing tombstone policy");
    static_assert(gears_ngx_mv::ObserverDescriptorBank<ResourceLease,HeapLease>::reuse_non_uav,"production reuses only non-UAV cells");
    ReusingBank bank;HeapFixture heap;heap.count=16;ResourceFixture mv;
    assert(bank.begin(scope)&&bank.observe_heap(scope,typed(heap)));
    for(unsigned i=0;i<8;++i)assert(bank.write_uav(scope,1000+16*i,typed(mv),nullptr,{}));
    auto original=bank.lookup(scope,1000);assert(bank.cell_counts().known_uav==8);
    assert(bank.invalidate_if_tracked(scope,1000));auto tombstone=bank.lookup(scope,1000);
    assert(!bank.current(original)&&bank.current(tombstone)&&!tombstone.retained_uav());
    assert(bank.cell_counts().known_uav==7&&bank.cell_counts().non_uav==1&&!bank.cell_counts().unused);
    assert(bank.write_uav(scope,1128,typed(mv),nullptr,{}));auto reused=bank.lookup(scope,1128);
    assert(bank.cells()==8&&bank.cell_counts().known_uav==8&&!bank.cell_counts().non_uav&&bank.cell_counts().reuses==1);
    assert(reused.version>tombstone.version&&!bank.lookup(scope,1000).version&&!bank.current(tombstone));
    assert(original.retained_uav()&&!bank.current(original)); // Historical owners remain valid.
    assert(bank.invalidate_if_tracked(scope,1016));
    assert(bank.write_uav(scope,1000,typed(mv),nullptr,{}));auto recreated=bank.lookup(scope,1000);
    assert(recreated.version>reused.version&&bank.current(recreated)&&!bank.current(original)&&!bank.current(tombstone));
    assert(bank.cell_counts().reuses==2);
    // Every remaining slot is a known UAV; no live evidence may be evicted.
    assert(!bank.write_uav(scope,1144,typed(mv),nullptr,{})&&bank.loss()==Loss::capacity);
    auto saturation=bank.saturation();assert(saturation.site==ReusingBank::CapacitySite::cells);
    assert(saturation.heaps==1&&saturation.cells.known_uav==8&&!saturation.cells.non_uav&&!saturation.cells.unused&&saturation.cells.reuses==2);
    assert(!bank.current(recreated)&&!bank.lookup(scope,1128).version&&recreated.retained_uav());
    assert(!bank.invalidate_if_tracked(scope,1000)&&bank.saturation().cells.known_uav==8); // Frozen first failure.
    Bank generic;assert(generic.begin(scope)&&generic.observe_heap(scope,typed(heap)));
    for(unsigned i=0;i<8;++i)assert(generic.unknown_write(scope,1000+16*i));
    assert(!generic.unknown_write(scope,1128)&&generic.loss()==Loss::capacity);
    const auto old_policy=generic.saturation();assert(old_policy.site==Bank::CapacitySite::cells);
    assert(!old_policy.cells.known_uav&&old_policy.cells.non_uav==8&&!old_policy.cells.reuses);
}
void reuse_copy_source_snapshot_and_validation(){
    using ReusingBank=gears_ngx_mv::DescriptorBank<ResourceLease,HeapLease,3,4,true>;
    using CopyRanges=gears_ngx_mv::Ranges<4>;
    HeapFixture cpu{},gpu{202,7,2000,10000,8,32,true,true};ResourceFixture mv;
    ReusingBank bank;assert(bank.begin(scope)&&bank.observe_heap(scope,typed(cpu))&&bank.observe_heap(scope,typed(gpu)));
    assert(bank.write_uav(scope,1000,typed(mv),nullptr,{})&&bank.write_uav(scope,1016,typed(mv),nullptr,{}));
    assert(bank.write_uav(scope,1032,typed(mv),nullptr,{})&&bank.copy_simple_sparse(scope,2000,1000,1));
    auto source=bank.lookup(scope,1000),historical=bank.lookup(scope,1032);
    std::weak_ptr<unsigned> historical_hold=historical.resource.hold;
    assert(bank.invalidate_if_tracked(scope,1032));auto erased=bank.lookup(scope,1032);
    CopyRanges d{},s{};d.count=s.count=2;d.starts[0]=2032;d.starts[1]=2064;s.starts[0]=1000;s.starts[1]=1032;
    assert(bank.copy_sparse(scope,d,s)); // First destination reuses the later source's tombstone.
    auto copied=bank.lookup(scope,2032);assert(copied.retained_uav()&&copied.source_handle==1000&&copied.source_version==source.version);
    assert(copied.resource.hold==source.resource.hold&&copied.origin==source.origin&&bank.current(source));
    assert(historical.retained_uav()&&!bank.current(historical)&&!historical_hold.expired());
    assert(!bank.current(erased)&&!bank.lookup(scope,1032).version&&!bank.lookup(scope,2064).version&&bank.cell_counts().reuses==1);
    // Full copy sources were frozen first; missing/erased provenance never
    // becomes the new UAV merely because its old storage slot was reused.
    assert(bank.cell_counts().known_uav==4);
    for(unsigned bad=0;bad<3;++bad){ReusingBank invalid;assert(invalid.begin(scope)&&invalid.observe_heap(scope,typed(cpu)));
        for(unsigned i=0;i<4;++i)assert(invalid.write_uav(scope,1000+16*i,typed(mv),nullptr,{}));
        assert(invalid.invalidate_if_tracked(scope,1048));
        CopyRanges dst{},src{};dst.count=src.count=1;dst.starts[0]=1064;src.starts[0]=1000;
        if(bad==0)dst.readable=false;if(bad==1)dst.starts[0]=1000;
        if(bad==2){dst.count=src.count=2;dst.starts[1]=1064;src.starts[1]=1016;}
        assert(!invalid.copy_sparse(scope,dst,src)&&invalid.failed()&&!invalid.cell_counts().reuses);
        assert(invalid.cell_counts().known_uav==3&&invalid.cell_counts().non_uav==1);
    }
}
void production_cell_and_copy_boundaries(){
    using ObserverBank=gears_ngx_mv::ObserverDescriptorBank<ResourceLease,HeapLease>;
    auto bank=std::make_unique<ObserverBank>();HeapFixture heap;heap.count=513;ResourceFixture mv;
    const auto early=gears_ngx_mv::lifetime_scope(1,7);
    assert(bank->begin(early)&&bank->observe_heap(early,typed(heap)));
    for(unsigned i=0;i<128;++i)assert(bank->write_uav(early,1000+16*i,typed(mv),nullptr,{}));
    auto original=bank->lookup(early,1000),last_source=bank->lookup(early,1000+16*127);
    // Persistent occupancy may grow across bounded batches; each copy still
    // freezes at most 128 sources and performs no heap allocation for scratch.
    for(unsigned batch=1;batch<4;++batch){
        assert(bank->copy_simple_sparse(early,1000+16*128*batch,1000,128));
        assert(bank->cells()==128*(batch+1)&&bank->current(original));
    }
    auto last=bank->lookup(early,1000+16*511);
    assert(last.retained_uav()&&last.source_version==last_source.version&&last.origin==last_source.origin&&bank->current(last));
    assert(bank->cell_counts().known_uav==512&&!bank->cell_counts().non_uav&&!bank->cell_counts().unused&&!bank->cell_counts().reuses);
    assert(!bank->write_uav(early,1000+16*512,typed(mv),nullptr,{})&&bank->loss()==Loss::capacity);
    auto saturation=bank->saturation();assert(saturation.site==ObserverBank::CapacitySite::cells&&saturation.cells.known_uav==512);
    assert(!bank->current(original)&&!bank->current(last)&&original.retained_uav()&&last.retained_uav());
    assert(!bank->invalidate_if_tracked(early,1000)&&!bank->begin(early)); // No capacity recovery/reset.

    auto oversized=std::make_unique<ObserverBank>();assert(oversized->begin(early)&&oversized->observe_heap(early,typed(heap)));
    assert(oversized->copy_simple_sparse(early,1000+16*256,1000,128)&&!oversized->cells());
    assert(!oversized->copy_simple_sparse(early,1000+16*256,1000,129)&&oversized->loss()==Loss::bad_range&&!oversized->cells());
    auto ranges=std::make_unique<ObserverBank>();assert(ranges->begin(early)&&ranges->observe_heap(early,typed(heap)));
    ObserverBank::CopyRanges d{},s{};d.count=129;
    assert(!ranges->copy_sparse(early,d,s)&&ranges->loss()==Loss::capacity&&!ranges->cells());
    assert(ranges->saturation().site==ObserverBank::CapacitySite::ranges&&ranges->saturation().cells.unused==512);
}
void source_ledger_aliases_fanout_and_rewrites(){
    using Ledger=gears_ngx_mv::SourceDescriptorLedger<ResourceLease,HeapLease>;
    static_assert(Ledger::heap_capacity==96&&Ledger::cell_capacity==128&&Ledger::copy_capacity==128&&Ledger::source_ledger&&!Ledger::reuse_non_uav,"bounded independent source scope");
    Ledger ledger;HeapFixture cpu{},aliases{202,7,100000,0,2048,16,false,true},gpu{203,7,1000000,10000000,4096,16,true,true};
    ResourceFixture mv{},other{102,7,true};const auto early=gears_ngx_mv::lifetime_scope(1,7);
    assert(ledger.begin(early)&&ledger.observe_heap(early,typed(cpu))&&ledger.observe_heap(early,typed(aliases))&&ledger.observe_heap(early,typed(gpu)));
    gears_ngx_mv::ViewDescription view;view.provided=true;view.value.Format=DXGI_FORMAT_R32_UINT;view.value.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;
    assert(ledger.write_uav(early,1000,typed(mv),typed(other),view));auto birth=ledger.lookup(early,1000);
    assert(ledger.write_uav(early,1000000,typed(other),nullptr,view));auto visible_birth=ledger.lookup(early,1000000);
    assert(visible_birth.retained_uav()&&visible_birth.heap.visible&&ledger.cells()==2);
    assert(ledger.copy_simple_sparse(early,100000,1000,1));auto alias=ledger.lookup(early,100000);
    assert(alias.retained_uav()&&alias.origin==birth.origin&&alias.source_version==birth.version&&alias.counter.canonical==other.canonical&&alias.view.provided);
    assert(ledger.copy_simple_sparse(early,100016,100000,1));auto alias_chain=ledger.lookup(early,100016);
    assert(alias_chain.origin==birth.origin&&alias_chain.source_version==alias.version&&ledger.cells()==4);
    // More visible replicas than ledger capacity do not allocate source cells.
    for(unsigned i=1;i<1024;++i)assert(ledger.copy_simple_sparse(early,1000000+16*i,100000,1));
    assert(ledger.cells()==4&&!ledger.failed()&&!ledger.lookup(early,1000016).version);
    // A direct-born visible cell remains tracked and must update on copy.
    assert(ledger.copy_simple_sparse(early,1000000,1000,1));auto visible_updated=ledger.lookup(early,1000000);
    assert(visible_updated.resource.canonical==mv.canonical&&!ledger.current(visible_birth)&&visible_updated.source_version==birth.version);
    assert(ledger.write_uav(early,1000,typed(other),nullptr,view));assert(!ledger.current(birth)&&ledger.current(alias));
    assert(ledger.copy_simple_sparse(early,100000,1000,1));assert(!ledger.current(alias)&&ledger.lookup(early,100000).resource.canonical==other.canonical);
    // Unknown source overwrites an existing CPU alias or visible direct birth.
    assert(ledger.copy_simple_sparse(early,100016,1016,1)&&!ledger.lookup(early,100016).retained_uav()&&!ledger.current(alias_chain));
    assert(ledger.copy_simple_sparse(early,1000000,1016,1)&&!ledger.lookup(early,1000000).retained_uav()&&!ledger.current(visible_updated));
    assert(ledger.invalidate_if_tracked(early,1000)&&ledger.invalidate_if_tracked(early,100000)); // SRV/CBV erasure.
    auto mv_lease=ResourceLease::observe(typed(mv));size_t visits{};
    assert(!ledger.visit_mv(early,mv_lease,[&](Ledger::Snapshot const&){++visits;})&&!visits);
    assert(birth.retained_uav()&&visible_birth.retained_uav()&&!ledger.current(birth));
}
void source_ledger_independence_and_failed_no_join(){
    using Legacy=gears_ngx_mv::DescriptorBank<ResourceLease,HeapLease,3,4>;
    using Ledger=gears_ngx_mv::SourceDescriptorLedger<ResourceLease,HeapLease>;
    Legacy legacy;Ledger ledger;HeapFixture cpu{},gpu{202,7,1000000,10000000,4096,16,true,true};ResourceFixture mv;
    const auto early=gears_ngx_mv::lifetime_scope(1,7);
    assert(legacy.begin(early)&&ledger.begin(early));
    HeapFixture* const observed[]{&cpu,&gpu};
    for(auto* h:observed)assert(legacy.observe_heap(early,typed(*h))&&ledger.observe_heap(early,typed(*h)));
    assert(legacy.write_uav(early,1000,typed(mv),nullptr,{})&&ledger.write_uav(early,1000,typed(mv),nullptr,{}));
    for(unsigned i=0;i<3;++i){assert(legacy.copy_simple_sparse(early,1000000+16*i,1000,1));assert(ledger.copy_simple_sparse(early,1000000+16*i,1000,1));}
    assert(!legacy.copy_simple_sparse(early,1000048,1000,1)&&legacy.loss()==Loss::capacity);
    // The caller continues admitting the independent ledger after legacy loss;
    // this does not mutate or rehabilitate the frozen legacy observation.
    const auto frozen=legacy.saturation();assert(ledger.copy_simple_sparse(early,1000048,1000,1));
    assert(ledger.write_uav(early,1016,typed(mv),nullptr,{}));
    auto typed_mv=ResourceLease::observe(typed(mv));size_t matched{};
    assert(ledger.visit_mv(early,typed_mv,[&](Ledger::Snapshot const& snapshot){
        ++matched;assert(snapshot.same_mv(typed_mv)&&ledger.current(snapshot)&&gears_ngx_mv::same(snapshot.scope,early)&&!snapshot.scope.world&&!snapshot.scope.ticket);})==2&&matched==2);
    assert(legacy.failed()&&legacy.saturation().cells.known_uav==frozen.cells.known_uav&&!legacy.lookup(early,1000).version);
    ledger.uncertain();matched=0;assert(!ledger.visit_mv(early,typed_mv,[&](Ledger::Snapshot const&){++matched;})&&!matched);
}
void source_ledger_skipped_copy_validation(){
    using Ledger=gears_ngx_mv::SourceDescriptorLedger<ResourceLease,HeapLease>;
    const auto early=gears_ngx_mv::lifetime_scope(1,7);HeapFixture cpu{},gpu{202,7,1000000,10000000,4096,16,true,true};ResourceFixture mv;
    for(unsigned bad=0;bad<4;++bad){Ledger ledger;assert(ledger.begin(early)&&ledger.observe_heap(early,typed(cpu))&&ledger.observe_heap(early,typed(gpu)));
        assert(ledger.write_uav(early,1000,typed(mv),nullptr,{}));auto original=ledger.lookup(early,1000);
        Ledger::CopyRanges d{},s{};d.count=s.count=1;d.starts[0]=1000016;s.starts[0]=1000;
        if(bad==0)d.readable=false;if(bad==1)d.starts[0]=1000017;
        if(bad==2){d.count=s.count=2;d.starts[1]=1000016;s.starts[1]=1016;}
        if(bad==3)s.starts[0]=1000000; // Shader-visible source prohibited even though destination is skipped.
        assert(!ledger.copy_sparse(early,d,s)&&ledger.failed()&&ledger.cells()==1&&!ledger.current(original));
    }
    Ledger full;HeapFixture large=cpu;large.count=129;
    assert(full.begin(early)&&full.observe_heap(early,typed(large)));
    assert(full.write_uav(early,1000,typed(mv),nullptr,{}));
    // Nonvisible aliases remain bounded, with no source-ledger eviction/rearm.
    for(unsigned i=1;i<128;++i)assert(full.copy_simple_sparse(early,1000+16*i,1000,1));
    auto old=full.lookup(early,1000);assert(full.cells()==128&&full.cell_counts().known_uav==128);
    assert(!full.copy_simple_sparse(early,1000+16*128,1000,1)&&full.loss()==Loss::capacity);
    size_t joined{};auto typed_mv=ResourceLease::observe(typed(mv));
    assert(!full.visit_mv(early,typed_mv,[&](Ledger::Snapshot const&){++joined;})&&!joined&&!full.current(old)&&old.retained_uav());
    assert(!full.begin(early)&&full.saturation().cells.known_uav==128);
}
void lease_only_seed_and_source_retirement(){
    using Source=gears_ngx_mv::SourceDescriptorLedger<ResourceLease,HeapLease>;
    using Target=gears_ngx_mv::SelectedMvDescriptorBank<ResourceLease,HeapLease>;
    static_assert(Target::heap_capacity==96&&Target::cell_capacity==128&&Target::copy_capacity==128&&Target::reuse_non_uav,"bounded future target core");
    const auto birth_scope=gears_ngx_mv::lifetime_scope(2,7);const gears_ngx_mv::Scope target_scope{5,6,7,7};
    auto target=std::make_unique<Target>();assert(target->begin(target_scope));
    std::weak_ptr<unsigned> resource_hold,counter_hold,heap_hold;uint64_t source_version{},source_origin{};
    Target::Snapshot seeded,copied;ResourceLease selected;
    {Source source;HeapFixture cpu{},gpu{202,7,2000,10000,8,32,true,true};ResourceFixture mv{},counter{102,7,true};
        assert(source.begin(birth_scope)&&source.observe_heap(birth_scope,typed(cpu))&&source.observe_heap(birth_scope,typed(gpu)));
        assert(source.unknown_write(birth_scope,1000)&&source.unknown_write(birth_scope,1032));
        gears_ngx_mv::ViewDescription view;view.provided=true;view.value.Format=DXGI_FORMAT_R32_UINT;view.value.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;
        assert(source.write_uav(birth_scope,1000,typed(mv),typed(counter),view)&&source.copy_simple_sparse(birth_scope,1016,1000,1));
        auto alias=source.lookup(birth_scope,1016);selected=ResourceLease::observe(typed(mv));
        resource_hold=alias.resource.hold;counter_hold=alias.counter.hold;heap_hold=alias.heap.hold;source_version=alias.version;source_origin=alias.origin;
        size_t heaps{};assert(source.visit_heap_leases(birth_scope,[&](HeapLease const& lease){++heaps;return target->observe_heap_lease(target_scope,lease);}));
        assert(heaps==2&&target->heaps()==2&&target->seed_current_mv(target_scope,source,alias,selected));
        seeded=target->lookup(target_scope,1016);
        assert(seeded.same_mv(selected)&&seeded.version==1&&seeded.origin==1&&seeded.version!=source_version&&seeded.origin!=source_origin);
        assert(seeded.seed&&gears_ngx_mv::same(seeded.seed->scope,birth_scope)&&seeded.seed->owner==alias.owner&&seeded.seed->version==source_version&&seeded.seed->origin==source_origin);
        assert(seeded.seed->source_handle==1000&&seeded.seed->source_version==source_origin&&seeded.counter.hold==alias.counter.hold&&seeded.view.value.Format==view.value.Format);
        assert(target->copy_simple_sparse(target_scope,2000,1016,1));copied=target->lookup(target_scope,10000,true);
        assert(copied.same_mv(selected)&&copied.seed==seeded.seed&&copied.source_version==seeded.version&&copied.origin==seeded.origin);
        // Re-seeding is a fresh local write; old target snapshots cannot become current.
        assert(target->seed_current_mv(target_scope,source,alias,selected)&&!target->current(seeded)&&target->current(copied));
    }
    selected={};assert(!resource_hold.expired()&&!counter_hold.expired()&&!heap_hold.expired());
    auto live=target->lookup(target_scope,1016);assert(live.retained_uav()&&live.seed->version==source_version&&gears_ngx_mv::same(live.scope,target_scope));
    assert(target->unknown_write(target_scope,1016)&&!target->lookup(target_scope,1016).seed&&!target->current(live));
    assert(target->unknown_write(target_scope,2000)&&!target->current(copied));
    seeded={};copied={};live={};assert(resource_hold.expired()&&counter_hold.expired()&&!heap_hold.expired());
    target.reset();assert(heap_hold.expired());
}
void seed_rejects_stale_owner_failure_device_and_non_mv(){
    using Target=gears_ngx_mv::SelectedMvDescriptorBank<ResourceLease,HeapLease>;
    for(unsigned bad=0;bad<7;++bad){Setup source;source.write();Setup other;other.write();
        auto captured=source.bank.lookup(scope,1000);auto selected=ResourceLease::observe(typed(source.mv));
        auto target_scope=scope;++target_scope.epoch;if(bad==3)target_scope.device=8;
        Target target;assert(target.begin(target_scope));
        if(bad!=3&&bad!=6)assert(source.bank.visit_heap_leases(scope,[&](HeapLease const& h){return target.observe_heap_lease(target_scope,h);}));
        if(bad==0)assert(source.bank.invalidate_if_tracked(scope,1000));
        if(bad==1)captured=other.bank.lookup(scope,1000);
        if(bad==2)source.bank.uncertain();
        if(bad==4){selected=ResourceLease::observe(typed(source.other));captured.resource=selected;} // Forged retained payload cannot bypass the live source check.
        if(bad==5)++captured.gpu;
        assert(!target.seed_current_mv(target_scope,source.bank,captured,selected)&&target.failed()&&!target.cells());
        assert(source.bank.heaps()==2); // Rejection never mutates the donating bank.
    }
}
void retained_heap_import_validation_and_authoritative_seed(){
    using Target=gears_ngx_mv::SelectedMvDescriptorBank<ResourceLease,HeapLease>;
    for(unsigned bad=0;bad<4;++bad){Setup source;source.write();auto captured=source.bank.lookup(scope,1000);auto lease=captured.heap;
        Target target;assert(target.begin(scope));
        if(bad==0)lease.hold.reset();if(bad==1)lease.device=8;
        if(bad==2)lease.cpu=UINT64_MAX-1;
        if(bad==3){assert(target.observe_heap_lease(scope,lease));++lease.canonical;}
        assert(!target.observe_heap_lease(scope,lease)&&target.failed());
    }
    Setup source;source.write();auto captured=source.bank.lookup(scope,1000);auto selected=ResourceLease::observe(typed(source.mv));
    Target target;auto next=scope;++next.epoch;assert(target.begin(next));
    assert(source.bank.visit_heap_leases(scope,[&](HeapLease const& h){return target.observe_heap_lease(next,h);}));
    // Public diagnostic fields cannot substitute another retained payload while
    // preserving a genuine owner/version: seed copies the live source row.
    captured.counter=ResourceLease::observe(typed(source.other));captured.view.value.Format=DXGI_FORMAT_R32_UINT;
    assert(target.seed_current_mv(next,source.bank,captured,selected));auto seeded=target.lookup(next,1000);
    assert(!seeded.counter.retained()&&seeded.view.value.Format==DXGI_FORMAT_UNKNOWN);
    source.bank.uncertain();size_t visited{};assert(!source.bank.visit_heap_leases(scope,[&](HeapLease const&){++visited;return true;})&&!visited);
}
}
int main(){_set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
    static_assert(!Bank::Snapshot::producer_authorized,"observation must not authorize producer");
    rewrite_and_lifetime();ranges();invalid_ranges();scope_capacity_and_heap_aba();typed_boundaries();
    early_lifetime_and_sparse_invalidations();
    sparse_ranges_and_rewrites();sparse_capacity_and_lifetime_scope();sparse_rejections();
    production_heap_boundary();retained_mv_join();
    erased_cell_reuse_and_snapshot_aba();reuse_copy_source_snapshot_and_validation();
    production_cell_and_copy_boundaries();
    source_ledger_aliases_fanout_and_rewrites();source_ledger_independence_and_failed_no_join();source_ledger_skipped_copy_validation();
    lease_only_seed_and_source_retirement();seed_rejects_stale_owner_failure_device_and_non_mv();retained_heap_import_validation_and_authoritative_seed();
    std::puts("NGX MV descriptor CPU provenance tests passed; no GPU/Dispatch/producer proof");
}
