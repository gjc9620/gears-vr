// Real Broker/runtime/sole-CF bridge; fake original and module ref only.
// Fresh process per scenario. No game, hook installation or engine is used.
#include "weapon_hand_broker.h"
#include <new>
#include <cstdio>
#include <cstdlib>
#include <thread>
namespace w=gears_weapon_hand;namespace a=gears_weapon_action;namespace b=gears_weapon_boundary;namespace c=gears_weapon_call;
namespace ctx=gears_native_context;
unsigned checks{},failures{};
#define C(x) do{++checks;if(!(x)){++failures;std::printf("FAIL %u: %s\n",__LINE__,#x);}}while(0)
thread_local std::uint32_t test_thread=1;constexpr std::uint32_t worker=1,gt=2;
bool continuous_session=true,stereo_xr=true,anchor_observer_enabled=true;
enum class Mode {normal,related,nested,unknown,pre_fault,end_fault,mutation,unresolved,stop_begin,stop_original,original_exception,v1_reenter,scene_original,external_nested,
    end_mutation,unwind_mutation,unwind_fault,unknown_end,end_unresolved,frame_reenter,invalidate_reenter,end_reenter,frame_external,noauth_reenter,stop_related};
struct Fake {
    unsigned pins{},unpins{},invalidations{},gates{},ends{},unwinds{},v1begins{},v1ends{},originals{},queries{},violations{};
    std::uintptr_t module{};Mode mode{};bool nested{};std::uint64_t cookie{};unsigned flags[128]{};
    char trace[1024]{};unsigned used{};w::Reply nested_open{};
    void log(char value) noexcept {if(used+1<sizeof(trace)){trace[used++]=value;trace[used]=0;}else ++violations;}
    static bool pin(void*,w::Registration const&,w::ModulePin&) noexcept;
    static bool unpin(void*,w::ModulePin) noexcept;
    static std::int64_t clock(void*) noexcept {return 1000000000;}
    static std::uint32_t thread(void*) noexcept {return test_thread;}
    static std::uint32_t prepare(void*,w::Frame const* frame,w::Proposal* proposal) noexcept;
    static std::uint32_t commit(void*,w::Frame const*,w::Proposal const*,w::Decision const*) noexcept {return unsigned(w::Reply::accepted);}
    static void invalidate(void*,w::Invalidation const*) noexcept;
    static bool call_module(void*,c::Registration const&,w::ModulePin) noexcept;
    static bool action_module(void*,a::Registration const&,w::ModulePin) noexcept;
    static void call_begin(void*,c::Invocation const*,c::State*) noexcept;
    static c::End call_returned(void*,c::Invocation const*,c::State*,std::uint64_t) noexcept;
    static c::End call_unwind(void*,c::Invocation const*,c::State*,c::UnwindReason) noexcept;
    static a::BeginReply action_begin(void*,a::Invocation const*,a::State*) noexcept;
    static a::End action_returned(void*,a::Invocation const*,a::State*,std::uint64_t) noexcept;
    static a::End action_unwind(void*,a::Invocation const*,a::State*,a::UnwindReason) noexcept;
} fake;
w::BrokerBackend fake_backend();
#define GEARS_WEAPON_HAND_RUNTIME_TEST_BACKEND ::fake_backend
#include "weapon_hand_runtime.inl"
#undef GEARS_WEAPON_HAND_RUNTIME_TEST_BACKEND
namespace rt=weapon_hand_runtime;
#include "weapon_call_live.inl"
constexpr std::uintptr_t sentinel=0xfedcba9876543210ull;
constexpr DWORD original_fault=0xe042c071,notice_fault=0xe042c072;
w::Frame frame(std::uint64_t number=1){
    w::Frame f{};auto binding=rt::session();f.flags=w::scene_owned|w::input_matched|w::gameplay_interactable;
    f.key={binding.game_start,binding.native_started_qpc,fake.cookie,1,number,3,binding.game_pid,gt};f.now_ns=1000000000;
    f.world={0x1000,0,0x9000,1,2,0,1};f.controller={0x2000,0x1000,0x9000,2,3,0,1};
    f.pawn={0x3000,0x1000,0x9000,3,4,0,1};f.mesh={0x4000,0x3000,0x9000,4,5,0,1};
    f.reference_generation=1;f.reference={{0,1.6,0},{}};f.stereo_status=unsigned(gears_vr::StereoStatus::accepted);
    auto& s=f.stereo;s.session=4;s.sequence=number;s.space_epoch=1;s.received_ns=f.now_ns;s.predicted_time=f.now_ns+10000;
    s.focused=s.visible=s.should_render=1;s.head_flags=s.view_flags=15;s.head={{0,1.6,0},{}};
    s.eyes[0].pose={{-.032,1.6,0},{}};s.eyes[1].pose={{.032,1.6,0},{}};for(auto& eye:s.eyes)eye.fov={-.7,.7,.7,-.7};
    auto& h=f.controllers;h.session=s.session;h.sequence=s.sequence;h.space_epoch=s.space_epoch;h.predicted_time=s.predicted_time;
    h.received_ns=f.now_ns;h.sync_ns=f.now_ns-1000;h.focused=1;f.press_token=7;f.left_grip_down=f.left_grip_known=f.left_tracked=1;return f;
}
w::Decision decision(w::Frame const& f,w::Proposal const& p){
    w::Decision d{};d.key=f.key;d.proposal_nonce=p.nonce;d.press_token=f.press_token;d.now_ns=f.now_ns;
    d.lease={w::LeaseKind::support,0,f.key.lifecycle,f.press_token};d.status=w::DecisionStatus::granted;d.new_grant=1;d.release_required=0;return d;
}
b::Request request(){b::Request r{};r.source=b::Source::process_event;r.flags=b::related;r.hand_cookie=fake.cookie;r.native_started_qpc=500;
    r.context=reinterpret_cast<std::uintptr_t>(&fake);r.object=1;r.function=4;r.thread_id=gt;return r;}
