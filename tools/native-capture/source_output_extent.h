#pragma once
#include <array>
#include <cstdint>

// Frozen scalar evidence from the authenticated original source, before the
// copied family's target is replaced. Addresses identify that source frame;
// they are never dereferenced later and grant no resource ownership.
namespace gears_source_output {
struct Extent {
    std::uint64_t ticket{},world{};
    std::uintptr_t source{},family{},target{};
    std::array<int,4> rectangle{};
    bool valid()const{return ticket&&world&&source&&family&&target&&rectangle[0]==0&&rectangle[1]==0&&
        rectangle[2]>=64&&rectangle[2]<=8192&&rectangle[3]>=64&&rectangle[3]<=8192;}
    bool matches(std::uint64_t current_ticket,std::uint64_t current_world)const{
        return valid()&&ticket==current_ticket&&world==current_world;
    }
};
inline Extent freeze(std::uint64_t ticket,std::uint64_t world,std::uintptr_t source,std::uintptr_t family,
    std::uintptr_t target,std::array<int,4> const& original,std::array<int,4> const& left,std::array<int,4> const& right){
    Extent out{ticket,world,source,family,target,original};
    return original==left&&original==right&&out.valid()?out:Extent{};
}
}
