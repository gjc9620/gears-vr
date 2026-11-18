#pragma once
#include "cover_grab_pose.h"
#include "cover_l2_grab_points.h"
#include "cover_input_owner.h"
#include <cstdint>

// Pure adaptation of already-authenticated original L2 reference positions.
// The Native caller must bind the actual Scene/actor/link and owner ticket.
// An L2 viewpoint is not a surface, collision result or permission to move.
namespace gears_cover_l2_contact {
namespace vr=gears_vr;namespace grab=gears_cover_grab_pose;
struct Observation {
    std::uint64_t frame{},cover_link{};
    std::int64_t observed_ns{};
    vr::CoverEpochs epochs{};
    vr::CoverL2GrabPoints points{};
    vr::CoverViewAnchor wall{};
    double units_per_metre{};
};
enum class Status {prepared,alias,inactive,binding,profile,geometry,stale};
inline bool overlaps(void const* a,std::size_t an,void const* b,std::size_t bn) noexcept {
    const auto x=reinterpret_cast<std::uintptr_t>(a),y=reinterpret_cast<std::uintptr_t>(b);
    return x>UINTPTR_MAX-an||y>UINTPTR_MAX-bn||!(x+an<=y||y+bn<=x);
}
inline Status prepare(Observation const& source,vr::CoverInputGrabSnapshot const& owner,
    grab::Profile const& profile,std::int64_t now,grab::Contact& out) noexcept {
    if(overlaps(&source,sizeof source,&out,sizeof out)||overlaps(&owner,sizeof owner,&out,sizeof out)||
       overlaps(&profile,sizeof profile,&out,sizeof out))return Status::alias;
    if(!source.frame||!source.cover_link||!owner.lifecycle||source.frame!=owner.frame||
       owner.frame!=owner.cover.sequence||!grab::same(source.epochs,owner.cover.epochs)||
       source.observed_ns!=owner.cover.observed_ns)return Status::binding;
    if(!grab::fresh(source.observed_ns,now))return Status::stale;
    auto const& edge=owner.cover.edge;
    if(!owner.cover.cover_valid||!owner.cover.interaction_enabled||!edge.allowed||edge.id<1||edge.id>3)
        return Status::inactive;
    const unsigned index=unsigned(edge.id-1);
    constexpr vr::CoverEdgeKind kinds[]={vr::CoverEdgeKind::left,vr::CoverEdgeKind::right,vr::CoverEdgeKind::top};
    if(edge.kind!=kinds[index])return Status::binding;
    if(!vr::cover_l2_allowed(source.points,index))return Status::inactive;
    if(!grab::profile_valid(profile)||profile.kind!=edge.kind)return Status::profile;
    if(!vr::cover_view_basis(source.wall)||!std::isfinite(source.units_per_metre)||
       source.units_per_metre<10||source.units_per_metre>500)return Status::geometry;
    grab::Contact next{};
    // Existing Key's generation slot names the cover-reference generation
    // here. It is explicitly not a physics shape or surface certificate.
    next.key={source.epochs,edge.id,source.cover_link,source.epochs.cover,profile.id};
    if(!grab::valid(next.key))return Status::binding;
    const double yaw=std::atan2(source.wall.forward_world.y,source.wall.forward_world.x);
    next.edge_world={source.points.points[index].world,{0,0,std::sin(yaw*.5),std::cos(yaw*.5)}};
    next.observed_ns=source.observed_ns;
    out=next;return Status::prepared;
}
} // namespace gears_cover_l2_contact
