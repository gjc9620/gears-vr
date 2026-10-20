#pragma once
#include "graphics_lock.h"
#include "graphics_menu_lock.h"
#if defined(GEARS_GRAPHICS_RUNTIME_LOCK) && GEARS_GRAPHICS_RUNTIME_LOCK
#if !defined(GEARS_GRAPHICS_LOCK_OBSERVER) || !GEARS_GRAPHICS_LOCK_OBSERVER
#error GEARS_GRAPHICS_RUNTIME_LOCK requires GEARS_GRAPHICS_LOCK_OBSERVER
#endif
#include "graphics_runtime_contract.h"
bool hash_matches();
#endif

// Reserve creates inactive trampolines. Startup policy, when selected, is owned
// by the target-prime watchdog until the continuous worker accepts handoff.
bool install(void*,void*,void**);
#if defined(GEARS_GRAPHICS_LOCK_OBSERVER) && GEARS_GRAPHICS_LOCK_OBSERVER
namespace graphics_observer {
using Menu=uint32_t (__fastcall*)(void*,int32_t);
using Collector=uint64_t (__fastcall*)(void*); // Raw machine RAX; no recovered C++ return claim.
using Commit=uint32_t (__fastcall*)(void*);
Menu menu_original{};Collector collector_original{};Commit commit_original{};
#if defined(GEARS_GRAPHICS_MENU_LOCK) && GEARS_GRAPHICS_MENU_LOCK
inline constexpr unsigned target_count=4;
#else
inline constexpr unsigned target_count=3;
#endif
std::array<void*,target_count> targets{};bool reserved{};
gears_graphics_lock::Entry const& target_entry(unsigned i){
 static const gears_graphics_lock::Entry selection{gears_graphics_menu::set_index_rva,gears_graphics_menu::entry_bytes};
 return i<3?gears_graphics_lock::entries[i]:selection;
}
thread_local unsigned depth{};thread_local uintptr_t current_settings{};
struct Fields {uintptr_t vtable{};uint32_t normal{},alternate{},queueflag{},selector_thread{},mask{};};
struct Row {DWORD thread{};uintptr_t caller{},settings{},collector_settings{},command{},command_vtable{};
 unsigned depth{},kind{},read_mask{};int32_t mode{};uint32_t command_dlss{};uint64_t result{};
 long long qpc{};Fields before{},after{};unsigned stack_count{};std::array<void*,8> stack{};
 std::array<unsigned char,32> caller_before{},caller_after{};};
std::array<std::array<Row,32>,3> rows{};
std::array<std::atomic<unsigned>,3> counts{};
std::array<std::atomic<uint64_t>,3> calls{};
#if defined(GEARS_GRAPHICS_RUNTIME_LOCK) && GEARS_GRAPHICS_RUNTIME_LOCK
namespace runtime {
using Failure=gears_graphics_runtime::Failure;
gears_graphics_lock::routes::Context context{};
std::atomic<bool> armed{};
std::atomic<bool> startup{},session_owned{};
std::atomic<uint64_t> menu_blocked{},normal_cleared{},command_cleared{},faults{};
struct Fault {Failure reason{};unsigned policy_reason{},route_status{};DWORD thread{};uintptr_t caller{},window{};};
Fault first_fault{};std::atomic<unsigned> fault_phase{};
struct Read {bool operator()(uintptr_t address,void* out,size_t size){return safe_copy(out,reinterpret_cast<void*>(address),size);}};
bool writable4(uintptr_t address){MEMORY_BASIC_INFORMATION region{};
 if(!address||address%4||VirtualQuery(reinterpret_cast<void*>(address),&region,sizeof(region))!=sizeof(region)||
    region.State!=MEM_COMMIT||(region.Protect&(PAGE_GUARD|PAGE_NOACCESS)))return false;
 const DWORD p=region.Protect&0xff;
 if(p!=PAGE_READWRITE&&p!=PAGE_WRITECOPY&&p!=PAGE_EXECUTE_READWRITE&&p!=PAGE_EXECUTE_WRITECOPY)return false;
 const auto start=reinterpret_cast<uintptr_t>(region.BaseAddress);
 return address>=start&&region.RegionSize>=4&&address-start<=region.RegionSize-4;
}
// The writer accepts one exact policy location; executable/page protection is
// never changed. The engine's original calls still own resource transitions.
struct Cas {uintptr_t allowed{};
 bool operator()(uintptr_t address,uint32_t expected,uint32_t replacement){
  if(address!=allowed||expected!=1||replacement||!writable4(address))return false;
  __try{return InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(address),0,1)==1;}
  __except(EXCEPTION_EXECUTE_HANDLER){return false;}}
};
bool cas_self_test(){alignas(4) uint32_t values[3]{77,1,88};Cas cas{reinterpret_cast<uintptr_t>(&values[1])};
 bool valid=!cas(reinterpret_cast<uintptr_t>(&values[0]),1,0)&&!cas(cas.allowed,2,0)&&
  cas(cas.allowed,1,0)&&values[0]==77&&values[1]==0&&values[2]==88&&!cas(cas.allowed,1,0);
 void* readonly=VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_READONLY);
 if(!readonly)return false;Cas rejected{reinterpret_cast<uintptr_t>(readonly)};
 valid=valid&&!writable4(rejected.allowed)&&!rejected(rejected.allowed,1,0);
 VirtualFree(readonly,0,MEM_RELEASE);return valid;}