void stop(){std::thread worker_call([]{test_thread=worker;rt::request_stop();rt::worker_tick();});worker_call.join();}
std::uintptr_t invoke();
std::uint32_t Fake::prepare(void*,w::Frame const* frame,w::Proposal* proposal) noexcept {
    *proposal={};proposal->key=frame->key;proposal->nonce=frame->key.frame*17+1;proposal->support=1;
    if(fake.mode==Mode::frame_reenter&&!fake.nested){fake.nested=true;(void)invoke();}
    if(fake.mode==Mode::frame_external){auto r=request();b::State state{};b::Receipt receipt{};
        C(rt::open_boundary(&r,&state,&receipt)==w::Reply::reentrant&&!receipt.flags&&!receipt.scope_id);}
    return unsigned(w::Reply::accepted);
}
bool Fake::pin(void*,w::Registration const&,w::ModulePin& out) noexcept {
    ++fake.pins;if(!w::weapon_broker_win32::executable_image(reinterpret_cast<std::uintptr_t>(prepare),fake.module))return false;
    out={fake.module,fake.module};return true;
}
bool Fake::unpin(void*,w::ModulePin) noexcept {
    ++fake.unpins;auto* broker=rt::current();
    if(test_thread!=worker||!broker||broker->pending_callbacks()||broker->pending_transactions()||broker->pending_aux_scopes()||
       broker->pending_action_scopes()||broker->pending_external_scopes()||broker->aux_debt()||broker->action_debt())++fake.violations;return true;
}
void Fake::invalidate(void*,w::Invalidation const*) noexcept {++fake.invalidations;fake.log('I');if(rt::current()->authorized())++fake.violations;
    if(fake.mode==Mode::invalidate_reenter&&!fake.nested){fake.nested=true;(void)invoke();}}
bool Fake::call_module(void*,c::Registration const& r,w::ModulePin pin) noexcept {++fake.queries;return w::weapon_broker_win32::call_module(nullptr,r,pin);}
bool Fake::action_module(void*,a::Registration const& r,w::ModulePin pin) noexcept {++fake.queries;return w::weapon_broker_win32::action_module(nullptr,r,pin);}
w::BrokerBackend fake_backend(){return {nullptr,Fake::pin,Fake::unpin,Fake::clock,Fake::thread,w::weapon_broker_win32::prepare,w::weapon_broker_win32::commit,w::weapon_broker_win32::invalidate,
    Fake::call_module,w::weapon_broker_win32::call_begin,w::weapon_broker_win32::call_returned,w::weapon_broker_win32::call_unwind,
    Fake::action_module,w::weapon_broker_win32::action_begin,w::weapon_broker_win32::action_returned,w::weapon_broker_win32::action_unwind};}
