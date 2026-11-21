// True Shared consumer/Broker/controller mapping, synthetic actor sampler.
// Separate client output memory. No game/XR/engine call or real module unpin.
#include "../vr-common/rig_tracking_frame.h"
#include "../vr-common/native_rig_tracking_frame.h"
#include <thread>
#define main RigTrackingSharedFixtureMain
#include "cover_shared_input_live_test.cpp"
#undef main
namespace {
std::atomic<bool> recording{true};std::atomic<uint64_t> committed_world_generation{1};
bool continuous_session=true,stereo_xr=true,anchor_observer_enabled=true;
long long qpc_frequency{};
namespace weapon_hand_runtime::detail {
std::atomic<bool> admission_closed{},stop_requested{},unknown_fault{};std::atomic<unsigned> calls{};
struct Call {Call(){++calls;}~Call(){--calls;}};
}
#if defined(GEARS_COVER_CONTEXT_READER) && GEARS_COVER_CONTEXT_READER
// Publisher integration only. The real reader's reflected identity, layout,
// closing and no-legacy-hook behavior is covered by CoverContextLiveOnTest.
#define GEARS_COVER_CONTEXT_LIVE_INCLUDED
namespace cover_context_live {
struct Observation {weapon_hand_scene_binding::Binding actor{};gears_anchor::Identity link{};
    int slot{-1};unsigned char cover_type{};};
unsigned calls{},fail_on{},change_on{},closing_mutation{};unsigned char type{};bool wrong_actor{};
bool same(Observation const& a,Observation const& b){return
    weapon_hand_scene_binding::same_key(a.actor,b.actor)&&a.actor.special_move==b.actor.special_move&&
    a.actor.scene_invocation==b.actor.scene_invocation&&gears_anchor::equal_identity(a.link,b.link)&&
    a.slot==b.slot&&a.cover_type==b.cover_type;}
bool sample(gears_anchor::Receipt const* receipt,Observation& out){
    if(++calls==fail_on||!receipt)return false;
    // Model the real cover reader's two fresh actor reads, not an actor flag
    // copied from its fixture. Existing binding-close/reference hooks must
    // also run on the ON path after outer overlapping samples are removed.
    namespace binding=weapon_hand_scene_binding;
    const auto opened_receipt=*receipt;
    if(calls==2){
        if(closing_mutation==1)++binding::actor.mesh.number;
        if(closing_mutation==2)++binding::actor.world.number;
        if(closing_mutation==3)++binding::actor.special_move;
        if(closing_mutation==4)++const_cast<gears_anchor::Receipt*>(receipt)->renderer_qpc;
    }
    binding::Binding first{},again{};
    if(!binding::sample(receipt,first)||!binding::sample(receipt,again)||
       !binding::same_key(first,again)||first.special_move!=again.special_move||
       first.scene_invocation!=again.scene_invocation||std::memcmp(&opened_receipt,receipt,sizeof opened_receipt))return false;
    out.actor=again;
    if(calls==2&&closing_mutation==5)++out.actor.scene_invocation;
    if(wrong_actor)out.actor.mesh.number++;
    out.cover_type=type;out.slot=type?0:-1;
    if(type)out.link=weapon_hand_scene_binding::actor.world; // Synthetic key only.
    if(calls==change_on)++out.slot;
    return true;
}
}
#endif
#include "rig_tracking_live.inl"
namespace rig=rig_tracking_live;namespace api=gears_rig_tracking;
int64_t fake_clock{};int64_t rig_clock(){return fake_clock;}
gears_vr::ReferenceContinuity frozen_reference{1,true,false,false};
gears_vr::StereoMatrix base_view(){return {0,0,1,0,1,0,0,0,0,1,0,0,-20,-30,-10,1};}
void selected(double up=0){next(up);scene.scene_qpc=now();scene.family_qpc=scene.scene_qpc+1;scene.renderer_qpc=scene.scene_qpc+2;
    view=base_view();run();fake_clock=gears_vr::monotonic_ns();}
bool publish(uint32_t policy=api::first_person){const auto prepares=pg.prepares,commits=pg.commits,invalidations=pg.invalidations;
    const auto ok=rig::publish(&scene,base_view(),view,stereo,policy,frozen_reference);C(pg.prepares==prepares&&pg.commits==commits&&pg.invalidations==invalidations);return ok;}
api::Reply read(api::ReadRequest const& request,api::Frame& out){const auto pg_calls=pg.prepares+pg.commits+pg.invalidations;
    const auto result=rig::read(&request,&out);C(pg_calls==pg.prepares+pg.commits+pg.invalidations&&runtime::detail::calls==0);return result;}
void change_binding(unsigned count){if(count==2)native::actor.mesh.number++;}
void reference_boundary(unsigned count){if(count==2)rig::reference_changed({2,false,true,false});}
int main_test(const char* mode){
    if(!strcmp(mode,"configure")){
        wchar_t folder[MAX_PATH]{},ini[MAX_PATH]{};C(GetTempPathW(MAX_PATH,folder)>0);C(GetTempFileNameW(folder,L"rig",0,ini)>0);
        C(!rig::enabled&&rig::configure(ini,nullptr));C(WritePrivateProfileStringW(L"RigTracking",L"enabled",L"1",ini)!=0);
        continuous_session=false;C(!rig::configure(ini,nullptr));continuous_session=true;stereo_xr=false;C(!rig::configure(ini,nullptr));stereo_xr=true;
        anchor_observer_enabled=false;C(!rig::configure(ini,nullptr));anchor_observer_enabled=true;runtime::enabled=false;C(!rig::configure(ini,nullptr));runtime::enabled=true;
        C(rig::configure(ini,nullptr)&&rig::enabled);C(!rig::configure(ini,nullptr));C(DeleteFileW(ini)!=0);
        printf("rig_tracking_live configure: %u checks, %u failures\n",checks,failures);return failures?1:0;
    }
    setup(false,true);attach();shared::continuity_enabled=true;gears_hand_continuity::InputBridge companion;gears_hand_continuity::SourceTracker tracker;
    if(!strcmp(mode,"companion"))C(companion.open(GetCurrentProcessId(),true));
    release();LARGE_INTEGER frequency{};C(QueryPerformanceFrequency(&frequency)!=0);qpc_frequency=frequency.QuadPart;
    rig::enabled=true;rig::clock_ns=rig_clock;selected(.2);api::ReadRequest request{};request.registration_cookie=cookie;request.native_started_qpc=500;
    api::Frame out{};C(rig::observed.stream==0&&rig::sequence==0);C(read(request,out)==api::Reply::unavailable&&out.status==api::Status::unavailable);
    // Closing the mapping reader proves publish/read consume only the selected copy.
    shared::TrackingBundle bundle{};C(shared::tracking_for(&scene,stereo,bundle)&&bundle.matched&&bundle.publisher_pid==GetCurrentProcessId());
    input::controllers.close();input::bridge_open=false;
    C(!publish()&&rig::sequence==0&&rig::observed.stream==0);rig::reference_changed(frozen_reference);
    native::sample_calls=0;
    C(publish());
#if defined(GEARS_COVER_CONTEXT_READER) && GEARS_COVER_CONTEXT_READER
    C(native::sample_calls==4);
#else
    C(native::sample_calls==2);
#endif
    C(!input::bridge_open);C(read(request,out)==api::Reply::accepted&&api::frame_valid(out,fake_clock,50000000,true));
    C(out.native_reference.epoch==1&&out.native_reference.flags==api::reference_stable);
    C(out.flags&api::controllers_matched);C(!(out.flags&api::producer_continuity_available)&&api::producer_empty(out.producer));
    C(!memcmp(&out.controllers,&bundle.hands,sizeof bundle.hands)&&out.controller_publisher_start==bundle.publisher_start);
    C(std::abs(out.world_head.position.z-50)<.001&&out.world_pose_mask==7);const auto first=out;
    // Convenience head origin must equal actual one-pass stereo midpoint.
    const gears_vr::StereoMatrix projection{1,0,0,0,0,1,0,0,0,0,0,1,0,0,10,0};gears_vr::StereoMatrices eyes{};
    C(gears_vr::stereo_matrices(view,projection,stereo.reference,stereo.sample,100,eyes,true));
    const auto l=gears_vr::stereo_camera_position(eyes.view[0]),r=gears_vr::stereo_camera_position(eyes.view[1]);
    C(std::abs((l.z+r.z)/2-out.world_head.position.z)<.001);
    out.pawn.address=0;out.base_view={};C(read(request,out)==api::Reply::accepted&&out.pawn.address==first.pawn.address&&out.base_view==first.base_view);
    const auto revision=rig::reference_revision.load();rig::reference_changed(frozen_reference);C(rig::reference_revision==revision);
    fake_clock+=1000000;C(publish());C(read(request,out)==api::Reply::accepted&&out.publication_sequence==first.publication_sequence&&out.published_ns==first.published_ns);
    if(!strcmp(mode,"normal")){
        request.registration_cookie++;C(read(request,out)==api::Reply::wrong_cookie&&out.status==api::Status::unavailable);request.registration_cookie=cookie;
        request.native_started_qpc++;C(read(request,out)==api::Reply::wrong_cookie&&out.status==api::Status::unavailable);request.native_started_qpc=500;
        request.minimum_publication_sequence=first.publication_sequence+1;C(read(request,out)==api::Reply::unavailable);request.minimum_publication_sequence=0;
        request.size--;C(read(request,out)==api::Reply::invalid);request.size=sizeof(request);C(rig::read(&request,nullptr)==api::Reply::invalid);
        C(rig::read(reinterpret_cast<api::ReadRequest const*>(uintptr_t(1)),&out)==api::Reply::invalid&&out.status==api::Status::unavailable&&runtime::detail::calls==0);
        fake_clock+=50000001;C(read(request,out)==api::Reply::unavailable&&out.status==api::Status::unavailable);
    }else if(!strcmp(mode,"mutated")){
        view[12]-=1;C(!publish());C(read(request,out)==api::Reply::unavailable&&out.status==api::Status::unavailable);
        C(rig::observed.stream>first.observed.stream);
    }else if(!strcmp(mode,"epochs")){
        input::last_open=0;input::attempted_open=false;selected(.3);C(publish());C(read(request,out)==api::Reply::accepted&&out.binding_generation==first.binding_generation);
        C(out.publication_sequence>first.publication_sequence&&out.observed.stream==first.observed.stream);
        ++stereo.reference_generation;next(.3);scene.scene_qpc=now();scene.family_qpc=scene.scene_qpc+1;scene.renderer_qpc=scene.scene_qpc+2;run();fake_clock=gears_vr::monotonic_ns();
        C(publish(api::first_person|api::run_stabilized));C(read(request,out)==api::Reply::accepted&&out.binding_generation>first.binding_generation&&out.observed.reference>first.observed.reference);
    }else if(!strcmp(mode,"focus")){
        stereo.sample.focused=0;C(!publish());const auto lost=rig::observed.stream;C(read(request,out)==api::Reply::unavailable);
        C(!publish()&&rig::observed.stream==lost&&lost>first.observed.stream);
    }else if(!strcmp(mode,"empty")){
        native::actor.weapon={};input::last_open=0;input::attempted_open=false;selected();C(publish());C(read(request,out)==api::Reply::accepted&&!out.weapon.address);
    }else if(!strcmp(mode,"ticket")){
        shared::clear_tracking();C(read(request,out)==api::Reply::unavailable&&out.status==api::Status::unavailable);
    }else if(!strcmp(mode,"old-source")){
        fake_clock+=100000001;C(!publish());C(read(request,out)==api::Reply::unavailable&&out.status==api::Status::unavailable);
    }else if(!strcmp(mode,"binding-close")){
        native::sample_calls=0;native::sample_hook=change_binding;C(!publish());native::sample_hook=nullptr;
        C(read(request,out)==api::Reply::unavailable&&out.status==api::Status::unavailable);
    }else if(!strcmp(mode,"publisher")||!strcmp(mode,"profile")){
        input::last_open=0;input::attempted_open=false;selected();
        if(!strcmp(mode,"publisher"))shared::tracking.publisher_start++;
        else shared::tracking.hands.profile_generation++;
        C(publish());C(read(request,out)==api::Reply::accepted&&out.observed.stream>first.observed.stream);
        C(api::producer_empty(out.producer)&&!(out.flags&api::producer_continuity_available));
    }else if(!strcmp(mode,"wrap")){
        input::last_open=0;input::attempted_open=false;selected();rig::sequence=UINT64_MAX;
        C(!publish()&&rig::sequence==UINT64_MAX&&rig::stopped.load());
    }else if(!strcmp(mode,"race")){
        std::atomic<bool> started{};std::atomic<unsigned> faults{};
        std::thread reader([&]{started=true;for(unsigned i=0;i<1000;++i){api::Frame client{};const auto reply=rig::read(&request,&client);
            if(reply==api::Reply::accepted){if(client.status!=api::Status::ready)++faults;}
            else if(client.status!=api::Status::unavailable)++faults;}});
        while(!started.load())std::this_thread::yield();rig::stop();reader.join();C(!faults&&runtime::detail::calls==0);
    }else if(!strcmp(mode,"reference")){
        rig::reference_changed({2,false,true,false});C(read(request,out)==api::Reply::unavailable&&out.status==api::Status::unavailable);
        rig::reference_changed({2,true,false,false});C(!publish());C(read(request,out)==api::Reply::unavailable);
        input::last_open=0;input::attempted_open=false;selected();frozen_reference={2,true,false,false};C(publish());
        C(read(request,out)==api::Reply::accepted&&out.native_reference.epoch==2&&out.observed.reference>first.observed.reference);
    }else if(!strcmp(mode,"reference-inflight")){
        native::sample_calls=0;native::sample_hook=reference_boundary;C(!publish());native::sample_hook=nullptr;
        C(read(request,out)==api::Reply::unavailable);rig::reference_changed({2,true,false,false});C(!publish());
    }else if(!strcmp(mode,"companion")){
        next(.3);gears_hand_continuity::TrackedInput tracked{};C(tracker.update(hands,tracked)&&companion.publish(tracked));
        scene.scene_qpc=now();scene.family_qpc=scene.scene_qpc+1;scene.renderer_qpc=scene.scene_qpc+2;
        input::last_open=0;input::attempted_open=false;view=base_view();run();fake_clock=gears_vr::monotonic_ns();C(publish());
        C(read(request,out)==api::Reply::accepted&&(out.flags&api::producer_continuity_available));
        C(out.producer.stream==tracked.stream_epoch&&out.producer.tracking[0]==tracked.tracking_epoch[0]&&out.producer.tracking[1]==tracked.tracking_epoch[1]);
        C(shared::continuity_open&&shared::continuity_pid==GetCurrentProcessId()&&shared::continuity_start);
        shared::close();C(!shared::continuity_open&&!shared::continuity_pid&&!shared::continuity_start&&!shared::continuity_attempted&&!shared::continuity_last_open);
        C(read(request,out)==api::Reply::unavailable&&out.status==api::Status::unavailable);
    }
#if defined(GEARS_COVER_CONTEXT_READER) && GEARS_COVER_CONTEXT_READER
    else if(!strcmp(mode,"cover-context")){
        namespace ctx=cover_context_live;
        // No legacy camera-owned view or old cover input is needed to publish
        // the independently certified cover key to the shared body source.
        C(!shared::previous.cover);ctx::type=2;selected();C(publish());
        C(read(request,out)==api::Reply::accepted&&(out.flags&api::cover_key_known)&&out.cover_type==2&&out.cover_slot==0);
        C(gears_weapon_hand::same(out.cover_link,rig::object(native::actor.world))&&out.binding_generation>first.binding_generation);
        const auto cover_generation=out.binding_generation;ctx::type=0;selected();C(publish());
        C(read(request,out)==api::Reply::accepted&&!(out.flags&api::cover_key_known)&&!out.cover_link.address&&out.cover_slot==-1);
        C(out.binding_generation>cover_generation);
        for(unsigned bad=0;bad<4;++bad){selected();ctx::calls=0;ctx::fail_on=bad<2?bad+1:0;
            ctx::change_on=bad==2?2:0;ctx::wrong_actor=bad==3;ctx::type=2;
            C(!publish());C(read(request,out)==api::Reply::unavailable);
            ctx::fail_on=ctx::change_on=0;ctx::wrong_actor=false;}
        // All payload/pose calculation is enclosed by the two complete cover
        // observations. A later actual actor/world/state/receipt change cannot
        // publish merely because the initial cover observation was valid.
        for(unsigned bad=1;bad<=5;++bad){
            selected();const auto original_actor=native::actor;const auto original_scene=scene;
            ctx::calls=0;ctx::closing_mutation=bad;native::sample_calls=0;
            C(!publish());C(ctx::calls==2&&native::sample_calls==4);
            C(read(request,out)==api::Reply::unavailable);
            native::actor=original_actor;scene=original_scene;ctx::closing_mutation=0;
        }
    }
#endif
    else C(false);
    rig::stop();C(read(request,out)==api::Reply::stopping&&out.status==api::Status::unavailable);C(!publish());
    finish();printf("rig_tracking_live %s: %u checks, %u failures; no Game/XR\n",mode,checks,failures);return failures?1:0;
}
}
#ifndef GEARS_RIG_TRACKING_NO_ENTRY
int main(int argc,char** argv){return main_test(argc>1?argv[1]:"normal");}
#endif
