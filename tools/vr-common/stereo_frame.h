#pragma once
#include "pose_math.h"
#include <cstdint>
#include <optional>

namespace gears_vr {
enum class StereoSource : std::uint32_t {RuntimeTracked=0,MetaOperatorSimulated=1};
inline bool stereo_source_known(StereoSource source) {
    return source==StereoSource::RuntimeTracked||source==StereoSource::MetaOperatorSimulated;
}
inline bool stereo_source_allowed(StereoSource source,bool allow_sim=false) {
    // Opt-in selects a provenance profile; it never silently accepts the other
    // profile. This must agree with the frozen publisher/receiver packet policy.
    return source==(allow_sim?StereoSource::MetaOperatorSimulated:StereoSource::RuntimeTracked);
}
struct StereoFov {double left{},right{},up{},down{};};
struct StereoEye {Pose pose{{},{0,0,0,0}};StereoFov fov{};};
struct StereoSample {
    StereoSource source{StereoSource::RuntimeTracked};
    std::uint64_t session{},sequence{};
    std::int64_t received_ns{},predicted_time{}; // predicted_time is opaque XrTime, not QPC.
    std::uint64_t head_flags{},view_flags{};
    std::uint32_t focused{},should_render{};
    Pose head{{},{0,0,0,0}};
    StereoEye eyes[2]{};
    // Changes only when the runtime's LOCAL origin change takes effect.
    std::uint64_t space_epoch{1};
    std::uint32_t visible{}; // Runtime visibility; focused remains input-focus evidence.
};
inline bool stereo_render_active(StereoSample const& sample) {
    return sample.focused<=1&&sample.visible<=1&&sample.should_render==1&&
        (sample.focused==1||sample.visible==1);
}
inline bool stereo_tracking_valid(StereoSample const& sample,bool allow_sim=false) {
    if(!stereo_source_allowed(sample.source,allow_sim)||(sample.view_flags&15)!=15)return false;
    const std::uint64_t required=sample.source==StereoSource::MetaOperatorSimulated?3:15;
    return (sample.head_flags&required)==required;
}
inline bool stereo_pose_valid(Pose const& p) {
    auto q=p.orientation;
    double norm=q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w;
    return finite(p.position) && std::isfinite(norm) && std::abs(norm-1.)<=.001;
}
// Native camera origin and LOCAL Y-up heading. Never changes the raw XR pose.
// Near vertical forward has no reliable horizontal heading; leave out intact.
inline bool stereo_heading_reference(Pose const& head,Pose& out) {
    if(!stereo_pose_valid(head))return false;
    auto forward=transform(rotation_matrix(head.orientation),{0,0,-1});
    double horizontal=std::hypot(forward.x,forward.z);
    if(!std::isfinite(horizontal)||horizontal<.05)return false;
    double yaw=std::atan2(-forward.x,-forward.z);
    out={head.position,{0,std::sin(yaw/2),0,std::cos(yaw/2)}};
    return true;
}
inline bool stereo_fov_valid(StereoFov const& f) {
    for(double a:{f.left,f.right,f.up,f.down})if(!std::isfinite(a)||std::abs(a)>=1.55)return false;
    return f.right-f.left>.001 && f.up-f.down>.001;
}
inline double stereo_distance(Vector a,Vector b) {return std::hypot(std::hypot(a.x-b.x,a.y-b.y),a.z-b.z);}
inline bool stereo_geometry_valid(StereoSample const& s) {
    if(!stereo_pose_valid(s.head))return false;
    for(auto const& eye:s.eyes)if(!stereo_pose_valid(eye.pose)||!stereo_fov_valid(eye.fov))return false;
    double distance=stereo_distance(s.eyes[0].pose.position,s.eyes[1].pose.position);
    return std::isfinite(distance)&&distance>=.04&&distance<=.085;
}
inline bool stereo_same_pose(Pose const& a,Pose const& b) {
    return a.position.x==b.position.x&&a.position.y==b.position.y&&a.position.z==b.position.z&&
        a.orientation.x==b.orientation.x&&a.orientation.y==b.orientation.y&&
        a.orientation.z==b.orientation.z&&a.orientation.w==b.orientation.w;
}
inline bool stereo_same(StereoSample const& a,StereoSample const& b) {
    if(a.source!=b.source||a.session!=b.session||a.sequence!=b.sequence||a.space_epoch!=b.space_epoch||a.received_ns!=b.received_ns||
       a.predicted_time!=b.predicted_time||a.head_flags!=b.head_flags||a.view_flags!=b.view_flags||
       a.focused!=b.focused||a.visible!=b.visible||a.should_render!=b.should_render||!stereo_same_pose(a.head,b.head))return false;
    for(unsigned i=0;i<2;++i) {
        auto x=a.eyes[i].fov,y=b.eyes[i].fov;
        if(!stereo_same_pose(a.eyes[i].pose,b.eyes[i].pose)||x.left!=y.left||x.right!=y.right||x.up!=y.up||x.down!=y.down)return false;
    }
    return true;
}
// 8/9 are reserved for native bridge-read/matrix diagnostic counters.
enum class StereoStatus {accepted,initial,untracked,inactive,stale,replayed,session_changed,invalid,heading_unavailable=10};
struct StereoResult {
    StereoStatus status{StereoStatus::invalid};StereoSample sample{};Pose reference{};
    std::uint64_t reference_generation{};
    bool recentered{},heading_deferred{};
    explicit operator bool() const {return status==StereoStatus::accepted||status==StereoStatus::initial;}
};
class StereoGate {
    std::optional<StereoSample> last_;
    std::optional<Pose> reference_;
    std::uint64_t reference_generation_{};
    bool recenter_pending_{};
public:
    void reset(){last_.reset();reference_.reset();reference_generation_=0;recenter_pending_=false;}
    void request_recenter(){recenter_pending_=true;}
    StereoResult update(StereoSample const& s,std::int64_t now_ns,bool allow_sim=false) {
        auto reject=[&](StereoStatus status){StereoResult result;result.status=status;
            if(reference_)result.reference=*reference_;
            result.reference_generation=reference_generation_;return result;};
        if(last_&&s.session!=last_->session)return reject(StereoStatus::session_changed);
        if(!stereo_source_allowed(s.source,allow_sim)||(last_&&s.source!=last_->source))return reject(StereoStatus::invalid);
        if(!s.session||!s.sequence||!s.space_epoch)return reject(StereoStatus::invalid);
        if(last_&&s.space_epoch<last_->space_epoch)return reject(StereoStatus::replayed);
        if(last_&&(s.sequence<last_->sequence||(s.sequence==last_->sequence&&!stereo_same(s,*last_))))
            return reject(StereoStatus::replayed);
        if(last_&&(s.received_ns<last_->received_ns||s.predicted_time<last_->predicted_time))
            return reject(StereoStatus::replayed);
        if(!stereo_render_active(s))return reject(StereoStatus::inactive);
        if(!stereo_tracking_valid(s,allow_sim))return reject(StereoStatus::untracked);
        if(now_ns<=0||s.received_ns<=0||s.received_ns>now_ns||now_ns-s.received_ns>100000000)
            return reject(StereoStatus::stale);
        if(!stereo_geometry_valid(s))return reject(StereoStatus::invalid);
        const bool new_space=last_&&s.space_epoch!=last_->space_epoch;
        if(new_space)reference_.reset(); // No old-origin reference survives this boundary.
        bool first=!reference_,recentered=false,deferred=false;
        last_=s; // Even an unavailable initial heading cannot erase replay guards.
        if(first||recenter_pending_){
            Pose heading{};
            if(stereo_heading_reference(s.head,heading)){
                reference_=heading;++reference_generation_;recentered=!first||new_space;
                // A request before initialization is satisfied by generation1.
                recenter_pending_=false;
            }else deferred=true;
        }
        if(!reference_){auto result=reject(StereoStatus::heading_unavailable);
            result.sample=s;result.heading_deferred=true;return result;}
        return {first?StereoStatus::initial:StereoStatus::accepted,s,*reference_,reference_generation_,recentered,deferred};
    }
};
using StereoMatrix=std::array<float,16>;
struct StereoMatrices {StereoMatrix view[2]{},projection[2]{};};
inline bool stereo_rigid(StereoMatrix const& v) {
    for(float x:v)if(!std::isfinite(x))return false;
    if(v[3]!=0||v[7]!=0||v[11]!=0||v[15]!=1)return false;
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j){
        double dot=0;for(unsigned k=0;k<3;++k)dot+=double(v[4*i+k])*v[4*j+k];
        if(std::abs(dot-(i==j?1.:0.))>1e-4)return false;
    }
    double det=double(v[0])*(double(v[5])*v[10]-double(v[6])*v[9])-
        double(v[1])*(double(v[4])*v[10]-double(v[6])*v[8])+
        double(v[2])*(double(v[4])*v[9]-double(v[5])*v[8]);
    return std::abs(det-1.)<=1e-4;
}
inline bool stereo_projection_supported(StereoMatrix const& p) {
    for(float x:p)if(!std::isfinite(x))return false;
    for(unsigned i:{1u,2u,3u,4u,6u,7u,12u,13u,15u})if(p[i]!=0)return false;
    return p[0]>0&&p[5]>0&&p[11]==1&&p[14]!=0;
}
inline Vector stereo_camera_position(StereoMatrix const& v) {
    return {-double(v[12])*v[0]-double(v[13])*v[1]-double(v[14])*v[2],
            -double(v[12])*v[4]-double(v[13])*v[5]-double(v[14])*v[6],
            -double(v[12])*v[8]-double(v[13])*v[9]-double(v[14])*v[10]};
}
// Row-vector native view, +Z forward. No physical scale calibration is implied.
// Commit both eyes only after every validation succeeds; failure leaves out intact.
inline bool stereo_matrices(StereoMatrix const& base_view,StereoMatrix const& base_projection,
    Pose const& reference,StereoSample const& sample,double units_per_metre,StereoMatrices& out,bool allow_sim=false) {
    if(!stereo_source_allowed(sample.source,allow_sim)||!std::isfinite(units_per_metre)||units_per_metre<10||units_per_metre>500||
       !stereo_rigid(base_view)||!stereo_projection_supported(base_projection)||
       !stereo_pose_valid(reference)||!stereo_geometry_valid(sample))return false;
    StereoMatrices result{};
    Matrix ref=transpose(rotation_matrix(reference.orientation));
    double separation=stereo_distance(sample.eyes[0].pose.position,sample.eyes[1].pose.position)*units_per_metre;
    for(unsigned eye=0;eye<2;++eye) {
        auto const& pose=sample.eyes[eye].pose;
        Matrix d=multiply(ref,rotation_matrix(pose.orientation));
        for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)if((i==2)!=(j==2))d[i][j]=-d[i][j];
        Vector delta=transform(ref,{pose.position.x-reference.position.x,pose.position.y-reference.position.y,pose.position.z-reference.position.z});
        double translation[3]={delta.x*units_per_metre,delta.y*units_per_metre,-delta.z*units_per_metre};
        auto& view=result.view[eye];view=base_view;
        for(unsigned i=0;i<4;++i)for(unsigned j=0;j<3;++j){
            double value=0;for(unsigned k=0;k<3;++k)value+=(i==3?double(base_view[12+k])-translation[k]:double(base_view[4*i+k]))*d[k][j];
            view[4*i+j]=static_cast<float>(value);
            if(!std::isfinite(view[4*i+j])||std::abs(double(view[4*i+j])-value)>separation*.001)return false;
        }
        if(!stereo_rigid(view))return false;
        auto f=sample.eyes[eye].fov;double l=std::tan(f.left),r=std::tan(f.right),u=std::tan(f.up),b=std::tan(f.down);
        double coefficients[]={2/(r-l),2/(u-b),-(r+l)/(r-l),-(u+b)/(u-b)};
        result.projection[eye]=base_projection;unsigned indices[]={0,5,8,9};
        for(unsigned i=0;i<4;++i){float quantized=static_cast<float>(coefficients[i]);
            if(!std::isfinite(quantized)||std::abs(double(quantized)-coefficients[i])>1e-6*(std::max)(1.,std::abs(coefficients[i])))return false;
            result.projection[eye][indices[i]]=quantized;
        }
        if(!stereo_projection_supported(result.projection[eye]))return false;
    }
    double actual=stereo_distance(stereo_camera_position(result.view[0]),stereo_camera_position(result.view[1]));
    if(!std::isfinite(actual)||std::abs(actual-separation)>separation*.01)return false;
    out=result;return true;
}
inline bool stereo_matrices(StereoMatrix const& view,StereoMatrix const& projection,
    Pose const& reference,StereoSample const& sample,StereoMatrices& out,bool allow_sim=false) {
    return stereo_matrices(view,projection,reference,sample,100.,out,allow_sim);
}
}
