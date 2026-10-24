// Included only by weapon_hand_broker_test.cpp, inside its test namespace.
namespace c=::gears_weapon_call;
enum class AuxFault {none,pre,returned,unwind,pre_input,returned_input,unwind_input,unknown_end,unresolved,reenter,returned_reenter,unwind_reenter,pre_unwind};
struct AuxFake {
    w::Broker* broker{};AuxFault fault{};bool module_ok=true,unpin_ok=true;
    unsigned pins{},unpins{},pres{},returns{},unwinds{},invalidations{},violations{},originals{};
    std::uint64_t rax{};c::Invocation observed_pre{},observed_end{};c::UnwindReason reason{};
    HANDLE entered{},resume{};unsigned block{};w::Reply reentered{};
    static bool pin(void* p,w::Registration const&,w::ModulePin& out) noexcept {auto& f=*static_cast<AuxFake*>(p);++f.pins;out={0x8800,0x8800};return true;}
    static bool unpin(void* p,w::ModulePin) noexcept {auto& f=*static_cast<AuxFake*>(p);++f.unpins;
        if(test_thread!=worker||f.broker->pending_callbacks()||f.broker->pending_transactions()||f.broker->pending_aux_scopes()||f.broker->aux_debt())++f.violations;return f.unpin_ok;}
    static std::int64_t clock(void*) noexcept {return 1000000000;}
    static std::uint32_t thread(void*) noexcept {return test_thread;}
    static std::uint32_t prepare(void*,w::Frame const* f,w::Proposal* p) noexcept {*p={};p->key=f->key;p->nonce=1;return unsigned(w::Reply::accepted);}
    static std::uint32_t commit(void*,w::Frame const*,w::Proposal const*,w::Decision const*) noexcept {return unsigned(w::Reply::accepted);}
    static void invalidate(void* p,w::Invalidation const*) noexcept {auto& f=*static_cast<AuxFake*>(p);++f.invalidations;if(f.broker->authorized())++f.violations;}
    void barrier(unsigned phase) noexcept {if(block==phase){SetEvent(entered);if(WaitForSingleObject(resume,3000)!=WAIT_OBJECT_0)++violations;}}
    static bool module(void* p,c::Registration const& r,w::ModulePin pin_value) noexcept {
        auto& f=*static_cast<AuxFake*>(p);return f.module_ok&&pin_value.module==0x8800&&r.begin==pre&&
            (r.returned==returned||r.returned==other_returned)&&r.unwind==unwind;
    }
    static void pre(void* p,c::Invocation const* invocation,c::State* state) noexcept {
        auto& f=*static_cast<AuxFake*>(p);++f.pres;f.observed_pre=*invocation;
        if(!f.broker->pending_aux_scopes()||!f.broker->pending_callbacks()||reinterpret_cast<std::uintptr_t>(state)%8)++f.violations;
        const c::State empty{};if(std::memcmp(&empty,state,sizeof(empty)))++f.violations;
        std::memcpy(state->storage.data(),&invocation->call_id,sizeof(invocation->call_id));
        if(f.fault==AuxFault::pre_input)++const_cast<c::Invocation*>(invocation)->callee;
        if(f.fault==AuxFault::reenter){w::CallScope nested{};if(f.broker->begin_call(1,2,3,4,nested))++f.violations;f.reentered=f.broker->unregister(invocation->hand_cookie);}
        f.barrier(1);if(f.fault==AuxFault::pre||f.fault==AuxFault::pre_unwind)RaiseException(0xe042c040,0,0,nullptr);
    }
    static void verify(AuxFake& f,c::Invocation const* invocation,c::State* state) noexcept {
        std::uint64_t token{};std::memcpy(&token,state->storage.data(),sizeof(token));
        if(token!=invocation->call_id||!f.broker->pending_aux_scopes()||!f.broker->pending_callbacks())++f.violations;
        f.observed_end=*invocation;
    }
    static c::End returned(void* p,c::Invocation const* invocation,c::State* state,std::uint64_t value) noexcept {
        auto& f=*static_cast<AuxFake*>(p);++f.returns;f.rax=value;verify(f,invocation,state);f.barrier(2);
        if(f.fault==AuxFault::returned_input)++const_cast<c::Invocation*>(invocation)->self;
        if(f.fault==AuxFault::returned_reenter)f.reentered=f.broker->unregister(invocation->hand_cookie);
        if(f.fault==AuxFault::returned)RaiseException(0xe042c041,0,0,nullptr);
        if(f.fault==AuxFault::unknown_end)return static_cast<c::End>(99);
        return f.fault==AuxFault::unresolved?c::End::unresolved:c::End::closed;
    }
    static c::End other_returned(void* p,c::Invocation const* i,c::State* s,std::uint64_t value) noexcept {return returned(p,i,s,value);}
    static c::End unwind(void* p,c::Invocation const* invocation,c::State* state,c::UnwindReason why) noexcept {
        auto& f=*static_cast<AuxFake*>(p);++f.unwinds;f.reason=why;verify(f,invocation,state);f.barrier(3);
        if(f.fault==AuxFault::unwind_input)++const_cast<c::Invocation*>(invocation)->result;
        if(f.fault==AuxFault::unwind_reenter)f.reentered=f.broker->unregister(invocation->hand_cookie);
        if(f.fault==AuxFault::unwind||f.fault==AuxFault::pre_unwind)RaiseException(0xe042c042,0,0,nullptr);
        return c::End::closed;
    }
    w::BrokerBackend backend() noexcept {return {this,pin,unpin,clock,thread,w::weapon_broker_win32::prepare,w::weapon_broker_win32::commit,w::weapon_broker_win32::invalidate,module,w::weapon_broker_win32::call_begin,w::weapon_broker_win32::call_returned,w::weapon_broker_win32::call_unwind};}
};
struct AuxFixture {
    AuxFake fake;w::Broker broker{{42,worker,400,500},fake.backend()};w::RegistrationResult result{};c::Registration registration{};
    AuxFixture(){fake.broker=&broker;test_thread=worker;}
    void hand(){w::Registration r{};r.context=&fake;r.prepare=AuxFake::prepare;r.commit=AuxFake::commit;r.invalidate=AuxFake::invalidate;
        C(broker.register_participant(&r,&result)==w::Reply::accepted);registration.hand_cookie=result.cookie;registration.native_started_qpc=500;
        registration.context=&fake;registration.begin=AuxFake::pre;registration.returned=AuxFake::returned;registration.unwind=AuxFake::unwind;}
    void attach(){hand();C(broker.attach_call_participant(&registration,&result,true)==w::Reply::accepted);C(broker.call_attached()&&fake.pins==1);test_thread=gt;}
    void finish(bool clean=true){test_thread=worker;broker.invalidate(w::InvalidateReason::native_stopping);
        C(broker.collect_on_owner_worker()==(clean?w::Reply::quiescent:w::Reply::draining));
        C(clean?(!broker.pin_retained()&&!broker.pending()):broker.pin_retained()&&broker.aux_debt()&&broker.pending());C(fake.violations==0);}
};
// POD-only leaf mirrors the parent's __finally responsibility. No SEH filter
// surrounds the real original; its exact exception propagates to this caller.
std::uint64_t aux_invoke(AuxFake* f,bool original_fault) {
    w::CallScope scope{};std::uint64_t value{};bool returned=false;
    f->broker->begin_call(0x1234,0,0x3456,0x4567,scope);
    __try {++f->originals;if(original_fault)RaiseException(0xe042c043,0,0,nullptr);value=0xfedcba9876543210ull;returned=true;}
    __finally {f->broker->finish_call(scope,returned,value);}
    return value;
}
unsigned aux_exception(AuxFake* f) noexcept {
    __try {(void)aux_invoke(f,true);return 0;}
    __except(GetExceptionCode()==0xe042c043?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){return GetExceptionCode();}
}
void auxiliary_registration_tests(){
    AuxFixture f;f.hand();auto r=f.registration;auto result=f.result;
    C(f.broker.attach_call_participant(&r,&result,false)==w::Reply::invalid&&!f.broker.call_attached());
    --r.size;C(f.broker.attach_call_participant(&r,&result,true)==w::Reply::invalid);r=f.registration;
    ++r.hand_cookie;C(f.broker.attach_call_participant(&r,&result,true)==w::Reply::wrong_cookie);r=f.registration;
    ++r.native_started_qpc;C(f.broker.attach_call_participant(&r,&result,true)==w::Reply::wrong_cookie);r=f.registration;
    r.context=&result;C(f.broker.attach_call_participant(&r,&result,true)==w::Reply::invalid);r=f.registration;
    f.fake.module_ok=false;C(f.broker.attach_call_participant(&r,&result,true)==w::Reply::invalid);f.fake.module_ok=true;
    C(f.broker.attach_call_participant(&r,&result,true)==w::Reply::accepted);const auto revision=f.broker.lifecycle_revision();
    C(f.broker.attach_call_participant(&r,&result,true)==w::Reply::accepted&&result.cookie==f.result.cookie&&f.fake.pins==1&&f.broker.lifecycle_revision()==revision);
    r.returned=AuxFake::other_returned;C(f.broker.attach_call_participant(&r,&result,true)==w::Reply::busy);f.finish();
    std::uintptr_t module{};C(w::weapon_broker_win32::executable_image(reinterpret_cast<std::uintptr_t>(AuxFake::pre),module));
    r=f.registration;C(w::weapon_broker_win32::call_module(nullptr,r,{module,module}));
    r.begin=reinterpret_cast<c::Begin>(GetTickCount64);C(!w::weapon_broker_win32::call_module(nullptr,r,{module,module}));
    r.begin=reinterpret_cast<c::Begin>(&result);C(!w::weapon_broker_win32::call_module(nullptr,r,{module,module}));
}
void auxiliary_normal_nested_tests(){
    AuxFixture f;f.attach();w::CallScope outer{},inner{};
    C(f.broker.begin_call(1,2,0,4,outer));const auto outer_id=outer.invocation.call_id;
    C(f.broker.pending_aux_scopes()==1&&!f.broker.pending_callbacks()&&!f.broker.pending_transactions());
    C(outer.invocation.self==1&&outer.invocation.caller_frame==2&&outer.invocation.result==0&&outer.invocation.callee==4&&outer.invocation.thread_id==gt);
    C(f.broker.begin_call(5,6,7,8,inner)&&inner.invocation.call_id>outer_id&&inner.previous==&outer);
    f.broker.finish_call(inner,true,22);C(f.broker.pending_aux_scopes()==1&&f.fake.rax==22&&!f.broker.aux_debt());
    f.broker.finish_call(outer,true,33);C(!f.broker.pending_aux_scopes()&&!f.broker.pending_callbacks()&&!f.broker.aux_debt()&&f.fake.rax==33);
    f.broker.finish_call(outer,true,99);C(f.fake.returns==2);
    C(f.broker.invalidate(w::InvalidateReason::input_lost)==w::Reply::accepted);C(f.broker.availability()==w::Availability::unavailable);
    C(aux_invoke(&f.fake,false)==0xfedcba9876543210ull&&f.fake.pres==3&&f.fake.originals==1); // soft loss still pre
    C(aux_exception(&f.fake)==0xe042c043&&f.fake.unwinds==1&&f.fake.reason==c::UnwindReason::original_exception&&f.fake.originals==2);
    C(!f.broker.aux_debt()&&!f.broker.pending_aux_scopes());f.finish();
}
void auxiliary_drain_retired_tests(){
    AuxFixture f;f.attach();w::CallScope scope{};C(f.broker.begin_call(1,2,3,4,scope));const auto id=scope.token,cookie=f.result.cookie;
    test_thread=worker;C(f.broker.unregister(cookie)==w::Reply::draining);C(!f.broker.quiescent()&&f.broker.pending_aux_scopes()==1&&!f.broker.pending_callbacks());
    C(f.broker.collect_on_owner_worker()==w::Reply::draining&&f.fake.unpins==0);C(f.broker.unregister(cookie)==w::Reply::draining);
    w::CallScope rejected{};C(!f.broker.begin_call(1,2,3,4,rejected));test_thread=gt;f.broker.finish_call(scope,true,1);
    C(f.broker.quiescent()&&!f.broker.pending_aux_scopes());test_thread=worker;f.fake.unpin_ok=false;
    C(f.broker.collect_on_owner_worker()==w::Reply::unavailable&&f.broker.cookie()==cookie&&f.broker.call_attached());
    f.fake.unpin_ok=true;C(f.broker.collect_on_owner_worker()==w::Reply::quiescent&&!f.broker.call_attached());
    C(f.broker.unregister(cookie)==w::Reply::quiescent);const auto old_registration=f.registration;f.hand();const auto newer=f.result.cookie;
    C(newer!=cookie&&f.broker.unregister(cookie)==w::Reply::quiescent&&f.broker.cookie()==newer);
    test_thread=gt;w::CallScope unattached{};const auto pres=f.fake.pres;
    C(!f.broker.begin_call(1,2,3,4,unattached)&&f.fake.pres==pres&&!f.broker.call_attached());
    C(f.broker.attach_call_participant(&old_registration,&f.result,true)==w::Reply::wrong_cookie&&!f.broker.call_attached());
    C(f.broker.attach_call_participant(&f.registration,&f.result,true)==w::Reply::accepted);test_thread=gt;w::CallScope next{};
    C(f.broker.begin_call(1,2,3,4,next)&&next.token>id);f.broker.finish_call(next,true,1);f.finish();
}
void auxiliary_fault_tests(){
    for(auto fault:{AuxFault::pre,AuxFault::returned,AuxFault::unwind,AuxFault::pre_input,AuxFault::returned_input,AuxFault::unwind_input,AuxFault::unknown_end,AuxFault::unresolved,AuxFault::reenter,AuxFault::returned_reenter,AuxFault::unwind_reenter,AuxFault::pre_unwind}){
        AuxFixture f;f.attach();f.fake.fault=fault;
        if(fault==AuxFault::unwind||fault==AuxFault::unwind_input||fault==AuxFault::unwind_reenter)C(aux_exception(&f.fake)==0xe042c043);
        else C(aux_invoke(&f.fake,false)==0xfedcba9876543210ull);
        C(f.fake.originals==1&&f.fake.pres==1&&f.broker.aux_debt()&&f.broker.draining()&&!f.broker.pending_aux_scopes()&&!f.broker.pending_callbacks());
        if(fault==AuxFault::pre||fault==AuxFault::pre_input||fault==AuxFault::reenter)C(f.fake.returns==0&&f.fake.unwinds==1&&f.fake.reason==c::UnwindReason::notification_fault);
        if(fault==AuxFault::returned||fault==AuxFault::returned_input||fault==AuxFault::unknown_end)C(f.fake.returns==1&&f.fake.unwinds==1&&f.fake.reason==c::UnwindReason::notification_fault);
        if(fault==AuxFault::pre_input||fault==AuxFault::returned_input)C(c::same(f.fake.observed_pre,f.fake.observed_end));
        if(fault==AuxFault::reenter)C(f.fake.reentered==w::Reply::reentrant);
        f.fake.fault=AuxFault::none;C(f.broker.unregister(f.result.cookie)==w::Reply::draining);C(!f.broker.quiescent());
        w::CallScope no_new{};C(!f.broker.begin_call(1,2,3,4,no_new));f.finish(false);
    }
}
void auxiliary_scope_validation_tests(){
    {AuxFixture f;f.attach();w::CallScope scope{};C(f.broker.begin_call(1,2,3,4,scope));auto copy=scope;
        f.broker.finish_call(copy,true,0);C(f.broker.aux_debt()&&f.broker.pending_aux_scopes()==1&&f.fake.returns==0);
        f.broker.finish_call(scope,true,0);C(!f.broker.pending_aux_scopes());f.finish(false);}
    {AuxFixture f;f.attach();w::CallScope scope{};C(f.broker.begin_call(1,2,3,4,scope));
        test_thread=worker;f.broker.finish_call(scope,true,0);C(f.broker.pending_aux_scopes()==1&&f.broker.aux_debt());
        test_thread=gt;f.broker.finish_call(scope,true,0);f.finish(false);}
    {AuxFixture f;f.attach();w::CallScope scope{};C(f.broker.begin_call(1,2,3,4,scope));C(!f.broker.begin_call(9,8,7,6,scope));
        C(scope.invocation.self==1&&f.broker.pending_aux_scopes()==1);f.broker.finish_call(scope,true,0);f.finish(false);}
    {AuxFixture f;f.attach();w::CallScope outer{},inner{};C(f.broker.begin_call(1,2,3,4,outer)&&f.broker.begin_call(5,6,7,8,inner));
        f.broker.finish_call(outer,true,0);C(f.broker.pending_aux_scopes()==2&&f.fake.returns==0&&f.broker.aux_debt());
        f.broker.finish_call(inner,true,0);f.broker.finish_call(outer,true,0);C(!f.broker.pending_aux_scopes()&&f.fake.returns==2);f.finish(false);}
    {AuxFixture f;f.attach();w::CallScope scopes[65]{};for(unsigned i=0;i<64;++i)C(f.broker.begin_call(i,0,0,0,scopes[i]));
        C(!f.broker.begin_call(0,0,0,0,scopes[64])&&f.broker.pending_aux_scopes()==64&&f.fake.pres==64);
        for(unsigned i=64;i>0;--i)f.broker.finish_call(scopes[i-1],true,0);
        C(!f.broker.pending_aux_scopes()&&f.fake.returns==64&&f.broker.aux_debt());f.finish(false);}
}
void auxiliary_stop_race_tests(){
    for(unsigned phase:{1u,2u,3u}){
        AuxFixture f;f.attach();f.fake.entered=CreateEventW(nullptr,TRUE,FALSE,nullptr);f.fake.resume=CreateEventW(nullptr,TRUE,FALSE,nullptr);f.fake.block=phase;
        std::thread invoking([&](){test_thread=gt;if(phase==3)(void)aux_exception(&f.fake);else (void)aux_invoke(&f.fake,false);});
        C(WaitForSingleObject(f.fake.entered,3000)==WAIT_OBJECT_0);test_thread=worker;f.broker.invalidate(w::InvalidateReason::native_stopping);
        C(f.broker.pending_aux_scopes()==1&&f.broker.pending_callbacks()==1&&!f.broker.quiescent());
        C(f.broker.collect_on_owner_worker()==w::Reply::draining&&f.fake.unpins==0);SetEvent(f.fake.resume);invoking.join();
        C(f.fake.invalidations>=2&&!f.broker.aux_debt()&&!f.broker.pending_aux_scopes()&&f.broker.quiescent());
        CloseHandle(f.fake.entered);CloseHandle(f.fake.resume);f.fake.block=0;f.finish();
    }
    AuxFixture f;f.attach();w::CallScope scope{};C(f.broker.begin_call(0,0,0,0,scope));
    std::thread stop([&](){test_thread=worker;f.broker.invalidate(w::InvalidateReason::native_stopping);C(f.broker.collect_on_owner_worker()==w::Reply::draining);});stop.join();
    C(f.broker.pending_aux_scopes()==1&&!f.broker.pending_callbacks()&&!f.broker.pending_transactions()&&f.fake.unpins==0);
    f.broker.finish_call(scope,true,0);f.finish();
}
void auxiliary_tests(){auxiliary_registration_tests();auxiliary_normal_nested_tests();auxiliary_drain_retired_tests();auxiliary_fault_tests();auxiliary_scope_validation_tests();auxiliary_stop_race_tests();}
