#include "vr_dof_compat.h"
#include <cassert>
#include <cstdio>
#include <limits>
int main(){using namespace gears_vr_dof;
    for(unsigned bits=0;bits<8;++bits){const bool owned=bits&1,bindings=bits&2,runtime=bits&4;
        assert(transport_supported(owned,bindings,runtime)==((!owned&&!bindings)||(owned&&bindings&&runtime)));}
    assert(quad_caller(0xd808d8)&&quad_caller(0xd80a7b));
    assert(!quad_caller(0)&&!quad_caller(0xd803bf)&&!quad_caller(0xd808d9)&&!quad_caller(0xd80a7a));
    Float4 source{1,-0.0f,2,3};Descriptor descriptor{0,192,16};assert(supported(descriptor,0,source));
    const auto copy=forwarded(source);assert(source[0]==1&&copy[0]==0&&std::memcmp(copy.data()+1,source.data()+1,12)==0);
    unsigned calls{};auto original=[&](uintptr_t rcx,const uint16_t* d,const void* data,unsigned index)->uint64_t{
        ++calls;assert(rcx==9&&d==descriptor.data()&&index==0);assert(static_cast<const float*>(data)[0]==0);return 0x123456789abcdef0ull;};
    assert(call_setter(original,9,descriptor.data(),source.data(),0,copy,true)==0x123456789abcdef0ull&&calls==1&&source[0]==1);
    source[0]=0;assert(supported(descriptor,0,source)&&equal(forwarded(source),source));
    assert(call_setter([&](uintptr_t,const uint16_t*,const void* data,unsigned)->uint64_t{assert(data==source.data());return 7;},9,descriptor.data(),source.data(),0,copy,false)==7);
    source[0]=2;assert(!supported(descriptor,0,source));source[0]=1;source[2]=std::numeric_limits<float>::infinity();assert(!supported(descriptor,0,source));source[2]=2;
    descriptor[1]=191;assert(!supported(descriptor,0,source));descriptor[1]=192;assert(!supported(descriptor,1,source));
    unsigned eye{};assert(native_eye(0x10000,0x10000,2,eye)&&eye==0&&native_eye(0x10d30,0x10000,2,eye)&&eye==1);
    assert(!native_eye(0x10001,0x10000,2,eye)&&!native_eye(0x11a60,0x10000,2,eye)&&!native_eye(0x10000,0x10000,1,eye));
    Store<> store;Key no_site{1,1};assert(store.decision(no_site)==Decision::missing);assert(store.begin(no_site));
    assert(store.decision(no_site)==Decision::incomplete);store.finish(no_site);assert(store.decision(no_site)==Decision::no_site);
    assert(store.decision({2,1})==Decision::missing);Key site{1,2};auto* e=store.begin(site);assert(e&&store.participate(site));store.finish(site);assert(store.decision(site)==Decision::rejected);
    Key good{1,3};e=store.begin(good);for(unsigned i=0;i<2;++i){assert(e->reserve(i,true));e->proved(i,true,true);assert(e->reserve(i,false));e->proved(i,false,true);}
    store.finish(good);assert(store.decision(good)==Decision::zero_proved);
    assert(!e->reserve(0,true)&&store.decision(good)==Decision::rejected);
    Key unsupported{1,4};e=store.begin(unsupported);store.participate(unsupported);e->fail(Failure::unsupported);e->proved(0,true,true);store.finish(unsupported);assert(store.decision(unsupported)==Decision::rejected);
    store.retire_window();assert(store.retired==4&&!store.begin(no_site)&&!store.participate(good)&&store.decision(good)==Decision::missing);
    Store<> bounded;for(unsigned i=1;i<=128;++i)assert(bounded.begin({1,i}));assert(!bounded.begin({1,129})&&bounded.capacity_lost&&bounded.overflow==1);
    bounded.retire_window();assert(bounded.retired==129&&bounded.decision({1,130})==Decision::overflow&&!bounded.begin({1,130}));
    Store<> sticky;e=sticky.begin({1,1});assert(e->reserve(0,true));assert(!e->reserve(0,true));e->proved(0,true,true);e->proved(1,true,true);e->proved(0,false,true);e->proved(1,false,true);sticky.finish({1,1});assert(sticky.decision({1,1})==Decision::rejected);
    Store<> failed;e=failed.begin({1,1});failed.participate({1,1});e->fail(Failure::identity_changed);failed.finish({1,1});assert(failed.decision({1,1})==Decision::rejected);
    assert(!failed.begin({0,2}));Store<> worlds;assert(worlds.begin({1,1}));assert(!worlds.begin({2,1}));worlds.finish({1,1});assert(worlds.decision({1,1})==Decision::rejected&&worlds.decision({2,1})==Decision::missing);
    Store<> reads;e=reads.begin({1,1});reads.participate({1,1});e->fail(Failure::read_failed);reads.finish({1,1});assert(reads.decision({1,1})==Decision::rejected);
    Store<> one_eye;e=one_eye.begin({1,1});assert(e->reserve(0,true));e->proved(0,true,true);assert(e->reserve(0,false));e->proved(0,false,true);one_eye.finish({1,1});assert(one_eye.decision({1,1})==Decision::rejected);
    Store<> missing_setter;e=missing_setter.begin({1,1});e->proved(0,true,true);missing_setter.finish({1,1});assert(missing_setter.decision({1,1})==Decision::rejected);
    Store<> missing_quad;e=missing_quad.begin({1,1});
    for(unsigned i=0;i<2;++i){assert(e->reserve(i,true));e->proved(i,true,true);}
    missing_quad.finish({1,1});assert(missing_quad.decision({1,1})==Decision::rejected);
    for(uintptr_t caller:{uintptr_t(0xd808d8),uintptr_t(0xd80a7b)}){Store<> accepted_site;e=accepted_site.begin({1,1});
        for(unsigned i=0;i<2;++i){assert(e->reserve(i,true));e->proved(i,true,true);
            assert(quad_caller(caller)&&e->reserve(i,false));e->proved(i,false,true);}
        accepted_site.finish({1,1});assert(accepted_site.decision({1,1})==Decision::zero_proved);
    }
    for(unsigned order=0;order<2;++order){Store<> shared;e=shared.begin({1,1});
        const uintptr_t callers[]{0xd808d8,0xd80a7b};
        for(unsigned i=0;i<2;++i){assert(e->reserve(i,true));e->proved(i,true,true);}
        assert(quad_caller(callers[order])&&e->reserve(0,false));e->proved(0,false,true);
        assert(quad_caller(callers[1-order])&&!e->reserve(0,false)); // Second site cannot supply another allowance.
        assert(e->failure==Failure::duplicate_quad);e->proved(0,false,true);
        e->reserve(1,false);e->proved(1,false,true);shared.finish({1,1});assert(shared.decision({1,1})==Decision::rejected);
    }
    // Owned Canvas can complete after desktop Present: partial retirement must
    // retain the later ticket and cannot silently claim an unobserved boundary.
    Store<> deferred;deferred.begin({1,1});deferred.finish({1,1});deferred.begin({1,2});deferred.finish({1,2});
    assert(deferred.retire_through(1)&&deferred.decision({1,1})==Decision::missing&&deferred.decision({1,2})==Decision::no_site);
    assert(!deferred.retire_through(3)&&deferred.decision({1,2})==Decision::no_site);
    assert(deferred.retire_through(2)&&deferred.decision({1,2})==Decision::missing&&!deferred.retire_through(1));
    Store<> sustained;
    for(unsigned id=1;id<=2000;++id){auto* entry=sustained.begin({1,id});assert(entry);
        for(unsigned eye_index=0;eye_index<2;++eye_index){assert(entry->reserve(eye_index,true));entry->proved(eye_index,true,true);
            assert(entry->reserve(eye_index,false));entry->proved(eye_index,false,true);}
        sustained.finish({1,id});assert(sustained.decision({1,id})==Decision::zero_proved);
        assert(sustained.retire_through(id)&&!sustained.capacity_lost);
    }
    std::puts("VrDofCompatTest passed: transport policy, exact proof, deferred owned retirement and 2000 publications");
}
