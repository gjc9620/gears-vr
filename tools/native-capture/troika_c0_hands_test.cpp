#define WIN32_LEAN_AND_MEAN
#define main existing_fire_owner_checks
#include "../vr-common/weapon_troika_fire_owner_test.cpp"
#undef main
#include "troika_c0_hands.h"
#undef NOMINMAX
#define main existing_native_backend_checks
#define Fixture NativeFixture
#define checks native_checks
#include "full_body_c0_native_backend_test.cpp"
#undef checks
#undef Fixture
#undef main
#undef C
#include <bcrypt.h>
#include <fstream>
#include <memory>
#include <string>
namespace t=gears_troika_c0_hands;namespace b=t::body;
bool load(std::string const& path,void* out,std::size_t bytes){std::ifstream f(path,std::ios::binary);
    return bool(f.read(static_cast<char*>(out),std::streamsize(bytes)))&&f.peek()==EOF;}
bool hash_geometry(void const* data,std::size_t bytes,b::rig::Signature& out){BCRYPT_ALG_HANDLE alg{};BCRYPT_HASH_HANDLE hash{};
    bool okay=BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
    if(okay)okay=BCryptCreateHash(alg,&hash,nullptr,0,nullptr,0,0)>=0;
    if(okay)okay=BCryptHashData(hash,reinterpret_cast<PUCHAR>(const_cast<void*>(data)),ULONG(bytes),0)>=0;
    if(okay)okay=BCryptFinishHash(hash,out.data(),ULONG(out.size()),0)>=0;
    if(hash)BCryptDestroyHash(hash);if(alg)BCryptCloseAlgorithmProvider(alg,0);return okay;}
t::source::Snapshot sample(Fixture const& f){t::source::Snapshot s{};s.source=f.source;s.control=f.decision;
    auto const& x=s.source.frame;auto& m=s.mapping;
    m.identity={x.world.address,x.world_generation,x.driver.address,x.controller.address,x.turret.address,s.source.entry_binding,
        x.weapon.address,s.source.entry_binding,x.controllers.session,x.controllers.space_epoch,x.controllers.input_epoch,x.controllers.profile_generation};
    m.publication_sequence=x.publication_sequence;m.source_ticket=x.source_ticket;m.units_per_metre=100;
    m.hands.input_sequence=x.controllers.sequence;m.hands.input_sync_ns=x.controllers.sync_ns;m.hands.input_received_ns=x.controllers.received_ns;
    m.hands.stereo_received_ns=x.stereo.received_ns;m.hands.tracked_mask=3;return s;}
v::Pose invert_mount(b::Pose const& wrist,t::mount::Mount const& m){
    const auto q=v::multiply(wrist.orientation,b::d::inverse(m.calibration.grip_to_wrist_rotation));
    return {b::d::sub(wrist.position,b::d::rotate(q,b::d::mul(m.calibration.grip_to_wrist_metres,100))),q};}
