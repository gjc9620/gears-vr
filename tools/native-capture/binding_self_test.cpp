#include <windows.h>
#include <cstdio>
int wmain(int argc,wchar_t** argv) {
    if(argc!=2 && argc!=3)return 2;
    bool readback=argc==3 && !wcscmp(argv[2],L"--readback");if(argc==3 && !readback)return 2;
    HMODULE dll=LoadLibraryW(argv[1]);if(!dll)return 3;
    auto test=reinterpret_cast<DWORD(WINAPI*)(void*)>(GetProcAddress(dll,readback?"GearsReadbackSelfTest":"GearsBindingSelfTest"));
    if(!test)return 4;
    DWORD result=test(nullptr);printf("Binding self-test=%lu (0=pass)\n",result);
    return static_cast<int>(result);
}
