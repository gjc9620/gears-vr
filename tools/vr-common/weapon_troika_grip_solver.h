#pragma once
#include <cstdint>
#include <initializer_list>
namespace gears_weapon {
// Coordinates are metres in the owner-certified fixed pivot basis. Canonical
// rotation is yaw about +Z followed by pitch about +Y. Native must calibrate
// signs/basis/order against actual turret axes before it may consume output.
struct TroikaSolverPoint {double x{},y{},z{};};
struct TroikaSolverCalibration {
    std::uint64_t binding{},calibration{};
    TroikaSolverPoint left{},right{};
    double min_yaw{},max_yaw{},min_pitch{},max_pitch{},max_hand_error_m{};
    bool original_axes_and_limits_verified{};
};
struct TroikaSolverSample {
    std::uint64_t binding{},calibration{},frame_ticket{};
    TroikaSolverPoint left{},right{};
    bool left_tracked{},right_tracked{},left_grip{},right_grip{},pivot_basis_verified{};
};
struct TroikaSolverResult {double yaw{},pitch{},error_squared{};std::uint64_t frame_ticket{};};
namespace troika_solver_detail {
constexpr double pi=3.14159265358979323846;
constexpr bool finite(double x){return x==x&&x>=-1e100&&x<=1e100;}
constexpr bool point(TroikaSolverPoint p){return finite(p.x)&&finite(p.y)&&finite(p.z);}
constexpr double sq(double x){return x*x;}
constexpr double norm(TroikaSolverPoint p){return sq(p.x)+sq(p.y)+sq(p.z);}
constexpr double distance(TroikaSolverPoint a,TroikaSolverPoint b){return sq(a.x-b.x)+sq(a.y-b.y)+sq(a.z-b.z);}
// Bounded [-pi,pi] Taylor evaluation enables compiler-only geometry fixtures.
// 17 terms; native/headset calibration remains separate from numerical checks.
constexpr double sine(double x){double term=x,sum=x;for(unsigned n=1;n<17;++n){term*=-x*x/((2.0*n)*(2.0*n+1));sum+=term;}return sum;}
constexpr double cosine(double x){double term=1,sum=1;for(unsigned n=1;n<17;++n){term*=-x*x/((2.0*n-1)*(2.0*n));sum+=term;}return sum;}
constexpr TroikaSolverPoint rotate(TroikaSolverPoint p,double yaw,double pitch){
    const double cy=cosine(yaw),sy=sine(yaw),cp=cosine(pitch),sp=sine(pitch);
    const double x=cp*p.x+sp*p.z,z=-sp*p.x+cp*p.z;
    return {cy*x-sy*p.y,sy*x+cy*p.y,z};
}
constexpr bool valid(TroikaSolverCalibration const& c){
    if(!c.binding||!c.calibration||!c.original_axes_and_limits_verified||!point(c.left)||!point(c.right)||
       !finite(c.min_yaw)||!finite(c.max_yaw)||!finite(c.min_pitch)||!finite(c.max_pitch)||
       c.min_yaw< -pi||c.max_yaw>pi||c.min_pitch< -pi/2||c.max_pitch>pi/2||
       c.min_yaw>=c.max_yaw||c.min_pitch>=c.max_pitch||!finite(c.max_hand_error_m)||
       c.max_hand_error_m<=0||c.max_hand_error_m>0.25||norm(c.left)>=100||norm(c.right)>=100)return false;
    // Two pivot-relative vectors must constrain the rotation, not form one line.
    TroikaSolverPoint cross{c.left.y*c.right.z-c.left.z*c.right.y,
       c.left.z*c.right.x-c.left.x*c.right.z,c.left.x*c.right.y-c.left.y*c.right.x};
    return norm(c.left)<100&&norm(c.right)<100&&distance(c.left,c.right)>0.000001&&norm(cross)>1e-10;
}
constexpr double error(TroikaSolverCalibration const& c,TroikaSolverSample const& s,double y,double p){
    return distance(rotate(c.left,y,p),s.left)+distance(rotate(c.right,y,p),s.right);
}
}
// Bounded coarse search then refinement of the best basin. A bad fit never
// changes output. No timer, HMD direction or previous fire command is an input.
constexpr bool solve_troika_grips(TroikaSolverCalibration const& c,TroikaSolverSample const& s,TroikaSolverResult& out){
    using namespace troika_solver_detail;
    if(!valid(c)||s.binding!=c.binding||s.calibration!=c.calibration||!s.frame_ticket||
       !s.left_tracked||!s.right_tracked||!s.left_grip||!s.right_grip||!s.pivot_basis_verified||
       !point(s.left)||!point(s.right)||norm(s.left)>100||norm(s.right)>100)return false;
    TroikaSolverResult best{c.min_yaw,c.min_pitch,1e10,s.frame_ticket};
    for(unsigned iy=0;iy<=8;++iy)for(unsigned ip=0;ip<=8;++ip){
        const double y=c.min_yaw+(c.max_yaw-c.min_yaw)*iy/8,p=c.min_pitch+(c.max_pitch-c.min_pitch)*ip/8;
        const double e=error(c,s,y,p);if(e<best.error_squared)best={y,p,e,s.frame_ticket};
    }
    double dy=(c.max_yaw-c.min_yaw)/8,dp=(c.max_pitch-c.min_pitch)/8;
    for(unsigned n=0;n<20;++n){
        const auto center=best;
        for(int iy=-1;iy<=1;++iy)for(int ip=-1;ip<=1;++ip){
            const double y=center.yaw+iy*dy,p=center.pitch+ip*dp;
            if(y<c.min_yaw||y>c.max_yaw||p<c.min_pitch||p>c.max_pitch)continue;
            const double e=error(c,s,y,p);if(e<best.error_squared)best={y,p,e,s.frame_ticket};
        }dy*=0.5;dp*=0.5;
    }
    if(distance(rotate(c.left,best.yaw,best.pitch),s.left)>sq(c.max_hand_error_m)||
       distance(rotate(c.right,best.yaw,best.pitch),s.right)>sq(c.max_hand_error_m))return false;
    out=best;return true;
}

// Serial original-joint geometry. Latitude1 is parent of longitude2; their
// actual centres are distinct. Handle/muzzle points descend from longitude2.
// The earlier coincident canonical solver is a synthetic special case only.
// Axes/radian bounds must come from the same authenticated ORIGINAL controls;
// names, a successful fit, or the reference asset alone do not certify them.
struct TroikaSerialJoint {TroikaSolverPoint centre{},axis{};double minimum{},maximum{};};
struct TroikaSerialCalibration {
    std::uint64_t binding{},calibration{};TroikaSerialJoint parent{},child{};
    TroikaSolverPoint left{},right{};double max_hand_error_m{};
    bool original_hierarchy_axes_limits_verified{};
    bool child_axis_follows_parent{true};
};
struct TroikaSerialResult {
    double parent_angle{},child_angle{},error_squared{};std::uint64_t frame_ticket{};
    TroikaSolverPoint left{},right{},child_centre{},child_axis{};
};
namespace troika_serial_detail {
using namespace troika_solver_detail;
constexpr TroikaSolverPoint add(TroikaSolverPoint a,TroikaSolverPoint b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
constexpr TroikaSolverPoint sub(TroikaSolverPoint a,TroikaSolverPoint b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
constexpr TroikaSolverPoint mul(TroikaSolverPoint a,double b){return {a.x*b,a.y*b,a.z*b};}
constexpr double dot(TroikaSolverPoint a,TroikaSolverPoint b){return a.x*b.x+a.y*b.y+a.z*b.z;}
constexpr TroikaSolverPoint cross(TroikaSolverPoint a,TroikaSolverPoint b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
constexpr bool axis(TroikaSolverPoint a){return point(a)&&norm(a)>1-1e-8&&norm(a)<1+1e-8;}
constexpr TroikaSolverPoint rotate_axis(TroikaSolverPoint p,TroikaSolverPoint a,double angle){
    const double c=cosine(angle),s=sine(angle);return add(add(mul(p,c),mul(cross(a,p),s)),mul(a,dot(a,p)*(1-c)));
}
constexpr TroikaSolverPoint about(TroikaSolverPoint p,TroikaSerialJoint const& j,double angle){return add(j.centre,rotate_axis(sub(p,j.centre),j.axis,angle));}
constexpr TroikaSolverPoint forward(TroikaSerialCalibration const& c,TroikaSolverPoint p,double parent,double child){
    // Child rotates in the parent's original basis; then the complete child
    // assembly (centre, axis and points) follows its parent. No bone stretching.
    if(c.child_axis_follows_parent)return about(about(p,c.child,child),c.parent,parent);
    // World/actor/component rotation spaces keep the child's allowed axis in
    // that fixed frame, while its centre still follows the parent joint.
    const TroikaSerialJoint moved{about(c.child.centre,c.parent,parent),c.child.axis,c.child.minimum,c.child.maximum};
    return about(about(p,c.parent,parent),moved,child);
}
constexpr bool geometry_valid(TroikaSerialCalibration const& c){
    if(!c.binding||!c.calibration||!point(c.left)||!point(c.right)||
       !finite(c.max_hand_error_m)||c.max_hand_error_m<=0||c.max_hand_error_m>.25||distance(c.left,c.right)<1e-6)return false;
    for(auto const* j:{&c.parent,&c.child})if(!point(j->centre)||norm(j->centre)>100||!axis(j->axis)||
       !finite(j->minimum)||!finite(j->maximum)||j->minimum< -pi||j->maximum>pi||j->minimum>=j->maximum)return false;
    // An observable two-axis pair is necessary; a rank-deficient hand Jacobian
    // is rejected again at the actual fitted configuration below.
    return norm(cross(c.parent.axis,c.child.axis))>1e-8&&norm(c.left)<100&&norm(c.right)<100;
}
constexpr bool valid(TroikaSerialCalibration const& c){return c.original_hierarchy_axes_limits_verified&&geometry_valid(c);}
constexpr double error(TroikaSerialCalibration const& c,TroikaSolverSample const& s,double p,double q){
    return distance(forward(c,c.left,p,q),s.left)+distance(forward(c,c.right,p,q),s.right);
}
constexpr TroikaSolverPoint parent_tangent(TroikaSerialCalibration const& c,TroikaSolverPoint point,
    TroikaSolverPoint centre,TroikaSolverPoint child_axis,double child_angle){
    if(c.child_axis_follows_parent)return cross(c.parent.axis,sub(point,c.parent.centre));
    const auto unrotated=rotate_axis(sub(point,centre),child_axis,-child_angle);
    return add(cross(c.parent.axis,sub(centre,c.parent.centre)),
        rotate_axis(cross(c.parent.axis,unrotated),child_axis,child_angle));
}
constexpr bool observable(TroikaSerialCalibration const& c,TroikaSerialResult const& r){
    const auto p0=parent_tangent(c,r.left,r.child_centre,r.child_axis,r.child_angle),
        p1=parent_tangent(c,r.right,r.child_centre,r.child_axis,r.child_angle);
    const auto q0=cross(r.child_axis,sub(r.left,r.child_centre)),q1=cross(r.child_axis,sub(r.right,r.child_centre));
    const double pp=norm(p0)+norm(p1),qq=norm(q0)+norm(q1),pq=dot(p0,q0)+dot(p1,q1);
    return pp>1e-8&&qq>1e-8&&pp*qq-pq*pq>pp*qq*1e-6;
}
}
namespace troika_serial_detail {
constexpr bool solve_geometry(TroikaSerialCalibration const& c,TroikaSolverSample const& s,TroikaSerialResult& out){
    namespace d=troika_serial_detail;using namespace troika_solver_detail;
    if(!d::geometry_valid(c)||s.binding!=c.binding||s.calibration!=c.calibration||!s.frame_ticket||!s.left_tracked||!s.right_tracked||
       !s.left_grip||!s.right_grip||!s.pivot_basis_verified||!point(s.left)||!point(s.right)||norm(s.left)>100||norm(s.right)>100)return false;
    TroikaSerialResult best{};best.error_squared=1e10;best.frame_ticket=s.frame_ticket;
    for(unsigned ip=0;ip<=12;++ip)for(unsigned iq=0;iq<=12;++iq){
        const double p=c.parent.minimum+(c.parent.maximum-c.parent.minimum)*ip/12;
        const double q=c.child.minimum+(c.child.maximum-c.child.minimum)*iq/12;
        const double e=d::error(c,s,p,q);if(e<best.error_squared){best.parent_angle=p;best.child_angle=q;best.error_squared=e;}
    }
    double dp=(c.parent.maximum-c.parent.minimum)/12,dq=(c.child.maximum-c.child.minimum)/12;
    for(unsigned n=0;n<24;++n){const auto centre=best;
        for(int ip=-1;ip<=1;++ip)for(int iq=-1;iq<=1;++iq){
            const double p=centre.parent_angle+ip*dp,q=centre.child_angle+iq*dq;
            if(p<c.parent.minimum||p>c.parent.maximum||q<c.child.minimum||q>c.child.maximum)continue;
            const double e=d::error(c,s,p,q);if(e<best.error_squared){best.parent_angle=p;best.child_angle=q;best.error_squared=e;}
        }dp*=.5;dq*=.5;
    }
    // The two handle trajectories are coupled. A shrinking coordinate grid
    // alone can stall along an oblique valley. Solve the actual 2x2 hand
    // Jacobian, with bounded projection and a decreasing-error line search.
    for(unsigned iteration=0;iteration<12;++iteration){
        const auto centre=d::about(c.child.centre,c.parent,best.parent_angle);
        const auto axis=c.child_axis_follows_parent?d::rotate_axis(c.child.axis,c.parent.axis,best.parent_angle):c.child.axis;
        double pp{},qq{},pq{},pg{},qg{};
        for(unsigned side=0;side<2;++side){
            const auto point=d::forward(c,side?c.right:c.left,best.parent_angle,best.child_angle);
            const auto residual=d::sub(point,side?s.right:s.left);
            const auto p=d::parent_tangent(c,point,centre,axis,best.child_angle),q=d::cross(axis,d::sub(point,centre));
            pp+=norm(p);qq+=norm(q);pq+=d::dot(p,q);pg+=d::dot(p,residual);qg+=d::dot(q,residual);
        }
        const double determinant=pp*qq-pq*pq;if(pp<=1e-8||qq<=1e-8||determinant<=pp*qq*1e-6)break;
        const double p_step=(pq*qg-qq*pg)/determinant,q_step=(pq*pg-pp*qg)/determinant;
        bool improved=false;double fraction=1;
        for(unsigned line=0;line<10;++line){
            const double raw_p=best.parent_angle+p_step*fraction,raw_q=best.child_angle+q_step*fraction;
            const double p=raw_p<c.parent.minimum?c.parent.minimum:raw_p>c.parent.maximum?c.parent.maximum:raw_p;
            const double q=raw_q<c.child.minimum?c.child.minimum:raw_q>c.child.maximum?c.child.maximum:raw_q;
            const double error=d::error(c,s,p,q);
            if(error<best.error_squared){best.parent_angle=p;best.child_angle=q;best.error_squared=error;improved=true;break;}
            fraction*=.5;
        }
        if(!improved)break;
    }
    best.left=d::forward(c,c.left,best.parent_angle,best.child_angle);best.right=d::forward(c,c.right,best.parent_angle,best.child_angle);
    best.child_centre=d::about(c.child.centre,c.parent,best.parent_angle);
    best.child_axis=c.child_axis_follows_parent?d::rotate_axis(c.child.axis,c.parent.axis,best.parent_angle):c.child.axis;
    if(distance(best.left,s.left)>sq(c.max_hand_error_m)||distance(best.right,s.right)>sq(c.max_hand_error_m)||!d::observable(c,best))return false;
    out=best;return true;
}
}
// Existing bounded calibration API retains its original strict proof gate.
constexpr bool solve_troika_serial_grips(TroikaSerialCalibration const& c,TroikaSolverSample const& s,TroikaSerialResult& out){
    return troika_serial_detail::valid(c)&&troika_serial_detail::solve_geometry(c,s,out);
}
struct TroikaSerialGeometry {
    std::uint64_t binding{},calibration{};TroikaSerialJoint parent{},child{};
    TroikaSolverPoint left{},right{};double max_hand_error_m{};
    bool original_hierarchy_axes_verified{},child_axis_follows_parent{true};
};
// A geometric candidate has NO original-limit certificate. Its search bounds
// are mathematical intervals, not original mechanical limits or write grants.
struct TroikaSerialCandidate {std::uint64_t binding{},calibration{};TroikaSerialResult fit{};};
constexpr bool propose_troika_serial_grips(TroikaSerialGeometry const& g,TroikaSolverSample const& s,TroikaSerialCandidate& out){
    if(!g.original_hierarchy_axes_verified)return false;
    const TroikaSerialCalibration geometry{g.binding,g.calibration,g.parent,g.child,g.left,g.right,g.max_hand_error_m,false,g.child_axis_follows_parent};
    TroikaSerialResult result{};if(!troika_serial_detail::solve_geometry(geometry,s,result))return false;
    out={g.binding,g.calibration,result};return true;
}
}