void fault(Failure reason,uintptr_t caller=0,uintptr_t window=0,unsigned policy=0,unsigned status=0){
 ++faults;unsigned expected=0;
 if(fault_phase.compare_exchange_strong(expected,1)){first_fault={reason,policy,status,GetCurrentThreadId(),caller,window};fault_phase.store(2,std::memory_order_release);}
 if(startup.load(std::memory_order_acquire)&&!session_owned.load()){
  if(prime_event)SetEvent(prime_event);
 }else request_session_stop(SessionStop::proof_failure);
}
gears_graphics_lock::routes::Call call(uintptr_t caller){return {GetCurrentThreadId(),caller>=base?caller-base:0};}
void decision(gears_graphics_lock::routes::Decision const& d,uintptr_t caller){
 if(d.status!=gears_graphics_lock::routes::Status::admitted&&d.status!=gears_graphics_lock::routes::Status::other_mode)
  fault(Failure::route,caller,0,unsigned(d.result.reason),unsigned(d.status));
}
bool admit(){Read read;context={};
 context.policy.admission=gears_graphics_lock::admit(base,hash_matches(),read);
 if(!context.policy.admission.valid()){fault(Failure::admission,0,0,unsigned(context.policy.admission.reason));return false;}
 uintptr_t failed{};if(!gears_graphics_runtime::bytes(base,read,failed)){fault(Failure::dispatcher_bytes,0,failed);return false;}
 uint32_t normal{},alternate{};
 if(!gears_graphics_lock::read_value(read,context.policy.admission.singleton+0x3c8,normal)||
    !gears_graphics_lock::read_value(read,context.policy.admission.singleton+0x56c,alternate)||normal||alternate){fault(Failure::initial_state);return false;}
 if(!writable4(context.policy.admission.singleton+0x3c8)){fault(Failure::normal_region);return false;}
 context.policy.enabled=true;context.policy.normal_write_authorized=true;
 armed.store(true,std::memory_order_release);return true;
}
void readback(uintptr_t caller){Read read;uint32_t normal{},alternate{};
 if(!gears_graphics_lock::read_value(read,context.policy.admission.singleton+0x3c8,normal)||
    !gears_graphics_lock::read_value(read,context.policy.admission.singleton+0x56c,alternate)||normal||alternate)fault(Failure::readback,caller);
}
}
#endif
bool observing(){
#if defined(GEARS_GRAPHICS_RUNTIME_LOCK) && GEARS_GRAPHICS_RUNTIME_LOCK
 if(runtime::startup.load(std::memory_order_acquire))return true;
#endif
 return recording.load();
}
Fields fields(uintptr_t settings){Fields f{};
 if(settings&&safe_copy(&f.vtable,reinterpret_cast<void*>(settings),8))f.mask|=1;
 if(settings&&safe_copy(&f.normal,reinterpret_cast<void*>(settings+0x3c8),4))f.mask|=2;
 if(settings&&safe_copy(&f.alternate,reinterpret_cast<void*>(settings+0x56c),4))f.mask|=4;
 if(safe_copy(&f.queueflag,reinterpret_cast<void*>(base+0x1c204b0),4))f.mask|=8;
 if(safe_copy(&f.selector_thread,reinterpret_cast<void*>(base+0x1c204dc),4))f.mask|=16;
 return f;}
