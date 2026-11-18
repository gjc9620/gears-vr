#pragma once

#include "weapon_boltok_reload.h"

#include "weapon_hammerburst_latch.h"

#include "weapon_reload_geometry.h"

#include "controller_relative_motion.h"



namespace gears_weapon {

inline bool boltok_swing_angle(Quaternion current,Quaternion closed,double& out) noexcept {

    if(!gears_vr::stereo_pose_valid({{},current})||!gears_vr::stereo_pose_valid({{},closed}))return false;

    auto delta=gears_vr::normalized(gears_vr::multiply(current,gears_vr::inverse(closed)));

    if(delta.w<0){delta.x=-delta.x;delta.y=-delta.y;delta.z=-delta.z;delta.w=-delta.w;}

    if(std::hypot(delta.x,delta.z)>.01)return false;

    const auto value=2*std::atan2(delta.y,delta.w);

    if(!std::isfinite(value)||value<-.02||value>1.42)return false;

    out=(std::max)(0.,value);return true;

}

// Reuse the existing angular detector; no authored 80-degree preview, raw

// package units or Snub animation is treated as a measured runtime calibration.

class BoltokSwingStroke {

    HammerburstLatchStroke stroke_;

    BoltokReloadStamp first_{},last_{};

    std::uint64_t token_{};

    std::int64_t max_gap_{};

    unsigned samples_{};

    double endpoint_{};

    bool acknowledged_{};

    Vector last_local_{};

    gears_hand_continuity::TrackedInput motion_source_{};

    gears_vr::Pose motion_root_{};Vector motion_offset_{};

    unsigned motion_control_{};bool motion_bound_{};

    static HammerburstLatchSample sample(BoltokReloadStamp const& s,

            Vector local,std::uint64_t token,bool held) noexcept {

        return {local,s.sequence,std::uint64_t(s.observed_ns/1000000),token,s.left_tracked,held,

            std::uint64_t(s.observed_ns)};

    }

    static bool usable(BoltokReloadStamp const& s,std::int64_t now) noexcept {

        return s.valid(now)&&s.left_tracked&&s.right_tracked&&s.weapon_owned;

    }

public:

    void reset() noexcept {*this=BoltokSwingStroke{};}

    bool begin(HammerburstLatchGeometry const& g,BoltokReloadStamp const& s,

               Vector hand_in_root_metres,std::uint64_t grip_token,bool held,

               double actual_angle,bool closing,std::int64_t now) noexcept {

        reset();

        if(!usable(s,now)||g.observation_interval_ms>100||

           !stroke_.begin(g,sample(s,hand_in_root_metres,grip_token,held),actual_angle))return false;

        first_=last_=s;last_local_=hand_in_root_metres;token_=grip_token;samples_=1;

        endpoint_=closing?0:g.open_angle;return true;

    }

    bool move(BoltokReloadStamp const& s,Vector local,std::uint64_t grip_token,

              bool held,std::int64_t now) noexcept {

        if(motion_bound_||!usable(s,now)||!(s.identity==first_.identity)||s.game_tick<last_.game_tick||

           s.sequence<=last_.sequence||s.observed_ns<=last_.observed_ns||

           s.observed_ns-last_.observed_ns>100000000||grip_token!=token_||acknowledged_||

           stroke_.observe(sample(s,local,grip_token,held))==HammerburstLatchMotion::rejected){

            reset();return false;

        }

        max_gap_=(std::max)(max_gap_,s.observed_ns-last_.observed_ns);last_=s;last_local_=local;++samples_;return true;

    }

    // Bind only at the real begin sample. The adapter must prove a fixed

    // original root/mount and current publisher/Native reference separately.

    // Boltok live-local geometry already includes instance scale in physical

    // metres; relative projection deliberately uses scale1, not clip units.

    // A single registration offset accounts for float world-matrix rounding;

    // it is frozen for the WHOLE stroke, never accumulated per consumer tick.

