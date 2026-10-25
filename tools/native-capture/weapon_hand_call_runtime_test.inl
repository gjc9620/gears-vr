// Included only by weapon_hand_runtime_test.cpp. Every scenario has a fresh
// process; no production singleton reset or self-unload is used.
namespace call=::gears_weapon_call;
bool Fake::call_module(void*,call::Registration const& r,w::ModulePin held) noexcept {
    return held.module==0x9000&&r.begin==call_pre&&r.returned==call_returned&&r.unwind==call_unwind;
}
void Fake::call_pre(void*,call::Invocation const* invocation,call::State* state) noexcept {
    ++fake.aux_pres;fake.aux_invocation=*invocation;
    auto* b=runtime::current();if(!b||!b->pending_aux_scopes()||!b->pending_callbacks())++fake.violations;
    std::memcpy(state->storage.data(),&invocation->call_id,sizeof(invocation->call_id));
    if(fake.aux_mode==1)RaiseException(0xe042c050,0,0,nullptr);
    if(fake.aux_mode==3)++const_cast<call::Invocation*>(invocation)->callee;
}
call::End Fake::call_returned(void*,call::Invocation const* invocation,call::State* state,std::uint64_t) noexcept {
    ++fake.aux_returns;std::uint64_t token{};std::memcpy(&token,state->storage.data(),sizeof(token));
    if(token!=invocation->call_id||!call::same(fake.aux_invocation,*invocation))++fake.violations;
    if(fake.aux_mode==2)RaiseException(0xe042c051,0,0,nullptr);
    return fake.aux_mode==4?call::End::unresolved:call::End::closed;
}
call::End Fake::call_unwind(void*,call::Invocation const* invocation,call::State* state,call::UnwindReason reason) noexcept {
    ++fake.aux_unwinds;fake.aux_reason=reason;std::uint64_t token{};std::memcpy(&token,state->storage.data(),sizeof(token));
    if(token!=invocation->call_id||!call::same(fake.aux_invocation,*invocation))++fake.violations;return call::End::closed;
}
call::Registration auxiliary_registration(w::RegistrationResult const& hand){
    call::Registration r{};r.hand_cookie=hand.cookie;r.native_started_qpc=hand.native_started_qpc;r.context=&fake;
    r.begin=Fake::call_pre;r.returned=Fake::call_returned;r.unwind=Fake::call_unwind;return r;
}
void attach_auxiliary(w::RegistrationResult const& hand){
    const auto pins=fake.pins.load();
    auto r=auxiliary_registration(hand);w::RegistrationResult result{};
    C(runtime::attach_call_participant(&r,&result,false)==w::Reply::invalid&&!runtime::current()->call_attached());
    C(runtime::attach_call_participant(&r,&result,true)==w::Reply::accepted&&result.cookie==hand.cookie&&result.native_started_qpc==hand.native_started_qpc);
    C(runtime::attach_call_participant(&r,&result,true)==w::Reply::accepted&&fake.pins==pins);
}
unsigned auxiliary_original_exception() noexcept {
    w::CallScope scope{};runtime::begin_call(1,2,3,4,scope);
    __try {__try {RaiseException(0xe042c052,0,0,nullptr);}__finally {runtime::finish_call(scope,false,0);}}
    __except(GetExceptionCode()==0xe042c052?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){return GetExceptionCode();}
    return 0;
}
void auxiliary_runtime(const char* mode){
    initialize();auto hand=attach();attach_auxiliary(hand);auto* b=runtime::current();test_thread=gt;
    if(!std::strcmp(mode,"aux-exception")){
        C(auxiliary_original_exception()==0xe042c052&&fake.aux_pres==1&&fake.aux_unwinds==1&&fake.aux_returns==0);
        C(fake.aux_reason==call::UnwindReason::original_exception&&!b->aux_debt()&&!b->pending_aux_scopes());finish();return;
    }
    if(!std::strcmp(mode,"aux-prepare-gap")){
        auto f=frame(hand.cookie);w::Proposal p{};C(b->prepare(f,p)==w::Reply::accepted&&b->pending_transactions()==1);
        w::CallScope scope{};C(runtime::begin_call(1,2,3,4,scope)&&b->pending_aux_scopes()==1);
        test_thread=worker;runtime::request_stop();runtime::worker_tick();C(!runtime::cleanup_ready()&&runtime::retained()&&fake.unpins==0);
        test_thread=gt;runtime::finish_call(scope,true,77);C(!b->pending_aux_scopes()&&b->pending_transactions()==1);
        auto d=decision(f,p);C(b->commit(f,p,d)==w::Reply::draining);C(b->abort_frame(f.key,p.nonce)==w::Reply::quiescent);
        finish();C(runtime::unregister(hand.cookie)==w::Reply::quiescent);return;
    }
    if(!std::strcmp(mode,"aux-fault-pre"))fake.aux_mode=1;
    else if(!std::strcmp(mode,"aux-fault-returned"))fake.aux_mode=2;
    else if(!std::strcmp(mode,"aux-mutated"))fake.aux_mode=3;
    else if(!std::strcmp(mode,"aux-unresolved"))fake.aux_mode=4;
    w::CallScope scope{};C(runtime::begin_call(1,2,3,4,scope));const auto token=scope.token;
    C(b->pending_aux_scopes()==1&&!b->pending_callbacks()&&!b->pending_transactions()&&runtime::retained());
    if(fake.aux_mode){
        runtime::finish_call(scope,true,0x1234);C(b->aux_debt()&&!b->pending_aux_scopes()&&fake.aux_pres==1);
        if(fake.aux_mode==1||fake.aux_mode==3)C(fake.aux_unwinds==1&&fake.aux_returns==0&&fake.aux_reason==call::UnwindReason::notification_fault);
        if(fake.aux_mode==2)C(fake.aux_unwinds==1&&fake.aux_returns==1&&fake.aux_reason==call::UnwindReason::notification_fault);
        test_thread=worker;fake.aux_mode=0;runtime::request_stop();runtime::worker_tick();
        C(runtime::retained()&&!runtime::cleanup_ready()&&b->pin_retained()&&fake.unpins==0);
        C(runtime::unregister(hand.cookie)==w::Reply::draining);runtime::worker_tick();C(b->aux_debt()&&!runtime::cleanup_ready()&&fake.violations==0);return;
    }
    if(!std::strcmp(mode,"aux-unregister")){
        test_thread=worker;C(runtime::unregister(hand.cookie)==w::Reply::draining);runtime::worker_tick();C(fake.unpins==0);
        test_thread=gt;runtime::finish_call(scope,true,55);test_thread=worker;runtime::worker_tick();
        C(fake.unpins==1&&runtime::unregister(hand.cookie)==w::Reply::quiescent&&!b->call_attached());
        auto next=attach();const auto revision=b->lifecycle_revision();C(next.cookie!=hand.cookie&&runtime::unregister(hand.cookie)==w::Reply::quiescent&&b->cookie()==next.cookie&&b->lifecycle_revision()==revision);
        test_thread=gt;w::CallScope not_attached{};C(!runtime::begin_call(1,2,3,4,not_attached)&&fake.aux_pres==1);
        attach_auxiliary(next);w::CallScope newer{};C(runtime::begin_call(1,2,3,4,newer)&&newer.token>token);runtime::finish_call(newer,true,0);finish();return;
    }
    C(!std::strcmp(mode,"aux-normal"));test_thread=worker;runtime::request_stop();runtime::worker_tick();
    C(runtime::retained()&&!runtime::cleanup_ready()&&fake.unpins==0&&b->pending_aux_scopes()==1);
    w::CallScope no_new{};C(!runtime::begin_call(1,2,3,4,no_new));auto registration=auxiliary_registration(hand);w::RegistrationResult result{};
    C(runtime::attach_call_participant(&registration,&result,true)==w::Reply::draining);
    test_thread=gt;runtime::finish_call(scope,true,99);C(fake.aux_returns==1&&!b->aux_debt()&&!b->pending_aux_scopes());
    runtime::finish_call(scope,true,99);C(fake.aux_returns==1);test_thread=worker;runtime::worker_tick();
    C(runtime::cleanup_ready()&&!runtime::retained()&&fake.unpins==1&&runtime::unregister(hand.cookie)==w::Reply::quiescent&&fake.violations==0);
}
