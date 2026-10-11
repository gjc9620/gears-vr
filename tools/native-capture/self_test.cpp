#include <windows.h>
#include <cstdio>
int wmain(int argc,wchar_t** argv) {
    if(argc!=2 && argc!=3) return 2;
    bool gpu=argc==3 && !wcscmp(argv[2],L"--gpu"),family=argc==3 && !wcscmp(argv[2],L"--family"),clone=argc==3 && !wcscmp(argv[2],L"--clone");if(argc==3 && !gpu && !family && !clone)return 2;
    HMODULE module=LoadLibraryW(argv[1]);
    if(!module) {printf("Load failed %lu\n",GetLastError());return 3;}
    auto test=reinterpret_cast<DWORD(WINAPI*)(void*)>(GetProcAddress(module,gpu?"GearsCaptureGPUCheck":family?"GearsCaptureFamilyCheck":clone?"GearsCaptureCloneCheck":"GearsCaptureSelfTest"));
    if(!test) return 4;
    DWORD result=test(nullptr);
    printf("Capture ABI/self-test result=%lu (0=pass)\n",result);
    // The instrumentation DLL deliberately remains resident until process exit.
    return static_cast<int>(result);
}