    bool bind_original_motion(gears_hand_continuity::TrackedInput const& raw,

        gears_vr::Pose const& root_in_right,unsigned control,bool fixed_mount_verified,

        std::int64_t now) noexcept {

        namespace motion=gears_controller_relative_motion;

        if(motion_bound_||!fixed_mount_verified||!stroke_.active()||samples_!=1||acknowledged_||

           !usable(last_,now)||raw.input.sequence!=last_.sequence||raw.input.sync_ns!=last_.observed_ns||

           raw.input.session!=first_.identity.session||raw.input.space_epoch!=first_.identity.space_epoch||

           raw.input.input_epoch!=first_.identity.input_epoch||raw.input.profile_generation!=first_.identity.profile_generation||

           (control!=gears_vr::control_squeeze&&control!=gears_vr::control_trigger)||raw.input.focused!=1||

           !motion::held(raw.input.hands[0],control))return false;

        Vector projected{};if(!gears_vr::stereo_pose_valid(root_in_right))return false;

        auto root=root_in_right;root.orientation=gears_vr::normalized(root.orientation);

        if(!motion::project(raw,root,1,projected))return false;

        const auto offset=difference(last_local_,projected);

        if(length(offset)>.00025)return false;

        motion_source_=raw;motion_root_=root;motion_offset_=offset;

        motion_control_=control;motion_bound_=true;return true;

    }

    bool original_motion_bound() const noexcept {return motion_bound_;}

    // Historical points are fed directly to the existing geometric detector.

    // No old BoltokReloadStamp/Hand/Stereo Frame or old game clock is invented.

    // Current freshness/ownership and actual Native endpoint ACK remain separate.

    struct OriginalMotionRejection {

        enum class Code:unsigned char {

            none,not_bound,mount,stamp,root_distance,root_orientation,build,

            path_start,path_shape,path_identity,count_overflow,local_endpoint,pair,detector

        };

        Code code{Code::none};

        unsigned point_index{};

        unsigned history_count{};

        unsigned path_count{};

        std::uint64_t prior_sequence{},next_sequence{};

        std::int64_t pair_gap_ns{};

        double root_distance_metres{};

        double root_dot{};

        double start_difference_metres{};

        double endpoint_difference_metres{};

        bool detector_rejected{};
        unsigned detector_reason{};
        Vector detector_previous{},detector_next{};

    };

    bool move_original_motion(BoltokReloadStamp const& s,Vector actual_local,

        std::uint64_t grip_token,bool held,std::int64_t now,

        gears_hand_continuity::TrackedInput const& current,

        gears_hand_continuity::MotionHistory const& history,

        gears_vr::Pose const& actual_root_in_right,bool same_fixed_mount_verified,

        OriginalMotionRejection* rejection=nullptr) noexcept {

        namespace motion=gears_controller_relative_motion;

        OriginalMotionRejection diagnostic{};

        if(rejection)*rejection=diagnostic;

        const auto reject=[&](OriginalMotionRejection::Code code,unsigned point=0,

            std::uint64_t prior_sequence=0,std::uint64_t next_sequence=0,

            std::int64_t pair_gap_ns=0,double root_distance=0,double root_dot=0,

            double start_difference=0,double endpoint_difference=0,

            bool detector_rejected=false,unsigned path_count=0){

            diagnostic.code=code;diagnostic.point_index=point;

            diagnostic.history_count=history.count;diagnostic.path_count=path_count;

            diagnostic.prior_sequence=prior_sequence;

            diagnostic.next_sequence=next_sequence;diagnostic.pair_gap_ns=pair_gap_ns;

            diagnostic.root_distance_metres=root_distance;diagnostic.root_dot=root_dot;

            diagnostic.start_difference_metres=start_difference;

            diagnostic.endpoint_difference_metres=endpoint_difference;

            diagnostic.detector_rejected=detector_rejected;

            if(rejection)*rejection=diagnostic;

            reset();return false;

        };

        if(!motion_bound_||!same_fixed_mount_verified)return reject(

            OriginalMotionRejection::Code::not_bound);

        if(!usable(s,now)||!(s.identity==first_.identity)||s.game_tick<last_.game_tick||

           s.sequence<=last_.sequence||s.observed_ns<=last_.observed_ns||

           grip_token!=token_||!held||acknowledged_||current.input.sequence!=s.sequence||

           current.input.sync_ns!=s.observed_ns||!gears_vr::stereo_pose_valid(actual_root_in_right))

            return reject(OriginalMotionRejection::Code::stamp);

        const auto root_distance=length(difference(actual_root_in_right.position,motion_root_.position));

        if(root_distance>.0001)return reject(OriginalMotionRejection::Code::root_distance,

            0,0,0,0,root_distance);

        const auto a=gears_vr::normalized(actual_root_in_right.orientation),b=motion_root_.orientation;

        const auto root_dot=std::abs(a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w);

        if(root_dot<std::cos(.0002*.5))return reject(

            OriginalMotionRejection::Code::root_orientation,0,0,0,0,root_distance,root_dot);

        motion::Segment path{};

        const bool built=motion::build(motion_source_,current,history,motion_control_,motion_root_,1,path);

        if(!built)return reject(OriginalMotionRejection::Code::build);

        if(path.count<2||path.count>path.points.size())

            return reject(OriginalMotionRejection::Code::path_start,0,0,0,0,

                root_distance,root_dot,0,0,false,path.count);

        if(path.points[0].sequence!=last_.sequence||path.points[0].sync_ns!=last_.observed_ns)

            return reject(OriginalMotionRejection::Code::path_identity,0,0,0,0,

                root_distance,root_dot,0,0,false,path.count);

        if(samples_>(std::numeric_limits<unsigned>::max)()-(path.count-1))

            return reject(OriginalMotionRejection::Code::count_overflow,0,0,0,0,

                root_distance,root_dot,0,0,false,path.count);

        for(unsigned i=0;i<path.count;++i)path.points[i].position=sum(path.points[i].position,motion_offset_);

        const auto start_difference=length(difference(path.points[0].position,last_local_));

        const auto endpoint_difference=gears_vr::finite(actual_local)?

            length(difference(path.points[path.count-1].position,actual_local)):

            (std::numeric_limits<double>::infinity)();

        if(start_difference>1e-9||!gears_vr::finite(actual_local)||endpoint_difference>.00025)

            return reject(OriginalMotionRejection::Code::local_endpoint,0,0,0,0,

                root_distance,root_dot,start_difference,endpoint_difference,false,path.count);

        auto candidate=stroke_;auto gap=max_gap_;

        for(unsigned i=1;i<path.count;++i){const auto& p=path.points[i];const auto& prior=path.points[i-1];

            const auto pair_gap=p.sync_ns-prior.sync_ns;

            if(p.sync_ns<=prior.sync_ns||pair_gap>100000000)

                return reject(OriginalMotionRejection::Code::pair,i,prior.sequence,p.sequence,pair_gap,

                    root_distance,root_dot,start_difference,endpoint_difference,false,path.count);

            if(candidate.observe({p.position,p.sequence,std::uint64_t(p.sync_ns/1000000),grip_token,true,true,

                    std::uint64_t(p.sync_ns)})==

                    HammerburstLatchMotion::rejected)

                {
                diagnostic.detector_reason=unsigned(candidate.last_rejection());
                diagnostic.detector_previous=prior.position;diagnostic.detector_next=p.position;
                return reject(OriginalMotionRejection::Code::detector,i,prior.sequence,p.sequence,pair_gap,

                    root_distance,root_dot,start_difference,endpoint_difference,true,path.count);
                }

            gap=(std::max)(gap,p.sync_ns-prior.sync_ns);

        }

        stroke_=candidate;last_=s;last_local_=path.points[path.count-1].position;

        motion_source_=current;max_gap_=gap;samples_+=path.count-1;return true;

    }

