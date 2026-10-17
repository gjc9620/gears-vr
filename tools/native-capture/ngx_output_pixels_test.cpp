// Actual NGX/copy/execute/reset forwarding wrappers; fake GPU backend only.
#define GEARS_NGX_OUTPUT_PIXELS 1
#define main ngx_observer_regression_main
#include "ngx_route_observer_test.cpp"
#undef main
namespace {
bool color_copy_diagnostics=true;
}
#include "ngx_output_pixels.inl"
namespace ngx_output_pixels {
struct FakeBackend final:Backend {
    bool typed=true,same_device=true,signal_ok=true,complete=true,start_ok=true,publish_ok=true,unknown_identity{},removed{};
    unsigned prepared{},appended{},released{},mapped{},signals{},publications{};unsigned format=26,width=2560,height=1080,destination_format=26,destination_width=2560,destination_height=1080;
    uint64_t generation=4;uintptr_t queue_id=0x900;
    bool start()override{return start_ok;}
    bool prepare(unsigned,Slot& s)override{++prepared;
        s.source_desc={0x101,width,height,format,3,1,1,1,0};s.destination_desc={0x102,destination_width,destination_height,destination_format,3,1,1,1,0};
        s.life.list={reinterpret_cast<uintptr_t>(s.raw_list),0x700,generation};s.life.held_identity=0x700;
        return typed&&same_device&&textures_supported(s.source_desc,s.destination_desc);}
    void append(unsigned,Slot& s,CopyFn forward)override{++appended;D3D12_TEXTURE_COPY_LOCATION own{};own.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        forward(s.raw_list,&own,0,0,0,&s.source,nullptr);}
    uintptr_t list_identity(ID3D12CommandList* list)override{return list&&!unknown_identity?0x700:0;}
    gears_frame_link::ListRef list_ref(uintptr_t id)override{return {id,generation};}
    bool queue_valid(unsigned,ID3D12CommandQueue* q,uintptr_t& id)override{id=queue_id;return q&&same_device;}
    bool signal(unsigned,ID3D12CommandQueue*)override{++signals;return signal_ok;}
    bool poll(unsigned,Slot& s)override{s.life.completion(removed?UINT64_MAX:complete?1:0);if(!s.life.may_map())return false;++mapped;s.life.saved=true;return true;}
    void release(unsigned)override{++released;}
    bool publish(uint64_t selected_ticket,uint64_t selected_world,uint64_t seq)override{
#if defined(GEARS_NGX_OUTPUT_DELAYED_CAPTURE) && GEARS_NGX_OUTPUT_DELAYED_CAPTURE
        assert(delayed.closed&&selected_ticket==delayed.selected_ticket&&selected_world==9&&seq==2);
#else
        assert(selected_ticket==32&&selected_world==9&&seq==2);
#endif
        ++publications;return publish_ok;}
    void close()override{}
};
FakeBackend fake_backend;Backend& backend(){return fake_backend;}
}
namespace {
unsigned pixel_mode{},original_copies{},extra_copies{},original_executes{},original_resets{},pixel_eval_calls{};
HRESULT reset_result=S_OK;
auto* pixel_list=reinterpret_cast<ID3D12GraphicsCommandList*>(uintptr_t(0x700));
auto* pixel_queue=reinterpret_cast<ID3D12CommandQueue*>(uintptr_t(0x900));
uint64_t __fastcall pixel_original_evaluate(uintptr_t a,uintptr_t b,uintptr_t c,const void* input);
void STDMETHODCALLTYPE pixel_original_copy(ID3D12GraphicsCommandList* list,const D3D12_TEXTURE_COPY_LOCATION* dst,UINT x,UINT y,UINT z,const D3D12_TEXTURE_COPY_LOCATION* src,const D3D12_BOX* box){
    assert(list==pixel_list&&!x&&!y&&!z&&!box);
    if(dst->Type==D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT){assert(!src->SubresourceIndex);++extra_copies;if(pixel_mode==5)throw std::runtime_error("extra copy");}
    else{++original_copies;if(pixel_mode==4)throw std::runtime_error("original copy");}
}
void copy(unsigned mismatch=0){D3D12_TEXTURE_COPY_LOCATION src{},dst{};
    std::memcpy(&src.pResource,evaluation.data()+8,8);std::memcpy(&dst.pResource,evaluation.data(),8);
    src.Type=dst.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    if(mismatch==1)dst.pResource=reinterpret_cast<ID3D12Resource*>(uintptr_t(0x999));
    if(mismatch==2)src.SubresourceIndex=1;
    ngx_output_pixels::copy_passthrough(pixel_list,&dst,0,0,0,&src,nullptr,0x1c6093,pixel_original_copy);
}
uint64_t __fastcall pixel_original_evaluate(uintptr_t a,uintptr_t b,uintptr_t c,const void* input){
    unlocked();++pixel_eval_calls;assert(a==0xaaaa&&b==0xbbbb&&c==0xccccdddd11112222ull&&input==evaluation.data());
    if(pixel_mode==1||pixel_mode==3||pixel_mode==4||pixel_mode==5)copy();
    if(pixel_mode==3)throw std::runtime_error("Eval after copy");
    if(pixel_mode==8){pixel_mode=0;assert(invoke_evaluate()==evaluate_result);pixel_mode=8;}
    if(pixel_mode==9)put(evaluation,0x60,uint32_t(777));
    return evaluate_result;
}
void STDMETHODCALLTYPE pixel_original_execute(ID3D12CommandQueue* q,UINT n,ID3D12CommandList* const* members){++original_executes;assert(q==pixel_queue&&n==1&&members[0]==pixel_list);if(pixel_mode==6)throw std::runtime_error("Execute");}
HRESULT STDMETHODCALLTYPE pixel_original_reset(ID3D12GraphicsCommandList* list,ID3D12CommandAllocator*,ID3D12PipelineState*){++original_resets;assert(list==pixel_list);if(pixel_mode==7)throw std::runtime_error("Reset");return reset_result;}
void pixel_reset(){
    reset();consuming_frame_ticket=32;consuming_frozen_frame.id=32;ngx_route_observer::original_evaluate=pixel_original_evaluate;
    ngx_output_pixels::enabled=true;ngx_output_pixels::stopped=ngx_output_pixels::published=false;ngx_output_pixels::slots={};
    ngx_output_pixels::count=ngx_output_pixels::failure=0;ngx_output_pixels::world=ngx_output_pixels::ticket=ngx_output_pixels::eval_sequence=ngx_output_pixels::copy_sequence=0;
    ngx_output_pixels::active_eval=ngx_output_pixels::pending_eval=-1;ngx_output_pixels::fake_backend=ngx_output_pixels::FakeBackend{};
    ngx_output_pixels::gap={};
    pixel_mode=original_copies=extra_copies=original_executes=original_resets=pixel_eval_calls=0;reset_result=S_OK;
}
void execute(){ID3D12CommandList* member=pixel_list;ngx_output_pixels::execute_passthrough(pixel_queue,1,&member,pixel_original_execute);}
void retire(){assert(ngx_output_pixels::reset_passthrough(pixel_list,nullptr,nullptr,pixel_original_reset)==reset_result);}
void paired(bool inside,bool same_base=false){pixel_mode=inside?1:0;for(unsigned i=0;i<2;++i){put(evaluation,0x60,uint32_t(same_base?0:i*1280));assert(invoke_evaluate()==evaluate_result);if(!inside)copy();}}
void pixel_cases(){using namespace ngx_output_pixels;
    for(bool inside:{false,true}){pixel_reset();ngx_route_observer::recorder.evaluate_count=8;paired(inside);
        assert(count==2&&!failure&&original_copies==2&&extra_copies==2&&pixel_eval_calls==2&&published&&fake_backend.publications==1);
        assert(slots[0].before.values[7]==0&&slots[1].before.values[7]==1280&&slots[0].before.resources==slots[1].before.resources);
        execute();poll();assert(!fake_backend.mapped);retire();poll();assert(fake_backend.mapped==2&&fake_backend.released==2&&original_executes==1&&original_resets==1);
        pixel_mode=0;assert(invoke_evaluate()==evaluate_result&&count==2);copy();assert(original_copies==3&&extra_copies==2);}
    pixel_reset();paired(false,true);assert(count==2&&slots[0].before.values[7]==slots[1].before.values[7]&&extra_copies==2&&published);
    execute();retire();poll();assert(fake_backend.mapped==2);
    pixel_reset();paired(false);execute();fake_backend.complete=false;retire();poll();assert(!fake_backend.mapped&&!fake_backend.released);
    fake_backend.complete=true;poll();assert(fake_backend.mapped==2);
    pixel_reset();assert(invoke_evaluate()==evaluate_result);assert(invoke_evaluate()==evaluate_result);copy();assert(failure==association_ambiguous&&!extra_copies);
    assert(gap.boundary&&gap.copy&&slots[0].exit<gap.boundary_qpc&&gap.boundary_qpc<gap.copy_qpc);
    FILE* receipt{};assert(!tmpfile_s(&receipt)&&receipt);print(receipt);assert(contains(receipt,"boundary_qpc=")&&contains(receipt,"before_fields_mask=")&&contains(receipt,"qpc_frequency="));fclose(receipt);
    for(unsigned bad:{1u,2u}){pixel_reset();assert(invoke_evaluate()==evaluate_result);copy(bad);assert(failure==typed_mismatch&&!extra_copies);}
    for(unsigned bad:{0u,1u,2u,3u}){pixel_reset();if(!bad)admitted=false;else if(bad==1)consuming_frame_ticket=31;else if(bad==2)consuming_frozen_frame.world_generation=10;else ngx_route_observer::attested=false;
        assert(invoke_evaluate()==evaluate_result);copy();assert(!count&&!extra_copies);}
    for(unsigned bad:{0u,1u,2u,3u}){pixel_reset();if(!bad)fake_backend.typed=false;else if(bad==1)fake_backend.same_device=false;else if(bad==2)fake_backend.format=24;else fake_backend.width=1280;
        assert(invoke_evaluate()==evaluate_result);copy();assert(failure==prepare_failed&&!extra_copies&&fake_backend.released==1);}
    for(unsigned exception:{3u,4u,5u}){pixel_reset();pixel_mode=exception;try{invoke_evaluate();assert(false);}catch(std::runtime_error const&){}
        assert(original_copies==1&&!slots[0].normal&&!published);
        assert(ngx_output_pixels::active_eval==-1&&ngx_route_observer::current==nullptr&&active.load()==0);
        assert(fake_backend.released==(exception==4?1u:0u));}
    pixel_reset();paired(false);execute();execute();poll();assert(!fake_backend.mapped&&!fake_backend.released&&slots[0].life.failure==gears_death_hdr_pixels::duplicate_execution);
    pixel_reset();paired(false);execute();fake_backend.queue_id=0x901;execute();assert(slots[0].life.failure==gears_death_hdr_pixels::cross_queue&&!fake_backend.released);
    pixel_reset();paired(false);fake_backend.signal_ok=false;execute();retire();poll();assert(!fake_backend.mapped&&!fake_backend.released&&slots[0].life.failure==gears_death_hdr_pixels::signal_failed);
    pixel_reset();paired(false);execute();reset_result=E_FAIL;retire();poll();assert(!fake_backend.mapped&&!fake_backend.released&&slots[0].life.failure==gears_death_hdr_pixels::reset_failed);
    pixel_reset();paired(false);retire();assert(slots[0].life.failure==gears_death_hdr_pixels::reset_before_execute&&!fake_backend.released);
    pixel_reset();paired(false);fake_backend.generation=5;execute();poll();assert(original_executes==1&&!fake_backend.signals&&!fake_backend.mapped&&slots[0].life.failure==gears_death_hdr_pixels::unknown_membership);
    pixel_reset();paired(false);fake_backend.unknown_identity=true;execute();assert(slots[0].life.failure==gears_death_hdr_pixels::unknown_membership&&!fake_backend.released);
    pixel_reset();paired(false);execute();fake_backend.unknown_identity=true;retire();assert(slots[0].life.failure==gears_death_hdr_pixels::unknown_membership&&!fake_backend.released);
    pixel_reset();paired(false);execute();retire();fake_backend.removed=true;poll();assert(slots[0].life.failure==gears_death_hdr_pixels::device_removed&&!fake_backend.released);
    for(unsigned exception:{6u,7u}){pixel_reset();paired(false);if(exception==7)execute();pixel_mode=exception;
        try{if(exception==6)execute();else retire();assert(false);}catch(std::runtime_error const&){}
        poll();assert(!fake_backend.mapped&&!fake_backend.released&&slots[0].life.failure&&original_executes==1&&original_resets==(exception==7?1u:0u));}
    pixel_reset();paired(false);execute();fake_backend.generation=0;retire();poll();assert(fake_backend.mapped==2); // Physical Reset remains authoritative after Present metadata retirement.
    pixel_reset();assert(invoke_evaluate()==evaluate_result);copy();consuming_frame_ticket=33;consuming_frozen_frame.id=33;
    assert(invoke_evaluate()==evaluate_result&&failure==association_ambiguous&&!published&&extra_copies==1);
    pixel_reset();assert(invoke_evaluate()==evaluate_result);assert(invoke_create()==create_result);copy();assert(failure==association_ambiguous&&!extra_copies&&create_calls==1);
    // Foreign TLS must not hide a new Eval/Create boundary from the selected
    // thread's pending interval, even if the foreign call cannot be admitted.
    for(unsigned foreign:{0u,1u,2u}){pixel_reset();assert(invoke_evaluate()==evaluate_result);
        std::thread other([&]{if(foreign==1){consuming_renderer=renderer.data();consuming_frame_ticket=31;consuming_frozen_frame.id=31;}
            if(foreign==2)assert(invoke_create()==create_result);else assert(invoke_evaluate()==evaluate_result);});other.join();
        copy();assert(failure==association_ambiguous&&!extra_copies&&!fake_backend.prepared&&gap.boundary&&gap.copy&&gap.thread!=owner_thread);
        assert(pixel_eval_calls==(foreign==2?1u:2u)&&create_calls==(foreign==2?1u:0u));}
    pixel_reset();assert(invoke_evaluate()==evaluate_result);copy();assert(invoke_evaluate()==evaluate_result);
    std::thread foreign_after_append([]{assert(invoke_evaluate()==evaluate_result);});foreign_after_append.join();
    copy();poll();assert(failure==association_ambiguous&&extra_copies==1&&original_copies==2&&!fake_backend.released&&!fake_backend.mapped&&slots[0].life.appended);
    // Once both copies are already recorded, an unrelated foreign Eval does
    // not retroactively make their completed association ambiguous.
    pixel_reset();paired(false);std::thread after_copies([]{assert(invoke_evaluate()==evaluate_result);});after_copies.join();
    assert(!failure&&extra_copies==2&&pixel_eval_calls==3);
    pixel_reset();pixel_mode=8;assert(invoke_evaluate()==evaluate_result&&failure==nested_eval&&pixel_eval_calls==2&&!extra_copies&&active_eval==-1);
    pixel_reset();pixel_mode=9;assert(invoke_evaluate()==evaluate_result);copy();assert(failure==eval_changed&&!extra_copies&&!slots[0].normal);
    pixel_reset();paired(false);stop();poll();assert(!fake_backend.mapped&&!fake_backend.released);
    // Pure lifetime: completed GPU alone is insufficient; exact successful
    // physical Reset retires even when Present erased the Tracker generation.
    gears_death_hdr_pixels::Lifetime life;life.appended=true;life.list={1,2,3};life.held_identity=2;
    assert(life.before(true,1,4,true));life.after(true);life.completion(1);assert(!life.may_map());life.reset(true,2,0,1,0);assert(life.may_map());
}
}
#if defined(GEARS_NGX_OUTPUT_DELAYED_TEST_INCLUDED)
#define main ngx_output_legacy_main
#endif
int main(){ngx_observer_regression_main();pixel_cases();std::puts("NgxOutputPixelsTest passed (actual wrappers, fake GPU; no game)");return 0;}
#if defined(GEARS_NGX_OUTPUT_DELAYED_TEST_INCLUDED)
#undef main
#endif
