#pragma once
#include "cover_physics_bone_map.h"
#include "cover_bone_getters.h"

// Read-only correspondence with the ORIGINAL MatchRefBone cache, never a call.
// Offsets/key/chain/value flow are from its complete certified main/cold code.
// The caller supplies the existing current C0/asset/layout/owner guard. Equal
// repeated reads do not establish an atomic pose or a physics-world fence.
namespace gears_cover_engine_bone_map {
namespace named=gears_cover_physics_bones;namespace body=named::body;
constexpr unsigned max_buckets=1024;
enum class Status {okay,request,alias,model,context,code,read,header,changed,chain,name,index,coverage};
struct Workspace {
    std::array<unsigned char,16> reference_header{},reference_header_repeat{};
    std::array<unsigned char,28> map_header{},map_header_repeat{};
    std::array<body::rig::ReferenceRecord68,body::bones> reference{},reference_repeat{};
    std::array<unsigned char,body::bones*16> pairs{},pairs_repeat{};
    std::array<std::int32_t,max_buckets> buckets{},buckets_repeat{};
};
struct Observation {
    std::uintptr_t asset{},reference_data{},pair_data{},bucket_data{};
    unsigned matched_bones{},bucket_count{};
    bool cache_matches_owned_reference{},closing_checked{};
    bool getter_called{},physical_pose_verified{},world_sync_verified{},activation_allowed{};
};
namespace detail {
template<class T,std::size_t N> inline T get(std::array<unsigned char,N> const& data,std::size_t offset) noexcept {
    T out{};std::memcpy(&out,data.data()+offset,sizeof out);return out;
}
inline bool span(std::uintptr_t address,std::size_t bytes,unsigned alignment) noexcept {
    return address&&bytes&&!(address&(alignment-1))&&bytes-1<=UINTPTR_MAX-address;
}
}
template<class Read,class Guard> inline Status observe(std::uintptr_t module_base,std::uintptr_t asset,
    body::Model const& model,body::Frame const& frame,Read read,Guard guard,Workspace& work,Observation& output) {
    if(!module_base||!detail::span(asset,0xd4,8))return Status::request;
    const body::d::Range owned[]={body::d::range(&model,sizeof model),body::d::range(&frame,sizeof frame),
        body::d::range(&work,sizeof work),body::d::range(&output,sizeof output)};
    if(!body::d::disjoint(owned,4))return Status::alias;
    if(!named::detail::geometry(model,frame))return Status::model;
    if(!guard())return Status::context;
    if(!gears_cover_bones::certify(module_base,read))return Status::code;
    if(!read(work.reference_header.data(),reinterpret_cast<void const*>(asset+0xa4),16)||
       !read(work.map_header.data(),reinterpret_cast<void const*>(asset+0xb8),28))return Status::read;
    const auto reference=detail::get<std::uintptr_t>(work.reference_header,0);
    const auto count=detail::get<std::int32_t>(work.reference_header,8);
    const auto capacity=detail::get<std::int32_t>(work.reference_header,12);
    const auto pairs=detail::get<std::uintptr_t>(work.map_header,0);
    const auto pair_count=detail::get<std::int32_t>(work.map_header,8);
    const auto buckets=detail::get<std::uintptr_t>(work.map_header,16);
    const auto bucket_count=detail::get<std::int32_t>(work.map_header,24);
    // Map+12 is retained as opaque bytes. MatchRefBone does not consume it;
    // no guessed TMap capacity/sparse-allocation layout is used here.
    if(count!=body::bones||capacity<count||capacity>512||pair_count!=body::bones||
       bucket_count<=0||bucket_count>int(max_buckets)||(bucket_count&(bucket_count-1))||
       !detail::span(reference,std::size_t(capacity)*68,4)||
       !detail::span(pairs,std::size_t(pair_count)*16,4)||
       !detail::span(buckets,std::size_t(bucket_count)*4,4))return Status::header;
    const body::d::Range sources[]={owned[0],owned[1],owned[2],owned[3],
        {asset+0xa4,16},{asset+0xb8,28},{reference,sizeof work.reference},
        {pairs,sizeof work.pairs},{buckets,std::size_t(bucket_count)*4}};
    if(!body::d::disjoint(sources,9))return Status::alias;
    if(!read(work.reference.data(),reinterpret_cast<void const*>(reference),sizeof work.reference)||
       !read(work.pairs.data(),reinterpret_cast<void const*>(pairs),sizeof work.pairs)||
       !read(work.buckets.data(),reinterpret_cast<void const*>(buckets),std::size_t(bucket_count)*4))return Status::read;
    if(std::memcmp(work.reference.data(),frame.reference.data(),sizeof work.reference))return Status::changed;
    std::array<bool,body::bones> visited{},mapped{};unsigned total{};
    for(int bucket=0;bucket<bucket_count;++bucket){
        auto entry=work.buckets[unsigned(bucket)];
        while(entry!=-1){
            if(entry<0||entry>=pair_count||visited[unsigned(entry)])return Status::chain;
            visited[unsigned(entry)]=true;++total;
            const auto at=std::size_t(entry)*16;
            const auto next=detail::get<std::int32_t>(work.pairs,at);
            const named::Name key{detail::get<std::int32_t>(work.pairs,at+4),detail::get<std::uint32_t>(work.pairs,at+8)};
            const auto index=detail::get<std::uint32_t>(work.pairs,at+12);
            if(key.index<=0||(unsigned(key.index)&unsigned(bucket_count-1))!=unsigned(bucket))return Status::name;
            if(index>=body::bones||mapped[index])return Status::index;
            if(!named::same(key,named::detail::name(frame.reference[index])))return Status::name;
            mapped[index]=true;entry=next;
        }
    }
    if(total!=body::bones)return Status::coverage;
    if(!guard())return Status::context;
    if(!gears_cover_bones::certify(module_base,read))return Status::code;
    if(!read(work.reference_header_repeat.data(),reinterpret_cast<void const*>(asset+0xa4),16)||
       !read(work.map_header_repeat.data(),reinterpret_cast<void const*>(asset+0xb8),28)||
       !read(work.reference_repeat.data(),reinterpret_cast<void const*>(reference),sizeof work.reference_repeat)||
       !read(work.pairs_repeat.data(),reinterpret_cast<void const*>(pairs),sizeof work.pairs_repeat)||
       !read(work.buckets_repeat.data(),reinterpret_cast<void const*>(buckets),std::size_t(bucket_count)*4))return Status::read;
    if(work.reference_header_repeat!=work.reference_header||work.map_header_repeat!=work.map_header||
       std::memcmp(work.reference_repeat.data(),work.reference.data(),sizeof work.reference)||work.pairs_repeat!=work.pairs||
       std::memcmp(work.buckets_repeat.data(),work.buckets.data(),std::size_t(bucket_count)*4))return Status::changed;
    if(!guard())return Status::context;
    output={asset,reference,pairs,buckets,total,unsigned(bucket_count),true,true,false,false,false,false};
    return Status::okay;
}
} // namespace gears_cover_engine_bone_map
