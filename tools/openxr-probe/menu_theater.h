#pragma once
#include <cmath>
#include <cstdint>

// A menu opens once in LOCAL space. Later head motion only changes the view
// of this panel; it never moves the panel itself. New menu/space epochs place
// it in front of the viewer again, without touching gameplay eye poses.
class MenuTheater {
    bool placed_{};
    std::uint64_t space_epoch_{},menu_epoch_{};
    XrPosef pose_{};
public:
    bool place(XrCompositionLayerQuad& quad,XrSpace local,XrSpaceLocation const& head,
               std::uint64_t space_epoch,std::uint64_t menu_epoch) {
        constexpr auto valid=XR_SPACE_LOCATION_POSITION_VALID_BIT|XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;
        if(!placed_||space_epoch_!=space_epoch||menu_epoch_!=menu_epoch){
            if((head.locationFlags&valid)!=valid)return false;
            auto const& q=head.pose.orientation;auto const& p=head.pose.position;
            const double yaw=std::atan2(2.*(q.w*q.y+q.x*q.z),1.-2.*(q.y*q.y+q.x*q.x));
            if(!std::isfinite(yaw)||!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z))return false;
            pose_={};pose_.orientation.y=float(std::sin(yaw*.5));pose_.orientation.w=float(std::cos(yaw*.5));
            pose_.position={p.x-float(2.5*std::sin(yaw)),p.y,p.z-float(2.5*std::cos(yaw))};
            space_epoch_=space_epoch;menu_epoch_=menu_epoch;placed_=true;
        }
        if(quad.size.width<=0||quad.size.height<=0)return false;
        const float aspect=quad.size.height/quad.size.width;
        quad.space=local;quad.pose=pose_;quad.size={3.2f,3.2f*aspect};
        return true;
    }
};
