#include "cover_physx_query.h"
#define main PriorPhysXQueryFixtureMain
#include "cover_physx_query_test.cpp"
#undef main
#include <memory>
#include <thread>
namespace own=gears_cover_physx_shape_owner;
namespace asset=gears_cover_physx_asset_actors;
namespace cx=gears_cover_physx_collider;
namespace parts=gears_cover_physx_components;
static parts::Source parts_input{};
static cx::Source collider_input{0x220000,0x200000,0x210000,0x180,-1,0x160000,0};
constexpr std::uintptr_t shape0=0x190000,inside0=0x1a0000,body0=0x1b0000,
    shape1=0x1c0000,actor1=0x1d0000,inside1=0x1e0000,body1=0x1f0000;
enum class Mode {normal,unsupported,links,read,source,malformed,fault,code,asset_changed,collider_changed,catalog_changed,component_asset_changed,actor_edge_changed};
static Mode mode{};static Memory* live_memory{};static unsigned original_calls{},close_calls{},retain_calls{},hit_count=2,owner_reads{};
static bool probe_called{},pending_inside{},close_inside{};static q::Code nested_code{};
static void(*owner_probe)(){};
struct HostImage {
    bool live{};
    std::uintptr_t base()const{return live?::base:0;}
    std::uint32_t size()const{return ::size;}
    bool prime(){live=true;return true;}
    void close(){++close_calls;live=false;}
    void retain_for_process(){++retain_calls;}
};
struct HostCalls {
    template<class Shape>static bool overlap(std::uintptr_t,std::uintptr_t,Shape const&,bool&){return false;}
    template<class Shape>static bool sweep(std::uintptr_t,std::uintptr_t,Shape const&,q::Vector const&,q::Sweep&){return false;}
    template<class Shape>static bool sweep_stream(std::uintptr_t fn,std::uintptr_t scene,Shape const&,q::Vector const&,
        q::Hit* batch,q::detail::StreamReporter& reporter,std::uint32_t& count){
        ++original_calls;check(scene==0x180000,"exact current public scene");
        check(fn==base+(std::is_same_v<Shape,q::Box>?0x171770:0x1716f0),"original entry selected for shape");
        if(mode==Mode::fault)return false;
        if(mode==Mode::unsupported)live_memory->put<std::uintptr_t>(shape1,base+0x241358);
        if(mode==Mode::links)live_memory->put<std::uintptr_t>(inside1+0xa0,shape1+8);
        if(mode==Mode::source)live_memory->put<std::uintptr_t>(0x150088,actor1);
        if(mode==Mode::code)live_memory->put<unsigned char>(base+0xc4e7,0x90);
        if(mode==Mode::asset_changed)live_memory->put<std::uintptr_t>(0x140088,actor1);
        if(mode==Mode::collider_changed)live_memory->put<std::uintptr_t>(0x210088,actor1);
        if(mode==Mode::catalog_changed)live_memory->put<std::uintptr_t>(0x280000,0);
        if(mode==Mode::component_asset_changed)live_memory->put<std::uintptr_t>(0x2b0008,0);
        if(mode==Mode::actor_edge_changed)live_memory->put<std::uintptr_t>(0x2f0000,0);
        count=0;
        while(count<hit_count){const auto take=(std::min)(unsigned(q::stream_batch_hits),hit_count-count);
            for(unsigned k=0;k<take;++k){batch[k]={};const auto shape=(count+k)%2?shape1:shape0;
                memcpy(batch[k].bytes.data()+8,&shape,sizeof shape);batch[k].bytes.back()=static_cast<unsigned char>(count+k);}
            count+=take;
            if(!q::detail::stream_report(&reporter,take,batch))break;
        }
        if(mode==Mode::malformed)++count;
        return true;
    }
};
using Engine=q::QueryEngine<HostImage,HostCalls>;
static Engine* current_engine{};
static q::Box box{{1,2,3},{1,2,3},{1,0,0,0,1,0,0,0,1}};
static q::Vector delta{1,0,0};
static Memory setup(char const* file){
    auto m=fixture();if(!load_code(file,m))throw 3;
    m.block(0x200000,0x188);m.block(0x210000,0x98);m.block(0x220000,0x320);
    m.block(0x230000,88);m.block(0x240000,0x258);m.block(0x250000,0x98);m.block(0x260000,88);m.block(0x280000,32);m.block(0x290000,32);
    parts_input={};parts_input.seed_count=parts_input.root_count=1;parts_input.component_count=3;
    auto& root=parts_input.roots[0];root.actor=0x220000;root.offsets={0x300,0x310};
    root.base_offset=0x2e0;root.attached_offset=0x2f0;
    parts_input.root_count=2;root.attached={0x2f0000,1,1};root.attached_members[0]=0x300000;
    auto& child=parts_input.roots[1];child.actor=0x300000;child.offsets={0x300,0x310};child.base=0x220000;child.base_offset=0x2e0;child.attached_offset=0x2f0;
    child.attached={0x310000,1,1};child.attached_members[0]=0x220000;
    m.block(0x2f0000,8);m.block(0x300000,0x320);m.block(0x310000,8);m.put<std::uintptr_t>(0x2f0000,0x300000);m.put<std::uintptr_t>(0x310000,0x220000);
    m.put(0x2202f0,root.attached);m.put(0x3002e0,child.base);m.put(0x3002f0,child.attached);
    root.lists={s::pose::ArrayHeader{0x280000,4,4},s::pose::ArrayHeader{0x290000,2,4}};
    root.members[0]={0x200000,0,0x230000,0x200000};root.members[1]={0x230000,0x240000};
    for(unsigned n=0;n<2;++n){m.put(root.actor+root.offsets[n],root.lists[n]);for(unsigned k=0;k<4;++k)m.put(root.lists[n].data+k*8,root.members[n][k]);}
    parts_input.components[0]={0x200000,0x220000,0x210000,0x200000,0x220000,1,1,0x180,true,false,parts::State::body_data_nonnull,-1,0x160000,0};
    parts_input.components[1]={0x230000,0x260000,0,0,0,0,0,0,false,false,parts::State::nonprimitive};
    parts_input.components[2]={0x240000,0x260000,0x250000,0x240000,0x260000,0,0,0x180,true,false,parts::State::body_data_nonnull,-1,actor1,0};
    parts_input.components[2].skeletal=true;parts_input.asset_body_count=2;
    auto& extra=parts_input.assets[2];extra.sampled=true;extra.asset=0x110000;extra.instance=0x2a0000;extra.owner=0x260000;
    extra.offsets={0x60,0x6c,0x7c};extra.array={0x2b0000,2,2};extra.root=-1;
    parts_input.asset_bodies[0]={0x250000,0x240000,0x260000,0,-1,actor1,0};
    parts_input.asset_bodies[1]={0x2d0000,0x240000,0x260000,0,5,0x2c0000,0};
    m.block(0x2a0000,0x80);m.block(0x2b0000,16);m.block(0x2c0000,0x48);m.block(0x2d0000,0x98);m.block(0x2e0000,0x18);
    m.put<std::uintptr_t>(0x240248,extra.asset);m.put<std::uintptr_t>(0x240250,extra.instance);m.put<std::uintptr_t>(0x2a0060,extra.owner);
    m.put(0x2a006c,extra.array);m.put(0x2a007c,extra.root);m.put<std::uintptr_t>(0x2b0000,0x250000);m.put<std::uintptr_t>(0x2b0008,0x2d0000);
    m.put<std::uintptr_t>(0x2d0088,0x2c0000);m.put<std::uintptr_t>(0x2c0000,base+0x2330a8);m.put<std::uintptr_t>(0x2c0038,0x170000);
    m.put<std::uintptr_t>(0x2c0040,0x2e0000);m.put<std::uintptr_t>(0x2e0010,0x2c0000);
    m.put<std::uintptr_t>(0x240180,0x250000);m.put<std::uintptr_t>(0x250088,actor1);
    m.put<std::uintptr_t>(0x200180,0x210000);m.put<std::uintptr_t>(0x210088,0x160000);
    for(auto const at:{shape0,shape1})m.block(at,0x40);
    for(auto const at:{inside0,inside1})m.block(at,0xd8);
    for(auto const at:{body0,body1})m.block(at,0x18);
    m.block(0x160000,0x48);m.block(actor1,0x48);
    m.put<std::uintptr_t>(0x160040,body0);m.put<std::uintptr_t>(body0+0x10,0x160000);
    m.put<std::uintptr_t>(actor1,base+0x2330a8);m.put<std::uintptr_t>(actor1+0x38,0x170000);
    m.put<std::uintptr_t>(actor1+0x40,body1);m.put<std::uintptr_t>(body1+0x10,actor1);
    for(unsigned i=0;i<2;++i){const auto shape=i?shape1:shape0,inside=i?inside1:inside0,actor=i?actor1:0x160000,body=i?body1:body0;
        m.put<std::uintptr_t>(shape,base+own::shape_tables[i]);m.put<std::uintptr_t>(shape+0x30,inside);m.put<std::uintptr_t>(shape+0x38,actor);
        m.put<std::uintptr_t>(inside,base+(i?0x2272d0:0x226c80));m.put<std::uintptr_t>(inside+0xa0,shape);m.put<std::uintptr_t>(inside+0xd0,body);}
    return m;
}
struct Guard {
    bool operator()(){return true;}
    bool operator()(s::Snapshot const& raw){return raw.actor==0x160000&&raw.scene==0x170000&&raw.public_scene==0x180000;}
    bool operator()(asset::Snapshot const& raw){return (*this)(raw.query_source)&&raw.body_count==2;}
    bool operator()(cx::Snapshot const& raw){return (*this)(raw.query_source)&&cx::same(raw.fields,collider_input);}
    bool operator()(parts::Snapshot const& raw){return (*this)(raw.query_source)&&parts::same(raw.fields,parts_input)&&parts::consistent(raw);}
};
static void probe(){
    pending_inside=current_engine->pending();close_inside=current_engine->close();
    auto nested=std::make_unique<q::AllSweepOwners>();
    nested_code=current_engine->sweep_all(source(),box,delta,q::StreamLimits{},
        [](void* p,void const* a,std::size_t n){return live_memory->read(p,a,n);},Guard{},*nested,&collider_input,&parts_input);
}
static bool equal(q::AllSweepOwners const& a,q::AllSweepOwners const& b){
    if(memcmp(&a.raw,&b.raw,sizeof a.raw)||!asset::same(a.asset_bodies,b.asset_bodies)||!cx::same(a.collider_source,b.collider_source)||!parts::same(a.component_sources,b.component_sources))return false;
    for(unsigned i=0;i<a.owners.size();++i)if(!own::same(a.owners[i],b.owners[i]))return false;
    return true;
}
int main(int argc,char**argv){
    if(argc!=2)return 2;
    try{
        const auto initial=setup(argv[1]);auto memory=initial;live_memory=&memory;
        auto output=std::make_unique<q::AllSweepOwners>();output->raw.raw_count=99;output->owners[0].actor=0xdead;
        const auto saved=std::make_unique<q::AllSweepOwners>(*output);
        auto read=[&](void* p,void const* a,std::size_t n){const auto at=reinterpret_cast<std::uintptr_t>(a);
            if(at==shape0+0x38||at==shape1+0x38){++owner_reads;if(owner_probe&&!probe_called){probe_called=true;owner_probe();}}
            if(mode==Mode::read&&at==shape1+0x38)return false;return memory.read(p,a,n);};
        Guard guard;
        {auto engine=std::make_unique<Engine>();current_engine=engine.get();memory.calls=0;
            check(engine->sweep_all(source(),box,delta,q::StreamLimits{},read,guard,*output,&collider_input,&parts_input)==q::Code::unavailable&&equal(*output,*saved)&&!memory.calls,"unprimed actual engine does not read");}
        original_calls=close_calls=retain_calls=owner_reads=0;
        {auto engine=std::make_unique<Engine>();current_engine=engine.get();check(engine->prime(),"Host image prime only");
            owner_probe=probe;probe_called=false;mode=Mode::normal;
            check(engine->sweep_all(source(),box,delta,q::StreamLimits{},read,guard,*output,&collider_input,&parts_input)==q::Code::queried,"same production transaction owns every hit");
            check(probe_called&&pending_inside&&!close_inside&&nested_code==q::Code::reentrant&&!close_calls,"pin/in-flight spans owner reads and rejects reentry");
            check(original_calls==1&&output->raw.raw_count==2&&output->raw.hit_count==2&&output->raw.hits[1].bytes.back()==1,"one query retains raw count/order");
            check(output->owners[0].actor==0x160000&&output->owners[1].actor==actor1&&output->owners[1].kind==own::Kind::capsule,"self and foreign actor both retained, no filter");
            check(output->asset_bodies.body_count==2&&output->asset_bodies.actor_count==1&&
                output->asset_bodies.bodies[0].presence==asset::Presence::body_data_field_null&&
                output->asset_bodies.actors[0]==0x160000&&!output->asset_bodies.actors[1],"declared Bodies preserves null row and root actor, foreign hit not added");
            check(output->collider_source.presence==cx::Presence::actor_observed&&output->collider_source.fields.body_data==0x160000&&output->collider_source.scene==0x170000,"selected collider shared actor observed on same query Scene");
            check(output->component_sources.fields.component_count==3&&output->component_sources.actor_count==3&&output->component_sources.actors[1]==actor1&&output->component_sources.fields.components[2].owner_roots==0,"all declared actor sources retained including foreign owner, no self filter");
            check(output->component_sources.fields.asset_body_count==2&&output->component_sources.actors[2]==0x2c0000&&
                output->component_sources.asset_observations[1].presence==parts::Presence::actor_observed,"secondary PAA actor outside the raw hits joins the same transaction");
            check(output->component_sources.fields.root_count==2&&output->component_sources.fields.seed_count==1&&
                output->component_sources.fields.roots[1].base==0x220000,"recursive actor edge and actual Base join the same owned transaction");
            check(output->owners[2].kind==own::Kind::unknown&&!output->owners[2].actor,"unused owner tail cleared");
            owner_probe=nullptr;hit_count=0;
            check(engine->sweep_all(source(),box,delta,q::StreamLimits{},read,guard,*output,&collider_input,&parts_input)==q::Code::queried&&!output->raw.hit_count&&!output->owners[0].actor,"empty data clears old owners, not occupancy permission");
            hit_count=q::max_stream_hits;
            check(engine->sweep_all(source(),box,delta,q::StreamLimits{},read,guard,*output,&collider_input,&parts_input)==q::Code::queried&&output->raw.hit_count==q::max_stream_hits&&output->owners.back().actor==actor1,"all1024 owners including final record");
            hit_count=2;q::Capsule cap{{0,0,0},{0,0,1},.5f};
            check(engine->sweep_all(source(),cap,delta,q::StreamLimits{},read,guard,*output,&collider_input,&parts_input)==q::Code::queried,"capsule path uses same ownership transaction");
            check(!engine->pending()&&engine->close(),"normal ownership query releases active scope");}
        const std::array<std::pair<Mode,q::Code>,12> failures{{{Mode::unsupported,q::Code::owner_unsupported},{Mode::links,q::Code::owner_links},
            {Mode::read,q::Code::owner_read},{Mode::source,q::Code::changed},{Mode::malformed,q::Code::callback_rejected},
            {Mode::fault,q::Code::fault_unknown},{Mode::code,q::Code::code},{Mode::asset_changed,q::Code::changed},{Mode::collider_changed,q::Code::changed},{Mode::catalog_changed,q::Code::changed},{Mode::component_asset_changed,q::Code::changed},{Mode::actor_edge_changed,q::Code::changed}}};
        for(auto const& test:failures){memory=initial;mode=test.first;*output=*saved;original_calls=owner_reads=retain_calls=0;
            auto engine=std::make_unique<Engine>();check(engine->prime(),"failure fixture prime");
            check(engine->sweep_all(source(),box,delta,q::StreamLimits{},read,guard,*output,&collider_input,&parts_input)==test.second&&equal(*output,*saved),"whole attributed output unchanged on failure");
            check(original_calls==1,"failure never retries original query");
            if(mode==Mode::source||mode==Mode::malformed)check(!owner_reads,"source/count failure precedes pointer follow");
            if(mode==Mode::fault)check(engine->uncertain()&&engine->pending()&&!engine->close()&&retain_calls==1,"original unknown fault preserves pin/debt");
            else check(!engine->uncertain()&&!engine->pending()&&engine->close(),"read-only rejection does not fabricate original lock debt");}
        memory=initial;mode=Mode::normal;*output=*saved;original_calls=owner_reads=0;
        {auto engine=std::make_unique<Engine>();check(engine->prime(),"truncation fixture");
            check(engine->sweep_all(source(),box,delta,q::StreamLimits{},read,[]{return true;},*output,&collider_input,&parts_input)==q::Code::source&&
                equal(*output,*saved)&&!original_calls,"missing typed inventory guard rejects before original");
            check(engine->sweep_all(source(),box,delta,q::StreamLimits{},read,guard,*output)==q::Code::source&&equal(*output,*saved)&&!original_calls,"missing typed collider source rejects before original");
            struct RejectCollider:Guard {using Guard::operator();bool operator()(cx::Snapshot const&){return false;}} reject_collider;
            check(engine->sweep_all(source(),box,delta,q::StreamLimits{},read,reject_collider,*output,&collider_input,&parts_input)==q::Code::source&&equal(*output,*saved)&&!original_calls,"typed collider rejection precedes original");
            check(engine->sweep_all(source(),box,delta,q::StreamLimits{},read,guard,*output,&collider_input)==q::Code::source&&equal(*output,*saved)&&!original_calls,"missing component catalog rejects before original");
            struct RejectParts:Guard {using Guard::operator();bool operator()(parts::Snapshot const&){return false;}} reject_parts;
            check(engine->sweep_all(source(),box,delta,q::StreamLimits{},read,reject_parts,*output,&collider_input,&parts_input)==q::Code::source&&equal(*output,*saved)&&!original_calls,"component typed guard rejects before original");
            struct Reject:Guard {using Guard::operator();bool operator()(asset::Snapshot const&){return false;}} reject;
            check(engine->sweep_all(source(),box,delta,q::StreamLimits{},read,reject,*output,&collider_input,&parts_input)==q::Code::source&&
                equal(*output,*saved)&&!original_calls,"typed inventory rejection precedes original");
            memory.put<std::uintptr_t>(0x140088,actor1);memory.put<std::uintptr_t>(actor1,base+0x2330b0);
            check(engine->sweep_all(source(),box,delta,q::StreamLimits{},read,guard,*output,&collider_input,&parts_input)==q::Code::owner_unsupported&&
                equal(*output,*saved)&&!original_calls,"unsupported non-root BodyData rejects before original");
            memory=initial;
            check(engine->sweep_all(source(),box,delta,q::StreamLimits{1,2},read,guard,*output,&collider_input,&parts_input)==q::Code::truncated&&equal(*output,*saved)&&!owner_reads,"truncated stream never obtains ownership data");
            const auto calls=original_calls;check(engine->sweep_all(source(),box,delta,q::StreamLimits{0,1},read,guard,*output,&collider_input,&parts_input)==q::Code::request&&original_calls==calls,"bad budget never calls original");
            q::Code wrong{};const auto request=source();std::thread t([&](){wrong=engine->sweep_all(request,box,delta,q::StreamLimits{},read,guard,*output,&collider_input,&parts_input);});t.join();
            check(wrong==q::Code::source&&original_calls==calls&&equal(*output,*saved),"wrong owner thread cannot read/call");
            q::AllSweep raw{};mode=Mode::unsupported;owner_reads=0;
            check(engine->sweep_all(source(),box,delta,q::StreamLimits{},read,guard,raw)==q::Code::queried&&raw.hit_count==2&&!owner_reads,"existing raw mode stays raw and never silently filters");check(engine->close(),"raw fixture close");}
        check(!GetModuleHandleW(L"PhysXCore.dll"),"original DLL stayed unloaded");
    }catch(...){check(false,"owned sweep fixture exception");}
    printf("CoverPhysXOwnedSweep checks=%u failed=%u Host image/call seams; actual transaction/source/ownership; no game\n",checks,failed);return failed?1:0;
}
