#pragma once
#include <array>
#include <cstdint>

// Observation data only. No mount/aim, getter-equivalence or firing grant.
namespace gears_weapon_lancer_muzzle {
using Address=std::uint64_t;
enum class SocketStep:std::uint32_t {not_run,entry,tables,identity,deleted,configured_name,
    array_descriptor,array_inner,array_value,socket_identity,socket_name,bone_name,
    location,rotation,repeat_complete,final_identity,final_structure,final_property,
    final_name,final_tables,completed};
struct SocketDiagnostic {
    SocketStep step{SocketStep::not_run};
    std::uint32_t pass{},metadata_reason{UINT32_MAX};
};
struct Name {std::uint32_t index{},number{};};
struct Socket {
    Address object{};
    Name configured_name{},socket_name{},bone_name{};
    std::array<float,3> location{};
    std::array<std::int32_t,3> rotation{};
};
inline bool same(Socket const& a,Socket const& b) noexcept {
    const auto name=[](Name x,Name y){return x.index==y.index&&x.number==y.number;};
    return a.object==b.object&&name(a.configured_name,b.configured_name)&&
        name(a.socket_name,b.socket_name)&&name(a.bone_name,b.bone_name)&&
        a.location==b.location&&a.rotation==b.rotation;
}
} // namespace gears_weapon_lancer_muzzle
