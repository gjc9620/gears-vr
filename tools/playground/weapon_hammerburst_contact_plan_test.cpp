#include "../vr-common/weapon_support_grip.h"
// Recorded9772 planning fixture: metadata identity is synthetic/aligned.
// Confirms input planning against the selected model basis, not a live grant
// or GPU/hand fitting. Actual source/right-grip pairs retained in analysis.
#ifdef NDEBUG
#error Assertions required
#endif
#include <cstdio>
#include <cassert>
int main(){namespace h=gears_weapon_hand;h::Frame f{};h::GripGameState game{};
f.controllers.hands[1].grip.position={0.15999999642372131,1.5200001001358032,0.059999998658895493};
f.controllers.hands[1].grip.orientation={0.14589586853981018,0.21400924026966095,-0.032344333827495575,0.96533328294754028};
f.controllers.hands[0].grip.position={-0.062997216271622775,1.424873934552954,-0.2332685825845292};
for(auto& hand:f.controllers.hands){hand.grip_active=1;hand.grip_flags=15;}
game.mount_known=true;game.mount_asset={0x1000,0x2000,0x3000,4,5,0,1};game.mount_revision=1;
game.mount.support_grip_in_weapon={-0.078163441454891874,-0.19727491860530189,-0.31584218513028894};game.mount.support_length_tolerance_metres=.22;game.mount.maximum_support_correction_radians=.8;
double distance{};const auto contact=h::support_contact(f,game,false,&distance);
std::printf("recorded9772 planned current support contact=%u distance_m=%.9g mount_valid=%u\n",unsigned(contact),distance,unsigned(game.mount.valid()));std::fflush(stdout);assert(contact);
game.mount.support_grip_in_weapon={-0.078160096300430457,0.019556146623430682,-0.37187578841131175};distance=0;
const bool predicted=h::support_contact(f,game,false,&distance);
std::printf("same tracked pair against model-based mount contact=%u distance_m=%.9g\n",unsigned(predicted),distance);std::fflush(stdout);assert(!predicted&&distance>.14);
f.controllers.hands[0].grip.position={-0.058536821430615782,1.6485796253392493,-0.22371114481425819};distance=0;
const bool replanned=h::support_contact(f,game,false,&distance);
std::printf("model-planned tracked input contact=%u distance_m=%.9g\n",unsigned(replanned),distance);std::fflush(stdout);assert(replanned);}
