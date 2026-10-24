#include "cover_view.h"
#include <cstdio>
#include <limits>

using namespace gears_vr;
unsigned checks{},failures{};
#define CHECK(x) do{++checks;if(!(x)){++failures;std::printf("FAIL line %u: %s\n",__LINE__,#x);}}while(0)
bool near(double a,double b,double tolerance=2e-4){return std::abs(a-b)<=tolerance;}
bool near(Vector a,Vector b,double tolerance=2e-4){return stereo_distance(a,b)<=tolerance;}
StereoSample sample(Pose head){
    StereoSample s{};s.source=StereoSource::MetaOperatorSimulated;s.session=1;s.sequence=1;s.space_epoch=1;
    s.head_flags=3;s.view_flags=15;s.focused=1;s.visible=1;s.should_render=1;s.head=head;
    // Physical eye positions rotate around the head, rather than around the
    // tracking-space origin. This catches accidental use of eye midpoint to
    // synthesize a stance when head orientation alone changes.
    for(unsigned i=0;i<2;++i){
        auto delta=transform(rotation_matrix(head.orientation),{i?.032:-.032,0,0});
        s.eyes[i].pose={{head.position.x+delta.x,head.position.y+delta.y,head.position.z+delta.z},head.orientation};
        s.eyes[i].fov={-.8,.8,.8,-.8};
    }
    return s;
}
Vector midpoint(StereoMatrices const& views){
    auto l=stereo_camera_position(views.view[0]),r=stereo_camera_position(views.view[1]);
    return {(l.x+r.x)/2,(l.y+r.y)/2,(l.z+r.z)/2};
}
int main(){
    const StereoMatrix projection{1,0,0,0,0,1,0,0,0,0,0,1,0,0,10,0};
    for(double yaw:{-3.,-1.5707963267948966,0.,1.5707963267948966,3.}){
        CoverViewAnchor a{{1000,-2000,-39.16},{std::cos(yaw),std::sin(yaw),0},{-std::sin(yaw),std::cos(yaw),0}};
        const Pose reference{{.4,1.55,-.2},{0,std::sin(.37),0,std::cos(.37)}};
        CoverTrackingBasis tracking{};CHECK(cover_tracking_basis(reference,tracking));
        CHECK(near(transform(transpose(rotation_matrix(reference.orientation)),tracking.right),{1,0,0}));
        CHECK(near(transform(transpose(rotation_matrix(reference.orientation)),tracking.up),{0,1,0}));
        for(CoverOffset offset: {CoverOffset{},CoverOffset{-.3,0},CoverOffset{.25,.4}}){
            CoverViewResult base{};CHECK(prepare_cover_view(a,offset,100,base));
            CHECK(near(base.base_eye_world,{a.neutral_eye_world.x+a.right_world.x*offset.right*100,
                a.neutral_eye_world.y+a.right_world.y*offset.right*100,a.neutral_eye_world.z+offset.up*100}));
            // Rotation-only at a fixed head position must not move the eye
            // midpoint, even with yaw, pitch and roll all present.
            for(Quaternion rotation:{Quaternion{},Quaternion{std::sin(.4),0,0,std::cos(.4)},
                                     Quaternion{0,std::sin(.8),0,std::cos(.8)},
                                     Quaternion{0,0,std::sin(.3),std::cos(.3)}}){
                Pose head{reference.position,normalized(multiply(reference.orientation,rotation))};
                CoverPhysicalDisplacement physical{9,9,9};
                CHECK(cover_physical_displacement(reference,head,physical));
                CHECK(physical.right==0&&physical.up==0&&physical.forward==0);
                StereoMatrices eyes{};CHECK(stereo_matrices(base.base_view,projection,reference,sample(head),100,eyes,true));
                CHECK(near(midpoint(eyes),base.base_eye_world));
            }
            // Actual standing/leaning is independent of the virtual offset.
            // The total is checked against BOTH eye origins, so adding the
            // physical translation to the base view as well would fail.
            for(Vector local: {Vector{-.18,.42,.08},Vector{.21,-.3,-.12},Vector{0,.7,0}}){
                auto raw=transform(rotation_matrix(reference.orientation),local);
                Pose head{{reference.position.x+raw.x,reference.position.y+raw.y,reference.position.z+raw.z},reference.orientation};
                CoverPhysicalDisplacement physical{};CHECK(cover_physical_displacement(reference,head,physical));
                CHECK(near(physical.right,local.x)&&near(physical.up,local.y)&&near(physical.forward,-local.z));
                StereoMatrices eyes{};CHECK(stereo_matrices(base.base_view,projection,reference,sample(head),100,eyes,true));
                const Vector expected{base.base_eye_world.x+100*(a.right_world.x*local.x-a.forward_world.x*local.z),
                    base.base_eye_world.y+100*(a.right_world.y*local.x-a.forward_world.y*local.z),base.base_eye_world.z+100*local.y};
                CHECK(near(midpoint(eyes),expected));
                Vector mapped_head{};CHECK(cover_tracking_point(a,base,reference,head.position,100,mapped_head));
                CHECK(near(mapped_head,midpoint(eyes)));
                // A hand beside the same physical head is mapped directly.
                // Physical head movement must not be added a second time.
                const auto hand_delta=transform(rotation_matrix(reference.orientation),{-.3,-.25,-.4});
                Vector hand{head.position.x+hand_delta.x,head.position.y+hand_delta.y,head.position.z+hand_delta.z},mapped_hand{};
                CHECK(cover_tracking_point(a,base,reference,hand,100,mapped_hand));
                CHECK(near(mapped_hand,{expected.x+100*(-.3*a.right_world.x+.4*a.forward_world.x),
                    expected.y+100*(-.3*a.right_world.y+.4*a.forward_world.y),expected.z-25}));
                CoverViewResult unchanged{};CHECK(prepare_cover_view(a,offset,100,unchanged));
                CHECK(unchanged.base_view==base.base_view); // Real motion never mutates the held virtual base.
            }
        }
    }
    // ab body3's measured head curve is a test input, not a built-in body
    // calibration. Quarter blend was NOT quarter world height.
    const CoverHeightKey curve[]={{0,0},{.25,.16212387},{.5,.35959511},{.75,.52463798},{1,.67705299}};
    double u=-1;CHECK(cover_up_weight(curve,5,curve[1].height_metres,u)&&near(u,.25,1e-12));
    CHECK(cover_up_weight(curve,5,(curve[1].height_metres+curve[2].height_metres)/2,u)&&near(u,.375,1e-12));
    CHECK(cover_up_weight(curve,5,-.2,u)&&u==0);
    CHECK(cover_up_weight(curve,5,2,u)&&u==1);
    CHECK(cover_up_weight(curve,5,.5,u)&&u>.5&&u<.75);
    CoverHeightKey bad[]={{0,0},{.5,.4},{1,.3}};u=7;
    CHECK(!cover_up_weight(bad,3,.2,u)&&u==7);
    CHECK(!cover_up_weight(nullptr,3,.2,u)&&u==7);
    CHECK(!cover_up_weight(curve,5,(std::numeric_limits<double>::quiet_NaN)(),u)&&u==7);
    CoverViewResult sentinel{};sentinel.base_eye_world={7,8,9};auto saved=sentinel;
    CoverViewAnchor wrong{{0,0,0},{1,0,0},{0,-1,0}};
    CHECK(!prepare_cover_view(wrong,{},100,sentinel)&&near(sentinel.base_eye_world,saved.base_eye_world));
    wrong.right_world={0,1,0};wrong.forward_world.z=.1;
    CHECK(!prepare_cover_view(wrong,{},100,sentinel));
    wrong.forward_world.z=0;
    CHECK(!prepare_cover_view(wrong,{},0,sentinel));
    CoverPhysicalDisplacement displacement{7,8,9};Pose invalid{{0,0,0},{0,0,0,0}};
    CHECK(!cover_physical_displacement(invalid,Pose{},displacement)&&displacement.right==7&&displacement.up==8&&displacement.forward==9);
    CoverTrackingBasis bad_basis{{7,8,9},{4,5,6}};
    CHECK(!cover_tracking_basis(invalid,bad_basis)&&bad_basis.right.x==7&&bad_basis.up.x==4);
    Vector unchanged_point{7,8,9};CoverViewAnchor valid_anchor{{},{1,0,0},{0,1,0}};CoverViewResult valid_base{};
    CHECK(prepare_cover_view(valid_anchor,{},100,valid_base));
    CHECK(!cover_tracking_point(valid_anchor,valid_base,invalid,{},100,unchanged_point)&&unchanged_point.x==7);
    CHECK(!cover_tracking_point(valid_anchor,valid_base,Pose{}, {},0,unchanged_point)&&unchanged_point.x==7);
    std::printf("cover_view: %u checks, %u failures\n",checks,failures);return failures?1:0;
}