std::uintptr_t __fastcall original(void* self,void* caller,void* result,void* callee){
    C(self==reinterpret_cast<void*>(1)&&caller==reinterpret_cast<void*>(2)&&result==reinterpret_cast<void*>(3)&&callee==reinterpret_cast<void*>(4));
    ++fake.originals;fake.log('O');auto* broker=rt::current();
    if(fake.mode==Mode::frame_reenter||fake.mode==Mode::invalidate_reenter)C(!broker->pending_action_scopes()&&!broker->pending_aux_scopes());
    else if(fake.mode==Mode::end_reenter||fake.mode==Mode::noauth_reenter)C(broker->pending_action_scopes());else C(broker->pending_action_scopes()&&broker->pending_aux_scopes());
    if(fake.mode==Mode::stop_original){stop();C(!fake.unpins&&broker->pin_retained());}
    if(fake.mode==Mode::related||fake.mode==Mode::stop_related)C(!fake.invalidations&&rt::action_barrier()&&!broker->authorized());
    if(fake.mode==Mode::scene_original){C(!rt::action_barrier());auto f=frame(2);w::Proposal p{};C(broker->prepare(f,p)==w::Reply::accepted);
        C(broker->commit(f,p,decision(f,p))==w::Reply::accepted&&broker->authorized());}
    if(fake.mode==Mode::original_exception||fake.mode==Mode::unwind_mutation||fake.mode==Mode::unwind_fault)RaiseException(original_fault,0,0,nullptr);return sentinel;
}
std::uintptr_t invoke(){return weapon_call_live::invoke(original,reinterpret_cast<void*>(1),reinterpret_cast<void*>(2),reinterpret_cast<void*>(3),reinterpret_cast<void*>(4));}
unsigned invoke_exception() noexcept {
    __try {(void)invoke();return 0;}__except(GetExceptionCode()==original_fault?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){return GetExceptionCode();}
}
void Fake::call_begin(void*,c::Invocation const* invocation,c::State* state) noexcept {
    ++fake.v1begins;fake.log('B');std::memcpy(state->storage.data(),&invocation->call_id,sizeof(invocation->call_id));
    if(fake.mode==Mode::v1_reenter&&!fake.nested){fake.nested=true;(void)invoke();}
}
c::End Fake::call_returned(void*,c::Invocation const* invocation,c::State* state,std::uint64_t value) noexcept {
    ++fake.v1ends;fake.log('C');std::uint64_t token{};std::memcpy(&token,state->storage.data(),sizeof(token));C(token==invocation->call_id&&value==sentinel);return c::End::closed;
}
c::End Fake::call_unwind(void*,c::Invocation const*,c::State*,c::UnwindReason) noexcept {++fake.v1ends;fake.log('c');return c::End::closed;}
a::BeginReply Fake::action_begin(void*,a::Invocation const* invocation,a::State* state) noexcept {
    C(a::invocation_valid(*invocation)&&rt::action_barrier()&&!rt::current()->authorized());
    C(invocation->self==1&&invocation->caller_frame==2&&invocation->result==3&&invocation->callee==4);
    fake.flags[fake.gates++]=invocation->flags;fake.log('A');const a::State zero{};C(std::memcmp(&zero,state,sizeof(zero))==0);
    std::memcpy(state->storage.data(),&invocation->gate_id,sizeof(invocation->gate_id));
    if((fake.mode==Mode::nested||fake.mode==Mode::noauth_reenter)&&!fake.nested){fake.nested=true;C(invoke()==sentinel);}
    if(fake.mode==Mode::external_nested){auto r=request();b::State nested{};b::Receipt receipt{};
        fake.nested_open=rt::open_boundary(&r,&nested,&receipt);const b::State empty{};
        C(fake.nested_open==w::Reply::reentrant&&std::memcmp(&empty,&nested,sizeof(empty))==0&&!receipt.flags&&!receipt.scope_id);}
    if(fake.mode==Mode::stop_begin||fake.mode==Mode::stop_related){stop();C(!fake.unpins&&!fake.invalidations);}
    if(fake.mode==Mode::pre_fault)RaiseException(notice_fault,0,0,nullptr);
    if(fake.mode==Mode::mutation)++const_cast<a::Invocation*>(invocation)->callee;
    if(fake.mode==Mode::unknown)return static_cast<a::BeginReply>(99);
    if(fake.mode==Mode::unresolved)return a::BeginReply::handback_unresolved;
    return fake.mode==Mode::related||fake.mode==Mode::stop_related?a::BeginReply::handback_complete:a::BeginReply::unrelated;
}
a::End Fake::action_returned(void*,a::Invocation const* invocation,a::State* state,std::uint64_t value) noexcept {
    ++fake.ends;fake.log('D');std::uint64_t token{};std::memcpy(&token,state->storage.data(),sizeof(token));C(token==invocation->gate_id&&value==sentinel);
    if(fake.mode==Mode::end_mutation)++const_cast<a::Invocation*>(invocation)->self;
    if(fake.mode==Mode::unknown_end)return static_cast<a::End>(99);
    if(fake.mode==Mode::end_unresolved)return a::End::unresolved;
    if(fake.mode==Mode::end_reenter&&!fake.nested){fake.nested=true;(void)invoke();}
    if(fake.mode==Mode::end_fault)RaiseException(notice_fault,0,0,nullptr);return a::End::closed;
}
a::End Fake::action_unwind(void*,a::Invocation const* invocation,a::State* state,a::UnwindReason reason) noexcept {
    ++fake.unwinds;fake.log('d');std::uint64_t token{};std::memcpy(&token,state->storage.data(),sizeof(token));C(token==invocation->gate_id);
    C(reason==((fake.mode==Mode::original_exception||fake.mode==Mode::unwind_mutation||fake.mode==Mode::unwind_fault)?a::UnwindReason::original_exception:a::UnwindReason::notification_fault));
    if(fake.mode==Mode::unwind_mutation)++const_cast<a::Invocation*>(invocation)->result;
    if(fake.mode==Mode::unwind_fault)RaiseException(notice_fault,0,0,nullptr);return a::End::closed;
}
void initialize(bool enable_restore=true,bool certify=true){
    wchar_t directory[MAX_PATH]{},path[MAX_PATH]{};C(GetTempPathW(MAX_PATH,directory)>0);C(GetTempFileNameW(directory,L"wag",0,path)!=0);
    C(WritePrivateProfileStringW(L"WeaponHandParticipant",L"enabled",L"1",path)!=FALSE);
    C(WritePrivateProfileStringW(L"WeaponActionGate",L"engine_restore",enable_restore?L"1":L"0",path)!=FALSE);
    C(rt::configure(path,nullptr));C(DeleteFileW(path)!=FALSE);C(rt::initialize(400,500,worker));
    w::Registration hand{};hand.context=&fake;hand.prepare=Fake::prepare;hand.commit=Fake::commit;hand.invalidate=Fake::invalidate;
    w::RegistrationResult receipt{};C(rt::register_participant(&hand,&receipt)==w::Reply::accepted);fake.cookie=receipt.cookie;
    a::Registration gate{};gate.hand_cookie=fake.cookie;gate.native_started_qpc=500;gate.context=&fake;gate.begin=Fake::action_begin;gate.returned=Fake::action_returned;gate.unwind=Fake::action_unwind;
    C(rt::attach_action_gate(&gate,&receipt,true)==w::Reply::invalid); // call must be first
    c::Registration call{};call.hand_cookie=fake.cookie;call.native_started_qpc=500;call.context=&fake;call.begin=Fake::call_begin;call.returned=Fake::call_returned;call.unwind=Fake::call_unwind;
    C(rt::attach_call_participant(&call,&receipt,true)==w::Reply::accepted);
    C(rt::attach_action_gate(&gate,&receipt,false)==w::Reply::invalid);
    auto changed=gate;changed.context=&receipt;C(rt::attach_action_gate(&changed,&receipt,true)==w::Reply::invalid);
    changed=gate;changed.begin=reinterpret_cast<a::Begin>(GetTickCount64);C(rt::attach_action_gate(&changed,&receipt,true)==w::Reply::invalid);
    changed=gate;changed.begin=reinterpret_cast<a::Begin>(&receipt);C(rt::attach_action_gate(&changed,&receipt,true)==w::Reply::invalid);
    C(rt::attach_action_gate(&gate,&receipt,true)==w::Reply::accepted);
    C(rt::attach_action_gate(&gate,&receipt,true)==w::Reply::accepted&&fake.pins==1);
    test_thread=gt;
    if(certify){auto f=frame();w::Proposal p{};C(rt::current()->prepare(f,p)==w::Reply::accepted);
        C(rt::current()->commit(f,p,decision(f,p))==w::Reply::accepted&&rt::current()->authorized());}
    fake.used=0;fake.trace[0]=0;
}
void finish(bool clean=true){stop();C(fake.violations==0);
    C(clean?(rt::cleanup_ready()&&!rt::retained()&&fake.unpins==1):(!rt::cleanup_ready()&&rt::retained()&&fake.unpins==0&&rt::current()->action_debt()));}
