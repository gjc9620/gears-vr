// One virtual pad inside this verified game process. Uses the ORIGINAL
// XInput/engine bindings; no OS key injection, driver, actor call or VR pose
// writer. Only the read worker opens tracking handles. SDK callbacks borrow
// its bounded private state and never enter the Native hand arbiter.
namespace vr_gamepad_live {
using StateFn=DWORD(WINAPI*)(DWORD,XINPUT_STATE*);
using CapsFn=DWORD(WINAPI*)(DWORD,DWORD,XINPUT_CAPABILITIES*);
using RumbleFn=DWORD(WINAPI*)(DWORD,XINPUT_VIBRATION*);
std::array<StateFn,3> original_state{};
std::array<StateFn,3> original_extended_state{};
std::array<CapsFn,3> original_caps{};
std::array<RumbleFn,3> original_rumble{};
std::array<HMODULE,3> modules{};
std::array<void*,12> targets{};unsigned target_count{};
std::atomic<bool> enabled{},stopping{true},worker_running{},restore_failed{},axes_owner{};
std::atomic<bool> focus_gate_lease{};
std::atomic<unsigned> callbacks{};
std::atomic<std::int64_t> last_sync{};
std::atomic<std::uint64_t> reads{},source_rejected{},game_polls{},other_polls{},caps_polls{};
HANDLE worker{};std::atomic<bool> allow_sim{};bool physical_allowed{};
uintptr_t game_begin{},game_end{};
std::mutex route_mutex;gears_vr_gamepad::Route route;
// A veto only. The sole game-thread equipment owner must restore its old
// stow journal before the original game receives a face-button action.
bool stow_fence{}; // route_mutex, including arm vs original game sampling.
constexpr std::uint16_t stow_action_mask=gears_vr_gamepad::equipment_restore_buttons;
DWORD original_action_pending() noexcept {
    std::lock_guard<std::mutex> lock(route_mutex);
    return enabled.load()?route.pending_buttons(gears_vr::monotonic_ns())&stow_action_mask:0;
}
DWORD set_stow_fence(void* argument) noexcept {
    const auto value=reinterpret_cast<uintptr_t>(argument);if(value>1)return ERROR_INVALID_PARAMETER;
    std::lock_guard<std::mutex> lock(route_mutex);
    if(value){
        if(!enabled.load()||stopping.load())return ERROR_INVALID_STATE;
        if(!stow_fence&&(route.pending_buttons(gears_vr::monotonic_ns())&stow_action_mask))return ERROR_BUSY;
    }
    stow_fence=value!=0;return ERROR_SUCCESS;
}
std::mutex lifecycle_mutex;
struct Call {Call(){++callbacks;}~Call(){--callbacks;}};
bool game_caller(void* address) noexcept {const auto p=reinterpret_cast<uintptr_t>(address);return p>=game_begin&&p<game_end;}
DWORD state(unsigned provider,DWORD index,XINPUT_STATE* out,void* caller,bool extended=false) noexcept {
    Call call;
    const auto original=extended?original_extended_state[provider]:original_state[provider];
    if(!enabled.load(std::memory_order_acquire))return original?original(index,out):ERROR_DEVICE_NOT_CONNECTED;
    if(!out)return ERROR_BAD_ARGUMENTS;
    *out={};if(index)return ERROR_DEVICE_NOT_CONNECTED;
    const bool game=game_caller(caller);if(game)++game_polls;else ++other_polls;
    std::lock_guard<std::mutex> lock(route_mutex);
    std::uint32_t packet{};const auto p=route.sample(gears_vr::monotonic_ns(),game,packet,stow_fence?stow_action_mask:0);
    out->dwPacketNumber=packet;out->Gamepad.wButtons=p.buttons;
    out->Gamepad.sThumbLX=p.lx;out->Gamepad.sThumbLY=p.ly;out->Gamepad.sThumbRX=p.rx;out->Gamepad.sThumbRY=p.ry;
    out->Gamepad.bLeftTrigger=p.lt;out->Gamepad.bRightTrigger=p.rt;
    // A display-mode hint is NOT action authority. LT/RT/grips stay with the
    // VR owners, including when a menu lease changes/expires on resume.
    return ERROR_SUCCESS;
}
DWORD capabilities(unsigned provider,DWORD index,DWORD flags,XINPUT_CAPABILITIES* out) noexcept {
    Call call;
    if(!enabled.load(std::memory_order_acquire))return original_caps[provider]?original_caps[provider](index,flags,out):ERROR_DEVICE_NOT_CONNECTED;
    if(!out)return ERROR_BAD_ARGUMENTS;*out={};if(index)return ERROR_DEVICE_NOT_CONNECTED;
    ++caps_polls;out->Type=XINPUT_DEVTYPE_GAMEPAD;out->SubType=XINPUT_DEVSUBTYPE_GAMEPAD;
    out->Gamepad.wButtons=gears_vr_gamepad::a|gears_vr_gamepad::b|gears_vr_gamepad::x|gears_vr_gamepad::y|
        gears_vr_gamepad::start|gears_vr_gamepad::left_thumb|gears_vr_gamepad::right_thumb|
        gears_vr_gamepad::up|gears_vr_gamepad::down|gears_vr_gamepad::left|gears_vr_gamepad::right;
    out->Gamepad.sThumbLX=out->Gamepad.sThumbLY=out->Gamepad.sThumbRX=out->Gamepad.sThumbRY=32767;
    return ERROR_SUCCESS; // Logical VR pad, not physical controller identity.
}
DWORD rumble(unsigned provider,DWORD index,XINPUT_VIBRATION* value) noexcept {
    Call call;
    if(!enabled.load(std::memory_order_acquire))return original_rumble[provider]?original_rumble[provider](index,value):ERROR_DEVICE_NOT_CONNECTED;
    if(!value)return ERROR_BAD_ARGUMENTS;
    // Do not send this virtual device's vibration to an unrelated physical pad.
    return index?ERROR_DEVICE_NOT_CONNECTED:ERROR_SUCCESS;
}
#define VR_PAD_WRAPPERS(i) \
DWORD WINAPI state_##i(DWORD n,XINPUT_STATE* s){return state(i,n,s,_ReturnAddress());} \
DWORD WINAPI extended_state_##i(DWORD n,XINPUT_STATE* s){return state(i,n,s,_ReturnAddress(),true);} \
DWORD WINAPI caps_##i(DWORD n,DWORD f,XINPUT_CAPABILITIES* c){return capabilities(i,n,f,c);} \
DWORD WINAPI rumble_##i(DWORD n,XINPUT_VIBRATION* v){return rumble(i,n,v);}
VR_PAD_WRAPPERS(0) VR_PAD_WRAPPERS(1) VR_PAD_WRAPPERS(2)
#undef VR_PAD_WRAPPERS
DWORD WINAPI read_worker(void*) noexcept {
    gears_current_tracking::Reader reader;
    try{
        while(!stopping.load(std::memory_order_acquire)){
            gears_vr::StereoSample stereo{};gears_vr::ControllerInputFrame input{};
            gears_hand_continuity::TrackedInput continuity{};gears_hand_continuity::Publisher publisher{};
            const auto result=reader.read(GetCurrentProcessId(),[](){return gears_vr::monotonic_ns();},allow_sim,
                stereo,input,continuity,publisher);
            {
                std::lock_guard<std::mutex> lock(route_mutex);
                if(result==gears_current_tracking::Failure::none&&route.observe(input,continuity,publisher,
                    gears_vr::monotonic_ns(),physical_allowed,allow_sim)){
                    ++reads;last_sync.store(input.sync_ns,std::memory_order_release);axes_owner.store(route.axes_owned(),std::memory_order_release);
                }else{++source_rejected;route.invalidate();last_sync=0;axes_owner=false;}
            }
            Sleep(8); // Outside every mapping/private-state lock; no stale retry.
        }
    }catch(...){std::lock_guard<std::mutex> lock(route_mutex);route.invalidate();last_sync=0;axes_owner=false;}
    reader.close_after_drain();worker_running.store(false,std::memory_order_release);return 0;
}
void request_stop() noexcept {
    std::lock_guard<std::mutex> lifecycle(lifecycle_mutex);
    stopping.store(true,std::memory_order_release);axes_owner=false;last_sync=0;
    {std::lock_guard<std::mutex> lock(route_mutex);route.invalidate();}
    // If the launcher dies while its controller-only focus gate is open,
    // keep this virtual-only neutral pad installed. Never expose a physical
    // background pad. Normal cleanup restores the certified gate FIRST.
    if(focus_gate_lease.load(std::memory_order_acquire))return;
    enabled.store(false,std::memory_order_release);
    for(unsigned i=0;i<target_count;++i){const auto status=MH_DisableHook(targets[i]);
        if(status!=MH_OK&&status!=MH_ERROR_DISABLED&&status!=MH_ERROR_NOT_CREATED&&status!=MH_ERROR_NOT_INITIALIZED)restore_failed=true;}
}
unsigned pending_count() noexcept {return callbacks.load()+unsigned(worker_running.load())+unsigned(restore_failed.load())+unsigned(focus_gate_lease.load());}
bool gate_lease(bool acquire) noexcept {
    std::unique_lock<std::mutex> lifecycle(lifecycle_mutex);
    if(acquire){
        if(!enabled.load()||stopping.load())return false;
        focus_gate_lease.store(true,std::memory_order_release);return true;
    }
    unsigned char first{};std::array<unsigned char,6> second{};
    const std::array<unsigned char,6> original{{0x0f,0x85,0x21,0x0d,0,0}};
    if(!safe_copy(&first,reinterpret_cast<void const*>(game_begin+0x19afcd),1)||first!=0x75||
       !safe_copy(second.data(),reinterpret_cast<void const*>(game_begin+0x19aff0),second.size())||second!=original)return false;
    focus_gate_lease.store(false,std::memory_order_release);
    lifecycle.unlock();
    if(stopping.load())request_stop();return true;
}
void close() noexcept {
    request_stop();
    if(worker&&WaitForSingleObject(worker,1000)!=WAIT_OBJECT_0)return;
    const auto until=GetTickCount64()+1000;
    while(callbacks.load()&&GetTickCount64()<until)Sleep(1);
    if(pending_count())return;
    for(unsigned i=0;i<target_count;++i){const auto status=MH_RemoveHook(targets[i]);
        if(status!=MH_OK&&status!=MH_ERROR_NOT_CREATED&&status!=MH_ERROR_NOT_INITIALIZED){restore_failed=true;return;}}
    target_count=0;if(worker){CloseHandle(worker);worker=nullptr;}
    for(auto& module:modules)if(module){FreeLibrary(module);module=nullptr;}
    original_state={};original_extended_state={};original_caps={};original_rumble={};
}
bool install(void* target,void* detour,void** original,FILE* log) noexcept {
    if(!target)return false;
    for(unsigned i=0;i<target_count;++i)if(targets[i]==target)return true; // System forwarder already owned.
    const auto result=MH_CreateHook(target,detour,original);
    if(result!=MH_OK){if(log)fprintf(log,"vr_gamepad_hook_rejected=%s\n",MH_StatusToString(result));return false;}
    targets[target_count++]=target;return true;
}
bool initialize(bool required,bool simulated,FILE* log) noexcept {
    if(!required)return true;
    if(enabled.load(std::memory_order_acquire)){
        // Owned background Source1 may be admitted after the dormant virtual
        // device was enumerated during priming. Never admit physical input.
        if(simulated&&!physical_allowed)allow_sim.store(true,std::memory_order_release);
        return !stopping.load()&&allow_sim.load()==simulated;
    }
    allow_sim=simulated;
    wchar_t test[8]{};const auto test_size=GetEnvironmentVariableW(L"GEARSVR_TEST_WINDOWS_BEHIND",test,8);
    // In an owner-started session, XR FOCUSED is the VR application's input
    // focus. A desktop window need not be clicked. Autonomous sessions admit
    // only their explicitly authorized Source1, never physical background input.
    physical_allowed=!simulated&&!(test_size==1&&test[0]==L'1');
    game_begin=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    const auto dos=reinterpret_cast<IMAGE_DOS_HEADER const*>(game_begin);
    const auto nt=reinterpret_cast<IMAGE_NT_HEADERS64 const*>(game_begin+dos->e_lfanew);
    if(dos->e_magic!=IMAGE_DOS_SIGNATURE||nt->Signature!=IMAGE_NT_SIGNATURE||!nt->OptionalHeader.SizeOfImage)return false;
    game_end=game_begin+nt->OptionalHeader.SizeOfImage;
    const StateFn states[]={state_0,state_1,state_2};const CapsFn caps[]={caps_0,caps_1,caps_2};
    const StateFn extended[]={extended_state_0,extended_state_1,extended_state_2};
    const RumbleFn rumbles[]={rumble_0,rumble_1,rumble_2};
    const wchar_t* names[]={L"xinput1_4.dll",L"xinput1_3.dll",L"xinput9_1_0.dll"};
    for(unsigned i=0;i<modules.size();++i){
        modules[i]=LoadLibraryExW(names[i],nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);if(!modules[i])continue;
        const auto get_extended=GetProcAddress(modules[i],MAKEINTRESOURCEA(100));
        if(get_extended&&!install(reinterpret_cast<void*>(get_extended),reinterpret_cast<void*>(extended[i]),reinterpret_cast<void**>(&original_extended_state[i]),log)){
            request_stop();close();return false;
        }
        if(!install(reinterpret_cast<void*>(GetProcAddress(modules[i],"XInputGetState")),reinterpret_cast<void*>(states[i]),reinterpret_cast<void**>(&original_state[i]),log)||
           !install(reinterpret_cast<void*>(GetProcAddress(modules[i],"XInputGetCapabilities")),reinterpret_cast<void*>(caps[i]),reinterpret_cast<void**>(&original_caps[i]),log)||
           !install(reinterpret_cast<void*>(GetProcAddress(modules[i],"XInputSetState")),reinterpret_cast<void*>(rumbles[i]),reinterpret_cast<void**>(&original_rumble[i]),log)){
            request_stop();close();return false;
        }
    }
    if(!target_count)return false;
    stopping=false;worker_running=true;enabled=true;
    worker=CreateThread(nullptr,0,read_worker,nullptr,0,nullptr);
    if(!worker){worker_running=false;request_stop();close();return false;}
    for(unsigned i=0;i<target_count;++i)if(MH_EnableHook(targets[i])!=MH_OK){request_stop();return false;}
    if(log)fprintf(log,"vr_native_gamepad=1 hooks=%u input_protocol=%u continuity_protocol=%u physical_allowed=%u keyboard_injection=0\n",
        target_count,gears_vr::ControllerInputPacket::version_value,gears_hand_continuity::ContinuityPacket::version_value,unsigned(physical_allowed));
    return true;
}
bool owns_axes() noexcept {
    const auto stamp=last_sync.load(std::memory_order_acquire),now=gears_vr::monotonic_ns();
    return enabled.load()&&axes_owner.load()&&stamp>0&&now>=stamp&&now-stamp<=100000000;
}
void print(FILE* f) noexcept {fprintf(f,"vr_native_gamepad_enabled=%u vr_native_gamepad_source_reads=%llu vr_native_gamepad_source_rejected=%llu vr_native_gamepad_game_polls=%llu vr_native_gamepad_other_polls=%llu vr_native_gamepad_caps=%llu vr_native_gamepad_pending=%u vr_native_gamepad_restore_failed=%u ",
    unsigned(enabled.load()),reads.load(),source_rejected.load(),game_polls.load(),other_polls.load(),caps_polls.load(),pending_count(),unsigned(restore_failed.load()));}
}