Row* begin(unsigned kind,void* argument,uintptr_t caller,int32_t mode=0){
 ++calls[kind];unsigned index=counts[kind].load();
 do {if(index>=32)return nullptr;}while(!counts[kind].compare_exchange_weak(index,index+1));
 auto& r=rows[kind][index];r.kind=kind;r.mode=mode;r.thread=GetCurrentThreadId();r.caller=caller;
 r.depth=depth;r.collector_settings=current_settings;r.settings=kind==2?base+gears_graphics_lock::singleton_rva:reinterpret_cast<uintptr_t>(argument);
 r.qpc=now();r.before=fields(r.settings);
 if(kind==2){r.command=reinterpret_cast<uintptr_t>(argument);unsigned char command[192]{};
  if(safe_copy(command,argument,sizeof(command))){r.read_mask|=1;memcpy(&r.command_vtable,command,8);memcpy(&r.command_dlss,command+0x78,4);}
  if(caller!=base+0x2ad008){
   r.stack_count=CaptureStackBackTrace(0,static_cast<DWORD>(r.stack.size()),r.stack.data(),nullptr);
   if(caller>=32&&safe_copy(r.caller_before.data(),reinterpret_cast<void*>(caller-32),32))r.read_mask|=2;
   if(safe_copy(r.caller_after.data(),reinterpret_cast<void*>(caller),32))r.read_mask|=4;
  }}return &r;}
uint32_t menu_dispatch(void* settings,int32_t mode,uintptr_t caller){Scope scope;
 Row* row=observing()?begin(0,settings,caller,mode):nullptr;
#if defined(GEARS_GRAPHICS_RUNTIME_LOCK) && GEARS_GRAPHICS_RUNTIME_LOCK
 if(runtime::armed.load(std::memory_order_acquire)){runtime::Read read;
  auto d=gears_graphics_lock::routes::menu_before(runtime::context,reinterpret_cast<uintptr_t>(settings),mode,runtime::call(caller),read);
  runtime::decision(d,caller);
  if(d.result.bypass_original){++runtime::menu_blocked;if(row){row->result=1;row->after=fields(row->settings);}return 1;}}
#endif
 const uint32_t result=menu_original(settings,mode);
 if(row){row->result=result;row->after=fields(row->settings);}return result;}
uint32_t __fastcall menu_hook(void* settings,int32_t mode){return menu_dispatch(settings,mode,reinterpret_cast<uintptr_t>(_ReturnAddress()));}
struct CollectorScope {
 const uintptr_t saved_settings=current_settings;const unsigned saved_depth=depth;
 explicit CollectorScope(void* settings){current_settings=reinterpret_cast<uintptr_t>(settings);
  depth=saved_depth==UINT_MAX?UINT_MAX:saved_depth+1;}
 ~CollectorScope(){depth=saved_depth;current_settings=saved_settings;}
};
uint64_t collector_dispatch(void* settings,uintptr_t caller){Scope scope;CollectorScope collector_scope(settings);
 Row* row=observing()?begin(1,settings,caller):nullptr;
#if defined(GEARS_GRAPHICS_RUNTIME_LOCK) && GEARS_GRAPHICS_RUNTIME_LOCK
 if(runtime::armed.load(std::memory_order_acquire)){runtime::Read read;auto local=runtime::context;
  runtime::Cas cas{local.policy.admission.singleton+0x3c8};local.policy.normal_write_authorized=runtime::writable4(cas.allowed);
  auto d=gears_graphics_lock::routes::collector_before(local,reinterpret_cast<uintptr_t>(settings),runtime::call(caller),read,cas);
  runtime::decision(d,caller);if(d.result.modified)++runtime::normal_cleared;
  gears_graphics_lock::routes::CollectorScope lock_scope(d);
  const uint64_t result=collector_original(settings);
  if(row){row->result=result;row->after=fields(row->settings);}return result;}
#endif
 const uint64_t result=collector_original(settings);
 if(row){row->result=result;row->after=fields(row->settings);}
 return result;}
