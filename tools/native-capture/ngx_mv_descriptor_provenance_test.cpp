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
}
int main(){_set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
    static_assert(!Bank::Snapshot::producer_authorized,"observation must not authorize producer");
    rewrite_and_lifetime();ranges();invalid_ranges();scope_capacity_and_heap_aba();typed_boundaries();
    early_lifetime_and_sparse_invalidations();
    std::puts("NGX MV descriptor CPU provenance tests passed; no GPU/Dispatch/producer proof");
}