    double wanted_angle() const noexcept {return stroke_.angle();}

    // The current component's measured angle must agree with the endpoint;

    // arriving with the hand alone cannot produce a completed motion receipt.

    bool acknowledge(BoltokReloadStamp const& s,double measured_angle,std::int64_t now) noexcept {

        if(!stroke_.active()||samples_<2||!usable(s,now)||!(s.identity==last_.identity)||

           s.sequence!=last_.sequence||s.observed_ns!=last_.observed_ns||s.game_tick!=last_.game_tick||

           !std::isfinite(measured_angle)||std::abs(measured_angle-endpoint_)>.02||

           std::abs(stroke_.angle()-endpoint_)>.02)return false;

        acknowledged_=true;return true;

    }

    bool receipt(BoltokReloadPlan const& plan,BoltokReloadStep const& step,

                 BoltokMotionReceipt& out) const noexcept {

        if(!acknowledged_||!(plan.identity==first_.identity)||!plan.generation||

           !plan.original_reload_serial||step.kind!=BoltokStepKind::move_original_parts||

           !step.parts||!step.phase_id||!step.contact_id||!step.travel_id||!step.before_state||

           !step.after_state||step.before_state==step.after_state)return false;

        out={plan.original_reload_serial,plan.generation,step.phase_id,step.contact_id,step.travel_id,

            step.before_state,step.after_state,first_.sequence,step.parts,first_.observed_ns,

            max_gap_,samples_,true,true,true,true,true,true};return true;

    }

};



struct BoltokChamberGeometry {

    std::uint64_t calibration{};

    std::array<FeedPortMount,6> ports{}; // Metres in current cylinder-local space.

    bool current_instance_verified{};

