#define GEARS_QUERY_SESSION_TEST_MAIN GuardPriorMain
#include "cover_physics_query_live_test.cpp"
#undef GEARS_QUERY_SESSION_TEST_MAIN
static bool owner_denied{};
static void deny_owner(){owner_denied=true;}
static HostSession* guard_session{};
static void drain_guard_session(){guard_session->request_drain();}
int main(){
    using Gate=gears_cover_query_guard::SessionStage;
    gears_cover_query_guard::Witness witness{};
    witness.enter();C(witness.reject(8,12,13)==false);
    witness.enter();C(witness.reject(9,14,15)==false);
    C(witness.calls==2&&witness.failed_call==1&&witness.stage==8&&witness.detail==12&&witness.extra==13);
    witness={};C(!witness.calls&&!witness.failed_call&&!witness.stage);
    arena=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,saved_arena.size(),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!arena)return 99;
    reset();auto owner=std::make_unique<HostSession>();guard_session=owner.get();C(owner->start(true,true));
    auto current=[](){return !owner_denied&&guard();};q::Overlap output{};
    C(owner->overlap(actor,physics_expected,test_box,source_read,current,output)==q::Code::queried&&output.raw_hit);
    auto observed=owner->observation();C(observed.guard.calls>0&&!observed.guard.failed_call);
    output.raw_hit=false;during_query=deny_owner;
    C(owner->overlap(actor,physics_expected,test_box,source_read,current,output)!=q::Code::queried&&!output.raw_hit);
    observed=owner->observation();C(observed.guard.failed_call>0&&observed.guard.stage==unsigned(Gate::physics_read));
    C(observed.guard.detail==unsigned(service::roles::Code::guard));
    // A later operation starts a new witness even if it fails before Engine.
    C(owner->overlap(actor,physics_expected,test_box,source_read,current,output)==q::Code::source&&!output.raw_hit);
    C(!owner->observation().guard.failed_call&&!owner->observation().guard.calls);
    owner_denied=false;during_query=nullptr;wrong_receiver=true;
    C(owner->overlap(actor,physics_expected,test_box,source_read,current,output)==q::Code::changed&&!output.raw_hit);
    observed=owner->observation();C(observed.guard.stage==unsigned(Gate::query_source_match)&&observed.guard.failed_call>0);
    wrong_receiver=false;
    C(owner->overlap(actor,physics_expected,test_box,source_read,current,output)==q::Code::queried&&output.raw_hit);
    C(!owner->observation().guard.failed_call);
    output.raw_hit=false;during_query=drain_guard_session;
    C(owner->overlap(actor,physics_expected,test_box,source_read,current,output)==q::Code::changed&&!output.raw_hit);
    C(owner->observation().guard.stage==unsigned(Gate::stopped));C(owner->poll_stop());
    C(native_calls==0&&query_lock_free);C(VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE)!=0);
    printf("CoverQueryGuard checks=%u failed=%u real typed reader/Session; Host dispatch, no game\n",checks,failures);
    return failures?1:0;
}
