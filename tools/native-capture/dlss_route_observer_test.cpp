// Offline execution of the actual hook wrappers with synthetic memory/TLS.
// Never loads the game, creates a detour, or executes native game code.
#include <Windows.h>
#include <bcrypt.h>
#include <intrin.h>
#include <unknwn.h>
#include <atomic>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <stdexcept>
#include <thread>
#include "dlss_output_control.h"
namespace {
uintptr_t base=0x140000000;bool self_testing{},continuous_session=true,frame_link_diagnostics=true,viewport_diagnostics=true;
std::atomic<bool> recording{true};std::atomic<uint64_t> committed_world_generation{9};
thread_local void* consuming_renderer{};thread_local uint64_t consuming_frame_ticket=7;
struct Frozen {bool enabled=true;uint64_t id=7,world_generation=9;};thread_local Frozen consuming_frozen_frame;
std::recursive_mutex frame_hook_mutex;bool admitted=true;
std::atomic<unsigned> active{};
struct Scope {Scope(){++active;}~Scope(){--active;}bool world()const{return admitted;}};
long long ticks{};long long now(){return ++ticks;}
#if defined(GEARS_DLSS_BIND_TEST_RUNTIME)
uintptr_t input_public_cell{},input_alternate_cell{};
bool input_public_missing{};
bool input_hash_fixture{},input_hash_tail_missing{};std::array<unsigned char,503> input_hash_bytes{};
#endif
#if defined(GEARS_DLSS_INPUT_BIND_OBSERVER_TEST)
uint32_t test_input_bind_output();void run_input_bind_cases();
#endif
bool safe_copy(void* out,void const* in,size_t size){
#if defined(GEARS_DLSS_BIND_TEST_RUNTIME)
    const auto at=reinterpret_cast<uintptr_t>(in);
    if(input_hash_fixture&&at==base+0x1665dc&&size==0x1cc){std::memcpy(out,input_hash_bytes.data(),size);return true;}
    if(input_hash_fixture&&at==base+0x8b492c&&size==0x2b){if(input_hash_tail_missing)return false;std::memcpy(out,input_hash_bytes.data()+0x1cc,size);return true;}
    if(size==8&&(at==base+0x1c3b410||at==base+0x1c3b488)){if(at==base+0x1c3b410&&input_public_missing)return false;
        std::memcpy(out,at==base+0x1c3b410?&input_public_cell:&input_alternate_cell,8);return true;}
#endif
    __try{std::memcpy(out,in,size);return true;}__except(EXCEPTION_EXECUTE_HANDLER){return false;}}
uintptr_t fake_root{},fake_caller{};
uintptr_t test_readgsqword(unsigned long){return fake_root;}
void* test_return_address(){return reinterpret_cast<void*>(base+fake_caller);}
#if defined(GEARS_DLSS_OUTPUT_CONTROL_TEST)
bool owned_bindings_enabled{},owned_xr_probe{},first_view_mask_control{},submit_duplicate_control{},submit_mono_control{},submit_mono_independent_state{},projection_control{},origin_trial_enabled{};
bool stereo_xr=true,stereo_gpu_publish=true,submit_pair_enabled=true;
bool quad_source_control_enabled(){return false;}
struct FakeTargets {bool descriptor_capacity_lost()const{return false;}} frame_target_stats;
struct FakeLate {bool active{};std::array<float,8> args{};std::array<unsigned,4> dimensions{};uintptr_t caller{};} late_quad_candidate;
struct ResourceIdentity {
    uintptr_t native{},canonical{};long result=-1;
    struct Desc {unsigned Dimension=3;uint64_t Width=2560;unsigned Height=1080,DepthOrArraySize=1,MipLevels=1,Format=24;struct Samples {unsigned Count=1;} SampleDesc;} desc;
};
bool source_available=true;unsigned source_width=2560,source_format=24;
uintptr_t source_native=0xabc,source_canonical=0x1234;
unsigned source_query_calls{};
uint64_t fake_list_generation=4;uintptr_t fake_list_canonical=0x500;bool fake_system_methods=true,drift_list_generation{};
std::array<void*,4> viewport_targets{};
bool system_resource_methods(uintptr_t value){return value&&fake_system_methods;}
uintptr_t canonical_identity(IUnknown* value){return value?fake_list_canonical:0;}
struct FakeFrameLink {gears_frame_link::ListRef current(uintptr_t value)const{return {value,fake_list_generation};}} frame_link;
void query_bound_resource(uintptr_t object,ResourceIdentity& out){
    ++source_query_calls;
    if(object&&source_available){out.result=0;out.native=source_native;out.canonical=source_canonical;out.desc.Width=source_width;out.desc.Format=source_format;}
    if(drift_list_generation)++fake_list_generation;
}
uint32_t test_control_output();void run_control_cases();
#endif
}
#define GEARS_DLSS_ROUTE_OBSERVER 1
#define __readgsqword test_readgsqword
#define _ReturnAddress test_return_address
#include "dlss_route_observer.inl"
#include "dlss_output_control.inl"
#undef _ReturnAddress
#undef __readgsqword
namespace {
using namespace gears_dlss_route;
alignas(8) std::array<unsigned char,0x1a0> renderer{};
alignas(8) std::array<unsigned char,0x1a60> views{};
alignas(8) std::array<unsigned char,0x1500> context{};
alignas(8) std::array<unsigned char,0x28> slot{};
alignas(8) std::array<unsigned char,0xb0> owner{};
alignas(8) std::array<unsigned char,0x50> family{},bank{};
alignas(8) std::array<unsigned char,0x100> data{};
std::array<uintptr_t,16> table{};uintptr_t first_slot{};
Float4 payload{1,2,3,4};unsigned output_calls{},setter_calls{},mode{};void* expected_view{};void* expected_pass{};
unsigned output_lock_probes{},setter_lock_probes{};
FILE* timeout_report{};
void assert_frame_mutex_unlocked(){
    bool acquired=false;
    std::thread probe([&]{acquired=frame_hook_mutex.try_lock();if(acquired)frame_hook_mutex.unlock();});
    probe.join();assert(acquired); // A different thread must acquire a recursive mutex.
}
template<class T,size_t N> void put(std::array<unsigned char,N>& bytes,size_t offset,T const& value){assert(offset+sizeof(T)<=N);std::memcpy(bytes.data()+offset,&value,sizeof(T));}
const uint16_t* descriptor(){return reinterpret_cast<const uint16_t*>(owner.data()+0xaa);}
uint64_t __fastcall original_setter(uintptr_t rcx,const uint16_t* desc,const void* source,unsigned index){
    assert_frame_mutex_unlocked();++setter_lock_probes;
    ++setter_calls;assert(rcx==0xdead&&desc==descriptor()&&source==payload.data()&&index==0);
    if(!gears_dlss_output::noop({desc[0],desc[1],desc[2]},index))std::memcpy(data.data()+16,source,16);
    if(mode==4)throw std::runtime_error("setter");return 0x123456789abcdef0ull;
}
uint32_t __fastcall original_output(void* pass,uintptr_t unused,void* input){
    assert_frame_mutex_unlocked();++output_lock_probes;
    ++output_calls;assert(pass==expected_pass&&input==expected_view&&unused==0xfeed);
#if defined(GEARS_DLSS_INPUT_BIND_OBSERVER_TEST)
    if(mode>=100)return test_input_bind_output();
#endif
#if defined(GEARS_DLSS_OUTPUT_CONTROL_TEST)
    if(mode>=10)return test_control_output();
#endif
    if(mode==1){mode=0;auto* parent=dlss_route_observer::current;
        assert(parent&&parent->row);assert(dlss_route_observer::hook_output(pass,unused,input)==0xfedcba98u);
        assert(dlss_route_observer::current==parent);mode=1;}
    if(mode==5){mode=2;auto* parent=dlss_route_observer::current;
        try{dlss_route_observer::hook_output(pass,unused,input);assert(false);}catch(std::runtime_error const&){}
        assert(dlss_route_observer::current==parent);mode=5;}
    if(mode==2)throw std::runtime_error("output");
    if(mode==6){
        recording=false;assert(active.load()==1&&timeout_report);
        // Simulate the cleanup deadline with this Output owner still in flight.
        std::thread printer([]{dlss_route_observer::print(timeout_report);});printer.join();
        assert(active.load()==1);
    }
    if(mode==3||mode==4){fake_caller=0xd83b69;
        assert(dlss_route_observer::hook_setter(0xdead,descriptor(),payload.data(),0)==0x123456789abcdef0ull);
        payload={8,8,8,8}; // Saved payload must survive reused source storage.
        dlss_route_observer::quad(0xd83cbf,{1,2,3,4,5,6,7,8},{2560,1080,2560,1080});}
    return 0xfedcba98u;
}
void reset(){
    assert(active.load()==0);
    dlss_route_observer::recorder={};dlss_route_observer::attested=true;dlss_route_observer::printed=false;dlss_route_observer::current=nullptr;
    dlss_route_observer::original_output=original_output;dlss_route_observer::original_setter=original_setter;
    admitted=true;recording=true;frame_link_diagnostics=true;consuming_frame_ticket=7;consuming_frozen_frame={};committed_world_generation=9;
    output_calls=setter_calls=mode=output_lock_probes=setter_lock_probes=0;payload={1,2,3,4};renderer={};views={};context={};slot={};owner={};family={};bank={};data={};table={};
    consuming_renderer=renderer.data();expected_pass=owner.data();expected_view=views.data();
    first_slot=reinterpret_cast<uintptr_t>(slot.data());fake_root=reinterpret_cast<uintptr_t>(&first_slot);
    put(slot,0x20,reinterpret_cast<uintptr_t>(context.data()));put(slot,0x10,uint8_t(1));
    put(context,0x14c0,reinterpret_cast<uintptr_t>(views.data()));put(context,0x13ac,reinterpret_cast<uintptr_t>(table.data()));
    put(renderer,0x68,reinterpret_cast<uintptr_t>(views.data()));put(renderer,0x70,int32_t(2));
    for(unsigned eye=0;eye<2;++eye){const auto offset=eye*0xd30;
        put(views,offset,reinterpret_cast<uintptr_t>(family.data()));put(views,offset+0x48,Float4{1,2,3,4});
        put(views,offset+0x58,std::array<int32_t,4>{0,0,1280,1080});put(views,offset+0x3d0,Float4{5,6,7,8});}
    put(family,0x4c,uint32_t(12));put(owner,8,uint32_t(0x100));put(owner,0xaa,Descriptor{1,16,16});
    table[1]=reinterpret_cast<uintptr_t>(bank.data());put(bank,0x3c,reinterpret_cast<uintptr_t>(data.data()));put(bank,0x44,uint32_t(32));
}
uint32_t call(){fake_caller=0xabcd;return dlss_route_observer::hook_output(expected_pass,0xfeed,expected_view);}
void assert_report(FILE* report,const char* expected,bool has_rows){
    assert(!fflush(report)&&!fseek(report,0,SEEK_SET));std::array<char,16384> text{};
    const auto size=fread(text.data(),1,text.size()-1,report);assert(size<text.size()-1);
    assert(std::strstr(text.data(),expected));assert((std::strstr(text.data(),"dlss_route_output row=")!=nullptr)==has_rows);
}
}
int main(){
    reset();assert(call()==0xfedcba98u&&output_calls==1&&dlss_route_observer::current==nullptr);
    assert(output_lock_probes==1);
    auto const& first=dlss_route_observer::recorder.rows[0];assert(first.exited&&!first.unwinding&&first.result==0xfedcba98u&&first.before.same_current);
    reset();admitted=false;assert(call()==0xfedcba98u&&output_calls==1&&!dlss_route_observer::recorder.size);
    assert(output_lock_probes==1);
    reset();consuming_frame_ticket=8;assert(call()==0xfedcba98u&&output_calls==1&&!dlss_route_observer::recorder.size);
    reset();consuming_frozen_frame.world_generation=10;assert(call()==0xfedcba98u&&!dlss_route_observer::recorder.size);
    reset();expected_view=nullptr;assert(call()==0xfedcba98u&&dlss_route_observer::recorder.missing==1&&!dlss_route_observer::recorder.size);
    assert(output_lock_probes==1);
    expected_view=views.data();assert(call()==0xfedcba98u&&dlss_route_observer::recorder.size==1);
    reset();mode=1;assert(call()==0xfedcba98u&&output_calls==2&&dlss_route_observer::recorder.size==1&&dlss_route_observer::recorder.nested==1&&!dlss_route_observer::current);
    assert(output_lock_probes==2);
    reset();mode=2;try{call();assert(false);}catch(std::runtime_error const&){}
    assert(output_calls==1&&!dlss_route_observer::current&&dlss_route_observer::recorder.rows[0].exited&&dlss_route_observer::recorder.rows[0].unwinding);
    reset();mode=5;assert(call()==0xfedcba98u&&output_calls==2&&dlss_route_observer::recorder.nested==1&&!dlss_route_observer::current);
    reset();mode=3;assert(call()==0xfedcba98u&&output_calls==1&&setter_calls==1);
    assert(output_lock_probes==1&&setter_lock_probes==1);
    const auto& r=dlss_route_observer::recorder.rows[0];assert(r.setter_count==1&&r.quad_count==1&&r.setters[0].mask==3);
    assert(r.setters[0].payload==Float4({1,2,3,4})&&r.setters[0].result==0x123456789abcdef0ull&&r.setters[0].after.readable);
    assert(r.quads[0].descriptor_matches[0]&&r.quads[0].consumed[0].value==r.setters[0].payload);
    reset();mode=3;table[1]=0;assert(call()==0xfedcba98u&&setter_calls==1);
    assert(!dlss_route_observer::recorder.rows[0].setters[0].after.readable&&dlss_route_observer::recorder.rows[0].setter_count==1);
    reset();mode=4;try{call();assert(false);}catch(std::runtime_error const&){}
    assert(output_calls==1&&setter_calls==1&&!dlss_route_observer::current&&dlss_route_observer::recorder.rows[0].setters[0].unwinding);
    reset();for(unsigned i=0;i<11;++i)assert(call()==0xfedcba98u);
    assert(output_calls==11&&dlss_route_observer::recorder.size==8&&dlss_route_observer::recorder.limited==3);
    assert(output_lock_probes==11); // Includes the three exhausted-budget bypasses.
    reset();put(context,0x14c0,reinterpret_cast<uintptr_t>(views.data()+0xd30));assert(call()==0xfedcba98u);
    assert(dlss_route_observer::recorder.size==1&&!dlss_route_observer::recorder.rows[0].before.same_current);
    assert(dlss_route_observer::recorder.rows[0].before.eye==0&&dlss_route_observer::recorder.rows[0].before.native.eye==1);
    reset();alignas(8) auto separate_view=views;expected_view=separate_view.data();assert(call()==0xfedcba98u);
    assert(dlss_route_observer::recorder.size==1&&!dlss_route_observer::recorder.rows[0].before.eye_valid&&dlss_route_observer::recorder.rows[0].before.native.eye_valid);
    reset();payload={1,2,3,4};assert(dlss_route_observer::hook_setter(0xdead,descriptor(),payload.data(),0)==0x123456789abcdef0ull&&setter_calls==1);
    assert(dlss_route_observer::setter_role(0xd83a92)==0&&dlss_route_observer::setter_role(0xd83b69)==1&&dlss_route_observer::setter_role(0)==2);
    FILE* report{};assert(!tmpfile_s(&report)&&report);
    // Exercise fresh safe_copy+SHA256 verification, including mismatch rejection.
    const char abc[]="abc";base=reinterpret_cast<uintptr_t>(abc)-0x100;
    assert(dlss_route_observer::verify_chunk(report,0x100,3,"ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));
    assert(!dlss_route_observer::verify_chunk(report,0x100,3,"wrong"));
    assert(!dlss_route_observer::verify_chunk(report,0x100,0x1001,"wrong"));base=0x140000000;
    reset();assert(call()==0xfedcba98u);recording=false;dlss_route_observer::print(report);const auto length=ftell(report);
    dlss_route_observer::print(report);assert(ftell(report)==length);fclose(report);
    assert(!tmpfile_s(&report)&&report);reset();assert(call()==0xfedcba98u);recording=false;dlss_route_observer::print(report);
    assert_report(report,"rows_unavailable=0",true);fclose(report);
    assert(!tmpfile_s(&report)&&report);reset();assert(call()==0xfedcba98u);dlss_route_observer::print(report);
    assert_report(report,"rows_unavailable=1",false);fclose(report); // recording is still open, even though active==0.
    assert(!tmpfile_s(&timeout_report)&&timeout_report);reset();mode=6;assert(call()==0xfedcba98u&&active.load()==0);
    assert_report(timeout_report,"rows_unavailable=1",false);fclose(timeout_report);timeout_report=nullptr;
    assert(dlss_route_observer::recorder.rows[0].exited); // Owner completed safely after the timeout print.
    mode=0;assert(call()==0xfedcba98u&&dlss_route_observer::recorder.size==1&&active.load()==0); // Closed-recording bypass cannot write a new row.
#if defined(GEARS_DLSS_OUTPUT_CONTROL_TEST)
    run_control_cases();
#else
    std::array<float,8> unchanged{1,2,3,4,5,6,7,8};
    dlss_output_control::QuadScope disabled(true,0xd83cbf,unchanged,{2560,1080,2560,1080});
    disabled.forward(unchanged[0],unchanged[1],unchanged[2],unchanged[3],unchanged[4],unchanged[5],unchanged[6],unchanged[7]);
    assert((!disabled.candidate()&&unchanged==std::array<float,8>({1,2,3,4,5,6,7,8})));
#endif
#if defined(GEARS_DLSS_INPUT_BIND_OBSERVER_TEST)
    run_input_bind_cases();
#endif
    std::puts("DlssRouteObserverTest passed (actual wrappers; synthetic TLS/memory only)");
}