uint64_t __fastcall collector_hook(void* settings){return collector_dispatch(settings,reinterpret_cast<uintptr_t>(_ReturnAddress()));}
uint32_t commit_dispatch(void* command,uintptr_t caller){Scope scope;
 Row* row=observing()?begin(2,command,caller):nullptr;
#if defined(GEARS_GRAPHICS_RUNTIME_LOCK) && GEARS_GRAPHICS_RUNTIME_LOCK
 const bool locking=runtime::armed.load(std::memory_order_acquire);
 if(locking){runtime::Read read;auto local=runtime::context;auto c=runtime::call(caller);uintptr_t failed{};
  bool admitted=true;
  if(c.return_rva!=0x2ad008){const auto failure=gears_graphics_runtime::queued(base,c,read,failed);
   admitted=failure==runtime::Failure::none;
   if(!admitted)runtime::fault(failure,caller,failed);
   else local.queued[0]={true,c.thread,c.return_rva};}
  unsigned char extent[192]{};const auto address=reinterpret_cast<uintptr_t>(command);
  if(admitted){uintptr_t vt{},entry{};local.policy.command_layout_verified=read(address,extent,sizeof(extent));
   if(local.policy.command_layout_verified){memcpy(&vt,extent,8);
    local.policy.command_layout_verified=vt==local.policy.admission.command_vtable&&
     read(vt+8,&entry,8)&&entry==base+gears_graphics_lock::commit_rva;}
   local.policy.command_write_authorized=address<=UINTPTR_MAX-0x78&&runtime::writable4(address+0x78);
   runtime::Cas cas{address<=UINTPTR_MAX-0x78?address+0x78:0};
   auto d=gears_graphics_lock::routes::commit_before(local,address,c,read,cas);runtime::decision(d,caller);
   if(d.result.modified)++runtime::command_cleared;}}
#endif
 const uint32_t result=commit_original(command);
#if defined(GEARS_GRAPHICS_RUNTIME_LOCK) && GEARS_GRAPHICS_RUNTIME_LOCK
 if(locking){if(result!=192)runtime::fault(runtime::Failure::return_stride,caller);runtime::readback(caller);}
#endif
 // Command/view/payload may already be retired: read only fixed singleton after.
 if(row){row->result=result;row->after=fields(row->settings);}return result;}
uint32_t __fastcall commit_hook(void* command){return commit_dispatch(command,reinterpret_cast<uintptr_t>(_ReturnAddress()));}
#if defined(GEARS_GRAPHICS_MENU_LOCK) && GEARS_GRAPHICS_MENU_LOCK
using Select=void (__fastcall*)(void*,int32_t);
Select select_original{};
std::atomic<uint64_t> selection_calls{},selection_blocked{};
struct SelectionRow {uintptr_t object{},caller{};DWORD thread{};int32_t before{},requested{},forward{},after{};};
std::array<SelectionRow,16> selection_rows{};std::atomic<unsigned> selection_count{};
bool certify_selection(){
 std::array<unsigned char,gears_graphics_menu::set_index_size> bytes{};
 if(!safe_copy(bytes.data(),reinterpret_cast<void*>(base+gears_graphics_menu::set_index_rva),bytes.size()))return false;
 BCRYPT_ALG_HANDLE algorithm{};BCRYPT_HASH_HANDLE hash{};unsigned char digest[32]{};
 bool ok=BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
 if(ok)ok=BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)>=0;
 if(ok)ok=BCryptHashData(hash,bytes.data(),static_cast<ULONG>(bytes.size()),0)>=0;
 if(ok)ok=BCryptFinishHash(hash,digest,32,0)>=0;
 if(hash)BCryptDestroyHash(hash);if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);
 char text[65]{};for(unsigned i=0;i<32;++i)sprintf_s(text+2*i,3,"%02x",digest[i]);
 return ok&&!strcmp(text,gears_graphics_menu::set_index_sha256);
}
void __fastcall select_hook(void* object,int32_t index){Scope scope;++selection_calls;
 runtime::Read read;const auto choice=gears_graphics_menu::select(read,base,reinterpret_cast<uintptr_t>(object),index);
 SelectionRow* row=nullptr;
 if(choice.matched){++selection_blocked;unsigned slot=selection_count.fetch_add(1);
  if(slot<selection_rows.size()){row=&selection_rows[slot];*row={reinterpret_cast<uintptr_t>(object),
   reinterpret_cast<uintptr_t>(_ReturnAddress()),GetCurrentThreadId(),choice.before,index,choice.forward,-1};}}
 select_original(object,choice.forward);
 if(row)safe_copy(&row->after,reinterpret_cast<const unsigned char*>(object)+0x5d8,4);
}
#endif
}
bool reserve_graphics_lock_observer(){using namespace graphics_observer;
 if(reserved)return true;
 for(unsigned i=0;i<target_count;++i){auto const& e=target_entry(i);targets[i]=reinterpret_cast<void*>(base+e.rva);
  std::array<unsigned char,32> actual{};if(!safe_copy(actual.data(),targets[i],32)||actual!=e.bytes)return false;}
 std::array<void*,target_count> detours{reinterpret_cast<void*>(menu_hook),reinterpret_cast<void*>(collector_hook),reinterpret_cast<void*>(commit_hook)};
 std::array<void**,target_count> originals{reinterpret_cast<void**>(&menu_original),reinterpret_cast<void**>(&collector_original),reinterpret_cast<void**>(&commit_original)};
#if defined(GEARS_GRAPHICS_MENU_LOCK) && GEARS_GRAPHICS_MENU_LOCK
 if(!certify_selection())return false;
 detours[3]=reinterpret_cast<void*>(select_hook);originals[3]=reinterpret_cast<void**>(&select_original);
#endif
 for(unsigned i=0;i<target_count;++i){if(MH_CreateHook(targets[i],detours[i],originals[i])!=MH_OK)return false;
  std::array<unsigned char,32> after{};if(!safe_copy(after.data(),targets[i],32)||after!=target_entry(i).bytes)return false;}
 reserved=true;return true;}
