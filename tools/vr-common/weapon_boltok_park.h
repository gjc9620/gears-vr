#pragma once
#include "weapon_hand_participant.h"
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
namespace gears_boltok_park {
namespace hand=gears_weapon_hand;
// Mechanical data only: deliberately no Frame, input packet, hand lease,
// timer, inventory writer, native handle or pose-journal pointer.
struct Key {
    hand::ObjectIdentity world{},pawn{},weapon{},component{},asset{};
    std::uint64_t game{},native{},registration{},world_generation{},session{},space{},input{},profile{},lifecycle{};
    bool valid() const noexcept {
        return hand::identity_valid(world)&&hand::identity_valid(pawn)&&hand::identity_valid(weapon)&&
            hand::identity_valid(component)&&hand::identity_valid(asset)&&game&&native&&registration&&
            world_generation&&session&&space&&input&&profile&&lifecycle;
    }
};
inline bool same_instance(Key const& a,Key const& b) noexcept {
    return hand::same(a.world,b.world)&&hand::same(a.pawn,b.pawn)&&hand::same(a.weapon,b.weapon)&&
        hand::same(a.component,b.component)&&hand::same(a.asset,b.asset)&&a.game==b.game&&a.native==b.native&&
        a.registration==b.registration&&a.world_generation==b.world_generation&&a.session==b.session&&
        a.space==b.space&&a.input==b.input&&a.profile==b.profile;
}
struct State {double angle{};bool hidden{};unsigned verified_epoch{};};
struct Ticket {unsigned slot{};std::uint64_t generation{};explicit operator bool() const noexcept{return generation!=0;}};
class Store {
    struct Row {Key key{};State state{};std::uint64_t generation{};};
    std::array<Row,16> rows_{};std::uint64_t next_{1};
public:
    bool save(Key const& key,State const& state,double maximum_angle) noexcept {
        if(!key.valid()||!std::isfinite(maximum_angle)||maximum_angle<=0||
           !std::isfinite(state.angle)||state.angle<0||state.angle>maximum_angle||
           !next_||next_==std::numeric_limits<std::uint64_t>::max())return false;
        unsigned slot=unsigned(rows_.size());
        for(unsigned i=0;i<rows_.size();++i)if(rows_[i].generation&&same_instance(rows_[i].key,key)){
            if(key.lifecycle<rows_[i].key.lifecycle)return false;slot=i;break;
        }
        if(slot==rows_.size())for(unsigned i=0;i<rows_.size();++i)if(!rows_[i].generation){slot=i;break;}
        if(slot==rows_.size())return false; // Never evict another live weapon.
        rows_[slot]={key,state,next_++};return true;
    }
    bool lookup(Key const& key,Ticket& ticket,State& state) const noexcept {
        ticket={};state={};if(!key.valid())return false;
        for(unsigned i=0;i<rows_.size();++i){const auto& row=rows_[i];
            if(row.generation&&same_instance(row.key,key)&&key.lifecycle>=row.key.lifecycle){ticket={i,row.generation};state=row.state;return true;}}
        return false;
    }
    bool consume(Ticket ticket) noexcept {
        if(!ticket||ticket.slot>=rows_.size()||rows_[ticket.slot].generation!=ticket.generation)return false;
        rows_[ticket.slot]={};return true;
    }
    template<class Retired>void prune(Retired retired) noexcept {
        for(auto& row:rows_)if(row.generation&&retired(row.key))row={};
    }
    void clear() noexcept {for(auto& row:rows_)row={};} // Generation never rewinds.
};
}
