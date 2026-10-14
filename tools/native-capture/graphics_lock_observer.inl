#pragma once
#include "graphics_lock.h"

// Continuous-session observation only. Reserve creates inactive trampolines;
// this experiment deliberately does not cover menus before level activation.
bool install(void*,void*,void**);
#if defined(GEARS_GRAPHICS_LOCK_OBSERVER) && GEARS_GRAPHICS_LOCK_OBSERVER
namespace graphics_observer {
using Menu=uint32_t (__fastcall*)(void*,int32_t);
using Collector=uint64_t (__fastcall*)(void*); // Raw machine RAX; no recovered C++ return claim.
using Commit=uint32_t (__fastcall*)(void*);
Menu menu_original{};Collector collector_original{};Commit commit_original{};
std::array<void*,3> targets{};bool reserved{};
thread_local unsigned depth{};thread_local uintptr_t current_settings{};
struct Fields {uintptr_t vtable{};uint32_t normal{},alternate{},queueflag{},selector_thread{},mask{};};
struct Row {DWORD thread{};uintptr_t caller{},settings{},collector_settings{},command{},command_vtable{};
 unsigned depth{},kind{},read_mask{};int32_t mode{};uint32_t command_dlss{};uint64_t result{};
 long long qpc{};Fields before{},after{};unsigned stack_count{};std::array<void*,8> stack{};
 std::array<unsigned char,32> caller_before{},caller_after{};};
std::array<std::array<Row,32>,3> rows{};
std::array<std::atomic<unsigned>,3> counts{};
std::array<std::atomic<uint64_t>,3> calls{};
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
uint32_t __fastcall menu_hook(void* settings,int32_t mode){Scope scope;
 Row* row=recording.load()?begin(0,settings,reinterpret_cast<uintptr_t>(_ReturnAddress()),mode):nullptr;
 const uint32_t result=menu_original(settings,mode);
 if(row){row->result=result;row->after=fields(row->settings);}return result;}
struct CollectorScope {
 const uintptr_t saved_settings=current_settings;const unsigned saved_depth=depth;
 explicit CollectorScope(void* settings){current_settings=reinterpret_cast<uintptr_t>(settings);
  depth=saved_depth==UINT_MAX?UINT_MAX:saved_depth+1;}
 ~CollectorScope(){depth=saved_depth;current_settings=saved_settings;}
};
uint64_t __fastcall collector_hook(void* settings){Scope scope;CollectorScope collector_scope(settings);
 Row* row=recording.load()?begin(1,settings,reinterpret_cast<uintptr_t>(_ReturnAddress())):nullptr;
 const uint64_t result=collector_original(settings);
 if(row){row->result=result;row->after=fields(row->settings);}
 return result;}
uint32_t __fastcall commit_hook(void* command){Scope scope;
 Row* row=recording.load()?begin(2,command,reinterpret_cast<uintptr_t>(_ReturnAddress())):nullptr;
 const uint32_t result=commit_original(command);
 // Command/view/payload may already be retired: read only fixed singleton after.
 if(row){row->result=result;row->after=fields(row->settings);}return result;}
}
bool reserve_graphics_lock_observer(){using namespace graphics_observer;
 if(reserved)return true;
 for(unsigned i=0;i<3;++i){auto const& e=gears_graphics_lock::entries[i];targets[i]=reinterpret_cast<void*>(base+e.rva);
  std::array<unsigned char,32> actual{};if(!safe_copy(actual.data(),targets[i],32)||actual!=e.bytes)return false;}
 std::array<void*,3> detours{reinterpret_cast<void*>(menu_hook),reinterpret_cast<void*>(collector_hook),reinterpret_cast<void*>(commit_hook)};
 std::array<void**,3> originals{reinterpret_cast<void**>(&menu_original),reinterpret_cast<void**>(&collector_original),reinterpret_cast<void**>(&commit_original)};
 for(unsigned i=0;i<3;++i){if(MH_CreateHook(targets[i],detours[i],originals[i])!=MH_OK)return false;
  std::array<unsigned char,32> after{};if(!safe_copy(after.data(),targets[i],32)||after!=gears_graphics_lock::entries[i].bytes)return false;}
 reserved=true;return true;}
bool graphics_lock_observer_reserved_target(void* target){using namespace graphics_observer;
 return reserved&&std::find(targets.begin(),targets.end(),target)!=targets.end();}
bool enable_graphics_lock_observer(){using namespace graphics_observer;
 if(!continuous_session)return true;
 if(!reserved&&!reserve_graphics_lock_observer())return false;
 return install(targets[0],reinterpret_cast<void*>(menu_hook),reinterpret_cast<void**>(&menu_original))&&
  install(targets[1],reinterpret_cast<void*>(collector_hook),reinterpret_cast<void**>(&collector_original))&&
  install(targets[2],reinterpret_cast<void*>(commit_hook),reinterpret_cast<void**>(&commit_original));}
void print_graphics_lock_observer(FILE* file,bool disabled){using namespace graphics_observer;
 fprintf(file,"graphics_lock_observer enabled=1 observe_only=1 lock_effective=0 startup_before_level=0 stopped_safe=%u fields_bits=vtable:1,normal:2,alternate:4,queueflag:8,selector_thread:16 command_bits=extent192:1,caller_before32:2,caller_after32:4\n",disabled&&!active.load()?1:0);
 if(!disabled||active.load())return;
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
bool enable_graphics_lock_observer(){return true;}
void print_graphics_lock_observer(FILE*,bool){}
bool graphics_lock_observer_self_test(){return true;}
#endif
