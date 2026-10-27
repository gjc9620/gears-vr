// Exercise actual opt-in host/wrapper code with synthetic memory and typed
// boundary injection. No detours, COM, SDK, GPU or game execution.
#define GEARS_NGX_STEREO_HOST_OBSERVER 1
#define GEARS_OWNED_OUTPUT_TRACKING 1
#define GEARS_NGX_STEREO_HOST_TEST 1
#define main route_observer_test_main
#include "ngx_route_observer_test.cpp"
#undef main
#include "gpu_viewport_stats.h"
namespace {
bool color_copy_diagnostics=true;
struct Frame {uint64_t generation=1;struct Ref {uintptr_t identity;uint64_t generation;};Ref current(uintptr_t id){return {id,generation};}} frame_link;
gears_viewport::Tracker frame_target_stats;
uintptr_t canonical_identity(IUnknown* object){return reinterpret_cast<uintptr_t>(object);}
bool install(void*,void*,void**){return true;}
#if defined(GEARS_NGX_INPUT_RECT_EXPERIMENT) && GEARS_NGX_INPUT_RECT_EXPERIMENT
enum class SessionStop {proof_failure};unsigned experiment_stops{};
void request_session_stop(SessionStop){++experiment_stops;}
namespace ngx_epoch_pair {unsigned changes{};void changed(){++changes;}}
#endif
}
#define _ReturnAddress test_return_address
#pragma warning(push)
#pragma warning(disable:4459) // Synthetic fixture globals named handle/parameters.
#include "ngx_stereo_host_live.inl"
#pragma warning(pop)
#undef _ReturnAddress
namespace ngx_stereo_host {
unsigned typed_calls{};
Typed typed(ID3D12Resource* resource){++typed_calls;Typed out;out.raw=out.canonical=reinterpret_cast<uintptr_t>(resource);out.device=0xd00;out.hr=S_OK;
    out.desc.Width=2560;out.desc.Height=1080;out.desc.Format=DXGI_FORMAT_R11G11B10_FLOAT;return out;}
bool vendor_owner(uintptr_t,uintptr_t&){return false;}
UINT descriptor_stride(ID3D12GraphicsCommandList*){return 32;}
}
namespace {
unsigned dispatch_calls{},copy_calls{},execute_calls{},release_calls{};bool throw_dispatch{};uint32_t create_raw=0x12345678;
uint32_t __fastcall forward_eval(uintptr_t command,uintptr_t h,uintptr_t p,uintptr_t callback){unlocked();++dispatch_calls;
    assert(command==0x100&&h==handle&&p==parameters&&!callback);if(throw_dispatch)throw std::runtime_error("dispatcher");return 0xbad01234;}
uint32_t __fastcall forward_create(uintptr_t command,uint32_t feature,uintptr_t p,uintptr_t* output){unlocked();++dispatch_calls;
    assert(command==0x100&&feature==1&&p==parameters&&reinterpret_cast<uintptr_t>(output)==base+handle_cell_rva);handle=0x12345;return create_raw;}
uint32_t __fastcall forward_release(uintptr_t h){unlocked();++release_calls;assert(h==handle);return 0x87654321;}
void STDMETHODCALLTYPE forward_copy(ID3D12GraphicsCommandList* command,D3D12_TEXTURE_COPY_LOCATION const* dst,UINT x,UINT y,UINT z,D3D12_TEXTURE_COPY_LOCATION const* src,D3D12_BOX const* box){
    unlocked();++copy_calls;assert(command==reinterpret_cast<ID3D12GraphicsCommandList*>(0x100)&&!x&&!y&&!z&&!box);assert(src->pResource==reinterpret_cast<ID3D12Resource*>(0x501)&&dst->pResource==reinterpret_cast<ID3D12Resource*>(0x500));}
void STDMETHODCALLTYPE forward_execute(ID3D12CommandQueue* queue,UINT count,ID3D12CommandList* const* commands){unlocked();++execute_calls;assert(queue==reinterpret_cast<ID3D12CommandQueue*>(0x200)&&count==1&&commands[0]==reinterpret_cast<ID3D12CommandList*>(0x100));}
void host_reset(){reset();using namespace ngx_stereo_host;attested=true;printed=false;bank={};rows={};releases={};row_count=epoch_evals=epochs=in_wrappers=release_count=0;
    sampled_tickets=typed_observations=total_sampled_tickets=total_typed_observations=0;
    latest_native_create={};native_create_revision=0;source_create_scope=nullptr;
    sequence=execute_sequence=limited=foreign_dispatches=read_failures=0;current_row=pending_row=-1;typed_calls=0;
    ngx_stereo_host::original_create=forward_create;ngx_stereo_host::original_evaluate=forward_eval;original_release=forward_release;frame_link.generation=1;
    dispatch_calls=copy_calls=execute_calls=release_calls=0;throw_dispatch=false;create_raw=0x12345678;
    for(unsigned i=0;i<4;++i)put(evaluation,resource_offsets[i],uintptr_t(0x500+i));}
void observe_inputs(){auto* command=reinterpret_cast<ID3D12GraphicsCommandList*>(0x100);
    std::array<D3D12_RESOURCE_BARRIER,4> barriers{};for(unsigned i=0;i<4;++i){auto& b=barriers[i];b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition.pResource=reinterpret_cast<ID3D12Resource*>(uintptr_t(0x500+i));b.Transition.StateAfter=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;}
    ngx_stereo_host::barriers(command,4,barriers.data(),base+0xabc);
    frame_target_stats.reset(0x100);frame_target_stats.create(0x1000,{0x500,2560,1080,26,0});
    D3D12_CPU_DESCRIPTOR_HANDLE descriptor{0x1000};ngx_stereo_host::targets(command,1,&descriptor,FALSE,nullptr);
    ngx_stereo_host::draw(command,base+0x123,6,1);
}
void eval(){ngx_stereo_host::WrapperScope scope(true,true,base+0x9a35d3,0x100,evaluation.data());
    const auto result=ngx_stereo_host::hook_evaluate(0x100,handle,parameters,0);assert(result==0xbad01234);scope.normal_return(0xfedcba9876543210);}
uint64_t __fastcall wrapper_forward(uintptr_t command,uintptr_t,uintptr_t,void const* structure){
    assert(structure==evaluation.data());assert(ngx_stereo_host::hook_evaluate(command,handle,parameters,0)==0xbad01234);return 0xfedcba9876543210;}
void copy_execute(){D3D12_TEXTURE_COPY_LOCATION src{},dst{};src.pResource=reinterpret_cast<ID3D12Resource*>(0x501);dst.pResource=reinterpret_cast<ID3D12Resource*>(0x500);
    src.Type=dst.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    ngx_stereo_host::copy_passthrough(reinterpret_cast<ID3D12GraphicsCommandList*>(0x100),&dst,0,0,0,&src,nullptr,base+0x1c6093,forward_copy);
    auto* list=reinterpret_cast<ID3D12CommandList*>(0x100);ngx_stereo_host::execute_passthrough(reinterpret_cast<ID3D12CommandQueue*>(0x200),1,&list,forward_execute);}
void source_cases(){
    using namespace ngx_stereo_host;
    auto dimensions=[](){const uint32_t values[]{1706,720,2560,1080,1,0x12};
        for(unsigned i=0;i<6;++i)put(creation,i*4,values[i]);put(creation,0x18,uint8_t(1));};
    auto dispatch=[](){return hook_create(0x100,1,parameters,reinterpret_cast<uintptr_t*>(base+handle_cell_rva));};
    auto complete=[&](){WrapperScope scope(false,true,base+0xbe66bc,0x100,creation.data());
        NativeCreateSource pending;assert(!native_create_source(9,pending));scope.normal_return(dispatch());};
    NativeCreateSource source;uint32_t w=7,h=9;
    host_reset();dimensions();assert(native_create_state(9,source)==NativeCreateAvailability::unavailable);
    handle=0;assert(native_create_state(9,source)==NativeCreateAvailability::inactive);
    cells_available=false;assert(native_create_state(9,source)==NativeCreateAvailability::unavailable);cells_available=true;
    complete();
    assert(native_create_source(9,source)&&source.fields[0]==1706&&source.fields[1]==720&&source.fields[4]==1&&source.fields[5]==0x12&&source.fields[6]==1);
    assert(source.split_input(2560,1080,w,h)&&w==853&&h==720);
    w=7;h=9;assert(!source.split_input(4096,2048,w,h)&&w==7&&h==9); // No ratio scaling to owned output.
    auto invalid=source;invalid.fields[0]=1707;assert(!invalid.split_input(2560,1080,w,h));
    invalid=source;invalid.fields[1]=1081;assert(!invalid.split_input(2560,1080,w,h));
    // Diagnostic quotas can exhaust; actual latest Create semantics remain fresh.
    const auto first=source.revision;for(unsigned i=0;i<5;++i)complete();
    assert(epochs>=4&&native_create_source(9,source)&&source.revision>first);
    assert(!native_create_source(8,source)&&!source.revision);
    ++handle;assert(!native_create_source(9,source));--handle;assert(!native_create_source(9,source));complete();
    ++parameters;assert(!native_create_source(9,source));--parameters;assert(!native_create_source(9,source));complete();
    committed_world_generation=10;assert(!native_create_source(9,source)&&!native_create_source(10,source));committed_world_generation=9;
    assert(!native_create_source(9,source));complete();
    cells_available=false;assert(!native_create_source(9,source));cells_available=true;assert(!native_create_source(9,source));complete();
    recording=false;assert(!native_create_source(9,source));recording=true;
    hook_release(handle);assert(!native_create_source(9,source));
    complete();assert(native_create_source(9,source));
    // Failure, missing/duplicate dispatcher, mutated fields, unrelated callsite
    // and nested/concurrent scope all invalidate rather than revive old fields.
    create_raw=0xbad00001;complete();assert(!native_create_source(9,source));create_raw=0x12345678;
    complete();{WrapperScope scope(false,true,base+0xbe66bc,0x100,creation.data());scope.normal_return(create_raw);}
    assert(!native_create_source(9,source));
    complete();{WrapperScope scope(false,true,base+0xbe66bc,0x100,creation.data());dispatch();scope.normal_return(dispatch());}
    assert(!native_create_source(9,source));
    complete();{WrapperScope scope(false,true,base+0xbe66bc,0x100,creation.data());scope.normal_return(dispatch());put(creation,0,uint32_t(1600));}
    assert(!native_create_source(9,source));dimensions();
    complete();{WrapperScope scope(false,true,base+0xbe66bd,0x100,creation.data());scope.normal_return(dispatch());}
    assert(!native_create_source(9,source));
    complete();{WrapperScope scope(false,true,base+0xbe66bc,0x100,creation.data());
        {WrapperScope nested(true,true,base+0x9a35d3,0x100,evaluation.data());}scope.normal_return(dispatch());}
    assert(!native_create_source(9,source));
    complete();{WrapperScope scope(false,true,base+0xbe66bc,0x100,creation.data());
        std::thread competing([&]{complete();});competing.join();scope.normal_return(dispatch());}
    assert(!native_create_source(9,source));
    complete();{WrapperScope scope(false,true,base+0xbe66bc,0x100,creation.data());scope.normal_return(dispatch());
        std::thread release_during_create([&]{hook_release(handle);});release_during_create.join();}
    assert(!native_create_source(9,source));
    complete();try{WrapperScope scope(false,true,base+0xbe66bc,0x100,creation.data());dispatch();throw std::runtime_error("Create wrapper");}
    catch(std::runtime_error const&){}assert(!native_create_source(9,source));
    complete();unreadable=reinterpret_cast<uintptr_t>(creation.data());complete();unreadable=0;
    assert(!native_create_source(9,source));
    complete();dispatch();assert(!native_create_source(9,source)); // Foreign dispatcher, no wrapper.
}
void constructor_source_cases(){
    using namespace ngx_stereo_host;host_reset();
    const uint32_t values[]{2560,1440,3840,2160,2,67};
    for(unsigned i=0;i<6;++i)put(creation,i*4,values[i]);put(creation,0x18,uint8_t(0));
    auto complete=[](){WrapperScope scope(false,true,base+0xbe66bc,0x100,creation.data());
        scope.normal_return(hook_create(0x100,1,parameters,reinterpret_cast<uintptr_t*>(base+handle_cell_rva)));};
    complete();NativeCreateSource saved;assert(native_create_source(9,saved)&&native_create_matches(saved));
    uint32_t width{},height{};gears_ngx_stereo::Layout layout;
    assert(saved.split_input(3840,2160,width,height)&&width==1280&&height==1440);
    assert(saved.create_layout(layout)&&layout.stride==1920&&layout.eye_output.width==1920&&layout.eye_output.height==2160);
    auto altered=saved;for(unsigned i=0;i<7;++i){altered=saved;++altered.fields[i];assert(!native_create_matches(altered));}
    altered=saved;++altered.world;assert(!native_create_matches(altered));
    altered=saved;++altered.revision;assert(!native_create_matches(altered));
    altered=saved;++altered.handle;assert(!native_create_matches(altered));
    altered=saved;++altered.parameters;assert(!native_create_matches(altered));
    assert(native_create_matches(saved)); // Bad caller metadata cannot poison current source.
#if defined(GEARS_NGX_INPUT_RECT_EXPERIMENT) && GEARS_NGX_INPUT_RECT_EXPERIMENT
    consuming_frozen_frame.ngx_constructor_source=saved;experiment_stops=ngx_epoch_pair::changes=0;
    eval();assert(experiment_stops==0&&ngx_epoch_pair::changes==0&&dispatch_calls==2);
    ++handle;eval();assert(experiment_stops==1&&ngx_epoch_pair::changes==1&&dispatch_calls==3);
    --handle;assert(!native_create_matches(saved)); // Observed loss is sticky even if same address returns.
    consuming_frozen_frame.ngx_constructor_source={};complete();assert(native_create_source(9,saved));
    consuming_frozen_frame.ngx_constructor_source=saved;complete(); // Late Create forwards, stops old renderer.
    assert(experiment_stops==2&&ngx_epoch_pair::changes==2&&!native_create_matches(saved));
    consuming_frozen_frame.ngx_constructor_source={};
#endif
    complete();assert(native_create_source(9,saved));++parameters;assert(!native_create_matches(saved));
    --parameters;assert(!native_create_matches(saved));complete();assert(native_create_source(9,saved));
    cells_available=false;assert(!native_create_matches(saved));cells_available=true;assert(!native_create_matches(saved));
    complete();assert(native_create_source(9,saved));committed_world_generation=10;assert(!native_create_matches(saved));
    committed_world_generation=9;assert(!native_create_matches(saved));complete();assert(native_create_source(9,saved));
    complete();NativeCreateSource newer;assert(native_create_source(9,newer)&&!native_create_matches(saved)&&native_create_matches(newer));
    hook_release(handle);assert(!native_create_matches(newer));
}
struct ControlFixture {
    unsigned creates{},evaluates{},copies{},releases{};uint64_t completed{};bool wrong_ticket{};
    unsigned queries{},query_failure{};
    std::array<uint64_t,2> used{};gears_ngx_stereo::CreateShadow consumed_create{};
    gears_ngx_stereo::EvaluateShadow consumed_eval{};
    static gears_ngx_stereo::Identity identity(){return {1,9,0xd00,4,1};}
    static gears_ngx_stereo::Layout layout(){gears_ngx_stereo::Layout l;
        for(auto& extent:l.physical)extent={2560,1080};l.full_output={2560,1080};l.input={853,720};
        l.eye_output={1280,1080};l.stride=1280;l.input_base={gears_ngx_stereo::Point{0,0},{1280,0}};return l;}
    static gears_ngx_stereo::Command command(unsigned index){return {0x100u*(index+1),0x100u*(index+1),index+1,0xd00,true};}
    static gears_ngx_stereo::ExecuteReceipt execution(unsigned index,uint64_t value){auto c=command(index);
        return {{0xd00,c.list,c.generation,0x20,0x30,value,true},value,0};}
    static std::array<gears_ngx_stereo::Resource,4> resources(){std::array<gears_ngx_stereo::Resource,4> r{};
        for(unsigned i=0;i<4;++i)r[i]={0x500u+i,0xd00,{2560,1080},26,0,1,0};return r;}
    static bool creation_binding(void*,ngx_stereo_host::NativeCreateSource const& source,uintptr_t raw,ngx_stereo_host::ControlCreateBinding& out){
        assert(source.fields[0]==1706&&source.fields[1]==720&&source.fields[2]==2560&&source.parameters==4&&raw==0x100);
        gears_ngx_stereo::Layout logical;assert(source.create_layout(logical));
        for(auto physical:logical.physical)assert(!physical.width&&!physical.height);
        assert(!source.handle);out={identity(),logical,command(0)};return true;}
    static bool input_binding(void* owner_context,ngx_stereo_host::ControlConsumption consumed,gears_ngx_stereo::EvaluateShadow const&,uintptr_t raw,
        ngx_stereo_host::ControlInputTicket& out){
        auto& f=*static_cast<ControlFixture*>(owner_context);out.consumption=consumed;if(f.wrong_ticket)++out.consumption.ticket;
        out.original_views={reinterpret_cast<uintptr_t>(views.data()),reinterpret_cast<uintptr_t>(views.data())+0xd30};
        out.command=command(raw==0x100?0:2);out.raw_resources={0x500,0x502,0x503,0x501};
        for(unsigned eye=0;eye<2;++eye){auto& p=out.producers[eye];auto cmd=command(2*eye);
            p.use={identity(),consumed.ticket,cmd.list,cmd.generation,eye};p.view=out.original_views[eye];
            p.resources=resources(); // Adapter order C,D,M,O differs from native C,O,D,M.
            p.resources={resources()[0],resources()[2],resources()[3],resources()[1]};
            for(auto& rect:p.rendered)rect={eye*1280,0,853,720};}
        return true;}
    static gears_ngx_stereo::Completion completion(void* owner_context,uint64_t){return {0xd00,0x20,0x30,static_cast<ControlFixture*>(owner_context)->completed};}
    static bool admit(void*,ngx_stereo_host::ControlConsumption used){
        // Entered through the complete real route hook/WrapperScope/Call/host
        // chain, not just the extracted Context callback. Neither lock may leak.
        unlocked();auto owner=ngx_stereo_host::current_control();assert(owner);
        {auto entry=owner->adapter().enter();assert(entry);}
        return used.world==identity().world&&used.ticket!=0;}
};
ControlFixture* control_fixture{};
uint32_t __fastcall control_optimal(uint32_t original_width,uint32_t width,uint32_t height,uint32_t quality,
    uint32_t* optimal_w,uint32_t* optimal_h,uint32_t* max_w,uint32_t* max_h,uint32_t* min_w,uint32_t* min_h,float* sharpness){
    unlocked();auto& f=*control_fixture;++f.queries;
    assert(original_width==width&&width==1280&&height==1080&&quality==1&&!f.creates&&handle==0&&parameters==4);
    *optimal_w=853;*optimal_h=720;*max_w=1280;*max_h=1080;*min_w=640;*min_h=540;*sharpness=0.25f;
    if(f.query_failure==1)return 0xbad0000c;
    if(f.query_failure==2)*min_w=900; // Original family/unsupported contract is never halved to pass.
    if(f.query_failure==3)++parameters;
    if(f.query_failure==4)++committed_world_generation;
    if(f.query_failure==5)++ngx_stereo_host::native_create_revision;
    if(f.query_failure==6)handle=444;
    if(f.query_failure==7)put(creation,0,uint32_t(1708));
    return 1;
}
uint32_t __fastcall control_sdk_create(uint64_t raw,uint32_t feature,uint64_t p,uint64_t* out){
    unlocked();assert(raw==0x100&&feature==1&&p==4&&control_fixture->queries==1);*out=100+control_fixture->creates++;return 1;}
uint32_t __fastcall control_sdk_evaluate(uint64_t raw,uint64_t h,uint64_t p,uint64_t callback){
    unlocked();auto& f=*control_fixture;assert(h==100||h==101);const auto eye=unsigned(h-100);
    assert(raw==ControlFixture::command(2*eye).raw&&p==4&&!callback);f.used[f.evaluates++]=h;
    uint32_t render[3]{};std::memcpy(render,f.consumed_eval.bytes.data()+0x30,sizeof(render));
    assert(render[0]==853&&render[1]==720&&render[2]==1);return 1;}
uint32_t __fastcall control_sdk_release(uint64_t h){unlocked();assert(h==100||h==101);++control_fixture->releases;return 1;}
void control_copy(void* owner_context,gears_ngx_stereo::CopyPlan const& plan){auto& f=*static_cast<ControlFixture*>(owner_context);
    ++f.copies;assert(plan.list==ControlFixture::command(plan.ordinal?3:1).list&&plan.destination[0]==plan.ordinal*1280);}
uint64_t __fastcall control_create_wrapper(uintptr_t a,uintptr_t,uintptr_t,uintptr_t,uintptr_t,void const* p){
    std::memcpy(control_fixture->consumed_create.bytes.data(),p,gears_ngx_stereo::create_consumed_span);
    return ngx_stereo_host::hook_create(a,1,parameters,reinterpret_cast<uintptr_t*>(base+handle_cell_rva));}
uint64_t __fastcall control_eval_wrapper(uintptr_t a,uintptr_t,uintptr_t,void const* p){
    std::memcpy(control_fixture->consumed_eval.bytes.data(),p,gears_ngx_stereo::evaluate_initialized_span);
    return ngx_stereo_host::hook_evaluate(a,handle,parameters,0);}
void control_cases(){
    using namespace gears_ngx_stereo;using namespace ngx_stereo_host;
    for(bool wrong_ticket:{false,true}){
        host_reset();const auto saved_base=base;base=reinterpret_cast<uintptr_t>(&handle)-handle_cell_rva;handle=0;parameters=4;
        auto fixture=std::make_shared<ControlFixture>();control_fixture=fixture.get();fixture->wrong_ticket=wrong_ticket;
        NativeDispatchFunctions functions{control_sdk_create,control_sdk_evaluate,control_sdk_release,fixture.get(),control_copy};
        functions.optimal=control_optimal;
        AdapterPrerequisites ready{true,true,true,true,true,true,true,true,true,true,true,true};
        ControlCallbacks callbacks{fixture,ControlFixture::creation_binding,ControlFixture::input_binding,ControlFixture::completion,ControlFixture::admit};
        auto owner=std::make_shared<ControlOwner>(functions,ready,callbacks);assert(attach_control(owner));assert(!attach_control(owner));
        ngx_route_observer::original_create=control_create_wrapper;ngx_route_observer::original_evaluate=control_eval_wrapper;
        ngx_stereo_host::original_create=control_sdk_create;ngx_stereo_host::original_evaluate=control_sdk_evaluate;ngx_stereo_host::original_release=control_sdk_release;
        const uint32_t family[]{1706,720,2560,1080,1,0x12};std::memcpy(creation.data(),family,sizeof(family));put(creation,0x18,uint8_t(1));
        const auto family_original=creation;test_caller=base+0xbe66bc;
        assert(ngx_route_observer::hook_create(0x100,0,0,0,0,creation.data())==1&&fixture->creates==2&&handle==100&&creation==family_original);
        uint32_t actual[4]{};std::memcpy(actual,fixture->consumed_create.bytes.data(),sizeof(actual));
        assert(actual[0]==853&&actual[1]==720&&actual[2]==1280&&actual[3]==1080);
        {auto entry=owner->adapter().enter();Layout physical;assert(!owner->adapter().bound_layout(entry,physical));}
        NativeCreateSource family_source;assert(native_create_source(9,family_source)&&family_source.fields[0]==1706&&family_source.fields[2]==2560);
        assert(!detach_control(owner));{auto e=owner->adapter().enter();assert(owner->adapter().submitted_creation(e,ControlFixture::execution(0,1)));}
        // Actual TLS stays on right view for BOTH evaluations, matching O/P.
        put(context,0x14c0,reinterpret_cast<uintptr_t>(views.data())+0xd30);
        for(unsigned eye=0;eye<(wrong_ticket?1u:2u);++eye){
            EvaluateShadow source;source.bytes.fill(0x5b);const uintptr_t raw[]{0x500,0x501,0x502,0x503};
            for(unsigned i=0;i<4;++i)std::memcpy(source.bytes.data()+resource_offsets[i],&raw[i],8);
            for(size_t offset:{size_t(0x60),size_t(0x68),size_t(0x70)}){const Point p{eye*1280,0};std::memcpy(source.bytes.data()+offset,&p,8);}
            uint32_t no_reset{};std::memcpy(source.bytes.data()+0x38,&no_reset,4);const auto original=source.bytes;
            test_caller=base+0x9a35d3;auto result=ngx_route_observer::hook_evaluate(ControlFixture::command(2*eye).raw,0,0,source.bytes.data());
            assert(source.bytes==original);if(wrong_ticket){assert(result==dispatch_unavailable&&!fixture->evaluates);break;}
            assert(result==1);{auto entry=owner->adapter().enter();Layout physical;assert(owner->adapter().bound_layout(entry,physical)&&same(physical,ControlFixture::layout()));}
            CopyFacts facts;facts.identity=ControlFixture::identity();facts.ticket=7;facts.caller=0x1c6093;
            auto cmd=ControlFixture::command(2*eye+1);facts.list=cmd.list;facts.generation=cmd.generation;facts.list_device=0xd00;
            facts.direct=facts.copy_source=facts.copy_dest=true;auto r=ControlFixture::resources();facts.resources={r[0],r[2],r[3],r[1]};
            facts.input_base=ControlFixture::layout().input_base;facts.feature_output={1280,1080};facts.evaluated_ordinal=eye;
            auto e=owner->adapter().enter();assert(owner->adapter().copy(e,facts,eye));
            assert(owner->adapter().submitted_eye(e,eye,ControlFixture::execution(2*eye,2+2*eye)));
            auto bad=ControlFixture::execution(2*eye+1,3+2*eye);++bad.submission.generation;
            assert(!owner->adapter().submitted_copy(e,eye,bad));
            assert(owner->adapter().submitted_copy(e,eye,ControlFixture::execution(2*eye+1,3+2*eye)));
        }
        assert(!detach_control(owner)&&!fixture->releases);
        // A CPU return or Eval-only fence never retires the later Copy list.
        fixture->completed=wrong_ticket?0:4;assert(ngx_stereo_host::hook_release(100)==dispatch_unavailable&&!fixture->releases);
        fixture->completed=wrong_ticket?1:5;assert(ngx_stereo_host::hook_release(100)==1&&fixture->releases==2);
        assert(detach_control(owner));if(!wrong_ticket)assert(fixture->evaluates==2&&fixture->used[0]==100&&fixture->used[1]==101&&fixture->copies==2);
        base=saved_base;control_fixture=nullptr;
    }
}
void optimal_host_failures(){
    using namespace gears_ngx_stereo;using namespace ngx_stereo_host;
    for(unsigned failure=1;failure<=7;++failure){
        host_reset();const auto saved_base=base;base=reinterpret_cast<uintptr_t>(&handle)-handle_cell_rva;handle=0;parameters=4;
        auto fixture=std::make_shared<ControlFixture>();control_fixture=fixture.get();fixture->query_failure=failure;
        NativeDispatchFunctions functions{control_sdk_create,control_sdk_evaluate,control_sdk_release,fixture.get(),control_copy};
        AdapterPrerequisites ready{true,true,true,true,true,true,true,true,true,true,true,true};
        ControlCallbacks callbacks{fixture,ControlFixture::creation_binding,ControlFixture::input_binding,ControlFixture::completion,ControlFixture::admit};
        assert(!std::make_shared<ControlOwner>(functions,ready,callbacks)->configured());
        functions.optimal=control_optimal;auto missing_admission=callbacks;missing_admission.admit=nullptr;
        assert(!std::make_shared<ControlOwner>(functions,ready,missing_admission)->configured());
        auto owner=std::make_shared<ControlOwner>(functions,ready,callbacks);assert(attach_control(owner));
        ngx_route_observer::original_create=control_create_wrapper;
        const uint32_t family[]{1706,720,2560,1080,1,0x12};std::memcpy(creation.data(),family,sizeof(family));put(creation,0x18,uint8_t(1));
        const auto original=creation;test_caller=base+0xbe66bc;
        const auto result=ngx_route_observer::hook_create(0x100,0,0,0,0,creation.data());
        assert(result==(failure==1?0xbad0000cu:dispatch_unavailable)&&fixture->queries==1&&!fixture->creates&&!fixture->evaluates&&!fixture->releases);
        if(failure!=7)assert(creation==original);else assert(creation!=original);
        assert(detach_control(owner));base=saved_base;control_fixture=nullptr;
    }
}
void host_cases(){
    host_reset();observe_inputs();eval();copy_execute();auto const& row=ngx_stereo_host::rows[0];
    assert(dispatch_calls==1&&copy_calls==1&&execute_calls==1&&row.dispatcher_match&&row.dispatcher_returned&&row.returned&&row.wrapper_result==0xfedcba9876543210);
    assert(row.typed_mask==15&&row.producer_mask==1&&row.copies==1&&row.copy_returned&&row.execute_generation_match&&row.execute_returned);
    assert(row.eval_execute_returned&&row.eval_execute_generation_match&&row.eval_execute_occurrences==1);
    assert(!row.retained_mask&&!row.typed_set_ready); // Synthetic raw metadata cannot become retained COM proof.
    assert(ngx_stereo_host::typed_calls==6&&!ngx_stereo_host::in_wrappers&&!active.load());
    host_reset();eval();assert(!ngx_stereo_host::rows[0].typed_mask&&!ngx_stereo_host::typed_calls); // Raw NGX pointers never probed.
    host_reset();observe_inputs();ngx_route_observer::original_evaluate=wrapper_forward;test_caller=base+0x9a35d3;
    assert(ngx_route_observer::hook_evaluate(0x100,0,0,evaluation.data())==0xfedcba9876543210);
    assert(ngx_stereo_host::row_count==1&&dispatch_calls==1&&ngx_stereo_host::rows[0].dispatcher_match&&ngx_stereo_host::rows[0].returned);
    host_reset();cells_available=false;eval();assert(!ngx_stereo_host::rows[0].dispatcher_match&&dispatch_calls==1);
    host_reset();ngx_stereo_host::typed_observations=512;observe_inputs();eval();
    assert(!ngx_stereo_host::typed_calls&&!ngx_stereo_host::rows[0].typed_mask&&ngx_stereo_host::rows[0].bank_dropped==4);
    host_reset();ngx_stereo_host::sampled_tickets=8;observe_inputs();eval();assert(!ngx_stereo_host::typed_calls&&!ngx_stereo_host::rows[0].typed_mask);
    // Pre-menu normal frames may exhaust a bank before any NGX feature exists.
    // Only a real observed Create epoch replenishes it, never arbitrary Eval.
    host_reset();
    for(unsigned epoch=0;epoch<4;++epoch){
        if(epoch){ngx_stereo_host::WrapperScope create(false,true,base+0xbe66bc,0x100,creation.data());create.normal_return(1);}
        assert(ngx_stereo_host::epochs==epoch);
        for(unsigned ticket=1;ticket<=8;++ticket){
            consuming_frame_ticket=consuming_frozen_frame.id=100*epoch+ticket;
            for(unsigned repeat=0;repeat<16;++repeat)observe_inputs();
        }
        assert(ngx_stereo_host::sampled_tickets==8&&ngx_stereo_host::typed_observations==512);
        assert(ngx_stereo_host::typed_calls==512*(epoch+1));
        observe_inputs(); // Same-ticket typed quota is exhausted.
        consuming_frame_ticket=consuming_frozen_frame.id=100*epoch+9;
        observe_inputs(); // A ninth ticket cannot bypass either quota.
        assert(ngx_stereo_host::typed_calls==512*(epoch+1));
        consuming_frame_ticket=consuming_frozen_frame.id=100*epoch+8;
        eval();assert(ngx_stereo_host::rows[ngx_stereo_host::row_count-1].typed_mask==15);
        observe_inputs();assert(ngx_stereo_host::typed_calls==512*(epoch+1)); // Eval never replenishes.
    }
    {ngx_stereo_host::WrapperScope create(false,true,base+0xbe66bc,0x100,creation.data());create.normal_return(1);}
    observe_inputs();assert(ngx_stereo_host::epochs==4&&ngx_stereo_host::typed_calls==2048);
    assert(ngx_stereo_host::total_sampled_tickets==32&&ngx_stereo_host::total_typed_observations==2048);
    host_reset();observe_inputs();eval();frame_link.generation=2;auto* list=reinterpret_cast<ID3D12CommandList*>(0x100);
    ngx_stereo_host::execute_passthrough(reinterpret_cast<ID3D12CommandQueue*>(0x200),1,&list,forward_execute);
    assert(!ngx_stereo_host::rows[0].execute_generation_match&&execute_calls==1);
    assert(!ngx_stereo_host::rows[0].eval_execute_generation_match);
    // Copy execution on another list is independent of Eval's actual recording.
    host_reset();observe_inputs();eval();ngx_stereo_host::rows[0].copy_list={0x300,0x300,1};
    auto* different=reinterpret_cast<ID3D12CommandList*>(0x300);
    const auto any_execute=[](ID3D12CommandQueue*,UINT,ID3D12CommandList* const*){unlocked();++execute_calls;};
    ngx_stereo_host::execute_passthrough(reinterpret_cast<ID3D12CommandQueue*>(0x200),1,&different,any_execute);
    assert(ngx_stereo_host::rows[0].execute_returned&&!ngx_stereo_host::rows[0].eval_execute_returned);
    list=reinterpret_cast<ID3D12CommandList*>(0x100);
    ngx_stereo_host::execute_passthrough(reinterpret_cast<ID3D12CommandQueue*>(0x200),1,&list,forward_execute);
    assert(ngx_stereo_host::rows[0].eval_execute_generation_match&&ngx_stereo_host::rows[0].eval_execute_order==2&&
        ngx_stereo_host::rows[0].execute_order==1&&execute_calls==2);
    ngx_stereo_host::execute_passthrough(reinterpret_cast<ID3D12CommandQueue*>(0x200),1,&list,forward_execute);
    assert(ngx_stereo_host::rows[0].eval_execute_occurrences==2&&!ngx_stereo_host::rows[0].eval_execute_generation_match);
    host_reset();throw_dispatch=true;try{eval();assert(false);}catch(std::runtime_error const&){}
    assert(dispatch_calls==1&&!ngx_stereo_host::rows[0].returned&&ngx_stereo_host::rows[0].unwinding&&!ngx_stereo_host::rows[0].dispatcher_returned&&!ngx_stereo_host::in_wrappers);
    host_reset();for(unsigned i=0;i<5;++i)eval();assert(dispatch_calls==5&&ngx_stereo_host::row_count==2&&!ngx_stereo_host::in_wrappers);
    host_reset();{ngx_stereo_host::WrapperScope outer(true,true,base+0x9a35d3,0x100,evaluation.data());recording=false;
        {ngx_stereo_host::WrapperScope skipped(true,true,base+0x9a35d3,0x100,evaluation.data());}assert(ngx_stereo_host::in_wrappers==1);recording=true;}
    assert(!ngx_stereo_host::in_wrappers);
    host_reset();{ngx_stereo_host::WrapperScope create(false,true,base+0xbe66bc,0x100,creation.data());
        auto result=ngx_stereo_host::hook_create(0x100,1,parameters,reinterpret_cast<uintptr_t*>(base+handle_cell_rva));assert(result==0x12345678);create.normal_return(result);}
    assert(ngx_stereo_host::rows[0].dispatcher_match&&ngx_stereo_host::rows[0].handle_output==handle);
    assert(ngx_stereo_host::hook_release(handle)==0x87654321&&release_calls==1&&ngx_stereo_host::releases[0].returned);
    host_reset();observe_inputs();eval();recording=false;FILE* report{};assert(!tmpfile_s(&report));ngx_stereo_host::print(report);
    assert(contains(report,"gpu_completion_verified=0")&&contains(report,"ngx_stereo_host_resource row=0 input=3")&&contains(report,"safe_retirement_verified=0")==false);fclose(report);
    host_reset();FILE* rejected{};assert(!tmpfile_s(&rejected));assert(!ngx_stereo_host::enable(rejected)&&!ngx_stereo_host::attested);fclose(rejected);
}
}
int main(){assert(route_observer_test_main()==0);source_cases();constructor_source_cases();control_cases();optimal_host_failures();host_cases();std::puts("NgxStereoHostLiveTest passed (forwarding/provenance only; no SDK/GPU completion claims)");return 0;}
