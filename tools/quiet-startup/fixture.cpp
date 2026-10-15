#include "quiet_startup.h"
#include <shellapi.h>
#include <fstream>
#include <filesystem>
#include <array>

int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,PWSTR,int) {
    int argc{};wchar_t** argv=CommandLineToArgvW(GetCommandLineW(),&argc);
    if(!argv || argc!=2)return 10;
    const std::filesystem::path output=argv[1];LocalFree(argv);
    const auto policy=GetModuleHandleW(L"GearsQuietStartup.dll");
    const auto query=reinterpret_cast<gears_quiet::Read>(policy?GetProcAddress(policy,"GearsQuietStatus"):nullptr);
    gears_quiet::Status before,after;
    if(!query || query(&before)!=0 || before.installed!=1)return 11;
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
    HWND window=step("create_window_w",[&](){return CreateWindowExW(0,cls.lpszClassName,L"Quiet startup fixture",WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                               -30000,-30000,320,240,nullptr,nullptr,instance,nullptr);});
    if(!window)return 13;
    HWND ansi=step("create_window_a",[&](){return CreateWindowExA(0,ansi_cls.lpszClassName,"Quiet startup ANSI fixture",WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                             -30000,-30000,320,240,nullptr,nullptr,instance,nullptr);});
    if(!ansi){step("destroy_window",[&](){return DestroyWindow(window);});return 16;}
    // Child coordinates remain inside the offscreen parent. WS_CHILD must not
    // become an owned top-level window or retain a temporary NOACTIVATE bit.
    HWND child=step("create_child_a",[&](){return CreateWindowExA(0,ansi_cls.lpszClassName,"Quiet child",WS_CHILD|WS_VISIBLE,
                              0,0,100,50,window,nullptr,instance,nullptr);});
    HWND owned=step("create_owned_w",[&](){return CreateWindowExW(0,cls.lpszClassName,L"Quiet owned popup",WS_POPUP|WS_VISIBLE,
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
    MSG message{};
    for(unsigned i=0;i<30;++i) {
        observe("message_pump","before");
        while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){TranslateMessage(&message);DispatchMessageW(&message);}
        observe("message_pump","after");
        if(i==15){step("window_restore",[&](){return ShowWindow(window,SW_RESTORE);});step("set_foreground",[&](){return SetForegroundWindow(window);});}
        Sleep(10);
    }
    const bool visible=IsWindowVisible(window)!=FALSE;
    const bool clickable=(GetWindowLongPtrW(window,GWL_EXSTYLE)&WS_EX_NOACTIVATE)==0;
    const bool topmost=(GetWindowLongPtrW(window,GWL_EXSTYLE)&WS_EX_TOPMOST)!=0;
    const HWND final=GetForegroundWindow();
    const bool same=final==prior && prior!=window&&prior!=ansi&&prior!=owned&&prior!=explicit_style;
    const bool queried=query(&after)==0&&after.installed==1;
    if(explicit_style)step("destroy_explicit",[&](){return DestroyWindow(explicit_style);});if(owned)step("destroy_owned",[&](){return DestroyWindow(owned);});
    step("destroy_ansi",[&](){return DestroyWindow(ansi);});step("destroy_window",[&](){return DestroyWindow(window);});
    std::ofstream out(output);
    out<<"{\"visible\":"<<visible<<",\"temporary_style_removed\":"<<clickable
       <<",\"topmost\":"<<topmost<<",\"foreground_preserved\":"<<same
       <<",\"ansi_visible\":"<<ansi_visible<<",\"ansi_temporary_style_removed\":"<<ansi_clickable
       <<",\"parent_child_preserved\":"<<parent_child<<",\"owned_popup_preserved\":"<<owned_popup
       <<",\"parent_hide_preserved\":"<<parent_hide<<",\"child_visibility_restored\":"<<child_restored
       <<",\"explicit_noactivate_preserved\":"<<explicit_preserved
       <<",\"hide_preserved\":"<<hidden<<",\"show_minimized_preserved\":"<<minimized
       <<",\"restore_preserved\":"<<restored<<",\"minimize_preserved\":"<<minimize_command
       <<",\"restore_again_preserved\":"<<restored_again<<",\"status_queried\":"<<queried<<",\"suppressed\":[";
    for(unsigned i=0;i<gears_quiet::count;++i){if(i)out<<',';out<<after.suppressed[i];}
    out<<"],\"windows\":{\"window\":"<<reinterpret_cast<uintptr_t>(window)
       <<",\"ansi\":"<<reinterpret_cast<uintptr_t>(ansi)<<",\"child\":"<<reinterpret_cast<uintptr_t>(child)
       <<",\"owned\":"<<reinterpret_cast<uintptr_t>(owned)<<",\"explicit\":"<<reinterpret_cast<uintptr_t>(explicit_style)
       <<"},\"timeline_overflow\":"<<timeline_overflow<<",\"timeline\":[";
    for(unsigned i=0;i<timeline_count;++i){const auto& row=timeline[i];if(i)out<<',';
        out<<"{\"tick_ms\":"<<row.tick<<",\"stage\":\""<<row.stage<<"\",\"edge\":\""<<row.edge
           <<"\",\"foreground\":"<<reinterpret_cast<uintptr_t>(row.foreground)
           <<",\"active\":"<<reinterpret_cast<uintptr_t>(row.active)<<'}';}
    out<<"]}\n";
    return visible&&clickable&&!topmost&&same&&ansi_visible&&ansi_clickable&&parent_child&&parent_hide&&child_restored&&owned_popup&&explicit_preserved&&
        hidden&&minimized&&restored&&minimize_command&&restored_again&&queried?0:14;
}
