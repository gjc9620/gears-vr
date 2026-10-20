#include <Windows.h>
#include <ShlObj.h>
#include <filesystem>
#include <cassert>
#include <string>
#include <iostream>

int wmain(int argc,wchar_t** argv) {
    assert(argc==3);
    const auto root=std::filesystem::absolute(argv[2]).make_preferred();
    const auto original=root/L"Original";
    for(const auto& name:{L"LocalAppData/Coalition/WarGame",L"RoamingAppData",L"Documents",L"Original/Coalition/WarGame"})
        std::filesystem::create_directories(root/name);
    SetEnvironmentVariableW(L"GEARSVR_INSTANCE_ID",L"fixture");
    SetEnvironmentVariableW(L"GEARSVR_INSTANCE_ROOT",root.c_str());
    SetEnvironmentVariableW(L"GEARSVR_INSTANCE_ORIGINAL_LOCALAPPDATA",original.c_str());
    HANDLE shared=CreateMutexW(nullptr,FALSE,L"Guardian_UnrealEngine3_1");assert(shared);
    const std::wstring other=L"GearsVR.InstancePolicy.Fixture.Other."+std::to_wstring(GetCurrentProcessId());
    HANDLE held=CreateMutexW(nullptr,FALSE,other.c_str());assert(held);
    const auto file_name=L"instance-write-"+std::to_wstring(GetCurrentProcessId())+L".txt";
    const auto old_file=original/L"Coalition/WarGame"/file_name;
    HANDLE sentinel=CreateFileW(old_file.c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_ALWAYS,0,nullptr);
    assert(sentinel!=INVALID_HANDLE_VALUE);DWORD sentinel_written{};
    assert(WriteFile(sentinel,"original",8,&sentinel_written,nullptr)&&sentinel_written==8);CloseHandle(sentinel);
    HANDLE original_directory=CreateFileW((original/L"Coalition/WarGame").c_str(),0,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                                          nullptr,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,nullptr);
    assert(original_directory!=INVALID_HANDLE_VALUE);
    wchar_t physical[32768]{};
    assert(GetFinalPathNameByHandleW(original_directory,physical,32768,VOLUME_NAME_GUID)>0);CloseHandle(original_directory);
    const auto physical_original=std::filesystem::path(physical)/file_name;
    wchar_t original_roaming[MAX_PATH]{};
    assert(SHGetFolderPathW(nullptr,CSIDL_APPDATA,nullptr,0,original_roaming)==S_OK);
    HMODULE dll=LoadLibraryW(argv[1]);assert(dll);
    auto install=reinterpret_cast<DWORD(WINAPI*)(void*)>(GetProcAddress(dll,"GearsQuietInstall"));
    const DWORD installed=install?install(nullptr):ERROR_PROC_NOT_FOUND;
    std::cout<<"install result="<<installed<<std::endl;
    assert(installed==ERROR_SUCCESS);
    SetLastError(0);
    HANDLE own=CreateMutexW(nullptr,FALSE,L"Guardian_UnrealEngine3_1");assert(own&&GetLastError()!=ERROR_ALREADY_EXISTS);
    HANDLE again=CreateMutexA(nullptr,FALSE,"Guardian_UnrealEngine3_1");assert(again&&GetLastError()==ERROR_ALREADY_EXISTS);
    HANDLE opened=OpenMutexW(SYNCHRONIZE,FALSE,L"Guardian_UnrealEngine3_1");assert(opened);
    HANDLE unchanged=CreateMutexW(nullptr,FALSE,other.c_str());assert(unchanged&&GetLastError()==ERROR_ALREADY_EXISTS);
    wchar_t folder[MAX_PATH]{};
    assert(SHGetFolderPathW(nullptr,CSIDL_LOCAL_APPDATA,nullptr,SHGFP_TYPE_CURRENT,folder)==S_OK);
    assert(std::filesystem::path(folder)==root/L"LocalAppData");
    PWSTR known=nullptr;
    assert(SHGetKnownFolderPath(FOLDERID_LocalAppData,0,nullptr,&known)==S_OK);
    assert(std::filesystem::path(known)==root/L"LocalAppData");CoTaskMemFree(known);
    wchar_t current_roaming[MAX_PATH]{};
    assert(SHGetFolderPathW(nullptr,CSIDL_APPDATA,nullptr,0,current_roaming)==S_OK);
    assert(wcscmp(original_roaming,current_roaming)==0);
    HANDLE file=CreateFileW(old_file.c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_ALWAYS,0,nullptr);assert(file!=INVALID_HANDLE_VALUE);
    DWORD written{};assert(WriteFile(file,"isolated",8,&written,nullptr)&&written==8);CloseHandle(file);
    assert(std::filesystem::exists(root/L"LocalAppData/Coalition/WarGame"/file_name));
    const auto renamed=original/L"Coalition/WarGame"/(file_name+L".moved");
    assert(MoveFileExW(old_file.c_str(),renamed.c_str(),MOVEFILE_REPLACE_EXISTING));
    assert(!std::filesystem::exists(old_file)&&std::filesystem::exists(renamed));
    assert(DeleteFileW(renamed.c_str()));
    HANDLE untouched=CreateFileW(physical_original.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);
    assert(untouched!=INVALID_HANDLE_VALUE);char bytes[8]{};DWORD read{};
    assert(ReadFile(untouched,bytes,8,&read,nullptr)&&read==8&&memcmp(bytes,"original",8)==0);CloseHandle(untouched);
    for(HANDLE h:{shared,held,own,again,opened,unchanged})CloseHandle(h);
    // Hooks belong to this disposable process; unloading their DLL is unsafe.
    std::cout<<"exact guardian namespace, unrelated mutex preservation, known folders and profile writes passed\n";
    return 0;
}
