#include "quiet_startup.h"
#include <shellapi.h>
#include <fstream>
#include <filesystem>
#include <array>
#include <memory>
#include <cstring>

int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,PWSTR,int) {
    int argc{};wchar_t** argv=CommandLineToArgvW(GetCommandLineW(),&argc);
    if(!argv || argc!=2)return 10;
    const std::filesystem::path output=argv[1];LocalFree(argv);
    const auto policy=GetModuleHandleW(L"GearsQuietStartup.dll");
    const auto query=reinterpret_cast<gears_quiet::Read>(policy?GetProcAddress(policy,"GearsQuietStatus"):nullptr);
    gears_quiet::Status before,after;
    if(!query || query(&before)!=0 || before.installed!=1)return 11;
    wchar_t main_option[2]{};
    const bool main_requested=GetEnvironmentVariableW(L"GEARSVR_QUIET_MAIN_NOACTIVATE",main_option,2)==1&&main_option[0]==L'1';
    WNDCLASSW cls{};cls.hInstance=instance;cls.lpfnWndProc=DefWindowProcW;cls.lpszClassName=L"GearsQuietStartupFixture";
    if(!RegisterClassW(&cls))return 12;
    WNDCLASSA ansi_cls{};ansi_cls.hInstance=instance;ansi_cls.lpfnWndProc=DefWindowProcA;ansi_cls.lpszClassName="GearsQuietStartupFixtureA";
    if(!RegisterClassA(&ansi_cls))return 15;
    struct Observation {ULONGLONG tick{};const char* stage{};const char* edge{};HWND foreground{},active{};};
    std::array<Observation,128> timeline{};unsigned timeline_count{};bool timeline_overflow=false;
    auto observe=[&](const char* stage,const char* edge){
        if(timeline_count==timeline.size()){timeline_overflow=true;return;}
        timeline[timeline_count++]={GetTickCount64(),stage,edge,GetForegroundWindow(),GetActiveWindow()};};
    auto step=[&](const char* stage,auto operation){observe(stage,"before");auto result=operation();observe(stage,"after");return result;};
    const HWND prior=GetForegroundWindow();
    const auto cbt_query=reinterpret_cast<gears_quiet::ReadTrace>(GetProcAddress(policy,"GearsQuietCbtTrace"));
    auto cbt=std::make_unique<gears_quiet::CbtSnapshot>();
    HWND message_only=CreateWindowExW(WS_EX_APPWINDOW,cls.lpszClassName,L"Message only",0,0,0,0,0,HWND_MESSAGE,nullptr,instance,nullptr);
    const bool message_style=message_only&&(GetWindowLongPtrW(message_only,GWL_EXSTYLE)&WS_EX_NOACTIVATE)==0;
    bool cbt_valid=cbt_query&&cbt_query(cbt.get())==0&&cbt->thread==0&&!cbt->installed;
    if(message_only)DestroyWindow(message_only);
    HWND window=step("create_window_w",[&](){return CreateWindowExW(0,cls.lpszClassName,L"Quiet startup fixture",WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                               -30000,-30000,320,240,nullptr,nullptr,instance,nullptr);});
    if(!window)return 13;
    HWND ansi=step("create_window_a",[&](){return CreateWindowExA(0,ansi_cls.lpszClassName,"Quiet startup ANSI fixture",WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                             -30000,-30000,320,240,nullptr,nullptr,instance,nullptr);});
    if(!ansi){step("destroy_window",[&](){return DestroyWindow(window);});return 16;}
    constexpr DWORD main_exstyle=WS_EX_APPWINDOW|WS_EX_ACCEPTFILES;
    HWND main_w=step("create_main_w",[&](){return CreateWindowExW(main_exstyle,cls.lpszClassName,L"Main W",WS_OVERLAPPEDWINDOW|WS_VISIBLE,
        -30000,-30000,320,240,nullptr,nullptr,instance,nullptr);});
    HWND main_a=step("create_main_a",[&](){return CreateWindowExA(main_exstyle,ansi_cls.lpszClassName,"Main A",WS_OVERLAPPEDWINDOW|WS_VISIBLE,
        -30000,-30000,320,240,nullptr,nullptr,instance,nullptr);});
    const DWORD expected_main=main_exstyle|(main_requested?WS_EX_NOACTIVATE:0);
    const auto main_matches=[&](HWND hwnd){return hwnd&&IsWindowVisible(hwnd)&&
        (GetWindowLongPtrW(hwnd,GWL_EXSTYLE)&(main_exstyle|WS_EX_NOACTIVATE))==expected_main;};
    const bool main_style=main_matches(main_w)&&main_matches(main_a);
    // Child coordinates remain inside the offscreen parent. WS_CHILD must not
    // become an owned top-level window or retain a temporary NOACTIVATE bit.
    HWND child=step("create_child_a",[&](){return CreateWindowExA(WS_EX_APPWINDOW,ansi_cls.lpszClassName,"Quiet child",WS_CHILD|WS_VISIBLE,
                              0,0,100,50,window,nullptr,instance,nullptr);});
    HWND owned=step("create_owned_w",[&](){return CreateWindowExW(WS_EX_APPWINDOW,cls.lpszClassName,L"Quiet owned popup",WS_POPUP|WS_VISIBLE,
                              -30000,-30000,100,50,window,nullptr,instance,nullptr);});
    HWND explicit_style=step("create_explicit_a",[&](){return CreateWindowExA(WS_EX_NOACTIVATE,ansi_cls.lpszClassName,"Explicit no-activate",WS_POPUP|WS_VISIBLE,
                                       -30000,-30000,100,50,nullptr,nullptr,instance,nullptr);});
    const bool ansi_visible=ansi&&IsWindowVisible(ansi);
    const bool ansi_clickable=ansi&&(GetWindowLongPtrA(ansi,GWL_EXSTYLE)&WS_EX_NOACTIVATE)==0;
    const bool parent_child=child&&GetParent(child)==window&&(GetWindowLongPtrA(child,GWL_STYLE)&WS_CHILD)&&
        IsWindowVisible(child)&&(GetWindowLongPtrA(child,GWL_EXSTYLE)&WS_EX_NOACTIVATE)==0;
    const bool owned_popup=owned&&GetWindow(owned,GW_OWNER)==window&&IsWindowVisible(owned)&&
        !(GetWindowLongPtrW(owned,GWL_STYLE)&WS_CHILD)&&(GetWindowLongPtrW(owned,GWL_EXSTYLE)&WS_EX_NOACTIVATE)==0;
    const bool explicit_preserved=explicit_style&&(GetWindowLongPtrA(explicit_style,GWL_EXSTYLE)&WS_EX_NOACTIVATE)!=0;
    step("parent_hide",[&](){return ShowWindow(window,SW_HIDE);});
    const bool parent_hide=IsWindowVisible(window)==FALSE&&child&&IsWindowVisible(child)==FALSE;
    step("window_show",[&](){return ShowWindow(window,SW_SHOW);});
    const bool child_restored=child&&IsWindowVisible(window)&&IsWindowVisible(child)&&GetParent(child)==window;
    step("ansi_hide",[&](){return ShowWindow(ansi,SW_HIDE);});
    const bool hidden=IsWindowVisible(ansi)==FALSE;
    step("ansi_show_minimized",[&](){return ShowWindow(ansi,SW_SHOWMINIMIZED);});
    const bool minimized=IsWindowVisible(ansi)&&IsIconic(ansi);
    step("ansi_restore",[&](){return ShowWindow(ansi,SW_RESTORE);});
    const bool restored=IsWindowVisible(ansi)&&!IsIconic(ansi);
    step("ansi_minimize",[&](){return ShowWindow(ansi,SW_MINIMIZE);});
    const bool minimize_command=IsIconic(ansi)!=FALSE;
    step("ansi_restore",[&](){return ShowWindow(ansi,SW_RESTORE);});
    const bool restored_again=IsWindowVisible(ansi)&&!IsIconic(ansi);
    step("window_show",[&](){return ShowWindow(window,SW_SHOW);});
    step("set_foreground",[&](){return SetForegroundWindow(window);});
    step("set_active",[&](){return SetActiveWindow(window);});
    step("set_focus",[&](){return SetFocus(window);});
    step("set_position_topmost",[&](){return SetWindowPos(window,HWND_TOPMOST,-30000,-30000,320,240,SWP_SHOWWINDOW);});
    step("bring_to_top",[&](){return BringWindowToTop(window);});
    wchar_t native_option[2]{};
    const bool native_requested=GetEnvironmentVariableW(L"GEARSVR_QUIET_NATIVE_FOREGROUND",native_option,2)==1&&native_option[0]==L'1';
    const auto native_fg=reinterpret_cast<decltype(&SetForegroundWindow)>(GetProcAddress(GetModuleHandleW(L"win32u.dll"),"NtUserSetForegroundWindow"));
    gears_quiet::Status native_before{},native_after{};
    bool native_valid=native_fg&&query(&native_before)==0;
    BOOL native_result=TRUE;
    if(native_valid)native_result=step("native_foreground",[&](){return native_fg(native_requested?window:nullptr);});
    native_valid=native_valid&&query(&native_after)==0&&
        native_after.suppressed[gears_quiet::foreground]==native_before.suppressed[gears_quiet::foreground]+(native_requested?1:0)&&
        (!native_requested||native_result==FALSE);
    MSG message{};
    for(unsigned i=0;i<30;++i) {
        observe("message_pump","before");
        while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){TranslateMessage(&message);DispatchMessageW(&message);}
        observe("message_pump","after");
        if(i==15){step("window_restore",[&](){return ShowWindow(window,SW_RESTORE);});step("set_foreground",[&](){return SetForegroundWindow(window);});}
        Sleep(10);
    }
    step("system_minimize",[&](){return SendMessageW(window,WM_SYSCOMMAND,SC_MINIMIZE,0);});
    step("system_restore",[&](){return ShowWindow(window,SW_RESTORE);});
    const bool visible=IsWindowVisible(window)!=FALSE;
    const bool clickable=(GetWindowLongPtrW(window,GWL_EXSTYLE)&WS_EX_NOACTIVATE)==0;
    const bool topmost=(GetWindowLongPtrW(window,GWL_EXSTYLE)&WS_EX_TOPMOST)!=0;
    const HWND final=GetForegroundWindow();
    const bool same=final==prior && prior!=window&&prior!=ansi&&prior!=owned&&prior!=explicit_style&&prior!=main_w&&prior!=main_a;
    const bool queried=query(&after)==0&&after.installed==1;
    const auto query_trace=reinterpret_cast<gears_quiet::ReadTrace>(GetProcAddress(policy,"GearsQuietTrace2"));
    auto trace=std::make_unique<gears_quiet::TraceSnapshot2>();
    bool trace_valid=query_trace&&query_trace(trace.get())==ERROR_SUCCESS;
    // Keep old readers safe and verify diagnostics do not change LastError.
    const auto legacy_query=reinterpret_cast<gears_quiet::ReadTrace>(GetProcAddress(policy,"GearsQuietTrace"));
    struct LegacyBuffer {gears_quiet::TraceSnapshot snapshot;std::uint64_t canary{0x123456789abcdef0ull};};
    auto legacy=std::make_unique<LegacyBuffer>();
    SetLastError(0x12345678);
    trace_valid=trace_valid&&legacy_query&&legacy_query(&legacy->snapshot)==ERROR_SUCCESS&&
        GetLastError()==0x12345678&&legacy->canary==0x123456789abcdef0ull&&legacy->snapshot.version==1;
    bool trace_requested=false;wchar_t trace_option[2]{};
    trace_requested=GetEnvironmentVariableW(L"GEARSVR_QUIET_TRACE",trace_option,2)==1&&trace_option[0]==L'1';
    cbt_valid=cbt_valid&&cbt_query(cbt.get())==0&&cbt->version==1&&cbt->size==sizeof(*cbt)&&cbt->busy_skipped==0;
    if(trace_requested){
        cbt_valid=cbt_valid&&cbt->installed==1&&cbt->thread==GetCurrentThreadId()&&cbt->install_error==0&&cbt->completed>0;
        for(unsigned i=0;i<cbt->completed;++i){const auto& r=cbt->records[i];
            cbt_valid=cbt_valid&&(r.call==HCBT_ACTIVATE||r.call==HCBT_SYSCOMMAND||r.call==HCBT_MINMAX)&&r.thread==cbt->thread&&
                r.enter_qpc>0&&r.leave_qpc>=r.enter_qpc&&r.return_valid&&r.native_valid&&r.returned_value==r.native_value;}
    }else cbt_valid=cbt_valid&&!cbt->installed&&!cbt->completed&&!cbt->thread;
    trace_valid=trace_valid&&cbt_valid;
    bool trace_null=false,trace_passed=false,trace_blocked=false,native_trace=false,async_trace=false;unsigned trace_call_mask{};
    if(trace_valid&&trace_requested){
        STARTUPINFOW startup{sizeof(startup)};GetStartupInfoW(&startup);
        trace_valid=trace->startup.flags==startup.dwFlags&&trace->startup.show_window==startup.wShowWindow;
        SetActiveWindow(nullptr);SetFocus(nullptr);
        const BOOL async_result=ShowWindowAsync(window,SW_SHOWNA);
        trace_valid=trace_valid&&query_trace(trace.get())==ERROR_SUCCESS&&trace->enabled==1&&trace->version==2&&trace->qpc_frequency>0&&
            trace->size==sizeof(*trace)&&trace->capacity==gears_quiet::trace_capacity&&trace->completed>0&&trace->busy_skipped==0;
        for(unsigned i=0;i<trace->completed&&trace_valid;++i){const auto& row=trace->records[i];
            trace_valid=row.sequence==i+1&&row.call<gears_quiet::trace_call_count&&row.thread!=0&&row.stack_count>0&&
                row.stack_count<=gears_quiet::trace_stack_capacity&&row.leave_tick>=row.enter_tick&&row.enter_qpc>0&&row.leave_qpc>=row.enter_qpc&&row.return_valid==1&&
                row.gui_before.valid==1&&row.gui_after.valid==1&&(!row.native_valid||row.native_call<gears_quiet::trace_call_count);
            native_trace|=row.call==gears_quiet::native_foreground&&row.target==reinterpret_cast<uintptr_t>(window)&&row.suppressed==1&&!row.native_valid&&row.returned_value==0;
            async_trace|=row.call==gears_quiet::show_async&&row.target==reinterpret_cast<uintptr_t>(window)&&
                row.argument==SW_SHOWNA&&row.effective_argument==SW_SHOWNA&&row.extra_argument==GetCurrentThreadId()&&
                row.suppressed==0&&row.native_valid&&row.native_value==static_cast<std::uint64_t>(async_result)&&row.returned_value==row.native_value;
            if(row.call<gears_quiet::count)trace_call_mask|=1u<<row.call;
            if(row.suppressed&&(row.call==gears_quiet::foreground||row.call==gears_quiet::active||
                row.call==gears_quiet::focus||row.call==gears_quiet::bring))trace_valid=trace_valid&&row.native_valid==0;
            trace_passed|=row.suppressed==0;trace_blocked|=row.suppressed==1;
            trace_null|=(row.call==gears_quiet::active||row.call==gears_quiet::focus)&&row.target==0&&row.suppressed==0;
        }
        trace_valid=trace_valid&&(!native_requested||native_trace)&&async_trace;
        auto saved=std::make_unique<gears_quiet::TraceSnapshot2>(*trace);
        for(unsigned i=0;i<gears_quiet::trace_capacity+1;++i)ShowWindow(window,SW_SHOWNA);
        trace_valid=trace_valid&&query_trace(trace.get())==ERROR_SUCCESS&&trace->completed==gears_quiet::trace_capacity&&
            trace->reserved_count==gears_quiet::trace_capacity&&trace->overflow==1&&trace->busy_skipped==0&&
            trace_passed&&trace_blocked&&trace_null&&trace_call_mask==((1u<<gears_quiet::count)-1)&&
            std::memcmp(trace->records,saved->records,saved->completed*sizeof(gears_quiet::TraceRecord2))==0;
    }else if(trace_valid){trace_valid=trace->enabled==0&&trace->completed==0&&trace->reserved_count==0&&trace->overflow==0;}
    trace_valid=trace_valid&&native_valid;
    const bool main_retained=main_matches(main_w)&&main_matches(main_a);
    if(main_w)step("destroy_main_w",[&](){return DestroyWindow(main_w);});if(main_a)step("destroy_main_a",[&](){return DestroyWindow(main_a);});
    if(explicit_style)step("destroy_explicit",[&](){return DestroyWindow(explicit_style);});if(owned)step("destroy_owned",[&](){return DestroyWindow(owned);});
    step("destroy_ansi",[&](){return DestroyWindow(ansi);});step("destroy_window",[&](){return DestroyWindow(window);});
    std::ofstream out(output);
    out<<"{\"visible\":"<<visible<<",\"temporary_style_removed\":"<<clickable
       <<",\"topmost\":"<<topmost<<",\"foreground_preserved\":"<<same
       <<",\"ansi_visible\":"<<ansi_visible<<",\"ansi_temporary_style_removed\":"<<ansi_clickable
       <<",\"parent_child_preserved\":"<<parent_child<<",\"owned_popup_preserved\":"<<owned_popup
       <<",\"parent_hide_preserved\":"<<parent_hide<<",\"child_visibility_restored\":"<<child_restored
       <<",\"explicit_noactivate_preserved\":"<<explicit_preserved
       <<",\"native_foreground_requested\":"<<native_requested<<",\"native_foreground_valid\":"<<native_valid
       <<",\"main_noactivate_requested\":"<<main_requested<<",\"main_style_matched\":"<<main_style
       <<",\"main_style_retained\":"<<main_retained<<",\"message_only_style_preserved\":"<<message_style
       <<",\"hide_preserved\":"<<hidden<<",\"show_minimized_preserved\":"<<minimized
       <<",\"restore_preserved\":"<<restored<<",\"minimize_preserved\":"<<minimize_command
       <<",\"restore_again_preserved\":"<<restored_again<<",\"status_queried\":"<<queried<<",\"suppressed\":[";
    for(unsigned i=0;i<gears_quiet::count;++i){if(i)out<<',';out<<after.suppressed[i];}
    out<<"],\"trace_requested\":"<<trace_requested<<",\"trace_valid\":"<<trace_valid
       <<",\"cbt_valid\":"<<cbt_valid<<",\"cbt_installed\":"<<cbt->installed<<",\"cbt_completed\":"<<cbt->completed
       <<",\"trace_completed\":"<<trace->completed<<",\"trace_overflow\":"<<trace->overflow
       <<",\"trace_null_passed\":"<<trace_null<<",\"trace_call_mask\":"<<trace_call_mask
       <<",\"windows\":{\"window\":"<<reinterpret_cast<uintptr_t>(window)
       <<",\"ansi\":"<<reinterpret_cast<uintptr_t>(ansi)<<",\"child\":"<<reinterpret_cast<uintptr_t>(child)
       <<",\"owned\":"<<reinterpret_cast<uintptr_t>(owned)<<",\"explicit\":"<<reinterpret_cast<uintptr_t>(explicit_style)
       <<",\"main_w\":"<<reinterpret_cast<uintptr_t>(main_w)<<",\"main_a\":"<<reinterpret_cast<uintptr_t>(main_a)
       <<"},\"timeline_overflow\":"<<timeline_overflow<<",\"timeline\":[";
    for(unsigned i=0;i<timeline_count;++i){const auto& row=timeline[i];if(i)out<<',';
        out<<"{\"tick_ms\":"<<row.tick<<",\"stage\":\""<<row.stage<<"\",\"edge\":\""<<row.edge
           <<"\",\"foreground\":"<<reinterpret_cast<uintptr_t>(row.foreground)
           <<",\"active\":"<<reinterpret_cast<uintptr_t>(row.active)<<'}';}
    out<<"]}\n";
    return visible&&clickable&&!topmost&&same&&ansi_visible&&ansi_clickable&&parent_child&&parent_hide&&child_restored&&owned_popup&&explicit_preserved&&
        hidden&&minimized&&restored&&minimize_command&&restored_again&&queried&&trace_valid&&main_style&&main_retained&&message_style?0:14;
}
