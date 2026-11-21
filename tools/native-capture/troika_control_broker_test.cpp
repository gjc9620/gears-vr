namespace gears_weapon_hand {struct BrokerBackend;}
namespace {void configure_troika_control_test_backend(gears_weapon_hand::BrokerBackend&) noexcept;}
#define GEARS_TROIKA_CONTROL_BROKER_TEST
#include "troika_geometry_broker_test.cpp"
namespace {
namespace tc=gears_troika_control;
unsigned control_mode{},native_decisions{},native_completions{},pg_commits{},aborted_completions{};
bool control_module=true,authoritative_lease{};
bool tc_module(void*,tc::Registration const&,w::ModulePin) noexcept{return control_module;}
bool tc_decide(void* ctx,tg::Request const& q,tg::Response const&,tc::Decision& d) noexcept {
    auto& f=*static_cast<Fake*>(ctx);++native_decisions;
    C(f.broker->pending_callbacks()==1&&f.broker->pin_retained()&&!f.broker->authorized());
    d={};d.query_id=q.query_id;d.publication_sequence=q.source.frame.publication_sequence;d.source_ticket=q.source.frame.source_ticket;
    d.lifecycle=3;d.press_token=7;d.lease={w::LeaseKind::support,0,3,7};d.status=w::DecisionStatus::granted;
    d.new_grant=1;d.release_required=0;d.left_down=d.left_known=d.left_tracked=1;d.decided_ns=f.stamp;
    if(control_mode==1)return false;
    if(control_mode==2)++d.source_ticket;
    if(control_mode==3)f.stamp+=100000001;
    f.barrier(8);return true;
}
bool tc_complete(void* ctx,tg::Request const&,tc::Decision const&,bool committed) noexcept {
    auto& f=*static_cast<Fake*>(ctx);++native_completions;
    C(f.broker->pending_callbacks()==1&&f.broker->pin_retained());
    if(!committed)++aborted_completions;
    authoritative_lease=committed&&control_mode!=10;
    if(control_mode==11&&committed)geometry_live=false;
    if(control_mode==12&&committed)f.broker->invalidate(w::InvalidateReason::native_stopping);
    f.barrier(10);return authoritative_lease;
}
unsigned __cdecl tc_commit(void* ctx,tg::Request const* q,tg::Response const* g,tc::Decision const* d) noexcept {
    auto& f=*static_cast<Fake*>(ctx);++pg_commits;
    C(f.broker->pending_callbacks()==1&&f.broker->pin_retained()&&!authoritative_lease);
    C(tc::valid(*d,*q,f.stamp));C(tg::valid(*g,*q));
    if(control_mode==4)++const_cast<tc::Decision*>(d)->press_token;
    if(control_mode==5)const_cast<tg::Response*>(g)->contacts[0]=0;
    if(control_mode==6)f.reentered=f.broker->unregister(q->source.frame.native.registration_cookie);
    if(control_mode==7)RaiseException(0xe042c0b1,0,0,nullptr);
    if(control_mode==8)geometry_live=false;
    if(control_mode==13)++const_cast<tg::Request*>(q)->source.entry_token;
    if(control_mode==14)f.stamp+=100000001;
    f.barrier(9);return unsigned(control_mode==9?tc::CommitReply::rejected:tc::CommitReply::accepted);
}
void configure_troika_control_test_backend(w::BrokerBackend& b) noexcept {
    b.troika_control_module=tc_module;b.troika_control_commit=w::weapon_broker_win32::troika_control_commit;
    b.troika_control_decide=tc_decide;b.troika_control_complete=tc_complete;
}
tc::Registration control_registration(TGFixture& f){tc::Registration r{};r.geometry=f.registration();r.commit=tc_commit;return r;}
void attach_control(TGFixture& f){auto r=control_registration(f);C(f.broker.attach_troika_control(&r,&f.result,true)==w::Reply::accepted);}
void control_tests(){
    using Code=w::MainMountQueryResult::Code;geometry_mode=control_mode=0;geometry_live=control_module=true;authoritative_lease=false;
    {TGFixture f;auto r=control_registration(f);C(f.broker.attach_troika_control(&r,&f.result,false)==w::Reply::invalid);
        auto bad=r;bad.geometry.hand_cookie++;C(f.broker.attach_troika_control(&bad,&f.result,true)==w::Reply::wrong_cookie);
        bad=r;bad.geometry.context=&r;C(f.broker.attach_troika_control(&bad,&f.result,true)==w::Reply::invalid);
        bad=r;bad.commit=nullptr;C(f.broker.attach_troika_control(&bad,&f.result,true)==w::Reply::invalid);
        control_module=false;C(f.broker.attach_troika_control(&r,&f.result,true)==w::Reply::invalid);control_module=true;attach_control(f);
        auto source=f.source();tc::Decision out{};tg::Response g{};
        const auto decides=native_decisions,commits=pg_commits,completes=native_completions;
        C(f.broker.transact_troika_control(source,out,g).code==Code::ready&&out.lease.kind==w::LeaseKind::support&&authoritative_lease&&!f.broker.authorized());
        C(native_decisions==decides+1&&pg_commits==commits+1&&native_completions==completes+1&&f.fake.pins==1);
        C(f.broker.transact_troika_control(source,out,g).code==Code::unavailable&&!out.query_id&&native_decisions==decides+1);
        f.finish();authoritative_lease=false;}
    for(unsigned mode=1;mode<=14;++mode){control_mode=mode;geometry_live=true;authoritative_lease=false;TGFixture f;attach_control(f);auto source=f.source();tc::Decision out{};tg::Response g{};
        const auto aborts=aborted_completions;const auto result=f.broker.transact_troika_control(source,out,g);
        C(result.code!=Code::ready&&!out.query_id&&!g.query_id&&!authoritative_lease&&!f.broker.authorized());
        C(aborted_completions>aborts);if(mode==6)C(f.fake.reentered==w::Reply::reentrant);
        C(!f.broker.pending_callbacks()&&!f.broker.pending_transactions());f.finish();}
    control_mode=0;geometry_live=true;
    // Preserve the SAME pin while only Native code runs BETWEEN callbacks,
    // during PG commit, and during Native complete; neither stop nor unregister
    // can collect in any of these otherwise invisible phases.
    for(unsigned phase:{8u,9u,10u})for(bool stop:{false,true}){
        TGFixture f;attach_control(f);auto source=f.source();f.fake.start_barrier(phase);tc::Decision out{};tg::Response g{};w::MainMountQueryResult result{};authoritative_lease=false;
        std::thread transaction([&]{test_thread=gt;result=f.broker.transact_troika_control(source,out,g);});
        C(WaitForSingleObject(f.fake.entered,3000)==WAIT_OBJECT_0);test_thread=worker;
        if(stop)f.broker.invalidate(w::InvalidateReason::native_stopping);else f.broker.unregister(f.result.cookie);
        C(f.broker.collect_on_owner_worker()==w::Reply::draining&&f.fake.unpins==0&&f.broker.pending_callbacks()==1);
        SetEvent(f.fake.resume);transaction.join();C(result.code!=Code::ready&&!out.query_id&&!authoritative_lease);
        f.fake.end_barrier();C(!f.broker.pending_callbacks());C(f.broker.collect_on_owner_worker()==w::Reply::quiescent&&f.fake.unpins==1);
    }
}
}
int main(){geometry_and_existing_tests();const auto before=checks;control_tests();
    std::printf("troika_control_broker: %u targeted checks, %u total failures\n",checks-before,failures);return failures?1:0;}
