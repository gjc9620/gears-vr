#define GEARS_QUERY_SESSION_TEST_MAIN PriorQuerySessionMain
#include "cover_physics_query_live_test.cpp"
#undef GEARS_QUERY_SESSION_TEST_MAIN
struct OverlapHostEngine:HostEngine {
    q::AllSweepOwners scratch{};
    template<class Read,class Guard>q::Code overlap_all(q::source::Source const& source,q::Capsule const& geometry,q::StreamLimits const& limits,
        Read read,Guard current,q::AllOverlapOwners& out,gears_cover_physx_collider::Source const* collider,gears_cover_physx_components::Source const* components){
        const auto code=HostEngine::sweep_all(source,geometry,q::Vector{1,0,0},limits,read,current,scratch,collider,components);
        if(code==q::Code::queried){out={};out.raw.raw_count=out.raw.shape_count=2;out.raw.reports=1;
            out.raw.shapes[0]=source.mesh;out.raw.shapes[1]=source.instance;out.owners=scratch.owners;
            out.asset_bodies=scratch.asset_bodies;out.collider_source=scratch.collider_source;out.component_sources=scratch.component_sources;}
        return code;
    }
};
using OverlapSession=service::Session<OverlapHostEngine>;
static OverlapSession* overlap_session{};static q::StreamLimits* borrowed_limits{};
static void overlap_stop(){overlap_session->request_drain();}
static void overlap_budget_change(){borrowed_limits->max_reports+=1;}
int main(){
    arena=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,saved_arena.size(),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!arena)return 99;
    auto output=std::make_unique<q::AllOverlapOwners>();output->raw.raw_count=99;output->owners[0].actor=0xdead;
    const auto saved=std::make_unique<q::AllOverlapOwners>(*output);
    q::Capsule capsule{{1,2,3},{1,2,5},.5f};q::StreamLimits limits{};
    reset();catalog_setup();
    {auto owner=std::make_unique<OverlapSession>();C(owner->start(true,true));
        C(owner->overlap_all(actor,physics_expected,capsule,limits,source_read,guard,*output)==q::Code::queried);
        C(query_calls==1&&output->raw.shape_count==2&&output->raw.shapes[0]==fixture_mesh);
        C(output->component_sources.fields.component_count>0&&output->collider_source.fields.pawn==fixture_pawn);
        C(owner->observation().stage==service::Stage::complete&&query_lock_free);
        owner->request_drain();C(owner->poll_stop());}
    reset();catalog_setup();*output=*saved;
    {auto owner=std::make_unique<OverlapSession>();overlap_session=owner.get();C(owner->start(true,true));during_query=overlap_stop;
        C(owner->overlap_all(actor,physics_expected,capsule,limits,source_read,guard,*output)==q::Code::changed);
        C(!memcmp(output.get(),saved.get(),sizeof *output));C(owner->poll_stop());}
    reset();catalog_setup();*output=*saved;
    {auto owner=std::make_unique<OverlapSession>();C(owner->start(true,true));borrowed_limits=&limits;during_query=overlap_budget_change;
        C(owner->overlap_all(actor,physics_expected,capsule,limits,source_read,guard,*output)==q::Code::changed);
        C(!memcmp(output.get(),saved.get(),sizeof *output));owner->request_drain();C(owner->poll_stop());}
    limits={};reset();catalog_setup();*output=*saved;
    {auto owner=std::make_unique<OverlapSession>();C(owner->start(true,true));fault_query=true;
        C(owner->overlap_all(actor,physics_expected,capsule,limits,source_read,guard,*output)==q::Code::fault_unknown);
        C(!owner->enabled()&&owner->pending_count()!=0&&!owner->poll_stop());
        C(!memcmp(output.get(),saved.get(),sizeof *output));}
    reset();catalog_setup();*output=*saved;
    {auto owner=std::make_unique<OverlapSession>();C(owner->start(true,true));
        C(owner->overlap_all(actor,physics_expected,capsule,q::StreamLimits{0,1},source_read,guard,*output)==q::Code::request&&query_calls==0);
        C(!memcmp(output.get(),saved.get(),sizeof *output));owner->request_drain();C(owner->poll_stop());}
    C(VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE)!=0);
    printf("CoverOverlapSession checks=%u failed=%u actual typed roles/Session; Host dispatch/owner packets; no game\n",checks,failures);return failures?1:0;
}
