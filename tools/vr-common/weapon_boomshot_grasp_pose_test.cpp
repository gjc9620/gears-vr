#include "weapon_boomshot_grasp_pose.h"
#include <cassert>
#include <cstdio>
namespace b=gears_boomshot_grasp_pose;namespace a=gears_hand_authored_grasp;
int main(){
 a::Joints reference{};for(unsigned i=0;i<15;++i)reference[i]=gears_hand_fingers::turn(1,.01*i);
 const auto signature=gears_hand_authored_mount::marcus115_signature;
 const gears_weapon_hand::Frame source{};std::array<a::Request,6> requests{};
 for(unsigned part=0;part<6;++part){auto& q=requests[part];
    assert(b::request(b::Part(part),signature,reference,source,{{1,2,3},{}},true,q));
    assert(a::structural(q)&&q.profile_id==b::profile_id(b::Part(part)));
    a::Binding binding{signature,reference};assert(a::reference_matches(q,binding));
    a::Joints low{},high{};assert(a::rotations(q,binding,0,low)&&a::rotations(q,binding,1,high));
    for(unsigned i=0;i<15;++i){assert(a::unit(low[i])&&a::unit(high[i]));
        if(part||i>=3)assert(!std::memcmp(&low[i],&high[i],sizeof low[i]));}
    if(!part)assert(q.mode==a::Mode::right_main_fingers&&q.proximity==0&&a::identity(q.grip));
    else assert(q.mode==a::Mode::left_part&&q.proximity==1&&q.grip.position.x==1);
    for(unsigned other=0;other<part;++other)assert(q.profile_id!=requests[other].profile_id&&std::memcmp(q.closed.data(),requests[other].closed.data(),sizeof q.closed));
 }
 a::Request output{};output.profile_id=123;
 for(unsigned part=0;part<6;++part){a::Request actual{};
    assert(b::marcus115_request(b::Part(part),source,{},false,actual));
    const auto& model_reference=part?gears_hand_marcus115_reference::left:gears_hand_marcus115_reference::right;
    assert(a::reference_matches(actual,a::Binding{signature,model_reference}));}
 assert(!b::request(b::Part(6),signature,reference,source,{},false,output)&&output.profile_id==123);
 auto bad_signature=signature;bad_signature[31]^=1;
 assert(!b::request(b::Part::carrier,bad_signature,reference,source,{},false,output)&&output.profile_id==123);
 reference[7].w=0;
 assert(!b::request(b::Part::carrier,signature,reference,source,{},false,output)&&output.profile_id==123);
 std::puts("Boomshot six distinct authored skin drafts: model signature/reference boundary,15unit locals, index-only real-trigger endpoint and V4 cosmetic request passed; no live mount/skin-fit claim");
}