void classify(){ctx::Ledger ledger{};C(w::classify_action_context(ledger,true,true,true));
    C(!w::classify_action_context(ledger,false,true,true)&&!w::classify_action_context(ledger,true,false,true)&&!w::classify_action_context(ledger,true,true,false));
    for(unsigned i=0;i<unsigned(ctx::Kind::count);++i){ctx::Ticket ticket{};C(ctx::enter(ledger,ticket,static_cast<ctx::Kind>(i)));
        C(!w::classify_action_context(ledger,true,true,true));C(ctx::leave(ledger,ticket));}
    ledger.poisoned=true;C(!w::classify_action_context(ledger,true,true,true));
    initialize(false);fake.mode=Mode::related;C(invoke()==sentinel&&fake.flags[0]==0);finish();
}
void normal(){initialize();fake.mode=Mode::scene_original;C(invoke()==sentinel);C(fake.flags[0]==a::engine_calls_allowed);
    C(!rt::current()->pending_action_scopes()&&!rt::current()->pending_aux_scopes()&&!rt::action_barrier());finish();}
void external(bool stopping){initialize();auto r=request();std::uint64_t generation{};const auto queries=fake.queries;
    C(rt::revoke_action(&r,&generation)==w::Reply::accepted&&generation==rt::action_generation()&&queries==fake.queries&&!rt::current()->authorized());
    b::State state{};b::Receipt receipt{};C(rt::open_boundary(&r,&state,&receipt)==w::Reply::accepted);
    C(receipt.flags==(b::scope_active|b::engine_calls_allowed)&&receipt.revocation_generation==generation&&rt::action_barrier());
    C(rt::current()->pending_external_scopes()==1);auto f=frame(2);w::Proposal p{};C(rt::current()->prepare(f,p)==w::Reply::busy);
    std::thread wrong_gt([&]{test_thread=3;b::State other{};b::Receipt noauth{};
        C(rt::open_boundary(&r,&other,&noauth)==w::Reply::unavailable&&!noauth.flags&&!noauth.scope_id);});wrong_gt.join();
    b::State nested{};b::Receipt noauth{};C(rt::open_boundary(&r,&nested,&noauth)==w::Reply::reentrant&&!noauth.flags&&!noauth.scope_id);
    fake.mode=Mode::external_nested;C(invoke()==sentinel&&fake.flags[0]==0&&!rt::current()->action_debt());
    if(stopping){stop();C(!fake.unpins&&!rt::cleanup_ready()&&rt::current()->pending_external_scopes()==1);}
    b::Finish end{};end.flags=b::stage_closed|b::handback_complete;end.outcome=b::Outcome::original_exception;
    C(rt::close_boundary(&state,&receipt,&end)==b::End::closed);const b::State zero{};C(std::memcmp(&state,&zero,sizeof(state))==0&&!rt::action_barrier());finish();
}
void generation(){initialize();auto f=frame(2);w::Proposal p{};C(rt::current()->prepare(f,p)==w::Reply::accepted);
    auto r=request();std::atomic<unsigned> rejected{};std::thread revoker([&]{for(unsigned i=0;i<100;++i){std::uint64_t gen{};if(rt::revoke_action(&r,&gen)!=w::Reply::accepted)++rejected;}});revoker.join();
    C(!rejected&&rt::current()->commit(f,p,decision(f,p))==w::Reply::unavailable&&!rt::current()->authorized());
    f=frame(3);C(rt::current()->prepare(f,p)==w::Reply::accepted);C(rt::current()->commit(f,p,decision(f,p))==w::Reply::accepted&&rt::current()->authorized());finish();
}
void bad_close(){initialize();auto r=request();b::State state{};b::Receipt receipt{};C(rt::open_boundary(&r,&state,&receipt)==w::Reply::accepted);
    auto changed=receipt;++changed.revocation_generation;b::Finish end{};end.flags=b::stage_closed|b::handback_complete;
    C(rt::close_boundary(&state,&changed,&end)==b::End::unresolved&&rt::current()->pending_external_scopes()==1);finish(false);}
