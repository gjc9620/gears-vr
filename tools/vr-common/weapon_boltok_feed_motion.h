#pragma once
#include "weapon_boltok_mechanics.h"

// Original producer data -> fixed-face geometry. No world/Hand Frame, live
// stamp, lease, clock renewal or inventory action is produced by this class.
namespace gears_boltok_feed_motion {
namespace source=gears_hand_continuity;
namespace relative=gears_controller_relative_motion;
using P=gears_vr::Pose;using V=gears_vr::Vector;using Q=gears_vr::Quaternion;
using Point=gears_weapon::BoltokChamberInsertion::OriginalPathPoint;
class Builder {
public:
    enum class Failure:unsigned {none,registration,source,publisher,item,pose,history,start,orientation,endpoint,count};
    struct Path {std::array<Point,source::motion_history_capacity> points{};unsigned count{};};
private:
    source::TrackedInput last_{};source::Publisher publisher_{};
    P face_{},mount_{};V offset_{},last_tip_{};
    std::uint64_t item_{};unsigned control_{};bool bound_{};Failure failure_{};
    bool fail(Failure why) noexcept {reset();failure_=why;return false;}
    static bool orientation_close(Q a,Q b) noexcept {
        a=gears_vr::normalized(a);b=gears_vr::normalized(b);
        return std::abs(a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w)>=std::cos(.0002*.5);
    }
    using M=decltype(gears_vr::rotation_matrix(Q{}));
    static M multiply(M const& a,M const& b) noexcept {
        M out{};for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c)
            for(unsigned k=0;k<3;++k)out[r][c]+=a[r][k]*b[k][c];
        return out;
    }
public:
    void reset() noexcept {last_={};publisher_={};face_=mount_={};offset_=last_tip_={};item_=0;control_=0;bound_=false;failure_=Failure::none;}
    bool bound() const noexcept {return bound_;}
    Failure failure() const noexcept {return failure_;}
    static bool current(source::TrackedInput const& raw,source::Publisher publisher,unsigned control,std::uint64_t item,std::int64_t now) noexcept {
        return publisher.pid&&publisher.start&&item&&source::tracked_input_shape_valid(raw)&&raw.input.focused==1&&
            raw.input.sync_ns>0&&now>=raw.input.sync_ns&&now-raw.input.sync_ns<=100000000&&
            (control==gears_vr::control_squeeze||control==gears_vr::control_trigger)&&relative::held(raw.input.hands[0],control);
    }
    static bool attached_tip(source::TrackedInput const& raw,P const& face,P const& mount,V& out) noexcept {
        return gears_vr::stereo_pose_valid(mount)&&relative::project_attached(raw,face,1,mount.position,out);
    }
    static bool attached_orientation(source::TrackedInput const& raw,P const& face,P const& mount,Q& out) noexcept {
        V unused{};if(!attached_tip(raw,face,mount,unused))return false;
        const auto relative_q=gears_vr::multiply(gears_vr::inverse(raw.input.hands[1].grip.orientation),raw.input.hands[0].grip.orientation);
        const auto relative_basis=gears_vr::xr_to_ue(gears_vr::rotation_matrix(relative_q));
        const auto local=multiply(gears_vr::rotation_matrix(gears_vr::inverse(face.orientation)),
            multiply(relative_basis,gears_vr::rotation_matrix(mount.orientation)));
        if(!gears_vr::rig_tracking::detail::quaternion(local,out))return false;
        out=gears_vr::normalized(out);return gears_vr::stereo_pose_valid({{},out});
    }
    bool bind(source::TrackedInput const& raw,source::Publisher publisher,unsigned control,std::uint64_t item,
        P const& face,P const& mount,V actual_tip,Q actual_orientation,std::int64_t now,bool fixed_verified,Path& out) noexcept {
        reset();out={};if(!fixed_verified)return fail(Failure::registration);
        if(!current(raw,publisher,control,item,now))return fail(Failure::source);
        if(!gears_vr::stereo_pose_valid(face)||!gears_vr::stereo_pose_valid(mount)||!gears_vr::finite(actual_tip)||
           !gears_vr::stereo_pose_valid({{},actual_orientation}))return fail(Failure::pose);
        face_=face;face_.orientation=gears_vr::normalized(face.orientation);mount_=mount;mount_.orientation=gears_vr::normalized(mount.orientation);
        V projected{};Q orientation{};
        if(!attached_tip(raw,face_,mount_,projected)||!attached_orientation(raw,face_,mount_,orientation))return fail(Failure::pose);
        const auto offset=gears_weapon::difference(actual_tip,projected);
        if(gears_weapon::length(offset)>.00025)return fail(Failure::registration);
        if(!orientation_close(orientation,actual_orientation))return fail(Failure::orientation);
        offset_=offset;last_tip_=gears_weapon::sum(projected,offset_);last_=raw;publisher_=publisher;item_=item;control_=control;bound_=true;
        out.points[0]={raw.input.sequence,raw.input.sync_ns,last_tip_,orientation};out.count=1;return true;
    }
    bool advance(source::TrackedInput const& raw,source::Publisher publisher,source::MotionHistory const& history,
        std::uint64_t item,P const& face,V actual_tip,Q actual_orientation,std::int64_t now,bool fixed_verified,Path& out) noexcept {
        out={};if(!bound_||!fixed_verified)return fail(Failure::registration);
        if(item!=item_)return fail(Failure::item);
        if(publisher.pid!=publisher_.pid||publisher.start!=publisher_.start)return fail(Failure::publisher);
        if(!current(raw,publisher,control_,item,now))return fail(Failure::source);
        if(!gears_vr::stereo_pose_valid(face)||!gears_vr::finite(actual_tip)||!gears_vr::stereo_pose_valid({{},actual_orientation}))return fail(Failure::pose);
        if(gears_weapon::length(gears_weapon::difference(face.position,face_.position))>.0001||
           !orientation_close(face.orientation,face_.orientation))return fail(Failure::registration);
        relative::Segment segment{};
        if(!relative::build_attached(last_,raw,history,control_,face_,1,mount_.position,segment))return fail(Failure::history);
        if(segment.count<2||segment.count>out.points.size()+1)return fail(Failure::count);
        if(segment.points[0].sequence!=last_.input.sequence||segment.points[0].sync_ns!=last_.input.sync_ns||
           gears_weapon::length(gears_weapon::difference(gears_weapon::sum(segment.points[0].position,offset_),last_tip_))>1e-9)return fail(Failure::start);
        Path candidate{};
        for(unsigned i=1;i<segment.count;++i){
            source::TrackedInput const* producer_sample=nullptr;
            for(unsigned n=0;n<history.count;++n)if(history.frames[n].input.sequence==segment.points[i].sequence){producer_sample=&history.frames[n];break;}
            Q orientation{};
            if(!producer_sample||!attached_orientation(*producer_sample,face_,mount_,orientation))return fail(Failure::orientation);
            candidate.points[candidate.count++]={segment.points[i].sequence,segment.points[i].sync_ns,
                gears_weapon::sum(segment.points[i].position,offset_),orientation};
        }
        const auto& final=candidate.points[candidate.count-1];
        if(final.sequence!=raw.input.sequence||final.monotonic_ns!=raw.input.sync_ns||
           gears_weapon::length(gears_weapon::difference(final.tip_in_face_metres,actual_tip))>.00025)return fail(Failure::endpoint);
        if(!orientation_close(final.item_orientation_in_face,actual_orientation))return fail(Failure::orientation);
        last_=raw;last_tip_=final.tip_in_face_metres;out=candidate;return true;
    }
};
}