    bool valid() const noexcept {

        if(!calibration||!current_instance_verified)return false;

        for(unsigned i=0;i<6;++i){

            const auto& p=ports[i];if(!p.valid()||p.maximum_sample_gap_ns>100000000)return false;

            for(unsigned j=0;j<i;++j){

                const auto& q=ports[j];

                // Reject overlapping capture volumes instead of letting the

                // generic magazine tolerance accept two adjacent small holes.

                const auto offset=difference(p.port_in_weapon.position,q.port_in_weapon.position);

                const auto lateral=difference(offset,scaled(p.outward_axis_in_weapon,dot(offset,p.outward_axis_in_weapon)));

                if(length(lateral)<=p.lateral_tolerance_metres+q.lateral_tolerance_metres)return false;

                if(length(difference(p.outward_axis_in_weapon,q.outward_axis_in_weapon))>1e-6)return false;

                if(length(difference(p.item_in_left_grip.position,q.item_in_left_grip.position))>1e-9||

                   std::abs(std::abs(p.item_in_left_grip.orientation.x*q.item_in_left_grip.orientation.x+

                       p.item_in_left_grip.orientation.y*q.item_in_left_grip.orientation.y+

                       p.item_in_left_grip.orientation.z*q.item_in_left_grip.orientation.z+

                       p.item_in_left_grip.orientation.w*q.item_in_left_grip.orientation.w)-1)>1e-9)return false;

            }

        }return true;

    }

};

class BoltokChamberInsertion {

    BoltokChamberGeometry geometry_{};

    BoltokReloadIdentity identity_{};

    FeedInsertionStroke stroke_;

    unsigned occupied_{},slot_{6};

    std::uint64_t item_{};

public:

    // Original producer geometry in the current cylinder/face frame. The tip

    // already includes the item mount; the orientation is that same item's.

    struct OriginalPathPoint {

        std::uint64_t sequence{};

        std::int64_t monotonic_ns{};

        Vector tip_in_face_metres{};

        Quaternion item_orientation_in_face{};

    };

    void reset() noexcept {*this=BoltokChamberInsertion{};}

    bool arm(BoltokReloadIdentity const& id,std::uint64_t item,unsigned occupied,

             BoltokChamberGeometry const& geometry) noexcept {

        reset();if(!id.valid()||!item||occupied>63||!geometry.valid())return false;

        identity_=id;item_=item;occupied_=occupied;geometry_=geometry;return true;

    }

    FeedResult observe(BoltokReloadStamp const& s,FeedSample const& sample,std::int64_t now) noexcept {

        if(!item_||!s.valid(now)||!(s.identity==identity_)||!s.left_tracked||!s.right_tracked||!s.weapon_owned||

           sample.sequence!=s.sequence||sample.monotonic_ns!=s.observed_ns||sample.weapon!=identity_.weapon||

           sample.world!=identity_.world||sample.space_epoch!=identity_.space_epoch||sample.item!=item_||

           !sample.tracked||!sample.item_held||!gears_vr::stereo_pose_valid(sample.weapon_pose)||

           !gears_vr::stereo_pose_valid(sample.left_grip)){reset();return FeedResult::rejected;}

        if(slot_==6){

            unsigned nearest=6;double best=1e9;

            const auto inverse=gears_vr::inverse(sample.weapon_pose.orientation);

            for(unsigned i=0;i<6;++i){

                const auto& p=geometry_.ports[i];

                const auto tip=sum(sample.left_grip.position,rotate(sample.left_grip.orientation,p.item_in_left_grip.position));

                const auto local=rotate(inverse,difference(tip,sample.weapon_pose.position));

                const auto offset=difference(local,p.port_in_weapon.position);

                const auto lateral=length(difference(offset,scaled(p.outward_axis_in_weapon,dot(offset,p.outward_axis_in_weapon))));

                if(lateral<=p.lateral_tolerance_metres&&lateral<best){best=lateral;nearest=i;}

            }

            // Select among ALL holes first. An occupied nearest hole cannot

            // fall through to a different empty chamber and transfer ammo.

            if(nearest==6||(occupied_&(1u<<nearest)))return FeedResult::rejected;

            slot_=nearest;if(!stroke_.arm(identity_.weapon,item_,identity_.world,identity_.space_epoch)){

                reset();return FeedResult::rejected;

            }

        }

        const auto& p=geometry_.ports[slot_];

        const auto tip=sum(sample.left_grip.position,rotate(sample.left_grip.orientation,p.item_in_left_grip.position));

        const auto local=rotate(gears_vr::inverse(sample.weapon_pose.orientation),difference(tip,sample.weapon_pose.position));

        if(dot(difference(local,p.port_in_weapon.position),p.outward_axis_in_weapon)<-p.seated_depth_metres){

            reset();return FeedResult::rejected;

        }

        const auto result=stroke_.observe(sample,p);

        if(result==FeedResult::rejected){

            // Discard the failed path, not the current authenticated held

            // item. A new approach must be observed; a seated sample alone

            // can never resume a path across a gap or misalignment.

            stroke_.reset();slot_=6;

        }

        // Keep completed slot for the inventory request, but prevent a second

        // insertion result without a fresh item/occupied-mask arm.

        if(result==FeedResult::inserted)item_=0;

        return result;

    }

