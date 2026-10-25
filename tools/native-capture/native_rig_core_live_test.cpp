// Actual candidate Core/public provider and actual Shared/Broker/bridge fixture.
// Actor sampling, PG callbacks/module pins, Scene admission and clock are host
// seams. No Native writer, node invocation, Game, XR or permission is proved.
#define GEARS_RIG_TRACKING_NO_ENTRY 1
#include "rig_tracking_live_test.cpp"
#include <condition_variable>
#include <chrono>
#include <type_traits>

namespace {
namespace nr=gears_native_rig_tracking;
using Core=nr::CoreSnapshot;
std::atomic<int64_t> controlled_clock{};
struct ClockGate {
    std::mutex mutex;std::condition_variable cv;unsigned hit{};
    bool arrived{},released{};std::atomic<bool> timed_out{};
    bool await(){std::unique_lock<std::mutex> lock(mutex);return cv.wait_for(lock,std::chrono::seconds(5),[&]{return arrived;});}
    void resume(){std::lock_guard<std::mutex> lock(mutex);released=true;cv.notify_all();}
    void pause(){std::unique_lock<std::mutex> lock(mutex);arrived=true;cv.notify_all();
        if(!cv.wait_for(lock,std::chrono::seconds(5),[&]{return released;}))timed_out=true;}
};
ClockGate* active_gate{}; // Set before worker creation; clear only after join.
thread_local bool gated_reader{};
thread_local unsigned reader_clock_calls{};
int64_t atomic_clock(){
    if(gated_reader&&active_gate&&++reader_clock_calls==active_gate->hit)active_gate->pause();
    return controlled_clock.load();
}
bool empty(Core const& f){return f.status==api::Status::unavailable&&!f.native.game_pid&&!f.publication_sequence&&!f.scene_invocation&&!f.source_ticket&&!f.mesh.address;}
bool empty(api::Frame const& f){return f.status==api::Status::unavailable&&!f.native.game_pid&&!f.publication_sequence&&!f.mesh.address;}
bool same_bytes(Core const& a,Core const& b){return std::memcmp(&a,&b,sizeof(a))==0;}
unsigned pg_calls(){return pg.prepares+pg.commits+pg.invalidations;}
unsigned long long core_reads(){std::lock_guard<std::mutex> lock(rig::mutex);return rig::core_reads;}
void select_new(){selected(.2);controlled_clock=fake_clock;}
void start_case(bool simulated=true,bool grip=true){
    setup(false,simulated);LARGE_INTEGER frequency{};C(QueryPerformanceFrequency(&frequency)!=0);qpc_frequency=frequency.QuadPart;
    rig::enabled=true;rig::clock_ns=atomic_clock;rig::reference_changed(frozen_reference);shared::continuity_enabled=true;
    if(!grip)for(auto& hand:hands.hands){hand.grip_active=0;hand.grip_flags=0;}
    C(runtime::broker.cookie()==0);select_new();C(publish());
}
rig::CoreReadRequest request(){
    rig::CoreReadRequest r{};r.native={GetCurrentProcessId(),scene.thread,400,500};
    r.world=rig::object(native::actor.world);r.pawn=rig::object(native::actor.pawn);r.mesh=rig::object(native::actor.mesh);
    r.world_generation=native::actor.world_generation;r.scene_invocation=scene.invocation;r.scene_qpc=scene.scene_qpc;return r;
}
Core get(rig::CoreReadRequest const& r){Core out{};const auto pg_before=pg_calls();C(rig::read_core(r,out));C(pg_calls()==pg_before);return out;}
void rejected(rig::CoreReadRequest const& r,Core const& initial){Core out=initial;const auto pg_before=pg_calls();C(!rig::read_core(r,out)&&empty(out));C(pg_calls()==pg_before);}
api::ReadRequest public_request(uint64_t registration){api::ReadRequest r{};r.registration_cookie=registration;r.native_started_qpc=500;return r;}
void no_input_reader(){input::controllers.close();input::bridge_open=false;C(!input::bridge_open);}
void detach(){as_worker=true;C(runtime::broker.unregister(cookie)==w::Reply::quiescent);C(runtime::broker.collect_on_owner_worker()==w::Reply::quiescent);as_worker=false;}
void cleanup(){rig::stop();C(runtime::detail::calls.load()==0);finish();}

void unregistered_case(bool simulated,bool grip){
    start_case(simulated,grip);const auto r=request();const auto first=get(r);C(pg_calls()==0&&pg.pins==0&&runtime::broker.cookie()==0);
    C(first.native.native_started_qpc==500&&first.native.game_thread==GetCurrentThreadId());
    C((first.flags&api::producer_continuity_available)==0);
    C(grip?(first.world_pose_mask==7):(first.world_pose_mask==api::head_pose));
    no_input_reader();producer.close();const auto sampled=native::sample_calls;
    for(unsigned i=0;i<4;++i)C(same_bytes(get(r),first));
    C(native::sample_calls==sampled&&!input::bridge_open&&pg_calls()==0);
    C(publish());C(same_bytes(get(r),first)&&!input::bridge_open); // exact same-Scene duplicate
    api::Frame out{};auto v1=public_request(0);C(read(v1,out)==api::Reply::invalid&&empty(out));
    v1.registration_cookie=0xdead;C(read(v1,out)==api::Reply::wrong_cookie&&empty(out));
    cleanup();C(pg_calls()==0&&pg.pins==0&&pg.unpins==0);
}
void registration_case(){
    start_case();const auto r=request();const auto first=get(r);no_input_reader();attach();const auto old=cookie;
    auto v1=public_request(cookie);api::Frame out{},expected{};
    const api::NativeKey authenticated{first.native.game_pid,first.native.game_thread,first.native.game_start,first.native.native_started_qpc,cookie};
    C(nr::project_for_authenticated_registration(first,authenticated,controlled_clock.load(),50000000,expected,true));
    C(read(v1,out)==api::Reply::accepted&&std::memcmp(&out,&expected,sizeof(out))==0);
    v1.registration_cookie++;C(read(v1,out)==api::Reply::wrong_cookie&&empty(out));v1.registration_cookie=old;
    as_worker=true;C(runtime::broker.unregister(old)==w::Reply::quiescent);as_worker=false;
    C(same_bytes(get(r),first));C(read(v1,out)!=api::Reply::accepted&&empty(out));
    as_worker=true;C(runtime::broker.collect_on_owner_worker()==w::Reply::quiescent);as_worker=false;
    C(runtime::broker.cookie()==0&&same_bytes(get(r),first));C(read(v1,out)==api::Reply::wrong_cookie&&empty(out));
    attach();C(cookie!=old&&same_bytes(get(r),first));C(read(v1,out)==api::Reply::wrong_cookie&&empty(out));
    v1.registration_cookie=cookie;C(read(v1,out)==api::Reply::accepted&&out.native.registration_cookie==cookie);
    C(out.publication_sequence==first.publication_sequence&&out.published_ns==first.published_ns&&out.scene_invocation==first.scene_invocation);
    C(!input::bridge_open&&pg.prepares==0&&pg.commits==0);cleanup();
}
void request_case(){
    start_case();const auto good=request();const auto first=get(good);
    for(unsigned field=0;field<12;++field){auto r=good;
        if(field==0)++r.native.game_pid;if(field==1)++r.native.game_thread;if(field==2)++r.native.game_start;if(field==3)++r.native.native_started_qpc;
        if(field==4)++r.world.registry_index;if(field==5)++r.pawn.name_number;if(field==6)++r.mesh.klass;
        if(field==7)++r.world_generation;if(field==8)++r.scene_invocation;if(field==9)++r.scene_qpc;
        if(field==10)r.minimum_publication_sequence=first.publication_sequence+1;if(field==11)r.maximum_source_age_ns=100000001;
        rejected(r,first);}
    auto r=good;r.maximum_source_age_ns=0;rejected(r,first);r=good;r.native={};rejected(r,first);
    C(same_bytes(get(good),first));cleanup();
}
void invalidation_case(char const* mode){
    start_case();const auto r=request();const auto first=get(r);
    if(!strcmp(mode,"ticket"))shared::clear_tracking();
    else if(!strcmp(mode,"reference"))rig::reference_changed({2,false,true,false});
    else if(!strcmp(mode,"world"))committed_world_generation=2;
    else if(!strcmp(mode,"recording"))recording=false;
    else if(!strcmp(mode,"fault"))runtime::detail::unknown_fault=true;
    else if(!strcmp(mode,"admission"))runtime::detail::admission_closed=true;
    else if(!strcmp(mode,"runtime-stop"))runtime::detail::stop_requested=true;
    else if(!strcmp(mode,"loss")){stereo.sample.focused=0;C(!publish());const auto observed=rig::observed.stream;C(observed>first.observed.stream);C(!publish()&&rig::observed.stream==observed);}
    else C(false);
    rejected(r,first);
    if(!strcmp(mode,"reference")){rig::reference_changed({2,true,false,false});rejected(r,first);}
    cleanup();
}
void age_case(){
    start_case();auto r=request();const auto first=get(r);
    controlled_clock=first.stereo.received_ns+50000001;rejected(r,first);
    r.maximum_source_age_ns=100000000;C(same_bytes(get(r),first));
    controlled_clock=first.stereo.received_ns+100000001;rejected(r,first);cleanup();
}
void tick_case(char const* mode){
    static_assert(!std::is_copy_constructible_v<rig::TickLatch>&&!std::is_copy_assignable_v<rig::TickLatch>);
    start_case();const auto r=request();const auto first=get(r);no_input_reader();const rig::TickKey key{r,7001};rig::TickLatch latch;
    const auto before=core_reads();C(rig::begin_tick(key,latch)&&core_reads()==before+1);
    for(unsigned i=0;i<4;++i){Core out{};C(rig::read_tick(key,latch,out)&&same_bytes(out,first));}
    C(core_reads()==before+1&&!input::bridge_open);
    if(!strcmp(mode,"tick-thread")){
        std::atomic<unsigned> faults{};
        std::thread worker([&]{Core out{};if(!rig::read_core(r,out)||!same_bytes(out,first))++faults;
            if(rig::begin_tick(key,latch))++faults;out=first;if(rig::read_tick(key,latch,out)||!empty(out))++faults;if(rig::end_tick(key,latch))++faults;});
        worker.join();C(faults.load()==0);Core out{};C(rig::read_tick(key,latch,out)&&same_bytes(out,first));
    }else if(!strcmp(mode,"tick-key")){
        auto wrong=key;wrong.outer_tick++;Core out=first;C(!rig::read_tick(wrong,latch,out)&&empty(out));
        out=first;C(!rig::read_tick(key,latch,out)&&empty(out));C(!rig::end_tick(wrong,latch));C(!rig::begin_tick(key,latch));
    }else if(!strcmp(mode,"tick-duplicate")){
        C(!rig::begin_tick(key,latch));Core out=first;C(!rig::read_tick(key,latch,out)&&empty(out));
    }else if(!strcmp(mode,"tick-scene")){
        select_new();C(publish());Core out=first;C(!rig::read_tick(key,latch,out)&&empty(out));
        const rig::TickKey fresh{request(),7002};out=first;C(!rig::read_tick(fresh,latch,out)&&empty(out));
        C(!rig::begin_tick(fresh,latch));C(rig::end_tick(key,latch));C(rig::begin_tick(fresh,latch));
        C(rig::read_tick(fresh,latch,out)&&out.scene_invocation==fresh.request.scene_invocation);C(rig::end_tick(fresh,latch));cleanup();return;
    }else if(!strcmp(mode,"tick-stop")){
        rig::stop();Core out=first;C(!rig::read_tick(key,latch,out)&&empty(out));C(!rig::begin_tick(key,latch));
    }else if(strcmp(mode,"tick"))C(false);
    C(rig::end_tick(key,latch));Core out=first;C(!rig::read_tick(key,latch,out)&&empty(out));
    if(!strcmp(mode,"tick-stop")){rig::TickLatch fresh;C(!rig::begin_tick(key,fresh));}
    cleanup();
}
void barrier_reregister(bool public_getter){
    start_case();const auto r=request();const auto first=get(r);attach();const auto old=cookie;const auto v1=public_request(old);
    ClockGate gate;gate.hit=public_getter?5u:4u;active_gate=&gate;
    std::atomic<unsigned> faults{};std::atomic<bool> accepted{};
    std::thread worker([&]{gated_reader=true;reader_clock_calls=0;
        if(public_getter){api::Frame out{};const auto reply=rig::read(&v1,&out);accepted=reply==api::Reply::accepted;if(accepted.load()||!empty(out))++faults;}
        else{Core out{};accepted=rig::read_core(r,out);if(!accepted.load()||!same_bytes(out,first))++faults;}
        gated_reader=false;});
    const bool arrived=gate.await();C(arrived);
    if(arrived){detach();attach();C(cookie!=old);}
    gate.resume();worker.join();active_gate=nullptr;
    C(!gate.timed_out.load()&&faults.load()==0);C(accepted.load()==!public_getter);C(same_bytes(get(r),first));
    api::Frame out{};C(read(v1,out)==api::Reply::wrong_cookie&&empty(out));const auto fresh=public_request(cookie);C(read(fresh,out)==api::Reply::accepted);
    cleanup();
}
void barrier_core_invalidation(bool age_boundary){
    start_case();const auto r=request();const auto first=get(r);
    ClockGate gate;gate.hit=age_boundary?4u:3u;active_gate=&gate;std::atomic<unsigned> faults{};
    std::thread worker([&]{gated_reader=true;reader_clock_calls=0;Core out=first;
        if(rig::read_core(r,out)||!empty(out))++faults;gated_reader=false;});
    const bool arrived=gate.await();C(arrived);
    if(arrived){if(age_boundary)controlled_clock=first.stereo.received_ns+50000001;else rig::stop();}
    gate.resume();worker.join();active_gate=nullptr;C(!gate.timed_out.load()&&faults.load()==0);
    rejected(r,first);cleanup();
}
void race_publish_stop(){
    start_case();std::mutex requests_mutex;auto latest=request();std::atomic<bool> started{},done{};std::atomic<unsigned> faults{},accepted{},attempts{};
    std::thread worker([&]{started=true;for(unsigned i=0;i<1024&&!done.load();++i){rig::CoreReadRequest r{};{std::lock_guard<std::mutex> lock(requests_mutex);r=latest;}
        Core out{};const bool okay=rig::read_core(r,out);++attempts;
        if(okay){++accepted;if(out.scene_invocation!=r.scene_invocation||out.scene_qpc!=r.scene_qpc||!nr::same(out.native,r.native)||
                !nr::core_snapshot_valid(out,static_cast<int64_t>(out.published_ns),100000000,true))++faults;}
        else if(!empty(out))++faults;std::this_thread::yield();}});
    const auto until=std::chrono::steady_clock::now()+std::chrono::seconds(5);
    while(!started.load()&&std::chrono::steady_clock::now()<until)std::this_thread::yield();C(started.load());
    for(unsigned i=0;i<8;++i){select_new();C(publish());const auto r=request();{std::lock_guard<std::mutex> lock(requests_mutex);latest=r;}}
    rig::stop();done=true;worker.join();C(!faults.load()&&attempts.load()>0);Core out{};C(!rig::read_core(latest,out)&&empty(out));C(!publish());
    std::printf("race_publish_stop attempts=%u accepted=%u\n",attempts.load(),accepted.load());cleanup();
}
void race_registration_readers(){
    start_case();const auto r=request();const auto first=get(r);attach();std::atomic<uint64_t> live_cookie{cookie};
    std::atomic<bool> done{};std::atomic<unsigned> faults{},started{},core_ok{},public_attempts{};
    std::thread native_reader([&]{for(unsigned i=0;i<1024&&!done.load();++i){Core out{};
        if(!rig::read_core(r,out)||!same_bytes(out,first))++faults;else ++core_ok;if(i==0)++started;std::this_thread::yield();}});
    std::thread public_reader([&]{for(unsigned i=0;i<1024&&!done.load();++i){const auto req=public_request(live_cookie.load());api::Frame out{};
        const auto reply=rig::read(&req,&out);++public_attempts;
        if(reply==api::Reply::accepted){if(out.native.registration_cookie!=req.registration_cookie||out.publication_sequence!=first.publication_sequence||out.published_ns!=first.published_ns)++faults;}
        else if(!empty(out))++faults;if(i==0)++started;std::this_thread::yield();}});
    const auto until=std::chrono::steady_clock::now()+std::chrono::seconds(5);
    while(started.load()!=2&&std::chrono::steady_clock::now()<until)std::this_thread::yield();C(started.load()==2);
    for(unsigned i=0;i<8;++i){live_cookie=0;detach();attach();live_cookie=cookie;}
    done=true;native_reader.join();public_reader.join();C(!faults.load()&&core_ok.load()>0&&public_attempts.load()>0);
    C(same_bytes(get(r),first)&&pg.prepares==0&&pg.commits==0);std::printf("race_registration core=%u public=%u\n",core_ok.load(),public_attempts.load());cleanup();
}
int native_case(char const* mode){
    if(!strcmp(mode,"unregistered-runtime"))unregistered_case(false,true);
    else if(!strcmp(mode,"unregistered-sim"))unregistered_case(true,true);
    else if(!strcmp(mode,"head-only"))unregistered_case(true,false);
    else if(!strcmp(mode,"registration"))registration_case();
    else if(!strcmp(mode,"request"))request_case();
    else if(!strcmp(mode,"age"))age_case();
    else if(!strcmp(mode,"race-core-reregister"))barrier_reregister(false);
    else if(!strcmp(mode,"race-public-reregister"))barrier_reregister(true);
    else if(!strcmp(mode,"race-core-stop"))barrier_core_invalidation(false);
    else if(!strcmp(mode,"race-core-age"))barrier_core_invalidation(true);
    else if(!strcmp(mode,"race-publish-stop"))race_publish_stop();
    else if(!strcmp(mode,"race-registration"))race_registration_readers();
    else if(!strcmp(mode,"tick")||!strncmp(mode,"tick-",5))tick_case(mode);
    else invalidation_case(mode);
    std::printf("native_rig_core_live %s: %u checks, %u failures; actual candidate/Shared/Broker/bridge CPU only, no node/writer permission\n",mode,checks,failures);
    return failures?1:0;
}
} // namespace
int main(int argc,char** argv){return native_case(argc>1?argv[1]:"unregistered-sim");}
