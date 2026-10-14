#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <dbghelp.h>
#include <array>
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <limits>
#include <string>
#include <vector>
#include <stdexcept>

// Win64, one caller/DbgHelp thread. Frozen stack bytes are never scanned for
// addresses. Unwind tables/code outside the stack are read LIVE after Resume.
namespace {
constexpr unsigned metadata_read_cap=1024,metadata_byte_cap=2*1024*1024;
struct Handle { HANDLE h{}; explicit Handle(HANDLE value=nullptr):h(value){}
 ~Handle(){if(h&&h!=INVALID_HANDLE_VALUE)CloseHandle(h);} Handle(const Handle&)=delete;Handle& operator=(const Handle&)=delete;};
uint64_t qpc(){LARGE_INTEGER t{};QueryPerformanceCounter(&t);return uint64_t(t.QuadPart);}
uint64_t creation(HANDLE p){FILETIME c{},e{},k{},u{};if(!GetProcessTimes(p,&c,&e,&k,&u))throw std::runtime_error("GetProcessTimes");return (uint64_t(c.dwHighDateTime)<<32)|c.dwLowDateTime;}
uint64_t number(const wchar_t* s,uint64_t limit){uint64_t n{};if(!s||!*s)throw std::runtime_error("empty number");
 for(;*s;++s){if(*s<L'0'||*s>L'9'||n>(limit-unsigned(*s-L'0'))/10)throw std::runtime_error("invalid decimal number");n=n*10+unsigned(*s-L'0');}if(!n)throw std::runtime_error("zero identity");return n;}
std::string quote(const std::string& s){std::string out="\"";for(unsigned char c:s){if(c=='\\'||c=='\"'){out+='\\';out+=char(c);}else if(c<32){char b[7];sprintf_s(b,"\\u%04x",c);out+=b;}else out+=char(c);}return out+'\"';}
struct Target {Handle process,thread;DWORD pid,tid;uint64_t born;
 Target(DWORD p,uint64_t c,DWORD t):process(OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,FALSE,p)),
  thread(OpenThread(THREAD_QUERY_INFORMATION|THREAD_GET_CONTEXT|THREAD_SUSPEND_RESUME,FALSE,t)),pid(p),tid(t),born(c){
  if(!process.h||!thread.h||creation(process.h)!=c||GetProcessIdOfThread(thread.h)!=p||
     (p==GetCurrentProcessId()&&t==GetCurrentThreadId()))throw std::runtime_error("process/thread identity rejected (or sampler thread)");}
 bool alive(){DWORD p{},t{};return GetExitCodeProcess(process.h,&p)&&GetExitCodeThread(thread.h,&t)&&p==STILL_ACTIVE&&t==STILL_ACTIVE;}
};
struct Snapshot {
 CONTEXT context{};std::array<unsigned char,65536> stack{};SIZE_T bytes{};
 uint64_t begin{},suspended{},resumed{},end{},stack_low{},stack_high{};
 DWORD suspend_previous{DWORD(-1)},resume_previous{DWORD(-1)},error{};bool suspend_ok{},resume_ok{},truncated{};
 unsigned metadata_reads{},metadata_bytes{},unwind_error{};const char* unwind_reason{"none"};
};
struct Suspension {
 HANDLE thread;Snapshot& s;bool owned{};
 Suspension(HANDLE t,Snapshot& v):thread(t),s(v){s.suspend_previous=SuspendThread(t);owned=s.suspend_previous!=DWORD(-1);s.suspend_ok=owned;s.suspended=qpc();if(!owned)s.error=GetLastError();}
 void resume(){if(!owned)return;s.resume_previous=ResumeThread(thread);s.resumed=qpc();
  if(s.resume_previous==DWORD(-1)){s.error=GetLastError();return;}owned=false;s.resume_ok=s.resume_previous==s.suspend_previous+1;
  // A concurrent suspender/resumer violates exclusive observation. Never issue
  // extra resumes to force a count to zero; stop the run instead.
 }
 ~Suspension(){if(owned)resume();}
};
Snapshot snap(Target& target){Snapshot s;s.begin=qpc();
 {Suspension suspended(target.thread.h,s);
  if(suspended.owned&&s.suspend_previous==0){
   s.context.ContextFlags=CONTEXT_FULL;
   if(!GetThreadContext(target.thread.h,&s.context))s.error=GetLastError();
   else {MEMORY_BASIC_INFORMATION m{};const auto sp=s.context.Rsp;
    if(!VirtualQueryEx(target.process.h,reinterpret_cast<void*>(sp),&m,sizeof(m))||m.State!=MEM_COMMIT||
       (m.Protect&(PAGE_GUARD|PAGE_NOACCESS)))s.error=ERROR_INVALID_ADDRESS;
    else {s.stack_low=reinterpret_cast<uint64_t>(m.AllocationBase);s.stack_high=reinterpret_cast<uint64_t>(m.BaseAddress)+m.RegionSize;
     SIZE_T count=static_cast<SIZE_T>((std::min)(uint64_t(s.stack.size()),s.stack_high-sp));SIZE_T got{};
     if(!count||!ReadProcessMemory(target.process.h,reinterpret_cast<void*>(sp),s.stack.data(),count,&got)||got!=count)s.error=GetLastError()?GetLastError():ERROR_PARTIAL_COPY;
     else {s.bytes=got;s.truncated=count==s.stack.size();if(s.truncated)s.unwind_reason="stack_snapshot_cap";}
    }
   }
  }
  suspended.resume(); // Resume before ANY DbgHelp, string allocation or I/O.
 }
 s.end=qpc();return s;
}
struct Frozen {Target* target;Snapshot* s;};Frozen* frozen{};
BOOL CALLBACK read_memory(HANDLE,DWORD64 address,PVOID output,DWORD size,LPDWORD actual){
 *actual=0;if(!frozen||!size)return FALSE;auto& s=*frozen->s;
 if(address>UINT64_MAX-size)return FALSE;const auto end=address+size,sp=s.context.Rsp;
 if(address>=sp&&end<=sp+s.bytes){memcpy(output,s.stack.data()+size_t(address-sp),size);*actual=size;return TRUE;}
 // Reject all other reads overlapping the captured stack allocation, including
 // beyond the 64 KiB cap: a live stack read would corrupt snapshot provenance.
 if(address<s.stack_high&&end>s.stack_low){s.truncated=true;s.unwind_error=ERROR_PARTIAL_COPY;s.unwind_reason="uncaptured_stack";return FALSE;}
 if(size>65536||s.metadata_reads>=metadata_read_cap||size>metadata_byte_cap-s.metadata_bytes){s.truncated=true;s.unwind_error=ERROR_INSUFFICIENT_BUFFER;s.unwind_reason="metadata_budget";return FALSE;}
 ++s.metadata_reads;s.metadata_bytes+=size;SIZE_T got{};
 if(!ReadProcessMemory(frozen->target->process.h,reinterpret_cast<void*>(address),output,size,&got)||got!=size){s.truncated=true;s.unwind_error=GetLastError();s.unwind_reason="live_metadata_read_failed";return FALSE;}
 *actual=size;return TRUE;
}
struct Frame {uint64_t pc{},sp{},base{},rva{},symbol_displacement{};std::string module,symbol;};
Frame frame(HANDLE process,uint64_t pc,uint64_t sp){Frame f;f.pc=pc;f.sp=sp;
 IMAGEHLP_MODULE64 m{};m.SizeOfStruct=sizeof(m);
 if(SymGetModuleInfo64(process,pc,&m)){f.base=m.BaseOfImage;f.rva=pc-f.base;f.module=m.ImageName[0]?m.ImageName:m.ModuleName;}
 alignas(SYMBOL_INFO) std::array<unsigned char,sizeof(SYMBOL_INFO)+512> b{};auto* symbol=reinterpret_cast<SYMBOL_INFO*>(b.data());symbol->SizeOfStruct=sizeof(SYMBOL_INFO);symbol->MaxNameLen=511;DWORD64 displacement{};
 if(SymFromAddr(process,pc,&displacement,symbol)){f.symbol=symbol->Name;f.symbol_displacement=displacement;}return f;}
