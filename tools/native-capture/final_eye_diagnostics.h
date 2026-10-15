#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include "gpu_viewport_stats.h"

// Caller holds frame_hook_mutex. Observations only; no acceptance policy.
namespace gears_final_eye_diagnostics {
struct Candidate {
    uintptr_t caller{};uint64_t ticket{};
    std::array<float,4> rect{};unsigned width{},height{};
};
struct Geometry {
    unsigned stage{},reason{};Candidate quad{};
    uintptr_t draw_caller{},resource{};uint64_t target_width{};
    unsigned target_height{},targets{},mip{},format{};
    bool eye_valid{};unsigned eye{},eye_width{},eye_height{};
    std::array<unsigned,4> eye_rect{};
    gears_viewport::BindingProvenance provenance{};
    bool operator==(Geometry const& b)const {
        // Ticket, list identity and recording generation vary every frame; keep
        // their first values in Row while coalescing the same failure geometry.
        const auto& p=provenance.trace;const auto& q=b.provenance.trace;
        // Copy writes may repeat every frame. Preserve first write/source
        // serials in Row while distinguishing actual Create lineage and epoch.
        const bool same_provenance=provenance.handle==b.provenance.handle&&p.origin_create_serial==q.origin_create_serial&&
            p.create_epoch==q.create_epoch&&p.last_write_epoch==q.last_write_epoch&&
            p.creation_live==q.creation_live&&p.copy_source_handle==q.copy_source_handle;
        return stage==b.stage&&reason==b.reason&&quad.caller==b.quad.caller&&
            std::memcmp(quad.rect.data(),b.quad.rect.data(),sizeof(quad.rect))==0&&
            quad.width==b.quad.width&&quad.height==b.quad.height&&draw_caller==b.draw_caller&&
            resource==b.resource&&target_width==b.target_width&&target_height==b.target_height&&
            targets==b.targets&&mip==b.mip&&format==b.format&&eye_valid==b.eye_valid&&
            eye==b.eye&&eye_width==b.eye_width&&eye_height==b.eye_height&&eye_rect==b.eye_rect&&same_provenance;
    }
};
struct Epoch {
    uint64_t world{},resize{},inventory{};uintptr_t chain{};
    bool operator==(Epoch const& b)const{return world==b.world&&resize==b.resize&&inventory==b.inventory&&chain==b.chain;}
};
struct Row {
    Epoch epoch{};Geometry geometry{};uint64_t count{},last_ticket{};
    long long first_qpc{},last_qpc{},inventory_qpc{};uintptr_t list{};uint64_t list_generation{};
    unsigned present_width{},present_height{},present_count{};bool present_valid{};
    std::array<uintptr_t,16> present_resources{};
};
class Recorder {
public:
    static constexpr unsigned capacity=32,per_epoch=4;
    std::array<Row,capacity> rows{};unsigned size{};uint64_t dropped{};
    void record(Row value){
        unsigned epoch_rows{};
        for(unsigned i=0;i<size;++i){auto& row=rows[i];if(!(row.epoch==value.epoch))continue;
            if(bool(row.geometry.quad.ticket)!=bool(value.geometry.quad.ticket))continue;
            ++epoch_rows;if(row.geometry==value.geometry){++row.count;row.last_qpc=value.first_qpc;
                row.last_ticket=value.geometry.quad.ticket;return;}}
        // Missing-ticket quads have a separate one-row budget. Preserve at
        // least half the global storage for positive-ticket failures.
        unsigned missing_rows{};for(unsigned i=0;i<size;++i)if(!rows[i].geometry.quad.ticket)++missing_rows;
        if(epoch_rows>=(value.geometry.quad.ticket?per_epoch:1u)||size==capacity||
           (!value.geometry.quad.ticket&&missing_rows>=capacity/2)){++dropped;return;}
        value.count=1;value.last_qpc=value.first_qpc;value.last_ticket=value.geometry.quad.ticket;rows[size++]=value;
    }
};
}
