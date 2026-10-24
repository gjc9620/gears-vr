// Real Broker/runtime integration with fake module refs and synthetic frozen
// Scene frames. Each command runs in a fresh test process: no global reset,
// Broker replacement, self-unload, game, owner/adapter or engine is involved.
#include "weapon_hand_broker.h"
#include <new>
#include <cstdio>
#include <cstdlib>
#include <thread>
namespace w=gears_weapon_hand;
unsigned checks{},failures{};
#define C(x) do{++checks;if(!(x)){++failures;std::printf("FAIL %u: %s\n",__LINE__,#x);}}while(0)
thread_local unsigned test_thread=1;
constexpr unsigned worker=1,gt=2;
bool continuous_session=true,stereo_xr=true,anchor_observer_enabled=true;
struct OwnFault {};
struct Fake {
    std::atomic<unsigned> pins{},unpins{},prepares{},commits{},invalidations{},violations{};
    std::atomic<long long> stamp{1000000000};
    bool unpin_ok=true,prepare_fault{},invalidate_fault{},reenter_stop{},factory_fault{};
    unsigned barrier_phase{};HANDLE entered{},resume{};
    w::Reply prepare_reply=w::Reply::accepted;
    static bool pin(void*,w::Registration const&,w::ModulePin&) noexcept;
    static bool unpin(void*,w::ModulePin) noexcept;
    static std::int64_t clock(void*) noexcept;
    static std::uint32_t thread(void*) noexcept;
    static std::uint32_t prepare(void*,w::Frame const*,w::Proposal*) noexcept;
    static std::uint32_t commit(void*,w::Frame const*,w::Proposal const*,w::Decision const*) noexcept;
    static void invalidate(void*,w::Invalidation const*) noexcept;
    void barrier(unsigned phase) noexcept {if(barrier_phase==phase){SetEvent(entered);if(WaitForSingleObject(resume,3000)!=WAIT_OBJECT_0)++violations;}}
    void start(unsigned phase){barrier_phase=phase;entered=CreateEventW(nullptr,TRUE,FALSE,nullptr);resume=CreateEventW(nullptr,TRUE,FALSE,nullptr);C(entered&&resume);}
    void end(){CloseHandle(entered);CloseHandle(resume);barrier_phase=0;entered=resume=nullptr;}
} fake;
w::BrokerBackend fake_backend();
#define GEARS_WEAPON_HAND_RUNTIME_TEST_BACKEND ::fake_backend
#include "weapon_hand_runtime.inl"
#undef GEARS_WEAPON_HAND_RUNTIME_TEST_BACKEND
namespace runtime=weapon_hand_runtime;