std::vector<Frame> unwind(Target& target,Snapshot& s){std::vector<Frame> out;out.reserve(32);
 CONTEXT context=s.context;STACKFRAME64 f{};
 f.AddrPC.Offset=context.Rip;f.AddrPC.Mode=AddrModeFlat;
 f.AddrStack.Offset=context.Rsp;f.AddrStack.Mode=AddrModeFlat;
 f.AddrFrame.Offset=context.Rbp;f.AddrFrame.Mode=AddrModeFlat;
 out.push_back(frame(target.process.h,context.Rip,context.Rsp));Frozen state{&target,&s};frozen=&state;
 for(unsigned i=1;i<=32&&out.size()<32;++i){SetLastError(0);
  if(!StackWalk64(IMAGE_FILE_MACHINE_AMD64,target.process.h,target.thread.h,&f,&context,read_memory,SymFunctionTableAccess64,SymGetModuleBase64,nullptr)){
   if(!s.unwind_error){s.unwind_error=GetLastError();s.unwind_reason="stackwalk_returned_false";s.truncated=true;}break;}
  if(!f.AddrPC.Offset)break;
  if(f.AddrPC.Offset==out.back().pc&&f.AddrStack.Offset==out.back().sp){if(i==1)continue;s.unwind_error=ERROR_INVALID_DATA;s.unwind_reason="repeated_frame";s.truncated=true;break;}
  out.push_back(frame(target.process.h,f.AddrPC.Offset,f.AddrStack.Offset));
 }
 if(out.size()==32){s.truncated=true;s.unwind_reason="frame_depth_cap";}frozen=nullptr;return out;
}
void write(HANDLE output,const std::string& row){DWORD got{};if(row.size()>MAXDWORD||!WriteFile(output,row.data(),DWORD(row.size()),&got,nullptr)||got!=row.size())throw std::runtime_error("JSONL write failed");}
struct Symbols {HANDLE process;explicit Symbols(HANDLE p):process(p){
 SymSetOptions(SYMOPT_DEFERRED_LOADS|SYMOPT_FAIL_CRITICAL_ERRORS|SYMOPT_NO_PROMPTS|SYMOPT_IGNORE_NT_SYMPATH|SYMOPT_IGNORE_CVREC);
 if(!SymInitialize(process,".",TRUE))throw std::runtime_error("SymInitialize failed");}
 ~Symbols(){SymCleanup(process);}};