void output_fault(){initialize();auto r=request();auto* state=static_cast<b::State*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));C(state!=nullptr);
    DWORD old{};C(VirtualProtect(state,4096,PAGE_READONLY,&old)!=FALSE);b::Receipt receipt{};
    C(rt::open_boundary(&r,state,&receipt)==w::Reply::invalid&&rt::current()->pending_external_scopes()==1);
    C(rt::current()->action_debt()&&!rt::cleanup_ready());finish(false);C(VirtualFree(state,0,MEM_RELEASE)!=FALSE);}
void unsafe(){initialize();auto r=request();b::State state{};b::Receipt receipt{};ctx::Ticket ticket{};
    C(ctx::enter(ctx::thread_ledger,ticket,ctx::Kind::stereo));C(rt::open_boundary(&r,&state,&receipt)==w::Reply::reentrant&&!receipt.flags&&!receipt.scope_id);
    C(ctx::leave(ctx::thread_ledger,ticket));test_thread=3;C(rt::open_boundary(&r,&state,&receipt)==w::Reply::unavailable&&!receipt.flags&&!receipt.scope_id);
    test_thread=gt;C(rt::open_boundary(&r,&state,&receipt)==w::Reply::accepted);b::State reopened{};
    // Even an overwritten-zero descriptor at the admitted address cannot be a clean refusal.
    state={};C(rt::open_boundary(&r,&state,&receipt)==w::Reply::invalid&&rt::current()->pending_external_scopes()==1);(void)reopened;finish(false);}
