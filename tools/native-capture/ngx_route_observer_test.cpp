// Actual wrappers with synthetic TLS/structures/cells only. No detours/game.
#include <Windows.h>
#include <bcrypt.h>
#include <intrin.h>
#include <atomic>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <stdexcept>
#include <thread>
#include "ngx_route_observer.h"
#include "ngx_stereo_host_live.h"
namespace {
using namespace gears_ngx_route;
uintptr_t base=0x140000000;bool self_testing{},continuous_session=true,frame_link_diagnostics=true,admitted=true;
std::atomic<bool> recording{true};std::atomic<unsigned> active{};std::atomic<uint64_t> committed_world_generation{9};
thread_local void* consuming_renderer{};thread_local uint64_t consuming_frame_ticket=7;
struct Frozen {bool enabled=true;uint64_t id=7,world_generation=9;ngx_stereo_host::NativeCreateSource ngx_constructor_source{};};thread_local Frozen consuming_frozen_frame;
std::recursive_mutex frame_hook_mutex;
struct Scope {Scope(){++active;}~Scope(){--active;}bool world()const{return admitted;}};
long long ticks{};long long now(){return ++ticks;}
alignas(8) std::array<unsigned char,0xd8> renderer{};
alignas(8) std::array<unsigned char,0x1a60> views{};
alignas(8) std::array<unsigned char,0x1500> context{};
alignas(8) std::array<unsigned char,0x28> test_tls_slot{};
std::array<unsigned char,0x19> creation{};std::array<unsigned char,0x90> evaluation{};
std::array<unsigned char,create_size> create_code{};std::array<unsigned char,evaluate_size> evaluate_code{};
uintptr_t root{},first_slot{},test_caller{},handle{},parameters{},unreadable{};bool cells_available=true,code_available=true,short_code{};
unsigned mode{},create_calls{},evaluate_calls{},create_probes{},evaluate_probes{};FILE* timeout_report{};
uintptr_t test_readgsqword(unsigned long){return root;}void* test_return_address(){return reinterpret_cast<void*>(test_caller);}
bool safe_copy(void* out,void const* in,size_t size){
    const auto address=reinterpret_cast<uintptr_t>(in);if(address==unreadable&&unreadable)return false;
    if(address==base+handle_cell_rva&&size==8){if(!cells_available)return false;std::memcpy(out,&handle,8);return true;}
    if(address==base+parameter_cell_rva&&size==8){if(!cells_available)return false;std::memcpy(out,&parameters,8);return true;}
    if(address==base+create_rva&&size==create_code.size()){if(!code_available||short_code)return false;std::memcpy(out,create_code.data(),size);return true;}
    if(address==base+evaluate_rva&&size==evaluate_code.size()){if(!code_available||short_code)return false;std::memcpy(out,evaluate_code.data(),size);return true;}
    __try{std::memcpy(out,in,size);return true;}__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
template<class T,size_t N> void put(std::array<unsigned char,N>& bytes,size_t offset,T const& value){assert(offset+sizeof(T)<=N);std::memcpy(bytes.data()+offset,&value,sizeof(T));}
void unlocked(){bool acquired=false;std::thread probe([&]{acquired=frame_hook_mutex.try_lock();if(acquired)frame_hook_mutex.unlock();});probe.join();assert(acquired);}
}
#define GEARS_NGX_ROUTE_OBSERVER 1
#define __readgsqword test_readgsqword
#define _ReturnAddress test_return_address
#include "ngx_route_observer.inl"
#undef _ReturnAddress
#undef __readgsqword
namespace {
constexpr uint64_t create_result=0xffeeddccbbaa9988ull,evaluate_result=0x8877665544332211ull;
uint64_t invoke_evaluate();
uint64_t __fastcall original_create(uintptr_t a,uintptr_t b,uintptr_t c,uintptr_t d,uintptr_t e,const void* input){
    unlocked();++create_probes;++create_calls;assert(a==0x1111&&b==0x2222&&c==0x3333&&d==0x4444&&e==0x5555&&input==creation.data());
    if(mode==1){auto* parent=ngx_route_observer::current;assert(parent&&ngx_route_observer::depth==1);assert(invoke_evaluate()==evaluate_result);
        assert(ngx_route_observer::current==parent&&ngx_route_observer::depth==1);}
    if(mode==7){mode=4;invoke_evaluate();assert(false);}
    if(mode==3)throw std::runtime_error("Create");
    if(mode==5){recording=false;assert(active.load()==1);ngx_route_observer::print(timeout_report);recording=true;}
    handle=0x9999888877776666ull;parameters=0xdead1234;put(creation,8,uint32_t(2561));return create_result;
}
uint64_t __fastcall original_evaluate(uintptr_t a,uintptr_t b,uintptr_t c,const void* input){
    unlocked();++evaluate_probes;++evaluate_calls;assert(a==0xaaaa&&b==0xbbbb&&c==0xccccdddd11112222ull&&input==evaluation.data());
    if(mode==2){auto* parent=ngx_route_observer::current;mode=0;assert(invoke_evaluate()==evaluate_result);mode=2;assert(ngx_route_observer::current==parent);}
    if(mode==4)throw std::runtime_error("Evaluate");
    if(mode==6){handle=0x123;parameters=0x456;put(evaluation,0x38,uint32_t(77));put(evaluation,0x60,uint32_t(321));}
    return evaluate_result;
}
uint64_t invoke_create(){test_caller=base+0xbe66bc;ngx_route_observer::CreateFn wrapper=ngx_route_observer::hook_create;
    return wrapper(0x1111,0x2222,0x3333,0x4444,0x5555,creation.data());}
uint64_t invoke_evaluate(){test_caller=base+0x9a35d3;ngx_route_observer::EvaluateFn wrapper=ngx_route_observer::hook_evaluate;
    return wrapper(0xaaaa,0xbbbb,0xccccdddd11112222ull,evaluation.data());}
void reset(){
    ngx_route_observer::recorder={};ngx_route_observer::attested=true;ngx_route_observer::printed=false;ngx_route_observer::current=nullptr;ngx_route_observer::depth=0;
    ngx_route_observer::original_create=original_create;ngx_route_observer::original_evaluate=original_evaluate;
    recording=true;admitted=true;frame_link_diagnostics=true;self_testing=false;continuous_session=true;consuming_frame_ticket=7;consuming_frozen_frame={};committed_world_generation=9;
    mode=create_calls=evaluate_calls=create_probes=evaluate_probes=0;unreadable=0;cells_available=code_available=true;short_code=false;create_code={};evaluate_code={};
    renderer={};views={};context={};test_tls_slot={};creation={};evaluation={};handle=0x1111222233334444ull;parameters=0xaaaa5555;
    consuming_renderer=renderer.data();first_slot=reinterpret_cast<uintptr_t>(test_tls_slot.data());root=reinterpret_cast<uintptr_t>(&first_slot);
    put(test_tls_slot,0x20,reinterpret_cast<uintptr_t>(context.data()));put(context,0x14c0,reinterpret_cast<uintptr_t>(views.data()));
    put(renderer,0x68,reinterpret_cast<uintptr_t>(views.data()));put(renderer,0x70,int(2));
    for(unsigned i=0;i<6;++i)put(creation,i*4,uint32_t(100+i));put(creation,0x18,uint8_t(0xfe));
    for(unsigned i=0;i<resource_offsets.size();++i)put(evaluation,resource_offsets[i],uintptr_t(0x1111000000000000ull+i));
    for(unsigned i=0;i<value_offsets.size();++i)put(evaluation,value_offsets[i],uint32_t(0x7fc00000u+i));
    put(evaluation,0x30,uint32_t(1280));put(evaluation,0x34,uint32_t(1080));put(evaluation,0x38,uint32_t(0));
}
bool contains(FILE* file,const char* value){assert(!fflush(file)&&!fseek(file,0,SEEK_SET));std::array<char,24000> text{};
    const auto count=fread(text.data(),1,text.size()-1,file);assert(count<text.size()-1);return std::strstr(text.data(),value)!=nullptr;}
void cases(){
    reset();assert(invoke_create()==create_result&&create_calls==1&&create_probes==1&&active.load()==0);
    auto const& c=ngx_route_observer::recorder.creates[0];assert(c.exited&&!c.unwinding&&!c.nested&&c.result==create_result&&c.before.fields_mask==127&&c.before.slots_mask==3);
    assert(c.returned&&c.args[4]==0x5555&&c.before.create[2]==102&&c.after.create[2]==2561&&c.before.create[6]==0xfe&&c.before.handle!=c.after.handle);
    reset();assert(invoke_evaluate()==evaluate_result&&evaluate_calls==1&&evaluate_probes==1);
    put(context,0x14c0,reinterpret_cast<uintptr_t>(views.data())+0xd30);assert(invoke_evaluate()==evaluate_result);
    auto const& e=ngx_route_observer::recorder.evaluations[0];auto const& right=ngx_route_observer::recorder.evaluations[1];
    assert(e.before.fields_mask==0x7ffff&&e.before.ownership.associated&&e.before.ownership.eye==0&&right.before.ownership.eye==1);
    assert(e.before.ownership.world==9&&right.before.ownership.ticket==7&&e.before.handle==right.before.handle&&e.before.values[2]==0);
    assert(e.before.resources[3]==0x1111000000000003ull&&e.before.values[3]==0x7fc00003&&e.result==evaluate_result);
    reset();mode=6;assert(invoke_evaluate()==evaluate_result);auto const& changed=ngx_route_observer::recorder.evaluations[0];
    assert(changed.before.handle!=changed.after.handle&&changed.before.parameters!=changed.after.parameters&&changed.after.values[2]==77&&changed.after.values[7]==321);
    reset();mode=1;assert(invoke_create()==create_result&&evaluate_calls==1);assert(ngx_route_observer::recorder.creates[0].nested&&ngx_route_observer::recorder.evaluations[0].nested);
    assert(ngx_route_observer::recorder.evaluations[0].depth==2&&!ngx_route_observer::current&&!ngx_route_observer::depth);
    reset();mode=2;assert(invoke_evaluate()==evaluate_result&&evaluate_calls==2&&ngx_route_observer::recorder.nested==1);
    for(unsigned value:{3u,4u}){reset();mode=value;try{if(value==3)invoke_create();else invoke_evaluate();assert(false);}catch(std::runtime_error const&){}
        auto const& row=value==3?ngx_route_observer::recorder.creates[0]:ngx_route_observer::recorder.evaluations[0];
        assert(row.exited&&!row.returned&&row.unwinding&&!ngx_route_observer::current&&!ngx_route_observer::depth&&active.load()==0);}
    reset();mode=7;try{invoke_create();assert(false);}catch(std::runtime_error const&){}
    assert(create_calls==1&&evaluate_calls==1&&ngx_route_observer::recorder.creates[0].unwinding&&ngx_route_observer::recorder.evaluations[0].unwinding&&
        !ngx_route_observer::current&&!ngx_route_observer::depth&&active.load()==0);
    reset();for(unsigned i=0;i<6;++i)assert(invoke_create()==create_result);for(unsigned i=0;i<10;++i)assert(invoke_evaluate()==evaluate_result);
    assert(create_calls==6&&evaluate_calls==10&&ngx_route_observer::recorder.create_count==4&&ngx_route_observer::recorder.evaluate_count==8&&
        ngx_route_observer::recorder.limited_create==2&&ngx_route_observer::recorder.limited_evaluate==2);
    reset();ngx_route_observer::attested=false;assert(invoke_create()==create_result&&invoke_evaluate()==evaluate_result&&!ngx_route_observer::recorder.create_count&&!ngx_route_observer::recorder.evaluate_count);
    reset();recording=false;assert(invoke_create()==create_result&&invoke_evaluate()==evaluate_result&&!ngx_route_observer::recorder.create_count&&!ngx_route_observer::recorder.evaluate_count);
    for(unsigned unknown:{0u,1u,2u,3u}){reset();if(!unknown)admitted=false;else if(unknown==1)consuming_frame_ticket=8;else if(unknown==2)consuming_frozen_frame.world_generation=10;else consuming_renderer=nullptr;
        assert(invoke_evaluate()==evaluate_result&&!ngx_route_observer::recorder.evaluations[0].before.ownership.associated);}
    reset();cells_available=false;unreadable=reinterpret_cast<uintptr_t>(evaluation.data())+0x18;assert(invoke_evaluate()==evaluate_result);
    assert(!ngx_route_observer::recorder.evaluations[0].before.slots_mask&&!(ngx_route_observer::recorder.evaluations[0].before.fields_mask&4));
    reset();unreadable=reinterpret_cast<uintptr_t>(creation.data())+0x18;assert(invoke_create()==create_result&&ngx_route_observer::recorder.creates[0].before.fields_mask==63);
    reset();assert(!tmpfile_s(&timeout_report)&&timeout_report);mode=5;assert(invoke_create()==create_result);
    assert(contains(timeout_report,"rows_unavailable=1")&&!contains(timeout_report,"ngx_route_call kind="));fclose(timeout_report);
    reset();assert(invoke_evaluate()==evaluate_result);FILE* running_report{};assert(!tmpfile_s(&running_report)&&running_report);ngx_route_observer::print(running_report);
    assert(contains(running_report,"rows_unavailable=1")&&!contains(running_report,"ngx_route_call kind="));fclose(running_report);
    reset();assert(invoke_evaluate()==evaluate_result);recording=false;FILE* report{};assert(!tmpfile_s(&report)&&report);ngx_route_observer::print(report);
    assert(contains(report,"ngx_route_call kind=evaluate")&&contains(report,"result_rax=8877665544332211")&&contains(report,"temporal_history_verified=0"));fclose(report);
    reset();assert(!tmpfile_s(&report)&&report);
    // These synthetic complete-range hashes exercise CNG length/tail checks;
    // the production enable path never accepts these as historical identity.
    const char* create_zero="9fb3871386d4d5abf3149a8fce19470ba4385fdc156f5da4642ba86489f1414e";
    const char* evaluate_zero="a793c66241bfd66404ca1bc9ae6353ebe5fc466cc8b7b6d4e7519a5221e3eade";
    assert(ngx_route_observer::verify(report,create_rva,create_size,create_zero));assert(ngx_route_observer::verify(report,evaluate_rva,evaluate_size,evaluate_zero));
    create_code.back()=1;assert(!ngx_route_observer::verify(report,create_rva,create_size,create_zero));create_code={};
    evaluate_code.back()=1;assert(!ngx_route_observer::verify(report,evaluate_rva,evaluate_size,evaluate_zero));evaluate_code={};
    short_code=true;assert(!ngx_route_observer::verify(report,create_rva,create_size,create_zero));assert(!ngx_route_observer::verify(report,evaluate_rva,evaluate_size,evaluate_zero));short_code=false;
    assert(!ngx_route_observer::enable(report)&&!ngx_route_observer::attested);
    code_available=false;assert(!ngx_route_observer::enable(report));code_available=true;short_code=true;assert(!ngx_route_observer::enable(report));fclose(report);
}
}
int main(){cases();std::puts("NgxRouteObserverTest passed (raw wrapper CPU facts; no SDK/GPU/history claims)");return 0;}
