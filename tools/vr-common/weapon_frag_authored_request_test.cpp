#include "hand_authored_grasp.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
// Match the production root's global public ABI + local grenade include order.
namespace local_grenade {
#include "weapon_frag_grasp.h"
#include "weapon_frag_authored_request.h"
}
namespace a=gears_hand_authored_grasp;
namespace hand=gears_weapon_hand;
namespace frag=local_grenade::gears_weapon_frag_authored;
namespace pose=local_grenade::gears_weapon_frag_grasp_pose;
unsigned checks{};
#define C(x) do{++checks;if(!(x)){std::printf("FAIL line%u %s\n",__LINE__,#x);std::exit(1);}}while(0)
int main(){
    hand::Frame frame{};
    frame.flags=hand::scene_owned|hand::input_matched|hand::gameplay_interactable;
    frame.now_ns=2000000000;frame.controllers.sync_ns=frame.now_ns-1000000;
    frame.controllers.focused=1;frame.weapon.address=0x1000;frame.weapon.klass=0x2000;frame.weapon.known=1;
    auto& right=frame.controllers.hands[1];
    right.squeeze=1;right.squeeze_active=1;right.pressed[gears_vr::control_squeeze]=1;
    right.grip_active=1;right.grip_flags=15;
    const auto original=frame;const auto signature=gears_hand_authored_mount::marcus115_signature;
    a::Request out{};C(frag::request(frame,signature,out));C(a::structural(out));
    C(std::memcmp(&frame,&original,sizeof frame)==0);
    C(std::memcmp(&out.source,&frame,sizeof frame)==0);
    C(out.profile_id==pose::profile_id&&out.side==a::Side::right&&out.mode==a::Mode::right_main_fingers);
    C(out.kind==a::object::Kind::none&&!out.has_trigger_endpoint);
    C(out.grip.position.x==0&&out.grip.position.y==0&&out.grip.position.z==0&&out.grip.orientation.w==1);
    C(std::memcmp(out.reference.data(),pose::reference.data(),sizeof out.reference)==0);
    C(std::memcmp(out.closed.data(),pose::closed.data(),sizeof out.closed)==0);
    auto exact=frame;exact.controllers.sync_ns=exact.now_ns-100000000;C(frag::request(exact,signature,out));
    for(unsigned mode=0;mode<14;++mode){
        auto bad=frame;auto sig=signature;
        switch(mode){
        case 0:bad.flags&=~hand::scene_owned;break;
        case 1:bad.controllers.focused=0;break;
        case 2:bad.now_ns=0;break;
        case 3:bad.controllers.sync_ns=0;break;
        case 4:bad.controllers.sync_ns=bad.now_ns+1;break;
        case 5:bad.controllers.sync_ns=bad.now_ns-100000001;break;
        case 6:bad.weapon.known=0;break;
        case 7:bad.controllers.hands[1].squeeze_active=0;break;
        case 8:bad.controllers.hands[1].squeeze=0;break;
        case 9:bad.controllers.hands[1].squeeze=.35f;break;
        case 10:bad.controllers.hands[1].grip_flags=0;break;
        case 11:bad.controllers.hands[1].grip_active=0;break;
        case 12:sig[0]^=1;break;
        case 13:bad.flags&=~hand::input_matched;break;
        }
        out.profile_id=777;const auto before=out;
        C(!frag::request(bad,sig,out));C(std::memcmp(&out,&before,sizeof out)==0);
    }
    std::printf("Frag cosmetic request: %u checks; global public ABI/local module include order; no C0 or game proof\n",checks);
}
