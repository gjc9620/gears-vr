#pragma once
#include "late_quad_diagnostics.h"

// CPU evidence only. The caller holds frame_hook_mutex for every operation.
// Remember exactly the first 49 unique consumed tickets, then close selection.
// IDs need not be monotonic and repeated consumption never advances selection.
namespace gears_unpossessed_quad {
struct Ticket {
    uint64_t world{},id{};
    bool operator==(Ticket const& b)const{return world==b.world&&id==b.id;}
};
inline bool native_eye_index(uintptr_t view,uintptr_t views,int count,unsigned& eye){
    eye=0;
    // Existing renderer array layout, accepted only for the exact stereo pair.
    // Subtraction after the lower-bound check avoids overflow in an end pointer.
    if(!views||views%8||view%8||count!=2||view<views)return false;
    const auto offset=view-views;
    if(offset%0xd30||offset/0xd30>=2)return false;
    eye=static_cast<unsigned>(offset/0xd30);return true;
}
struct NativeView {
    uintptr_t renderer{},current_view{},renderer_views{};int renderer_count{};
    unsigned read_mask{},eye{};bool eye_valid{};
    bool operator==(NativeView const& b)const{
        return renderer==b.renderer&&current_view==b.current_view&&renderer_views==b.renderer_views&&
            renderer_count==b.renderer_count&&read_mask==b.read_mask&&eye==b.eye&&eye_valid==b.eye_valid;
    }
};
struct Key {
    gears_late_quad::Key call{};
    uintptr_t raw_list{},list_identity{};uint64_t list_generation{};
    NativeView native_view{};
    bool final_quad_eye_valid{},without_draw{};unsigned final_quad_eye{};
    bool operator==(Key const& b)const{
        return call==b.call&&raw_list==b.raw_list&&list_identity==b.list_identity&&list_generation==b.list_generation&&
            native_view==b.native_view&&final_quad_eye_valid==b.final_quad_eye_valid&&
            final_quad_eye==b.final_quad_eye&&without_draw==b.without_draw;
    }
};
template<class Payload> struct Row {
    Key key{};Payload payload{};uint64_t count{};long long first_qpc{},last_qpc{};
};
template<class Payload> struct Bank {
    static constexpr unsigned capacity=64;
    Ticket ticket{};unsigned ordinal{},size{};uint64_t dropped{},quad_calls{},without_draw{};
    std::array<Row<Payload>,capacity> rows{};
    // Only new shapes return a row, bounding expensive resource hints to 64.
    Row<Payload>* admit(Key const& key,long long qpc){
        for(unsigned i=0;i<size;++i){auto& row=rows[i];if(row.key==key){++row.count;row.last_qpc=qpc;return nullptr;}}
        if(size==capacity){++dropped;return nullptr;}
        auto& row=rows[size++];row.key=key;row.count=1;row.first_qpc=row.last_qpc=qpc;return &row;
    }
};
template<class Payload> struct Recorder {
    static constexpr unsigned ticket_limit=49,bank_limit=4;
    std::array<Ticket,ticket_limit> seen{};unsigned distinct{},size{};
    std::array<Bank<Payload>,bank_limit> banks{};
    Bank<Payload>* find(Ticket ticket){
        for(unsigned i=0;i<size;++i)if(banks[i].ticket==ticket)return &banks[i];return nullptr;
    }
    Bank<Payload>* select(bool unpossessed,Ticket ticket){
        if(!unpossessed||!ticket.id)return nullptr;
        if(auto* bank=find(ticket))return bank;
        for(unsigned i=0;i<distinct;++i)if(seen[i]==ticket)return nullptr;
        if(distinct==ticket_limit)return nullptr;
        seen[distinct++]=ticket;
        if(distinct>3&&distinct!=ticket_limit)return nullptr;
        auto& bank=banks[size++];bank.ticket=ticket;bank.ordinal=distinct;return &bank;
    }
};
}