bool graphics_lock_observer_reserved_target(void* target){using namespace graphics_observer;
 return reserved&&std::find(targets.begin(),targets.end(),target)!=targets.end();}
bool graphics_lock_primed_bytes(void* target,std::array<unsigned char,32>& bytes){
#if defined(GEARS_GRAPHICS_RUNTIME_LOCK) && GEARS_GRAPHICS_RUNTIME_LOCK
 using namespace graphics_observer;
 if(runtime::startup.load(std::memory_order_acquire))for(unsigned i=0;i<target_count;++i)
  if(target==targets[i]){bytes=target_entry(i).bytes;return true;}
#endif
 return false;
}
unsigned graphics_lock_prime_count(){
#if defined(GEARS_GRAPHICS_RUNTIME_LOCK) && GEARS_GRAPHICS_RUNTIME_LOCK
 return graphics_observer::runtime::startup.load(std::memory_order_acquire)?graphics_observer::target_count:0;
#else
 return 0;
#endif
}
bool begin_graphics_lock_prime(FILE* status,bool owned){
#if defined(GEARS_GRAPHICS_STARTUP_LOCK) && GEARS_GRAPHICS_STARTUP_LOCK
 using namespace graphics_observer;
 // CPU descriptor-prime tests have no reserved game entries. Production owned
 // priming always follows GearsCaptureReserve and its executable certification.
 if(!owned||!reservation_ready)return true;
 if(!reserved||!runtime::admit()){
  if(runtime::fault_phase.load(std::memory_order_acquire)==2){auto const& f=runtime::first_fault;
   fprintf(status,"graphics_prime_admission rejected=1 reason=%u policy_reason=%u window=0x%llx\n",unsigned(f.reason),f.policy_reason,f.window);}
  return false;}
 runtime::startup.store(true,std::memory_order_release);
 for(auto target:targets)if(MH_QueueEnableHook(target)!=MH_OK)return false;
 fprintf(status,"graphics_prime_admission admitted=1 hooks=%u\n",target_count);
#else
 (void)status;(void)owned;
#endif
 return true;
}
bool disable_graphics_lock_prime(){
 if(!graphics_lock_prime_count())return true;
 bool ok=true;for(auto target:graphics_observer::targets){
  auto queued=MH_QueueDisableHook(target);auto disabled=MH_DisableHook(target);
  ok=(queued==MH_OK&&(disabled==MH_OK||disabled==MH_ERROR_DISABLED))&&ok;}
 return ok;
}
bool graphics_lock_prime_restored(){
 if(!graphics_lock_prime_count())return true;
 std::array<unsigned char,32> bytes{};
 for(unsigned i=0;i<graphics_observer::target_count;++i)if(!safe_copy(bytes.data(),graphics_observer::targets[i],32)||
  bytes!=graphics_observer::target_entry(i).bytes)return false;
 return true;
}
bool graphics_lock_accept_handoff(){
#if defined(GEARS_GRAPHICS_RUNTIME_LOCK) && GEARS_GRAPHICS_RUNTIME_LOCK
 // SC ownership/latch ordering: a concurrent fault is either consumed here or
 // observes session_owned and requests continuous stop itself.
 graphics_observer::runtime::session_owned.store(true);
 if(graphics_observer::runtime::faults.load()){
  request_session_stop(SessionStop::proof_failure);return false;}
#endif
 return true;
}
bool enable_graphics_lock_observer(){using namespace graphics_observer;
 if(!continuous_session)return true;
 if(!reserved&&!reserve_graphics_lock_observer())return false;
#if defined(GEARS_GRAPHICS_RUNTIME_LOCK) && GEARS_GRAPHICS_RUNTIME_LOCK
 if(runtime::armed.load(std::memory_order_acquire))runtime::readback(0);
 if(runtime::faults.load()||(!runtime::armed.load(std::memory_order_acquire)&&!runtime::admit())){
  if(diagnostic_status&&runtime::fault_phase.load(std::memory_order_acquire)==2){auto const& f=runtime::first_fault;
   fprintf(diagnostic_status,"graphics_runtime_lock_admission rejected=1 reason=%u policy_reason=%u failed_window_rva=0x%llx\n",unsigned(f.reason),f.policy_reason,f.window);}
  return false;}
#endif
 bool ok=install(targets[0],reinterpret_cast<void*>(menu_hook),reinterpret_cast<void**>(&menu_original))&&
  install(targets[1],reinterpret_cast<void*>(collector_hook),reinterpret_cast<void**>(&collector_original))&&
  install(targets[2],reinterpret_cast<void*>(commit_hook),reinterpret_cast<void**>(&commit_original));
#if defined(GEARS_GRAPHICS_MENU_LOCK) && GEARS_GRAPHICS_MENU_LOCK
 ok=ok&&install(targets[3],reinterpret_cast<void*>(select_hook),reinterpret_cast<void**>(&select_original));
#endif
 return ok;}
