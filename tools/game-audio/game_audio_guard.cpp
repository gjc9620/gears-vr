#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <mmdeviceapi.h>
#include <audiopolicy.h>
#include <wrl/client.h>
#include "audio_guard_policy.h"
#include <cstdio>
#include <map>
#include <stdexcept>
using Microsoft::WRL::ComPtr;
using namespace gears_test_audio;
struct Handle {
    HANDLE value{};
    ~Handle() { if (value) CloseHandle(value); }
};
static std::uint64_t creation(HANDLE process) {
    FILETIME c{},e{},k{},u{};
    if (!GetProcessTimes(process,&c,&e,&k,&u)) return 0;
    return (std::uint64_t(c.dwHighDateTime)<<32)|c.dwLowDateTime;
}
static bool alive(HANDLE process) { return WaitForSingleObject(process,0)==WAIT_TIMEOUT; }
struct AudioSession final : Session {
    std::wstring id;
    std::wstring instance;
    DWORD pid{};
    ComPtr<IAudioSessionControl2> control;
    ComPtr<ISimpleAudioVolume> volume;
    std::wstring key() const override { return id; }
    bool mute(bool& value) override {
        BOOL state{}; if (FAILED(volume->GetMute(&state))) return false;
        value=state!=FALSE; return true;
    }
    bool same_session() {
        DWORD current{};
        if (control->GetProcessId(&current)!=S_OK || current!=pid) return false;
        LPWSTR raw{};
        if (FAILED(control->GetSessionInstanceIdentifier(&raw))) return false;
        bool same=raw && instance==raw; CoTaskMemFree(raw);
        return same;
    }
    bool set_mute(bool value) override {
        return same_session() && SUCCEEDED(volume->SetMute(value?TRUE:FALSE,nullptr));
    }
    bool set_volume(float value) override {
        return same_session() && SUCCEEDED(volume->SetMasterVolume(value,nullptr));
    }
};
struct ScanCounts { unsigned endpoints{}, sessions{}, unavailable{}, rejected_shared{}; };
static ScanCounts scan(IMMDeviceEnumerator* enumerator, DWORD target, HANDLE process, Policy* policy, ManualPolicy* manual=nullptr) {
    ScanCounts result; ComPtr<IMMDeviceCollection> devices;
    if (FAILED(enumerator->EnumAudioEndpoints(eRender,DEVICE_STATE_ACTIVE,&devices))) {
        ++result.unavailable; return result;
    }
    UINT count{}; if (FAILED(devices->GetCount(&count))) { ++result.unavailable; return result; }
    result.endpoints=count;
    for (UINT i=0;i<count;++i) {
        ComPtr<IMMDevice> device; ComPtr<IAudioSessionManager2> manager; ComPtr<IAudioSessionEnumerator> sessions;
        if (FAILED(devices->Item(i,&device)) ||
            FAILED(device->Activate(__uuidof(IAudioSessionManager2),CLSCTX_ALL,nullptr,&manager)) ||
            FAILED(manager->GetSessionEnumerator(&sessions))) { ++result.unavailable; continue; }
        LPWSTR raw_device{};
        if (FAILED(device->GetId(&raw_device))) { ++result.unavailable; continue; }
        std::wstring device_id(raw_device); CoTaskMemFree(raw_device);
        int total{}; if (FAILED(sessions->GetCount(&total))) { ++result.unavailable; continue; }
        for (int j=0;j<total;++j) {
            ComPtr<IAudioSessionControl> control; ComPtr<IAudioSessionControl2> control2;
            if (FAILED(sessions->GetSession(j,&control)) || FAILED(control.As(&control2))) { ++result.unavailable; continue; }
            ++result.sessions;
            if (!policy && !manual) continue; // Read-only enumeration never queries/modifies volume.
            DWORD pid{}; HRESULT owner=control2->GetProcessId(&pid);
            if (owner!=S_OK) { if (SUCCEEDED(owner)) ++result.rejected_shared; continue; }
            if (pid!=target || !alive(process) || control2->IsSystemSoundsSession()!=S_FALSE) continue;
            AudioSessionState state{};
            if (FAILED(control2->GetState(&state)) || state==AudioSessionStateExpired) continue;
            LPWSTR raw_instance{};
            if (FAILED(control2->GetSessionInstanceIdentifier(&raw_instance))) { ++result.unavailable; continue; }
            auto session=std::make_unique<AudioSession>(); session->instance=raw_instance;
            session->id=device_id+L"\n"+raw_instance; session->pid=pid; session->control=control2;
            CoTaskMemFree(raw_instance);
            if (FAILED(control.As(&session->volume))) { ++result.unavailable; continue; }
            if (manual) manual->observe(std::move(session),alive(process));
            else policy->observe(std::move(session),alive(process));
        }
    }
    return result;
}
struct RestoreFallback {
    Policy& policy;
    bool armed{true};
    void finish() noexcept {
        auto deadline=GetTickCount64()+3000;
        while (!policy.restore() && GetTickCount64()<deadline) Sleep(50);
        armed=false;
    }
    ~RestoreFallback() { if (armed) finish(); }
};
static std::uint64_t number(std::wstring const& text) {
    if (text.empty() || text.find_first_not_of(L"0123456789")!=std::wstring::npos) throw std::runtime_error("number");
    std::size_t end{}; auto value=std::stoull(text,&end); if (end!=text.size() || !value) throw std::runtime_error("number");
    return value;
}
int wmain(int argc,wchar_t** argv) {
    HRESULT initialized=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
    if (FAILED(initialized)) return 2;
    struct Uninit { ~Uninit() { CoUninitialize(); } } uninit;
    try {
        ComPtr<IMMDeviceEnumerator> enumerator;
        if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator),nullptr,CLSCTX_ALL,IID_PPV_ARGS(&enumerator)))) return 2;
        if (argc==2 && std::wstring(argv[1])==L"--enumerate") {
            auto c=scan(enumerator.Get(),0,nullptr,nullptr);
            std::printf("{\"event\":\"read_only_audio_inventory\",\"active_render_endpoints\":%u,\"sessions\":%u,\"unavailable\":%u,\"mutations\":0}\n",c.endpoints,c.sessions,c.unavailable);
            return 0;
        }
        if (argc!=14) return 2;
        const bool manual=std::wstring(argv[1])==L"--manual-volume";
        if (!manual && std::wstring(argv[1])!=L"--guard") return 2;
        wchar_t mute[2]{};
        if (manual && GetEnvironmentVariableW(L"GEARSVR_TEST_MUTE_GAME",mute,2)==1 && mute[0]==L'1') return 2;
        std::map<std::wstring,std::wstring> args;
        for (int i=2;i<argc;i+=2) if (!args.emplace(argv[i],argv[i+1]).second) return 2;
        auto pid64=number(args.at(L"--pid")), owner64=number(args.at(L"--owner-pid"));
        if (pid64>MAXDWORD || owner64>MAXDWORD || pid64==owner64) return 2;
        DWORD pid=static_cast<DWORD>(pid64), owner=static_cast<DWORD>(owner64);
        auto game_creation=number(args.at(L"--creation-time")), owner_creation=number(args.at(L"--owner-creation-time"));
        auto seconds=number(args.at(L"--max-seconds")); if (seconds>21600) return 2;
        std::wstring nonce=args.at(L"--nonce");
        if (nonce.size()!=32 || nonce.find_first_not_of(L"0123456789abcdef")!=std::wstring::npos) return 2;
        std::wstring event=L"Local\\GearsVR.TestAudio.Stop."+nonce;
        Handle game{OpenProcess(SYNCHRONIZE|PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid)};
        Handle parent{OpenProcess(SYNCHRONIZE|PROCESS_QUERY_LIMITED_INFORMATION,FALSE,owner)};
        Handle stop{OpenEventW(SYNCHRONIZE,FALSE,event.c_str())};
        if (!game.value || !parent.value || !stop.value || creation(game.value)!=game_creation ||
            creation(parent.value)!=owner_creation || !alive(game.value) || !alive(parent.value)) {
            std::printf("{\"event\":\"identity_rejected\",\"mutations\":0}\n"); return 3;
        }
        Policy policy; ManualPolicy manual_policy; RestoreFallback fallback{policy};
        std::printf("{\"event\":\"guard_ready\",\"pid\":%lu,\"creation_time\":%llu,\"matched\":0,\"muted\":0}\n",pid,game_creation); std::fflush(stdout);
        HANDLE waits[]{stop.value,parent.value,game.value};
        auto deadline=GetTickCount64()+seconds*1000; char const* reason="deadline";
        std::uint64_t scans{}, unavailable{}, shared{}, last_muted{}, last_configured{};
        while (GetTickCount64()<deadline) {
            DWORD signal=WaitForMultipleObjects(3,waits,FALSE,0);
            if (signal!=WAIT_TIMEOUT) { reason=signal==WAIT_OBJECT_0?"stop":signal==WAIT_OBJECT_0+1?"owner_exit":signal==WAIT_OBJECT_0+2?"game_exit":"wait_error"; break; }
            auto c=scan(enumerator.Get(),pid,game.value,manual?nullptr:&policy,manual?&manual_policy:nullptr); ++scans; unavailable+=c.unavailable; shared+=c.rejected_shared;
            if (manual && manual_policy.configured()!=last_configured) {
                last_configured=manual_policy.configured();
                std::printf("{\"event\":\"manual_audio_configured\",\"sessions\":%llu,\"volume\":1.0,\"muted\":false}\n",last_configured); std::fflush(stdout);
            }
            auto counts=policy.counts();
            if (counts.muted!=last_muted) {
                last_muted=counts.muted;
                std::printf("{\"event\":\"audio_muted\",\"matched\":%llu,\"muted\":%llu}\n",counts.matched,counts.muted); std::fflush(stdout);
            }
            signal=WaitForMultipleObjects(3,waits,FALSE,250);
            if (signal!=WAIT_TIMEOUT) { reason=signal==WAIT_OBJECT_0?"stop":signal==WAIT_OBJECT_0+1?"owner_exit":signal==WAIT_OBJECT_0+2?"game_exit":"wait_error"; break; }
        }
        if (manual) {
            std::printf("{\"event\":\"manual_audio_cleanup\",\"reason\":\"%s\",\"sessions_configured\":%llu,\"configure_failed\":%llu}\n",reason,manual_policy.configured(),manual_policy.failed);
            std::fflush(stdout); return 0;
        }
        // Original COM objects survive target/owner exit and are restored before
        // their final Release. Never locate a successor PID/session to restore.
        fallback.finish();
        auto c=policy.counts();
        std::printf("{\"event\":\"guard_cleanup\",\"reason\":\"%s\",\"matched\":%llu,\"muted\":%llu,\"restore_attempted\":%llu,\"restored\":%llu,\"restore_pending\":%llu,\"read_failed\":%llu,\"mute_failed\":%llu,\"scans\":%llu,\"endpoint_unavailable\":%llu,\"shared_rejected\":%llu}\n",reason,c.matched,c.muted,c.restore_attempted,c.restored,policy.pending(),c.read_failed,c.mute_failed,scans,unavailable,shared);
        std::fflush(stdout); return policy.pending()?4:0;
    } catch (...) { std::printf("{\"event\":\"guard_error\"}\n"); return 2; }
}
