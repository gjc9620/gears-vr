#pragma once
#include <array>
#include <cstdint>
#include <cstddef>
namespace gears_death_pso {
constexpr size_t list_limit=64,pso_limit=2;
constexpr size_t blob_limit=4*1024*1024;
inline bool selected(bool enabled,bool recording,bool diagnostics,bool frozen,bool unpossessed,
                     uint64_t ticket,uint64_t frozen_ticket,unsigned ordinal){
    return enabled&&recording&&diagnostics&&frozen&&unpossessed&&ticket&&ticket==frozen_ticket&&ordinal==49;
}
struct State {uintptr_t identity{},pso{};uint64_t generation{};long long witness_qpc{};bool known{};};
struct Bank {
    std::array<State,list_limit> lists{};uint64_t overflow{},invalidations{};bool capacity_lost{};
    size_t slot(uintptr_t identity){
        if(!identity)return list_limit;
        for(size_t i=0;i<lists.size();++i)if(lists[i].identity==identity)return i;
        for(size_t i=0;i<lists.size();++i)if(!lists[i].identity){lists[i].identity=identity;return i;}
        ++overflow;capacity_lost=true;return list_limit;
    }
    void invalidate(uintptr_t identity){
        ++invalidations;for(auto& s:lists)if(!identity||s.identity==identity){s.known=false;s.pso=0;s.witness_qpc=0;}
    }
    void witness(size_t index,uint64_t generation,uintptr_t pso,long long qpc){
        if(index>=lists.size())return;auto& s=lists[index];s.generation=generation;s.pso=pso;s.witness_qpc=qpc;
        s.known=!capacity_lost&&generation&&pso;
    }
    State const* current(uintptr_t identity,uint64_t generation)const{
        if(capacity_lost||!identity||!generation)return nullptr;
        for(auto const& s:lists)if(s.identity==identity&&s.generation==generation&&s.known)return &s;
        return nullptr;
    }
};
struct Captures {
    std::array<uintptr_t,pso_limit> identities{};size_t count{};uint64_t overflow{};
    size_t admit(uintptr_t identity){
        if(!identity)return pso_limit;
        for(size_t i=0;i<count;++i)if(identities[i]==identity)return i;
        if(count==identities.size()){++overflow;return pso_limit;}
        identities[count]=identity;return count++;
    }
};
inline bool save_size(size_t size){return size&&size<=blob_limit;}
}