template<class T>auto bits(T const& value){std::array<unsigned char,sizeof(T)> out{};std::memcpy(out.data(),&value,sizeof value);return out;}
#if !defined(GEARS_TROIKA_C0_REUSE_FIXTURE)
int main(int argc,char** argv){
    if(argc!=2)return 2;
    const std::string dir=argv[1];auto frame=std::make_unique<b::Frame>();auto model=std::make_unique<b::Model>();
    auto work=std::make_unique<t::Workspace>();auto out=std::make_unique<b::Output>();auto original=std::make_unique<b::Workspace>();
    CHECK(load(dir+"/reference.bin",frame->reference.data(),sizeof frame->reference));
    CHECK(load(dir+"/C0-local.bin",frame->local.data(),sizeof frame->local));
    CHECK(load(dir+"/C0-component.bin",&frame->component_to_world,sizeof frame->component_to_world));
    CHECK(b::prepare_model(frame->reference,hash_geometry,*model));
    for(unsigned i=0;i<16;++i)original->component.m[i]=frame->component_to_world.values[i];
    std::copy(frame->local.begin(),frame->local.end(),original->pose.local);CHECK(b::d::rebuild(*model,*original));
    Fixture fixture;auto s=sample(fixture);std::array<std::uint64_t,2> available{{UINT64_MAX,UINT64_MAX}};
    b::Pose targets[2]{};
    for(unsigned side=0;side<2;++side){const unsigned wrist=side?35u:11u,shoulder=side?32u:8u;
        targets[side]=b::d::pose(original->pose.world[wrist]);const auto from=b::d::position(original->pose.world[shoulder]);
        targets[side].position=b::d::add(from,b::d::mul(b::d::sub(targets[side].position,from),.8));
        t::mount::Mount mount{};CHECK(t::mount::marcus115(model->signature,side?t::mount::Side::right:t::mount::Side::left,1,100,mount));
        s.mapping.hands.world[side]=invert_mount(targets[side],mount);
    }
    CHECK(t::source::valid(s,fixture.now));const auto frame_before=bits(*frame);const auto model_before=bits(*model);const auto s_before=bits(s);
    auto result=t::compose(*model,*frame,s,fixture.now,available,*work,*out);
    if(result.status!=t::Status::okay)std::printf("planner status=%u\n",unsigned(result.status));
    CHECK(result.status==t::Status::okay&&result.solved_mask==3&&!result.projected_mask);
    CHECK(bits(*frame)==frame_before&&bits(*model)==model_before&&bits(s)==s_before);
    for(unsigned i=0;i<b::bones;++i){
        CHECK(!std::memcmp(frame->local[i].translation,out->local[i].translation,12));CHECK(frame->local[i].scale==out->local[i].scale);
        if(!((i>=8&&i<=29)||(i>=32&&i<=53)))CHECK(!std::memcmp(&frame->local[i],&out->local[i],sizeof(b::Atom)));
    }
    for(unsigned side=0;side<2;++side){const unsigned wrist=side?35u:11u;
        CHECK(t::hands::detail::pose_matches(work->body_work.pose.world[wrist],targets[side]));}
    // HMD observation cannot turn a hand target or disturb the seated body.
    auto head=s;head.mapping.world_head={{100,-200,300},{0,0,.7071067811865475,.7071067811865475}};
    auto head_work=std::make_unique<t::Workspace>();b::Output head_out{};
    CHECK(t::compose(*model,*frame,head,fixture.now,available,*head_work,head_out).status==t::Status::okay);
    if(bits(head_out)!=bits(*out)){const auto before=bits(*out),after=bits(head_out);unsigned shown{};
        for(unsigned i=0;i<before.size()&&shown<8;++i)if(before[i]!=after[i]){std::printf("head difference byte=%u %u/%u local_end=%zu report_offset=%zu\n",i,unsigned(before[i]),unsigned(after[i]),sizeof out->local,offsetof(b::Output,report));++shown;}}
    CHECK(bits(head_out)==bits(*out));
    // The same tracked wrist must not inherit an unrelated animated wrist roll.
    auto noisy=*frame;for(unsigned side=0;side<2;++side){const unsigned wrist=side?35u:11u;
        auto q=noisy.local[wrist].q;auto rotated=v::multiply({q[0],q[1],q[2],q[3]},v::Quaternion{.3894183423086505,0,0,.9210609940028851});
        CHECK(b::d::set_quat(noisy.local[wrist],rotated));}
    auto noise_work=std::make_unique<t::Workspace>();b::Output noise_out{};
    CHECK(t::compose(*model,noisy,s,fixture.now,available,*noise_work,noise_out).status==t::Status::okay);
    for(unsigned bone:{8u,9u,10u,11u,29u,32u,33u,34u,35u,53u})
        CHECK(!std::memcmp(noise_out.local[bone].q,out->local[bone].q,16));
    auto rolled=*frame;
    for(unsigned side=0;side<2;++side){const unsigned fore=side?34u:10u,wrist=side?35u:11u;
        auto const& translation=rolled.local[wrist].translation;
        const b::Vec axis{translation[0],translation[1],translation[2]};const double length=b::d::length(axis);
        const v::Quaternion roll{axis.x/length*.3894183423086505,axis.y/length*.3894183423086505,axis.z/length*.3894183423086505,.9210609940028851};
        auto const& local=rolled.local[fore].q;CHECK(b::d::set_quat(rolled.local[fore],v::multiply({local[0],local[1],local[2],local[3]},roll)));
    }
    auto roll_work=std::make_unique<t::Workspace>();b::Output roll_out{};
    CHECK(t::compose(*model,rolled,s,fixture.now,available,*roll_work,roll_out).status==t::Status::okay);
    for(unsigned bone:{8u,9u,10u,11u,29u,32u,33u,34u,35u,53u}){auto const& a=roll_out.local[bone].q;auto const& c=out->local[bone].q;
        const double angle=b::d::angle({a[0],a[1],a[2],a[3]},{c[0],c[1],c[2],c[3]});
        if(angle>1e-4)std::printf("animated forearm roll leaked bone=%u radians=%g\n",bone,angle);
        CHECK(angle<=1e-4);
    }
    // Reaching too far changes only the visual endpoint; it is reported, not a
    // permission for the original gun, and never stretches the source bones.
    auto distant=s;for(auto& grip:distant.mapping.hands.world)grip.position.x+=10000;
    auto far_work=std::make_unique<t::Workspace>();b::Output far_out{};
    result=t::compose(*model,*frame,distant,fixture.now,available,*far_work,far_out);
    CHECK(result.status==t::Status::okay&&result.projected_mask==3);
    for(unsigned i=0;i<b::bones;++i)CHECK(!std::memcmp(frame->local[i].translation,far_out.local[i].translation,12));
    // The handle's right index has an R2 pose; the left handle never does.
    std::array<v::Quaternion,15> left0{},left1{},right0{},right1{};
    CHECK(t::shaft_pose(*model,0,0,left0)&&t::shaft_pose(*model,0,1,left1));
    CHECK(t::shaft_pose(*model,1,0,right0)&&t::shaft_pose(*model,1,1,right1));
    CHECK(bits(left0)==bits(left1));CHECK(bits(right0)!=bits(right1));
    for(unsigned n=0;n<15;++n)if(n<3||n>=6)CHECK(bits(right0[n])==bits(right1[n]));
    // Failure is atomic for the entire proposed Output, including stale input,
    // changed role/reference, an invalid quaternion and overlapping storage.
    for(unsigned fault=0;fault<5;++fault){auto bad=s;auto body=*frame;const auto before=bits(*out);auto scratch=std::make_unique<t::Workspace>();
        auto now=fixture.now;
        if(fault==0)now+=100000001;
        if(fault==1)bad.mapping.identity.pawn=bad.source.frame.turret.address;
        if(fault==2)body.reference[114].bytes[12]^=1;
        if(fault==3)bad.mapping.hands.world[0].orientation={0,0,0,0};
        if(fault==4)body.local[55].scale=2;
        CHECK(t::compose(*model,body,bad,now,available,*scratch,*out).status!=t::Status::okay);CHECK(bits(*out)==before);
    }
    CHECK(t::compose(*model,*frame,s,fixture.now,available,*work,work->body_work.candidate).status==t::Status::alias);
    // The production Native Backend and SAME Kernel really install this plan
    // with CAS128 in owned pages. Host original_space emulates Compose only;
    // this is not game, renderer or physics evidence.
    {
        NativeFixture host(*frame);tx::Kernel kernel;tx::Lease lease;CHECK(host.capture());const auto key=host.backend.stamp();
        CHECK(kernel.prepare(host.backend,key,host.captured,*out,host.desired_copy,lease).code==tx::Code::prepared);
        CHECK(kernel.commit(host.backend,lease,key).code==tx::Code::committed);
        CHECK(!std::memcmp(host.local.data,out->local.data(),sizeof out->local));
        CHECK(kernel.notify_consumer_entry(host.backend,lease,key).code==tx::Code::consumer_marked);
        host.original_space();CHECK(kernel.finish(host.backend,lease,key,tx::End::original_returned).code==tx::Code::scope_closed);
        n::ConsumedPose consumed{};CHECK(host.backend.read_consumed_pose(kernel,key,kernel.serial(),consumed)==n::ConsumedPoseCode::verified);
        CHECK(t::local_pose_matches(*out,consumed.local));
        CHECK(kernel.outstanding_fields()>0&&kernel.pending()>0); // Real overwrite is still owed.
        auto changed=host.atoms()[11];const auto q=v::multiply({changed.q[0],changed.q[1],changed.q[2],changed.q[3]},v::Quaternion{.0998334166468282,0,0,.9950041652780258});
        CHECK(b::d::set_quat(changed,q));host.local.put(11*sizeof(b::Atom),changed);host.original_space();
        CHECK(host.backend.read_consumed_pose(kernel,key,kernel.serial(),consumed)==n::ConsumedPoseCode::verified);
        CHECK(!t::local_pose_matches(*out,consumed.local));
        // A coherent finger/cuff overwrite also passes the old seven-body-
        // target receipt. Verify every local hand/helper rotation, not just wrist.
        for(unsigned bone:{12u,29u,36u,53u}){
            std::memcpy(host.local.data,out->local.data(),sizeof out->local);auto atom=host.atoms()[bone];
            const auto turn=v::multiply({atom.q[0],atom.q[1],atom.q[2],atom.q[3]},v::Quaternion{0,0,.0998334166468282,.9950041652780258});
            CHECK(b::d::set_quat(atom,turn));host.local.put(bone*sizeof(b::Atom),atom);host.original_space();
            CHECK(host.backend.read_consumed_pose(kernel,key,kernel.serial(),consumed)==n::ConsumedPoseCode::verified);
            CHECK(!t::local_pose_matches(*out,consumed.local));
        }
        auto sign=out->local;for(auto& atom:sign)for(auto& value:atom.q)value=-value;
        CHECK(t::local_pose_matches(*out,sign));
    }
    {
        NativeFixture host(*frame);tx::Kernel kernel;tx::Lease lease;CHECK(host.capture());const auto key=host.backend.stamp();
        CHECK(kernel.prepare(host.backend,key,host.captured,*out,host.desired_copy,lease).code==tx::Code::prepared);
        host.context.allowed=false;CHECK(kernel.commit(host.backend,lease,key).code==tx::Code::guard_rejected);
        CHECK(!std::memcmp(host.local.data,frame->local.data(),sizeof frame->local));
    }
    std::printf("Troika mounted C0 hand planner: %u checks passed; real reference fixture, synthetic tracking/control; no game, fit, engine or write grant.\n",checks);
    return 0;
}
#endif
