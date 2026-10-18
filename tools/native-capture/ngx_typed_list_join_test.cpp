#include "ngx_epoch_pair_probe.h"
#include <memory>
#include <stdexcept>
using namespace gears_ngx_epoch;
using Microsoft::WRL::ComPtr;
static unsigned checks{};
static void check(bool value){++checks;if(!value)throw std::runtime_error("typed list join check failed");}
static void hr(HRESULT value){check(SUCCEEDED(value));}
static List identity(ID3D12CommandList* p,uint64_t generation){ComPtr<IUnknown> id;hr(p->QueryInterface(IID_PPV_ARGS(&id)));return {reinterpret_cast<uintptr_t>(p),reinterpret_cast<uintptr_t>(id.Get()),generation};}
static ULONG references(IUnknown* p){const auto n=p->AddRef();p->Release();return n;}
int main(){try{
    ComPtr<ID3D12Device> device;hr(D3D12CreateDevice(nullptr,D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)));
    ComPtr<ID3D12CommandAllocator> allocator;hr(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)));
    auto make=[&](){ComPtr<ID3D12GraphicsCommandList> p;hr(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&p)));hr(p->Close());return p;};
    auto owner=std::make_unique<TypedLists>();auto& d=*owner;auto first=make();const auto key=identity(first.Get(),1);const auto refs=references(first.Get());
    auto current=[](List saved){return saved;};
    d.boundary(first.Get(),key,2,TypedLists::ok);check(d.held()==1&&references(first.Get())==refs+1);
    auto found=d.lookup(key.raw,2,current);check(found.reason==0&&same(found.saved,key)&&d.pin(found));
    check(d.pinned==1&&references(first.Get())==refs+2);
    auto second=make();const auto key2=identity(second.Get(),2);d.boundary(second.Get(),key2,2,TypedLists::ok);check(d.pin(d.lookup(key2.raw,2,current)));
    // More than the observed 575 distinct non-NGX lists: only 64 candidate
    // leases survive. The two accepted Eval identities remain strongly held.
    for(unsigned i=0;i<600;++i){auto p=make();d.boundary(p.Get(),identity(p.Get(),3+i),2,TypedLists::ok);check(d.held()<=TypedLists::capacity+2);}
    check(d.evicted==538&&d.held()==66&&d.pinned==2);
    check(d.lookup(key.raw,2,current).reason==TypedLists::miss&&references(first.Get())==refs+1);
    check(same(d.pins[0].list,key)&&same(d.pins[1].list,key2)&&!d.pin(found));
    d.release();check(!d.held()&&references(first.Get())==refs);
    // Explicit successful, closed, failed and untracked typed boundaries.
    d.boundary(first.Get(),key,2,TypedLists::closed);check(d.lookup(key.raw,2,current).reason==TypedLists::closed);
    auto next=key;++next.generation;d.boundary(first.Get(),next,2,TypedLists::ok);check(d.lookup(key.raw,2,current).reason==0);
    d.boundary(first.Get(),next,2,TypedLists::boundary_failed);check(d.lookup(key.raw,2,current).reason==TypedLists::boundary_failed);
    d.boundary(first.Get(),next,2,TypedLists::ok);
    check(d.lookup(key.raw,3,current).reason==TypedLists::world_mismatch);
    check(d.lookup(key.raw,2,[](List){return List{};}).reason==TypedLists::generation_missing);
    check(d.lookup(key.raw,2,[](List value){++value.generation;return value;}).reason==TypedLists::generation_changed);
    next.generation=0;d.boundary(first.Get(),next,2,TypedLists::ok);check(d.lookup(key.raw,2,current).reason==TypedLists::generation_missing);
    next.generation=9;d.boundary(first.Get(),next,3,TypedLists::ok);check(d.pin(d.lookup(key.raw,3,current)));
    ++next.generation;d.boundary(first.Get(),next,3,TypedLists::ok);
    check(d.pins[0].list.generation==9&&d.pins[0].reason==TypedLists::boundary_failed&&d.lookup(key.raw,3,current).saved.generation==10);
    d.release();check(references(first.Get())==refs);
    std::printf("NGX typed list join tests passed: %u checks (real D3D typed object lifetimes; synthetic tracker generations)\n",checks);return 0;
}catch(std::exception const& e){std::fprintf(stderr,"%s (%u)\n",e.what(),checks);return 1;}}
