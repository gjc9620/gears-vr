#include "source_map_snapshot.h"
#include <algorithm>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <vector>
#define CHECK(c) do{if(!(c)){std::printf("failed %d: %s\n",__LINE__,#c);return __LINE__;}}while(0)
using namespace gears_source_map;
struct Memory {
    std::vector<unsigned char> bytes=std::vector<unsigned char>(65536);
    unsigned reads{};std::uintptr_t failed_address{};
    template<class T> void put(std::uintptr_t address,T value){std::memcpy(bytes.data()+address,&value,sizeof(value));}
    bool read(void* output,const void* input,std::size_t size){
        ++reads;auto address=reinterpret_cast<std::uintptr_t>(input);
        if(address==failed_address||address>bytes.size()||size>bytes.size()-address)return false;
        std::memcpy(output,bytes.data()+address,size);return true;
    }
    void map(std::uintptr_t header,std::uintptr_t bucket,const std::vector<std::uintptr_t>& nodes,
        const std::vector<std::uintptr_t>& payloads){
        put(header,nodes.empty()?std::uintptr_t(0):nodes[0]);put(header+8,std::int32_t(nodes.size()));
        put(header+12,bucket);put(header+20,std::int32_t(4));
        for(unsigned i=0;i<4;++i)put(bucket+i*8,std::uintptr_t(0));
        if(!nodes.empty())put(bucket,nodes[0]);
        for(unsigned i=0;i<nodes.size();++i){auto node=nodes[i];
            put(node,payloads[i]);put(node+8,i?nodes[i-1]+16:bucket);
            put(node+16,i+1<nodes.size()?nodes[i+1]:std::uintptr_t(0));
            put(node+24,i?nodes[i-1]+32:header);
            put(node+32,i+1<nodes.size()?nodes[i+1]:std::uintptr_t(0));
        }
    }
};
int main(){
    Memory mem;auto a=std::make_unique<Snapshot>(),b=std::make_unique<Snapshot>(),after=std::make_unique<Snapshot>();
    auto read_map=[&](std::uintptr_t address,Snapshot& out,const void* header=nullptr){
        return gears_source_map::read([&](void* dst,const void* src,std::size_t n){return mem.read(dst,src,n);},address,out,header);
    };
    auto reset=[&](){std::fill(mem.bytes.begin(),mem.bytes.end(),static_cast<unsigned char>(0));mem.failed_address=0;mem.reads=0;
        // Repeated/null/invalid-looking payloads are borrowed opaque values.
        mem.map(0x100,0x200,{0x300,0x400,0x500},{0xfeed,0xfeed,0});
        mem.map(0x1000,0x1100,{0x1500,0x1400,0x1300},{0,0xfeed,0xfeed});
    };
    reset();CHECK(read_map(0x100,*a));CHECK(read_map(0x1000,*b));
    CHECK(a->node_count==3&&a->bucket_count==4&&same_contents(*a,*b)&&independent(*a,*b));
    CHECK(!unchanged(*a,*b));CHECK(read_map(0x100,*after)&&unchanged(*a,*after));
    CHECK(mem.reads==15); // header,buckets,3nodes per read; no payload reads.
    auto copied=std::array<unsigned char,24>{};std::memcpy(copied.data(),mem.bytes.data()+0x100,24);
    mem.failed_address=0x100;CHECK(read_map(0x100,*after,copied.data())&&unchanged(*a,*after));
    CHECK(!read_map(0x100,*after)&&!after->valid&&!same_contents(*a,*after));
    for(auto address:{std::uintptr_t(0x200),std::uintptr_t(0x400)}){
        reset();mem.failed_address=address;CHECK(!read_map(0x100,*after)&&!after->valid);
    }
    CHECK(!gears_source_map::read([](void*,const void*,std::size_t)->bool{throw std::runtime_error("read failed");},0x100,*after));
    bool (*missing_read)(void*,const void*,std::size_t)=nullptr;
    CHECK(!gears_source_map::read(missing_read,0x100,*after)&&!after->valid);
    reset();mem.put(0x100+8,std::int32_t(2));CHECK(!read_map(0x100,*after));
    reset();mem.put(0x100+8,std::int32_t(4));CHECK(!read_map(0x100,*after));
    reset();mem.put(0x100,std::uintptr_t(0));CHECK(!read_map(0x100,*after));
    reset();mem.put(0x100+8,std::int32_t(0));CHECK(!read_map(0x100,*after));
    for(auto count:{-1,129}){reset();mem.put(0x100+8,std::int32_t(count));CHECK(!read_map(0x100,*after));}
    for(auto count:{0,-1,3,512}){reset();mem.put(0x100+20,std::int32_t(count));CHECK(!read_map(0x100,*after));}
    reset();mem.put(0x500+32,std::uintptr_t(0x300));CHECK(!read_map(0x100,*after)); // list cycle
    reset();mem.put(0x400+32,std::uintptr_t(0x400));CHECK(!read_map(0x100,*after)); // duplicate list node
    reset();mem.put(0x300+24,std::uintptr_t(0x108));CHECK(!read_map(0x100,*after)); // first prev link
    reset();mem.put(0x400+24,std::uintptr_t(0x300));CHECK(!read_map(0x100,*after)); // later prev link
    reset();mem.put(0x500+16,std::uintptr_t(0x300));CHECK(!read_map(0x100,*after)); // hash cycle
    reset();mem.put(0x200+8,std::uintptr_t(0x300));CHECK(!read_map(0x100,*after)); // same node in2 buckets
    reset();mem.put(0x200,std::uintptr_t(0));CHECK(!read_map(0x100,*after)); // no hash coverage
    reset();mem.put(0x200,std::uintptr_t(0x600));CHECK(!read_map(0x100,*after)); // hash outside list
    reset();mem.put(0x300+8,std::uintptr_t(0x208));CHECK(!read_map(0x100,*after)); // first hash prev
    reset();mem.put(0x400+8,std::uintptr_t(0x300+32));CHECK(!read_map(0x100,*after)); // later hash prev
    reset();mem.put(0x100+12,std::uintptr_t(0x108));CHECK(!read_map(0x100,*after)); // buckets overlap header
    reset();mem.put(0x100,std::uintptr_t(0x108));CHECK(!read_map(0x100,*after)); // node overlaps header
    reset();mem.put(0x100,std::uintptr_t(0x208));CHECK(!read_map(0x100,*after)); // node overlaps bucket storage
    reset();mem.put(0x300+32,std::uintptr_t(0x310));CHECK(!read_map(0x100,*after)); // overlapping node ranges
    const auto maximum=(std::numeric_limits<std::uintptr_t>::max)();
    reset();CHECK(!read_map(maximum-8,*after));CHECK(!read_map(0,*after));
    reset();mem.put(0x100+12,maximum-8);CHECK(!read_map(0x100,*after));
    reset();mem.put(0x100,maximum-8);CHECK(!read_map(0x100,*after));
    reset();CHECK(read_map(0x100,*a));
    // Different headers, same complete owned node/bucket storage: each map can
    // be read validly at its own time but the owned snapshots are not independent.
    mem.map(0x1000,0x200,{0x300,0x400,0x500},{0xfeed,0xfeed,0});CHECK(read_map(0x1000,*b));
    CHECK(same_contents(*a,*b)&&!independent(*a,*b));
    reset();CHECK(read_map(0x100,*a));
    mem.map(0x1000,0x1100,{0x300,0x400,0x500},{0,0xfeed,0xfeed});CHECK(read_map(0x1000,*b));CHECK(!independent(*a,*b)); // cross-eye nodes
    reset();CHECK(read_map(0x100,*a));
    mem.map(0x200,0x1100,{0x1500,0x1400,0x1300},{0,0xfeed,0xfeed});CHECK(read_map(0x200,*b));CHECK(!independent(*a,*b)); // foreign header inA buckets
    reset();CHECK(read_map(0x100,*a));
    mem.map(0x310,0x1100,{0x1500,0x1400,0x1300},{0,0xfeed,0xfeed});CHECK(read_map(0x310,*b));CHECK(!independent(*a,*b)); // foreign header inA node
    reset();CHECK(read_map(0x100,*a));CHECK(read_map(0x1000,*b));
    mem.put(0x300,std::uintptr_t(0xbeef));CHECK(read_map(0x100,*after));CHECK(!unchanged(*a,*after)&&!same_contents(*a,*after));
    // Structural values can change while payload multiset stays the same.
    mem.map(0x100,0x200,{0x500,0x400,0x300},{0,0xfeed,0xfeed});CHECK(read_map(0x100,*after));
    CHECK(same_contents(*a,*after)&&!unchanged(*a,*after));
    reset();mem.map(0x100,0x200,{},{});CHECK(read_map(0x100,*a));CHECK(!a->node_count&&a->bucket_count==4);
    mem.put(0x200,std::uintptr_t(0x300));CHECK(!read_map(0x100,*after));
    // Full configured bounds, still a finite header+bucket+128-node read budget.
    std::fill(mem.bytes.begin(),mem.bytes.end(),static_cast<unsigned char>(0));std::vector<std::uintptr_t> nodes,payloads;
    for(unsigned i=0;i<128;++i){nodes.push_back(0x2000+i*40);payloads.push_back(i%4);}
    mem.map(0x100,0x400,nodes,payloads);mem.put(0x100+20,std::int32_t(256));mem.failed_address=0;mem.reads=0;
    CHECK(read_map(0x100,*a)&&a->node_count==128&&a->bucket_count==256&&mem.reads==130);
    std::printf("source map bounded read/list+hash coverage/range independence/opaque payload multiset/unchanged CPU checks passed\n");
    return 0;
}
