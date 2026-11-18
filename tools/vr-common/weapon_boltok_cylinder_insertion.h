#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "weapon_boltok_mechanics.h"
#include "controller_relative_motion.h"

namespace gears_weapon {

class BoltokCylinderInsertion {
public:
    using Point=BoltokChamberInsertion::OriginalPathPoint;

    void reset() noexcept {
        identity_={};
        item_=0;
        mount_={};
        stroke_.reset();
    }

    bool armed() const noexcept {return item_!=0;}

    bool arm(BoltokReloadIdentity const& identity,std::uint64_t item,
        FeedPortMount const& mount) noexcept {
        reset();
        if(!identity.valid()||!item||!mount.valid())
            return false;
        identity_=identity;
        item_=item;
        mount_=mount;
        if(!stroke_.arm(identity_.weapon,item_,identity_.world,
               identity_.space_epoch)){
            item_=0;
            return false;
        }
        return true;
    }

    FeedResult observe_original_geometry_path(BoltokReloadStamp const& stamp,
        std::int64_t now,std::uint64_t expected_item,Point const* points,
        std::size_t count,bool original_path_verified=false) noexcept {
        const auto discard_path=[&]() noexcept {
            stroke_.reset();
            if(identity_.valid()&&item_&&mount_.valid())
                (void)stroke_.arm(identity_.weapon,item_,identity_.world,
                    identity_.space_epoch);
            return FeedResult::rejected;
        };

        if(!identity_.valid()||!item_||!mount_.valid())
            return FeedResult::rejected;
        if(expected_item!=item_||!(stamp.identity==identity_)){
            reset();return FeedResult::rejected;
        }
        if(!original_path_verified||!points||!count||
           count>gears_hand_continuity::motion_history_capacity)
            return discard_path();

        // Invalid/replaced current authentication resets the whole detector,
        // not merely the approach progress.
        constexpr std::int64_t maximum_gap_ns=100000000;
        if(!stamp.left_tracked||!stamp.right_tracked||
           !stamp.weapon_owned||!stamp.valid(now)){
            reset();
            return FeedResult::rejected;
        }
        const auto observed=stamp.observed_ns;
        if(observed<=0||observed>now||now-observed>maximum_gap_ns){
            reset();
            return FeedResult::rejected;
        }

        // Validate the complete real history before any detector mutation.
        for(std::size_t i=0;i<count;++i){
            if(!points[i].sequence||points[i].monotonic_ns<=0||
               points[i].monotonic_ns>observed||
               points[i].sequence>stamp.sequence||
               !std::isfinite(points[i].tip_in_face_metres.x)||
               !std::isfinite(points[i].tip_in_face_metres.y)||
               !std::isfinite(points[i].tip_in_face_metres.z)||
               !gears_vr::stereo_pose_valid(
                   {{},points[i].item_orientation_in_face}))
                return discard_path();
            if(i){
                if(points[i].sequence<=points[i-1].sequence||
                   points[i].monotonic_ns<=points[i-1].monotonic_ns||
                   points[i].monotonic_ns-points[i-1].monotonic_ns>
                       maximum_gap_ns)
                    return discard_path();
            }
        }
        const auto& last=points[count-1];
        if(last.sequence!=stamp.sequence||last.monotonic_ns!=observed)
            return discard_path();

        // Process only a copy. One rejected real step discards partial progress
        // but preserves the owned item and requires a NEW approach.
        BoltokCylinderInsertion candidate=*this;
        const auto& port=mount_.port_in_weapon;
        if(!gears_vr::stereo_pose_valid(port)){
            reset();
            return FeedResult::rejected;
        }

        auto last_result=FeedResult::rejected;
        for(std::size_t i=0;i<count;++i){
            const auto& point=points[i];
            const Vector offset=gears_weapon::difference(
                point.tip_in_face_metres,port.position);
            const double depth=gears_weapon::dot(
                offset,mount_.outward_axis_in_weapon);
            const double lateral=gears_weapon::length(
                gears_weapon::difference(offset,
                    gears_weapon::scaled(mount_.outward_axis_in_weapon,depth)));
            const auto& target=port.orientation;
            const auto& actual=point.item_orientation_in_face;
            const double orientation_dot=std::abs(
                double(target.x)*actual.x+double(target.y)*actual.y+
                double(target.z)*actual.z+double(target.w)*actual.w);
            if(!std::isfinite(depth)||!std::isfinite(lateral)||
               !std::isfinite(orientation_dot)){
                stroke_.reset();
                (void)stroke_.arm(identity_.weapon,item_,identity_.world,
                    identity_.space_epoch);
                return FeedResult::rejected;
            }

            if(depth < -mount_.seated_depth_metres){
                stroke_.reset();
                (void)stroke_.arm(identity_.weapon,item_,identity_.world,
                    identity_.space_epoch);
                return FeedResult::rejected;
            }

            FeedGeometrySample sample{};
            sample.weapon=identity_.weapon;
            sample.item=item_;
            sample.world=identity_.world;
            sample.space_epoch=identity_.space_epoch;
            sample.sequence=point.sequence;
            sample.monotonic_ns=point.monotonic_ns;
            sample.tracked=true;
            sample.item_held=true;
            sample.depth_metres=depth;
            sample.lateral_metres=lateral;
            sample.orientation_dot=orientation_dot;

            const auto result=candidate.stroke_.observe_geometry(sample,mount_);
            last_result=result;
            if(result==FeedResult::rejected){
                candidate.stroke_.reset();
                (void)candidate.stroke_.arm(identity_.weapon,item_,identity_.world,
                    identity_.space_epoch);
                continue; // Later real points must establish a NEW approach.
            }
            if(result==FeedResult::inserted){
                // A historical crossing alone cannot commit after the actual
                // current endpoint has already withdrawn from the socket.
                const auto final_offset=difference(last.tip_in_face_metres,port.position);
                const auto final_depth=dot(final_offset,mount_.outward_axis_in_weapon);
                const auto final_lateral=length(difference(final_offset,scaled(mount_.outward_axis_in_weapon,final_depth)));
                const auto& final_q=last.item_orientation_in_face;
                const double final_dot=std::abs(target.x*final_q.x+target.y*final_q.y+target.z*final_q.z+target.w*final_q.w);
                if(final_depth < -mount_.seated_depth_metres||final_depth>mount_.seated_depth_metres||
                   final_lateral>mount_.lateral_tolerance_metres||
                   final_dot<std::cos(mount_.maximum_orientation_error_radians*.5))return discard_path();
                item_=0;
                stroke_.reset();
                return FeedResult::inserted;
            }
            if(result!=FeedResult::approaching){
                stroke_.reset();
                (void)stroke_.arm(identity_.weapon,item_,identity_.world,
                    identity_.space_epoch);
                return FeedResult::rejected;
            }
        }

        stroke_=candidate.stroke_;
        return last_result;
    }

private:
    BoltokReloadIdentity identity_{};
    std::uint64_t item_{};
    FeedPortMount mount_{};
    FeedInsertionStroke stroke_{};
};

} // namespace gears_weapon