    // Geometry data only. The caller must authenticate the complete original
    // producer path and current lease/actual endpoint before setting verified.
    FeedResult observe_original_geometry_path(BoltokReloadStamp const& current,
        std::int64_t now,std::uint64_t expected_item,OriginalPathPoint const* points,
        unsigned count,bool original_path_verified=false) noexcept {
        const auto discard_path=[&](){stroke_.reset();slot_=6;return FeedResult::rejected;};
        if(!item_||!current.valid(now)||!(current.identity==identity_)||!current.left_tracked||
           !current.right_tracked||!current.weapon_owned||expected_item!=item_){reset();return FeedResult::rejected;}
        if(!original_path_verified||!points||!count||count>gears_hand_continuity::motion_history_capacity)return discard_path();
        std::uint64_t prior_sequence{};std::int64_t prior_ns{};
        for(unsigned i=0;i<count;++i){const auto& p=points[i];
            if(!p.sequence||p.monotonic_ns<=0||!gears_vr::finite(p.tip_in_face_metres)||
               !gears_vr::stereo_pose_valid({{},p.item_orientation_in_face})||
               (i&&(p.sequence<=prior_sequence||p.monotonic_ns<=prior_ns||p.monotonic_ns-prior_ns>100000000))||
               p.sequence>current.sequence||p.monotonic_ns>current.observed_ns)return discard_path();
            prior_sequence=p.sequence;prior_ns=p.monotonic_ns;
        }
        const auto& last=points[count-1];
        if(last.sequence!=current.sequence||last.monotonic_ns!=current.observed_ns)return discard_path();
        auto candidate=*this;auto last_result=FeedResult::rejected;
        for(unsigned i=0;i<count;++i){const auto& point=points[i];
            if(candidate.slot_==6){
                unsigned nearest=6;double best=1e9;
                for(unsigned n=0;n<6;++n){const auto& p=candidate.geometry_.ports[n];
                    const auto offset=difference(point.tip_in_face_metres,p.port_in_weapon.position);
                    const auto lateral=length(difference(offset,scaled(p.outward_axis_in_weapon,dot(offset,p.outward_axis_in_weapon))));
                    if(lateral<=p.lateral_tolerance_metres&&lateral<best){best=lateral;nearest=n;}
                }
                if(nearest==6||(candidate.occupied_&(1u<<nearest))){last_result=FeedResult::rejected;continue;}
                candidate.slot_=nearest;
                if(!candidate.stroke_.arm(candidate.identity_.weapon,candidate.item_,candidate.identity_.world,candidate.identity_.space_epoch)){
                    candidate.reset();*this=candidate;return FeedResult::rejected;
                }
            }
            const auto& p=candidate.geometry_.ports[candidate.slot_];
            const auto offset=difference(point.tip_in_face_metres,p.port_in_weapon.position);
            const auto depth=dot(offset,p.outward_axis_in_weapon);
            const auto lateral=length(difference(offset,scaled(p.outward_axis_in_weapon,depth)));
            if(depth<-p.seated_depth_metres){candidate.reset();*this=candidate;return FeedResult::rejected;}
            const auto q=gears_vr::normalized(p.port_in_weapon.orientation);
            const auto r=gears_vr::normalized(point.item_orientation_in_face);
            FeedGeometrySample sample{};sample.weapon=candidate.identity_.weapon;sample.item=candidate.item_;
            sample.world=candidate.identity_.world;sample.space_epoch=candidate.identity_.space_epoch;
            sample.sequence=point.sequence;sample.monotonic_ns=point.monotonic_ns;sample.tracked=sample.item_held=true;
            sample.depth_metres=depth;sample.lateral_metres=lateral;
            sample.orientation_dot=std::abs(q.x*r.x+q.y*r.y+q.z*r.z+q.w*r.w);
            last_result=candidate.stroke_.observe_geometry(sample,p);
            if(last_result==FeedResult::rejected){candidate.stroke_.reset();candidate.slot_=6;}
            else if(last_result==FeedResult::inserted){candidate.item_=0;*this=candidate;return last_result;}
        }
        *this=candidate;return last_result;
    }
    unsigned slot() const noexcept {return slot_;}

};

}

