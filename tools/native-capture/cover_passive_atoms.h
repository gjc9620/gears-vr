#pragma once
#include "cover_skeleton_ik.h"
// Marcus115 C0 observations r272: passive leaves65/67 (parent56), helper106
// (parent0, child107), outside every solved body/hand chain. Retail F6100
// converts raw q directly to matrix (302B SHA088dfc98...), no normalization.
// Admit a non-unit representation ONLY as unchanged rigid auxiliary data.
namespace gears_cover_passive_atoms {
namespace ik=gears_cover_skeleton_ik;
inline bool auxiliary(unsigned bone,int parent,ik::Atom const& atom)noexcept{
    if(!((bone==65||bone==67)?parent==56:bone==106&&parent==0))return false;
    double norm{};for(float q:atom.q){if(!std::isfinite(q))return false;norm+=double(q)*q;}
    if(!std::isfinite(norm)||norm<=0||!ik::detail::finite_vector({atom.translation[0],atom.translation[1],atom.translation[2]})||
       !std::isfinite(atom.scale)||std::abs(double(atom.scale)-1)>0x1p-23)return false;
    const auto m=ik::detail::atom_matrix(atom);
    if(!ik::detail::similarity(m))return false;
    for(unsigned axis=0;axis<3;++axis)if(std::abs(ik::detail::length(ik::detail::row(m,axis))-1)>2e-5)return false;
    return true;
}
inline ik::Status validate(ik::Atom const* local,int const* parents,unsigned count,
                          ik::Matrix const& component,bool cover)noexcept{
    if(!cover)return ik::detail::validate(local,parents,count,component);
    if(!ik::detail::similarity(component))return ik::Status::component;
    for(unsigned i=0;i<count;++i){
        if((!i&&parents[i])||(i&&(parents[i]<0||unsigned(parents[i])>=i)))return ik::Status::parent;
        auto const& a=local[i];ik::Quat q{};
        if((!ik::detail::normalized({a.q[0],a.q[1],a.q[2],a.q[3]},q)&&!auxiliary(i,parents[i],a))||
           !ik::detail::finite_vector({a.translation[0],a.translation[1],a.translation[2]})||!std::isfinite(a.scale)||a.scale<=0)
            return ik::Status::atom;
    }return ik::Status::okay;
}
}
