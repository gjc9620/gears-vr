// Repeatable synthetic cleanup regression tests. No game process, snapshots,
// files, hook installation or engine calls are used. All object/registry names
// and addresses below belong to the private VirtualAlloc fixture.
//
// Compile the tested module even when the product CMake option is OFF. World
// entry attestation and selected reflection proofs are fixture inputs; the real
// RecursiveAdmission gate is exercised across original/post/SEH boundaries.
// These tests verify journal/guard behavior, not retail metadata discovery.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#ifdef GEARS_COVER_BODY_PROTOTYPE
#undef GEARS_COVER_BODY_PROTOTYPE
#endif
#define GEARS_COVER_BODY_PROTOTYPE 1
#include <windows.h>
#include <array>
#include <atomic>
#include <mutex>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cwchar>
#include <cmath>
#include "anchor_observer.h"
#include "world_epoch_gate.h"
#include "MinHook.h"
uintptr_t base=0x140000000;
std::atomic<uint64_t> committed_world_generation{1};
std::atomic<bool> recording{true};
bool continuous_session=true,anchor_observer_enabled=true;
gears_anchor::Config anchor_config;
std::atomic<unsigned> active{};
unsigned scope_entries{},scope_live{};bool scope_admitted=true;
struct OwnCppFailure {};
bool throw_scope{};uintptr_t throwing_read{};
gears_world::EpochGate world_gate;thread_local gears_world::ThreadState world_thread;
struct Scope {gears_world::RecursiveAdmission admission{world_thread,world_gate};bool admitted=scope_admitted;
 Scope(){++active;try{if(throw_scope){throw_scope=false;throw OwnCppFailure{};}++scope_entries;++scope_live;}catch(...){--active;throw;}}
 ~Scope(){--scope_live;--active;}bool world()const{return admitted&&bool(admission);}};
