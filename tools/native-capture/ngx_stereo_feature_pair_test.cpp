#include "ngx_stereo_feature_pair.h"
#include <cassert>
#include <iostream>
using namespace gears_ngx_stereo;
static Identity id(){return {1,2,3,4,5};}
static Layout layout(){Layout l;for(auto& e:l.physical)e={2560,1080};l.full_output={2560,1080};l.input={853,720};l.eye_output={1280,1080};l.stride=1280;l.input_base={Point{0,0},Point{1280,0}};return l;}
static void create(FeaturePair& p){assert(p.created(0,100,true));assert(!p.usable());assert(p.created(1,200,true));assert(!p.usable());assert(p.commit()&&p.usable());}
static EvalUse use(unsigned eye,uint64_t ticket=10){return {id(),ticket,50,6,eye};}
static Submission submit(uint64_t value=8){return {3,50,6,70,80,value,true};}
static Completion completion(uint64_t value=8){return {3,70,80,value};}
static void run_eye(FeaturePair& p,unsigned eye,uint64_t ticket,uint64_t fence_value,bool reset_expected){
    uint64_t h{};bool reset{};auto u=use(eye,ticket);assert(p.begin_eval(u,h,reset)&&h==(eye?200u:100u)&&reset==reset_expected);assert(p.end_eval(u,true));assert(p.submitted(h,u,submit(fence_value)));
}
static void creation(){
    for(unsigned failure=0;failure<5;++failure){FeaturePair p(id(),layout());
        if(failure==0){assert(!p.created(0,0,false));assert(!p.created(1,200,true));}
        if(failure==1){assert(p.created(0,100,true));assert(!p.created(1,0,false));}
        if(failure==2){assert(!p.created(0,100,false));assert(!p.created(1,200,true));}
        if(failure==3){assert(p.created(0,100,true));assert(!p.created(1,200,false));}
        if(failure==4){assert(p.created(0,100,true));assert(!p.created(1,100,true));}
        assert(!p.commit()&&!p.usable()&&p.retired());uint64_t release{};
        assert(!p.claim_unused_release(999,id(),release));
        for(uint64_t h:{100u,200u})if(p.owns(h)){assert(p.claim_unused_release(h,id(),release)&&release==h);assert(!p.claim_unused_release(h,id(),release));}
        assert(p.drained());
    }
    FeaturePair partial(id(),layout());assert(partial.created(0,100,true));assert(!partial.commit());uint64_t release{};assert(partial.claim_unused_release(100,id(),release));
    FeaturePair p(id(),layout());create(p);assert(!p.claim_unused_release(100,id(),release));p.invalidate();auto wrong=id();wrong.epoch++;assert(!p.claim_unused_release(100,wrong,release));
    assert(p.claim_unused_release(100,id(),release));assert(p.claim_unused_release(200,id(),release)&&p.drained());
    FeaturePair restarted(id(),layout());create(restarted); // Fresh owner after old state drains.
}
static void evaluations(){
    for(unsigned n=0;n<8;++n){FeaturePair p(id(),layout());create(p);auto u=use(0);switch(n){
        case 0:u.eye=1;break;case 1:u.eye=2;break;case 2:u.identity.epoch++;break;case 3:u.identity.world++;break;case 4:u.ticket=0;break;case 5:u.list=0;break;case 6:u.generation=0;break;case 7:u.identity.parameters++;break;}
        uint64_t h{};bool reset{};assert(!p.begin_eval(u,h,reset)&&p.retired());
    }
    {FeaturePair p(id(),layout());create(p);uint64_t h{};bool r{};assert(p.begin_eval(use(0),h,r));assert(!p.begin_eval(use(0),h,r));assert(p.state(0).lifetime==Lifetime::recorded_unsubmitted);}
    {FeaturePair p(id(),layout());create(p);run_eye(p,0,10,8,true);uint64_t h{};bool r{};assert(!p.begin_eval(use(0),h,r));}
    {FeaturePair p(id(),layout());create(p);run_eye(p,0,10,8,true);uint64_t h{};bool r{};assert(!p.begin_eval(use(1,11),h,r));}
    for(unsigned n=0;n<2;++n){FeaturePair p(id(),layout());create(p);run_eye(p,0,10,8,true);uint64_t h{};bool r{};auto u=use(1);if(n)u.generation++;else u.list++;assert(p.begin_eval(u,h,r)&&h==200);assert(p.end_eval(u,true));auto s=submit();s.list=u.list;s.generation=u.generation;assert(p.submitted(h,u,s));}
    {FeaturePair p(id(),layout());create(p);run_eye(p,0,10,8,true);run_eye(p,1,10,8,true);run_eye(p,0,11,9,false);run_eye(p,1,11,9,false);uint64_t h{};bool r{};assert(!p.begin_eval(use(0,11),h,r));}
    {FeaturePair p(id(),layout());create(p);uint64_t h{},release{};bool r{};assert(p.begin_eval(use(0),h,r));assert(!p.end_eval(use(0),false));assert(!p.claim_unused_release(h,id(),release));assert(!p.claim_completed_release(h,completion(),release));assert(p.submitted(h,use(0),submit()));assert(p.claim_completed_release(h,completion(),release));assert(p.claim_unused_release(200,id(),release));}
}
static void retirement(){
    FeaturePair p(id(),layout());create(p);run_eye(p,0,10,8,true);run_eye(p,1,10,9,true);p.invalidate();uint64_t release{};uint64_t rejected_handle{};bool rejected_reset{};assert(!p.begin_eval(use(0,11),rejected_handle,rejected_reset));assert(p.state(0).handle==100&&p.state(1).handle==200);
    assert(!p.claim_unused_release(100,id(),release));assert(!p.claim_completed_release(999,completion(),release));
    for(unsigned n=0;n<5;++n){auto c=completion();switch(n){case 0:c.device++;break;case 1:c.queue++;break;case 2:c.fence++;break;case 3:c.value=7;break;case 4:c.value=UINT64_MAX;break;}assert(!p.claim_completed_release(100,c,release));}
    assert(p.claim_completed_release(100,completion(),release)&&release==100);assert(!p.claim_completed_release(100,completion(),release));
    assert(!p.claim_completed_release(200,completion(),release));assert(p.claim_completed_release(200,completion(9),release)&&p.drained());
    for(unsigned n=0;n<9;++n){FeaturePair q(id(),layout());create(q);uint64_t h{};bool r{};assert(q.begin_eval(use(0),h,r));assert(q.end_eval(use(0),true));q.invalidate();auto s=submit();switch(n){
        case 0:s.device++;break;case 1:s.list++;break;case 2:s.generation++;break;case 3:s.queue=0;break;case 4:s.fence=0;break;case 5:s.value=0;break;case 6:s.direct=false;break;case 7:h=999;break;case 8:s.value=UINT64_MAX;break;}
        assert(!q.submitted(h,use(0),s));assert(!q.claim_unused_release(100,id(),release));assert(!q.claim_completed_release(100,completion(100),release));
    }
    for(unsigned n=0;n<4;++n){FeaturePair q(id(),layout());create(q);run_eye(q,0,10,8,true);run_eye(q,1,10,8,true);uint64_t h{};bool r{};assert(q.begin_eval(use(0,11),h,r));assert(q.end_eval(use(0,11),true));auto s=submit(9);if(n==0)s.queue++;if(n==1)s.fence++;if(n==2)s.value=7;if(n==3)s.value=8;assert(!q.submitted(h,use(0,11),s));q.invalidate();assert(!q.claim_completed_release(h,completion(100),release));}
}
static void unadmitted_creation(){
    for(unsigned eye:{1u,2u}){FeaturePair p(id(),layout());auto result=p.created(eye,300,true);
        assert(!result&&result.ownership==CreationOwnership::caller_quarantine&&result.quarantine_handle==300);
        assert(!p.owns(300)&&p.retired());
    }
    FeaturePair p(id(),layout());auto first=p.created(0,100,true);assert(first&&first.ownership==CreationOwnership::pair_retained);
    auto duplicate=p.created(0,300,true);assert(!duplicate&&duplicate.ownership==CreationOwnership::caller_quarantine&&duplicate.quarantine_handle==300);
    auto repeated=p.created(0,100,true);assert(!repeated&&repeated.ownership==CreationOwnership::pair_already_owns&&!repeated.quarantine_handle);
    auto zero=p.created(2,0,false);assert(!zero&&zero.ownership==CreationOwnership::none);
    assert(p.owns(100)&&!p.owns(300));uint64_t release{};assert(p.claim_unused_release(100,id(),release));
    auto recycled=p.created(0,100,true);assert(!recycled&&recycled.ownership==CreationOwnership::caller_quarantine&&recycled.quarantine_handle==100);
    FeaturePair committed(id(),layout());create(committed);auto extra=committed.created(1,400,true);
    assert(!extra&&extra.ownership==CreationOwnership::caller_quarantine&&extra.quarantine_handle==400);
    assert(committed.state(0).handle==100&&committed.state(1).handle==200&&!committed.usable());
    FeaturePair failed(id(),layout());auto vendor_failed=failed.created(0,500,false);
    assert(!vendor_failed&&vendor_failed.ownership==CreationOwnership::pair_retained&&failed.owns(500));
}
static void measured_two_lists(){
    FeaturePair p(id(),layout());create(p);auto left=use(0);left.list=0x161b4d9b30;left.generation=1988765;
    auto right=use(1);right.list=0x1150860d0;right.generation=1988767;
    uint64_t h{};bool reset{};
    assert(p.begin_eval(left,h,reset)&&h==100&&p.end_eval(left,true));
    assert(p.begin_eval(right,h,reset)&&h==200&&p.end_eval(right,true));
    // Observed list/queue pattern; fence identities and values are synthetic.
    auto s0=submit(8);s0.list=left.list;s0.generation=left.generation;s0.queue=0x595086d0;
    auto s1=submit(9);s1.list=right.list;s1.generation=right.generation;s1.queue=0x595086d0;
    // Each handle is bound to its own recorded list and exact generation.
    assert(!p.submitted(100,left,s1));assert(!p.submitted(200,right,s0));
    auto bad=s0;bad.generation=right.generation;assert(!p.submitted(100,left,bad));
    auto wrong_use=left;wrong_use.list=right.list;assert(!p.submitted(100,wrong_use,s0));
    assert(p.submitted(100,left,s0)&&p.submitted(200,right,s1));
    p.invalidate();uint64_t release{};auto c=completion(9);c.queue=0x595086d0;
    assert(p.claim_completed_release(100,c,release)&&p.claim_completed_release(200,c,release)&&p.drained());
    // No method above attests ordered GPU execution or authorizes publication.
}
int main(){creation();unadmitted_creation();evaluations();retirement();measured_two_lists();std::cout<<"ngx stereo feature pair policy tests passed\n";}
