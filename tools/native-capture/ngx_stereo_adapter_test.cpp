#include "ngx_stereo_adapter.h"
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace gears_ngx_stereo;
static Identity identity(){return {1,2,3,4,5};}
static Layout layout(){Layout l;for(auto& e:l.physical)e={2560,1080};l.full_output={2560,1080};
    l.input={853,720};l.eye_output={1280,1080};l.stride=1280;l.input_base={Point{0,0},Point{1280,0}};return l;}
static AdapterPrerequisites ready(){return {true,true,true,true,true,true,true,true,true,true,true,true};}
static Command command(unsigned eye=2){return {1000u+eye,50u+eye,6u+eye,3,true};}
static EvalUse use(unsigned eye,uint64_t ticket=10){auto c=command(eye);return {identity(),ticket,c.list,c.generation,eye};}
static ExecuteReceipt execution(unsigned eye,uint64_t order,uint64_t value){auto c=command(eye);
    return {{3,c.list,c.generation,70,80,value,true},order,0};}
static Completion completion(uint64_t value){return {3,70,80,value};}
static CopyFacts facts(unsigned eye,uint64_t ticket=10){CopyFacts f;f.identity=identity();f.ticket=ticket;f.caller=0x1c6093;
    auto c=command(eye);f.list=c.list;f.generation=c.generation;f.list_device=3;f.direct=f.copy_source=f.copy_dest=true;
    for(unsigned i=0;i<4;++i)f.resources[i]={100u+i,3,{2560,1080},26,0,1,0};
    f.input_base=layout().input_base;f.feature_output=layout().eye_output;f.evaluated_ordinal=eye;return f;}
static ProducerObservation producer(unsigned eye,uint64_t ticket=10){ProducerObservation p;
    p.use=use(eye,ticket);p.view=900+eye;p.resources=facts(eye,ticket).resources;
    for(auto& r:p.rendered)r={eye*1280,0,853,720};return p;}
static std::array<RetainedOutputObservation,2> observations(uint64_t ticket=10,uint64_t completed=3){
    std::array<RetainedOutputObservation,2> out{};
    for(unsigned eye=0;eye<2;++eye)out[eye]={use(eye,ticket),100+eye,facts(eye,ticket).resources[3],{0,0,1280,1080},completion(completed)};
    return out;
}
static void accept(StereoAdapter& a,uint64_t ticket=10,uint64_t completed=3){auto entry=a.enter();
    assert(a.accept_candidate(entry,observations(ticket,completed))&&a.phase(entry)==AcceptancePhase::accepted);}
static uint32_t word(std::array<unsigned char,evaluate_initialized_span> const& b,size_t offset){uint32_t v{};std::memcpy(&v,b.data()+offset,4);return v;}
struct Fake final:AdapterBackend {
    unsigned creates{},evaluates{},releases{},copies{};unsigned fail_create{99},zero_create{99},fail_eval{99};
    NativeResult accepted_result{named_result_success},release_result{named_result_success};
    bool duplicate{},throw_create{},throw_eval{},throw_copy{},throw_release{},failed_release{};
    StereoAdapter* nested{};std::vector<uint64_t> handles,released;
    alignas(8) std::array<unsigned char,evaluate_initialized_span> eval_structure{};
    std::array<unsigned char,evaluate_initialized_span> opaque_expected{};
    std::array<unsigned,2> histories{};
    std::vector<unsigned char> color=std::vector<unsigned char>(2560*1080,0),output=color;
    Fake(){for(size_t i=0;i<eval_structure.size();++i)eval_structure[i]=static_cast<unsigned char>((i*37+0x5b)%256);
        opaque_expected=eval_structure;}
    NativeResult create(uint64_t cmd,uint32_t feature,uint64_t parameters,uint64_t* handle)override{
        assert(cmd==command().raw&&feature==1&&parameters==4);unsigned eye=creates++;
        *handle=duplicate?100u:100u+eye; // Even failed/throwing SDK may return a handle.
        if(eye==zero_create)*handle=0;
        if(throw_create)throw std::runtime_error("create");return eye==fail_create?dispatch_unavailable:accepted_result;
    }
    NativeResult evaluate(uint64_t cmd,uint64_t handle,uint64_t parameters,uint64_t callback)override{
        assert(parameters==4&&handle>=100&&handle<=101&&callback==0);unsigned eye=unsigned(handle-100);
        assert(cmd==command(eye).raw);handles.push_back(handle);unsigned index=evaluates++;
        if(nested){auto entry=nested->enter();assert(!entry);}
        if(throw_eval)throw std::runtime_error("eval");
        if(index==fail_eval)return dispatch_unavailable;
        assert(word(eval_structure,0x30)==853&&word(eval_structure,0x34)==720);
        unsigned x=word(eval_structure,0x60);assert(x==eye*1280&&word(eval_structure,0x68)==x&&word(eval_structure,0x70)==x);
        assert(word(eval_structure,0x88)==0&&word(eval_structure,0x8c)==0);
        for(size_t i=0x90;i<evaluate_initialized_span;++i)assert(eval_structure[i]==opaque_expected[i]);
        assert(word(eval_structure,0x160)==word(opaque_expected,0x160));
        if(word(eval_structure,0x38))histories[eye]=0;++histories[eye];
        // Synthetic SDK consumes reduced input, writes a COMPLETE eye image at
        // Output origin, and leaves the unused physical right half untouched.
        for(unsigned y=0;y<1080;++y)for(unsigned xx=0;xx<1280;++xx)
            output[y*2560+xx]=color[(y*720/1080)*2560+x+xx*853/1280];
        return accepted_result;
    }
    NativeResult release(uint64_t handle)override{++releases;released.push_back(handle);if(throw_release)throw std::runtime_error("release");return failed_release?dispatch_unavailable:release_result;}
    void copy(CopyPlan const& p)override{
        ++copies;if(throw_copy)throw std::runtime_error("copy");
        assert((p.source_box==std::array<uint32_t,6>{0,0,0,1280,1080,1}));
        assert(p.source==103&&p.target==100&&p.destination[0]==p.ordinal*1280&&p.destination[1]==0&&p.destination[2]==0);
        for(unsigned y=0;y<1080;++y)for(unsigned x=0;x<1280;++x)color[y*2560+p.destination[0]+x]=output[y*2560+x];
    }
};
static void start(StereoAdapter& a){auto entry=a.enter();assert(entry&&a.begin_epoch(entry,identity(),layout()));
    auto result=a.create(entry,command(),1,4);assert(result.admitted&&result.raw_valid&&result.raw==named_result_success&&result.public_handle==100);}