uintptr_t denied{},changing_text{};unsigned text_reads{};
bool safe_copy(void* out,void const* from,size_t n){
 auto a=reinterpret_cast<uintptr_t>(from);if(a==denied)return false;
 if(a==throwing_read){throwing_read=0;throw OwnCppFailure{};}
 if(a==changing_text&&++text_reads==2)*reinterpret_cast<wchar_t*>(changing_text)=L'X';
 __try {memcpy(out,from,n);return true;} __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
long long now(){return static_cast<long long>(GetTickCount64());}
// Stub only: the test never reserves or installs a hook.
extern "C" MH_STATUS WINAPI MH_CreateHook(void*,void*,void**){return MH_ERROR_UNSUPPORTED_FUNCTION;}
#include "cover_body_live.inl"
namespace cover_body_live {bool own_entries(){return true;}}
extern "C" void GearsCoverBlendTickHook(){}
extern "C" void GearsCoverChildPropagateHook(){}
namespace b=cover_body_live;
// Fixed synthetic register packet invocation only; the separate MASM test
// verifies the actual ABI bridge. No real code address is called by this test.
extern "C" void GearsCoverRawInvoke(b::CallPacket* packet,void* original) noexcept(false){reinterpret_cast<void(*)(b::CallPacket*)>(original)(packet);}
unsigned checks{},failures{};
#define C(x) do{++checks;if(!(x)){++failures;printf("FAIL %u: %s\n",__LINE__,#x);}}while(0)
uintptr_t arena{},node{},pawn{},mesh{},fixture_world{},records{},node_text{};
b::Call fixture_parent{};
template<class T>void put(uintptr_t a,T const& v){memcpy(reinterpret_cast<void*>(a),&v,sizeof v);}
void setup(){
 memset(reinterpret_cast<void*>(arena),0,1024*1024);b::metadata={};auto& c=b::metadata;
 c.objects_header=arena+0x100;c.names_header=arena+0x200;c.identity_count=5;c.object_internal_index=8;c.object_name=72;c.object_class=80;c.object_outer=64;c.name_entry_index=0;c.name_entry_text=20;c.field_super=96;
 c.role_controller=0;c.role_local=1;c.role_camera=2;c.role_pawn=3;c.role_level=4;c.controller_pawn=0x100;c.pawn_controller=0x100;c.player_camera=0x108;c.pc_owner=0x108;c.player=0x110;c.camera_view_target=0x100;
 gears_anchor::Registry objects{arena+0x1000,2001,2100},names{arena+0x6000,2001,2100};put(c.objects_header,objects);put(c.names_header,names);
 for(unsigned i=0;i<9;++i){gears_anchor::Identity id{};id.address=arena+0x30000+uintptr_t(i)*0x400;id.index=id.name_index=i;id.cls=arena+0x48000+uintptr_t(i)*0x100;id.outer=i==3?arena+0x31000:0;
  swprintf_s(id.name.data(),id.name.size(),L"Synthetic%u",i);put(id.address+8,id.index);put(id.address+72,id.name_index);put(id.address+76,id.number);put(id.address+80,id.cls);put(id.address+64,id.outer);
  put(objects.data+uintptr_t(i)*8,id.address);auto e=arena+0x20000+uintptr_t(i)*0x200;put(names.data+uintptr_t(i)*8,e);put(e,id.name_index);put(e+20,id.name);
  if(i<5)c.identities[i]=id;if(i==5){node=id.address;node_text=e+20;}if(i==6)mesh=id.address;if(i==7)fixture_world=id.address;
 }
 auto pc=c.identities[0].address,local=c.identities[1].address,camera=c.identities[2].address; pawn=c.identities[3].address;
 put(pc+c.controller_pawn,pawn);put(pc+c.player_camera,camera);put(pc+c.player,local);put(pawn+c.pawn_controller,pc);put(camera+c.camera_view_target,pawn);put(camera+c.pc_owner,pc);
 b::layout={};b::layout.children=0xe4;b::layout.anim=8;b::layout.weight=0x10;b::layout.total=0x14;b::layout.skel=0x60;b::layout.owner=0x128;b::layout.mesh=0x108;b::layout.world=0x110;b::layout.cover=0x118;
 put(node,base+0x13d0080);put(node+0x60,mesh);put(node+0x128,pawn);put(pawn+0x108,mesh);put(pawn+0x110,fixture_world);put(pawn+0x118,static_cast<unsigned char>(2));
 records=arena+0x50000;b::Children children{records,5,5};put(node+0xe4,children);
 b::journals={};auto& j=b::journals[0];j.used=true;C(b::identity(node,j.id,true));j.pawn=pawn;j.mesh=mesh;j.world=fixture_world;j.generation=1;j.children=children;j.installed_step=2;
 for(unsigned i=0;i<5;++i){float native=i==0?1.f:0.f,installed=i==0||i==4?.5f:0.f;put(records+uintptr_t(i)*0x44+0x10,installed);put(records+uintptr_t(i)*0x44+0x14,123.25f);memcpy(&j.native[i],&native,4);memcpy(&j.installed[i],&installed,4);j.owned[i]=i==0||i==4;}
 b::stopping=false;b::failed=false;b::external_change=false;b::drain_tick=0;b::writes=2;b::restores=0;b::retired=0;b::external=0;b::row_count=0;b::step_rows={};b::recount();denied=changing_text=0;text_reads=0;committed_world_generation=1;
 b::enabled=false;b::game_thread=0;b::tick={};b::injecting=false;b::withdrawing=false;b::timed_out=false;b::last_advance=0;b::next_step=0;b::step=0;scope_entries=scope_live=0;scope_admitted=true;active=0;
 b::calls={};b::call_depth=0;b::current_blend=nullptr;b::transactions=0;b::in_flight=0;fixture_parent={};
 b::child_posts=b::native_posts=b::pose_rearms=b::abandoned=b::reentries=0;b::layout_ready=false;b::initial_world=1;
 throw_scope=false;throwing_read=0;
}
float weight(unsigned i){float v{};memcpy(&v,reinterpret_cast<void*>(records+uintptr_t(i)*0x44+0x10),4);return v;}
void bind_target(){
 b::enabled=true;b::game_thread=GetCurrentThreadId();b::step=2;b::injecting=true;b::last_advance=GetTickCount64();
 b::tick={node,pawn,mesh,fixture_world,b::journals[0].id,b::journals[0].children,1,true};
 fixture_parent={};fixture_parent.live=fixture_parent.selected=true;fixture_parent.self=node;b::current_blend=&fixture_parent;
}
void before_blend(uintptr_t self){Scope scope;std::lock_guard<std::mutex> lock(b::mutex);b::blend_before(self,scope);}
// A synthetic FA680 invocation consumes the selected Weight exactly once.
void child(uintptr_t self,int index,float incoming){
 b::CallPacket packet{};packet.gpr[1]=self;packet.gpr[2]=uint32_t(index);memcpy(packet.xmm[2].data(),&incoming,4);uintptr_t token{};
 b::begin_call(packet,1,token);
 if(token&&b::token_call(token)->tracked_child){float w=weight(unsigned(index)),total=incoming*w;put(records+uintptr_t(index)*0x44+0x14,total);}
 b::finish_call(token,true);b::release_call(token);
}
void fixture_finish(){
 auto& c=b::calls[0];c=fixture_parent;c.live=true;c.kind=0;c.saved_tick={};c.previous_blend=nullptr;c.scope=nullptr;b::call_depth=1;b::current_blend=&c;b::transactions=1;
 const auto token=reinterpret_cast<uintptr_t>(&c);b::finish_call(token,true);b::release_call(token);
}
uintptr_t npc_node(){
 gears_anchor::Registry objects{},names{};memcpy(&objects,reinterpret_cast<void*>(b::metadata.objects_header),sizeof objects);memcpy(&names,reinterpret_cast<void*>(b::metadata.names_header),sizeof names);
 auto object=[&](uintptr_t address,unsigned index,uintptr_t cls,uintptr_t outer,const wchar_t* label){
  put(address+8,index);put(address+72,index);put(address+76,0u);put(address+80,cls);put(address+64,outer);put(objects.data+uintptr_t(index)*8,address);
  auto entry=arena+0x20000+uintptr_t(index)*0x200;put(names.data+uintptr_t(index)*8,entry);put(entry,index);std::array<wchar_t,64> text{};std::copy_n(label,wcslen(label)+1,text.begin());put(entry+20,text);
 };
 const auto cover_class=b::journals[0].id.cls,npc=arena+0x32000,npc_pawn=arena+0x36000,npc_mesh=arena+0x36400;
 object(cover_class,10,arena+0x49f00,0,L"WarAnim_CoverBlend");
 object(npc_pawn,11,b::metadata.identities[b::metadata.role_pawn].cls,b::metadata.identities[b::metadata.role_level].address,L"NPC_Pawn");
 object(npc_mesh,12,arena+0x49e00,npc_pawn,L"NPC_Mesh");object(npc,8,cover_class,npc_mesh,L"NPC_CoverBlend");
 put(npc,base+0x13d0080);put(npc+0x128,npc_pawn);put(npc+0x60,npc_mesh);return npc;
}
enum class Mode {normal,npc,reentrant,reset,reset_npc,expiry,external_post,own_post,own_post_seh,seh_before,seh_after_first,seh_after_last,cpp_after_first,out_of_order};
Mode mode{};uintptr_t other_node{};unsigned original_depth{},original_blends{},original_children{},native_test_count{};
float native_test_weight{},total_first{},total_last{};bool native_test_transition{};
constexpr DWORD synthetic_exception=0xe042c012;
struct SyntheticCpp {unsigned marker;};
HANDLE phase_entered{},phase_release{};unsigned barrier_phase{};
bool barrier_unknown_header{},barrier_wait_ok{},barrier_timeout{},barrier_draining{},barrier_failed{},barrier_gate_blocked{};unsigned barrier_fields{},barrier_busy{};float barrier_w0{},barrier_w4{};
b::CallPacket packet_for(uintptr_t self,int index=0,float incoming=.8f){b::CallPacket p{};p.gpr[1]=self;p.gpr[2]=uint32_t(index);memcpy(p.xmm[2].data(),&incoming,4);return p;}
void phase(unsigned value){
 if(barrier_phase!=value)return;
 C(SetEvent(phase_entered)!=0);C(WaitForSingleObject(phase_release,2000)==WAIT_OBJECT_0);
}
DWORD WINAPI stop_at_phase(void*){
 barrier_wait_ok=WaitForSingleObject(phase_entered,2000)==WAIT_OBJECT_0;if(!barrier_wait_ok)return 1;
 {std::lock_guard<std::mutex> lock(b::mutex);barrier_fields=b::pending.load();barrier_busy=b::pending_count();barrier_w0=weight(0);barrier_w4=weight(4);}
 b::request_drain();
 gears_world::EpochGate::CloseToken close{};barrier_gate_blocked=!world_gate.try_close(close);if(!barrier_gate_blocked)world_gate.open(close);
 barrier_draining=b::draining();if(barrier_timeout){b::drain_tick=GetTickCount64()-5001;barrier_failed=b::drain_failed();}
 if(barrier_unknown_header){auto header=b::journals[0].children;header.data+=0x1000;put(node+0xe4,header);}
 SetEvent(phase_release);return 0;
}
void synthetic_child(b::CallPacket* packet){
 ++original_children;const auto self=uintptr_t(packet->gpr[1]);if(self!=node)return;
 const auto index=unsigned(packet->gpr[2]);float incoming{};memcpy(&incoming,packet->xmm[2].data(),4);
 C(active.load()>0&&b::in_flight.load()>0);C(scope_live>0||!b::enabled||!scope_admitted);
 float total=incoming*weight(index);put(records+uintptr_t(index)*0x44+0x14,total);
 if(original_depth==1&&((mode==Mode::seh_after_first&&index==0)||(mode==Mode::seh_after_last&&index==4)))RaiseException(synthetic_exception,0,0,nullptr);
 if(original_depth==1&&mode==Mode::cpp_after_first&&index==0)throw SyntheticCpp{0xaabbccdd};
}
void invoke_child(uintptr_t self,int index){auto p=packet_for(self,index);b::GearsCoverInvokeProtected(&p,1);}
void synthetic_blend(b::CallPacket* packet){
 ++original_blends;const auto self=uintptr_t(packet->gpr[1]);
 C(active.load()>0&&b::in_flight.load()>0);C(scope_live>0||!b::enabled);
 if(self!=node){invoke_child(self,0);return;}
 ++original_depth;
 if(mode==Mode::seh_before)RaiseException(synthetic_exception,0,0,nullptr);
 if(original_depth>1){invoke_child(self,0);invoke_child(self,4);--original_depth;return;}
 invoke_child(self,0);phase(1);
 if(mode==Mode::npc){const auto before=scope_entries;auto p=packet_for(other_node);b::GearsCoverInvokeProtected(&p,0);C(scope_entries==before+1);C(weight(0)==.5f&&weight(4)==.5f&&b::tick.node==node);}
 if(mode==Mode::reset_npc){C(SetEvent(b::reset_event)!=0);C(SetEvent(b::advance_event)!=0);auto p=packet_for(other_node);b::GearsCoverInvokeProtected(&p,0);C(b::withdrawing&&b::rounds_open());}
 if(mode==Mode::reentrant){auto p=packet_for(node);b::GearsCoverInvokeProtected(&p,0);C(weight(0)==.5f&&weight(4)==.5f&&b::tick.node==node);}
 if(mode==Mode::reset){C(SetEvent(b::reset_event)!=0);}
 if(mode==Mode::expiry)b::last_advance=GetTickCount64()-10001;
 if(mode==Mode::out_of_order)invoke_child(self,0);
 invoke_child(self,4);
 // This models the native CoverBlend post test. Weight must be native here;
 // TotalWeight must remain the sole original child propagation's W product.
 native_test_weight=weight(0);native_test_transition=native_test_weight<.999f;++native_test_count;
 memcpy(&total_first,reinterpret_cast<void*>(records+0x14),4);memcpy(&total_last,reinterpret_cast<void*>(records+4*0x44+0x14),4);
 C(b::transactions.load()>0||!b::enabled||!scope_admitted);C(scope_live>0||!b::enabled);phase(2);
 if(mode==Mode::external_post)put(records+4*0x44+0x10,.33f);
 if(mode==Mode::own_post||mode==Mode::own_post_seh)throwing_read=node+8;
 if(mode==Mode::own_post_seh)RaiseException(synthetic_exception,0,0,nullptr);
 --original_depth;
}
void prepare_round(unsigned target_step=2){
 setup();b::restore_all();other_node=npc_node();put(fixture_world+64,b::metadata.identities[b::metadata.role_level].address);
 b::layout.special=0x119;put(pawn+0x119,static_cast<unsigned char>(0));b::layout_ready=true;b::initial_world=1;anchor_config=b::metadata;
 put(records+8,arena+0x3c000);put(records+4*0x44+8,arena+0x3c400);
 b::enabled=true;b::game_thread=GetCurrentThreadId();b::injecting=true;b::last_advance=GetTickCount64();b::step=target_step;
 b::writes=b::restores=b::external=b::blend_calls=b::child_calls=0;
 b::GearsCoverBlendOriginal=reinterpret_cast<void*>(&synthetic_blend);b::GearsCoverChildOriginal=reinterpret_cast<void*>(&synthetic_child);
 mode=Mode::normal;original_depth=original_blends=original_children=native_test_count=0;native_test_weight=total_first=total_last=0;native_test_transition=false;
 phase_entered=phase_release=nullptr;barrier_phase=0;barrier_unknown_header=barrier_wait_ok=barrier_timeout=barrier_draining=barrier_failed=barrier_gate_blocked=false;barrier_fields=barrier_busy=0;
}
void invoke_round(){auto p=packet_for(node);b::GearsCoverInvokeProtected(&p,0);}
bool catch_round(){
 __try{invoke_round();}
 __except(GetExceptionCode()==synthetic_exception?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){return GetExceptionCode()==synthetic_exception;}
 return false;
}
void balanced(){C(!active.load()&&!scope_live&&!b::call_depth&&!b::current_blend&&!b::in_flight.load()&&!b::transactions.load()&&!world_thread.depth);
 gears_world::EpochGate::CloseToken close{};C(world_gate.try_close(close));C(world_gate.open(close));}
void unlocked(){const bool acquired=b::mutex.try_lock();C(acquired);if(acquired)b::mutex.unlock();}
void forward_only(b::CallPacket*){++original_blends;C(active.load()>0&&b::in_flight.load()>0);}
void phase_stop(unsigned target_step,unsigned at,bool unknown_header=false,bool timeout=false){
 prepare_round(target_step);barrier_phase=at;barrier_unknown_header=unknown_header;barrier_timeout=timeout;
 phase_entered=CreateEventW(nullptr,FALSE,FALSE,nullptr);phase_release=CreateEventW(nullptr,FALSE,FALSE,nullptr);C(phase_entered&&phase_release);
 auto worker=CreateThread(nullptr,0,stop_at_phase,nullptr,0,nullptr);C(worker!=nullptr);invoke_round();C(WaitForSingleObject(worker,2000)==WAIT_OBJECT_0);C(barrier_wait_ok);
 C(CloseHandle(worker)!=0);C(CloseHandle(phase_entered)!=0);C(CloseHandle(phase_release)!=0);phase_entered=phase_release=nullptr;barrier_phase=0;
 balanced();C(barrier_busy>0&&barrier_draining&&barrier_gate_blocked&&b::stopping.load()&&!b::pose_rearms&&original_blends==1&&original_children==2);if(timeout)C(barrier_failed);
 C(native_test_count==1&&native_test_weight==1&&!native_test_transition);
 if(at==1&&target_step==2)C(barrier_w0==.5f&&barrier_w4==.5f&&barrier_fields==2);
 if(at==2)C(barrier_w0==1&&barrier_w4==0&&barrier_fields==(target_step?2u:0u));
 if(unknown_header){
  C(b::pending.load()==2&&b::pending_count()==2&&b::journals[0].native_visible[0]&&b::journals[0].native_visible[4]);
  b::drain_tick=GetTickCount64()-5001;C(b::drain_failed());C(weight(0)==1&&weight(4)==0);
  put(node+0xe4,b::journals[0].children);before_blend(other_node);C(!b::pending_count()&&b::restores==2&&!b::retired);
 }else C(!b::pending_count()&&weight(0)==1&&weight(4)==0);
}
void protected_tests(){
 // Normal parent post sees native weights; pose evaluation receives W after
 // return. TotalWeight is written by the synthetic native exactly once/child.
 prepare_round();std::array<unsigned char,0x400> pawn_bytes{},node_bytes{};memcpy(pawn_bytes.data(),reinterpret_cast<void*>(pawn),pawn_bytes.size());memcpy(node_bytes.data(),reinterpret_cast<void*>(node),node_bytes.size());
 invoke_round();balanced();C(original_blends==1&&original_children==2&&native_test_count==1);
 C(!memcmp(pawn_bytes.data(),reinterpret_cast<void*>(pawn),pawn_bytes.size())&&!memcmp(node_bytes.data(),reinterpret_cast<void*>(node),node_bytes.size()));
 C(native_test_weight==1&&!native_test_transition&&total_first==.4f&&total_last==.4f);
 C(weight(0)==.5f&&weight(4)==.5f&&b::pending_count()==2&&b::writes==4&&b::restores==2&&b::native_posts==1&&b::pose_rearms==1);
 C(!b::journals[0].round_open&&!b::journals[0].native_visible[0]);b::request_drain();before_blend(other_node);C(!b::pending_count()&&weight(0)==1&&weight(4)==0);
 // All seven experimental steps retain their first native-post/pose samples;
 // repeated frames must not consume a later step's fixed evidence budget.
 prepare_round(0);
 for(unsigned s=0;s<b::steps.size();++s){b::step=s;invoke_round();invoke_round();balanced();C(native_test_weight==1&&!native_test_transition);}
 C(b::row_count<=126&&!b::rows_dropped&&!b::pending_count());
 for(unsigned s=0;s<b::steps.size();++s){C(b::journals[0].logged[s][5]==2&&b::journals[0].logged[s][6]==2&&b::step_rows[s]<=18);
  for(unsigned i=0;i<b::row_count;++i){auto const& r=b::rows[i];if(r.step!=s)continue;
   if(r.kind==5)C(r.weight==(r.child==0?1.f:0.f));
   if(r.kind==6)C(r.weight==(r.child==0?1.f-b::steps[s]:b::steps[s]));}}
 // Stop between children and in the native-post window. A neutral no-op round
 // owns no fields but still cannot report cleanup until its transaction ends.
 phase_stop(2,1);phase_stop(2,2);phase_stop(0,2);phase_stop(2,2,true);phase_stop(2,1,false,true);phase_stop(0,2,false,true);
 prepare_round();mode=Mode::reset;b::reset_event=CreateEventW(nullptr,FALSE,FALSE,nullptr);C(b::reset_event!=nullptr);invoke_round();balanced();
 C(!b::pending_count()&&!b::pose_rearms&&native_test_weight==1&&total_first==.4f&&total_last==.4f&&b::withdrawing);C(CloseHandle(b::reset_event)!=0);b::reset_event=nullptr;
 prepare_round(0);mode=Mode::reset_npc;b::reset_event=CreateEventW(nullptr,FALSE,FALSE,nullptr);b::advance_event=CreateEventW(nullptr,FALSE,FALSE,nullptr);C(b::reset_event&&b::advance_event);
 invoke_round();balanced();C(!b::pending_count()&&!b::pose_rearms&&b::withdrawing&&native_test_weight==1);C(WaitForSingleObject(b::advance_event,0)==WAIT_OBJECT_0);
 C(CloseHandle(b::reset_event)!=0);C(CloseHandle(b::advance_event)!=0);b::reset_event=b::advance_event=nullptr;
 prepare_round();mode=Mode::expiry;invoke_round();balanced();C(b::timed_out.load()&&!b::pending_count()&&!b::pose_rearms&&native_test_weight==1&&total_first==.4f&&total_last==.4f);
 // Nested NPCs preserve the target's phase. Same-journal recursion poisons
 // only rearm authority and forwards every original once without half-restore.
 prepare_round();mode=Mode::npc;invoke_round();balanced();C(original_blends==2&&original_children==3&&b::child_calls==2&&b::pose_rearms==1&&weight(0)==.5f);
 b::request_drain();before_blend(other_node);C(!b::pending_count());
 prepare_round();mode=Mode::reentrant;invoke_round();balanced();C(original_blends==2&&original_children==4&&b::reentries==1&&b::child_calls==2);
 C(b::stopping.load()&&b::failed.load()&&!b::pose_rearms&&!b::pending_count()&&weight(0)==1&&weight(4)==0&&native_test_weight==1);
 // External modification in native post causes a failed expected-value CAS;
 // any already rearmed peer rolls back, and the external field survives.
 prepare_round();mode=Mode::external_post;invoke_round();balanced();C(b::external_change.load()&&b::external==1&&!b::pending_count()&&weight(0)==1&&weight(4)==.33f&&!b::pose_rearms);
 prepare_round();mode=Mode::out_of_order;invoke_round();balanced();C(original_children==3&&b::failed.load()&&b::stopping.load()&&!b::pose_rearms&&!b::pending_count()&&weight(0)==1&&weight(4)==0);
 // Original SEH before injection / after native propagation continues outward;
 // finally releases Scope/TLS exactly once and never reinstalls pose weights.
 for(auto value:{Mode::seh_before,Mode::seh_after_first,Mode::seh_after_last}){prepare_round();mode=value;C(catch_round());balanced();C(b::failed.load()&&b::stopping.load()&&!b::pending_count()&&!b::pose_rearms&&weight(0)==1&&weight(4)==0);}
 prepare_round();mode=Mode::cpp_after_first;bool caught{};try{invoke_round();}catch(SyntheticCpp const& e){caught=e.marker==0xaabbccdd;}
 C(caught);balanced();C(b::failed.load()&&!b::pending_count()&&!b::pose_rearms&&weight(0)==1&&weight(4)==0);
 // Closed admission / disabled policy still call each original once while
 // keeping code residency, without constructing any unrelated child Scope.
 prepare_round();gears_world::EpochGate::CloseToken closed{};C(world_gate.try_close(closed));scope_admitted=false;invoke_round();C(world_gate.open(closed));balanced();C(original_blends==1&&original_children==2&&scope_entries==1&&!b::pending_count()&&!b::writes);
 prepare_round();invoke_round();const auto before_writes=b::writes,before_restores=b::restores;const auto before_scopes=scope_entries;
 C(world_gate.try_close(closed));scope_admitted=false;invoke_round();C(world_gate.open(closed));balanced();C(b::pending_count()==2&&b::writes==before_writes&&b::restores==before_restores&&scope_entries==before_scopes+1&&weight(0)==.5f&&weight(4)==.5f);
 scope_admitted=true;b::request_drain();before_blend(other_node);C(!b::pending_count()&&weight(0)==1&&weight(4)==0);
 prepare_round();b::enabled=false;invoke_round();balanced();C(original_blends==1&&original_children==2&&!scope_entries&&!b::pending_count()&&!b::writes);
 // Only our own ordinary C++ preparation/post failures are contained. A
 // constructor throw unwinds its admission; a metadata throw under the mutex
 // destroys lock_guard before the catch. None suppresses/duplicates original.
 prepare_round();b::GearsCoverBlendOriginal=reinterpret_cast<void*>(&forward_only);throw_scope=true;invoke_round();balanced();unlocked();
 C(b::failed.load()&&b::stopping.load()&&original_blends==1&&!b::pending_count()&&!b::writes&&!scope_entries);
 prepare_round();b::GearsCoverBlendOriginal=reinterpret_cast<void*>(&forward_only);throwing_read=node+8;invoke_round();balanced();unlocked();
 C(b::failed.load()&&b::stopping.load()&&original_blends==1&&!b::pending_count()&&!b::writes&&scope_entries==1);
 prepare_round();mode=Mode::own_post;invoke_round();balanced();unlocked();C(b::failed.load()&&b::stopping.load()&&original_blends==1&&original_children==2&&!b::pose_rearms&&b::pending_count()==2&&weight(0)==1&&weight(4)==0);
 before_blend(other_node);C(!b::pending_count()&&b::restores==2);
 prepare_round();mode=Mode::own_post_seh;C(catch_round());balanced();unlocked();C(b::failed.load()&&original_blends==1&&original_children==2&&!b::pose_rearms&&b::pending_count()==2&&weight(0)==1&&weight(4)==0);
 before_blend(other_node);C(!b::pending_count()&&b::restores==2);
}
int main(){
 arena=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,1024*1024,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!arena)return 99;
 // Owned fields restore; native TotalWeight stays untouched.
 setup();C(b::pending_count()==2);b::restore_all();C(!b::pending_count()&&b::restores==2);C(weight(0)==1&&weight(4)==0);float total{};memcpy(&total,reinterpret_cast<void*>(records+0x14),4);C(total==123.25f);
 // An external Weight change releases that field without overwriting it.
 setup();put(records+0x10,.33f);b::restore_all();C(!b::pending_count()&&b::restores==1&&b::external==1&&b::external_change.load());C(weight(0)==.33f&&weight(4)==0);
 // Positive identity change may retire; an unknown read must retain pending.
 setup();put(node+76,1u);b::restore_all();C(!b::pending_count()&&b::restores==0&&b::retired==2);C(weight(0)==.5f&&weight(4)==.5f);
 setup();denied=node+8;b::restore_all();C(b::pending_count()==2&&b::restores==0&&b::stopping.load());denied=0;b::restore_all();C(!b::pending_count()&&b::restores==2);
 // Still-live old objects keep their obligation when context/storage changes.
 setup();put(node+0x60,mesh+0x400);b::restore_all();C(b::pending_count()==2&&b::retired==0&&b::restores==0&&b::stopping.load());C(weight(0)==.5f&&weight(4)==.5f);put(node+0x60,mesh);b::restore_all();C(!b::pending_count()&&b::restores==2);
 setup();b::Children replacement{records+0x1000,5,5};put(node+0xe4,replacement);b::restore_all();C(b::pending_count()==2&&b::retired==0&&b::restores==0&&b::stopping.load());C(weight(0)==.5f&&weight(4)==.5f);put(node+0xe4,b::journals[0].children);b::restore_all();C(!b::pending_count()&&b::restores==2);
 // Even after a world change, only later proven slot loss permits retirement.
 setup();committed_world_generation=2;b::restore_all();C(b::pending_count()==2&&b::retired==0&&b::restores==0&&b::stopping.load());C(weight(0)==.5f&&weight(4)==.5f);
 b::enabled=true;b::request_drain();b::drain_tick=GetTickCount64()-5001;C(b::drain_failed()&&b::pending_count()==2);
 gears_anchor::Registry registry{};memcpy(&registry,reinterpret_cast<void*>(b::metadata.objects_header),sizeof registry);put(registry.data+uintptr_t(b::journals[0].id.index)*8,uintptr_t(0));b::restore_all();C(!b::pending_count()&&b::retired==2&&b::restores==0);
 setup();put(b::metadata.identities[b::metadata.role_controller].address+b::metadata.controller_pawn,arena+0x32000);b::restore_all();C(b::pending_count()==2&&b::retired==0&&b::restores==0&&b::stopping.load());C(weight(0)==.5f&&weight(4)==.5f);
 gears_anchor::Identity live_old{};C(b::identity(node,live_old)&&b::same(live_old,b::journals[0].id));put(b::metadata.identities[b::metadata.role_controller].address+b::metadata.controller_pawn,pawn);b::restore_all();C(!b::pending_count()&&b::restores==2);
 setup();put(node+0x128,pawn+0x400);b::restore_all();C(b::pending_count()==2&&b::retired==0&&b::restores==0&&b::stopping.load());C(weight(0)==.5f&&weight(4)==.5f);
 setup();put(pawn+0x110,fixture_world+0x400);b::restore_all();C(b::pending_count()==2&&b::retired==0&&b::restores==0&&b::stopping.load());C(weight(0)==.5f&&weight(4)==.5f);
 setup();put(node,base+0x13d0088);b::restore_all();C(b::pending_count()==2&&b::retired==0&&b::restores==0&&b::stopping.load());C(weight(0)==.5f&&weight(4)==.5f);
 // No ownership grants no restore authority; CAS preserves mismatching values.
 setup();auto& j=b::journals[0];j.owned.fill(false);b::recount();b::restore_all();C(!b::pending_count()&&b::restores==0&&weight(0)==.5f);
 setup();uint32_t expected{},observed{};float half=.5f,quarter=.25f;memcpy(&expected,&half,4);uint32_t desired{};memcpy(&desired,&quarter,4);C(b::exchange(records+0x10,expected,desired,observed)&&observed==expected&&weight(0)==quarter);C(b::exchange(records+0x10,expected,0,observed)&&observed==desired&&weight(0)==quarter);C(!b::exchange(records+0x11,expected,0,observed));
 // FName padding is irrelevant; changed text and overlong selected names reject.
 setup();put(node_text+sizeof(L"Synthetic5"),uint64_t(0xffffffffffffffff));gears_anchor::Identity id{};C(b::identity(node,id,true));changing_text=node_text;text_reads=0;C(!b::identity(node,id,true));changing_text=0;
 setup();for(unsigned i=0;i<64;++i)put(node_text+uintptr_t(i)*2,wchar_t(L'A'));put(node_text+128,wchar_t(0));C(!b::identity(node,id));C(b::identity(node,id,false,true));
 // Drain is bounded and never clears pending by timing out.
 setup();b::enabled=true;b::request_drain();C(b::stopping.load()&&b::draining());b::drain_tick=GetTickCount64()-5001;C(b::drain_failed()&&b::pending_count()==2);b::restore_all();C(!b::drain_failed()&&!b::pending_count());

 // Exercise real child_before guard/write logic with synthetic selected layout.
 setup();b::restore_all();b::enabled=true;b::game_thread=GetCurrentThreadId();b::layout.special=0x119;put(pawn+0x119,static_cast<unsigned char>(0));
 b::tick={node,pawn,mesh,fixture_world,b::journals[0].id,b::journals[0].children,1,false};b::injecting=true;b::last_advance=GetTickCount64();b::step=2;fixture_parent.selected=true;fixture_parent.self=node;b::current_blend=&fixture_parent;
 put(records+8,arena+0x3c000);put(records+uintptr_t(4)*0x44+8,arena+0x3c400);b::writes=0;
 child(node,0,0.f);C(!b::pending_count()&&weight(0)==1&&weight(4)==0&&b::writes==0);
 put(pawn+0x119,static_cast<unsigned char>(12));child(node,0,1.f);C(!b::pending_count()&&b::writes==0);put(pawn+0x119,static_cast<unsigned char>(0));
 b::layout.sources[0].address=arena+0x80000;b::layout.sources[0].size=4;uint32_t source_value=100;put(arena+0x80000,source_value);memcpy(b::layout.sources[0].bytes.data(),&source_value,4);b::layout.source_count=1;put(arena+0x80000,uint32_t(101));
 child(node,0,1.f);C(!b::pending_count()&&b::writes==0);b::layout.source_count=0;
 child(node,0,1.f);C(b::pending_count()==2&&weight(0)==.5f&&weight(4)==.5f&&b::writes==2);
 put(records+0x14,.5f);child(node,4,1.f);put(records+uintptr_t(4)*0x44+0x14,.5f);C(b::pending_count()==2&&weight(0)==1&&weight(4)==0);fixture_finish();before_blend(node);C(!b::pending_count()&&weight(0)==1&&weight(4)==0);
 setup();b::restore_all();b::enabled=true;b::game_thread=GetCurrentThreadId();b::layout.special=0x119;put(pawn+0x119,static_cast<unsigned char>(0));
 // Make only the second changed field unwritable: the first CAS must roll back.
 auto split_records=arena+0x51fe8;memcpy(reinterpret_cast<void*>(split_records),reinterpret_cast<void*>(records),5*0x44);records=split_records;b::Children split_header{records,5,5};put(node+0xe4,split_header);b::journals[0].children=split_header;
 put(records+8,arena+0x3c000);put(records+uintptr_t(4)*0x44+8,arena+0x3c400);b::tick={node,pawn,mesh,fixture_world,b::journals[0].id,split_header,1,false};b::injecting=true;b::last_advance=GetTickCount64();b::step=2;fixture_parent.selected=true;fixture_parent.self=node;b::current_blend=&fixture_parent;b::writes=0;b::restores=0;
 DWORD old_protection{};C(VirtualProtect(reinterpret_cast<void*>(arena+0x52000),4096,PAGE_READONLY,&old_protection)!=0);
 child(node,0,1.f);C(b::writes==1&&b::restores==1&&!b::pending_count()&&b::failed.load()&&weight(0)==1&&weight(4)==0);
 DWORD ignored_protection{};C(VirtualProtect(reinterpret_cast<void*>(arena+0x52000),4096,old_protection,&ignored_protection)!=0);
 // Normal NPC entries leave another node's installed Weight and TLS context
 // intact until the target's own next entry, independent of NPC qualification.
 setup();bind_target();auto npc=npc_node();auto saved_tick=b::tick;
 before_blend(npc);C(b::pending_count()==2&&b::restores==0&&weight(0)==.5f&&weight(4)==.5f);
 C(b::tick.node==saved_tick.node&&b::tick.pawn==saved_tick.pawn&&b::tick.generation==saved_tick.generation&&b::tick.injected);
 before_blend(node);C(!b::pending_count()&&b::restores==2&&weight(0)==1&&weight(4)==0);C(!b::tick.node);

 // Both the C entry and direct helper must reject unrelated child receivers
 // before Scope, mutex, and selected-call counting.
 setup();bind_target();npc=npc_node();auto old_calls=b::child_calls;auto old_scopes=scope_entries;
 child(npc,0,1.f);child(npc,0,1.f);
 C(b::child_calls==old_calls&&scope_entries==old_scopes);C(b::pending_count()==2&&b::restores==0&&b::tick.node==node);
 b::enabled=false;child(node,0,1.f);child(node,0,1.f);C(b::child_calls==old_calls&&scope_entries==old_scopes);
 b::enabled=true;child(node,0,0.f);C(b::child_calls==old_calls+1&&scope_entries==old_scopes+1);C(b::pending_count()==2&&weight(0)==.5f);

 // Reset is an explicit all-node withdrawal, but only an already-bound thread
 // may consume it. The queued event survives unknown/wrong-thread entries.
 setup();bind_target();npc=npc_node();b::reset_event=CreateEventW(nullptr,FALSE,TRUE,nullptr);C(b::reset_event!=nullptr);b::game_thread=0;
 before_blend(npc);C(b::pending_count()==2&&b::restores==0&&!b::game_thread.load()&&b::tick.node==node);
 b::game_thread=GetCurrentThreadId()+1;before_blend(npc);C(b::pending_count()==2&&b::restores==0&&b::tick.node==node);
 b::game_thread=GetCurrentThreadId();before_blend(npc);C(!b::pending_count()&&b::restores==2&&!b::injecting&&!b::withdrawing&&!b::tick.node);
 C(CloseHandle(b::reset_event)!=0);b::reset_event=nullptr;

 // Expiry and stop/drain also service peers on the bound thread. Neither path
 // treats a still-live old instance in a changed world as restored/retired.
 setup();bind_target();npc=npc_node();b::last_advance=GetTickCount64()-10001;before_blend(npc);
 C(!b::pending_count()&&b::restores==2&&b::timed_out.load()&&!b::injecting&&!b::withdrawing);
 setup();bind_target();npc=npc_node();b::request_drain();before_blend(npc);C(!b::pending_count()&&b::restores==2&&b::stopping.load());
 setup();bind_target();npc=npc_node();committed_world_generation=2;b::request_drain();before_blend(npc);
 C(b::pending_count()==2&&b::restores==0&&b::retired==0&&weight(0)==.5f);b::drain_tick=GetTickCount64()-5001;C(b::drain_failed()&&b::pending_count()==2);
 memcpy(&registry,reinterpret_cast<void*>(b::metadata.objects_header),sizeof registry);put(registry.data+uintptr_t(b::journals[0].id.index)*8,uintptr_t(0));before_blend(npc);
 C(!b::pending_count()&&b::retired==2&&b::restores==0);

 // A withdrawal blocked by unknown storage keeps the journal and a queued
 // Advance. It cannot inject a new step over an unreleased old field.
 setup();bind_target();npc=npc_node();replacement={records+0x1000,5,5};put(node+0xe4,replacement);
 b::reset_event=CreateEventW(nullptr,FALSE,TRUE,nullptr);b::advance_event=CreateEventW(nullptr,FALSE,TRUE,nullptr);C(b::reset_event&&b::advance_event);
 before_blend(npc);C(b::pending_count()==2&&b::restores==0&&b::retired==0&&b::withdrawing&&b::stopping.load()&&!b::injecting);
 C(WaitForSingleObject(b::advance_event,0)==WAIT_OBJECT_0);C(CloseHandle(b::reset_event)!=0);C(CloseHandle(b::advance_event)!=0);b::reset_event=b::advance_event=nullptr;

 protected_tests();
 C(VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE)!=0);printf("cover_body_live_journal: %u checks, %u failures\n",checks,failures);return failures?1:0;
}
