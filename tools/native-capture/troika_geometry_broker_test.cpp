#define main existing_broker_tests
#include "weapon_hand_broker_test.cpp"
#undef main
namespace {
namespace tg=gears_troika_geometry;
unsigned geometry_calls{},geometry_checks{},geometry_mode{};bool geometry_module=true,geometry_live=true;
bool tg_module(void*,tg::Registration const&,w::ModulePin) noexcept {return geometry_module;}
bool tg_current(void*,tg::Source const&) noexcept {++geometry_checks;return geometry_live;}
unsigned __cdecl tg_prepare(void* ctx,tg::Request const* q,tg::Response* r) noexcept {
    ++geometry_calls;auto& f=*static_cast<Fake*>(ctx);
    C(f.broker->pending_callbacks()==1&&f.broker->pin_retained());
    *r={};r->query_id=q->query_id;r->publication_sequence=q->source.frame.publication_sequence;r->source_ticket=q->source.frame.source_ticket;
    r->entry_binding=q->source.entry_binding;r->entry_token=q->source.entry_token;r->component=q->source.frame.turret_mesh;
    r->asset={0x7700,0,0x9000,17,18,0,1};r->tracked_mask=3;r->contacts[0]=1;r->contacts[1]=2;r->distances[0][0]=r->distances[1][1]=.01;
    if(geometry_mode==1)++r->source_ticket;
    if(geometry_mode==2)++const_cast<tg::Request*>(q)->source.entry_token;
    if(geometry_mode==3)f.stamp+=100000001;
    if(geometry_mode==4)geometry_live=false;
    if(geometry_mode==5)f.reentered=f.broker->unregister(q->source.frame.native.registration_cookie);
    if(geometry_mode==6)RaiseException(0xe042c0a1,0,0,nullptr);
    if(geometry_mode==7)r->distances[0][0]=.02;
    if(geometry_mode==8)r->tracked_mask=2;
    if(geometry_mode==9){*r={};return unsigned(tg::Status::geometry_unavailable);}
    if(geometry_mode==10)return unsigned(tg::Status::geometry_unavailable); // Nonempty denial is malformed.
    if(geometry_mode==11){r->contacts[0]=0;r->distances[0][0]=10;r->tracked_mask=2;} // Cannot invent tracking loss either.
    if(geometry_mode==12)r->distances[0][0]=std::numeric_limits<double>::quiet_NaN();
    if(geometry_mode==13)r->hand_handle[1]=r->hand_handle[0];
    if(geometry_mode==14)r->hand_handle[0]=2;
    f.barrier(7);return unsigned(tg::Status::ready);
}
struct TGFixture {
    Fake fake;
    static w::BrokerBackend backend(Fake& f){auto b=f.backend();b.troika_geometry_module=tg_module;
        b.troika_geometry_prepare=w::weapon_broker_win32::troika_geometry_prepare;b.troika_geometry_current=tg_current;
#if defined(GEARS_TROIKA_CONTROL_BROKER_TEST)
        configure_troika_control_test_backend(b);
#endif
        return b;}
    w::Broker broker{{42,worker,400,500},backend(fake)};w::RegistrationResult result{};
    TGFixture(){test_thread=worker;fake.broker=&broker;w::Registration r{};r.context=&fake;r.prepare=Fake::prepare;r.commit=Fake::commit;r.invalidate=Fake::invalidate;
        C(broker.register_participant(&r,&result)==w::Reply::accepted);}
    tg::Registration registration(){tg::Registration r{};r.context=&fake;r.hand_cookie=result.cookie;r.native_started_qpc=500;r.prepare=tg_prepare;return r;}
    void attach(){auto r=registration();C(broker.attach_troika_geometry(&r,&result,true)==w::Reply::accepted);}
    tg::Source source(){Fixture helper;helper.result.cookie=result.cookie;auto f=helper.frame();f.weapon={0x6000,0x3000,0x9000,6,7,0,1};
        w::Proposal p{};C(broker.prepare(f,p)==w::Reply::accepted);C(broker.commit(f,p,helper.decision(f,p,false))==w::Reply::accepted);
        broker.invalidate(w::InvalidateReason::context_lost);C(!broker.authorized());
        tg::Source value{};auto& s=value.frame;s.native={42,gt,400,500,result.cookie};s.world=f.world;s.controller=f.controller;
        s.driver=f.pawn;s.driver_mesh=f.mesh;s.turret={0x7000,0x1000,0x9000,7,8,0,1};s.turret_mesh={0x7100,0x7000,0x9000,8,9,0,1};s.weapon=f.weapon;
        s.publication_sequence=s.source_ticket=s.scene_invocation=s.world_generation=s.reference_generation=1;
        s.scene_qpc=s.published_ns=f.now_ns;s.qpc_frequency=1000000000;s.native_reference_revision=2;s.native_reference={1,1,0};
        s.producer={1,{1,1}};s.publisher_pid=42;s.publisher_start=400;s.reference=f.reference;s.stereo=f.stereo;s.controllers=f.controllers;s.control_policy=gears_troika_source::mounted_look_suppressed;
        for(auto& h:s.controllers.hands){h.grip_active=1;h.grip_flags=15;}
        value.entry_binding=value.entry_token=7;C(tg::source_valid(value,fake.stamp.load(),false));return value;}
    void finish(){test_thread=worker;broker.invalidate(w::InvalidateReason::native_stopping);C(broker.collect_on_owner_worker()==w::Reply::quiescent);
        C(fake.pins==1&&fake.unpins==1&&!broker.pin_retained()&&!broker.pending_callbacks()&&!fake.violations);}
};
void troika_geometry_tests(){
    using Code=w::MainMountQueryResult::Code;geometry_mode=0;geometry_live=geometry_module=true;
    {TGFixture f;auto r=f.registration();C(f.broker.attach_troika_geometry(&r,&f.result,false)==w::Reply::invalid);
        auto bad=r;++bad.hand_cookie;C(f.broker.attach_troika_geometry(&bad,&f.result,true)==w::Reply::wrong_cookie);
        bad=r;bad.context=&r;C(f.broker.attach_troika_geometry(&bad,&f.result,true)==w::Reply::invalid);
        geometry_module=false;C(f.broker.attach_troika_geometry(&r,&f.result,true)==w::Reply::invalid);geometry_module=true;f.attach();
        auto s=f.source();tg::Response out{};const auto before=geometry_calls;
        C(f.broker.query_troika_geometry(s,out).code==Code::ready&&out.contacts[0]==1&&out.contacts[1]==2&&!f.broker.authorized());
        C(geometry_calls==before+1);C(f.broker.query_troika_geometry(s,out).code==Code::unavailable&&!out.query_id&&geometry_calls==before+1);
        ++s.frame.publication_sequence;
        w::context::Ticket ticket{};C(w::context::enter(w::context::thread_ledger,ticket,w::context::Kind::stereo));
        C(f.broker.query_troika_geometry(s,out).code==Code::context_rejected);C(w::context::leave(w::context::thread_ledger,ticket));
        C(f.broker.query_troika_geometry(s,out).code==Code::ready);f.finish();}
    for(unsigned mode=1;mode<=14;++mode){geometry_mode=mode;geometry_live=true;TGFixture f;f.attach();auto s=f.source();tg::Response out{};
        const auto result=f.broker.query_troika_geometry(s,out);const auto expected=mode==3?Code::stale:mode==4?Code::context_rejected:mode==9?Code::not_ready:Code::fault;
        C(result.code==expected&&!out.query_id&&!f.broker.authorized());if(mode==5)C(f.fake.reentered==w::Reply::reentrant);
        C(!f.broker.pending_callbacks());geometry_mode=0;f.finish();}
    geometry_live=false;
    {TGFixture f;f.attach();auto s=f.source();tg::Response out{};const auto calls=geometry_calls;
        C(f.broker.query_troika_geometry(s,out).code==Code::context_rejected&&geometry_calls==calls);
        geometry_live=true;C(f.broker.query_troika_geometry(s,out).code==Code::unavailable&&geometry_calls==calls);f.finish();}
    for(bool stop:{false,true}){TGFixture f;f.attach();auto s=f.source();f.fake.start_barrier(7);tg::Response out{};w::MainMountQueryResult result{};
        std::thread query([&]{test_thread=gt;result=f.broker.query_troika_geometry(s,out);});C(WaitForSingleObject(f.fake.entered,3000)==WAIT_OBJECT_0);
        test_thread=worker;if(stop)f.broker.invalidate(w::InvalidateReason::native_stopping);else f.broker.unregister(f.result.cookie);
        C(f.broker.collect_on_owner_worker()==w::Reply::draining&&f.fake.unpins==0&&f.broker.pending_callbacks());
        SetEvent(f.fake.resume);query.join();C(result.code==Code::unavailable&&!out.query_id&&!f.broker.pending_callbacks());
        f.fake.end_barrier();C(f.broker.collect_on_owner_worker()==w::Reply::quiescent&&f.fake.unpins==1);}
}
}
#if defined(GEARS_TROIKA_CONTROL_BROKER_TEST)
int geometry_and_existing_tests(){
#else
int main(){
#endif
    existing_broker_tests();const auto before=checks;troika_geometry_tests();
    std::printf("troika_geometry_broker: %u targeted checks, %u total failures\n",checks-before,failures);return failures?1:0;}
