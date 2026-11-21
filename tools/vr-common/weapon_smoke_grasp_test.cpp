#include "weapon_smoke_grasp.h"
#include "weapon_mesh_follow.h"
#include "grenade_throw.h"
#include <cstdio>
#include <cstdlib>
namespace vr=gears_vr;
namespace smoke=gears_weapon_smoke_grasp;
unsigned checks{};
void check(bool value){++checks;if(!value){std::fprintf(stderr,"Smoke mount check %u failed\n",checks);std::exit(1);}}
bool near(vr::Vector a,vr::Vector b){return std::hypot(std::hypot(a.x-b.x,a.y-b.y),a.z-b.z)<1e-6;}
int main(){
    for(double units:{80.,100.,115.,150.})for(double scale:{.75,1.,1.3}){
        smoke::Profile p{};
        check(smoke::marcus115(gears_hand_authored_mount::marcus115_signature,1,units,p,scale));
        vr::Vector centre{};check(smoke::release_centre(p,scale,units,centre));
        // Independently compare the production mesh mount with the throw
        // coordinate conversion for the same raw controller pose. This proves
        // neither actual model contact nor Native/Engine execution.
        std::array<float,16> matrix{};matrix[0]=matrix[5]=matrix[10]=float(scale);matrix[15]=1;
        for(double yaw:{0.,37.,-93.,171.}){
            vr::Viewpoint grip{{120,30,80},{0,yaw,0}};
            gears_weapon::WeaponMeshFollow mount{};
            check(mount.bind_mount(grip,matrix,p.palm_in_mesh,p.wrist_in_mesh,
                vr::rotation_matrix(p.hand.calibration.grip_to_wrist_rotation)));
            vr::Viewpoint mesh{};check(mount.solve(grip,mesh));
            const auto predicted=vr::transform(vr::ue_matrix(grip.rotation),vr::xr_to_ue(centre));
            check(near(mesh.position,{grip.position.x+predicted.x*units,grip.position.y+predicted.y*units,grip.position.z+predicted.z*units}));
        }
    }
    smoke::Profile sentinel{};sentinel.palm_in_mesh={99,98,97};auto p=sentinel;
    check(!smoke::marcus115({},1,100,p));check(near(p.palm_in_mesh,sentinel.palm_in_mesh));
    check(!smoke::marcus115(gears_hand_authored_mount::marcus115_signature,1,100,p,0));
    vr::Vector centre{7,8,9};check(!smoke::release_centre(p,1,0,centre));check(near(centre,{7,8,9}));
    std::printf("Smoke basic mount/throw-origin consistency passed (%u checks); no contact or runtime claim\n",checks);
}
