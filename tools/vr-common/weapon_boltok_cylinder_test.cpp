#ifdef NDEBUG
#undef NDEBUG
#endif
#include "weapon_boltok_cylinder.h"
#include <cassert>
#include <cstdio>
using namespace gears_weapon;
namespace {
const BoltokReloadIdentity id{1,2,3,4,5,6,7,8,9};
BoltokInventory inventory(unsigned loaded=0,unsigned reserve=6) {return {id,1,77,6,loaded,reserve};}
BoltokInventoryRequest request(BoltokInventory const& v,std::uint64_t token=1) {
    return {v.identity,token,2,3,token,v.revision,v.loaded,v.reserve,v.loaded+1,v.reserve-1};
}
BoltokOriginalReceipt receipt(BoltokInventoryRequest const& q,std::uint64_t event=1) {
    auto v=inventory(q.after_loaded,q.after_reserve);v.revision=q.before_revision+1;v.identity=q.identity;
    return {q.token,q.original_reload_serial,q.plan_generation,event,v,true,true,false,0,
        BoltokReloadEvent::rejected};
}
void all_orders() {
    // All 720 insertion orders: no hole can count twice and every accepted
    // original transfer preserves loaded+reserve, including partial closes.
    std::array<unsigned,6> order{0,1,2,3,4,5};unsigned permutations=0;
    do {
        BoltokCylinder c;auto v=inventory();assert(c.bind(v)&&c.opened());unsigned mask=0;
        for(unsigned i=0;i<6;++i){
            auto q=request(v,i+1);assert(c.prepare(order[i],q));
            assert(!c.closed()&&!c.prepare(order[i],q));
            assert(c.live_mask()==mask);auto r=receipt(q,i+1);
            assert(c.acknowledge(r));mask|=1u<<order[i];v=r.observed;
            assert(c.live_mask()==mask&&!c.pending()&&v.loaded+v.reserve==6);
            assert(!c.acknowledge(r));
            if(i<5)assert(!c.prepare(order[i],request(v,i+2)));
            assert(c.closed());assert(c.opened());
        }
        assert(c.live_mask()==63&&c.closed());++permutations;
    }while(std::next_permutation(order.begin(),order.end()));
    assert(permutations==720);
}
void rejection_and_recovery() {
    for(unsigned bad=0;bad<13;++bad){
        BoltokCylinder c;auto v=inventory();assert(c.bind(v)&&c.opened());auto q=request(v);
        switch(bad){
            case 0:q.identity.weapon++;break;case 1:q.token=0;break;
            case 2:q.item_serial=0;break;case 3:q.original_reload_serial=0;break;
            case 4:q.plan_generation=0;break;case 5:q.before_revision++;break;
            case 6:q.before_loaded++;break;case 7:q.before_reserve++;break;
            case 8:q.after_loaded=2;break;case 9:q.after_reserve=0;break;
            case 10:q.before_reserve=0;break;case 11:q.identity.binding_generation++;break;
            case 12:q.before_loaded=6;break;
        }
        assert(!c.prepare(0,q)&&!c.pending()&&c.live_mask()==0);
        assert(!c.prepare(6,request(v))&&c.prepare(0,request(v)));
    }
    for(unsigned bad=0;bad<9;++bad){
        BoltokCylinder c;auto v=inventory();assert(c.bind(v)&&c.opened());auto q=request(v);
        assert(c.prepare(3,q));auto r=receipt(q);
        switch(bad){
            case 0:r.token++;break;case 1:r.original_reload_serial++;break;
            case 2:r.plan_generation++;break;case 3:r.original_event_sequence=0;break;
            case 4:r.observed.loaded++;break;case 5:r.observed.reserve++;break;
            case 6:r.observed.identity.weapon++;break;
            case 7:r.original_effect_applied_once=false;break;case 8:r.observed.revision=0;break;
        }
        assert(!c.acknowledge(r)&&c.live_mask()==0&&c.pending());
        if(bad<4)assert(c.acknowledge(receipt(q)));else {
            assert(c.needs_reconciliation());assert(!c.bind(v));
            ++v.identity.binding_generation;assert(c.bind(v)&&c.live_mask()==0&&!c.pending());
            assert(!c.acknowledge(receipt(q)));
        }
    }
    BoltokCylinder c;auto v=inventory();assert(c.bind(v)&&c.opened());auto q=request(v);
    assert(c.prepare(0,q));auto denied=receipt(q);denied.accepted=denied.original_effect_applied_once=false;
    denied.observed=v;assert(!c.acknowledge(denied)&&!c.pending()&&!c.needs_reconciliation());
    assert(!c.prepare(0,q));q.token=2;assert(c.prepare(0,q));
    assert(c.acknowledge(receipt(q,2))&&c.live_mask()==1);
}
void shot_and_binding() {
    for(unsigned loaded=0;loaded<=6;++loaded){
        BoltokCylinder c;auto v=inventory(loaded,4);assert(c.bind(v));
        assert(c.live_mask()==(1u<<loaded)-1u&&!c.bind(v));
        for(unsigned i=0;i<loaded;++i){auto next=v;--next.loaded;++next.revision;
            assert(!c.shot(v,1)&&c.opened()&&!c.shot(next,1)&&c.closed());
            auto wrong=next;++wrong.reserve;assert(!c.shot(wrong,1));
            assert(c.shot(next,1)&&!c.shot(next,1));v=next;
        }
        assert(c.live_mask()==0);c.interrupt();assert(!c.opened());
    }
    for(unsigned capacity:{0u,5u,7u}){BoltokCylinder c;auto v=inventory();v.capacity=capacity;assert(!c.bind(v));}
}
}
int main(){all_orders();rejection_and_recovery();shot_and_binding();
    std::puts("Boltok cylinder: 720 fill orders, rejection/recovery and shot mapping passed (offline only).");}
