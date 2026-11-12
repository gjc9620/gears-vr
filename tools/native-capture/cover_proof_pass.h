#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <limits>
// One synchronous, no-dispatch proof pass only. Every distinct identity and
// exact byte window is freshly read in each pass. Duplicate expectations are
// compared to that pass's actual read, never merely to another expectation.
// Capacity/collision exhaustion falls back to the ordinary fresh read.
namespace gears_cover_proof_pass {
struct Counts {std::uint64_t passes{},identity_visits{},identity_reads{},field_visits{},field_reads{},fallbacks{};};
template<class Identity,unsigned IdSlots=1024,unsigned FieldSlots=4096> struct Cache {
    static_assert(IdSlots&&!(IdSlots&(IdSlots-1))&&FieldSlots&&!(FieldSlots&(FieldSlots-1)));
    struct Id {std::uint64_t epoch{};std::uintptr_t address{};Identity actual{};};
    struct Field {std::uint64_t epoch{};std::uintptr_t address{};unsigned size{};std::array<unsigned char,8> actual{};};
    std::array<Id,IdSlots> ids{};std::array<Field,FieldSlots> fields{};
    std::atomic<bool> active{};std::uint64_t epoch{};unsigned id_count{},field_count{};
    Counts last{},total{};
    static std::uint64_t hash(std::uintptr_t address,unsigned size=0)noexcept{
        const auto key=std::uint64_t(address);return ((key>>3)^(key>>33)^size)*0x9e3779b97f4a7c15ull;
    }
    bool begin()noexcept{
        bool empty=false;if(!active.compare_exchange_strong(empty,true))return false;
        if(epoch==(std::numeric_limits<std::uint64_t>::max)()){
            for(auto& x:ids)x.epoch=0;for(auto& x:fields)x.epoch=0;epoch=0;
        }
        ++epoch;id_count=field_count=0;last={};last.passes=1;return true;
    }
    void end()noexcept{
        if(!active.load())return;
        total.passes+=last.passes;total.identity_visits+=last.identity_visits;total.identity_reads+=last.identity_reads;
        total.field_visits+=last.field_visits;total.field_reads+=last.field_reads;total.fallbacks+=last.fallbacks;active=false;
    }
    template<class Capture,class Same> bool identity(Identity const& expected,Capture capture,Same same){
        if(!active.load())return false;++last.identity_visits;
        Id* available{};auto at=unsigned(hash(expected.address)&(IdSlots-1));
        for(unsigned n=0;n<IdSlots&&n<64;++n){auto& slot=ids[at];
            if(slot.epoch!=epoch){available=&slot;break;}
            if(slot.address==expected.address)return same(slot.actual,expected);
            at=(at+1)&(IdSlots-1);
        }
        Identity actual{};++last.identity_reads;
        if(!capture(expected.address,actual)||!same(actual,expected))return false;
        if(available&&id_count<IdSlots/2+IdSlots%2){available->actual=actual;available->address=expected.address;available->epoch=epoch;++id_count;}
        else ++last.fallbacks;
        return true;
    }
    template<class Read> bool field(std::uintptr_t address,unsigned size,unsigned char const* expected,Read read){
        if(!active.load()||!address||!size||size>8||!expected||address>UINTPTR_MAX-size)return false;++last.field_visits;
        Field* available{};auto at=unsigned(hash(address,size)&(FieldSlots-1));
        for(unsigned n=0;n<FieldSlots&&n<64;++n){auto& slot=fields[at];
            if(slot.epoch!=epoch){available=&slot;break;}
            if(slot.address==address&&slot.size==size)return !std::memcmp(slot.actual.data(),expected,size);
            at=(at+1)&(FieldSlots-1);
        }
        std::array<unsigned char,8> actual{};++last.field_reads;
        if(!read(actual.data(),address,size)||std::memcmp(actual.data(),expected,size))return false;
        if(available&&field_count<FieldSlots/2+FieldSlots%2){available->actual=actual;available->address=address;available->size=size;available->epoch=epoch;++field_count;}
        else ++last.fallbacks;
        return true;
    }
};
}
