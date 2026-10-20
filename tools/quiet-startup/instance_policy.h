#pragma once
// Opt-in, process-local instance isolation. Ordinary launches install no hooks.
#include <Windows.h>
#include <ShlObj.h>
#include <MinHook.h>
#include <atomic>
#include <string>
#include <cwchar>
#include <cstring>
#include <algorithm>
#include <intrin.h>

namespace gears_instance {
inline std::wstring id, root, local, roaming, documents, original_game_data, game_data, guardian, original_logs, logs;
inline std::atomic<unsigned long long> mutex_redirects{}, folder_redirects{}, file_redirects{};
inline unsigned failure{};
inline uintptr_t game_begin{},game_end{};
inline bool from_game(void* caller){const auto address=reinterpret_cast<uintptr_t>(caller);return address>=game_begin&&address<game_end;}
inline decltype(&CreateMutexW) create_mutex_w{};
inline decltype(&CreateMutexA) create_mutex_a{};
inline decltype(&OpenMutexW) open_mutex_w{};
inline decltype(&OpenMutexA) open_mutex_a{};
inline decltype(&SHGetFolderPathW) folder_w{};
inline decltype(&SHGetFolderPathA) folder_a{};
inline decltype(&SHGetKnownFolderPath) known_folder{};
inline decltype(&CreateFileW) create_file_w{};
inline decltype(&CreateDirectoryW) create_directory_w{};
inline decltype(&DeleteFileW) delete_file_w{};
inline decltype(&MoveFileExW) move_file_ex_w{};
inline decltype(&GetFileAttributesW) attributes_w{};
inline decltype(&GetFileAttributesExW) attributes_ex_w{};
inline decltype(&SetFileAttributesW) set_attributes_w{};
inline decltype(&FindFirstFileExW) find_first_w{};
inline std::wstring environment(const wchar_t* key) {
    const DWORD size=GetEnvironmentVariableW(key,nullptr,0);
    if(!size)return {};
    std::wstring value(size,L'\0');
    const DWORD got=GetEnvironmentVariableW(key,value.data(),size);
    if(!got||got>=size)return {};
    value.resize(got);return value;
}
inline bool valid_id(const std::wstring& value) {
    if(value.empty()||value.size()>40)return false;
    for(wchar_t ch:value)if(!((ch>=L'a'&&ch<=L'z')||(ch>=L'0'&&ch<=L'9')||ch==L'-'||ch==L'_'))return false;
    return true;
}
inline bool is_guardian(const wchar_t* name) {return name&&wcscmp(name,L"Guardian_UnrealEngine3_1")==0;}
inline bool is_guardian(const char* name) {return name&&strcmp(name,"Guardian_UnrealEngine3_1")==0;}
inline HANDLE WINAPI on_create_mutex_w(LPSECURITY_ATTRIBUTES attributes,BOOL owner,LPCWSTR name) {
    if(is_guardian(name)){++mutex_redirects;return create_mutex_w(attributes,owner,guardian.c_str());}
    return create_mutex_w(attributes,owner,name);
}
inline HANDLE WINAPI on_create_mutex_a(LPSECURITY_ATTRIBUTES attributes,BOOL owner,LPCSTR name) {
    if(is_guardian(name)){++mutex_redirects;return create_mutex_w(attributes,owner,guardian.c_str());}
    return create_mutex_a(attributes,owner,name);
}
inline HANDLE WINAPI on_open_mutex_w(DWORD access,BOOL inherit,LPCWSTR name) {
    if(is_guardian(name)){++mutex_redirects;return open_mutex_w(access,inherit,guardian.c_str());}
    return open_mutex_w(access,inherit,name);
}
inline HANDLE WINAPI on_open_mutex_a(DWORD access,BOOL inherit,LPCSTR name) {
    if(is_guardian(name)){++mutex_redirects;return open_mutex_w(access,inherit,guardian.c_str());}
    return open_mutex_a(access,inherit,name);
}
inline const std::wstring* folder(int csidl) {
    switch(csidl&0xff){case CSIDL_LOCAL_APPDATA:return &local;default:return nullptr;}
}
inline HRESULT WINAPI on_folder_w(HWND window,int csidl,HANDLE token,DWORD flags,LPWSTR output) {
    if(from_game(_ReturnAddress())&&!token&&output)if(auto selected=folder(csidl)){
        if(selected->size()>=MAX_PATH)return HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
        wcscpy_s(output,MAX_PATH,selected->c_str());++folder_redirects;return S_OK;
    }
    return folder_w(window,csidl,token,flags,output);
}
inline HRESULT WINAPI on_folder_a(HWND window,int csidl,HANDLE token,DWORD flags,LPSTR output) {
    if(from_game(_ReturnAddress())&&!token&&output)if(auto selected=folder(csidl)){
        if(!WideCharToMultiByte(CP_ACP,WC_NO_BEST_FIT_CHARS,selected->c_str(),-1,output,MAX_PATH,nullptr,nullptr))
            return HRESULT_FROM_WIN32(GetLastError());
        ++folder_redirects;return S_OK;
    }
    return folder_a(window,csidl,token,flags,output);
}
inline HRESULT WINAPI on_known_folder(REFKNOWNFOLDERID which,DWORD flags,HANDLE token,PWSTR* output) {
    const std::wstring* selected=nullptr;
    if(IsEqualGUID(which,FOLDERID_LocalAppData))selected=&local;
    if(from_game(_ReturnAddress())&&!token&&output&&selected){
        *output=static_cast<PWSTR>(CoTaskMemAlloc((selected->size()+1)*sizeof(wchar_t)));
        if(!*output)return E_OUTOFMEMORY;
        memcpy(*output,selected->c_str(),(selected->size()+1)*sizeof(wchar_t));++folder_redirects;return S_OK;
    }
    return known_folder(which,flags,token,output);
}
inline std::wstring file_path(LPCWSTR path) {
    if(!path)return {};
    std::wstring normalized(path);
    std::replace(normalized.begin(),normalized.end(),L'/',L'\\');
    const wchar_t* selected=normalized.c_str();
    if(wcsncmp(selected,L"\\\\?\\",4)==0)selected+=4;
    for(const auto& pair:{std::pair{&original_game_data,&game_data},std::pair{&original_logs,&logs}}){
        const auto size=pair.first->size();
        if(size&&_wcsnicmp(selected,pair.first->c_str(),size)==0&&(selected[size]==L'\\'||!selected[size])){
            ++file_redirects;return *pair.second+std::wstring(selected+size);
        }
    }
    return {};
}
inline HANDLE WINAPI on_create_file_w(LPCWSTR path,DWORD access,DWORD sharing,LPSECURITY_ATTRIBUTES attributes,
                                      DWORD creation,DWORD flags,HANDLE model) {
    const auto redirected=file_path(path);
    return create_file_w(redirected.empty()?path:redirected.c_str(),access,sharing,attributes,creation,flags,model);
}
inline BOOL WINAPI on_create_directory_w(LPCWSTR path,LPSECURITY_ATTRIBUTES attributes) {
    const auto redirected=file_path(path);
    return create_directory_w(redirected.empty()?path:redirected.c_str(),attributes);
}
inline BOOL WINAPI on_delete_file_w(LPCWSTR path) {
    const auto redirected=file_path(path);
    return delete_file_w(redirected.empty()?path:redirected.c_str());
}
inline BOOL WINAPI on_move_file_ex_w(LPCWSTR before,LPCWSTR after,DWORD flags) {
    const auto a=file_path(before),b=file_path(after);
    return move_file_ex_w(a.empty()?before:a.c_str(),b.empty()?after:b.c_str(),flags);
}
inline DWORD WINAPI on_attributes_w(LPCWSTR path) {
    const auto redirected=file_path(path);
    return attributes_w(redirected.empty()?path:redirected.c_str());
}
inline BOOL WINAPI on_attributes_ex_w(LPCWSTR path,GET_FILEEX_INFO_LEVELS level,LPVOID info) {
    const auto redirected=file_path(path);
    return attributes_ex_w(redirected.empty()?path:redirected.c_str(),level,info);
}
inline BOOL WINAPI on_set_attributes_w(LPCWSTR path,DWORD flags) {
    const auto redirected=file_path(path);
    return set_attributes_w(redirected.empty()?path:redirected.c_str(),flags);
}
inline HANDLE WINAPI on_find_first_w(LPCWSTR path,FINDEX_INFO_LEVELS info,LPVOID data,
                                     FINDEX_SEARCH_OPS operation,LPVOID filter,DWORD flags) {
    const auto redirected=file_path(path);
    return find_first_w(redirected.empty()?path:redirected.c_str(),info,data,operation,filter,flags);
}
inline bool prepare() {
    id=environment(L"GEARSVR_INSTANCE_ID");root=environment(L"GEARSVR_INSTANCE_ROOT");
    if(id.empty()&&root.empty())return true;
    if(!valid_id(id)||root.size()<4||root.size()>180||root[1]!=L':'||root[2]!=L'\\')return false;
    const auto original=environment(L"GEARSVR_INSTANCE_ORIGINAL_LOCALAPPDATA");
    if(original.empty())return false;
    game_begin=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    const auto* dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(game_begin);
    if(dos->e_magic!=IMAGE_DOS_SIGNATURE||dos->e_lfanew<=0||dos->e_lfanew>65536)return false;
    const auto* nt=reinterpret_cast<const IMAGE_NT_HEADERS64*>(game_begin+dos->e_lfanew);
    if(nt->Signature!=IMAGE_NT_SIGNATURE||nt->OptionalHeader.SizeOfImage==0)return false;
    game_end=game_begin+nt->OptionalHeader.SizeOfImage;
    local=root+L"\\LocalAppData";roaming=root+L"\\RoamingAppData";documents=root+L"\\Documents";
    original_game_data=original+L"\\Coalition\\WarGame";game_data=local+L"\\Coalition\\WarGame";
    original_logs=environment(L"GEARSVR_INSTANCE_GAME_LOGS");logs=root+L"\\GameLogs";
    guardian=L"Guardian_UnrealEngine3_1.GearsVR."+id;
    for(const auto* path:{&local,&roaming,&documents,&game_data})
        if(GetFileAttributesW(path->c_str())==INVALID_FILE_ATTRIBUTES)return false;
    return true;
}
template<class F,class H> inline bool hook(HMODULE module,const char* name,H replacement,F& original) {
    auto target=GetProcAddress(module,name);
    if(!target&&module==GetModuleHandleW(L"kernelbase.dll"))target=GetProcAddress(GetModuleHandleW(L"kernel32.dll"),name);
    if(!target){failure=ERROR_PROC_NOT_FOUND;return false;}
    auto status=MH_CreateHook(reinterpret_cast<void*>(target),reinterpret_cast<void*>(replacement),
                              reinterpret_cast<void**>(&original));
    if(status!=MH_OK){failure=2000+unsigned(status);return false;}
    return true;
}
inline bool create_hooks() {
    if(id.empty())return true;
    HMODULE kernel=GetModuleHandleW(L"kernelbase.dll"),shell=LoadLibraryW(L"shell32.dll");
    return kernel&&shell&&
        hook(kernel,"CreateMutexW",on_create_mutex_w,create_mutex_w)&&
        hook(kernel,"CreateMutexA",on_create_mutex_a,create_mutex_a)&&
        hook(kernel,"OpenMutexW",on_open_mutex_w,open_mutex_w)&&
        hook(GetModuleHandleW(L"kernel32.dll"),"OpenMutexA",on_open_mutex_a,open_mutex_a)&&
        hook(shell,"SHGetFolderPathW",on_folder_w,folder_w)&&
        hook(shell,"SHGetFolderPathA",on_folder_a,folder_a)&&
        hook(shell,"SHGetKnownFolderPath",on_known_folder,known_folder)&&
        hook(kernel,"CreateFileW",on_create_file_w,create_file_w)&&
        hook(kernel,"CreateDirectoryW",on_create_directory_w,create_directory_w)&&
        hook(kernel,"DeleteFileW",on_delete_file_w,delete_file_w)&&
        hook(kernel,"MoveFileExW",on_move_file_ex_w,move_file_ex_w)&&
        hook(kernel,"GetFileAttributesW",on_attributes_w,attributes_w)&&
        hook(kernel,"GetFileAttributesExW",on_attributes_ex_w,attributes_ex_w)&&
        hook(kernel,"SetFileAttributesW",on_set_attributes_w,set_attributes_w)&&
        hook(kernel,"FindFirstFileExW",on_find_first_w,find_first_w);
}
}
