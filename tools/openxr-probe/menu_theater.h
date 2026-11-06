#pragma once
#include <cmath>
#include <cstdint>
#include <array>
#include <initializer_list>

// A menu opens once in LOCAL space. Later head motion only changes the view
// of this panel; it never moves the panel itself. New menu/space epochs place
// it in front of the viewer again, without touching gameplay eye poses.
class MenuTheater {
    bool placed_{};
    std::uint64_t space_epoch_{},menu_epoch_{};
    XrPosef pose_{};
    float width_{},height_{},aspect_{};
    using V=std::array<double,3>;
    using Q=std::array<double,4>;
    static V rotate(Q const& q,V const& v){
        const V t{2*(q[1]*v[2]-q[2]*v[1]),2*(q[2]*v[0]-q[0]*v[2]),2*(q[0]*v[1]-q[1]*v[0])};
        return {v[0]+q[3]*t[0]+q[1]*t[2]-q[2]*t[1],
                v[1]+q[3]*t[1]+q[2]*t[0]-q[0]*t[2],
                v[2]+q[3]*t[2]+q[0]*t[1]-q[1]*t[0]};
    }
    static bool fit(XrPosef const& panel,float aspect,XrView const* views,std::uint32_t count,float& width){
        if(!views||count!=2)return false;
        struct Eye {V position;Q inverse;std::array<double,4> bounds;};
        std::array<Eye,2> eyes{};
        for(unsigned i=0;i<2;++i){const auto& view=views[i];auto& e=eyes[i];const auto& q=view.pose.orientation;
            const double norm=double(q.x)*q.x+double(q.y)*q.y+double(q.z)*q.z+double(q.w)*q.w;
            if(!std::isfinite(norm)||norm<.98||norm>1.02)return false;
            const double scale=1/std::sqrt(norm);e.inverse={-q.x*scale,-q.y*scale,-q.z*scale,q.w*scale};
            e.position={view.pose.position.x,view.pose.position.y,view.pose.position.z};
            for(double v:e.position)if(!std::isfinite(v))return false;
            const auto& f=view.fov;e.bounds={f.angleLeft,f.angleRight,f.angleDown,f.angleUp};
            for(unsigned k=0;k<4;++k){double& a=e.bounds[k];
                if(!std::isfinite(a)||std::abs(a)>=1.55||((k==0||k==2)?a>=0:a<=0))return false;
                a=std::tan(a)*.88;}
        }
        const auto& q=panel.orientation;const Q orientation{q.x,q.y,q.z,q.w};
        const auto inside=[&](double w){
            for(int x:{-1,1})for(int y:{-1,1}){
                auto corner=rotate(orientation,{x*w*.5,y*w*aspect*.5,0});
                corner[0]+=panel.position.x;corner[1]+=panel.position.y;corner[2]+=panel.position.z;
                for(auto const& eye:eyes){const auto local=rotate(eye.inverse,{corner[0]-eye.position[0],corner[1]-eye.position[1],corner[2]-eye.position[2]});
                    const double depth=-local[2];if(!std::isfinite(depth)||depth<=.01)return false;
                    const double u=local[0]/depth,v=local[1]/depth;
                    if(!std::isfinite(u)||!std::isfinite(v)||u<eye.bounds[0]||u>eye.bounds[1]||v<eye.bounds[2]||v>eye.bounds[3])return false;
                }
            }
            return true;
        };
        if(!inside(0))return false;
        double low=0,high=6.4;
        for(unsigned i=0;i<24;++i){const double middle=(low+high)*.5;if(inside(middle))low=middle;else high=middle;}
        width=float(low*.9999);return std::isfinite(width)&&width>.01f;
    }
public:
    bool place(XrCompositionLayerQuad& quad,XrSpace local,XrSpaceLocation const& head,
               std::uint64_t space_epoch,std::uint64_t menu_epoch,XrView const* views,std::uint32_t view_count) {
        constexpr auto valid=XR_SPACE_LOCATION_POSITION_VALID_BIT|XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;
        if(!std::isfinite(quad.size.width)||!std::isfinite(quad.size.height)||quad.size.width<=0||quad.size.height<=0)return false;
        const float aspect=quad.size.height/quad.size.width;
        const bool new_placement=!placed_||space_epoch_!=space_epoch||menu_epoch_!=menu_epoch;
        if(!std::isfinite(aspect)||aspect<=0)return false;
        XrPosef desired=pose_;
        if(new_placement){
            if((head.locationFlags&valid)!=valid)return false;
            auto const& q=head.pose.orientation;auto const& p=head.pose.position;
            const double yaw=std::atan2(2.*(q.w*q.y+q.x*q.z),1.-2.*(q.y*q.y+q.x*q.x));
            if(!std::isfinite(yaw)||!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z))return false;
            desired={};desired.orientation.y=float(std::sin(yaw*.5));desired.orientation.w=float(std::cos(yaw*.5));
            desired.position={p.x-float(2.5*std::sin(yaw)),p.y,p.z-float(2.5*std::cos(yaw))};
        }
        if(new_placement||aspect!=aspect_){
            float fitted{};
            // Test all four screen corners against BOTH actual eye poses/FOVs,
            // including asymmetric or canted displays. Cache the fit so later
            // head motion cannot move or resize this world-fixed screen.
            if(!fit(desired,aspect,views,view_count,fitted))return false;
            width_=fitted;height_=width_*aspect;aspect_=aspect;
        }
        if(new_placement){
            pose_=desired;space_epoch_=space_epoch;menu_epoch_=menu_epoch;placed_=true;
        }
        // The stock pause widgets occupy only the central part of the game
        // canvas. A cinema-size surface keeps that text readable without
        // cropping dialogs or attaching the menu to the viewer's head.
        quad.space=local;quad.pose=pose_;quad.size={width_,height_};
        return true;
    }
};