void noauth_gt(){initialize(true,false);auto r=request();b::State state{};b::Receipt receipt{};
    C(rt::open_boundary(&r,&state,&receipt)==w::Reply::unavailable&&!receipt.flags&&!receipt.scope_id);
    fake.mode=Mode::related;C(invoke()==sentinel&&fake.flags[0]==0);finish();}
void clean_noauth(){initialize();auto r=request();b::State state{};b::Receipt receipt{};std::memset(&receipt,0xa5,sizeof(receipt));
    ++r.hand_cookie;C(rt::open_boundary(&r,&state,&receipt)==w::Reply::wrong_cookie&&!receipt.flags&&!receipt.scope_id);r=request();
    auto f=frame(2);w::Proposal p{};C(rt::current()->prepare(f,p)==w::Reply::accepted);
    C(rt::open_boundary(&r,&state,&receipt)==w::Reply::busy&&!receipt.flags&&!receipt.scope_id);
    C(rt::current()->abort_frame(f.key,p.nonce)==w::Reply::accepted);finish();
    C(rt::open_boundary(&r,&state,&receipt)==w::Reply::draining&&!receipt.flags&&!receipt.scope_id);
    const b::State zero{};C(std::memcmp(&state,&zero,sizeof(state))==0&&rt::cleanup_ready());}
void oldframe_gap(){initialize();auto f=frame(2);w::Proposal p{};C(rt::current()->prepare(f,p)==w::Reply::accepted);
    w::ActionScope gate{};w::CallScope call{};C(rt::begin_action(1,2,3,4,gate)&&gate.invocation.flags==0);
    C(rt::begin_call(1,2,3,4,call,&gate));stop();C(!fake.unpins&&rt::current()->pending_transactions()==1);
    rt::finish_call(call,true,sentinel);rt::finish_action(gate,true,sentinel);
    C(!rt::current()->pending_action_scopes()&&!rt::current()->pending_aux_scopes()&&rt::current()->pending_transactions()==1);
    C(rt::current()->abort_frame(f.key,p.nonce)==w::Reply::accepted&&rt::current()->pending_action_invalidation()&&rt::current()->pin_retained());finish();}
void external_obligation(){initialize();auto r=request();b::State state{};b::Receipt receipt{};
    C(rt::open_boundary(&r,&state,&receipt)==w::Reply::accepted);w::CallScope call{};C(rt::begin_call(1,2,3,4,call));
    b::Finish end{};end.flags=b::stage_closed|b::handback_complete;
    C(rt::close_boundary(&state,&receipt,&end)==b::End::closed&&rt::current()->pending_aux_scopes()==1);
    stop();C(!fake.unpins&&rt::retained());rt::finish_call(call,true,sentinel);finish();}
void external_bad(const char* mode){initialize();auto r=request();b::State state{};b::Receipt receipt{};
    C(rt::open_boundary(&r,&state,&receipt)==w::Reply::accepted);b::Finish end{};end.flags=b::stage_closed|b::handback_complete;
    if(!std::strcmp(mode,"external-copy")){auto copy=state;C(rt::close_boundary(&copy,&receipt,&end)==b::End::unresolved);}
    else {C(rt::close_boundary(&state,&receipt,&end)==b::End::closed);C(rt::close_boundary(&state,&receipt,&end)==b::End::unresolved);}
    finish(false);}
void fast_fault(const char* mode){initialize();auto r=request();const auto callbacks=fake.invalidations,queries=fake.queries;std::uint64_t generation{};
    if(!std::strcmp(mode,"fast-output-fault"))C(rt::revoke_action(&r,reinterpret_cast<std::uint64_t*>(1))==w::Reply::invalid);
    else if(!std::strcmp(mode,"fast-input-fault"))C(rt::revoke_action(reinterpret_cast<b::Request const*>(1),&generation)==w::Reply::invalid);
    else if(!std::strcmp(mode,"fast-unresolved")){r.flags|=b::unresolved;C(rt::revoke_action(&r,&generation)==w::Reply::draining&&generation);}
    else {++r.abi_version;C(rt::revoke_action(&r,&generation)==w::Reply::invalid);}
    C(callbacks==fake.invalidations&&queries==fake.queries&&rt::current()->action_debt());finish(false);}
