#pragma once
#include "ui_index_membership.h"
#include <array>
#include <cstdint>
#include <limits>
#include <map>
#include <string>

// Diagnostic ownership only. This never authorizes a draw or changes rendering.
namespace gears_reticle_sample {
inline constexpr unsigned row_capacity=512,ticket_capacity=256,collection_ms=2000;
struct Identity {
    std::uint64_t pid{},creation_time{},base{},started_qpc{},world_generation{};
};
inline bool same(Identity const& a,Identity const& b){return a.pid==b.pid&&a.creation_time==b.creation_time&&
    a.base==b.base&&a.started_qpc==b.started_qpc&&a.world_generation==b.world_generation;}
inline bool valid(Identity const& a){return a.pid&&a.pid<=0xffffffffull&&a.creation_time&&a.base&&
    a.base<=(std::numeric_limits<std::uint64_t>::max)()-0x2000000&&a.started_qpc&&a.world_generation;}
struct Request {Identity identity{};std::uint64_t nonce{},request_qpc{},expires_qpc{};};
inline bool decimal(std::string const& s,std::uint64_t& out){
    if(s.empty()||s.size()>20)return false;std::uint64_t v{};
    for(char c:s){if(c<'0'||c>'9'||v>((std::numeric_limits<std::uint64_t>::max)()-unsigned(c-'0'))/10)return false;
        v=v*10+unsigned(c-'0');}out=v;return true;
}
inline bool parse(std::string const& wire,Request& out){
    if(wire.empty()||wire.size()>2048||wire.find('\0')!=std::string::npos)return false;
    std::map<std::string,std::string> fields;bool section=false;
    for(std::size_t start=0;start<wire.size();){auto end=wire.find('\n',start);if(end==std::string::npos)end=wire.size();
        auto line=wire.substr(start,end-start);start=end+1;if(!line.empty()&&line.back()=='\r')line.pop_back();
        if(line.empty())continue;if(line=="[ReticleSample]"){if(section)return false;section=true;continue;}
        const auto split=line.find('=');if(!section||split==std::string::npos||!split||line.size()>96||
            !fields.emplace(line.substr(0,split),line.substr(split+1)).second)return false;
    }
    if(fields.size()!=9)return false;Request r;std::uint64_t schema{};
    auto read=[&](char const* key,std::uint64_t& value){auto it=fields.find(key);return it!=fields.end()&&decimal(it->second,value);};
    if(!read("schema",schema)||schema!=1||!read("pid",r.identity.pid)||!read("creation_time",r.identity.creation_time)||
       !read("base",r.identity.base)||!read("started_qpc",r.identity.started_qpc)||!read("world_generation",r.identity.world_generation)||
       !read("nonce",r.nonce)||!r.nonce||!read("request_qpc",r.request_qpc)||!r.request_qpc||
       !read("expires_qpc",r.expires_qpc)||r.expires_qpc<=r.request_qpc||!valid(r.identity))return false;
    out=r;return true;
}
enum class Rejection:unsigned {none,disabled,parse,identity,clock,already_armed,ack_failed};
struct Gate {
    Request request{};std::uint64_t arm_qpc{},end_qpc{},frequency{};bool accepted{},acknowledged{},frozen{};
    bool arm(bool enabled,std::string const& wire,Identity const& current,std::uint64_t now,std::uint64_t hz,Rejection& reason){
        reason=Rejection::none;
        auto fail=[&](Rejection r){reason=r;return false;};
        if(!enabled)return fail(Rejection::disabled);Request candidate{};
        if(!parse(wire,candidate))return fail(Rejection::parse);
        if(!valid(current)||!same(candidate.identity,current))return fail(Rejection::identity);
        // Requests are fresh for at most one second; collection lasts two
        // seconds after ACK. Subtract only after checking ordering/overflow.
        if(!hz||hz>(std::numeric_limits<std::uint64_t>::max)()/2||now<current.started_qpc||
           candidate.request_qpc<current.started_qpc||candidate.request_qpc>now||now>=candidate.expires_qpc||
           candidate.expires_qpc-candidate.request_qpc>hz||now>(std::numeric_limits<std::uint64_t>::max)()-2*hz)
            return fail(Rejection::clock);
        if(accepted)return fail(Rejection::already_armed);
        request=candidate;arm_qpc=now;end_qpc=now+2*hz;frequency=hz;accepted=true;return true;
    }
    bool collect(std::uint64_t now,Identity const& current){
        if(!accepted||!acknowledged||frozen)return false;
        if(!same(request.identity,current)||now<arm_qpc||now>=end_qpc){frozen=true;return false;}return true;
    }
};
inline bool membership(gears_ui_indices::Snapshot const& snapshot,unsigned ordinal,
    std::uintptr_t payload,std::uint64_t generation,gears_ui_indices::DrawArgs const& actual,bool callback){
    if(!callback||snapshot.payload!=payload||snapshot.generation!=generation||
       !gears_ui_indices::detail::bounded_snapshot(snapshot)||ordinal>=snapshot.segment_count)return false;
    const auto& segment=snapshot.segments[ordinal];return segment.index_count&&segment.kind_mask==1&&
        !segment.unknown&&segment.known[0]==segment.index_count&&segment.entry_mask&&
        gears_ui_indices::same(segment.expected,actual);
}
template<class Row> struct Bank {
    Gate gate{};std::array<Row,row_capacity> rows{};unsigned count{},ticket_count{};
    std::array<std::uint64_t,ticket_capacity> tickets{};std::uint64_t rejected{},capacity_rejected{};
    // Caller synchronizes arm/observe/complete. No eviction or production cache.
    Row* append(std::uint64_t ticket,std::uint64_t now,Identity const& identity,bool member,bool geometry){
        if(!gate.collect(now,identity))return nullptr;
        if(!ticket||!member||!geometry){++rejected;return nullptr;}
        bool found=false;for(unsigned i=0;i<ticket_count;++i)found|=tickets[i]==ticket;
        if(count>=rows.size()||(!found&&ticket_count>=tickets.size())){++capacity_rejected;gate.frozen=true;return nullptr;}
        if(!found)tickets[ticket_count++]=ticket;return &rows[count++];
    }
    template<class Complete> void complete(std::uint64_t sequence,bool success,Complete apply){
        // Completion remains legal after TTL/capacity/world stop. It only
        // annotates rows already retained, never admits a later observation.
        for(unsigned i=0;i<count;++i)apply(rows[i],sequence,success);
    }
};
template<class Row,class Acknowledge> bool worker_admit(Bank<Row>& bank,bool enabled,std::string const& wire,
    Identity const& current,std::uint64_t now,std::uint64_t hz,Acknowledge acknowledge,Rejection& reason){
    if(!bank.gate.arm(enabled,wire,current,now,hz,reason))return false;
    if(!acknowledge(bank.gate)){reason=Rejection::ack_failed;return false;}
    bank.gate.acknowledged=true;return true;
}
}
