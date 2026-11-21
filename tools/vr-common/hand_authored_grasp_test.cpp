#include "hand_authored_grasp.h"
#include "hand_part_grasp.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
namespace a=gears_hand_authored_grasp;
unsigned checks{};
#define C(x) do{++checks;if(!(x)){std::printf("FAIL line%u %s\n",__LINE__,#x);std::exit(1);}}while(0)
int main(){
    a::Request q{};q.geometry_signature=gears_hand_authored_mount::marcus115_signature;q.profile_id=19;q.side=a::Side::right;q.mode=a::Mode::right_main_fingers;
    a::Binding binding{};binding.geometry_signature=gears_hand_authored_mount::marcus115_signature;binding.reference=q.reference;
    C(a::structural(q));C(a::reference_matches(q,binding));
    const auto original=q; a::Joints out{};out[0]={0,0,1,0};const auto sentinel=out;
    q.reference[0]={0,0,.01,std::sqrt(.9999)};C(!a::rotations(q,binding,0,out));C(std::memcmp(out.data(),sentinel.data(),sizeof out)==0);q=original;
    for(auto& r:q.reference)r.w=-1;C(a::reference_matches(q,binding));q=original;
    auto reject=[&](a::Request const& bad){C(!a::structural(bad));};
    auto bad=q;bad.size--;reject(bad);bad=q;bad.version=3;reject(bad);bad=q;bad.reserved=1;reject(bad);
    bad=q;bad.geometry_signature={};reject(bad);bad=q;bad.profile_id=0;reject(bad);
    bad=q;bad.side=static_cast<a::Side>(2);reject(bad);bad=q;bad.mode=static_cast<a::Mode>(2);reject(bad);
    bad=q;bad.kind=a::object::Kind::support;reject(bad);bad=q;bad.proximity=1;reject(bad);bad=q;bad.grip.position.x=.001;reject(bad);
    bad=q;bad.has_trigger_endpoint=2;reject(bad);bad=q;bad.closed[14].x=std::numeric_limits<double>::quiet_NaN();reject(bad);
    bad=q;bad.closed[3].w=.99;reject(bad);bad=q;bad.trigger_closed[0]={0,0,1,0};reject(bad);
    q.has_trigger_endpoint=1;q.trigger_closed[0]={0,0,1,0};C(a::structural(q));
    C(a::rotations(q,binding,0,out)&&out[0].w==1);C(a::rotations(q,binding,1,out)&&out[0].z==1);
    C(a::rotations(q,binding,.5,out)&&std::abs(out[0].z-std::sqrt(.5))<1e-12);
    C(!a::rotations(q,binding,1.001,out));C(!a::rotations(q,binding,-.001,out));C(!a::rotations(q,binding,std::numeric_limits<double>::infinity(),out));
    bad=q;bad.trigger_closed[3]={0,0,1,0};reject(bad);
    binding.geometry_signature[31]^=1;C(!a::reference_matches(q,binding));binding.geometry_signature[31]^=1;
    binding.reference[14].w=0;C(!a::reference_matches(q,binding));binding.reference=original.reference;
    q=original;q.side=a::Side::left;q.mode=a::Mode::left_part;
    for(auto kind:{a::object::Kind::magazine,a::object::Kind::loading,a::object::Kind::starter,a::object::Kind::support}){
        q.kind=kind;C(a::structural(q));q.proximity=1;C(a::structural(q));q.proximity=0;
    }
    q.kind=a::object::Kind::none;reject(q);q.kind=a::object::Kind::magazine;q.has_trigger_endpoint=1;reject(q);
    C(sizeof(a::object::Request)==1496); // Includes final eight-byte alignment padding.
    C(sizeof(gears_hand_part_grasp::Request)==1512);
    std::printf("Authored cosmetic wire: %u contract checks passed; no broker, C0 or game proof\n",checks);
}