bool Fake::pin(void*,w::Registration const&,w::ModulePin& pin_value) noexcept {++fake.pins;fake.barrier(1);pin_value={0x9000,0x9000};return true;}
bool Fake::unpin(void*,w::ModulePin pin_value) noexcept {
    ++fake.unpins;auto* b=runtime::current();
    if(test_thread!=worker||pin_value.handle!=0x9000||!b||b->pending_callbacks()||b->pending_transactions())++fake.violations;
    return fake.unpin_ok;
}
std::int64_t Fake::clock(void*) noexcept {return fake.stamp.load();}
std::uint32_t Fake::thread(void*) noexcept {return test_thread;}
std::uint32_t Fake::prepare(void*,w::Frame const* f,w::Proposal* p) noexcept {
    ++fake.prepares;if(fake.prepare_fault)RaiseException(0xe042c030,0,0,nullptr);
    if(fake.reenter_stop)runtime::request_stop();
    *p={};p->key=f->key;p->nonce=f->key.frame*17+1;p->support=1;return unsigned(fake.prepare_reply);
}
std::uint32_t Fake::commit(void*,w::Frame const*,w::Proposal const*,w::Decision const*) noexcept {++fake.commits;return unsigned(w::Reply::accepted);}
void Fake::invalidate(void*,w::Invalidation const*) noexcept {++fake.invalidations;if(fake.invalidate_fault)RaiseException(0xe042c031,0,0,nullptr);}
w::BrokerBackend fake_backend(){
    fake.barrier(2);if(fake.factory_fault)throw OwnFault{};
    return {nullptr,Fake::pin,Fake::unpin,Fake::clock,Fake::thread,w::weapon_broker_win32::prepare,w::weapon_broker_win32::commit,w::weapon_broker_win32::invalidate};
}
bool configure(unsigned request=1,unsigned mask=7){
    continuous_session=(mask&1)!=0;stereo_xr=(mask&2)!=0;anchor_observer_enabled=(mask&4)!=0;
    wchar_t directory[MAX_PATH]{},path[MAX_PATH]{};C(GetTempPathW(MAX_PATH,directory)>0);C(GetTempFileNameW(directory,L"whr",0,path)!=0);
    wchar_t value[32]{};swprintf_s(value,L"%u",request);C(WritePrivateProfileStringW(L"WeaponHandParticipant",L"enabled",value,path)!=FALSE);
    FILE* status{};C(tmpfile_s(&status)==0&&status);const bool result=runtime::configure(path,status);
    if(status)fclose(status);C(DeleteFileW(path)!=FALSE);return result;
}
void initialize(){C(configure());C(!runtime::current()&&runtime::cleanup_ready());C(runtime::initialize(400,500,worker));C(runtime::current()!=nullptr);}
w::Registration registration(){w::Registration r{};r.context=&fake;r.prepare=Fake::prepare;r.commit=Fake::commit;r.invalidate=Fake::invalidate;return r;}
w::RegistrationResult attach(){auto r=registration();w::RegistrationResult result{};C(runtime::register_participant(&r,&result)==w::Reply::accepted);C(result.cookie&&result.native_started_qpc==500);return result;}
w::Frame frame(std::uint64_t cookie,std::uint64_t number=1){
    test_thread=gt;const auto binding=runtime::session();w::Frame f{};f.flags=w::scene_owned|w::input_matched|w::gameplay_interactable;
    f.key={binding.game_start,binding.native_started_qpc,cookie,1,number,3,binding.game_pid,gt};f.now_ns=fake.stamp.load();
    f.world={0x1000,0,0x9000,1,2,0,1};f.controller={0x2000,0x1000,0x9000,2,3,0,1};
    f.pawn={0x3000,0x1000,0x9000,3,4,0,1};f.mesh={0x4000,0x3000,0x9000,4,5,0,1};
    f.reference_generation=1;f.reference={{0,1.6,0},{}};f.stereo_status=unsigned(gears_vr::StereoStatus::accepted);
    auto& s=f.stereo;s.session=4;s.sequence=number;s.space_epoch=1;s.received_ns=f.now_ns;s.predicted_time=f.now_ns+10000;
    s.focused=s.visible=s.should_render=1;s.head_flags=s.view_flags=15;s.head={{0,1.6,0},{}};
    s.eyes[0].pose={{-.032,1.6,0},{}};s.eyes[1].pose={{.032,1.6,0},{}};for(auto& eye:s.eyes)eye.fov={-.7,.7,.7,-.7};
    auto& h=f.controllers;h.session=s.session;h.sequence=s.sequence;h.space_epoch=s.space_epoch;h.predicted_time=s.predicted_time;
    h.received_ns=f.now_ns;h.sync_ns=f.now_ns-1000;h.focused=1;
    // Synthetic predecoded logical input, not an owner or raw-counter adapter.
    f.press_token=7;f.left_grip_down=f.left_grip_known=f.left_tracked=1;C(w::frame_active(f,f.now_ns));return f;
}
w::Decision decision(w::Frame const& f,w::Proposal const& p){
    w::Decision d{};d.key=f.key;d.proposal_nonce=p.nonce;d.press_token=f.press_token;d.now_ns=fake.stamp.load();
    d.lease={w::LeaseKind::support,0,f.key.lifecycle,f.press_token};d.status=w::DecisionStatus::granted;d.new_grant=1;d.release_required=0;return d;
}
void finish(){test_thread=worker;runtime::request_stop();runtime::worker_tick();C(runtime::cleanup_ready()&&!runtime::retained());C(fake.violations==0);}
void lifecycle(){
    initialize();auto* original=runtime::current();auto binding=runtime::session();C(binding.game_pid==GetCurrentProcessId()&&binding.owner_worker_thread==worker&&binding.game_start==400&&binding.native_started_qpc==500);
    C(runtime::initialize(400,500,worker)&&runtime::current()==original);C(!runtime::initialize(401,500,worker)&&runtime::current()==original);
    test_thread=3;C(!runtime::initialize(400,500,worker));test_thread=worker;
    auto result=attach();C(runtime::retained()&&!runtime::cleanup_ready());C(runtime::unregister(result.cookie)==w::Reply::quiescent);
    runtime::worker_tick();C(!runtime::retained()&&!runtime::cleanup_ready()&&runtime::current()==original); // Gate still open.
    C(runtime::unregister(result.cookie)==w::Reply::quiescent);auto next=attach();C(next.cookie!=result.cookie);
    const auto revision=original->lifecycle_revision();C(runtime::unregister(result.cookie)==w::Reply::quiescent&&original->cookie()==next.cookie&&original->lifecycle_revision()==revision);
    finish();C(runtime::current()==original);C(runtime::initialize(400,500,worker)&&runtime::cleanup_ready());
    auto r=registration();w::RegistrationResult rejected{};C(runtime::register_participant(&r,&rejected)==w::Reply::draining);
    C(runtime::unregister(next.cookie)==w::Reply::quiescent);
}
void prepare_gap(){
    initialize();auto result=attach();auto f=frame(result.cookie);w::Proposal p{};auto* b=runtime::current();
    C(b->prepare(f,p)==w::Reply::accepted&&b->pending_transactions()==1);C(b->prepare(f,p)==w::Reply::busy&&fake.prepares==1);
    test_thread=worker;runtime::request_stop();runtime::worker_tick();C(!runtime::cleanup_ready()&&runtime::retained()&&fake.unpins==0);
    C(runtime::unregister(result.cookie)==w::Reply::draining);test_thread=gt;auto d=decision(f,p);
    C(b->commit(f,p,d)==w::Reply::draining&&fake.commits==0&&b->pending_transactions()==1);
    runtime::worker_tick();C(fake.unpins==0&&!runtime::cleanup_ready());
    C(b->abort_frame(f.key,p.nonce)==w::Reply::quiescent);test_thread=worker;runtime::worker_tick();
    C(runtime::cleanup_ready()&&!runtime::retained()&&fake.unpins==1);C(runtime::unregister(result.cookie)==w::Reply::quiescent);C(fake.violations==0);
}
void register_race(){
    initialize();fake.start(1);auto r=registration();w::RegistrationResult result{};std::atomic<w::Reply> reply{w::Reply::invalid};
    std::thread registering([&](){test_thread=3;reply=runtime::register_participant(&r,&result);});
    C(WaitForSingleObject(fake.entered,3000)==WAIT_OBJECT_0);test_thread=worker;runtime::request_stop();runtime::worker_tick();
    C(runtime::retained()&&!runtime::cleanup_ready()&&fake.unpins==0);SetEvent(fake.resume);registering.join();fake.end();
    C(reply==w::Reply::draining&&result.cookie==0&&runtime::retained());runtime::worker_tick();C(runtime::cleanup_ready()&&!runtime::retained()&&fake.unpins==1&&fake.violations==0);
}
void initialize_race(){
    C(configure());fake.start(2);std::atomic<bool> result{true};
    std::thread initializing([&](){test_thread=worker;result=runtime::initialize(400,500,worker);});
    C(WaitForSingleObject(fake.entered,3000)==WAIT_OBJECT_0);test_thread=3;runtime::request_stop();
    C(!runtime::current()&&!runtime::cleanup_ready()&&runtime::retained());SetEvent(fake.resume);initializing.join();fake.end();
    C(!result&&runtime::current()!=nullptr);test_thread=worker;runtime::worker_tick();C(runtime::cleanup_ready()&&!runtime::retained());
    auto r=registration();w::RegistrationResult rejected{};C(runtime::register_participant(&r,&rejected)==w::Reply::draining);
}
void fault_input(){
    initialize();w::RegistrationResult result{};
    C(runtime::register_participant(reinterpret_cast<w::Registration const*>(1),&result)==w::Reply::invalid);
    C(runtime::retained()&&!runtime::cleanup_ready()&&fake.pins==0);test_thread=worker;runtime::worker_tick();C(runtime::retained()&&!runtime::cleanup_ready());
}
void fault_output(){
    initialize();void* page=VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);C(page!=nullptr);
    w::RegistrationResult initial{};std::memcpy(page,&initial,sizeof(initial));DWORD old{};C(VirtualProtect(page,4096,PAGE_READONLY,&old)!=FALSE);
    auto r=registration();C(runtime::register_participant(&r,static_cast<w::RegistrationResult*>(page))==w::Reply::invalid);
    C(runtime::retained()&&!runtime::cleanup_ready()&&fake.pins==1);runtime::worker_tick();
    C(!runtime::current()->pin_retained()&&runtime::retained()&&!runtime::cleanup_ready()&&fake.unpins==1);
    C(VirtualFree(page,0,MEM_RELEASE)!=FALSE);
}
void factory_fault(){C(configure());fake.factory_fault=true;C(!runtime::initialize(400,500,worker));C(!runtime::current()&&runtime::retained()&&!runtime::cleanup_ready());runtime::worker_tick();C(runtime::retained()&&!runtime::cleanup_ready());}
void callback_fault(){
    initialize();auto result=attach();auto f=frame(result.cookie);w::Proposal p{};fake.prepare_fault=fake.invalidate_fault=true;
    C(runtime::current()->prepare(f,p)==w::Reply::invalid);test_thread=worker;runtime::request_stop();runtime::worker_tick();
    C(runtime::retained()&&!runtime::cleanup_ready()&&fake.unpins==0);
    fake.invalidate_fault=false;C(runtime::unregister(result.cookie)==w::Reply::quiescent);runtime::worker_tick();
    C(runtime::cleanup_ready()&&!runtime::retained()&&fake.unpins==1);
}
void unpin_failure(){
    initialize();auto result=attach();auto* original=runtime::current();fake.unpin_ok=false;runtime::request_stop();runtime::worker_tick();
    C(runtime::retained()&&!runtime::cleanup_ready()&&original->pin_retained()&&fake.unpins==1);
    C(runtime::unregister(result.cookie)==w::Reply::quiescent);fake.unpin_ok=true;runtime::worker_tick();
    C(runtime::cleanup_ready()&&!runtime::retained()&&fake.unpins==2&&runtime::current()==original);
    C(runtime::unregister(result.cookie)==w::Reply::quiescent);
}
void concurrent_summary(){
    initialize();auto* original=runtime::current();FILE* status{};C(tmpfile_s(&status)==0&&status);std::atomic<unsigned> mismatches{};
    std::thread observer([&](){for(unsigned n=0;n<100;++n){auto s=runtime::session();if(runtime::current()!=original||s.native_started_qpc!=500||s.game_start!=400)++mismatches;runtime::summary(status);}});
    for(unsigned n=0;n<16;++n){auto r=attach();C(runtime::unregister(r.cookie)==w::Reply::quiescent);runtime::worker_tick();}
    observer.join();fclose(status);C(mismatches==0&&runtime::current()==original);finish();
}
void reentrant_stop(){
    initialize();auto r=attach();auto f=frame(r.cookie);w::Proposal p{};fake.reenter_stop=true;
    C(runtime::current()->prepare(f,p)==w::Reply::invalid&&runtime::retained());test_thread=worker;runtime::worker_tick();
    C(runtime::cleanup_ready()&&!runtime::retained()&&fake.unpins==1);
}
int main(int argc,char** argv){
    const char* mode=argc>1?argv[1]:"disabled";
    if(std::strcmp(mode,"mask")==0){unsigned mask=argc>2?unsigned(std::strtoul(argv[2],nullptr,10)):0;bool expected=false;
#if defined(GEARS_COVER_BODY_PROTOTYPE) && GEARS_COVER_BODY_PROTOTYPE
        expected=mask==7;
#endif
        C(configure(1,mask)==expected&&runtime::enabled==expected);C(!runtime::current());runtime::request_stop();C(runtime::cleanup_ready()&&!runtime::retained());
    }else if(std::strcmp(mode,"disabled")==0){C(configure(0));C(!runtime::enabled&&!runtime::current()&&runtime::cleanup_ready());C(!runtime::initialize(400,500,worker));auto r=registration();w::RegistrationResult result{};C(runtime::register_participant(&r,&result)==w::Reply::unavailable);C(runtime::unregister(1)==w::Reply::wrong_cookie);finish();}
    else if(std::strcmp(mode,"unknown")==0){C(!configure(2));C(!runtime::enabled&&!runtime::current());finish();}
    else if(std::strcmp(mode,"lifecycle")==0)lifecycle();
    else if(std::strcmp(mode,"prepare-gap")==0)prepare_gap();
    else if(std::strcmp(mode,"register-race")==0)register_race();
    else if(std::strcmp(mode,"initialize-race")==0)initialize_race();
    else if(std::strcmp(mode,"fault-input")==0)fault_input();
    else if(std::strcmp(mode,"fault-output")==0)fault_output();
    else if(std::strcmp(mode,"factory-fault")==0)factory_fault();
    else if(std::strcmp(mode,"callback-fault")==0)callback_fault();
    else if(std::strcmp(mode,"unpin-failure")==0)unpin_failure();
    else if(std::strcmp(mode,"summary-race")==0)concurrent_summary();
    else if(std::strcmp(mode,"reentrant-stop")==0)reentrant_stop();
    else if(std::strcmp(mode,"configure-repeat")==0){C(configure());C(configure());C(!configure(1,3));C(runtime::enabled&&!runtime::current());C(configure());C(runtime::initialize(400,500,worker));finish();}
    else if(std::strcmp(mode,"before-init")==0){C(configure());runtime::request_stop();C(!runtime::initialize(400,500,worker)&&!runtime::current()&&runtime::cleanup_ready());}
    else {C(false);}
    std::printf("weapon_hand_runtime %s: %u checks, %u failures\n",mode,checks,failures);return failures?1:0;
}
