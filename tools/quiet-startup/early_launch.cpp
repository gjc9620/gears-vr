// Launch only a new owned child. Hold its EXE entry while installing the policy.
#include <Windows.h>
#include <TlHelp32.h>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include "quiet_startup.h"
#include "test_background_policy.h"
#include "command_line.h"

namespace fs=std::filesystem;
void require(bool ok,const char* operation) {
    if(!ok)throw std::runtime_error(std::string(operation)+" error="+std::to_string(GetLastError()));
}
struct OwnedChild {
    PROCESS_INFORMATION process{};bool attached{},pending{},released{};
    DEBUG_EVENT event{};
    ~OwnedChild() {
        if(process.hProcess && !released) {
            const BOOL killed=TerminateProcess(process.hProcess,125);
            if(pending)ContinueDebugEvent(event.dwProcessId,event.dwThreadId,DBG_CONTINUE);
            if(attached)DebugActiveProcessStop(process.dwProcessId);
            const DWORD waited=WaitForSingleObject(process.hProcess,5000);
            std::cerr<<"owned_child_cleanup pid="<<process.dwProcessId<<" termination_requested="
                     <<killed<<" exited="<<(waited==WAIT_OBJECT_0)<<"\n";
        }
        if(process.hThread)CloseHandle(process.hThread);
        if(process.hProcess)CloseHandle(process.hProcess);
    }
    void proceed(DWORD status) {
        require(ContinueDebugEvent(event.dwProcessId,event.dwThreadId,status)!=FALSE,"ContinueDebugEvent");pending=false;
    }
};
struct StartupAttributes {
    std::vector<unsigned char> storage;
    LPPROC_THREAD_ATTRIBUTE_LIST list{};
    DWORD64 mitigation{PROCESS_CREATION_MITIGATION_POLICY_EXTENSION_POINT_DISABLE_ALWAYS_ON};
    ~StartupAttributes(){if(list)DeleteProcThreadAttributeList(list);}
    void initialize(){
        SIZE_T size{};
        InitializeProcThreadAttributeList(nullptr,1,0,&size);
        require(size!=0,"Size startup attributes");storage.resize(size);
        auto* candidate=reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(storage.data());
        require(InitializeProcThreadAttributeList(candidate,1,0,&size)!=FALSE,"Initialize startup attributes");
        list=candidate;
        require(UpdateProcThreadAttribute(list,0,PROC_THREAD_ATTRIBUTE_MITIGATION_POLICY,
            &mitigation,sizeof(mitigation),nullptr,nullptr)!=FALSE,"Set extension point experiment");
    }
};
template<class T>T read(HANDLE process,uintptr_t address) {
    T value{};SIZE_T count{};
    require(ReadProcessMemory(process,reinterpret_cast<void*>(address),&value,sizeof(value),&count)&&count==sizeof(value),"ReadProcessMemory");
    return value;
}
void write_byte(HANDLE process,uintptr_t address,unsigned char value) {
    DWORD old{},restored{};SIZE_T count{};
    require(VirtualProtectEx(process,reinterpret_cast<void*>(address),1,PAGE_EXECUTE_READWRITE,&old)!=FALSE,"VirtualProtectEx");
    const BOOL written=WriteProcessMemory(process,reinterpret_cast<void*>(address),&value,1,&count);
    const BOOL protected_again=VirtualProtectEx(process,reinterpret_cast<void*>(address),1,old,&restored);
    require(written&&count==1&&protected_again,"Write/restore entry breakpoint");
    require(FlushInstructionCache(process,reinterpret_cast<void*>(address),1)!=FALSE,"FlushInstructionCache");
}
uintptr_t module_base(DWORD pid,const fs::path& path) {
    HANDLE snap=INVALID_HANDLE_VALUE;
    for(unsigned i=0;i<20;++i) {
        snap=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,pid);
        if(snap!=INVALID_HANDLE_VALUE || GetLastError()!=ERROR_BAD_LENGTH)break;
        Sleep(5);
    }
    require(snap!=INVALID_HANDLE_VALUE,"CreateToolhelp32Snapshot");
    MODULEENTRY32W row{};row.dwSize=sizeof(row);uintptr_t found{};
    for(BOOL ok=Module32FirstW(snap,&row);ok;ok=Module32NextW(snap,&row)) {
        std::error_code error;
        if(fs::equivalent(path,row.szExePath,error)&&!error){found=reinterpret_cast<uintptr_t>(row.modBaseAddr);break;}
    }
    CloseHandle(snap);require(found!=0,"Required remote module missing");return found;
}
DWORD call(HANDLE process,uintptr_t entry,void* argument) {
    HANDLE thread=CreateRemoteThread(process,nullptr,0,reinterpret_cast<LPTHREAD_START_ROUTINE>(entry),argument,0,nullptr);
    require(thread!=nullptr,"CreateRemoteThread");
    const DWORD waited=WaitForSingleObject(thread,10000);DWORD code{};
    const BOOL queried=waited==WAIT_OBJECT_0 && GetExitCodeThread(thread,&code);
    CloseHandle(thread);require(queried!=FALSE,"Remote call timeout/failure (new child will be terminated)");return code;
}
void install(OwnedChild& child,const fs::path& dll) {
    const auto loader=GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"LoadLibraryW");
    require(loader!=nullptr,"LoadLibraryW export");HMODULE owner{};
    require(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                              reinterpret_cast<LPCWSTR>(loader),&owner)!=FALSE,"Loader owner");
    wchar_t owner_path[32768]{};require(GetModuleFileNameW(owner,owner_path,32768)!=0,"Loader module path");
    const uintptr_t remote_loader=module_base(child.process.dwProcessId,owner_path)+
        reinterpret_cast<uintptr_t>(loader)-reinterpret_cast<uintptr_t>(owner);
    const auto path=dll.wstring();const SIZE_T size=(path.size()+1)*sizeof(wchar_t);
    void* argument=VirtualAllocEx(child.process.hProcess,nullptr,size,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    require(argument!=nullptr,"Allocate DLL path");SIZE_T written{};
    require(WriteProcessMemory(child.process.hProcess,argument,path.c_str(),size,&written)&&written==size,"Write DLL path");
    call(child.process.hProcess,remote_loader,argument); // DWORD result is NOT a 64-bit HMODULE.
    require(VirtualFreeEx(child.process.hProcess,argument,0,MEM_RELEASE)!=FALSE,"Release DLL path");
    const uintptr_t remote_dll=module_base(child.process.dwProcessId,dll);
    HMODULE local=LoadLibraryExW(dll.c_str(),nullptr,DONT_RESOLVE_DLL_REFERENCES);
    require(local!=nullptr,"Read policy exports");
    auto entry=GetProcAddress(local,"GearsQuietInstall");
    auto query=GetProcAddress(local,"GearsQuietStatus");
    const uintptr_t install_rva=reinterpret_cast<uintptr_t>(entry)-reinterpret_cast<uintptr_t>(local);
    const uintptr_t query_rva=reinterpret_cast<uintptr_t>(query)-reinterpret_cast<uintptr_t>(local);
    FreeLibrary(local);require(entry&&query,"Required policy exports");
    require(call(child.process.hProcess,remote_dll+install_rva,nullptr)==ERROR_SUCCESS,"Policy install rejected");
    void* result=VirtualAllocEx(child.process.hProcess,nullptr,sizeof(gears_quiet::Status),MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    require(result!=nullptr,"Allocate policy status");
    require(call(child.process.hProcess,remote_dll+query_rva,result)==ERROR_SUCCESS,"Policy query rejected");
    const auto status=read<gears_quiet::Status>(child.process.hProcess,reinterpret_cast<uintptr_t>(result));
    require(VirtualFreeEx(child.process.hProcess,result,0,MEM_RELEASE)!=FALSE,"Release policy status");
    require(status.version==gears_quiet::abi && status.installed==1,"Policy readiness missing");
}
struct VisibleWindows { DWORD pid{};unsigned count{}; };
BOOL CALLBACK count_visible(HWND window,LPARAM argument) {
    auto& state=*reinterpret_cast<VisibleWindows*>(argument);DWORD pid{};
    GetWindowThreadProcessId(window,&pid);
    if(pid==state.pid && IsWindowVisible(window))++state.count;
    return TRUE;
}
int wmain(int argc,wchar_t** argv) {
    try {
        if(argc<3)throw std::runtime_error("Usage: GearsEarlyLaunch.exe EXE policy.dll [game arguments]");
        const fs::path exe=fs::canonical(argv[1]),dll=fs::canonical(argv[2]);
        std::wstring command=gears_quiet::quote_argument(exe.wstring());
        for(int i=3;i<argc;++i)command+=L" "+gears_quiet::quote_argument(argv[i]);
        wchar_t experiment[2]{};
        const bool disable_extensions=GetEnvironmentVariableW(L"GEARSVR_QUIET_DISABLE_EXTENSION_POINTS",experiment,2)==1&&experiment[0]==L'1';
        StartupAttributes attributes;STARTUPINFOEXW start{};
        start.StartupInfo.cb=disable_extensions?sizeof(start):sizeof(STARTUPINFOW);
        // Diagnostic single-variable control: the game's first ShowWindow asks
        // to minimize. STARTF_USESHOWWINDOW otherwise overrides that request.
        // Preserve the shipped default until actual-game evidence validates it.
        wchar_t minimized_option[2]{};
        const bool start_minimized=GetEnvironmentVariableW(L"GEARSVR_QUIET_START_MINIMIZED",minimized_option,2)==1&&minimized_option[0]==L'1';
        start.StartupInfo.dwFlags=STARTF_USESHOWWINDOW;
        wchar_t behind_option[2]{};
        const bool test_behind=GetEnvironmentVariableW(L"GEARSVR_TEST_WINDOWS_BEHIND",behind_option,2)==1&&behind_option[0]==L'1';
        start.StartupInfo.wShowWindow=static_cast<WORD>(gears_quiet_test::startup_show(start_minimized,test_behind));
        if(disable_extensions){attributes.initialize();start.lpAttributeList=attributes.list;}
        OwnedChild child;
        require(CreateProcessW(exe.c_str(),command.data(),nullptr,nullptr,FALSE,
                              DEBUG_ONLY_THIS_PROCESS|CREATE_UNICODE_ENVIRONMENT|(disable_extensions?EXTENDED_STARTUPINFO_PRESENT:0),
                              nullptr,exe.parent_path().c_str(),&start.StartupInfo,&child.process)!=FALSE,"Create owned child");
        child.attached=true;
        PROCESS_MITIGATION_EXTENSION_POINT_DISABLE_POLICY extension_policy{};
        require(GetProcessMitigationPolicy(child.process.hProcess,ProcessExtensionPointDisablePolicy,
            &extension_policy,sizeof(extension_policy))!=FALSE,"Read extension point policy");
        require(!disable_extensions||extension_policy.DisableExtensionPoints,"Extension point experiment not applied");
        // Keep debugger-exit termination until detach: no abandoned int3 child.
        require(DebugSetProcessKillOnExit(TRUE)!=FALSE,"Debug kill-on-exit");
        uintptr_t entry{};unsigned char original{};bool patched{},initial_break{};
        uintptr_t remote_ntdll{};DWORD ntdll_size{};
        const ULONGLONG deadline=GetTickCount64()+15000;
        while(GetTickCount64()<deadline) {
            if(!WaitForDebugEvent(&child.event,100)) {
                require(GetLastError()==ERROR_SEM_TIMEOUT,"WaitForDebugEvent");continue;
            }
            child.pending=true;auto& event=child.event;DWORD disposition=DBG_CONTINUE;
            require(event.dwProcessId==child.process.dwProcessId,"Unexpected debug process");
            if(event.dwDebugEventCode==CREATE_PROCESS_DEBUG_EVENT) {
                if(event.u.CreateProcessInfo.hFile)CloseHandle(event.u.CreateProcessInfo.hFile);
                const auto base=reinterpret_cast<uintptr_t>(event.u.CreateProcessInfo.lpBaseOfImage);
                auto dos=read<IMAGE_DOS_HEADER>(child.process.hProcess,base);
                require(dos.e_magic==IMAGE_DOS_SIGNATURE && dos.e_lfanew>0 && dos.e_lfanew<65536,"DOS header");
                auto nt=read<IMAGE_NT_HEADERS64>(child.process.hProcess,base+dos.e_lfanew);
                require(nt.Signature==IMAGE_NT_SIGNATURE && nt.OptionalHeader.Magic==IMAGE_NT_OPTIONAL_HDR64_MAGIC &&
                        nt.OptionalHeader.AddressOfEntryPoint<nt.OptionalHeader.SizeOfImage,"PE64 entry");
                entry=base+nt.OptionalHeader.AddressOfEntryPoint;
                original=read<unsigned char>(child.process.hProcess,entry);
                require(original!=0xcc,"Entry already has a breakpoint");
                write_byte(child.process.hProcess,entry,0xcc);patched=true;
            } else if(event.dwDebugEventCode==LOAD_DLL_DEBUG_EVENT) {
                if(event.u.LoadDll.hFile)CloseHandle(event.u.LoadDll.hFile);
            } else if(event.dwDebugEventCode==EXCEPTION_DEBUG_EVENT) {
                const auto& exception=event.u.Exception;
                const auto exception_address=reinterpret_cast<uintptr_t>(exception.ExceptionRecord.ExceptionAddress);
                if(exception.ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT &&
                   exception_address==entry && patched) {
                    require(event.dwThreadId==child.process.dwThreadId,"Entry on unexpected thread");
                    require(exception.dwFirstChance!=0 && initial_break,"Unexpected entry breakpoint sequence");
                    CONTEXT context{};context.ContextFlags=CONTEXT_CONTROL;
                    require(GetThreadContext(child.process.hThread,&context)!=FALSE && context.Rip==entry+1,"Entry context");
                    require(read<unsigned char>(child.process.hProcess,entry)==0xcc,"Entry breakpoint ownership");
                    write_byte(child.process.hProcess,entry,original);patched=false;context.Rip=entry;
                    require(SetThreadContext(child.process.hThread,&context)!=FALSE,"Restore entry RIP");
                    require(SuspendThread(child.process.hThread)==0,"Hold original entry thread");
                    child.proceed(DBG_CONTINUE);
                    require(DebugActiveProcessStop(child.process.dwProcessId)!=FALSE,"Detach debugger");child.attached=false;
                    VisibleWindows visible{child.process.dwProcessId};
                    require(EnumWindows(count_visible,reinterpret_cast<LPARAM>(&visible))!=FALSE,"Inspect pre-entry windows");
                    require(visible.count==0,"Visible window created before EXE entry; quiet startup too late");
                    install(child,dll);
                    require(read<unsigned char>(child.process.hProcess,entry)==original,"Original entry restored");
                    FILETIME created{},exited{},kernel{},user{};
                    require(GetProcessTimes(child.process.hProcess,&created,&exited,&kernel,&user)!=FALSE,"Process creation time");
                    const auto stamp=(static_cast<unsigned long long>(created.dwHighDateTime)<<32)|created.dwLowDateTime;
                    require(ResumeThread(child.process.hThread)==1,"Release original entry thread");child.released=true;
                    std::cout<<"{\"pid\":"<<child.process.dwProcessId<<",\"creation_time\":"<<stamp
                             <<",\"pre_entry_visible_windows\":0,\"entry_restored\":true,\"debugger_detached\":true,\"quiet_policy_installed\":true"
                             <<",\"extension_point_disable_requested\":"<<(disable_extensions?"true":"false")
                             <<",\"extension_points_disabled\":"<<(extension_policy.DisableExtensionPoints?"true":"false")
                             <<",\"extension_point_policy_flags\":"<<extension_policy.Flags<<"}\n";
                    return 0;
                }
                if(exception.ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT && !initial_break &&
                   event.dwThreadId==child.process.dwThreadId && exception.dwFirstChance) {
                    wchar_t ntdll_path[32768]{};
                    require(GetModuleFileNameW(GetModuleHandleW(L"ntdll.dll"),ntdll_path,32768)!=0,"System breakpoint module path");
                    remote_ntdll=module_base(child.process.dwProcessId,ntdll_path);
                    const auto dos=read<IMAGE_DOS_HEADER>(child.process.hProcess,remote_ntdll);
                    require(dos.e_magic==IMAGE_DOS_SIGNATURE && dos.e_lfanew>0 && dos.e_lfanew<65536,"ntdll DOS header");
                    const auto nt=read<IMAGE_NT_HEADERS64>(child.process.hProcess,remote_ntdll+dos.e_lfanew);
                    require(nt.Signature==IMAGE_NT_SIGNATURE && nt.OptionalHeader.Magic==IMAGE_NT_OPTIONAL_HDR64_MAGIC,"ntdll PE header");
                    ntdll_size=nt.OptionalHeader.SizeOfImage;
                    // Do not swallow a TLS/packer/application breakpoint.
                    if(exception_address>=remote_ntdll && exception_address-remote_ntdll<ntdll_size &&
                       read<unsigned char>(child.process.hProcess,exception_address)==0xcc)initial_break=true;
                    else disposition=DBG_EXCEPTION_NOT_HANDLED;
                } else disposition=DBG_EXCEPTION_NOT_HANDLED;
            } else if(event.dwDebugEventCode==EXIT_PROCESS_DEBUG_EVENT) {
                child.proceed(DBG_CONTINUE);child.attached=false;
                throw std::runtime_error("Child exited before entry policy installation");
            }
            child.proceed(disposition);
        }
        throw std::runtime_error("Entry breakpoint deadline; no child released");
    } catch(const std::exception& error) {std::cerr<<error.what()<<"\n";return 1;}
}
