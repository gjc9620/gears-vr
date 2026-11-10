#define main ExistingFixtureMain
#include "body_goal_view_adapter_test.cpp"
#undef main
#include <utility>
#include <cstdlib>
#include <limits>
namespace cpu_fixture {
#include "../../../tools/native-capture/cover_measurement_core_fixture.h"
}
int main(int argc,char** argv){
    if(argc!=2)return 2;
    auto frame=std::make_unique<b::Frame>();auto model=std::make_unique<b::Model>();
    auto work=std::make_unique<b::Workspace>();auto marker_work=std::make_unique<mount::Workspace>();
    auto source=cpu_fixture::core(cpu_fixture::frame(1));mount::Marker marker{};
    C(read(std::string(argv[1])+"/reference.bin",frame->reference.data(),sizeof frame->reference));
    C(read(std::string(argv[1])+"/C0-local.bin",frame->local.data(),sizeof frame->local));
    C(read(std::string(argv[1])+"/C0-component.bin",&frame->component_to_world,sizeof frame->component_to_world));
    if(failures)return 1;
    frame->units_per_metre=source.units_per_metre;C(b::prepare_model(frame->reference,digest,*model));
    C(mount::prepare(*model,*frame,mount::Basis::marcus_mesh_face_forward_y,*marker_work,marker)==mount::Status::okay);
    source.flags|=a::core::rig::cover_key_known;source.cover_type=2;source.cover_slot=0;source.cover_link=source.world;
    namespace src=gears_body_tick_source;
    const auto now=std::int64_t(source.published_ns)+1000000000;
    src::Input input{};input.scene_cache_revision=2;input.metadata_begin_ns=now-10000000;input.metadata_end_ns=now-8000000;
    auto& t=input.observed;t.parent_scene=source;t.tick=1;t.thread=source.native.game_thread;t.parent_cookie=123;
    t.begin_ns=now-7000000;t.end_ns=now-3000000;
    t.stereo=source.stereo;t.stereo.sequence+=5;t.stereo.received_ns=now-4000000;t.stereo.predicted_time+=1000000000;
    t.controllers=source.controllers;t.controllers.sequence=t.stereo.sequence;t.controllers.predicted_time=t.stereo.predicted_time;
    t.controllers.received_ns=now-5000000;t.controllers.sync_ns=now-6000000;
    t.continuity.input=t.controllers;t.continuity.stream_epoch=source.producer.stream;
    for(unsigned i=0;i<2;++i)t.continuity.tracking_epoch[i]=source.producer.tracking[i];
    t.reference_revision=source.native_reference_revision;t.native_reference=source.native_reference;
    t.publisher_pid=source.controller_publisher_pid;t.publisher_start=source.controller_publisher_start;
    const auto classified=src::classify(input,source,now,true);
    if(classified!=src::Verdict::okay)std::printf("fixture verdict=%u hard_detail=0x%x published=%llu scene=%lld/%lld flags=%u\n",unsigned(classified),
        src::rejection_detail(input,source,now,true),(unsigned long long)source.published_ns,(long long)source.scene_qpc,(long long)source.qpc_frequency,source.flags);
    C(classified==src::Verdict::okay);
    a::Policy policy{};policy.now_ns=now;policy.allow_sim=true;a::CoverPoseMeasurement output{};
    const auto old_output=bytes(output);const auto saved=bytes(input);const auto raw=bytes(*frame);
    C(a::measure_cover_pose(source,*model,*frame,marker,policy,*work,output)==a::Status::core);
    C(bytes(output)==old_output);
    C(a::measure_cover_pose(input,*model,*frame,marker,policy,*work,output)==a::Status::okay);
    C(bytes(input)==saved&&bytes(*frame)==raw);
    C(output.binding.observed_ns==input.metadata_end_ns&&output.binding.publication==source.publication_sequence&&output.binding.scene==source.scene_invocation);
    auto different_head=input;different_head.observed.stereo.head.position.x+=.1;a::CoverPoseMeasurement unchanged{};
    C(a::measure_cover_pose(different_head,*model,*frame,marker,policy,*work,unchanged)==a::Status::okay);
    C(bytes(output)==bytes(unchanged)); // Original C0 geometry does not apply HMD again.
    for(unsigned bad=0;bad<8;++bad){auto rejected=input;auto late=policy;
        switch(bad){case 0:rejected.metadata_begin_ns=now-50000001;break;
        case 1:rejected.observed.stereo.received_ns=now-40000001;break;
        case 2:rejected.scene_cache_revision=0;break;
        case 3:rejected.observed.reference_revision|=1;break;
        case 4:rejected.observed.publisher_start=0;break;
        case 5:rejected.metadata_end_ns=now+1;break;
        case 6:rejected.observed.controllers.sequence++;break;
        case 7:late.now_ns+=40000001;break;}
        const auto before=bytes(output);C(a::measure_cover_pose(rejected,*model,*frame,marker,late,*work,output)!=a::Status::okay);
        C(bytes(output)==before);
    }
    const auto saved_alias=bytes(input);auto& alias=*reinterpret_cast<a::CoverPoseMeasurement*>(&input);
    C(a::measure_cover_pose(input,*model,*frame,marker,policy,*work,alias)==a::Status::alias);C(bytes(input)==saved_alias);
    std::printf("current cover C0 measurement: %u checks, %u failures; synthetic Core/Tick and recorded skeleton only\n",checks,failures);
    return failures?1:0;
}