struct Outcome {unsigned samples{},good{},sleep_frames{},kernelbase_sleep{};bool restored{true},success{true};const char* reason{"duration_limit"};};
Outcome run(Target& target,HANDLE output,unsigned duration){LARGE_INTEGER freq{};QueryPerformanceFrequency(&freq);Symbols symbols(target.process.h);
 write(output,"{\"kind\":\"start\",\"pid\":"+std::to_string(target.pid)+",\"creation_filetime\":"+std::to_string(target.born)+",\"tid\":"+std::to_string(target.tid)+",\"qpc_frequency\":"+std::to_string(freq.QuadPart)+",\"max_samples\":400,\"stack_bytes_cap\":65536,\"max_pause_ms\":20,\"metadata\":\"live outside frozen stack; no network symbols\"}\n");
 write(output,"{\"kind\":\"unwind_budget\",\"metadata_reads\":"+std::to_string(metadata_read_cap)+",\"metadata_bytes\":"+std::to_string(metadata_byte_cap)+"}\n");
 Outcome result;const auto deadline=GetTickCount64()+duration;
 while(result.samples<400&&GetTickCount64()<deadline&&target.alive()){
  const auto slot=GetTickCount64();auto s=snap(target);++result.samples;
  const bool slow=s.resume_ok&&s.resumed-s.begin>uint64_t(freq.QuadPart)/50;
  std::vector<Frame> frames;if(s.resume_ok&&!s.error&&s.bytes&&s.suspend_previous==0&&!slow){frames=unwind(target,s);++result.good;}
  s.end=qpc();std::string row="{\"kind\":\"sample\",\"index\":"+std::to_string(result.samples)+",\"begin_qpc\":"+std::to_string(s.begin)+",\"suspended_qpc\":"+std::to_string(s.suspended)+",\"resumed_qpc\":"+std::to_string(s.resumed)+",\"end_qpc\":"+std::to_string(s.end)+",\"rip\":"+std::to_string(s.context.Rip)+",\"rsp\":"+std::to_string(s.context.Rsp)+",\"rbp\":"+std::to_string(s.context.Rbp)+",\"eflags\":"+std::to_string(s.context.EFlags)+",\"stack_bytes\":"+std::to_string(s.bytes)+",\"suspend_previous\":"+std::to_string(s.suspend_previous)+",\"resume_previous\":"+std::to_string(s.resume_previous)+",\"restored\":"+(s.resume_ok?"true":"false")+",\"pause_over_budget\":"+(slow?"true":"false")+",\"truncated\":"+(s.truncated?"true":"false")+",\"error\":"+std::to_string(s.error)+",\"unwind_error\":"+std::to_string(s.unwind_error)+",\"metadata_reads\":"+std::to_string(s.metadata_reads)+",\"frames\":[";
  for(size_t i=0;i<frames.size();++i){const auto& f=frames[i];if(i)row+=',';
   row+="{\"pc\":"+std::to_string(f.pc)+",\"sp\":"+std::to_string(f.sp)+",\"module\":"+quote(f.module)+",\"base\":"+std::to_string(f.base)+",\"rva\":"+std::to_string(f.rva)+",\"symbol\":"+quote(f.symbol)+",\"symbol_displacement\":"+std::to_string(f.symbol_displacement)+"}";
   if(f.symbol.find("Sleep")!=std::string::npos||f.symbol.find("DelayExecution")!=std::string::npos)++result.sleep_frames;
   if(f.symbol.find("Sleep")!=std::string::npos&&(f.module.find("KERNELBASE")!=std::string::npos||f.module.find("KernelBase")!=std::string::npos))++result.kernelbase_sleep;
  }
  write(output,row+"],\"unwind_reason\":"+quote(s.unwind_reason)+"}\n");
  if(!s.resume_ok||s.suspend_previous!=0||slow||s.error){result.restored=s.resume_ok;result.success=false;
   result.reason=!s.suspend_ok?"suspend_failed":!s.resume_ok?"resume_failed_or_count_changed":s.suspend_previous?"preexisting_suspend":slow?"pause_budget_exceeded":"snapshot_failed";break;}
  const auto elapsed=GetTickCount64()-slot;if(elapsed<20)Sleep(DWORD(20-elapsed));
 }
 if(result.success&&!target.alive()){result.success=false;result.reason="process_or_thread_exited";}
 if(result.success&&result.samples==400)result.reason="sample_limit";
 write(output,"{\"kind\":\"end\",\"samples\":"+std::to_string(result.samples)+",\"good\":"+std::to_string(result.good)+",\"sleep_frames\":"+std::to_string(result.sleep_frames)+",\"kernelbase_sleep\":"+std::to_string(result.kernelbase_sleep)+",\"restored\":"+(result.restored?"true":"false")+",\"success\":"+(result.success?"true":"false")+",\"reason\":"+quote(result.reason)+"}\n");return result;
}
std::atomic<bool> test_stop{};std::atomic<unsigned> beats{};
DWORD WINAPI sleeper(void*){while(!test_stop.load()){++beats;Sleep(40);}return 0;}
}
int wmain(int argc,wchar_t** argv){
 try {if(argc==3&&!wcscmp(argv[1],L"--self-test")){
  Handle output(CreateFileW(argv[2],GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr));if(output.h==INVALID_HANDLE_VALUE)throw std::runtime_error("output create failed");
  DWORD tid{};Handle thread(CreateThread(nullptr,0,sleeper,nullptr,0,&tid));if(!thread.h)throw std::runtime_error("Sleep thread failed");
  Sleep(10);Target target(GetCurrentProcessId(),creation(GetCurrentProcess()),tid);bool rejected{};
  try{Target wrong(target.pid,target.born+1,tid);}catch(...){rejected=true;}
  write(output.h,std::string("{\"kind\":\"identity_test\",\"wrong_creation_rejected\":")+(rejected?"true":"false")+"}\n");
  auto result=run(target,output.h,1000);
  // An existing suspension must remain exactly one after the core restores its
  // own added count. This outer test suspension is then removed separately.
  const auto prior=SuspendThread(thread.h);const auto blocked=run(target,output.h,60);
  const auto remaining=ResumeThread(thread.h);
  const bool preserved=prior==0&&blocked.samples==1&&blocked.restored&&!blocked.success&&remaining==1;
  write(output.h,std::string("{\"kind\":\"preexisting_suspend_test\",\"preserved\":")+(preserved?"true":"false")+"}\n");
  const auto before=beats.load();Sleep(80);const bool progresses=beats.load()>before;
  test_stop=true;const bool finished=WaitForSingleObject(thread.h,1000)==WAIT_OBJECT_0;
  const bool pass=rejected&&result.samples>=20&&result.good>=20&&result.kernelbase_sleep&&result.restored&&result.success&&preserved&&progresses&&finished;
  write(output.h,std::string("{\"kind\":\"self_test\",\"progresses\":")+(progresses?"true":"false")+",\"thread_exited\":"+(finished?"true":"false")+",\"passed\":"+(pass?"true":"false")+"}\n");
  std::printf("Sleep sampling self-test: %s, samples=%u good=%u sleep_frames=%u\n",pass?"PASS":"FAIL",result.samples,result.good,result.sleep_frames);return pass?0:1;
 }
 if(argc!=5)throw std::runtime_error("Usage: thread_wait_sample PID CREATION_FILETIME TID OUTPUT.jsonl | --self-test OUTPUT.jsonl");
 Target target(DWORD(number(argv[1],MAXDWORD)),number(argv[2],UINT64_MAX),DWORD(number(argv[3],MAXDWORD)));
 Handle output(CreateFileW(argv[4],GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr));if(output.h==INVALID_HANDLE_VALUE)throw std::runtime_error("output create failed");
 const auto result=run(target,output.h,8000);return result.good&&result.restored&&result.success?0:1;
 }catch(const std::exception& e){std::fprintf(stderr,"ERROR: %s\n",e.what());return 2;}
}