void overlap(){initialize();auto r=request();b::State state{};
    C(rt::open_boundary(&r,&state,reinterpret_cast<b::Receipt*>(&state))==w::Reply::invalid&&!rt::current()->pending_external_scopes());finish(false);}
void forbidden(const char* mode){initialize();
    if(!std::strcmp(mode,"frame-external")){fake.mode=Mode::frame_external;auto f=frame(2);w::Proposal p{};
        C(rt::current()->prepare(f,p)==w::Reply::accepted);C(rt::current()->commit(f,p,decision(f,p))==w::Reply::accepted);finish();return;}
    if(!std::strcmp(mode,"frame-reenter")){fake.mode=Mode::frame_reenter;auto f=frame(2);w::Proposal p{};C(rt::current()->prepare(f,p)==w::Reply::invalid);}
    else {fake.mode=Mode::invalidate_reenter;(void)rt::current()->invalidate(w::InvalidateReason::input_lost);}
    C(fake.originals==1&&!fake.gates&&!fake.v1begins);finish(false);}
void deferred_frame(){initialize();fake.mode=Mode::related;C(invoke()==sentinel&&fake.invalidations==0);
    C(rt::current()->pending_action_invalidation()&&!rt::current()->authorized()&&!rt::action_barrier());
    std::thread observe([]{test_thread=worker;rt::worker_tick();});observe.join();
    C(fake.invalidations==0&&rt::current()->pin_retained());fake.mode=Mode::normal;auto inactive=frame(2);inactive.flags&=~w::gameplay_interactable;w::Proposal p{};
    C(rt::current()->prepare(inactive,p)==w::Reply::unavailable&&fake.invalidations==0&&rt::current()->pending_action_invalidation());auto f=frame(3);
    C(rt::current()->prepare(f,p)==w::Reply::accepted&&fake.invalidations==1&&!rt::current()->pending_action_invalidation());
    C(rt::current()->commit(f,p,decision(f,p))==w::Reply::accepted&&rt::current()->authorized());
    f=frame(4);C(rt::current()->prepare(f,p)==w::Reply::accepted&&fake.invalidations==1);
    C(rt::current()->commit(f,p,decision(f,p))==w::Reply::accepted);finish();}
void deferred_stop(){initialize();fake.mode=Mode::related;C(invoke()==sentinel&&fake.invalidations==0&&rt::current()->pending_action_invalidation());
    stop();C(fake.invalidations==1&&!rt::current()->pending_action_invalidation()&&rt::cleanup_ready());
    stop();C(fake.invalidations==1&&fake.unpins==1);}
void none_commits(){initialize();for(unsigned i=0;i<3;++i){auto f=frame(i+2);w::Proposal p{};C(rt::current()->prepare(f,p)==w::Reply::accepted);
    auto d=decision(f,p);d.lease={};d.new_grant=0;d.release_required=i==2?1u:0u;
    d.status=i==0?w::DecisionStatus::released:i==1?w::DecisionStatus::no_candidate:w::DecisionStatus::needs_release;
    C(rt::current()->commit(f,p,d)==w::Reply::accepted&&!rt::current()->authorized()&&!rt::current()->pending_transactions());}finish();}
void scope_validation(const char* mode){initialize();w::ActionScope scopes[65]{};
    const unsigned count=!std::strcmp(mode,"scope-capacity")?64u:!std::strcmp(mode,"scope-lifo")?2u:1u;
    for(unsigned i=0;i<count;++i)C(rt::begin_action(1,2,3,4,scopes[i]));
    if(!std::strcmp(mode,"scope-copy")){auto copy=scopes[0];rt::finish_action(copy,true,sentinel);}
    else if(!std::strcmp(mode,"scope-thread")){test_thread=3;rt::finish_action(scopes[0],true,sentinel);test_thread=gt;}
    else if(!std::strcmp(mode,"scope-lifo"))rt::finish_action(scopes[0],true,sentinel);
    else C(!rt::begin_action(1,2,3,4,scopes[64]));
    C(rt::current()->action_debt()&&rt::current()->pending_action_scopes()==count);
    for(unsigned i=count;i>0;--i)rt::finish_action(scopes[i-1],true,sentinel);
    C(!rt::current()->pending_action_scopes()&&fake.ends==count);finish(false);}
