#include <openxr/openxr.h>
#include "menu_theater.h"
#include <cassert>
#include <limits>
#include <cstring>
int main(){
    XrView views[2]{};for(auto& view:views){view.fov={-.78539816f,.78539816f,.78539816f,-.78539816f};view.pose.orientation.w=1;view.pose.position.y=1.7f;}
    views[0].pose.position.x=-.032f;views[1].pose.position.x=.032f;
    XrSpaceLocation head{};head.locationFlags=XR_SPACE_LOCATION_POSITION_VALID_BIT|XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;
    head.pose.orientation.w=1;head.pose.position.y=1.7f;
    XrCompositionLayerQuad quad{};quad.size={16,9};MenuTheater theater;
    assert(theater.place(quad,XR_NULL_HANDLE,head,1,1,views,2));
    assert(quad.size.width>4&&quad.size.width<4.4f&&std::abs(quad.size.height/quad.size.width-9.f/16)<1e-6f);
    for(unsigned i=0;i<2;++i){const double eye_x=i?.032:-.032;
        assert((quad.size.width/2+std::abs(eye_x))/2.5<std::tan(views[i].fov.angleRight));
        assert((quad.size.width/2+std::abs(eye_x))/2.5<std::tan(-views[i].fov.angleLeft));
        assert(quad.size.height/5<std::tan(views[i].fov.angleUp));}
    assert(quad.pose.position.z==-2.5f&&quad.pose.position.y==1.7f);
    const auto fixed=quad;
    head.pose.position.x=.2f;head.pose.orientation={0,.25881904f,0,.9659258f};
    quad.size={16,9};views[0].fov.angleLeft=-.6f;
    assert(theater.place(quad,XR_NULL_HANDLE,head,1,1,views,2));
    assert(!std::memcmp(&quad.pose,&fixed.pose,sizeof(quad.pose))&&quad.size.width==fixed.size.width);
    quad.size={16,9};assert(theater.place(quad,XR_NULL_HANDLE,head,1,2,views,2));
    assert(quad.size.width<fixed.size.width&&quad.pose.position.x!=fixed.pose.position.x);
    MenuTheater invalid;quad.size={16,9};views[1].fov.angleUp=std::numeric_limits<float>::quiet_NaN();
    assert(!invalid.place(quad,XR_NULL_HANDLE,head,1,1,views,2));
    views[1].fov.angleUp=.78f;assert(!invalid.place(quad,XR_NULL_HANDLE,head,1,1,views,1));
    head.locationFlags=0;assert(!invalid.place(quad,XR_NULL_HANDLE,head,1,1,views,2));
    head.locationFlags=XR_SPACE_LOCATION_POSITION_VALID_BIT|XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;
    head.pose.orientation={0,0,0,1};head.pose.position={0,1.7f,0};
    views[0].fov.angleLeft=-.78539816f;views[1].fov.angleUp=.78539816f;
    views[0].pose.orientation={0,float(std::sin(.15)),0,float(std::cos(.15))};
    views[1].pose.orientation={0,-float(std::sin(.15)),0,float(std::cos(.15))};
    MenuTheater canted;quad.size={16,9};assert(canted.place(quad,XR_NULL_HANDLE,head,1,1,views,2));
    assert(quad.size.width<fixed.size.width*.7f&&quad.size.width>1);
    return 0;
}
