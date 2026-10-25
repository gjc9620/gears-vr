#include "native_rig_tracking_frame.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <utility>

#ifdef NDEBUG
#error Native Rig core CPU assertions must remain enabled
#endif
#if defined(GEARS_NATIVE_RIG_FROZEN_V1) && GEARS_NATIVE_RIG_FROZEN_V1
// The build supplies the exact frozen BEFORE header on its include path.
// Namespace isolation changes no frozen bytes or dependent shared types.
#define gears_rig_tracking frozen_gears_rig_tracking
#include "rig_tracking_frame.before.h"
#undef gears_rig_tracking
#endif

namespace nr=gears_native_rig_tracking;
namespace rig=gears_rig_tracking;
namespace {
unsigned checks{},groups{},differential_frames{},differential_requests{},byte_exact_projections{};
#define CHECK(x) do {++checks;if(!(x)){std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);std::abort();}} while(false)
constexpr std::int64_t now=2000000000;
constexpr std::uint64_t age=100000000;
template<class T,class=void>struct has_cookie:std::false_type {};
template<class T>struct has_cookie<T,std::void_t<decltype(std::declval<T>().registration_cookie)>>:std::true_type {};
static_assert(!has_cookie<nr::NativeSessionKey>::value&&!std::is_same_v<nr::CoreSnapshot,rig::Frame>);
static_assert(!std::is_convertible_v<nr::CoreSnapshot,rig::Frame>);
static_assert(noexcept(nr::core_snapshot_valid(nr::CoreSnapshot{},now,age)));
static_assert(noexcept(nr::project_for_authenticated_registration(nr::CoreSnapshot{},rig::NativeKey{},now,age,std::declval<rig::Frame&>())));

gears_weapon_hand::ObjectIdentity object(std::uint64_t address,std::uint32_t index){
    return {address,0x8000+address,0x9000,index,index+10,index+2,1};
}
rig::Frame frame(unsigned source=0,bool hands=true){
    rig::Frame f{};f.status=rig::Status::ready;f.flags=rig::cover_key_known;
    f.native={21,31,41,51,61};f.publication_sequence=71;f.scene_invocation=81;f.world_generation=91;
    f.binding_generation=101;f.reference_generation=111;f.published_ns=now-1000000;
    f.scene_qpc=121;f.qpc_frequency=10000000;f.observed={131,141,151,{161,171}};
    f.native_reference={181,rig::reference_stable,0};f.view_policy=15;
    f.world=object(0x1000,1);f.controller=object(0x1100,2);f.pawn=object(0x1200,3);
    f.mesh=object(0x1300,4);f.weapon=object(0x1400,5);f.cover_link=object(0x1500,6);f.cover_slot=3;f.cover_type=2;
    f.units_per_metre=100;f.source_view={1,0,0,0,0,1,0,0,0,0,1,0,1,2,3,1};
    f.base_view=f.source_view;f.base_view[12]=4;f.base_view[13]=5;f.base_view[14]=6;
    f.reference.position={.1,.2,.3};f.reference.orientation={0,.6,0,.8};
    auto& s=f.stereo;s.source=static_cast<gears_vr::StereoSource>(source);s.session=191;s.sequence=201;s.space_epoch=211;
    s.received_ns=now-2000000;s.predicted_time=100000;s.head_flags=s.view_flags=15;s.focused=s.visible=s.should_render=1;
    s.head={{.2,.3,-.4},{0,0,.6,.8}};
    for(unsigned i=0;i<2;++i){s.eyes[i].pose={};s.eyes[i].pose.position.x=i?.032:-.032;s.eyes[i].fov={-.7,.7,.6,-.6};}
    f.world_pose_mask=rig::head_pose;
    CHECK(gears_vr::rig_tracking::pose(f.base_view,f.reference,s.head,f.units_per_metre,f.world_head));
    if(hands){
        f.flags|=rig::controllers_matched|rig::producer_continuity_available;f.producer={221,{231,241}};
        f.controller_publisher_pid=251;f.controller_publisher_start=261;
        auto& input=f.controllers;input.source=s.source;input.session=s.session;input.sequence=s.sequence;input.space_epoch=s.space_epoch;
        input.profile_generation=271;input.input_epoch=281;input.predicted_time=s.predicted_time;
        input.received_ns=now-3000000;input.sync_ns=now-4000000;input.focused=1;
        for(unsigned i=0;i<2;++i){auto& h=input.hands[i];h.grip_active=h.aim_active=1;h.grip_flags=h.aim_flags=15;
            h.grip={{.1+double(i)*.2,.4,-.5},{.6,0,0,.8}};h.aim={{-.1,.5,.2},{0,.6,0,.8}};
            h.trigger_active=h.squeeze_active=h.stick_active=1;h.trigger=.3f;h.squeeze=.4f;h.stick_x=-.2f;h.stick_y=.1f;
            h.last_squeeze_press_grip={{1,2,3},{0,0,.6,.8}};
            CHECK(gears_vr::rig_tracking::pose(f.base_view,f.reference,h.grip,f.units_per_metre,f.world_grip[i]));}
        f.world_pose_mask|=rig::left_grip_pose|rig::right_grip_pose;
    }
    CHECK(rig::frame_valid(f,now,age,source==1));return f;
}
nr::CoreSnapshot core(rig::Frame const& f){
    nr::CoreSnapshot c{};rig::detail::copy_payload(f,c);c.source_ticket=291;c.native_reference_revision=302;return c;
}
template<class T>std::array<unsigned char,sizeof(T)> bytes(T const& value){
    std::array<unsigned char,sizeof(T)> out{};std::memcpy(out.data(),&value,sizeof(value));return out;
}
bool compare_frame(rig::Frame const& f,std::int64_t stamp=now,std::uint64_t maximum=age,bool sim=false){
    const bool result=rig::frame_valid(f,stamp,maximum,sim);
#if defined(GEARS_NATIVE_RIG_FROZEN_V1) && GEARS_NATIVE_RIG_FROZEN_V1
    frozen_gears_rig_tracking::Frame old{};static_assert(sizeof(old)==sizeof(f));std::memcpy(&old,&f,sizeof(old));
    CHECK(result==frozen_gears_rig_tracking::frame_valid(old,stamp,maximum,sim));++differential_frames;
#endif
    return result;
}
bool compare_request(rig::ReadRequest const& request){
    const bool result=rig::request_valid(request);
#if defined(GEARS_NATIVE_RIG_FROZEN_V1) && GEARS_NATIVE_RIG_FROZEN_V1
    frozen_gears_rig_tracking::ReadRequest old{};static_assert(sizeof(old)==sizeof(request));std::memcpy(&old,&request,sizeof(old));
    CHECK(result==frozen_gears_rig_tracking::request_valid(old));++differential_requests;
#endif
    return result;
}
void projection_failure(nr::CoreSnapshot const& c,rig::NativeKey key,std::int64_t stamp=now,std::uint64_t maximum=age,bool sim=false){
    rig::Frame out=frame();const auto before=bytes(out);
    CHECK(!nr::project_for_authenticated_registration(c,key,stamp,maximum,out,sim));CHECK(bytes(out)==before);
}
template<class Change>void payload_rejection(Change change){
    auto f=frame();auto c=core(f);change(f);change(c);
    CHECK(!compare_frame(f));CHECK(!nr::core_snapshot_valid(c,now,age));projection_failure(c,f.native);
}
template<class Test>void run(char const* name,Test test){test();++groups;std::printf("PASS %s\n",name);}

void abi(){
    static_assert(sizeof(rig::Frame)==1824&&alignof(rig::Frame)==8);
    static_assert(sizeof(rig::ReadRequest)==40&&alignof(rig::ReadRequest)==8);
    static_assert(sizeof(nr::CoreSnapshot)==1832&&alignof(nr::CoreSnapshot)==8);
    static_assert(offsetof(nr::CoreSnapshot,source_ticket)==1816&&offsetof(nr::CoreSnapshot,native_reference_revision)==1824);
    std::printf("ABI Frame=%zu/%zu ReadRequest=%zu/%zu NativeSessionKey=%zu/%zu CoreSnapshot=%zu/%zu source_ticket=%zu native_reference_revision=%zu\n",
        sizeof(rig::Frame),alignof(rig::Frame),sizeof(rig::ReadRequest),alignof(rig::ReadRequest),sizeof(nr::NativeSessionKey),alignof(nr::NativeSessionKey),
        sizeof(nr::CoreSnapshot),alignof(nr::CoreSnapshot),offsetof(nr::CoreSnapshot,source_ticket),offsetof(nr::CoreSnapshot,native_reference_revision));
#if defined(GEARS_NATIVE_RIG_FROZEN_V1) && GEARS_NATIVE_RIG_FROZEN_V1
    namespace old=frozen_gears_rig_tracking;
#define TYPE(T) static_assert(sizeof(rig::T)==sizeof(old::T)&&alignof(rig::T)==alignof(old::T))
    TYPE(Frame);TYPE(ReadRequest);TYPE(NativeKey);TYPE(ObservedContinuity);TYPE(ProducerContinuity);TYPE(NativeReference);
#undef TYPE
#define FIELD(T,M) static_assert(offsetof(rig::T,M)==offsetof(old::T,M)&&sizeof(decltype(rig::T::M))==sizeof(decltype(old::T::M))); \
    std::printf("ABI " #T "." #M " offset=%zu bytes=%zu frozen_equal=1\n",offsetof(rig::T,M),sizeof(decltype(rig::T::M)))
    FIELD(Frame,size);FIELD(Frame,abi_version);FIELD(Frame,status);FIELD(Frame,flags);FIELD(Frame,native);
    FIELD(Frame,publication_sequence);FIELD(Frame,scene_invocation);FIELD(Frame,world_generation);FIELD(Frame,binding_generation);
    FIELD(Frame,reference_generation);FIELD(Frame,published_ns);FIELD(Frame,scene_qpc);FIELD(Frame,qpc_frequency);
    FIELD(Frame,observed);FIELD(Frame,producer);FIELD(Frame,native_reference);FIELD(Frame,controller_publisher_pid);
    FIELD(Frame,view_policy);FIELD(Frame,controller_publisher_start);FIELD(Frame,world);FIELD(Frame,controller);FIELD(Frame,pawn);
    FIELD(Frame,mesh);FIELD(Frame,weapon);FIELD(Frame,cover_link);FIELD(Frame,cover_slot);FIELD(Frame,cover_type);FIELD(Frame,units_per_metre);
    FIELD(Frame,source_view);FIELD(Frame,base_view);FIELD(Frame,reference);FIELD(Frame,stereo);FIELD(Frame,controllers);
    FIELD(Frame,world_pose_mask);FIELD(Frame,reserved);FIELD(Frame,world_head);FIELD(Frame,world_grip);
    FIELD(ReadRequest,size);FIELD(ReadRequest,abi_version);FIELD(ReadRequest,registration_cookie);FIELD(ReadRequest,native_started_qpc);
    FIELD(ReadRequest,minimum_publication_sequence);FIELD(ReadRequest,maximum_source_age_ns);
    FIELD(NativeKey,game_pid);FIELD(NativeKey,game_thread);FIELD(NativeKey,game_start);FIELD(NativeKey,native_started_qpc);FIELD(NativeKey,registration_cookie);
    FIELD(ObservedContinuity,stream);FIELD(ObservedContinuity,reference);FIELD(ObservedContinuity,head_tracking);FIELD(ObservedContinuity,hand_tracking);
    FIELD(ProducerContinuity,stream);FIELD(ProducerContinuity,tracking);FIELD(NativeReference,epoch);FIELD(NativeReference,flags);FIELD(NativeReference,reserved);
#undef FIELD
    CHECK(rig::version==old::version&&rig::maximum_age_ns==old::maximum_age_ns);
    CHECK(std::strcmp(rig::read_export,old::read_export)==0);
#endif
}
} // namespace

int main(){
    run("all-public-layout-members-offsets-and-native-independent-type",abi);
    run("core-without-cookie-and-byte-exact-authenticated-public-projection",[]{
        for(unsigned source=0;source<2;++source)for(bool hands:{false,true}){
            const auto f=frame(source,hands);const auto c=core(f);const auto original=bytes(c);
            CHECK(nr::core_snapshot_valid(c,now,age,source==1));
            rig::Frame out{};CHECK(nr::project_for_authenticated_registration(c,f.native,now,age,out,source==1));
            CHECK(bytes(out)==bytes(f)&&bytes(c)==original);CHECK(compare_frame(out,now,age,source==1));
            auto zero_cookie=f;zero_cookie.native.registration_cookie=0;CHECK(!compare_frame(zero_cookie,now,age,source==1));
            CHECK(nr::core_snapshot_valid(c,now,age,source==1));
        }
    });
    run("unknown-default-wrong-session-and-zero-registration-preserve-output",[]{
        const auto f=frame();const auto c=core(f);CHECK(!nr::core_snapshot_valid({},now,age));projection_failure({},f.native);
        projection_failure(c,{});auto key=f.native;key.registration_cookie=0;projection_failure(c,key);
        for(unsigned field=0;field<4;++field){key=f.native;
            if(field==0)++key.game_pid;if(field==1)++key.game_thread;if(field==2)++key.game_start;if(field==3)++key.native_started_qpc;
            projection_failure(c,key);}
        // Only the caller can prove that a nonzero cookie is currently owned.
        key=f.native;++key.registration_cookie;rig::Frame projected{};
        CHECK(nr::project_for_authenticated_registration(c,key,now,age,projected));CHECK(projected.native.registration_cookie==key.registration_cookie);
    });
    run("internal-latch-metadata-shape-is-not-live-permission",[]{
        const auto f=frame();auto c=core(f);c.source_ticket=0;CHECK(!nr::core_snapshot_valid(c,now,age));projection_failure(c,f.native);
        c=core(f);c.native_reference_revision=0;CHECK(!nr::core_snapshot_valid(c,now,age));projection_failure(c,f.native);
        c=core(f);++c.native_reference_revision;CHECK(!nr::core_snapshot_valid(c,now,age));projection_failure(c,f.native);
        c=core(f);++c.source_ticket;c.native_reference_revision+=2;CHECK(nr::core_snapshot_valid(c,now,age));
        // Changed nonzero ticket/even revision still passes data shape; no live
        // comparison is possible in this module. The owner must do that check.
        rig::Frame projected{};CHECK(nr::project_for_authenticated_registration(c,f.native,now,age,projected));CHECK(bytes(projected)==bytes(f));
    });
    run("targeted-shared-payload-rejections-and-unchanged-failed-projection",[]{
#define ZERO(M) payload_rejection([](auto& f){f.M={};})
        payload_rejection([](auto& f){--f.size;});payload_rejection([](auto& f){++f.abi_version;});
        payload_rejection([](auto& f){f.status=rig::Status::stopped;});payload_rejection([](auto& f){f.flags|=8;});
        payload_rejection([](auto& f){f.view_policy|=16;});payload_rejection([](auto& f){f.reserved=1;});
        ZERO(native.game_pid);ZERO(native.game_thread);ZERO(native.game_start);ZERO(native.native_started_qpc);
        ZERO(publication_sequence);ZERO(scene_invocation);ZERO(world_generation);ZERO(binding_generation);ZERO(reference_generation);ZERO(published_ns);
        ZERO(scene_qpc);ZERO(qpc_frequency);ZERO(observed.stream);ZERO(observed.reference);ZERO(observed.head_tracking);
        ZERO(observed.hand_tracking[0]);ZERO(observed.hand_tracking[1]);ZERO(native_reference.epoch);
        payload_rejection([](auto& f){f.native_reference.flags=rig::reference_pending;});
        payload_rejection([](auto& f){f.native_reference.flags=rig::reference_exhausted;});payload_rejection([](auto& f){f.native_reference.reserved=1;});
        ZERO(world);ZERO(controller);ZERO(pawn);ZERO(mesh);
        payload_rejection([](auto& f){f.weapon.known=0;});
        payload_rejection([](auto& f){f.source_view[15]=0;});payload_rejection([](auto& f){f.base_view[3]=.1f;});
        payload_rejection([](auto& f){f.reference.orientation={0,0,0,0};});
        payload_rejection([](auto& f){f.units_per_metre=9;});payload_rejection([](auto& f){f.units_per_metre=501;});
        payload_rejection([](auto& f){f.units_per_metre=(std::numeric_limits<double>::quiet_NaN)();});
        payload_rejection([](auto& f){f.stereo.source=static_cast<gears_vr::StereoSource>(2);});ZERO(stereo.space_epoch);
        payload_rejection([](auto& f){f.stereo.head_flags=0;});payload_rejection([](auto& f){f.stereo.view_flags=0;});
        payload_rejection([](auto& f){f.stereo.received_ns=now;});ZERO(stereo.received_ns);
        ZERO(producer.stream);ZERO(producer.tracking[0]);ZERO(producer.tracking[1]);
        payload_rejection([](auto& f){f.flags&=~rig::controllers_matched;});
        payload_rejection([](auto& f){f.flags&=~rig::producer_continuity_available;});
        ZERO(cover_type);ZERO(cover_link);payload_rejection([](auto& f){f.cover_slot=-1;});
        payload_rejection([](auto& f){f.flags&=~rig::cover_key_known;});
        payload_rejection([](auto& f){f.world_pose_mask|=8;});payload_rejection([](auto& f){f.world_pose_mask&=~rig::head_pose;});
        payload_rejection([](auto& f){f.world_head.orientation={0,0,0,0};});
        ZERO(controller_publisher_pid);ZERO(controller_publisher_start);
        payload_rejection([](auto& f){++f.controllers.session;});payload_rejection([](auto& f){++f.controllers.sequence;});
        payload_rejection([](auto& f){++f.controllers.space_epoch;});payload_rejection([](auto& f){++f.controllers.predicted_time;});
        ZERO(controllers.profile_generation);ZERO(controllers.input_epoch);ZERO(controllers.focused);
        ZERO(controllers.received_ns);ZERO(controllers.sync_ns);payload_rejection([](auto& f){f.controllers.received_ns=now;});
        for(unsigned hand=0;hand<2;++hand){
            payload_rejection([hand](auto& f){f.controllers.hands[hand].grip_active=0;});
            payload_rejection([hand](auto& f){f.controllers.hands[hand].grip_flags=3;});
            payload_rejection([hand](auto& f){f.world_grip[hand].orientation={0,0,0,0};});
        }
#undef ZERO
    });
    run("original-age-boundaries-source-profiles-and-head-only-semantics",[]{
        const auto f=frame();const auto c=core(f);
        for(auto delta:{std::int64_t(-1),std::int64_t(0),std::int64_t(96000000),std::int64_t(97000000),std::int64_t(98000000),std::int64_t(100000000)})
            for(auto maximum:{std::uint64_t(0),std::uint64_t(1),std::uint64_t(50000000),age,age+1}){
                const bool expected=compare_frame(f,now+delta,maximum);
                CHECK(nr::core_snapshot_valid(c,now+delta,maximum)==expected);
                rig::Frame out{};CHECK(nr::project_for_authenticated_registration(c,f.native,now+delta,maximum,out)==expected);
                if(expected)CHECK(bytes(out)==bytes(f));
            }
        CHECK(!nr::core_snapshot_valid(c,0,age));CHECK(!nr::core_snapshot_valid(c,-1,age));
        auto sim=frame(1);CHECK(!compare_frame(sim));CHECK(!nr::core_snapshot_valid(core(sim),now,age));
        CHECK(compare_frame(sim,now,age,true)&&nr::core_snapshot_valid(core(sim),now,age,true));
        CHECK(!compare_frame(f,now,age,true));
        auto head=frame(0,false);head.controllers.source=static_cast<gears_vr::StereoSource>(99);
        head.world_grip[0].position.x=(std::numeric_limits<double>::quiet_NaN)();
        CHECK(compare_frame(head));CHECK(nr::core_snapshot_valid(core(head),now,age));
        rig::Frame projected{};CHECK(nr::project_for_authenticated_registration(core(head),head.native,now,age,projected));
        CHECK(bytes(projected)==bytes(head)); // Preserve ignored bytes too.
    });
    run("all-frame-byte-bit-mutations-against-frozen-V1-validator",[]{
        for(unsigned source=0;source<2;++source)for(bool hands:{false,true}){
            const auto good=frame(source,hands);
            for(std::size_t offset=0;offset<sizeof(good);++offset)for(unsigned bit=0;bit<8;++bit){
                auto changed=good;auto* raw=reinterpret_cast<unsigned char*>(&changed);raw[offset]^=static_cast<unsigned char>(1u<<bit);
                if(compare_frame(changed,now,age,source==1)){
                    const auto native=core(changed);CHECK(nr::core_snapshot_valid(native,now,age,source==1));
                    rig::Frame projected{};
                    CHECK(nr::project_for_authenticated_registration(native,changed.native,now,age,projected,source==1));
                    CHECK(bytes(projected)==bytes(changed));++byte_exact_projections;
                }
            }
        }
    });
    run("all-request-byte-bit-mutations-against-frozen-V1-validator",[]{
        rig::ReadRequest good{};good.registration_cookie=61;good.native_started_qpc=51;good.minimum_publication_sequence=71;
        CHECK(compare_request(good));for(std::size_t offset=0;offset<sizeof(good);++offset)for(unsigned bit=0;bit<8;++bit){
            auto changed=good;auto* raw=reinterpret_cast<unsigned char*>(&changed);raw[offset]^=static_cast<unsigned char>(1u<<bit);(void)compare_request(changed);
        }
        for(auto maximum:{std::uint64_t(0),std::uint64_t(1),std::uint64_t(50000000),age,age+1,(std::numeric_limits<std::uint64_t>::max)()}){
            auto changed=good;changed.maximum_source_age_ns=maximum;CHECK(compare_request(changed)==(maximum>0&&maximum<=age));}
        good.registration_cookie=0;CHECK(!compare_request(good));
    });
    std::printf("PASS NativeRigTrackingFrame groups=%u checks=%u frozen_frame_comparisons=%u frozen_request_comparisons=%u byte_exact_valid_mutant_projections=%u live_permission_proved=0\n",
        groups,checks,differential_frames,differential_requests,byte_exact_projections);return 0;
}
