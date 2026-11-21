#define GEARS_TROIKA_C0_REUSE_FIXTURE 1
#include "troika_c0_hands_test.cpp"
#include "troika_c0_readback.h"
#include <functional>
namespace readback=gears_troika_c0_readback;namespace api=gears_troika_c0;
struct QualifiedFire final:f::Backend {
    std::function<bool()> check;unsigned starts{},stops{};bool quiet{};
    bool qualified(f::Run const&)noexcept override{return check();}
    f::Dispatch start(f::Run const&)noexcept override{++starts;return f::Dispatch::returned;}
    f::Dispatch stop(f::Run const&)noexcept override{++stops;quiet=true;return f::Dispatch::returned;}
    bool quiescent(f::Run const&)noexcept override{return quiet;}
    bool extinct(f::Run const&)noexcept override{return false;}
};
std::int64_t actual_clock(){LARGE_INTEGER q{},f{};QueryPerformanceCounter(&q);QueryPerformanceFrequency(&f);
    return q.QuadPart/f.QuadPart*1000000000+(q.QuadPart%f.QuadPart)*1000000000/f.QuadPart;}
int main(int argc,char** argv){
    if(argc!=2)return 2;const std::string dir=argv[1];
    auto frame=std::make_unique<b::Frame>();auto model=std::make_unique<b::Model>();auto work=std::make_unique<t::Workspace>();auto plan=std::make_unique<b::Output>();
    CHECK(load(dir+"/reference.bin",frame->reference.data(),sizeof frame->reference));CHECK(load(dir+"/C0-local.bin",frame->local.data(),sizeof frame->local));
    CHECK(load(dir+"/C0-component.bin",&frame->component_to_world,sizeof frame->component_to_world));CHECK(b::prepare_model(frame->reference,hash_geometry,*model));
    Fixture input;input.now=actual_clock()-10000000;input.next();input.fresh_edge();auto source=sample(input);
    LARGE_INTEGER frequency{},qpc{};QueryPerformanceFrequency(&frequency);QueryPerformanceCounter(&qpc);
    source.source.frame.qpc_frequency=frequency.QuadPart;source.source.frame.scene_qpc=qpc.QuadPart;
    b::Workspace original{};for(unsigned i=0;i<16;++i)original.component.m[i]=frame->component_to_world.values[i];
    std::copy(frame->local.begin(),frame->local.end(),original.pose.local);CHECK(b::d::rebuild(*model,original));
    for(unsigned side=0;side<2;++side){const unsigned wrist=side?35u:11u,shoulder=side?32u:8u;
        auto target=b::d::pose(original.pose.world[wrist]);const auto start=b::d::position(original.pose.world[shoulder]);
        target.position=b::d::add(start,b::d::mul(b::d::sub(target.position,start),.8));
        t::mount::Mount mount{};CHECK(t::mount::marcus115(model->signature,side?t::mount::Side::right:t::mount::Side::left,1,100,mount));
        source.mapping.hands.world[side]=invert_mount(target,mount);
    }
    std::array<std::uint64_t,2> available{UINT64_MAX,UINT64_MAX};
    CHECK(t::compose(*model,*frame,source,actual_clock(),available,*work,*plan).status==t::Status::okay);
    auto host=std::make_unique<NativeFixture>(*frame);tx::Kernel kernel;tx::Lease lease;
    source.source.frame.native.game_thread=GetCurrentThreadId();source.source.frame.driver_mesh.address=host->mesh.address();
    host->owner.world.address=source.source.frame.world.address;host->owner.pawn.address=source.source.frame.driver.address;
    host->owner.world_generation=source.source.frame.world_generation;host->owner.session=source.source.frame.native.native_started_qpc;
    host->seed.binding.world_generation=host->seed.binding.metadata_generation=host->owner.world_generation;host->seed.binding.session=host->owner.session;
    CHECK(host->capture());const auto key=host->backend.stamp();
    auto asset=source.source.frame.driver_mesh;asset.address=host->asset.address();
    auto store=std::make_unique<readback::Store>();auto consumed=std::make_unique<n::ConsumedPose>();
    const auto publish=[&](unsigned solved=3,unsigned projected=0){return store->publish(host->backend,kernel,key,source,asset,
        *model,host->captured,*plan,work->target,solved,projected,21,22,actual_clock,*consumed);};
    CHECK(!publish());CHECK(!store->active());
    CHECK(kernel.prepare(host->backend,key,host->captured,*plan,host->desired_copy,lease).code==tx::Code::prepared);
    CHECK(kernel.commit(host->backend,lease,key).code==tx::Code::committed);CHECK(!publish());
    CHECK(kernel.notify_consumer_entry(host->backend,lease,key).code==tx::Code::consumer_marked);host->original_space();
    CHECK(kernel.finish(host->backend,lease,key,tx::End::original_returned).code==tx::Code::scope_closed);
    CHECK(publish());CHECK(store->active());api::Request query{};query.source=source.source;query.control=source.control;
    CHECK(api::valid(store->frame(),query,actual_clock()));
    const auto first=store->frame();api::Frame out{};
    const auto metadata=[&](p::Metadata& result){p::Request q{};q.owner=host->owner;
        auto copy=[](void* to,void const* from,std::size_t bytes){return n::read_memory(to,reinterpret_cast<std::uintptr_t>(from),bytes);};
        return p::metadata(copy,q,result);};
    const auto read=[&](auto&& current){return store->read(query,actual_clock,*consumed,host->captured,current,metadata,n::read_memory,out);};
    CHECK(read([](){return true;}));CHECK(out.completed_ns==first.completed_ns&&out.observed_qpc==first.observed_qpc);
    CHECK(!read([](){return false;}));
    for(unsigned fault=0;fault<5;++fault){auto bad=query;
        if(fault==0)++bad.source.entry_token;if(fault==1)++bad.source.frame.source_ticket;if(fault==2)++bad.control.lease.id;
        if(fault==3)bad.maximum_pose_age_ns=api::maximum_age_ns+1;if(fault==4)bad.source.frame.control_policy=0;
        CHECK(!store->read(bad,actual_clock,*consumed,host->captured,[](){return true;},metadata,n::read_memory,out));
    }
    auto* actual_local=host->local.data;actual_local[12*sizeof(b::Atom)]^=1;CHECK(!read([](){return true;}));actual_local[12*sizeof(b::Atom)]^=1;
    auto* actual_space=host->space.data;actual_space[11*64+48]^=1;CHECK(!read([](){return true;}));actual_space[11*64+48]^=1;
    host->reference.data[68+12]^=1;CHECK(!read([](){return true;}));host->reference.data[68+12]^=1;
    CHECK(read([](){return true;}));
    host->mesh.put(0x338,std::uint32_t(89));CHECK(!read([](){return true;}));host->mesh.put(0x338,std::uint32_t(88));
    f::Owner firing;QualifiedFire fire_backend;fire_backend.check=[&](){return read([](){return true;});};
    CHECK(firing.offer(source.source,source.control,actual_clock(),fire_backend));CHECK(firing.owed()&&fire_backend.starts==1);
    actual_local[12*sizeof(b::Atom)]^=1;CHECK(!firing.offer(source.source,source.control,actual_clock(),fire_backend));
    CHECK(!firing.owed()&&fire_backend.stops==1);actual_local[12*sizeof(b::Atom)]^=1;
    CHECK(!firing.offer(source.source,source.control,actual_clock(),fire_backend));CHECK(fire_backend.starts==1); // Proof restoration never replays held R2.
    unsigned closes{};CHECK(!read([&](){return ++closes<2;}));
    for(unsigned fault=0;fault<9;++fault){auto bad=first;
        if(fault==0)bad.flags=3;if(fault==1)bad.projected_mask=1;if(fault==2)bad.solved_mask=1;if(fault==3)bad.reference_signature[0]^=1;
        if(fault==4)bad.wrist_world[1].position.z+=1;if(fault==5)bad.fingers[0][0]={0,0,0,0};
        if(fault==6)bad.helpers[3]={0,0,0,0};if(fault==7)bad.observed_qpc=0;if(fault==8)bad.completed_ns=actual_clock()+1000000000;
        CHECK(!api::valid(bad,query,actual_clock()));
    }
    CHECK(!store->read(query,[&](){return first.completed_ns+api::maximum_age_ns+1;},*consumed,host->captured,[](){return true;},metadata,n::read_memory,out));
    store->revoke();CHECK(!read([](){return true;}));CHECK(!publish(1));CHECK(!publish(3,1));CHECK(!publish()); // Same completed Tick/Compose cannot mint a new observation time.
    std::printf("Troika same-Kernel C0 readback store: %u checks passed; real reference and actual Native Backend/CAS/Kernel on owned pages, Host-emulated original Compose only.\n",checks);
}
