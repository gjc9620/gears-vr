#define GEARS_OWNED_SOURCE_PIXELS 1
#define main route_cpu_regression_main
#include "ngx_route_observer_test.cpp"
#undef main
#include "owned_source_pixels.h"
namespace {bool consuming_owned_scene=true,color_copy_diagnostics=true;bool range_match=true;unsigned range_calls{};}
namespace final_transfer_live {bool verify_range(FILE*,uintptr_t,unsigned,const char*){++range_calls;return range_match;}}
namespace owned_ticket_size_trace {
bool finished{};uint64_t selected_ticket=7,selected_world=9;
bool matches(void* r,Frozen const& f){return r==renderer.data()&&f.enabled&&f.id==selected_ticket&&f.world_generation==selected_world;}
}
#define __readgsqword test_readgsqword
#include "owned_source_pixels.inl"
#undef __readgsqword
namespace ngx_output_pixels {
struct Fake final:Backend {
    unsigned allocated{},appended{},released{},mapped{},signals{},width=5120,height=2800,format=26;
    bool prepared=true,complete=true,signal_ok=true,queue_ok=true;uint64_t generation=4;uintptr_t identity=0x700;
    bool start()override{assert(false);return false;} // No shared match mapping in this mode.
    bool prepare(unsigned,Slot& s)override{++allocated;s.source_desc={0x101,width,height,format,3,1,1,1,0};s.destination_desc={0x102,width,height,format,3,1,1,1,0};
        s.life.list={reinterpret_cast<uintptr_t>(s.raw_list),identity,generation};s.life.held_identity=identity;return prepared;}
    void append(unsigned,Slot& s,CopyFn f)override{++appended;D3D12_TEXTURE_COPY_LOCATION own{};own.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;f(s.raw_list,&own,0,0,0,&s.source,nullptr);}
    uintptr_t list_identity(ID3D12CommandList* l)override{return l?identity:0;}
    gears_frame_link::ListRef list_ref(uintptr_t id)override{return {id,generation};}
    bool queue_valid(unsigned,ID3D12CommandQueue* q,uintptr_t& id)override{id=0x900;return q&&queue_ok;}
    bool signal(unsigned,ID3D12CommandQueue*)override{++signals;return signal_ok;}
    bool poll(unsigned,Slot& s)override{s.life.completion(complete?1:0);if(!s.life.may_map())return false;++mapped;s.life.saved=true;return true;}
    void release(unsigned)override{++released;}
    bool publish(uint64_t,uint64_t,uint64_t)override{assert(false);return false;}void close()override{}
} fake;
Backend& backend(){return fake;}
}
namespace {
unsigned original_count{},extra_count{},exec_count{},reset_count{},action{};HRESULT reset_hr=S_OK;
auto* own_list=reinterpret_cast<ID3D12GraphicsCommandList*>(0x700);auto* own_queue=reinterpret_cast<ID3D12CommandQueue*>(0x900);
void STDMETHODCALLTYPE copied(ID3D12GraphicsCommandList* l,D3D12_TEXTURE_COPY_LOCATION const* d,UINT x,UINT y,UINT z,D3D12_TEXTURE_COPY_LOCATION const* s,D3D12_BOX const* b){
    assert(l==own_list);if(d->Type==D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT){++extra_count;assert(!x&&!y&&!z&&!b&&!s->SubresourceIndex);if(action==5)throw std::runtime_error("append");}
    else{++original_count;if(action==1)++committed_world_generation;if(action==2)++ngx_output_pixels::fake.generation;if(action==3)throw std::runtime_error("original");if(action==4)owned_source_pixels::stop();}
}
void STDMETHODCALLTYPE submitted(ID3D12CommandQueue*,UINT,ID3D12CommandList* const*){++exec_count;if(action==6)throw std::runtime_error("execute");}
HRESULT STDMETHODCALLTYPE reset_list(ID3D12GraphicsCommandList*,ID3D12CommandAllocator*,ID3D12PipelineState*){++reset_count;return reset_hr;}
void copy_source(uintptr_t caller=0x1c6093,unsigned variant=0){D3D12_TEXTURE_COPY_LOCATION a{},b{};a.Type=b.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    a.pResource=reinterpret_cast<ID3D12Resource*>(0x101);b.pResource=reinterpret_cast<ID3D12Resource*>(0x102);D3D12_BOX box{};
    if(variant==1)a.SubresourceIndex=1;if(variant==2)a.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    owned_source_pixels::copy_passthrough(own_list,&b,variant==3?1:0,0,0,variant==5?nullptr:&a,variant==4?&box:nullptr,caller,copied);}
void set_eye(unsigned eye){put(context,0x14c0,reinterpret_cast<uintptr_t>(views.data())+uintptr_t(eye)*0xd30);}
void fresh(){reset();put(renderer,0x74,int(2));consuming_owned_scene=true;owned_ticket_size_trace::finished=false;
    owned_ticket_size_trace::selected_ticket=7;owned_ticket_size_trace::selected_world=9;
    using namespace owned_source_pixels;initialized=false;enabled=true;stopped=false;count=eyes=rejected=failure=0;ticket=world=execute_order=0;owned_source_pixels::renderer=0;slots={};
    ngx_output_pixels::fake={};original_count=extra_count=exec_count=reset_count=action=0;reset_hr=S_OK;range_match=true;range_calls=0;set_eye(0);}
void pair(){copy_source();set_eye(1);copy_source();}
void execute(unsigned n=1){ID3D12CommandList* members[]{own_list,own_list};owned_source_pixels::execute_passthrough(own_queue,n,members,submitted);}
void retire(){owned_source_pixels::reset_passthrough(own_list,nullptr,nullptr,reset_list);}
void source_cases(){using namespace owned_source_pixels;
    fresh();pair();assert(count==2&&eyes==3&&original_count==2&&extra_count==2&&!failure);copy_source();assert(original_count==3&&extra_count==2);
    ngx_output_pixels::fake.complete=false;execute();poll();assert(!ngx_output_pixels::fake.mapped);retire();poll();assert(!ngx_output_pixels::fake.mapped&&!ngx_output_pixels::fake.released);
    ngx_output_pixels::fake.complete=true;poll();assert(ngx_output_pixels::fake.mapped==2&&ngx_output_pixels::fake.released==2);
    for(unsigned first:{0u,1u})for(unsigned second:{0u,1u}){
        fresh();set_eye(first);copy_source();set_eye(second);copy_source();
        assert(count==2&&extra_count==2&&original_count==2&&!rejected&&!failure);
        assert(slots[0].before.ownership.eye==first&&slots[1].before.ownership.eye==second);
        assert(slots[0].sequence==1&&slots[1].sequence==2&&eyes==((1u<<first)|(1u<<second)));
        copy_source();assert(count==2&&original_count==3&&extra_count==2&&ngx_output_pixels::fake.allocated==2);
    }
    for(unsigned bad=0;bad<8;++bad){fresh();if(bad==0)enabled=false;if(bad==1)consuming_owned_scene=false;if(bad==2)owned_ticket_size_trace::finished=true;
        if(bad==3)committed_world_generation=10;if(bad==4)consuming_frozen_frame.id=8;if(bad==5)put(::renderer,0x74,int(1));if(bad==6)set_eye(2);if(bad==7)consuming_frozen_frame.world_generation=1;
        copy_source();assert(original_count==1&&!count&&!extra_count&&!ngx_output_pixels::fake.allocated);}
    for(unsigned bad=1;bad<=5;++bad){fresh();copy_source(0x1c6093,bad);assert(original_count==1&&!extra_count&&!count);}
    fresh();copy_source(base+0x1c6093);copy_source(0x1c6094);assert(original_count==2&&!count);
    for(unsigned bad=0;bad<4;++bad){fresh();if(bad==0)ngx_output_pixels::fake.width=3840;if(bad==1)ngx_output_pixels::fake.format=28;if(bad==2)ngx_output_pixels::fake.prepared=false;if(bad==3)ngx_output_pixels::fake.height=2160;
        copy_source();assert(original_count==1&&!extra_count&&failure&&ngx_output_pixels::fake.released==1);}
    for(unsigned bad:{1u,2u,3u,4u,5u}){fresh();action=bad;bool threw=false;try{copy_source();}catch(...){threw=true;}
        assert(original_count==1&&failure);assert(threw==(bad==3||bad==5));if(bad==5)assert(extra_count==1&&!ngx_output_pixels::fake.released);else assert(!extra_count&&ngx_output_pixels::fake.released==1);}
    fresh();pair();execute(2);assert(slots[0].life.failure==gears_death_hdr_pixels::duplicate_execution);retire();poll();assert(!ngx_output_pixels::fake.released);
    fresh();pair();execute();execute();assert(slots[0].life.failure==gears_death_hdr_pixels::duplicate_execution);
    fresh();pair();retire();assert(slots[0].life.failure==gears_death_hdr_pixels::reset_before_execute);
    fresh();pair();execute();reset_hr=E_FAIL;retire();poll();assert(slots[0].life.failure==gears_death_hdr_pixels::reset_failed&&!ngx_output_pixels::fake.released);
    fresh();pair();execute();stop();retire();poll();assert(!ngx_output_pixels::fake.released);
    fresh();pair();execute(513);assert(slots[0].life.failure==gears_death_hdr_pixels::unknown_membership&&!ngx_output_pixels::fake.released);
    fresh();pair();ngx_output_pixels::fake.queue_ok=false;execute();assert(slots[0].life.failure==gears_death_hdr_pixels::cross_queue);
    fresh();pair();ngx_output_pixels::fake.signal_ok=false;execute();assert(slots[0].life.failure==gears_death_hdr_pixels::signal_failed);
    fresh();pair();++ngx_output_pixels::fake.generation;execute();assert(slots[0].life.failure==gears_death_hdr_pixels::unknown_membership);
    fresh();set_eye(1);copy_source();++consuming_frame_ticket;++consuming_frozen_frame.id;owned_ticket_size_trace::selected_ticket=8;copy_source();assert(count==1&&failure&&extra_count==1);
    fresh();FILE* file{};assert(!tmpfile_s(&file));assert(enable(file)&&range_calls==6);assert(!enable(file));fclose(file);
    fresh();range_match=false;assert(!tmpfile_s(&file));assert(!enable(file)&&range_calls==6);copy_source();assert(!count);fclose(file);
    fresh();pair();assert(!tmpfile_s(&file));print(file);assert(contains(file,"source_pixel_only=1 final_match_verified=0"));fclose(file);
    puts("OwnedSourcePixels CPU gates/forwarding/ownership/drift/retirement tests passed; no GPU or pixel evidence");
}
}
int main(){_set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);source_cases();}
