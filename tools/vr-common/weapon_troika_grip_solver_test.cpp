#include "weapon_troika_grip_solver.h"
#include <cmath>
#include <initializer_list>
#include <cstdio>
#include <cstdlib>
using namespace gears_weapon;
namespace {unsigned checks{};
void check(bool okay,int line){++checks;if(!okay){std::fprintf(stderr,"line %d\n",line);std::exit(1);}}
#define CHECK(x) check(bool(x),__LINE__)
}
int main(){
    // Synthetic pivot basis/units/limits only. Actual retail axes and scale
    // must be supplied by the same mounted instance, never these fixtures.
    TroikaSolverCalibration c{1,2,{-.2,-.5,.1},{.2,-.5,.1},-.8,.8,-.4,.4,.015,true};
    for(auto angles:{TroikaSolverPoint{0,0,0},{.3,.2,0},{-.7,-.3,0},{.8,.4,0}}){
        TroikaSolverSample s{1,2,3,troika_solver_detail::rotate(c.left,angles.x,angles.y),
            troika_solver_detail::rotate(c.right,angles.x,angles.y),true,true,true,true,true};
        TroikaSolverResult r{};CHECK(solve_troika_grips(c,s,r));
        CHECK(std::abs(r.yaw-angles.x)<1e-5&&std::abs(r.pitch-angles.y)<1e-5);
        CHECK(r.frame_ticket==3);
        for(unsigned gate=0;gate<7;++gate){
            auto bad=s;switch(gate){case 0:bad.left_tracked=false;break;
                case 1:bad.right_grip=false;break;case 2:++bad.binding;break;
                case 3:++bad.calibration;break;case 4:bad.pivot_basis_verified=false;break;
                case 5:bad.left.x+=.2;break;default:bad.right.z=std::nan("");break;}
            const auto before=r;CHECK(!solve_troika_grips(c,bad,r));
            CHECK(r.yaw==before.yaw&&r.pitch==before.pitch&&r.frame_ticket==before.frame_ticket);
        }
    }
    std::printf("Troika fixed-pivot solver: %u synthetic checks passed; retail calibration pending.\n",checks);
}