static bool eval(StereoAdapter& a,Fake& f,unsigned eye,uint64_t ticket=10,bool native_reset=false,bool expected_reset=true){
    auto entry=a.enter();assert(entry&&a.observe_producer(entry,producer(eye,ticket)));bool reset{};assert(a.prepare_evaluate(entry,use(eye,ticket),command(eye),100,4,native_reset,reset));assert(reset==expected_reset);
    auto original=f.eval_structure;assert(patch_evaluate(original,f.eval_structure,layout(),eye,reset));
    auto r=a.evaluate(entry,command(eye).raw,100,4,0);assert(r.raw_valid);return r.admitted;
}
static void copy(StereoAdapter& a,unsigned eye,uint64_t ticket=10){auto entry=a.enter();assert(a.copy(entry,facts(eye,ticket),eye));}
static void submit(StereoAdapter& a,unsigned eye,uint64_t order,uint64_t value){auto entry=a.enter();assert(a.submitted_eye(entry,eye,execution(eye,order,value)));}
static void submit_create(StereoAdapter& a){auto entry=a.enter();assert(a.submitted_creation(entry,execution(2,1,1)));}
static void patch_boundaries(){
    static_assert(alignof(CreateShadow)>=8&&alignof(EvaluateShadow)>=8);
    CreateShadow minimal_original{},minimal_owned{};minimal_original.bytes.fill(0xcd);
    assert(patch_create(minimal_original.bytes,minimal_owned.bytes,layout()));
    for(size_t i=16;i<create_consumed_span;++i)assert(minimal_owned.bytes[i]==0xcd);
    unsigned eye=99;assert(select_input_eye(layout(),{853,720},{Point{1280,0},Point{1280,0},Point{1280,0}},eye)&&eye==1);
    assert(!select_input_eye(layout(),{853,720},{Point{1280,0},Point{0,0},Point{1280,0}},eye));
    assert(!select_input_eye(layout(),{1280,1080},{Point{1280,0},Point{1280,0},Point{1280,0}},eye));
    std::array<unsigned char,0x24> original;original.fill(0xcd);auto owned=original;assert(patch_create(original,owned,layout()));
    assert(original[0]==0xcd);for(size_t i=16;i<owned.size();++i)assert(owned[i]==0xcd);
    std::array<unsigned char,evaluate_initialized_span> input;
    for(size_t i=0;i<input.size();++i)input[i]=static_cast<unsigned char>((i*37+0x5b)%256);auto output=input;
    assert(patch_evaluate(input,output,layout(),1,false));assert(word(output,0x30)==853&&word(output,0x34)==720&&word(output,0x38)==0);
    assert(word(output,0x60)==1280&&word(output,0x68)==1280&&word(output,0x70)==1280);
    for(size_t i=0;i<input.size();++i){bool patched=(i>=0x30&&i<0x3c)||(i>=0x60&&i<0x78)||(i>=0x88&&i<0x90);
        if(!patched)assert(output[i]==input[i]);assert(input[i]==static_cast<unsigned char>((i*37+0x5b)%256));}
    auto bad=layout();bad.input_base[1].x=853;auto prior=output;assert(!patch_evaluate(input,output,bad,1,true)&&output==prior);
}
static void disabled_and_prerequisites(){
    for(unsigned missing=0;missing<12;++missing){auto p=ready();switch(missing){
        case 0:p.enabled=false;break;case 1:p.current_wrappers=false;break;case 2:p.current_dispatchers=false;break;
        case 3:p.current_release=false;break;case 4:p.current_builder=false;break;case 5:p.consumed_structure_spans=false;break;
        case 6:p.result_semantics=false;break;case 7:p.serial_parameter_consumption=false;break;case 8:p.typed_resource_domains=false;break;
        case 9:p.release_context=false;break;case 10:p.candidate_isolation=false;break;case 11:p.execution_dependencies=false;break;}
        Fake f;StereoAdapter a(f,p);auto entry=a.enter();assert(!a.begin_epoch(entry,identity(),layout()));
        assert(!a.create(entry,command(),1,4).admitted&&f.creates==0);}
    Fake f;StereoAdapter a(f);auto entry=a.enter();assert(!a.begin_epoch(entry,identity(),layout()));assert(!a.create(entry,command(),1,4).admitted&&f.creates==0);
}
static void independent_images_and_histories(){
    Fake f;StereoAdapter a(f,ready());start(a);submit_create(a);
    std::array<std::vector<unsigned char>,2> expected;
    for(unsigned eye=0;eye<2;++eye){expected[eye].resize(1280*1080);
        for(unsigned y=0;y<720;++y)for(unsigned x=0;x<853;++x)f.color[y*2560+eye*1280+x]=static_cast<unsigned char>(20+eye*120+(x+y)%40);
        for(unsigned y=0;y<1080;++y)for(unsigned x=0;x<1280;++x)expected[eye][y*1280+x]=f.color[(y*720/1080)*2560+eye*1280+x*853/1280];}
    auto before=f.color;assert(eval(a,f,0));copy(a,0);
    for(unsigned y=0;y<720;++y)for(unsigned x=0;x<853;++x)assert(f.color[y*2560+1280+x]==before[y*2560+1280+x]);
    assert(eval(a,f,1));copy(a,1);
    for(unsigned eye=0;eye<2;++eye)for(unsigned y=0;y<1080;++y)for(unsigned x=0;x<1280;++x)
        assert(f.color[y*2560+eye*1280+x]==expected[eye][y*1280+x]);
    assert(expected[0]!=expected[1]&&f.handles==std::vector<uint64_t>({100,101}));
    {auto entry=a.enter();RecordedOutput out;assert(a.phase(entry)==AcceptancePhase::candidate&&!a.recorded_output(entry,out)&&!a.ordered_output(entry,out));}
    submit(a,0,2,2);submit(a,1,3,3);
    {auto entry=a.enter();RecordedOutput out;assert(!a.recorded_output(entry,out)&&!a.ordered_output(entry,out));}
    accept(a);
    {auto entry=a.enter();RecordedOutput out;assert(a.recorded_output(entry,out)&&out.color==100&&out.copies[0].list!=out.copies[1].list&&a.ordered_output(entry,out));
        Rect source{},destination{};assert(output_geometry(out,identity(),10,100,1,{1280,0,853,720},{2560,1080},source,destination));
        assert(source.x==1280&&source.width==1280&&source.height==1080&&destination.x==1280&&destination.width==1280);
        assert(!output_geometry(out,identity(),10,100,1,{1280,0,1280,1080},{2560,1080},source,destination));
        auto wrong=out;wrong.copies[0].target++;assert(!output_geometry(wrong,identity(),10,100,1,{1280,0,853,720},{2560,1080},source,destination));
        assert(!output_geometry(out,identity(),11,100,1,{1280,0,853,720},{2560,1080},source,destination));}
    for(uint64_t ticket=11;ticket<14;++ticket){bool cut=ticket==12;
        assert(eval(a,f,0,ticket,cut,cut));copy(a,0,ticket);assert(eval(a,f,1,ticket,cut,cut));copy(a,1,ticket);
        submit(a,0,2*ticket,2*ticket);submit(a,1,2*ticket+1,2*ticket+1);}
    assert(f.histories[0]==2&&f.histories[1]==2);assert(f.creates==2&&f.evaluates==8&&f.copies==8);
    {auto entry=a.enter();a.invalidate(entry);NativeResult raw{};assert(!a.release_completed(entry,100,completion(25),raw));
        assert(a.release_completed(entry,100,completion(27),raw)&&raw==named_result_success);assert(a.release_completed(entry,101,completion(27),raw));
        assert(!a.release_completed(entry,100,completion(99),raw)&&a.drained(entry));assert(a.begin_epoch(entry,{2,2,3,4,6},layout()));
        assert(a.phase(entry)==AcceptancePhase::candidate);RecordedOutput out;assert(!a.recorded_output(entry,out));}
}
static void create_failures_and_gpu_retirement(){
    for(unsigned failure=0;failure<4;++failure){Fake f;f.fail_create=failure<2?failure:99;f.duplicate=failure==2;f.throw_create=failure==3;
        StereoAdapter a(f,ready());auto entry=a.enter();assert(a.begin_epoch(entry,identity(),layout()));bool threw{};
        try{auto r=a.create(entry,command(),1,4);assert(!r.admitted&&r.raw_valid);if(failure<2)assert(r.raw==dispatch_unavailable);}
        catch(std::runtime_error const&){threw=true;}assert(threw==(failure==3));
        NativeResult raw{};assert(!a.release_completed(entry,100,completion(99),raw)&&!a.begin_epoch(entry,identity(),layout()));
        assert(a.submitted_creation(entry,execution(2,1,5)));assert(!a.release_completed(entry,100,completion(4),raw));
        assert(a.release_completed(entry,100,completion(5),raw));
        if(failure==1)assert(a.release_completed(entry,101,completion(5),raw));
        assert(a.drained(entry)&&f.releases==(failure==1?2u:1u));}
}
static void failed_recorded_eval_and_copy(){
    for(unsigned failure=0;failure<3;++failure){Fake f;StereoAdapter a(f,ready());start(a);submit_create(a);
        f.fail_eval=failure==0?0:99;f.throw_eval=failure==1;f.throw_copy=failure==2;bool threw{};
        try{bool ok=eval(a,f,0);if(failure==2){assert(ok);copy(a,0);}else assert(!ok);}
        catch(std::runtime_error const&){threw=true;}assert(threw==(failure!=0));
        submit(a,0,2,8);auto entry=a.enter();NativeResult raw{};RecordedOutput out;
        assert(!a.recorded_output(entry,out)&&!a.release_completed(entry,100,completion(7),raw));
        assert(a.release_completed(entry,100,completion(8),raw));assert(a.release_completed(entry,101,completion(8),raw));assert(a.drained(entry));}
}
static void bad_order_and_concurrency(){
    for(unsigned bad=0;bad<4;++bad){Fake f;StereoAdapter a(f,ready());start(a);submit_create(a);assert(eval(a,f,0));copy(a,0);assert(eval(a,f,1));copy(a,1);submit(a,0,3,3);
        auto r=execution(1,4,4);if(bad==0)r.submission.queue++;if(bad==1)r.submission.fence++;if(bad==2)r.order=2;if(bad==3)r.submission.value=2;
        auto entry=a.enter();assert(!a.submitted_eye(entry,1,r));RecordedOutput out;assert(!a.ordered_output(entry,out));}
    {Fake f;StereoAdapter a(f,ready());start(a);submit_create(a);f.nested=&a;assert(!eval(a,f,0));submit(a,0,2,2);
        auto entry=a.enter();NativeResult raw{};assert(a.release_completed(entry,100,completion(2),raw));assert(a.release_completed(entry,101,completion(2),raw));}
    {Fake f;StereoAdapter a(f,ready());start(a);auto entry=a.enter();auto nested=a.enter();assert(!nested);assert(!a.create(entry,command(),1,4).admitted&&f.creates==2);}
    {Fake f;StereoAdapter a(f,ready());start(a);submit_create(a);auto entry=a.enter();a.invalidate(entry);f.throw_release=true;NativeResult raw{};bool threw{};
        try{(void)a.release_completed(entry,100,completion(1),raw);}catch(std::runtime_error const&){threw=true;}
        assert(threw&&!a.release_completed(entry,100,completion(1),raw)&&f.releases==1);
        f.throw_release=false;assert(a.release_completed(entry,101,completion(1),raw));
        assert(!a.drained(entry)&&!a.begin_epoch(entry,{2,2,3,4,6},layout()));}
    {Fake f;StereoAdapter a(f,ready());start(a);submit_create(a);auto entry=a.enter();a.invalidate(entry);f.failed_release=true;NativeResult raw=99;
        assert(a.release_completed(entry,100,completion(1),raw)&&raw==dispatch_unavailable);assert(a.release_completed(entry,101,completion(1),raw));
        assert(!a.drained(entry)&&!a.begin_epoch(entry,{2,2,3,4,6},layout()));}
}
static void boundary_rejections(){
    {Fake f;StereoAdapter a(f,ready());start(a);submit_create(a);
        {auto entry=a.enter();assert(a.observe_producer(entry,producer(0)));bool reset{};assert(a.prepare_evaluate(entry,use(0),command(0),100,4,false,reset));
            auto r=a.evaluate(entry,command(0).raw,100,4,1);assert(!r.admitted&&!r.raw_valid&&f.evaluates==0);}
        auto entry=a.enter();NativeResult raw{};assert(a.release_completed(entry,100,completion(1),raw));
        assert(a.release_completed(entry,101,completion(1),raw)&&a.drained(entry));}
    {Fake f;StereoAdapter a(f,ready());start(a);submit_create(a);
        {auto entry=a.enter();assert(a.observe_producer(entry,producer(0)));bool reset{};assert(a.prepare_evaluate(entry,use(0),command(0),100,4,false,reset));}
        // No SDK Eval was entered. Abandonment invalidates publication, but
        // does not invent an unsubmitted GPU obligation that leaks the handles.
        auto entry=a.enter();NativeResult raw{};assert(f.evaluates==0&&a.release_completed(entry,100,completion(1),raw));
        assert(a.release_completed(entry,101,completion(1),raw)&&a.drained(entry));}
    for(unsigned bad=0;bad<6;++bad){Fake f;StereoAdapter a(f,ready());start(a);submit_create(a);auto entry=a.enter();
        assert(a.observe_producer(entry,producer(0)));
        auto u=use(0);auto c=command(0);uint64_t handle=100,parameters=4;
        if(bad==0)u=use(1);if(bad==1)u.identity.epoch++;if(bad==2)c.generation++;if(bad==3)handle=101;
        if(bad==4)parameters++;if(bad==5)u.ticket=0;bool reset{};
        assert(!a.prepare_evaluate(entry,u,c,handle,parameters,false,reset)&&f.evaluates==0);}
    {Fake f;StereoAdapter a(f,ready());start(a);submit_create(a);assert(eval(a,f,0));auto entry=a.enter();
        auto wrong=facts(0);wrong.input_base[1].x=853;assert(!a.copy(entry,wrong,0)&&f.copies==0);RecordedOutput out;assert(!a.recorded_output(entry,out));}
    {Fake f;StereoAdapter a(f,ready());start(a);submit_create(a);assert(eval(a,f,0));copy(a,0);assert(eval(a,f,1));f.throw_copy=true;bool threw{};
        try{copy(a,1);}catch(std::runtime_error const&){threw=true;}
        auto entry=a.enter();RecordedOutput out;assert(threw&&f.copies==2&&!a.recorded_output(entry,out));}
    // Members in one batch may share a fence. This verifies submission receipt
    // bookkeeping only; host execution_dependencies must separately attest the
    // actual scratch-resource barriers. Member order is not that dependency.
    {Fake f;StereoAdapter a(f,ready());start(a);submit_create(a);assert(eval(a,f,0));copy(a,0);assert(eval(a,f,1));copy(a,1);
        auto entry=a.enter();auto left=execution(0,2,5),right=execution(1,2,5);right.member=1;
        assert(a.submitted_eye(entry,0,left)&&a.submitted_eye(entry,1,right));RecordedOutput out;assert(!a.ordered_output(entry,out));
        assert(a.accept_candidate(entry,observations(10,5))&&a.ordered_output(entry,out));}
}
static void result_mask_contract(){
    static_assert(sizeof(NativeResult)==4&&named_result_success==1);
    static_assert(evaluate_initialized_span==0x168&&create_consumed_span==0x19);
    for(NativeResult raw:{0u,1u,0xbacfffffu,0xbae00000u,0xffffffffu}){
        assert(native_result_accepted(raw));
        Fake f;f.accepted_result=raw;f.release_result=raw;StereoAdapter a(f,ready());
        {auto entry=a.enter();assert(a.begin_epoch(entry,identity(),layout()));auto r=a.create(entry,command(),1,4);
            assert(r.admitted&&r.raw_valid&&r.raw==raw&&r.public_handle==100);}
        submit_create(a);
        {auto entry=a.enter();assert(a.observe_producer(entry,producer(0)));bool reset{};assert(a.prepare_evaluate(entry,use(0),command(0),100,4,false,reset));
            auto original=f.eval_structure;assert(patch_evaluate(original,f.eval_structure,layout(),0,reset));
            auto r=a.evaluate(entry,command(0).raw,100,4,0);assert(r.admitted&&r.raw_valid&&r.raw==raw);}
        submit(a,0,2,2);
        {auto entry=a.enter();a.invalidate(entry);NativeResult released{};
            assert(a.release_completed(entry,100,completion(2),released)&&released==raw);
            assert(a.release_completed(entry,101,completion(2),released)&&released==raw&&a.drained(entry));}
    }
    for(NativeResult raw:{0xbad00000u,dispatch_unavailable,0xbadfffffu})assert(!native_result_accepted(raw));
    // The dispatch ABI returns uint32_t. Upper RAX bits aren't SDK status bits.
    assert(native_result_accepted(static_cast<NativeResult>(0xbad0000000000001ull)));
    assert(!native_result_accepted(static_cast<NativeResult>(0xffffffffbad00007ull)));
}
static void candidate_observations_and_quarantine(){
    // All capability gates true still cannot publish, or Eval without observed
    // producer data. The capability object has no dynamic-acceptance flags.
    {Fake f;StereoAdapter a(f,ready());start(a);submit_create(a);auto entry=a.enter();
        RecordedOutput out;bool reset{};assert(a.phase(entry)==AcceptancePhase::candidate);
        assert(!a.recorded_output(entry,out));assert(!a.prepare_evaluate(entry,use(0),command(0),100,4,false,reset));
        assert(f.evaluates==0&&a.phase(entry)==AcceptancePhase::quarantined);}
    for(unsigned bad=0;bad<8;++bad){Fake f;StereoAdapter a(f,ready());start(a);submit_create(a);
        auto p=producer(0);switch(bad){case 0:++p.use.identity.epoch;break;case 1:p.view=0;break;
            case 2:p.rendered[1].x=1280;break;case 3:p.rendered[2].width=1280;break;
            case 4:p.resources[3].extent.width=1280;break;case 5:p.resources[0].device++;break;
            case 6:p.resources[3].identity=p.resources[0].identity;break;case 7:p.use.eye=1;break;}
        auto entry=a.enter();assert(!a.observe_producer(entry,p)&&f.evaluates==0&&a.phase(entry)==AcceptancePhase::quarantined);
    }
    // Copy must use the exact resources whose producers were observed.
    {Fake f;StereoAdapter a(f,ready());start(a);submit_create(a);assert(eval(a,f,0));
        auto changed=facts(0);changed.resources[2].identity+=20;auto entry=a.enter();
        assert(!a.copy(entry,changed,0)&&f.copies==0&&a.phase(entry)==AcceptancePhase::quarantined);}
    // A candidate is bounded to one pair. A new ticket cannot overwrite its
    // pending acceptance evidence even after both original submissions return.
    {Fake f;StereoAdapter a(f,ready());start(a);submit_create(a);assert(eval(a,f,0));copy(a,0);assert(eval(a,f,1));copy(a,1);
        submit(a,0,2,2);submit(a,1,3,3);auto entry=a.enter();
        assert(!a.observe_producer(entry,producer(0,11))&&f.evaluates==2&&a.phase(entry)==AcceptancePhase::quarantined);}
    // Unknown or incomplete acceptance is a failed trial, not an automatic
    // retry. Already-recorded work is still retired through its real receipts.
    for(unsigned bad=0;bad<12;++bad){Fake f;StereoAdapter a(f,ready());start(a);submit_create(a);
        assert(eval(a,f,0));copy(a,0);assert(eval(a,f,1));copy(a,1);submit(a,0,2,2);submit(a,1,3,3);
        auto observed=observations();switch(bad){case 0:observed[1].feature=100;break;
            case 1:observed[0].use.identity.epoch++;break;case 2:observed[1].use.ticket++;break;
            case 3:observed[1].use.generation++;break;case 4:observed[0].output.identity++;break;
            case 5:observed[0].output.extent.height=720;break;case 6:observed[1].produced.x=1280;break;
            case 7:observed[1].produced.width=853;break;case 8:observed[1].completion.value=2;break;
            case 9:observed[0].completion.fence++;break;case 10:observed[0].completion.value=UINT64_MAX;break;
            case 11:observed[0].use.eye=1;break;}
        auto entry=a.enter();RecordedOutput out;assert(!a.accept_candidate(entry,observed));
        assert(a.phase(entry)==AcceptancePhase::quarantined&&!a.recorded_output(entry,out)&&!a.ordered_output(entry,out));
        assert(!a.accept_candidate(entry,observations()));NativeResult raw{};
        assert(!a.release_completed(entry,100,completion(UINT64_MAX),raw)&&f.releases==0);
        assert(a.release_completed(entry,100,completion(3),raw)&&a.release_completed(entry,101,completion(3),raw)&&a.drained(entry));
    }
    {Fake f;StereoAdapter a(f,ready());start(a);submit_create(a);assert(eval(a,f,0));copy(a,0);assert(eval(a,f,1));copy(a,1);
        auto entry=a.enter();assert(!a.accept_candidate(entry,observations()));assert(a.phase(entry)==AcceptancePhase::quarantined);
        assert(a.submitted_eye(entry,0,execution(0,2,2))&&a.submitted_eye(entry,1,execution(1,3,3)));
        NativeResult raw{};assert(a.release_completed(entry,100,completion(3),raw)&&a.release_completed(entry,101,completion(3),raw));}
    // After acceptance, last ticket's observations cannot authorize another
    // ticket's SDK invocation; persistent acceptance does not waive this join.
    {Fake f;StereoAdapter a(f,ready());start(a);submit_create(a);assert(eval(a,f,0));copy(a,0);assert(eval(a,f,1));copy(a,1);
        submit(a,0,2,2);submit(a,1,3,3);accept(a);auto entry=a.enter();bool reset{};
        assert(!a.prepare_evaluate(entry,use(0,11),command(0),100,4,false,reset)&&f.evaluates==2);}
    {Fake f;StereoAdapter a(f,ready());start(a);submit_create(a);assert(eval(a,f,0));copy(a,0);assert(eval(a,f,1));copy(a,1);
        submit(a,0,2,2);submit(a,1,3,3);accept(a);auto changed=producer(0,11);changed.resources[3].identity+=20;
        auto entry=a.enter();assert(!a.observe_producer(entry,changed)&&f.evaluates==2&&a.phase(entry)==AcceptancePhase::quarantined);}
}
static void zero_handle_creation_retirement(){
    for(unsigned outcome=0;outcome<3;++outcome){Fake f;f.zero_create=0;f.fail_create=outcome==0?0:99;f.throw_create=outcome==2;
        StereoAdapter a(f,ready());auto entry=a.enter();assert(a.begin_epoch(entry,identity(),layout()));
        bool threw{};try{auto result=a.create(entry,command(),1,4);assert(!result.admitted&&result.raw_valid);
            assert(result.raw==(outcome==0?dispatch_unavailable:named_result_success));}
        catch(std::runtime_error const&){threw=true;}
        assert(threw==(outcome==2)&&f.creates==1&&!a.owns(entry,100)&&!a.drained(entry));
        assert(!a.complete_creation(entry,completion(99))&&!a.begin_epoch(entry,{2,2,3,4,6},layout()));
        assert(a.submitted_creation(entry,execution(2,1,5)));
        for(unsigned bad=0;bad<5;++bad){auto c=completion(5);switch(bad){case 0:c.value=4;break;
            case 1:c.device++;break;case 2:c.queue++;break;case 3:c.fence++;break;case 4:c.value=UINT64_MAX;break;}
            assert(!a.complete_creation(entry,c)&&!a.drained(entry)&&!a.begin_epoch(entry,{2,2,3,4,6},layout()));}
        NativeResult raw=99;assert(!a.release_completed(entry,100,completion(4),raw)&&raw==99&&f.releases==0);
        assert(a.complete_creation(entry,completion(5))&&a.drained(entry)&&f.releases==0);
        assert(a.begin_epoch(entry,{2,2,3,4,6},layout())&&a.phase(entry)==AcceptancePhase::candidate);
        assert(!a.complete_creation(entry,completion(99))&&f.releases==0);
    }
    // A zero second result leaves the first handle and the shared Create
    // command obligation owned. Completing Create must not release that handle.
    {Fake f;f.zero_create=1;f.fail_create=1;StereoAdapter a(f,ready());auto entry=a.enter();
        assert(a.begin_epoch(entry,identity(),layout()));auto result=a.create(entry,command(),1,4);
        assert(!result.admitted&&result.raw==dispatch_unavailable&&f.creates==2&&a.owns(entry,100)&&!a.owns(entry,101));
        assert(!a.drained(entry)&&!a.begin_epoch(entry,{2,2,3,4,6},layout()));
        assert(a.submitted_creation(entry,execution(2,1,5))&&a.complete_creation(entry,completion(5)));
        assert(!a.drained(entry)&&!a.begin_epoch(entry,{2,2,3,4,6},layout())&&f.releases==0);
        NativeResult raw{};assert(a.release_completed(entry,100,completion(5),raw)&&raw==named_result_success);
        assert(a.drained(entry)&&f.releases==1&&a.begin_epoch(entry,{2,2,3,4,6},layout()));}
}
static void native_create_before_resources(){
    auto logical=layout();logical.physical={};assert(valid_create_layout(logical)&&!valid(logical));
    auto partial=logical;partial.physical[0]={2560,1080};assert(!valid_create_layout(partial));
    for(unsigned fault=0;fault<9;++fault){Fake f;StereoAdapter a(f,ready());
        {auto entry=a.enter();assert(a.begin_epoch(entry,identity(),logical));assert(a.create(entry,command(),1,4).admitted);
            Layout unbound;assert(!a.bound_layout(entry,unbound)&&f.creates==2&&!f.evaluates);
            std::array<PendingRecording,5> pending;assert(a.pending_recordings(entry,pending)==1&&pending[0].kind==RecordingKind::creation);
            auto p=producer(0);switch(fault){case 0:p.resources[1].identity=0;break;case 1:p.resources[2].device++;break;
                case 2:p.resources[1].extent.width=1280;break;case 3:p.resources[2].samples=4;break;
                case 4:p.rendered[1].width=1280;break;case 5:p.view=0;break;case 6:p.use.identity.world++;break;
                case 7:p.use.ticket=0;break;case 8:p.resources[3].identity=p.resources[0].identity;break;}
            assert(!a.observe_producer(entry,p)&&!a.bound_layout(entry,unbound)&&f.evaluates==0);
            NativeResult raw{};assert(!a.release_completed(entry,100,completion(99),raw));
            assert(a.submitted_creation(entry,execution(2,1,1)));
            assert(a.release_completed(entry,100,completion(1),raw)&&a.release_completed(entry,101,completion(1),raw)&&a.drained(entry));}
    }
    {Fake f;StereoAdapter a(f,ready());{auto entry=a.enter();assert(a.begin_epoch(entry,identity(),logical));assert(a.create(entry,command(),1,4).admitted);}
        submit_create(a);assert(eval(a,f,0));copy(a,0);
        {auto entry=a.enter();Layout bound;assert(a.bound_layout(entry,bound)&&same(bound,layout()));
            auto changed=producer(1);changed.resources[2].identity+=99;assert(!a.observe_producer(entry,changed)&&f.evaluates==1);
            std::array<PendingRecording,5> pending;assert(a.pending_recordings(entry,pending)==2&&pending[0].kind==RecordingKind::evaluate&&pending[1].kind==RecordingKind::copy);}
        submit(a,0,2,2);auto entry=a.enter();NativeResult raw{};
        assert(a.release_completed(entry,100,completion(2),raw)&&a.release_completed(entry,101,completion(2),raw)&&a.drained(entry));}
}
static ExecuteReceipt copy_execution(unsigned eye){return {{3,90+eye,30+eye,70,80,eye?5u:3u,true},eye?5u:3u,0};}
static void transport_pair(StereoAdapter& a,Fake& f,bool final_copy=true){
    start(a);submit_create(a);
    for(unsigned eye=0;eye<2;++eye){assert(eval(a,f,eye));auto copied=facts(eye);copied.list=90+eye;copied.generation=30+eye;
        auto entry=a.enter();assert(a.copy(entry,copied,eye));}
    submit(a,0,2,2);{auto entry=a.enter();assert(a.submitted_copy(entry,0,copy_execution(0)));}
    submit(a,1,4,4);if(final_copy){auto entry=a.enter();assert(a.submitted_copy(entry,1,copy_execution(1)));}
}
static void transport_admission(){
    // Production may signal before hardware completes. Repeated normal holds
    // retain the candidate and never run another SDK call or reset its history.
    {Fake f;StereoAdapter a(f,ready());{auto e=a.enter();assert(a.complete_candidate_transport(e,{})==TransportCompletion::incomplete);}
        transport_pair(a,f);
        for(uint64_t completed=0;completed<5;++completed){auto e=a.enter();
            assert(a.complete_candidate_transport(e,completion(completed))==TransportCompletion::pending);
            assert(a.admit_ticket(e,2,11,completion(completed))==TicketAdmission::hold);
            assert(a.phase(e)==AcceptancePhase::candidate&&f.evaluates==2);}
        {auto e=a.enter();assert(a.admit_ticket(e,2,11,completion(5))==TicketAdmission::admitted);
            assert(a.complete_candidate_transport(e,completion(5))==TransportCompletion::accepted);
            assert(a.complete_candidate_transport(e,completion(5))==TransportCompletion::accepted);
            RecordedOutput out;assert(a.ordered_output(e,out)&&out.ticket==10);}
        assert(eval(a,f,0,11,false,false)); // Fresh producer and independent existing history.
        assert(f.histories[0]==2&&f.histories[1]==1);
    }
    // No wait/admission can stand in for a missing or reset Copy recording.
    {Fake f;StereoAdapter a(f,ready());transport_pair(a,f,false);auto e=a.enter();
        auto changed=copy_execution(1);++changed.submission.generation;assert(!a.submitted_copy(e,1,changed));
        assert(a.complete_candidate_transport(e,completion(999))==TransportCompletion::incomplete);
        assert(a.admit_ticket(e,2,11,completion(999))==TicketAdmission::rejected);}
    for(unsigned bad=0;bad<4;++bad){Fake f;StereoAdapter a(f,ready());transport_pair(a,f);auto c=completion(5);
        if(bad==0)++c.device;if(bad==1)++c.queue;if(bad==2)++c.fence;if(bad==3)c.value=UINT64_MAX;
        auto e=a.enter();assert(a.complete_candidate_transport(e,c)==TransportCompletion::rejected);}
    {Fake f;StereoAdapter a(f,ready());transport_pair(a,f);auto e=a.enter();
        assert(!a.submitted_eye(e,0,execution(0,2,2))); // Duplicate/old recording cannot accept.
        assert(a.complete_candidate_transport(e,completion(5))==TransportCompletion::rejected);}
    {Fake f;StereoAdapter a(f,ready());transport_pair(a,f);auto e=a.enter();
        assert(a.admit_ticket(e,2,11,completion(4))==TicketAdmission::hold);a.invalidate(e); // stop/timeout
        assert(a.admit_ticket(e,2,11,completion(5))==TicketAdmission::rejected);NativeResult raw{};
        assert(!a.release_completed(e,100,completion(4),raw)&&!f.releases);
        assert(a.release_completed(e,100,completion(5),raw)&&a.release_completed(e,101,completion(5),raw)&&a.drained(e));}
    {Fake f;StereoAdapter a(f,ready());transport_pair(a,f);auto e=a.enter();
        assert(a.admit_ticket(e,3,11,completion(5))==TicketAdmission::rejected);}
}
int main(){transport_admission();native_create_before_resources();patch_boundaries();disabled_and_prerequisites();independent_images_and_histories();create_failures_and_gpu_retirement();
    failed_recorded_eval_and_copy();bad_order_and_concurrency();boundary_rejections();result_mask_contract();candidate_observations_and_quarantine();zero_handle_creation_retirement();
    std::cout<<"NGX adapter SDK/copy/retirement boundary tests passed\n";}