int main(int argc,char** argv){const char* mode=argc>1?argv[1]:"classify";
    if(!std::strcmp(mode,"classify"))classify();else if(!std::strcmp(mode,"normal"))normal();
    else if(!std::strcmp(mode,"external"))external(false);else if(!std::strcmp(mode,"external-stop"))external(true);
    else if(!std::strcmp(mode,"generation"))generation();else if(!std::strcmp(mode,"wrong-close"))bad_close();
    else if(!std::strcmp(mode,"output-fault"))output_fault();else if(!std::strcmp(mode,"unsafe"))unsafe();
    else if(!std::strcmp(mode,"noauth-gt"))noauth_gt();else if(!std::strcmp(mode,"clean-noauth"))clean_noauth();
    else if(!std::strcmp(mode,"oldframe-gap"))oldframe_gap();else if(!std::strcmp(mode,"external-obligation"))external_obligation();
    else if(!std::strcmp(mode,"external-copy")||!std::strcmp(mode,"external-duplicate"))external_bad(mode);
    else if(!std::strncmp(mode,"fast-",5))fast_fault(mode);else if(!std::strcmp(mode,"output-overlap"))overlap();
    else if(!std::strcmp(mode,"frame-reenter")||!std::strcmp(mode,"invalidate-reenter")||!std::strcmp(mode,"frame-external"))forbidden(mode);
    else if(!std::strcmp(mode,"deferred-frame"))deferred_frame();else if(!std::strcmp(mode,"deferred-stop"))deferred_stop();
    else if(!std::strcmp(mode,"none-commits"))none_commits();else if(!std::strncmp(mode,"scope-",6))scope_validation(mode);
    else {initialize(std::strcmp(mode,"noauth-reenter")!=0);bool clean=false;
        if(!std::strcmp(mode,"related")){fake.mode=Mode::related;clean=true;}
        else if(!std::strcmp(mode,"nested")){fake.mode=Mode::nested;clean=true;}
        else if(!std::strcmp(mode,"unknown"))fake.mode=Mode::unknown;
        else if(!std::strcmp(mode,"pre-fault"))fake.mode=Mode::pre_fault;
        else if(!std::strcmp(mode,"end-fault"))fake.mode=Mode::end_fault;
        else if(!std::strcmp(mode,"mutation"))fake.mode=Mode::mutation;
        else if(!std::strcmp(mode,"unresolved"))fake.mode=Mode::unresolved;
        else if(!std::strcmp(mode,"stop-begin")){fake.mode=Mode::stop_begin;clean=true;}
        else if(!std::strcmp(mode,"stop-original")){fake.mode=Mode::stop_original;clean=true;}
        else if(!std::strcmp(mode,"stop-related")){fake.mode=Mode::stop_related;clean=true;}
        else if(!std::strcmp(mode,"original-exception")){fake.mode=Mode::original_exception;clean=true;}
        else if(!std::strcmp(mode,"v1-reenter"))fake.mode=Mode::v1_reenter;
        else if(!std::strcmp(mode,"end-reenter"))fake.mode=Mode::end_reenter;
        else if(!std::strcmp(mode,"end-mutation"))fake.mode=Mode::end_mutation;
        else if(!std::strcmp(mode,"unwind-mutation"))fake.mode=Mode::unwind_mutation;
        else if(!std::strcmp(mode,"unwind-fault"))fake.mode=Mode::unwind_fault;
        else if(!std::strcmp(mode,"unknown-end"))fake.mode=Mode::unknown_end;
        else if(!std::strcmp(mode,"end-unresolved"))fake.mode=Mode::end_unresolved;
        else if(!std::strcmp(mode,"noauth-reenter"))fake.mode=Mode::noauth_reenter;
        else C(false);
        if(fake.mode==Mode::original_exception||fake.mode==Mode::unwind_mutation||fake.mode==Mode::unwind_fault)C(invoke_exception()==original_fault);else C(invoke()==sentinel);
        C(fake.originals==(fake.mode==Mode::nested||fake.mode==Mode::v1_reenter||fake.mode==Mode::end_reenter||fake.mode==Mode::noauth_reenter?2u:1u));
        C(fake.v1begins==(fake.mode==Mode::nested?2u:1u)&&fake.v1ends==fake.v1begins);
        C(!rt::current()->pending_action_scopes()&&!rt::current()->pending_aux_scopes());
        if(fake.mode==Mode::related)C(!std::strcmp(fake.trace,"ABOCD")&&!fake.invalidations&&!rt::current()->authorized());
        if(fake.mode==Mode::stop_related){auto r=request();std::uint64_t generation{};
            C(rt::revoke_action(&r,&generation)==w::Reply::wrong_cookie&&!generation&&!fake.invalidations);}
        if(fake.mode==Mode::nested)C(fake.flags[0]==1&&fake.flags[1]==0&&!rt::current()->action_debt());
        finish(clean);
    }
    std::printf("weapon_action_provider %s: %u checks, %u failures\n",mode,checks,failures);return failures?1:0;
}
