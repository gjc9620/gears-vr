#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include "gpu_viewport_stats.h"

// Pure bounded observation storage. Caller serializes access; no acceptance gates.
namespace gears_late_quad {
// Recorder admission only; this does not classify or authorize a final draw.
inline bool observe_target(uintptr_t quad_caller,bool known_buffer,bool final_eye_valid,uint64_t ticket){
    // Live repro 11028 also uses d83cbf for paired CPU quads. Observe its
    // intermediate target/source without granting final-eye publication.
    return quad_caller==0x2b11a0||quad_caller==0xd83cbf||(known_buffer&&!(final_eye_valid&&ticket));
}
struct Epoch {
    uint64_t world{},inventory{};uintptr_t chain{};
    bool operator==(Epoch const& b)const{return world==b.world&&inventory==b.inventory&&chain==b.chain;}
};
struct Key {
    std::array<float,8> args{};std::array<unsigned,4> dimensions{};
    uintptr_t context{},quad_caller{},draw_caller{};unsigned read_mask{},slot_count{},kind{},elements{},instances{},targets{};
    std::array<uintptr_t,32> objects{};
    gears_viewport::Target target{};gears_viewport::DrawState rectangles{};
    bool known_buffer{},descriptor_capacity_lost{};
    bool operator==(Key const& b)const{
        return std::memcmp(args.data(),b.args.data(),sizeof(args))==0&&dimensions==b.dimensions&&context==b.context&&
            quad_caller==b.quad_caller&&draw_caller==b.draw_caller&&read_mask==b.read_mask&&slot_count==b.slot_count&&kind==b.kind&&
            elements==b.elements&&instances==b.instances&&targets==b.targets&&objects==b.objects&&target==b.target&&
            rectangles.tracked==b.rectangles.tracked&&rectangles.viewport==b.rectangles.viewport&&
            rectangles.scissor==b.rectangles.scissor&&rectangles.viewport_count==b.rectangles.viewport_count&&
            rectangles.scissor_count==b.rectangles.scissor_count&&rectangles.depth_known==b.rectangles.depth_known&&
            rectangles.min_depth==b.rectangles.min_depth&&rectangles.max_depth==b.rectangles.max_depth&&
            known_buffer==b.known_buffer&&descriptor_capacity_lost==b.descriptor_capacity_lost;
    }
};
template<class Payload> struct Row {
    Epoch epoch{};Key key{};Payload payload{};uint64_t count{},last_ticket{};
    long long first_qpc{},last_qpc{};
};
template<class Payload> class Recorder {
public:
    static constexpr unsigned capacity=32,per_epoch=8;
    std::array<Row<Payload>,capacity> rows{};unsigned size{};uint64_t dropped{};
    // A new row is the only admission to expensive identity queries. Returning
    // nullptr for duplicates also avoids per-frame COM calls after coalescing.
    Row<Payload>* admit(Epoch epoch,Key const& key,uint64_t ticket,long long qpc){
        unsigned epoch_rows{};
        for(unsigned i=0;i<size;++i){auto& row=rows[i];if(!(row.epoch==epoch))continue;
            ++epoch_rows;if(row.key==key){++row.count;row.last_ticket=ticket;row.last_qpc=qpc;return nullptr;}}
        if(epoch_rows==per_epoch||size==capacity){++dropped;return nullptr;}
        auto& row=rows[size++];row.epoch=epoch;row.key=key;row.count=1;row.last_ticket=ticket;
        row.first_qpc=row.last_qpc=qpc;return &row;
    }
};
}
