#include "weapon_hammerburst_part_hand.h"
#include <cstdio>
#include <cstring>

namespace api=gears_hand_authored_grasp;using Kind=api::object::Kind;
static int cases{};
#define CHECK(x) do{if(!(x)){std::printf("FAIL line %d: %s\n",__LINE__,#x);return 1;}++cases;}while(0)
static bool same(gears_vr::Quaternion a,gears_vr::Quaternion b){return a.x==b.x&&a.y==b.y&&a.z==b.z&&a.w==b.w;}
static bool same_joints(api::Joints const& a,const gears_vr::Quaternion* b){for(unsigned i=0;i<15;++i)if(!same(a[i],b[i]))return false;return true;}
static bool same_request(api::Request const& a,api::Request const& b){return std::memcmp(&a,&b,sizeof(api::Request))==0;}
static bool same_frame(gears_weapon_hand::Frame const& a,gears_weapon_hand::Frame const& b){return std::memcmp(&a,&b,sizeof(gears_weapon_hand::Frame))==0;}
int main(){
    gears_weapon_hand::Frame source{};source.key.frame=19;source.key.lifecycle=23;source.press_token=29;
    gears_vr::Pose grip{};grip.position={1.,2.,3.};grip.orientation={0.,0.,0.,1.};
    api::Request support{},magazine{};
    CHECK(gears_hammerburst_part_hand::request(Kind::support,source,grip,support));
    CHECK(support.size==sizeof(api::Request)&&support.version==4&&support.reserved==0);
    CHECK(support.kind==Kind::support&&support.side==api::Side::left&&support.mode==api::Mode::left_part);
    CHECK(support.profile_id==gears_hammerburst_part_hand::support_profile_id&&
          support.profile_id!=gears_hammerburst_part_hand::magazine_profile_id);
    CHECK(support.geometry_signature==gears_hand_authored_mount::marcus115_signature);
    CHECK(same_joints(support.reference,gears_hand_marcus115_reference::left.data()));
    CHECK(same_joints(support.closed,gears_weapon_hammerburst_foregrip::captured_finger_locals));
    CHECK(support.has_trigger_endpoint==0&&support.proximity==0);
    CHECK(same_frame(support.source,source));
    CHECK(support.grip.position.x==1.&&support.grip.position.y==2.&&support.grip.position.z==3.&&
          support.grip.orientation.x==0.&&support.grip.orientation.y==0.&&support.grip.orientation.z==0.&&support.grip.orientation.w==1.);
    CHECK(api::structural(support));
    CHECK(gears_hammerburst_part_hand::request(Kind::magazine,source,grip,magazine));
    CHECK(magazine.kind==Kind::magazine&&magazine.side==api::Side::left&&magazine.mode==api::Mode::left_part);
    CHECK(magazine.profile_id==gears_hammerburst_part_hand::magazine_profile_id&&
          magazine.profile_id!=gears_hammerburst_part_hand::support_profile_id);
    CHECK(magazine.geometry_signature==gears_hand_authored_mount::marcus115_signature);
    CHECK(same_joints(magazine.reference,gears_hand_marcus115_reference::left.data()));
    CHECK(same_joints(magazine.closed,gears_weapon_hammerburst_owner_magazine::authored_finger_locals));
    CHECK(magazine.has_trigger_endpoint==0&&magazine.proximity==0);
    CHECK(same_frame(magazine.source,source));
    CHECK(magazine.grip.position.x==1.&&magazine.grip.position.y==2.&&magazine.grip.position.z==3.);
    CHECK(api::structural(magazine));
    api::Request sentinel{};sentinel.profile_id=0xfeedfaceull;sentinel.kind=Kind::support;
    const auto before=sentinel;
    for(Kind kind:{Kind::none,Kind::loading,Kind::starter,static_cast<Kind>(0xffffffffu)}){
        CHECK(!gears_hammerburst_part_hand::request(kind,source,grip,sentinel));
        CHECK(same_request(sentinel,before));
    }
    gears_vr::Pose bad{};bad.position={0.,0.,0.};bad.orientation={0.,0.,0.,0.};
    CHECK(!gears_hammerburst_part_hand::request(Kind::support,source,bad,sentinel));
    CHECK(same_request(sentinel,before));
    CHECK(!gears_hammerburst_part_hand::request(Kind::magazine,source,bad,sentinel));
    CHECK(same_request(sentinel,before));
    std::printf("HammerburstPartHandTest: %d cases; exact owner arrays/reference/signature/profile; invalid kind and grip leave out unchanged\n",cases);
    return 0;
}