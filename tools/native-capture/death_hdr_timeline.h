#pragma once
#include "unpossessed_quad_observer.h"

namespace gears_death_hdr {
enum Kind:unsigned {sample,hdr_draw,targets,barrier,copy,execute,kind_count};
struct List {uintptr_t raw{},identity{};uint64_t generation{};
    bool operator==(List const& b)const{return raw==b.raw&&identity==b.identity&&generation==b.generation;}};
struct Footprint {uint64_t offset{};unsigned format{},width{},height{},depth{},row_pitch{};};
struct Event {
    unsigned kind{},flags{},read_mask{},slots{};uintptr_t caller{},quad_caller{};List list{};
    gears_unpossessed_quad::NativeView view{};
    gears_viewport::Target target{};unsigned target_count{};gears_viewport::DrawState rectangles{};
    bool descriptor_capacity_lost{};
    std::array<uintptr_t,2> resources{};uintptr_t source_object{};
    std::array<float,8> args{};std::array<unsigned,4> dimensions{};
    unsigned before{},after{},subresource{},barrier_type{};
    unsigned source_type{~0u},destination_type{~0u},source_subresource{~0u},destination_subresource{~0u};
    std::array<unsigned,3> xyz{};bool has_box{};std::array<unsigned,6> box{};
    std::array<Footprint,2> footprints{};
    // Only consecutive HDR draws aggregate, with exact list, eye and first-RT geometry.
    bool same_draw(Event const& b)const{
        return kind==hdr_draw&&b.kind==hdr_draw&&list==b.list&&view==b.view&&target==b.target&&target_count==b.target_count&&
            descriptor_capacity_lost==b.descriptor_capacity_lost&&
            rectangles.tracked==b.rectangles.tracked&&rectangles.viewport==b.rectangles.viewport&&rectangles.scissor==b.rectangles.scissor&&
            rectangles.viewport_count==b.rectangles.viewport_count&&rectangles.scissor_count==b.rectangles.scissor_count&&
            rectangles.depth_known==b.rectangles.depth_known&&rectangles.min_depth==b.rectangles.min_depth&&rectangles.max_depth==b.rectangles.max_depth;
    }
};
struct Row {Event event{};uint64_t first{},last{},count{},execute_seq{},execute_order{};uintptr_t queue{};unsigned member{};
    long long first_qpc{},last_qpc{};};
template<class Identity> struct IdentityCache {
    static constexpr unsigned capacity=32;
    struct Entry {uintptr_t raw{};Identity identity{};};
    std::array<Entry,capacity> entries{};unsigned size{};uint64_t dropped{};
    Identity const* find(uintptr_t raw)const{for(unsigned i=0;i<size;++i)if(entries[i].raw==raw)return &entries[i].identity;return nullptr;}
    template<class Query> Identity const* query(uintptr_t raw,Query query_once){
        if(!raw)return nullptr;if(auto* found=find(raw))return found;
        if(size==capacity){++dropped;return nullptr;}
        auto& e=entries[size++];e.raw=raw;query_once(raw,e.identity);return &e.identity;
    }
};
template<class Identity> struct Bank {
    static constexpr unsigned capacity=256,low_priority_limit=128;
    std::array<Row,capacity> rows{};unsigned size{},low_priority_rows{};uint64_t sequence{};
    std::array<uint64_t,kind_count> dropped{};uint64_t read_failures{};IdentityCache<Identity> identities;
    Row* record(Event const& event,long long qpc){
        const auto seq=++sequence;
        if(event.kind==hdr_draw&&size){auto& row=rows[size-1];if(row.last==seq-1&&row.event.same_draw(event)){
            row.last=seq;row.last_qpc=qpc;++row.count;return &row;}}
        const bool low=event.kind==targets||event.kind==barrier;
        if(size==capacity||(low&&low_priority_rows==low_priority_limit)){++dropped[event.kind];return nullptr;}
        auto& row=rows[size++];row.event=event;row.first=row.last=seq;row.count=1;row.first_qpc=row.last_qpc=qpc;
        low_priority_rows+=low;return &row;
    }
    void skip(unsigned kind,uint64_t count){sequence+=count;dropped[kind]+=count;}
    void executed(List list,uintptr_t queue,uint64_t order,unsigned member,long long qpc){
        if(!list.identity||!list.generation)return;
        bool found=false;const auto seq=sequence+1;
        for(unsigned i=0;i<size;++i){auto& row=rows[i];if(row.event.kind==execute||row.event.list.identity!=list.identity||
            row.event.list.generation!=list.generation)continue;
            found=true;if(!row.execute_seq){row.execute_seq=seq;row.queue=queue;row.execute_order=order;row.member=member;}}
        if(found){Event event{};event.kind=execute;event.list=list;auto* row=record(event,qpc);
            if(row){row->queue=queue;row->execute_order=order;row->member=member;}}
    }
    bool incomplete()const{if(identities.dropped||read_failures)return true;for(auto n:dropped)if(n)return true;return false;}
};
}
