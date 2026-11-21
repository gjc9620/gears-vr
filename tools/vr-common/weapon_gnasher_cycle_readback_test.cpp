#include "weapon_gnasher_cycle_readback.h"
#include <cstdio>
#include <cstdlib>
#define CHECK(v) do{if(!(v)){std::fprintf(stderr,"line %u: %s\n",unsigned(__LINE__),#v);std::exit(1);}}while(false)
using namespace gears_weapon;
int main(){
 const GnasherIdentity id{1,2,3,4,5,6,7,8};
 const GnasherCycleRequest request{id,11,500,MechanismAction::lever_down_return,ReloadEvent::cycled,20,1000000000};
 const GnasherClosedLeverReference reference{22,{{0,.2,-.03},{}}};
 GnasherCycleReadback value{id,11,22,20,1,1000000001,reference.in_main,true,true};GnasherCycleReceipt receipt{};
 for(unsigned n=0;n<12;++n){
  GnasherCycleReadbackGate gate;CHECK(gate.begin(request,reference,1000000001));auto bad=value;
  switch(n){
   case 0:++bad.identity.space_epoch;break;
   case 1:++bad.request_token;break;
   case 2:++bad.calibration_id;break;
   case 3:--bad.input_sequence;break;
   case 4:bad.original_observation_sequence=0;break;
   case 5:bad.observed_ns=request.physical_completion_ns-1;break;
   case 6:bad.original_bones_verified=false;break;
   case 7:bad.original_ready=false;break;
   case 8:bad.lever_in_main.position.y+=.003;break;
   case 9:bad.lever_in_main.orientation={std::sin(.02),0,0,std::cos(.02)};break;
   case 10:bad.lever_in_main.orientation={0,0,0,0};break;
   case 11:bad.observed_ns=1000000003;break;
  }
  CHECK(gate.endpoint_current(bad,1000000002)==(n==7));
  CHECK(!gate.observe(bad,1000000002,receipt));CHECK(!receipt.accepted&&gate.pending());
 }
 GnasherCycleReadbackGate stale;CHECK(stale.begin(request,reference,1000000001));CHECK(!stale.observe(value,1100000002,receipt));
 GnasherCycleReadbackGate gate;CHECK(gate.begin(request,reference,1000000001));
 auto moving=value;moving.original_ready=false;CHECK(gate.endpoint_current(moving,1000000002));
 CHECK(gate.pending()&&!receipt.accepted);CHECK(!gate.observe(moving,1000000002,receipt));
 CHECK(!gate.endpoint_current(value,1000000002)); // No reuse of an observed record.
 CHECK(!gate.observe(value,1000000002,receipt)); // Same observation cannot be reinterpreted as ready.
 ++value.original_observation_sequence;CHECK(gate.observe(value,1000000002,receipt));
 CHECK(receipt.accepted&&receipt.request_token==request.token&&receipt.original_event_sequence==2&&!gate.pending());
 CHECK(!gate.endpoint_current(value,1000000002));CHECK(!gate.observe(value,1000000002,receipt));
 GnasherCycleReadbackGate late;CHECK(!late.begin(request,reference,1100000001));
 GnasherCycleReadbackGate qsign;CHECK(qsign.begin(request,reference,1000000001));value.lever_in_main.orientation={0,0,0,-1};CHECK(qsign.observe(value,1000000002,receipt));
 std::puts("Gnasher original-cycle readback: physical request alone cannot unlock; real original endpoint/readiness, identity, freshness, source sequence, and once-only acknowledgement checks passed");
}
