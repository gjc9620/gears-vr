#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <type_traits>

// Read-only, bounded snapshots of the observed source+0x140 map. Payloads are
// opaque borrowed values: never dereference, allocate, release or refcount them.
namespace gears_source_map {
inline constexpr unsigned max_nodes=128,max_buckets=256;
inline constexpr std::size_t header_size=24,node_size=40;
static_assert(sizeof(std::uintptr_t)==8,"Observed map format is AMD64 only");
struct Header {std::uintptr_t head{},buckets{};std::int32_t count{},bucket_count{};};
struct Node {std::uintptr_t address{},payload{},hash_prev_link{},hash_next{},list_prev_link{},list_next{};};
struct Snapshot {bool valid{};std::uintptr_t header_address{};Header header{};
    unsigned node_count{},bucket_count{};
    std::array<Node,max_nodes> nodes{};
    std::array<std::uintptr_t,max_buckets> buckets{};};
inline bool range_valid(std::uintptr_t start,std::size_t size) noexcept {
    return start&&size&&start<=(std::numeric_limits<std::uintptr_t>::max)()-size;
}
inline bool overlap(std::uintptr_t a,std::size_t as,std::uintptr_t b,std::size_t bs) noexcept {
    // Invalid/wrapping ranges fail closed rather than appear independent.
    return !range_valid(a,as)||!range_valid(b,bs)||(a<b+bs&&b<a+as);
}
template<class T> inline T field(const unsigned char* bytes,std::size_t offset) noexcept {
    T value{};std::memcpy(&value,bytes+offset,sizeof(value));return value;
}
inline bool equal(Header const& a,Header const& b) noexcept {
    return a.head==b.head&&a.buckets==b.buckets&&a.count==b.count&&a.bucket_count==b.bucket_count;
}
inline bool equal(Node const& a,Node const& b) noexcept {
    return a.address==b.address&&a.payload==b.payload&&a.hash_prev_link==b.hash_prev_link&&
        a.hash_next==b.hash_next&&a.list_prev_link==b.list_prev_link&&a.list_next==b.list_next;
}
// callback(void* destination,const void* address,size_t bytes) must return true
// only for a complete read. copied_header, when supplied, is a readable local
// 24-byte snapshot, not a live target pointer. Allocate Snapshot on the heap in
// callers with tight stack budgets. A failed/throwing read leaves valid=false.
template<class Read> bool read(Read callback,std::uintptr_t address,Snapshot& out,
    const void* copied_header=nullptr) noexcept {
    out.valid=false;out.header_address=address;out.header={};out.node_count=out.bucket_count=0;
    try {
        if constexpr(std::is_pointer_v<Read>)if(!callback)return false;
        if(!range_valid(address,header_size))return false;
        std::array<unsigned char,header_size> raw{};
        if(copied_header)std::memcpy(raw.data(),copied_header,raw.size());
        else if(!callback(raw.data(),reinterpret_cast<const void*>(address),raw.size()))return false;
        auto& h=out.header;
        h.head=field<std::uintptr_t>(raw.data(),0);h.count=field<std::int32_t>(raw.data(),8);
        h.buckets=field<std::uintptr_t>(raw.data(),12);h.bucket_count=field<std::int32_t>(raw.data(),20);
        if(h.count<0||h.count>int(max_nodes)||(h.head==0)!=(h.count==0)||h.bucket_count<=0||
            h.bucket_count>int(max_buckets)||(unsigned(h.bucket_count)&(unsigned(h.bucket_count)-1)))return false;
        const auto bucket_bytes=std::size_t(h.bucket_count)*sizeof(std::uintptr_t);
        if(!range_valid(h.buckets,bucket_bytes)||overlap(address,header_size,h.buckets,bucket_bytes))return false;
        out.bucket_count=unsigned(h.bucket_count);
        if(!callback(out.buckets.data(),reinterpret_cast<const void*>(h.buckets),bucket_bytes))return false;
        auto current=h.head,previous=address;
        for(int i=0;i<h.count;++i){
            if(!range_valid(current,node_size)||overlap(current,node_size,address,header_size)||
                overlap(current,node_size,h.buckets,bucket_bytes))return false;
            for(unsigned j=0;j<out.node_count;++j)
                if(overlap(current,node_size,out.nodes[j].address,node_size))return false;
            std::array<unsigned char,node_size> bytes{};
            if(!callback(bytes.data(),reinterpret_cast<const void*>(current),bytes.size()))return false;
            Node n{current,field<std::uintptr_t>(bytes.data(),0),field<std::uintptr_t>(bytes.data(),8),
                field<std::uintptr_t>(bytes.data(),16),field<std::uintptr_t>(bytes.data(),24),
                field<std::uintptr_t>(bytes.data(),32)};
            if(n.list_prev_link!=previous)return false;
            out.nodes[out.node_count++]=n;previous=current+32;current=n.list_next;
        }
        if(current)return false; // Exact declared count; also catches a tail cycle.
        std::array<bool,max_nodes> covered{};unsigned covered_count{};
        for(unsigned bucket=0;bucket<out.bucket_count;++bucket){
            current=out.buckets[bucket];previous=h.buckets+bucket*sizeof(std::uintptr_t);
            while(current){
                unsigned index=0;
                while(index<out.node_count&&out.nodes[index].address!=current)++index;
                if(index==out.node_count||covered[index])return false;
                auto const& n=out.nodes[index];
                if(n.hash_prev_link!=previous)return false;
                covered[index]=true;++covered_count;previous=n.address+16;current=n.hash_next;
            }
        }
        if(covered_count!=out.node_count)return false;
        out.valid=true;return true;
    }catch(...){return false;}
}
inline bool sane(Snapshot const& s) noexcept {
    return s.valid&&s.node_count<=max_nodes&&s.bucket_count>0&&s.bucket_count<=max_buckets&&
        s.header.count==int(s.node_count)&&s.header.bucket_count==int(s.bucket_count)&&
        range_valid(s.header_address,header_size)&&range_valid(s.header.buckets,std::size_t(s.bucket_count)*8);
}
inline bool same_contents(Snapshot const& a,Snapshot const& b) noexcept {
    if(!sane(a)||!sane(b)||a.node_count!=b.node_count)return false;
    std::array<bool,max_nodes> used{};
    for(unsigned i=0;i<a.node_count;++i){unsigned j=0;
        while(j<b.node_count&&(used[j]||a.nodes[i].payload!=b.nodes[j].payload))++j;
        if(j==b.node_count)return false;used[j]=true;
    }
    return true;
}
inline bool independent(Snapshot const& a,Snapshot const& b) noexcept {
    if(!sane(a)||!sane(b))return false;
    const auto ab=std::size_t(a.bucket_count)*8,bb=std::size_t(b.bucket_count)*8;
    if(overlap(a.header_address,header_size,b.header_address,header_size)||
        overlap(a.header.buckets,ab,b.header.buckets,bb)||
        overlap(a.header.buckets,ab,b.header_address,header_size)||
        overlap(b.header.buckets,bb,a.header_address,header_size))return false;
    for(unsigned i=0;i<a.node_count;++i){auto address=a.nodes[i].address;
        if(overlap(address,node_size,b.header_address,header_size)||overlap(address,node_size,b.header.buckets,bb))return false;
        for(unsigned j=0;j<b.node_count;++j)if(overlap(address,node_size,b.nodes[j].address,node_size))return false;
    }
    for(unsigned j=0;j<b.node_count;++j)
        if(overlap(b.nodes[j].address,node_size,a.header_address,header_size)||
            overlap(b.nodes[j].address,node_size,a.header.buckets,ab))return false;
    return true;
}
inline bool unchanged(Snapshot const& a,Snapshot const& b) noexcept {
    if(!sane(a)||!sane(b)||a.header_address!=b.header_address||!equal(a.header,b.header)||
        a.node_count!=b.node_count||a.bucket_count!=b.bucket_count)return false;
    for(unsigned i=0;i<a.bucket_count;++i)if(a.buckets[i]!=b.buckets[i])return false;
    for(unsigned i=0;i<a.node_count;++i)if(!equal(a.nodes[i],b.nodes[i]))return false;
    return true;
}
}
