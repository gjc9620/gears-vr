#define GEARS_QUERY_SESSION_TEST_MAIN ExistingQuerySessionMain
#include "cover_physics_query_live_test.cpp"
#undef GEARS_QUERY_SESSION_TEST_MAIN
namespace cp=gears_cover_capsule_path;
static unsigned at_call{};
static void(*path_action)(){};
static q::Vector* borrowed_delta{};
static bool empty_path{};
static unsigned path_seen_count{};
static std::array<q::Capsule,64> path_seen_shapes{};
static std::array<q::Vector,64> path_seen_steps{};
static std::array<q::StreamLimits,64> path_seen_limits{};
static q::Capsule path_seen_endpoint{};
static q::StreamLimits endpoint_seen_limits{};
static unsigned endpoint_corruption{};
static void path_callback(){if(query_calls==at_call&&path_action)path_action();}
static void mutate_delta(){borrowed_delta->x+=1;}
static void make_fault(){fault_query=true;}
struct PathEngine:HostEngine {
    q::AllSweepOwners endpoint_scratch{};
    template<class Read,class Guard>q::Code overlap_all(q::source::Source const& source,q::Capsule const& shape,
        q::StreamLimits const& limits,Read read,Guard current,q::AllOverlapOwners& out,
        gears_cover_physx_collider::Source const* collider,gears_cover_physx_components::Source const* components){
        path_seen_endpoint=shape;endpoint_seen_limits=limits;
        const auto code=HostEngine::sweep_all(source,shape,q::Vector{1,0,0},limits,read,current,endpoint_scratch,collider,components);
        if(code!=q::Code::queried)return code;
        std::memset(&out,0,sizeof out);
        out.raw.shape_count=out.raw.raw_count=2;out.raw.reports=1;
        out.raw.shapes[0]=source.mesh;out.raw.shapes[1]=source.instance;
        out.owners=endpoint_scratch.owners;out.self_sources[0]=1;out.self_sources[1]=0;
        out.asset_bodies=endpoint_scratch.asset_bodies;out.collider_source=endpoint_scratch.collider_source;
        out.component_sources=endpoint_scratch.component_sources;
        if(empty_path)out.raw={};
        if(endpoint_corruption==1)++out.raw.raw_count;
        if(endpoint_corruption==2)out.raw.shape_count=out.raw.raw_count=limits.max_hits+1;
        if(endpoint_corruption==3)out.raw.reports=limits.max_reports+1;
        if(endpoint_corruption==4)out.asset_bodies.bodies[0].actor+=8;
        if(endpoint_corruption==5)out.collider_source.fields.pawn+=8;
        if(endpoint_corruption==6)++out.component_sources.fields.component_count;
        if(endpoint_corruption==7)return q::Code::truncated;
        return code;
    }
    template<class Shape,class Read,class Guard>q::Code sweep_all(q::source::Source const& source,Shape const& shape,q::Vector const& step,
        q::StreamLimits const& limits,Read read,Guard current,q::AllSweepOwners& out,
        gears_cover_physx_collider::Source const* collider,gears_cover_physx_components::Source const* components){
        C(q::detail::sweep_motion(shape,step));
        if(path_seen_count<64){path_seen_shapes[path_seen_count]=shape;path_seen_steps[path_seen_count]=step;path_seen_limits[path_seen_count]=limits;++path_seen_count;}
        const auto code=HostEngine::sweep_all(source,shape,step,limits,read,current,out,collider,components);
        if(code==q::Code::queried){out.self_sources[0]=1;out.self_sources[1]=0;}
        if(code==q::Code::queried&&empty_path){out.raw={};out.owners={};}
        if(code==q::Code::queried&&(out.raw.hit_count>limits.max_hits||out.raw.reports>limits.max_reports))return q::Code::truncated;
        return code;
    }
};
using PathSession=service::Session<PathEngine>;
static PathSession* path_owner{};
static void stop_path(){path_owner->request_drain();}
static void reenter_path(){q::Overlap out{};nested_code=path_owner->overlap(actor,physics_expected,test_box,source_read,guard,out);}
static bool equal_path(cp::RawPath const& a,cp::RawPath const& b){return !std::memcmp(&a,&b,sizeof a);}
int main(){
    arena=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,saved_arena.size(),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!arena)return 99;
    auto output=std::make_unique<cp::RawPath>();output->plan.count=42;output->collected.raw.raw_count=999;
    const auto saved=std::make_unique<cp::RawPath>(*output);
    q::Capsule capsule{{1,2,3},{1,2,5},.5f};q::Vector delta_path{300,0,0};cp::Policy policy{64,.1};
    auto setup_path=[&](){reset();catalog_setup();during_query=path_callback;at_call=0;path_action=nullptr;empty_path=false;endpoint_corruption=0;path_seen_count=0;*output=*saved;delta_path={300,0,0};};
    setup_path();
    {auto owner=std::make_unique<PathSession>();path_owner=owner.get();C(owner->start(true,true));
        C(owner->sweep_capsule_path(actor,physics_expected,capsule,delta_path,policy,q::StreamLimits{},source_read,guard,*output)==q::Code::queried);
        C(query_calls==5&&output->plan.count==4&&output->endpoint.raw.shape_count==2);
        C(output->endpoint.raw.shapes[0]==fixture_mesh&&output->endpoint.raw.shapes[1]==physics_instance);
        C(output->endpoint.owners[0].actor==fixture_mesh&&output->endpoint.owners[1].actor==physics_instance);
        C(output->endpoint.self_sources[0]==1&&output->endpoint.self_sources[1]==0);
        C(output->collected.raw.hit_count==8&&output->collected.raw.raw_count==8&&output->collected.raw.reports==4);
        for(unsigned i=0;i<4;++i){C(output->spans[i].first==i*2&&output->spans[i].count==2&&output->spans[i].reports==1);
            C(output->collected.owners[i*2].actor==fixture_mesh&&output->collected.owners[i*2+1].actor==physics_instance);
            C(output->collected.self_sources[i*2]==1&&output->collected.self_sources[i*2+1]==0);
            C(path_seen_shapes[i].first.x==1+75.f*i&&path_seen_shapes[i].second.x==1+75.f*i&&path_seen_shapes[i].radius==.5f);
            C(path_seen_steps[i].x==75&&path_seen_steps[i].y==0&&path_seen_steps[i].z==0);}
        C(path_seen_endpoint.first.x==301&&path_seen_endpoint.second.x==301&&path_seen_endpoint.radius==.5f);
        C(output->collected.owners[8].actor==0&&output->spans[4].count==0&&query_lock_free);
        owner->request_drain();C(owner->poll_stop());}
    setup_path();empty_path=true;
    {auto owner=std::make_unique<PathSession>();C(owner->start(true,true));
        C(owner->sweep_capsule_path(actor,physics_expected,capsule,delta_path,policy,q::StreamLimits{},source_read,guard,*output)==q::Code::queried);
        C(query_calls==5&&!output->collected.raw.hit_count&&!output->collected.raw.reports&&!output->endpoint.raw.shape_count&&output->collected.owners[0].actor==0);
        for(unsigned i=0;i<4;++i)C(output->spans[i].count==0);
        owner->request_drain();C(owner->poll_stop());}
    setup_path();
    {auto owner=std::make_unique<PathSession>();C(owner->start(true,true));
        C(owner->sweep_capsule_path(actor,physics_expected,capsule,delta_path,policy,q::StreamLimits{10,5},source_read,guard,*output)==q::Code::queried);
        C(query_calls==5&&output->collected.raw.hit_count==8&&output->collected.raw.reports==4);owner->request_drain();C(owner->poll_stop());}
    for(unsigned i=0;i<4;++i)C(path_seen_limits[i].max_hits==10-i*2&&path_seen_limits[i].max_reports==5-i);
    C(endpoint_seen_limits.max_hits==2&&endpoint_seen_limits.max_reports==1);
    for(auto const budget:{q::StreamLimits{8,5},q::StreamLimits{10,4}}){
        setup_path();auto owner=std::make_unique<PathSession>();C(owner->start(true,true));
        C(owner->sweep_capsule_path(actor,physics_expected,capsule,delta_path,policy,budget,source_read,guard,*output)==q::Code::truncated);
        C(query_calls==4&&equal_path(*output,*saved));owner->request_drain();C(owner->poll_stop());
    }
    setup_path();
    {auto owner=std::make_unique<PathSession>();C(owner->start(true,true));
        C(owner->sweep_capsule_path(actor,physics_expected,capsule,delta_path,policy,q::StreamLimits{2,20},source_read,guard,*output)==q::Code::truncated);
        C(query_calls==1&&equal_path(*output,*saved));owner->request_drain();C(owner->poll_stop());}
    for(auto const action:{replace_root,replace_component_owner,make_fault,stop_path,mutate_delta,reenter_path}){
        setup_path();auto owner=std::make_unique<PathSession>();path_owner=owner.get();borrowed_delta=&delta_path;
        at_call=2;path_action=action;C(owner->start(true,true));
        const auto code=owner->sweep_capsule_path(actor,physics_expected,capsule,delta_path,policy,q::StreamLimits{},source_read,guard,*output);
        if(action==reenter_path){C(code==q::Code::queried&&nested_code==q::Code::reentrant&&query_calls==5);}
        else {C(code!=q::Code::queried&&equal_path(*output,*saved)&&query_calls==2);}
        owner->request_drain();
        if(action==make_fault)C(code==q::Code::fault_unknown&&owner->uncertain()&&!owner->poll_stop()&&owner->pending_count());
        else C(owner->poll_stop());
    }
    for(auto const action:{replace_root,replace_component_owner,make_fault,stop_path,mutate_delta,reenter_path}){
        setup_path();auto owner=std::make_unique<PathSession>();path_owner=owner.get();borrowed_delta=&delta_path;
        at_call=5;path_action=action;C(owner->start(true,true));
        const auto code=owner->sweep_capsule_path(actor,physics_expected,capsule,delta_path,policy,q::StreamLimits{},source_read,guard,*output);
        if(action==reenter_path)C(code==q::Code::queried&&nested_code==q::Code::reentrant);
        else C(code!=q::Code::queried&&equal_path(*output,*saved));
        C(query_calls==5);owner->request_drain();
        if(action==make_fault)C(code==q::Code::fault_unknown&&owner->uncertain()&&!owner->poll_stop());
        else C(owner->poll_stop());
    }
    for(unsigned corrupt=1;corrupt<=7;++corrupt){
        setup_path();endpoint_corruption=corrupt;auto owner=std::make_unique<PathSession>();C(owner->start(true,true));
        const auto code=owner->sweep_capsule_path(actor,physics_expected,capsule,delta_path,policy,q::StreamLimits{},source_read,guard,*output);
        C(code==(corrupt<=3?q::Code::callback_rejected:corrupt<=6?q::Code::changed:q::Code::truncated));
        C(query_calls==5&&equal_path(*output,*saved));owner->request_drain();C(owner->poll_stop());
    }
    setup_path();
    {auto owner=std::make_unique<PathSession>();C(owner->start(true,true));
        C(owner->sweep_capsule_path(actor,physics_expected,capsule,delta_path,policy,q::StreamLimits{7,20},source_read,guard,*output)==q::Code::truncated);
        C(query_calls==4&&equal_path(*output,*saved));owner->request_drain();C(owner->poll_stop());}
    setup_path();
    {auto owner=std::make_unique<PathSession>();C(owner->start(true,true));delta_path={10000,0,0};
        C(owner->sweep_capsule_path(actor,physics_expected,capsule,delta_path,policy,q::StreamLimits{},source_read,guard,*output)==q::Code::truncated);
        C(!query_calls&&equal_path(*output,*saved));owner->request_drain();C(owner->poll_stop());}
    C(VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE)!=0);
    std::printf("CoverCapsulePathSession checks=%u failed=%u typed-source/Session real; module/dispatch Host; no game\n",checks,failures);
    return failures?1:0;
}
