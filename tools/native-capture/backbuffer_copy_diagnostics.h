#pragma once
#include <d3d12.h>
#include <array>
#include <cstdint>

namespace gears_backbuffer_copy {
struct Location {
    unsigned type{},subresource{};uint64_t offset{};
    std::array<unsigned,5> footprint{};
    bool operator==(Location const& b)const{return type==b.type&&subresource==b.subresource&&offset==b.offset&&footprint==b.footprint;}
};
struct Key {
    uintptr_t source{},destination{},caller{};Location src{},dst{};
    std::array<unsigned,3> xyz{};bool has_box{};std::array<unsigned,6> box{};
    unsigned source_match{},destination_match{};
    bool operator==(Key const& b)const{return source==b.source&&destination==b.destination&&caller==b.caller&&src==b.src&&dst==b.dst&&
        xyz==b.xyz&&has_box==b.has_box&&box==b.box&&source_match==b.source_match&&destination_match==b.destination_match;}
};
inline bool decode_location(D3D12_TEXTURE_COPY_LOCATION const& input,Location& out){
    if(!input.pResource)return false;
    out={};out.type=unsigned(input.Type);
    if(input.Type==D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX){out.subresource=input.SubresourceIndex;return true;}
    if(input.Type!=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT)return false;
    out.subresource=UINT32_MAX;out.offset=input.PlacedFootprint.Offset;const auto& f=input.PlacedFootprint.Footprint;
    out.footprint={unsigned(f.Format),f.Width,f.Height,f.Depth,f.RowPitch};return true;
}
// Pure parser of caller-copied API arguments; never dereferences pResource.
inline bool decode(D3D12_TEXTURE_COPY_LOCATION const& source,D3D12_TEXTURE_COPY_LOCATION const& destination,
    unsigned x,unsigned y,unsigned z,D3D12_BOX const* box,Key& out){
    Key value{};if(!decode_location(source,value.src)||!decode_location(destination,value.dst))return false;
    value.xyz={x,y,z};value.has_box=box!=nullptr;
    if(box)value.box={box->left,box->top,box->front,box->right,box->bottom,box->back};out=value;return true;
}
struct Epoch {
    uint64_t world{},inventory{};uintptr_t chain{};
    bool operator==(Epoch const& b)const{return world==b.world&&inventory==b.inventory&&chain==b.chain;}
};
inline unsigned match_identity(uintptr_t identity,bool known,const uintptr_t* rejected,unsigned count){
    if(!identity)return 0;unsigned flags=known?1u:0u;
    for(unsigned i=0;i<count;++i)if(rejected[i]&&rejected[i]==identity)flags|=2u;
    return flags;
}
template<class Payload> struct Row {
    Epoch epoch{};Key key{};Payload payload{};uint64_t count{},last_candidate_ticket{};
    long long first_qpc{},last_qpc{};
};
template<class Payload> class Recorder {
public:
    static constexpr unsigned capacity=32,per_epoch=8;
    std::array<Row<Payload>,capacity> rows{};unsigned size{};uint64_t dropped{};
    Row<Payload>* admit(Epoch epoch,Key const& key,uint64_t candidate_ticket,long long qpc){
        unsigned epoch_rows{};
        for(unsigned i=0;i<size;++i){auto& row=rows[i];if(!(row.epoch==epoch))continue;
            ++epoch_rows;if(row.key==key){++row.count;row.last_candidate_ticket=candidate_ticket;row.last_qpc=qpc;return nullptr;}}
        if(epoch_rows==per_epoch||size==capacity){++dropped;return nullptr;}
        auto& row=rows[size++];row.epoch=epoch;row.key=key;row.count=1;row.last_candidate_ticket=candidate_ticket;
        row.first_qpc=row.last_qpc=qpc;return &row;
    }
};
}
