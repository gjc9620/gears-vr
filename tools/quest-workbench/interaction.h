#pragma once
#include "core.h"

namespace workbench {
inline V axis_vector(unsigned i){return i==0?V{1,0,0}:i==1?V{0,1,0}:V{0,0,1};}
inline bool plane_hit(P ray,V center,V normal,V& at){auto d=rotate(ray.orientation,{0,0,-1});double den=dot(d,normal);if(std::abs(den)<1e-7)return false;double t=dot(sub(center,ray.position),normal)/den;if(t<0||t>3)return false;at=add(ray.position,mul(d,t));return true;}
inline double slider_value(P ray,P control,double width){V at{};if(width<=0||!plane_hit(ray,control.position,rotate(control.orientation,{0,0,1}),at))return -1;return std::clamp((relative(control,{at,{}}).position.x/width)/.86+.5,0.,1.);}
inline double ray_volume_score(P ray,Volume const& a,P object){
    auto p=relative(compose(object,a.local),ray);auto d=rotate(p.orientation,{0,0,-1});double t=std::clamp(-dot(p.position,d),0.,3.);auto near=add(p.position,mul(d,t));
    double bound=a.shape=="box"?length(a.half):a.radius+(a.shape=="capsule"?a.half_length:0);
    if(t<=0||length(near)>bound)return -1;
    // Prefer the visibly aimed center when acquisition volumes overlap.
    return length(near)/std::max(t,.1);
}
struct RotationDrag {
    bool active{};unsigned pointer{},component{};P frame{};Q original{};V start{};
    bool begin(unsigned id,P ray,P world,Q local,unsigned selected){
        double best=.012;int found=-1;V point{};
        for(unsigned a=0;a<3;++a){V hit{};if(!plane_hit(ray,world.position,rotate(world.orientation,axis_vector(a)),hit))continue;double error=std::abs(length(sub(hit,world.position))-.065);if(error<best){found=int(a);best=error;point=hit;}}
        if(found<0)return false;active=true;pointer=id;component=unsigned(found);frame=world;original=local;start=unit(sub(point,world.position));(void)selected;return true;
    }
    bool update(P ray,Q& local)const{if(!active)return false;V normal=rotate(frame.orientation,axis_vector(component)),hit{};if(!plane_hit(ray,frame.position,normal,hit)||length(sub(hit,frame.position))<.01)return false;auto now=unit(sub(hit,frame.position));double angle=std::atan2(dot(normal,cross(start,now)),dot(start,now));local=v::normalized(v::multiply(original,axis(axis_vector(component),angle)));return true;}
};
inline bool hand_menu_allowed(unsigned hand,unsigned posing,bool enabled,bool system_gesture){return enabled&&!system_gesture&&(posing==2||hand!=posing);}
}
