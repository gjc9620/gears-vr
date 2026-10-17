#include "vr_dof_compat.h"
#include <cassert>
#include <cstdio>
#include <limits>
int main(){using namespace gears_vr_dof;
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
    std::puts("VrDofCompatTest passed");
}
