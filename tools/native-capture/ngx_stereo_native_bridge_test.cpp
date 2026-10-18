#include "ngx_stereo_native_bridge.h"
#include "ngx_stereo_native_api.h"
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>
#include <limits>
using namespace gears_ngx_stereo;
static Identity id(){return {1,2,3,4,5};}
static Layout layout(){Layout l;for(auto& e:l.physical)e={2560,1080};l.full_output={2560,1080};
    l.input={853,720};l.eye_output={1280,1080};l.stride=1280;l.input_base={Point{0,0},Point{1280,0}};return l;}
static AdapterPrerequisites ready(){return {true,true,true,true,true,true,true,true,true,true,true,true};}
static Command command(unsigned eye=2){return {1000u+eye,50u+eye,6u+eye,3,true};}
static ExecuteReceipt execution(unsigned eye,uint64_t value){auto c=command(eye);return {{3,c.list,c.generation,70,80,value,true},value,0};}
static Completion completion(uint64_t value=3){return {3,70,80,value};}
static void put(EvaluateShadow& s,size_t offset,uint64_t v,size_t bytes=4){std::memcpy(s.bytes.data()+offset,&v,bytes);}
static uint32_t word(void const* p,size_t offset){uint32_t v{};std::memcpy(&v,static_cast<unsigned char const*>(p)+offset,4);return v;}
struct Backend final:AdapterBackend {
    unsigned creates{},evaluates{},copies{},releases{};std::vector<uint64_t> used,released;
    unsigned failed_create{99},zero_create{99},failed_eval{99},failed_release{99};bool throw_create{},throw_evaluate{};
    NativeResult success{1};EvaluateShadow eval{};CreateShadow create_shadow{};
    NativeResult create(uint64_t c,uint32_t feature,uint64_t p,uint64_t* h)override{
        assert(c==command().raw&&feature==1&&p==4);auto index=creates++;*h=index==zero_create?0:100+index;
        assert(word(create_shadow.bytes.data(),0)==853&&word(create_shadow.bytes.data(),4)==720&&
               word(create_shadow.bytes.data(),8)==1280&&word(create_shadow.bytes.data(),12)==1080);
        if(throw_create)throw std::runtime_error("Create");return index==failed_create?dispatch_unavailable:success;
    }
    NativeResult evaluate(uint64_t c,uint64_t h,uint64_t p,uint64_t callback)override{
        assert(h==100||h==101);unsigned eye=unsigned(h-100);assert(c==command(eye).raw&&p==4&&!callback);
        assert(word(eval.bytes.data(),0x30)==853&&word(eval.bytes.data(),0x34)==720);
        assert(word(eval.bytes.data(),0x60)==eye*1280&&word(eval.bytes.data(),0x68)==eye*1280&&word(eval.bytes.data(),0x70)==eye*1280);
        assert(word(eval.bytes.data(),0x88)==0&&word(eval.bytes.data(),0x8c)==0);used.push_back(h);
        auto index=evaluates++;if(throw_evaluate)throw std::runtime_error("Eval");return index==failed_eval?dispatch_unavailable:success;
    }
    NativeResult release(uint64_t h)override{released.push_back(h);return releases++==failed_release?dispatch_unavailable:success;}
    void copy(CopyPlan const& p)override{++copies;assert(p.destination[0]==p.ordinal*1280);}
};
struct Fixture {
    Backend backend;StereoAdapter adapter{backend,ready()};NativeWrapperBridge bridge{adapter,true};uint64_t cell{};
    CreateShadow source{};
    unsigned wrappers{},fallback{};bool omit_dispatch{},duplicate_dispatch{},nested{},concurrent{},throw_wrapper{},bad_slot{};
    unsigned char alignment_padding[2]{};
    Fixture(){source.bytes.fill(0xa5);}
    NativeResult original_create(uint64_t,uint32_t,uint64_t,uint64_t*){++fallback;return 7;}
    NativeResult original_eval(uint64_t,uint64_t,uint64_t,uint64_t){++fallback;return 7;}
    WrapperOutcome start(){return bridge.create_wrapper(id(),layout(),command(),&cell,source,source.bytes.data(),[&](void const* ptr)->uint64_t{
        ++wrappers;assert(ptr!=source.bytes.data());std::memcpy(backend.create_shadow.bytes.data(),ptr,create_consumed_span);
        for(size_t i=16;i<source.bytes.size();++i)assert(backend.create_shadow.bytes[i]==source.bytes[i]);
        if(throw_wrapper)throw std::runtime_error("wrapper");if(omit_dispatch)return 1;
        uint64_t wrong_cell{};
        auto invoke=[&](){return bridge.create_dispatch(command().raw,1,4,bad_slot?&wrong_cell:&cell,
            [&](auto a,auto b,auto c,auto d){return original_create(a,b,c,d);});};
        auto raw=invoke();if(duplicate_dispatch)raw=invoke();return raw;});}
    EvaluationBinding binding(unsigned eye){EvaluationBinding b;b.producer.use={id(),10,command(eye).list,command(eye).generation,eye};
        b.producer.view=900+eye;b.command=command(eye);b.public_handle=cell;
        for(unsigned i=0;i<4;++i){b.producer.resources[i]={200+i,3,{2560,1080},26,0,1,0};b.raw_resources[i]=1200+i;}
        for(auto& r:b.producer.rendered)r={eye*1280,0,853,720};return b;}
    EvaluateShadow shadow(unsigned eye){EvaluateShadow s;s.bytes.fill(0xb7);auto b=binding(eye);
        constexpr size_t offsets[]{0,0x18,0x20,8};for(unsigned i=0;i<4;++i)put(s,offsets[i],b.raw_resources[i],8);
        put(s,0x30,853);put(s,0x34,360);put(s,0x38,0);
        for(size_t offset:{size_t(0x60),size_t(0x68),size_t(0x70)}){put(s,offset,eye*1280);put(s,offset+4,0);}return s;}
    WrapperOutcome evaluate(unsigned eye){auto b=binding(eye);auto s=shadow(eye);return bridge.evaluate_wrapper(b,s,s.bytes.data(),[&](void const* ptr)->uint64_t{
        ++wrappers;assert(ptr!=s.bytes.data());std::memcpy(backend.eval.bytes.data(),ptr,evaluate_initialized_span);
        assert(word(ptr,0x38)==1); // Both newly owned histories reset independently.
        for(size_t i=0x90;i<evaluate_initialized_span;++i)assert(backend.eval.bytes[i]==s.bytes[i]);
        if(throw_wrapper)throw std::runtime_error("wrapper");if(omit_dispatch)return 1;
        if(nested){auto result=bridge.evaluate_wrapper(b,s,s.bytes.data(),[&](void const*)->uint64_t{assert(false);return 0;});assert(!result.forwarded);}
        if(concurrent){std::thread thread([&]{auto entry=adapter.enter();assert(!entry);});thread.join();}
        auto invoke=[&](){return bridge.evaluate_dispatch(b.command.raw,cell,4,0,[&](auto a,auto c,auto d,auto e){return original_eval(a,c,d,e);});};
        auto raw=invoke();if(duplicate_dispatch)raw=invoke();return raw;});}
    void submit_create(){auto entry=adapter.enter();assert(adapter.submitted_creation(entry,execution(2,1)));}
    void copy(unsigned eye){auto b=binding(eye);CopyFacts f;f.identity=id();f.ticket=10;f.caller=0x1c6093;
        f.list=b.command.list;f.generation=b.command.generation;f.list_device=3;f.direct=f.copy_source=f.copy_dest=true;
        f.resources=b.producer.resources;f.input_base=layout().input_base;f.feature_output=layout().eye_output;f.evaluated_ordinal=eye;
        auto entry=adapter.enter();assert(adapter.copy(entry,f,eye));}
    void submit(unsigned eye){auto entry=adapter.enter();assert(adapter.submitted_eye(entry,eye,execution(eye,eye+2)));}
    NativeResult release(uint64_t h=100,Completion c=completion()){return bridge.release_dispatch(h,c,[&](uint64_t){++fallback;return NativeResult(17);});}
};
static void disabled(){Backend b;StereoAdapter adapter(b);NativeWrapperBridge bridge(adapter);CreateShadow source;EvaluateShadow eval;
    uint64_t sentinel=0xabcdef0123456789ull;unsigned forwarded{};
    auto call=[&](void const* ptr){assert(ptr==&sentinel);++forwarded;return sentinel;};
    auto c=bridge.create_wrapper({}, {}, {},nullptr,source,&sentinel,call);auto e=bridge.evaluate_wrapper({},eval,&sentinel,call);
    assert(c.raw==sentinel&&e.raw==sentinel&&forwarded==2&&b.creates==0&&b.evaluates==0);
    uint64_t handle{};assert(bridge.create_dispatch(1,2,3,&handle,[&](uint64_t a,uint32_t f,uint64_t p,uint64_t* h){assert(a==1&&f==2&&p==3&&h==&handle);return NativeResult(23);})==23);
    assert(bridge.evaluate_dispatch(1,2,3,4,[](auto a,auto h,auto p,auto cb){assert(a==1&&h==2&&p==3&&cb==4);return NativeResult(24);})==24);
    assert(bridge.release_dispatch(2,{},[](uint64_t h){assert(h==2);return NativeResult(25);})==25);
}
static void normal(){for(NativeResult raw:{0u,1u,17u,0xffffffffu}){Fixture f;f.backend.success=raw;auto c=f.start();
    assert(c.admitted&&c.raw==raw&&f.cell==100&&f.backend.creates==2&&!f.fallback);f.submit_create();
    assert(f.evaluate(0).admitted);f.copy(0);assert(f.evaluate(1).admitted);f.copy(1);f.submit(0);f.submit(1);
    assert((f.backend.used==std::vector<uint64_t>{100,101})&&f.cell==100&&f.wrappers==3&&!f.fallback);
    assert(f.release(999)==17&&f.fallback==1); // Foreign release untouched.
    auto invalid=completion();invalid.value=UINT64_MAX;assert(f.release(100,invalid)==dispatch_unavailable&&!f.backend.releases);
    assert(f.release(100,completion(2))==dispatch_unavailable&&!f.backend.releases);
    assert(f.release()==raw&&f.backend.releases==2);
    assert(f.release()==dispatch_unavailable&&f.backend.releases==2);auto entry=f.adapter.enter();assert(f.adapter.drained(entry));}}
