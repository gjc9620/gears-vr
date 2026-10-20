#include "death_hdr_timeline.h"
#include <cassert>
#include <cstdio>
#include <memory>
int main(){
    using namespace gears_death_hdr;
    struct Identity {uintptr_t canonical{};bool valid{};};
    auto bank=std::make_unique<Bank<Identity>>();Event e{};e.kind=targets;e.list={10,20,3};e.target.resource=77;
    auto* first=bank->record(e,100);assert(first&&first->first==1&&first->last==1);
    e.kind=hdr_draw;e.view.eye_valid=true;e.view.eye=0;auto* draw=bank->record(e,101);assert(draw&&draw->first==2);
    e.kind=copy;assert(bank->record(e,102)->first==3);
    e.kind=hdr_draw;auto* segment=bank->record(e,103);assert(segment!=draw&&draw->last==2&&draw->count==1);
    assert(bank->record(e,104)==segment&&segment->first==4&&segment->last==5&&segment->count==2);
    e.view.eye=1;assert(bank->record(e,105)!=segment);
    ++e.list.generation;assert(bank->record(e,106)!=segment);
    bank->executed({10,20,3},90,7,1,107);
    assert(first->execute_seq==8&&draw->execute_seq==8&&draw->queue==90&&draw->execute_order==7&&draw->member==1);
    assert(bank->rows[5].execute_seq==0); // Other list generation stays unassociated.
    bank->executed({10,20,3},91,8,2,108);assert(draw->queue==90&&draw->execute_seq==8);
    auto low=std::make_unique<Bank<Identity>>();e={};e.kind=barrier;
    for(unsigned i=0;i<128;++i)assert(low->record(e,i));
    assert(!low->record(e,128)&&low->dropped[barrier]==1&&low->sequence==129);
    e.kind=copy;for(unsigned i=0;i<128;++i)assert(low->record(e,i));
    assert(low->size==256&&!low->record(e,300)&&low->dropped[copy]==1);
    low->skip(barrier,25);assert(low->sequence==283&&low->dropped[barrier]==26&&low->incomplete());
    unsigned queries{};IdentityCache<Identity> cache;
    auto query=[&](uintptr_t raw,Identity& out){++queries;if(raw!=3){out.canonical=raw==2?100:raw+99;out.valid=true;}};
    assert(cache.query(1,query)->canonical==100&&cache.query(2,query)->canonical==100&&queries==2);
    assert(!cache.query(3,query)->valid&&queries==3);assert(!cache.query(3,query)->valid&&queries==3);
    for(unsigned i=4;i<=32;++i)assert(cache.query(i,query));
    assert(cache.size==32&&queries==32&&!cache.query(33,query)&&cache.dropped==1);
    assert(cache.query(1,query)->canonical==100&&queries==32&&!cache.find(33)&&!cache.query(0,query));
    assert(cache.query(2,query)->canonical==cache.query(1,query)->canonical); // COM result, never pointer arithmetic.
    std::puts("DeathHdrTimelineTest passed");
}
