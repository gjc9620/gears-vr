#pragma once
#include "weapon_pose_solver.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace gears_weapon {

// Inspection-only approximation of the visible chain loop in the verified
// COGAssaultRifle LOD0 mesh (source SHA256 df356ff3dda45e060f87b5ed94c806d96bbfe36b74b21a0f1d2032596f9dcf4d).
// The chain-blur material is a planar ring at local X=0, Y=26.30..75.86,
// Z=-7.21..10.31. These sparse edge points are not original mesh vertices.
// A live adapter must still verify the current retail mesh, same-tick matrix
// and visible pose before it may treat the result as a cutting surface.
inline constexpr std::array<Vector,12> lancer_chain_edge_local_raw{{
    {0,28,5},{0,32,9},{0,55,9},{0,69,8},
    {0,74,6},{0,75,2},{0,74,-1},{0,69,-5},
    {0,62,-7},{0,49,-6.5},{0,35,-5},{0,29,-2}
}};

struct LancerBladeOutline {
    std::array<Vector,lancer_chain_edge_local_raw.size()> world_raw{};
    bool verified_current_mesh{};
};

// UE3 LocalToWorld stores the local basis in rows and translation at 12..14.
// Reject a stale, sheared, mirrored or implausibly scaled component matrix.
inline bool lancer_chain_outline_from_mesh(
    std::array<float,16> const& matrix,bool current_mesh_verified,
    LancerBladeOutline& out) noexcept {
    if(!current_mesh_verified)return false;
    for(float value:matrix)if(!std::isfinite(value))return false;
    if(std::abs(matrix[3])>1e-4||std::abs(matrix[7])>1e-4||
       std::abs(matrix[11])>1e-4||std::abs(matrix[15]-1)>1e-4)return false;
    double scale[3]{};
    for(int axis=0;axis<3;++axis){
        scale[axis]=std::hypot(std::hypot(double(matrix[axis*4]),
            double(matrix[axis*4+1])),double(matrix[axis*4+2]));
        if(scale[axis]<.1||scale[axis]>10)return false;
    }
    for(int i=0;i<3;++i)for(int j=i+1;j<3;++j){
        if(std::abs(scale[i]-scale[j])>(std::max)(scale[i],scale[j])*.02)return false;
        double alignment=0;
        for(int k=0;k<3;++k)
            alignment+=double(matrix[i*4+k])*matrix[j*4+k]/(scale[i]*scale[j]);
        if(std::abs(alignment)>1e-3)return false;
    }
    const double determinant=(
        double(matrix[0])*(double(matrix[5])*matrix[10]-double(matrix[6])*matrix[9])-
        double(matrix[1])*(double(matrix[4])*matrix[10]-double(matrix[6])*matrix[8])+
        double(matrix[2])*(double(matrix[4])*matrix[9]-double(matrix[5])*matrix[8]))/
        (scale[0]*scale[1]*scale[2]);
    if(determinant<.999||determinant>1.001)return false;
    LancerBladeOutline candidate{};
    for(std::size_t i=0;i<lancer_chain_edge_local_raw.size();++i){
        const auto p=lancer_chain_edge_local_raw[i];
        candidate.world_raw[i]={
            matrix[12]+double(matrix[0])*p.x+double(matrix[4])*p.y+double(matrix[8])*p.z,
            matrix[13]+double(matrix[1])*p.x+double(matrix[5])*p.y+double(matrix[9])*p.z,
            matrix[14]+double(matrix[2])*p.x+double(matrix[6])*p.y+double(matrix[10])*p.z};
        if(!gears_vr::finite(candidate.world_raw[i]))return false;
    }
    candidate.verified_current_mesh=true;
    out=candidate;
    return true;
}

} // namespace gears_weapon