static void rejected_create(){for(unsigned mode=0;mode<6;++mode){Fixture f;
    if(mode==0)f.omit_dispatch=true;if(mode==1)f.duplicate_dispatch=true;if(mode==2)f.bad_slot=true;
    if(mode==3)f.backend.failed_create=1;if(mode==4)f.backend.zero_create=1;if(mode==5)f.cell=999;
    auto c=f.start();assert(!c.admitted&&c.raw==dispatch_unavailable&&!f.fallback);
    if(mode==0||mode==2||mode==5)assert(!f.backend.creates);
    if(mode==3||mode==4)assert(f.backend.creates==2&&f.cell==0);
    auto entry=f.adapter.enter();assert(f.adapter.phase(entry)!=AcceptancePhase::accepted);
}}
static void rejected_evaluate(){for(unsigned mode=0;mode<6;++mode){Fixture f;assert(f.start().admitted);f.submit_create();
    if(mode==0)f.omit_dispatch=true;if(mode==1)f.duplicate_dispatch=true;if(mode==2)f.nested=true;
    if(mode==3)f.concurrent=true;if(mode==4)f.backend.failed_eval=0;if(mode==5)f.throw_wrapper=true;
    bool threw{};WrapperOutcome result;try{result=f.evaluate(0);}catch(std::runtime_error const&){threw=true;}
    assert(threw==(mode==5)&&!result.admitted&&result.raw==dispatch_unavailable&&!f.fallback);
    if(mode==0||mode==2||mode==3||mode==5)assert(!f.backend.evaluates);
    if(f.backend.evaluates){assert(f.release()==dispatch_unavailable&&!f.backend.releases);f.submit(0);}
    assert(f.release()==1&&f.backend.releases==2);
}}
static void resource_binding(){for(unsigned mode=0;mode<4;++mode){Fixture f;assert(f.start().admitted);auto b=f.binding(0);auto s=f.shadow(0);
    if(mode==0)put(s,0,999,8);if(mode==1)put(s,0x68,1280);if(mode==2)b.producer.use.eye=1;if(mode==3)b.producer.resources[2].identity=0;
    auto r=f.bridge.evaluate_wrapper(b,s,s.bytes.data(),[](void const*)->uint64_t{assert(false);return 0;});assert(!r.forwarded&&!f.backend.evaluates);
}}
static void exceptions_and_release(){
    {Fixture f;f.backend.throw_create=true;bool threw{};try{(void)f.start();}catch(std::runtime_error const&){threw=true;}
        assert(threw&&!f.cell&&f.backend.creates==1);assert(f.release()==dispatch_unavailable);f.submit_create();assert(f.release()==1&&f.backend.releases==1);}
    {Fixture f;assert(f.start().admitted);f.submit_create();f.backend.throw_evaluate=true;bool threw{};
        try{(void)f.evaluate(0);}catch(std::runtime_error const&){threw=true;}assert(threw&&f.backend.evaluates==1);
        assert(f.release()==dispatch_unavailable);f.submit(0);assert(f.release()==1&&f.backend.releases==2);}
    {Fixture f;assert(f.start().admitted);f.submit_create();f.backend.failed_release=0;
        assert(f.release()==dispatch_unavailable&&f.backend.releases==1);assert(f.release()==dispatch_unavailable&&f.backend.releases==1);
        auto entry=f.adapter.enter();assert(!f.adapter.drained(entry));}
}
static NativeResult __fastcall api_create(uint64_t c,uint32_t feature,uint64_t p,uint64_t* h){assert(c==12&&feature==1&&p==14);*h=99;return 17;}
static NativeResult __fastcall api_evaluate(uint64_t c,uint64_t h,uint64_t p,uint64_t cb){assert(c==12&&h==99&&p==14&&cb==0);return 18;}
static NativeResult __fastcall api_release(uint64_t h){assert(h==99);return 19;}
static unsigned optimal_calls{};
static NativeResult __fastcall api_optimal(uint32_t first,uint32_t w,uint32_t h,uint32_t quality,
    uint32_t* ow,uint32_t* oh,uint32_t* maxw,uint32_t* maxh,uint32_t* minw,uint32_t* minh,float* sharpness){
    assert(first==w&&w==1920&&h==2160&&quality==2);++optimal_calls;
    *ow=1280;*oh=1440;*maxw=1920;*maxh=2160;*minw=960;*minh=1080;*sharpness=.25f;return 1;
}
static void optimal_api(){
    NativeDispatchFunctions functions;NativeDispatchBackend missing(functions);
    assert(!missing.optimal({1920,2160},2).called);
    functions.optimal=api_optimal;NativeDispatchBackend backend(functions);
    assert(!backend.optimal({},2).called&&!optimal_calls);
    auto result=backend.optimal({1920,2160},2);
    assert(optimal_calls==1&&!backend.create_attempts()&&result.supports({1280,1440}));
    assert(result.output.width==1920&&result.output.height==2160&&result.quality==2&&result.sharpness==.25f);
    auto changed=result;changed.minimum.width=1920;assert(!changed.supports({1280,1440}));
    changed=result;changed.result=0xbad0000c;assert(!changed.supports({1280,1440}));
    changed=result;changed.maximum.height=0;assert(!changed.supports({1280,1440}));
    changed=result;changed.optimal.width=2000;assert(!changed.supports({1280,1440}));
    changed=result;changed.sharpness=std::numeric_limits<float>::quiet_NaN();assert(!changed.supports({1280,1440}));
    assert(!result.supports({0,1440})&&!result.supports({1280,2200})&&!result.supports({900,1440}));
}
static void raw_api(){
    unsigned copies{};NativeDispatchFunctions functions{api_create,api_evaluate,api_release,&copies,
        [](void* context,CopyPlan const& p){assert(p.ticket==123);++*static_cast<unsigned*>(context);}};
    assert(functions.ready());NativeDispatchBackend backend(functions);uint64_t h{};
    assert(backend.create(12,1,14,&h)==17&&h==99);assert(backend.evaluate(12,h,14,0)==18&&backend.release(h)==19);
    CopyPlan plan;plan.ticket=123;backend.copy(plan);assert(copies==1);
    NativeDispatchBackend missing({});assert(missing.create(12,1,14,&h)==dispatch_unavailable&&missing.evaluate(12,h,14,0)==dispatch_unavailable&&missing.release(h)==dispatch_unavailable);
    bool threw{};try{missing.copy(plan);}catch(std::logic_error const&){threw=true;}assert(threw);
}
int main(){disabled();normal();rejected_create();rejected_evaluate();resource_binding();exceptions_and_release();raw_api();optimal_api();
    std::cout<<"NGX complete-wrapper bridge, independent dispatch and release tests passed\n";}
