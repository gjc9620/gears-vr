// Execute the production candidate against an in-memory engine boundary.
// This target neither loads the game nor opens/injects into another process.
#include <windows.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "../vr-common/hand_ik_controlled_trial_policy.h"
namespace {
using uptr=std::uintptr_t;
std::atomic<bool> enabled{true};
std::atomic<DWORD> game_thread{};
FILE* log_file{};
ULONGLONG fake_ms=1000;
ULONGLONG fake_clock(){return fake_ms;}
bool registered(uptr value){return value!=0;}
namespace playground {
bool named(uptr object,const wchar_t* name){
    return (object==100&&!wcscmp(name,L"PlayerTick"))||(object==101&&!wcscmp(name,L"Tick"));
}
}
namespace freehands {
bool read_field(uptr,const wchar_t*,const wchar_t*,void*,unsigned){return false;}
void print_matrix(const wchar_t* field,std::array<float,16> const& value){
    fwprintf(log_file,L" %ls=",field);
    for(unsigned i=0;i<16;++i)fwprintf(log_file,L"%ls%.9g",i?L",":L"",value[i]);
}
}
namespace handik {
using gears_hand_ik::Triple;
constexpr unsigned right_ik=2;
struct Control {
    uptr object{};Triple strength{},effector{};std::vector<unsigned char> fixed;
};
struct Snapshot {
    uptr pc{1},pawn{2},mesh{3},tree{4},weapon{5};
    std::vector<unsigned char> identity{1};
    std::array<Control,8> controls{};
    std::array<float,16> mesh_world{},left{},right{};
    std::uint64_t ms{};
    float world_time{},last_render_time{};
    int mesh_tick{};
    bool control_tick_tag{},mesh_tick_known{true},control_tag_known{true},action_idle{true};
};
struct {unsigned setter_calls{},effector_writes{};} trial;
Snapshot memory;
struct Write {unsigned window{};bool strength{};float value{};Triple target{},hand{};ULONGLONG ms{};};
std::vector<Write> writes;
bool fail_zero{},fail_target_restore{},missing_pulse_mesh_tick{},missing_pulse_render{},no_natural_render{};
bool unknown_snapshot{};
bool freeze_mesh_tick{},alternate_mesh_tick{},unknown_mesh_tick{};
bool same_controls(Snapshot const& a,Snapshot const& b,bool skip_right=false){
    if(a.identity!=b.identity||a.pc!=b.pc||a.pawn!=b.pawn||a.mesh!=b.mesh||a.tree!=b.tree||a.weapon!=b.weapon)return false;
    for(unsigned i=0;i<8;++i){
        if(a.controls[i].fixed!=b.controls[i].fixed||a.controls[i].object!=b.controls[i].object)return false;
        if(skip_right&&i==right_ik)continue;
        if(!gears_hand_ik::exact(a.controls[i].strength,b.controls[i].strength)||
           !gears_hand_ik::exact(a.controls[i].effector,b.controls[i].effector))return false;
    }
    return true;
}
bool baseline_strengths(Snapshot const& value){
    for(unsigned i=0;i<8;++i){
        const float expected=(i%4)<2?1.f:0.f;
        if(!gears_hand_ik::exact(value.controls[i].strength,{{expected,expected,0}}))return false;
    }
    return true;
}
Triple position(std::array<float,16> const& matrix){return {{matrix[12],matrix[13],matrix[14]}};}
bool snapshot(uptr pc,Snapshot& out){
    out=memory;out.ms=fake_ms;out.world_time=float(fake_ms)/1000.f;
    out.last_render_time=no_natural_render?1.f:out.world_time;
    const bool pulse=memory.controls[right_ik].strength[0]!=0;
    if(missing_pulse_mesh_tick&&pulse)out.mesh_tick_known=false;
    if(unknown_mesh_tick)out.mesh_tick_known=false;
    if(missing_pulse_render&&pulse)out.last_render_time=out.world_time-.08f;
    return !unknown_snapshot&&pc==memory.pc;
}
bool read_control(uptr pawn,uptr mesh,uptr tree,unsigned index,Control& out){
    out=memory.controls[index];return pawn==memory.pawn&&mesh==memory.mesh&&tree==memory.tree;
}
bool set_strength(uptr object,float value){
    ++trial.setter_calls;
    if(object!=memory.controls[right_ik].object)return false;
    writes.push_back({0,true,value,{},{},fake_ms});
    if(fail_zero&&value==0)return false;
    memory.controls[right_ik].strength={{value,value,0}};return true;
}
bool write_effector(uptr object,Triple const& expected,Triple const& value){
    if(object!=memory.controls[right_ik].object||
       !gears_hand_ik::exact(memory.controls[right_ik].effector,expected))return false;
    ++trial.effector_writes;
    writes.push_back({0,false,0,value,position(memory.right),fake_ms});
    if(fail_target_restore&&value==Triple{{900,800,700}})return false;
    memory.controls[right_ik].effector=value;return true;
}
void reset(){trial={};}
}
#define GetTickCount64 fake_clock
#include "vr_hand_ik_controlled_trial.inl"
#undef GetTickCount64
std::wstring last_log;
void reset_test(){
    if(log_file)fclose(log_file);
    assert(tmpfile_s(&log_file)==0&&log_file);
    handikcontrolled::reset();fake_ms=1000;
    handik::memory={};handik::writes.clear();
    handik::fail_zero=handik::fail_target_restore=handik::missing_pulse_mesh_tick=false;
    handik::missing_pulse_render=handik::no_natural_render=false;
    handik::unknown_snapshot=false;
    handik::freeze_mesh_tick=handik::alternate_mesh_tick=handik::unknown_mesh_tick=false;
    for(unsigned i=0;i<8;++i){
        auto& value=handik::memory.controls[i];value.object=10+i;value.fixed={static_cast<unsigned char>(i)};
        const float strength=(i%4)<2?1.f:0.f;value.strength={{strength,strength,0}};
    }
    handik::memory.controls[2].effector={{900,800,700}};
    handik::memory.mesh_world[0]=handik::memory.mesh_world[5]=handik::memory.mesh_world[10]=handik::memory.mesh_world[15]=1;
    handik::memory.left=handik::memory.right=handik::memory.mesh_world;
    handik::memory.left[12]=10;handik::memory.right[12]=20;
    game_thread=GetCurrentThreadId();
    handikcontrolled::safe_to_unload=false;handikcontrolled::status=1;handikcontrolled::requested=true;
}
void frame(unsigned elapsed=40){
    fake_ms+=elapsed;
    if(!handik::freeze_mesh_tick)++handik::memory.mesh_tick;
    if(handik::alternate_mesh_tick)handik::memory.mesh_tick&=1;
    // Unrelated natural animation affects both bones throughout the experiment.
    // A controlled recording still must never report a causal IK effect.
    handik::memory.left[12]+=.003f;handik::memory.right[12]+=.004f;
    handikcontrolled::TickScope pc_tick(handik::memory.pc,100);
    handikcontrolled::pre_tick(handik::memory.pc);
    {handikcontrolled::TickScope pawn_tick(handik::memory.pawn,101);}
    handikcontrolled::post_tick(handik::memory.pc);
    assert(handikcontrolled::status.load()!=4);
}
void until_pulse(){
    for(unsigned i=0;i<130&&handikcontrolled::status.load()==1;++i)frame();
    assert(handikcontrolled::status.load()==2&&handikcontrolled::trial.dirty);
}
void until_terminal(){
    for(unsigned i=0;i<500;++i){
        const auto state=handikcontrolled::status.load();
        if(state==5||state==6||state==7||state==8)return;
        frame();
    }
    assert(false);
}
std::wstring terminal(){
    fflush(log_file);rewind(log_file);last_log.clear();
    wchar_t buffer[2048]{};
    while(fgetws(buffer,2048,log_file))last_log+=buffer;
    const auto start=last_log.find(L"hand_ik_controlled_trial_end ");
    assert(start!=std::wstring::npos&&last_log.find(L"hand_ik_controlled_trial_end ",start+1)==std::wstring::npos);
    return last_log.substr(start,last_log.find(L'\n',start)-start);
}
void safe_incomplete(){
    assert(handikcontrolled::status.load()==7&&handikcontrolled::safe_to_unload.load());
    assert(!handikcontrolled::trial.dirty&&!handikcontrolled::trial.evidence.complete());
    assert(gears_hand_ik::exact(handik::memory.controls[2].effector,{{900,800,700}}));
    assert(gears_hand_ik::zero_strength(handik::memory.controls[2].strength));
}
}
int main(){
    reset_test();until_terminal();
    assert(handikcontrolled::status.load()==5&&handikcontrolled::safe_to_unload.load());
    assert(handikcontrolled::trial.evidence.complete()&&!gears_hand_ik_controlled::effect_proved());
    assert(handik::trial.setter_calls==12&&handik::trial.effector_writes==12&&handik::writes.size()==24);
    assert(terminal()==L"hand_ik_controlled_trial_end status=5 completed_windows=6 evidence_complete=1 restored=1 setter_calls=12 effector_writes=12 effect_evidence=0 inconclusive=1 safe_to_unload=1 reason=six_windows_recorded_offline_analysis_required");
    for(unsigned i=0;i<6;++i){
        const auto& target=handik::writes[i*4];const auto& pulse=handik::writes[i*4+1];
        const auto& zero=handik::writes[i*4+2];const auto& restore=handik::writes[i*4+3];
        assert(!target.strength&&pulse.strength&&zero.strength&&!restore.strength);
        assert(target.target[0]==target.hand[0]+float(gears_hand_ik_controlled::offset(i)));
        assert(pulse.value==.1f&&zero.value==0);
        assert(gears_hand_ik::exact(restore.target,{{900,800,700}}));
        assert(zero.ms==restore.ms&&zero.ms>pulse.ms&&zero.ms-pulse.ms<=400);
        if(i)assert(target.ms-handik::writes[i*4-1].ms>=250);
    }
    assert(last_log.find(L"kind=PawnTick phase=post")!=std::wstring::npos);
    assert(last_log.find(L"side=before")!=std::wstring::npos&&last_log.find(L"side=after")!=std::wstring::npos);
    // Pickup/equip can still be active when Start is queued. No initial wait
    // observation may create an anchor, pad qualification or write a field.
    reset_test();handik::memory.action_idle=false;
    for(unsigned i=0;i<10;++i){
        frame();assert(handikcontrolled::status.load()==1&&!handikcontrolled::trial.anchored);
        assert(handikcontrolled::samples.load()==0&&handikcontrolled::trial.stability.ticks()==0);
        assert(handik::writes.empty());
    }
    const auto original_began=handikcontrolled::trial.began_ms;
    handik::memory.action_idle=true;frame();
    const auto became_ready=fake_ms;
    assert(handikcontrolled::trial.anchored&&handikcontrolled::trial.stability.ticks()==1);
    assert(handikcontrolled::trial.began_ms==original_began);
    assert(handikcontrolled::trial.wait_deadline==became_ready+gears_hand_ik_controlled::wait_limit_ms);
    for(unsigned i=0;i<6;++i){frame();assert(handik::writes.empty());}
    until_pulse();assert(handik::writes[0].ms-became_ready>=250);
    until_terminal();assert(handikcontrolled::trial.evidence.complete());
    terminal();assert(last_log.find(L"reason=action_not_idle")!=std::wstring::npos);
    assert(last_log.find(L"strength0=1,1,0")!=std::wstring::npos&&last_log.find(L"strength7=0,0,0")!=std::wstring::npos);
    // Initial unknown snapshots and transient strengths are also bounded,
    // read-only waits; a fresh anchor can be acquired only after full recovery.
    reset_test();handik::unknown_snapshot=true;
    for(unsigned i=0;i<10;++i){frame();assert(!handikcontrolled::trial.anchored&&handik::writes.empty());}
    handik::unknown_snapshot=false;frame();
    assert(handikcontrolled::trial.stability.ticks()==1&&handik::writes.empty());
    until_terminal();assert(handikcontrolled::trial.evidence.complete());
    terminal();assert(last_log.find(L"snapshot_ok=0 action_idle_known=0")!=std::wstring::npos);
    reset_test();handik::memory.controls[4].strength={{0,0,0}};
    for(unsigned i=0;i<10;++i){
        handik::memory.identity={static_cast<unsigned char>(i)};
        frame();assert(!handikcontrolled::trial.anchored&&handik::writes.empty());
    }
    handik::memory.controls[4].strength={{1,1,0}};frame();
    assert(handikcontrolled::trial.anchored&&handikcontrolled::trial.stability.ticks()==1);
    until_terminal();assert(handikcontrolled::trial.evidence.complete());
    terminal();assert(last_log.find(L"strength_mismatch_mask=16 reason=strengths_not_baseline")!=std::wstring::npos);
    reset_test();handik::memory.action_idle=false;until_terminal();safe_incomplete();
    assert(!handikcontrolled::trial.anchored&&handikcontrolled::samples.load()==0&&handik::writes.empty());
    assert(terminal().find(L"reason=initial_baseline_wait_timeout")!=std::wstring::npos);
    assert(handikcontrolled::trial.wait_observations>0&&handikcontrolled::trial.wait_nonidle>0);
    reset_test();handik::unknown_snapshot=true;until_terminal();safe_incomplete();assert(handik::writes.empty());
    assert(terminal().find(L"reason=initial_baseline_wait_timeout")!=std::wstring::npos);
    reset_test();handik::memory.controls[4].strength={{0,0,0}};until_terminal();safe_incomplete();
    assert(handik::writes.empty()&&!handikcontrolled::trial.anchored);
    reset_test();handik::memory.action_idle=false;frame();handikcontrolled::stop_requested=true;frame();
    assert(handikcontrolled::status.load()==6&&handikcontrolled::safe_to_unload.load()&&handik::writes.empty());
    reset_test();frame();handik::memory.action_idle=false;frame();safe_incomplete();assert(handik::writes.empty());
    // The initial wait never extends the overall limit, even after a baseline
    // becomes ready and its separate qualification deadline is refreshed.
    reset_test();handik::memory.action_idle=false;frame();
    const auto initial_began=handikcontrolled::trial.began_ms;
    frame(4800);assert(!handikcontrolled::trial.anchored&&handik::writes.empty());
    handik::memory.action_idle=true;frame();
    assert(handikcontrolled::trial.began_ms==initial_began&&handikcontrolled::trial.stability.ticks()==1);
    handikcontrolled::trial.wait_deadline=initial_began+20000;
    fake_ms=initial_began+gears_hand_ik_controlled::total_limit_ms;
    frame(1);safe_incomplete();assert(handik::writes.empty());
    // Different millisecond stamps and advancing rendering are insufficient:
    // baseline eligibility needs eight different known natural mesh Ticks.
    reset_test();handik::freeze_mesh_tick=true;
    for(unsigned i=0;i<10;++i){frame();assert(handik::writes.empty());}
    assert(handikcontrolled::trial.natural.qualification.summary().distinct_mesh_ticks==1);
    handik::freeze_mesh_tick=false;
    for(unsigned i=0;i<6;++i){frame();assert(handik::writes.empty());}
    assert(handikcontrolled::trial.natural.qualification.summary().distinct_mesh_ticks==7);
    until_pulse();assert(handikcontrolled::trial.window_evidence.before.distinct_mesh_ticks==8);
    until_terminal();assert(handikcontrolled::trial.evidence.complete());
    reset_test();handik::alternate_mesh_tick=true;until_terminal();safe_incomplete();
    assert(handik::writes.empty()&&handikcontrolled::trial.natural.qualification.summary().distinct_mesh_ticks==2);
    terminal();assert(last_log.find(L"hand_ik_controlled_trial_qualification_timeout phase=1")!=std::wstring::npos);
    assert(last_log.find(L"distinct_mesh_ticks=2")!=std::wstring::npos);
    reset_test();handik::unknown_mesh_tick=true;until_terminal();safe_incomplete();
    assert(handik::writes.empty()&&handikcontrolled::trial.natural.qualification.summary().distinct_mesh_ticks==0);
    reset_test();frame();handik::memory.identity={2};frame();safe_incomplete();assert(handik::writes.empty());
    reset_test();until_pulse();handik::memory.action_idle=false;frame();safe_incomplete();assert(handik::writes.size()==4);
    reset_test();until_pulse();handik::memory.mesh_world[12]=1;frame();safe_incomplete();assert(handik::writes.size()==4);
    reset_test();until_pulse();handik::memory.controls[4].strength={{0,0,0}};frame();safe_incomplete();
    assert(handik::writes.size()==4&&gears_hand_ik::zero_strength(handik::memory.controls[4].strength));
    reset_test();until_pulse();handik::memory.controls[7].effector={{7,8,9}};frame();safe_incomplete();
    assert(handik::writes.size()==4&&gears_hand_ik::exact(handik::memory.controls[7].effector,{{7,8,9}}));
    reset_test();until_pulse();frame(401);safe_incomplete();assert(handik::writes.size()==4);
    reset_test();until_pulse();handikcontrolled::stop_requested=true;frame();
    assert(handikcontrolled::status.load()==6&&handikcontrolled::safe_to_unload.load()&&handik::writes.size()==4);
    reset_test();until_pulse();handik::memory.controls[2].effector[0]+=.001f;frame();
    assert(handikcontrolled::status.load()==8&&!handikcontrolled::safe_to_unload.load()&&handik::writes.size()==2);
    reset_test();until_pulse();handik::memory.controls[2].strength[0]+=.001f;frame();
    assert(handikcontrolled::status.load()==8&&!handikcontrolled::safe_to_unload.load()&&handik::writes.size()==2);
    reset_test();until_pulse();handik::memory.identity={2};frame();
    assert(handikcontrolled::status.load()==8&&!handikcontrolled::safe_to_unload.load()&&handik::writes.size()==2);
    reset_test();until_pulse();handik::fail_zero=true;frame();
    assert(handikcontrolled::status.load()==8&&!handikcontrolled::safe_to_unload.load()&&handik::writes.size()==3);
    reset_test();until_pulse();handik::fail_target_restore=true;frame();
    assert(handikcontrolled::status.load()==8&&!handikcontrolled::safe_to_unload.load()&&handik::writes.size()==4);
    reset_test();until_pulse();frame();assert(handikcontrolled::status.load()==3);
    handik::memory.controls[2].effector[0]+=.001f;frame();
    assert(handikcontrolled::status.load()==7&&handikcontrolled::safe_to_unload.load()&&handik::writes.size()==4);
    reset_test();handik::missing_pulse_mesh_tick=true;until_terminal();
    assert(handikcontrolled::status.load()==5&&handikcontrolled::trial.evidence.completed()==6);
    assert(!handikcontrolled::trial.evidence.complete()&&handikcontrolled::safe_to_unload.load());
    assert(terminal().find(L"evidence_complete=0")!=std::wstring::npos);
    reset_test();handik::missing_pulse_render=true;until_terminal();
    assert(handikcontrolled::status.load()==5&&!handikcontrolled::trial.evidence.complete());
    reset_test();handik::no_natural_render=true;until_terminal();safe_incomplete();assert(handik::writes.empty());
    fclose(log_file);log_file=nullptr;
    std::puts("Actual controlled IK state machine passed: six windows, rollback order, delays, cancellation and ownership failures");
}