void print_graphics_lock_observer(FILE* file,bool disabled){using namespace graphics_observer;
 unsigned observe_only=1,effective=0,startup=graphics_lock_prime_count()?1:0;
#if defined(GEARS_GRAPHICS_RUNTIME_LOCK) && GEARS_GRAPHICS_RUNTIME_LOCK
 observe_only=0;
 effective=runtime::armed.load()&&!runtime::faults.load()?1:0;
#endif
 fprintf(file,"graphics_lock_observer enabled=1 observe_only=%u lock_effective=%u startup_before_level=%u stopped_safe=%u fields_bits=vtable:1,normal:2,alternate:4,queueflag:8,selector_thread:16 command_bits=extent192:1,caller_before32:2,caller_after32:4\n",observe_only,effective,startup,disabled&&!active.load()?1:0);
 if(!disabled||active.load())return;
#if defined(GEARS_GRAPHICS_MENU_LOCK) && GEARS_GRAPHICS_MENU_LOCK
 fprintf(file,"graphics_menu_lock calls=%llu blocked=%llu rows=%u\n",selection_calls.load(),selection_blocked.load(),(std::min)(selection_count.load(),unsigned(selection_rows.size())));
 for(unsigned i=0;i<(std::min)(selection_count.load(),unsigned(selection_rows.size()));++i){auto const& r=selection_rows[i];
  fprintf(file,"graphics_menu_lock_row object=0x%llx caller_rva=0x%llx thread=%lu before=%d requested=%d forward=%d after=%d\n",
   r.object,r.caller>=base?r.caller-base:0,r.thread,r.before,r.requested,r.forward,r.after);}
#endif
#if defined(GEARS_GRAPHICS_RUNTIME_LOCK) && GEARS_GRAPHICS_RUNTIME_LOCK
 fprintf(file,"graphics_runtime_lock enabled=1 continuous_only=%u startup_before_level=%u admitted=%u protected=%u effective=%u menuBlocked=%llu normalCleared=%llu commandCleared=%llu faults=%llu actual_toggle_verified=0\n",startup?0:1,startup,
  runtime::armed.load()?1:0,runtime::armed.load()?1:0,runtime::armed.load()&&!runtime::faults.load()?1:0,
  runtime::menu_blocked.load(),runtime::normal_cleared.load(),runtime::command_cleared.load(),runtime::faults.load());
 if(runtime::fault_phase.load(std::memory_order_acquire)==2){auto const& f=runtime::first_fault;
  fprintf(file,"graphics_runtime_lock_fault reason=%u policy_reason=%u route_status=%u thread=%lu caller=0x%llx failed_window_rva=0x%llx\n",
   unsigned(f.reason),f.policy_reason,f.route_status,f.thread,f.caller,f.window);}
#endif
 auto rva=[](uintptr_t value){return value>=base?value-base:0;};
 for(unsigned kind=0;kind<3;++kind){fprintf(file,"graphics_lock_observer_kind kind=%u calls=%llu rows=%u\n",kind,calls[kind].load(),counts[kind].load());
  for(unsigned i=0;i<counts[kind].load();++i){auto const& r=rows[kind][i];
   fprintf(file,"graphics_lock_observer_row kind=%u row=%u qpc=%lld thread=%lu caller=0x%llx caller_rva=0x%llx depth=%u collector_settings=0x%llx settings=0x%llx mode=%d command=0x%llx command_vtable=0x%llx command_dlss=%u read_mask=%u original_rax=0x%llx before_mask=%u before_vtable=0x%llx normal_before=%u alternate_before=%u queueflag_before=%u selector_thread_before=%u after_mask=%u normal_after=%u alternate_after=%u queueflag_after=%u selector_thread_after=%u stack_count=%u\n",
    kind,i,r.qpc,r.thread,r.caller,rva(r.caller),r.depth,r.collector_settings,r.settings,r.mode,r.command,r.command_vtable,r.command_dlss,r.read_mask,r.result,
    r.before.mask,r.before.vtable,r.before.normal,r.before.alternate,r.before.queueflag,r.before.selector_thread,
    r.after.mask,r.after.normal,r.after.alternate,r.after.queueflag,r.after.selector_thread,r.stack_count);
   if(kind==2&&r.caller!=base+0x2ad008){fprintf(file,"graphics_lock_observer_caller row=%u before32=",i);for(auto b:r.caller_before)fprintf(file,"%02x",b);
    fprintf(file," after32=");for(auto b:r.caller_after)fprintf(file,"%02x",b);fprintf(file,"\n");
    for(unsigned s=0;s<r.stack_count;++s){auto pc=reinterpret_cast<uintptr_t>(r.stack[s]);fprintf(file,"graphics_lock_observer_stack row=%u index=%u pc=0x%llx game_rva_candidate=0x%llx\n",i,s,pc,rva(pc));}}
  }}
}
namespace graphics_observer {
unsigned test_menu_calls{},test_collector_calls{},test_commit_calls{};
std::array<unsigned char,192> test_command{};
uint32_t __fastcall test_menu(void*,int32_t mode){++test_menu_calls;return static_cast<uint32_t>(mode+17);}
uint32_t __fastcall test_commit(void* command){++test_commit_calls;memset(command,0,192);return 192;}
uint64_t __fastcall test_collector(void*){++test_collector_calls;const uint32_t enabled=1;memcpy(test_command.data()+0x78,&enabled,4);
 if(commit_hook(test_command.data())!=192)return 0;return 0x12345678abcdef01ull;}
uint64_t __fastcall test_collector_throw(void*){throw 17;}
#if defined(GEARS_GRAPHICS_RUNTIME_LOCK) && GEARS_GRAPHICS_RUNTIME_LOCK
std::array<unsigned char,192> runtime_test_command{},runtime_test_expected{};
unsigned runtime_test_collectors{},runtime_test_commits{};bool runtime_test_opaque{};
uint32_t __fastcall runtime_test_commit(void* command){++runtime_test_commits;
 runtime_test_opaque=runtime_test_opaque&&!memcmp(command,runtime_test_expected.data(),192);return 192;}
uint64_t __fastcall runtime_test_collector(void* settings){++runtime_test_collectors;uint32_t normal{};
 memcpy(&normal,reinterpret_cast<unsigned char*>(settings)+0x3c8,4);
 runtime_test_opaque=runtime_test_opaque&&!normal&&commit_dispatch(runtime_test_command.data(),base+0x2ad008)==192;
 return 0x12345678abcdef01ull;}
bool runtime_dispatch_self_test(){
 const auto saved_context=runtime::context;const auto saved_menu=menu_original;
 const auto saved_collector=collector_original;const auto saved_commit=commit_original;
 const bool saved_recording=recording.load();recording=false;
 alignas(8) std::array<unsigned char,0x574> settings{};uintptr_t settings_vt=0x11223344;
 std::array<uintptr_t,2> command_vt{0,base+gears_graphics_lock::commit_rva};
 memcpy(settings.data(),&settings_vt,8);runtime_test_command.fill(0x5a);
 const auto vt=reinterpret_cast<uintptr_t>(command_vt.data());memcpy(runtime_test_command.data(),&vt,8);
 const uint32_t one=1,zero=0;memcpy(runtime_test_command.data()+0x78,&one,4);
 runtime_test_expected=runtime_test_command;memcpy(runtime_test_expected.data()+0x78,&zero,4);
 runtime::context={};runtime::context.policy.enabled=true;runtime::context.policy.normal_write_authorized=true;
 runtime::context.policy.admission={gears_graphics_lock::Reason::applied,base,reinterpret_cast<uintptr_t>(settings.data()),settings_vt,vt};
 menu_original=test_menu;collector_original=runtime_test_collector;commit_original=runtime_test_commit;
 runtime_test_opaque=true;runtime::armed=true;
 bool valid=menu_dispatch(settings.data(),3,base+0x28047a)==1&&test_menu_calls==0;
 valid=menu_dispatch(settings.data(),0,base+0x28047a)==17&&test_menu_calls==1&&valid;
 memcpy(settings.data()+0x3c8,&one,4);
 valid=collector_dispatch(settings.data(),base+0x1f9a9b)==0x12345678abcdef01ull&&valid;
 valid=valid&&runtime_test_collectors==1&&runtime_test_commits==1&&runtime_test_opaque&&
  runtime::menu_blocked==1&&runtime::normal_cleared==1&&runtime::command_cleared==1&&runtime::faults==0&&!active.load();
 runtime::armed=false;runtime::context=saved_context;runtime::menu_blocked=0;runtime::normal_cleared=0;runtime::command_cleared=0;runtime::faults=0;
 runtime::fault_phase=0;runtime::first_fault={};test_menu_calls=runtime_test_collectors=runtime_test_commits=0;
 menu_original=saved_menu;collector_original=saved_collector;commit_original=saved_commit;recording=saved_recording;return valid;
}
#endif
bool test_nested_scope_valid{};
uint64_t __fastcall test_collector_nested(void* settings){
 // Use the production wrapper recursively with a distinct non-dereferenced
 // marker while recording is off, so this self-test cannot read fake memory.
 static unsigned nested{};
 if(nested){test_nested_scope_valid=test_nested_scope_valid&&depth==2&&current_settings==reinterpret_cast<uintptr_t>(settings);return 42;}
 const auto parent=current_settings;++nested;
 const auto result=collector_hook(reinterpret_cast<void*>(uintptr_t(0x1234)));--nested;
 test_nested_scope_valid=test_nested_scope_valid&&result==42&&depth==1&&current_settings==parent;return 43;
}
}
bool graphics_lock_observer_self_test(){using namespace graphics_observer;
#if defined(GEARS_GRAPHICS_RUNTIME_LOCK) && GEARS_GRAPHICS_RUNTIME_LOCK
 if(!runtime::cas_self_test()||!runtime_dispatch_self_test())return false;
#endif
 // CPU callbacks only; no game entry hooks/objects. Test through the production
 // wrappers, including raw RAX, nested scope and destroyed-command snapshot.
 const auto saved_menu=menu_original;const auto saved_collector=collector_original;const auto saved_commit=commit_original;
 const bool saved_recording=recording.load();menu_original=test_menu;collector_original=test_collector;commit_original=test_commit;
 recording=true;std::array<unsigned char,0x574> settings{};bool valid=true;
 for(unsigned i=0;i<40;++i){valid=menu_hook(settings.data(),3)==20&&valid;
  valid=collector_hook(settings.data())==0x12345678abcdef01ull&&valid;}
 valid=valid&&test_menu_calls==40&&test_collector_calls==40&&test_commit_calls==40&&depth==0&&!current_settings&&!active.load();
 for(unsigned kind=0;kind<3;++kind)valid=valid&&counts[kind]==32&&calls[kind]==40;
 valid=valid&&rows[2][0].depth==1&&rows[2][0].collector_settings==reinterpret_cast<uintptr_t>(settings.data())&&
  rows[2][0].command_dlss==1&&(rows[2][0].read_mask&1)&&rows[2][0].result==192&&rows[1][0].result==0x12345678abcdef01ull;
 // Restoration includes a parent scope, nested wrappers and C++ unwinding.
 // No native/game callback is invoked by these fake originals.
 recording=false;collector_original=test_collector_nested;test_nested_scope_valid=true;
 valid=collector_hook(settings.data())==43&&test_nested_scope_valid&&depth==0&&!current_settings&&!active.load()&&valid;
 const auto parent_settings=current_settings;const auto parent_depth=depth;
 current_settings=0x5678;depth=7;collector_original=test_collector_throw;bool caught=false;
 try{(void)collector_hook(settings.data());}catch(int value){caught=value==17;}
 valid=valid&&caught&&depth==7&&current_settings==0x5678&&!active.load();
 depth=parent_depth;current_settings=parent_settings;
 menu_original=saved_menu;collector_original=saved_collector;commit_original=saved_commit;recording=saved_recording;
 for(unsigned kind=0;kind<3;++kind){counts[kind]=0;calls[kind]=0;}rows={};
 test_menu_calls=test_collector_calls=test_commit_calls=0;return valid;
}
#else
bool reserve_graphics_lock_observer(){return true;}
bool graphics_lock_observer_reserved_target(void*){return false;}
bool graphics_lock_primed_bytes(void*,std::array<unsigned char,32>&){return false;}
unsigned graphics_lock_prime_count(){return 0;}
bool begin_graphics_lock_prime(FILE*,bool){return true;}
bool disable_graphics_lock_prime(){return true;}
bool graphics_lock_prime_restored(){return true;}
bool graphics_lock_accept_handoff(){return true;}
bool enable_graphics_lock_observer(){return true;}
void print_graphics_lock_observer(FILE*,bool){}
bool graphics_lock_observer_self_test(){return true;}
#endif
