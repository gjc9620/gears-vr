#pragma once
#include "troika_c0_hands.h"
#include "full_body_c0_native_backend.h"
#include "../vr-common/troika_c0_consumer.h"

// Private Native store; every publication performs the actual existing
// Backend/Kernel readback. Never an alternate C0 writer or ordinary binding.
namespace gears_troika_c0_readback {
namespace api=gears_troika_c0;namespace body=gears_full_body_c0;namespace native=gears_full_body_c0_native;
namespace tx=gears_full_body_c0_transaction;namespace pose=gears_cover_pose_phase;
class Store {
    api::Frame frame_{};bool active_{};std::uint64_t sequence_{};
    std::array<body::Atom,body::bones> local_{};
    std::array<body::Matrix64,body::bones> space_{};
    std::array<body::rig::ReferenceRecord68,body::bones> reference_{};
public:
    void revoke() noexcept {active_=false;}
    bool active()const noexcept{return active_;}
    api::Frame const& frame()const noexcept{return frame_;}
    template<class Clock>bool publish(native::Backend& backend,tx::Kernel const& kernel,tx::Stamp const& stamp,
        gears_troika_driver::Snapshot const& input,gears_weapon_hand::ObjectIdentity asset,
        body::Model const& model,body::Frame const& source,body::Output const& expected,
        body::Pose const(&targets)[2],unsigned solved,unsigned projected,
        std::uint64_t tick,std::uint64_t compose,Clock&& clock,native::ConsumedPose& consumed) noexcept {
        revoke();api::Request query{};query.source=input.source;query.control=input.control;const auto now=clock();
        if(sequence_==UINT64_MAX||(frame_.receipt_sequence&&gears_rig_tracking::same(frame_.source.frame.native,input.source.frame.native)&&
           (tick<=frame_.tick||compose<=frame_.compose))||!api::request_valid(query,now)||!gears_troika_driver::valid(input,now)||solved!=3||projected||
           GetCurrentThreadId()!=input.source.frame.native.game_thread||stamp.binding.owner!=input.source.frame.driver_mesh.address||
           stamp.binding.model!=asset.address||stamp.binding.session!=input.source.frame.native.native_started_qpc||
           stamp.binding.world_generation!=input.source.frame.world_generation||
           !tick||!compose||backend.read_consumed_pose(kernel,stamp,kernel.serial(),consumed)!=native::ConsumedPoseCode::verified||
           consumed.metadata.asset!=asset.address||
           !gears_troika_c0_hands::local_pose_matches(expected,consumed.local))return false;
        auto& f=frame_;std::memset(&f,0,sizeof f);f.size=sizeof f;f.abi_version=api::version;f.source=input.source;f.control=input.control;
        f.asset=asset;f.reference_signature=model.signature;f.receipt_sequence=sequence_+1;f.tick=tick;f.compose=compose;f.kernel_serial=kernel.serial();
        f.completed_ns=clock();f.observed_qpc=consumed.observed_qpc;f.flags=api::verified_flags;f.solved_mask=solved;f.projected_mask=projected;
        f.units_per_metre=source.units_per_metre;f.model_scale=source.model_scale;
        for(unsigned side=0;side<2;++side)f.grip_world[side]=input.mapping.hands.world[side];
        body::ik::Matrix component{};for(unsigned j=0;j<16;++j)component.m[j]=consumed.metadata.component.values[j];
        const auto view=[](body::Pose const& p){return gears_vr::Pose{{p.position.x,p.position.y,p.position.z},{p.orientation.x,p.orientation.y,p.orientation.z,p.orientation.w}};};
        for(unsigned side=0;side<2;++side){const unsigned wrist=side?35u:11u;body::ik::Matrix matrix{};
            for(unsigned j=0;j<16;++j)matrix.m[j]=consumed.space[wrist].values[j];
            f.wrist_world[side]=view(body::d::pose(body::d::multiply(matrix,component)));f.target_world[side]=view(targets[side]);
            for(unsigned n=0;n<15;++n){auto const& q=consumed.local[(side?36u:12u)+n].q;f.fingers[side][n]={q[0],q[1],q[2],q[3]};}
        }
        constexpr unsigned helpers[]={9,29,33,53};for(unsigned n=0;n<4;++n){auto const& q=consumed.local[helpers[n]].q;f.helpers[n]={q[0],q[1],q[2],q[3]};}
        if(!api::valid(f,query,clock()))return false;sequence_=f.receipt_sequence;active_=true;return true;
    }
    template<class Clock,class Current,class Metadata,class Read>bool read(api::Request const& q,Clock&& clock,
        native::ConsumedPose const& expected,body::Frame const& original,Current&& current,Metadata&& metadata,Read&& copy,api::Frame& out) noexcept {
        if(!active_||!api::valid(frame_,q,clock())||!current())return false;
        pose::Metadata before{},after{};
        if(!metadata(before)||!pose::metadata_equal(before,expected.metadata))return false;
        const auto payload=[&](){return copy(local_.data(),before.local.data,sizeof local_)&&
            copy(space_.data(),before.space.data,sizeof space_)&&copy(reference_.data(),before.reference.data,sizeof reference_)&&
            !std::memcmp(local_.data(),expected.local.data(),sizeof local_)&&!std::memcmp(space_.data(),expected.space.data(),sizeof space_)&&
            !std::memcmp(reference_.data(),original.reference.data(),sizeof reference_);};
        if(!payload()||!current()||!payload()||!metadata(after)||!pose::metadata_equal(after,before)||!active_||!current()||!api::valid(frame_,q,clock()))return false;
        out=frame_;return true; // completed_ns/observed_qpc retain their original values.
    }
};
} // namespace gears_troika_c0_readback
