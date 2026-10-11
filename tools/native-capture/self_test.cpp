#include <windows.h>
#include <cstdio>
int wmain(int argc,wchar_t** argv) {
    if(argc!=2) return 2;
    HMODULE module=LoadLibraryW(argv[1]);
    if(!module) {printf("Load failed %lu\n",GetLastError());return 3;}
    auto test=reinterpret_cast<DWORD(WINAPI*)(void*)>(GetProcAddress(module,"GearsCaptureSelfTest"));
    if(!test) return 4;
    DWORD result=test(nullptr);
    printf("Capture ABI/self-test result=%lu (0=pass)\n",result);
    // The instrumentation DLL deliberately remains resident until process exit.
    return static_cast<int>(result);
}
