// The new post-compose consumer must share the parent's lifetime and must not
// turn a visual update into authority to fire. Exercise the actual broker.
namespace hv=gears_committed_hand_consumer;
unsigned visual_calls{},visual_mode{};bool visual_module_ok=true;
bool visual_module(void*,hv::Registration const&,w::ModulePin pin) noexcept {
    return visual_module_ok&&pin.handle==0x8000&&pin.module==0x8000;
}
hv::Result visual_notify(void* context,hv::Notice const* notice) noexcept {
    auto& f=*static_cast<Fake*>(context);++visual_calls;
    C(f.broker->pending_callbacks()&&f.broker->pin_retained());
    C(!w::classify_action_context(w::context::thread_ledger,true,true,true));
    C(!f.broker->notify_hand_visual(*notice)); // No recursive notification.
    if(visual_mode==1)++const_cast<hv::Notice*>(notice)->frame.serial;
    if(visual_mode==2)RaiseException(0xe042c081,0,0,nullptr);
    if(visual_mode==3)return hv::Result::fault;
    if(visual_mode==4){
        w::CallScope call{};C(f.broker->begin_call(1,2,3,4,call));
        C(f.broker->pending_aux_scopes()==1);f.broker->finish_call(call,true,0);
        C(!f.broker->pending_aux_scopes()&&!f.broker->aux_debt());
    }
    if(visual_mode==5)return hv::Result::unchanged;
    f.barrier(5);return hv::Result::applied;
}
void configure_visual_test_backend(w::BrokerBackend& b) noexcept {
    b.hand_visual_module=visual_module;b.hand_visual_notify=w::weapon_broker_win32::hand_visual_notify;
}
hv::Registration visual_registration(Fixture& f){
    hv::Registration r{};r.hand_cookie=f.result.cookie;r.native_started_qpc=500;
    r.context=&f.fake;r.notify=visual_notify;return r;
}
hv::Notice visual_notice(Fixture& f){
    auto x=mount_committed(f);hv::Notice n{};n.kind=hv::Kind::committed;
    n.frame.parent=f.fake.observed_frame.key;n.frame.serial=1;return n;
}
void hand_visual_tests(){
    visual_mode=0;visual_module_ok=true;
    {
        Fixture f;f.attach();auto r=visual_registration(f);auto bad=r;
        C(f.broker.attach_hand_visual(&r,&f.result,false)==w::Reply::invalid);
        ++bad.hand_cookie;C(f.broker.attach_hand_visual(&bad,&f.result,true)==w::Reply::wrong_cookie);
        visual_module_ok=false;C(f.broker.attach_hand_visual(&r,&f.result,true)==w::Reply::invalid);visual_module_ok=true;
        C(f.broker.attach_hand_visual(&r,&f.result,true)==w::Reply::accepted&&f.fake.pins==1);
        auto n=visual_notice(f);const auto original=n;
        bool applied=false;
        C(f.broker.notify_hand_visual(n,&applied)&&applied);C(!std::memcmp(&n,&original,sizeof n));
        visual_mode=5;applied=true;
        C(f.broker.notify_hand_visual(n,&applied)&&!applied);
        visual_mode=0;
        C(!f.broker.pending_callbacks()&&!f.broker.authorized());
        gears_weapon_call::Registration cr{};cr.hand_cookie=f.result.cookie;cr.native_started_qpc=500;
        cr.context=&f.fake;cr.begin=mount_call_begin;cr.returned=mount_call_end;cr.unwind=mount_call_unwind;
        C(f.broker.attach_call_participant(&cr,&f.result,true)==w::Reply::accepted);
        visual_mode=4;C(f.broker.notify_hand_visual(n));visual_mode=0;
        const auto calls=visual_calls;test_thread=worker;C(!f.broker.notify_hand_visual(n));test_thread=gt;
        w::context::Ticket ticket{};C(w::context::enter(w::context::thread_ledger,ticket,w::context::Kind::scene));
        C(!f.broker.notify_hand_visual(n)&&visual_calls==calls);C(w::context::leave(w::context::thread_ledger,ticket));
        ++n.frame.parent.registration_cookie;applied=true;
        C(!f.broker.notify_hand_visual(n,&applied)&&!applied&&visual_calls==calls);
        f.finish();
    }
    for(unsigned mode=1;mode<=3;++mode){
        Fixture f;f.attach();auto r=visual_registration(f);C(f.broker.attach_hand_visual(&r,&f.result,true)==w::Reply::accepted);
        auto n=visual_notice(f);visual_mode=mode;bool applied=true;
        C(!f.broker.notify_hand_visual(n,&applied)&&!applied);
        C(!f.broker.pending_callbacks()&&f.fake.observed_invalidation.reason==w::InvalidateReason::callback_fault);
        visual_mode=0;f.finish();
    }
    // A worker stop cannot unload a consumer that is still on the game stack.
    {
        Fixture f;f.attach();auto r=visual_registration(f);C(f.broker.attach_hand_visual(&r,&f.result,true)==w::Reply::accepted);
        auto n=visual_notice(f);f.fake.start_barrier(5);
        std::thread callback([&]{test_thread=gt;(void)f.broker.notify_hand_visual(n);});
        C(WaitForSingleObject(f.fake.entered,3000)==WAIT_OBJECT_0);test_thread=worker;
        (void)f.broker.unregister(f.result.cookie);
        C(f.broker.collect_on_owner_worker()==w::Reply::draining&&f.fake.unpins==0&&f.broker.pending_callbacks());
        SetEvent(f.fake.resume);callback.join();f.fake.end_barrier();
        C(f.broker.collect_on_owner_worker()==w::Reply::quiescent&&f.fake.unpins==1);
        C(!f.broker.notify_hand_visual(n));C(f.fake.violations==0);
    }
    std::printf("committed hand visual consumer: %u calls\n",visual_calls);
}
