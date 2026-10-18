#include <Windows.h>
#include <cstdio>
#include <vector>
#include "test_background_policy.h"

// Real USER32 regression on a private desktop. Never SwitchDesktop, activate,
// or address windows on the user's input desktop. Fail rather than fall back.
int wmain() {
    wchar_t name[96]{};
    swprintf_s(name,L"GearsQuietPolicy-%lu-%llu",GetCurrentProcessId(),GetTickCount64());
    const auto previous=GetThreadDesktop(GetCurrentThreadId());
    auto desktop=CreateDesktopW(name,nullptr,nullptr,0,DESKTOP_CREATEWINDOW|DESKTOP_READOBJECTS|DESKTOP_WRITEOBJECTS,nullptr);
    if(!desktop){std::printf("CreateDesktop failed: %lu\n",GetLastError());return 2;}
    if(!SetThreadDesktop(desktop)){std::printf("SetThreadDesktop failed: %lu\n",GetLastError());CloseDesktop(desktop);return 2;}
    unsigned cases{},legacy_violations{},atomic_violations{},errors{};
    for(bool atomic:{false,true}) {
    for(DWORD extended:{DWORD(0),DWORD(WS_EX_TOPMOST)}) {
        for(bool asynchronous:{false,true}) {
            // Use the exact ordinary no-activation show used by both candidates.
            HWND existing=CreateWindowExW(0,L"STATIC",L"Existing",WS_OVERLAPPEDWINDOW,0,0,200,200,nullptr,nullptr,nullptr,nullptr);
            HWND test=CreateWindowExW(extended,L"STATIC",L"Test",WS_OVERLAPPEDWINDOW,0,0,200,200,nullptr,nullptr,nullptr,nullptr);
            if(!existing||!test){++errors;}
            else {
                constexpr UINT base=SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_NOOWNERZORDER;
                if(!SetWindowPos(existing,HWND_TOP,0,0,0,0,base|SWP_SHOWWINDOW))++errors;
                if(atomic) {
                    if(!SetWindowPos(test,HWND_BOTTOM,0,0,0,0,gears_quiet_test::behind_show_flags(asynchronous)))++errors;
                } else {
                    if(!SetWindowPos(test,HWND_BOTTOM,0,0,0,0,base))++errors;
                    if(asynchronous)ShowWindowAsync(test,SW_SHOWNA);else ShowWindow(test,SW_SHOWNA);
                }
                MSG message{};
                while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){TranslateMessage(&message);DispatchMessageW(&message);}
                bool before_existing=false,saw_existing=false,saw_test=false;
                unsigned steps{};
                for(HWND h=GetTopWindow(nullptr);h&&steps++<64;h=GetWindow(h,GW_HWNDNEXT)) {
                    if(h==existing)saw_existing=true;
                    if(h==test){saw_test=true;before_existing=!saw_existing;}
                }
                if(!saw_existing||!saw_test||!IsWindowVisible(test))++errors;
                if(before_existing){if(atomic)++atomic_violations;else ++legacy_violations;}
                ++cases;
                std::printf("atomic=%u topmost=%u async=%u test_above_existing=%u visible=%u\n",
                    unsigned(atomic),unsigned(extended!=0),unsigned(asynchronous),unsigned(before_existing),unsigned(IsWindowVisible(test)!=0));
            }
            if(test)DestroyWindow(test);
            if(existing)DestroyWindow(existing);
        }
    }
    }
    if(!SetThreadDesktop(previous)){std::printf("Restore thread desktop failed: %lu\n",GetLastError());++errors;}
    if(!CloseDesktop(desktop))++errors;
    std::printf("private desktop cases=%u legacy_order_violations=%u atomic_order_violations=%u errors=%u; input desktop never switched\n",cases,legacy_violations,atomic_violations,errors);
    return errors?2:atomic_violations?1:0;
}
