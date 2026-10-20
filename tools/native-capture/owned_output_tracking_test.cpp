#include <windows.h>
#include <cstdio>
#include <cstdlib>
int wmain(int argc,wchar_t** argv){
    if(argc!=3)return 2;
    const unsigned mode=static_cast<unsigned>(wcstoul(argv[2],nullptr,10));
    HMODULE dll=LoadLibraryW(argv[1]);if(!dll)return 3;
    auto test=reinterpret_cast<DWORD(WINAPI*)(void*)>(GetProcAddress(dll,"GearsCaptureOwnedOutputTrackingCheck"));
    if(!test)return 4;
    DWORD result=test(const_cast<unsigned*>(&mode));
    printf("Owned output tracking mode=%u result=%lu (0=pass)\n",mode,result);
    return static_cast<int>(result);
}
