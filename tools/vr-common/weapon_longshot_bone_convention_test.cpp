#include "weapon_longshot_animation_tracks.h"
#include <cassert>
#include <cstdio>
#include <limits>
namespace tracks=gears_weapon::longshot_tracks;
int main(){
 tracks::Samples samples{};samples.count=3;samples.length=1;
 const gears_vr::Quaternion stored{.1,.2,.3,std::sqrt(.86)};
 for(unsigned bone=0;bone<4;++bone)for(unsigned frame=0;frame<3;++frame){samples.bones[bone][frame].position={double(bone),double(frame),3};samples.bones[bone][frame].orientation=stored;}
 assert(tracks::native_relative_rotations(samples));assert(samples.count==3&&samples.length==1);
 for(unsigned bone=0;bone<4;++bone)for(unsigned frame=0;frame<3;++frame){const auto& value=samples.bones[bone][frame];
  assert(value.position.x==bone&&value.position.y==frame&&value.position.z==3);
  assert(value.orientation.x==-stored.x&&value.orientation.y==-stored.y&&value.orientation.z==-stored.z&&value.orientation.w==stored.w);}
 auto broken=samples;broken.bones[3][2].orientation.w=std::numeric_limits<double>::quiet_NaN();
 const auto before=broken.bones[0][0].orientation;assert(!tracks::native_relative_rotations(broken));assert(broken.bones[0][0].orientation.x==before.x);
 broken=samples;broken.bones[3][2].position.y=std::numeric_limits<double>::infinity();assert(!tracks::native_relative_rotations(broken));assert(broken.bones[0][0].orientation.x==before.x);
 broken=samples;broken.bones[3][2].orientation={1,1,1,1};assert(!tracks::native_relative_rotations(broken));assert(broken.bones[0][0].orientation.x==before.x);
 broken=samples;broken.count=257;assert(!tracks::native_relative_rotations(broken));
 assert(tracks::native_relative_rotations(samples));assert(samples.bones[0][0].orientation.x==stored.x);
 std::puts("Longshot stored-to-native rotation convention: inverse rotations, fixed translations/count/time, continuous signs and failure-atomic validation passed");
}
