// Own-process guard page pauses the REAL Broker's result-copy window after its
// reserved cookie is stored. No Game/XR, production hook, fake Broker state or
// public writer permission is involved. The Native runtime endpoint is a stub.
#define GEARS_RIG_TRACKING_NO_ENTRY 1
#include "rig_tracking_live_test.cpp"
#include <chrono>
#include <condition_variable>
#include <new>

namespace registration_window {
std::mutex barrier_mutex;std::condition_variable barrier;
bool entered{},release{};void* result_page{};
std::atomic<bool> protection_ok{};std::atomic<unsigned> guard_hits{};
void arm_output_guard() noexcept {
    DWORD old{};protection_ok=VirtualProtect(result_page,4096,PAGE_READWRITE|PAGE_GUARD,&old)!=FALSE;
}
LONG CALLBACK guard(EXCEPTION_POINTERS* exception){
    const auto* record=exception->ExceptionRecord;
    if(record->ExceptionCode!=0x80000001u||record->NumberParameters<2||record->ExceptionInformation[0]!=1)return EXCEPTION_CONTINUE_SEARCH;
    const auto address=record->ExceptionInformation[1],begin=reinterpret_cast<ULONG_PTR>(result_page);
    if(!begin||address<begin||address-begin>=4096)return EXCEPTION_CONTINUE_SEARCH;
    ++guard_hits;
    try{
        std::unique_lock<std::mutex> lock(barrier_mutex);entered=true;barrier.notify_all();
        if(!barrier.wait_for(lock,std::chrono::seconds(5),[]{return release;}))return EXCEPTION_CONTINUE_SEARCH;
    }catch(...){return EXCEPTION_CONTINUE_SEARCH;}
    // Continue to the production short-copy SEH bridge. It returns false, and
    // the real registration stays unpublished/quiescent, with a reserved cookie.
    return EXCEPTION_CONTINUE_SEARCH;
}
rig::CoreReadRequest request(){
    rig::CoreReadRequest r{};std::lock_guard<std::mutex> lock(rig::mutex);
    r.native=rig::value.native;r.world=rig::value.world;r.pawn=rig::value.pawn;r.mesh=rig::value.mesh;
    r.world_generation=rig::value.world_generation;r.scene_invocation=rig::value.scene_invocation;r.scene_qpc=rig::value.scene_qpc;
    return r;
}
int run(){
    setup(false,true);LARGE_INTEGER frequency{};C(QueryPerformanceFrequency(&frequency)!=FALSE);qpc_frequency=frequency.QuadPart;
    rig::enabled=true;rig::clock_ns=rig_clock;rig::reference_changed(frozen_reference);selected(.2);
    C(runtime::broker.cookie()==0&&runtime::broker.availability()==w::Availability::absent_quiescent);
    input::controllers.close();input::bridge_open=false;C(publish());
    const auto internal=request();gears_native_rig_tracking::CoreSnapshot first{};C(rig::read_core(internal,first));
    C(pg.prepares==0&&pg.commits==0&&pg.invalidations==0&&!input::bridge_open);
    result_page=VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);C(result_page!=nullptr);if(!result_page)return 1;
    auto* output=::new(result_page) w::RegistrationResult{};
    PVOID handler=AddVectoredExceptionHandler(1,guard);C(handler!=nullptr);if(!handler){VirtualFree(result_page,0,MEM_RELEASE);return 1;}
    pg_pin_hook=arm_output_guard;
    w::Registration registration{};registration.context=&pg;registration.prepare=PG::prepare;registration.commit=PG::commit;registration.invalidate=PG::invalidate;
    std::atomic<w::Reply> registration_reply{w::Reply::busy};
    std::thread pending([&]{registration_reply=runtime::broker.register_participant(&registration,output);});
    bool observed{};{std::unique_lock<std::mutex> lock(barrier_mutex);observed=barrier.wait_for(lock,std::chrono::seconds(5),[]{return entered;});}
    C(observed&&protection_ok&&guard_hits==1);
    if(observed){
        const auto reserved=runtime::broker.cookie();C(reserved!=0);
        C(!runtime::broker.registration_current(reserved,500));
        C(!runtime::broker.registration_current(0,500)&&!runtime::broker.registration_current(reserved,0));
        api::ReadRequest external{};external.registration_cookie=reserved;external.native_started_qpc=500;api::Frame out{};
        C(read(external,out)==api::Reply::unavailable&&out.status==api::Status::unavailable);
        gears_native_rig_tracking::CoreSnapshot during{};C(rig::read_core(internal,during));
        C(!memcmp(&first,&during,sizeof first)&&!input::bridge_open);
    }
    {std::lock_guard<std::mutex> lock(barrier_mutex);release=true;}barrier.notify_all();pending.join();pg_pin_hook=nullptr;
    C(registration_reply==w::Reply::invalid&&guard_hits==1);
    const auto failed_cookie=runtime::broker.cookie();C(failed_cookie&&!runtime::broker.registration_current(failed_cookie,500));
    api::ReadRequest external{};external.registration_cookie=failed_cookie;external.native_started_qpc=500;api::Frame out{};
    C(read(external,out)==api::Reply::unavailable&&out.status==api::Status::unavailable);
    gears_native_rig_tracking::CoreSnapshot after{};C(rig::read_core(internal,after)&&!memcmp(&first,&after,sizeof first));
    as_worker=true;C(runtime::broker.collect_on_owner_worker()==w::Reply::quiescent);as_worker=false;
    C(runtime::broker.cookie()==0&&pg.pins==1&&pg.unpins==1&&pg.violations==0);
    C(rig::read_core(internal,after)&&!memcmp(&first,&after,sizeof first));
    C(RemoveVectoredExceptionHandler(handler)!=0);C(VirtualFree(result_page,0,MEM_RELEASE)!=FALSE);result_page=nullptr;
    rig::stop();finish();C(runtime::detail::calls==0);
    printf("native_rig_registration_window: %u checks, %u failures; real Broker/own-process guard, Native endpoint stub, no Game/XR\n",checks,failures);
    return failures?1:0;
}
}
int main(){return registration_window::run();}
