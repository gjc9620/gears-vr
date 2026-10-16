#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include "crosshair_writer_view.h"

// Owning diagnostic assignment association only. No camera/aim certification.
namespace gears_crosshair_scene {
struct Metadata {
    std::uintptr_t controller{},pawn{},weapon{},root_frame{};
    std::uint64_t world{},tick{};
    std::uint32_t thread{};
    std::int64_t begin_qpc{},assignment_qpc{},finish_qpc{},scene_qpc{};
    std::array<float,3> point{};
    std::uint64_t invocation{},ticket{};
    std::uintptr_t local{},family{},source{},owner{};
    std::uint32_t stamp{};
    std::array<unsigned char,128> matrices{};
    gears_crosshair_view::Sample writer_view{};
};
template<class Key> bool associate(Metadata& value,Key const& key,std::uint64_t invocation,
    std::uint32_t thread,std::uint64_t world,std::uintptr_t controller,std::uintptr_t pawn) {
    if(!invocation||!thread||value.thread!=thread||!world||value.world!=world||
       !value.tick||!value.weapon||!value.root_frame||value.controller!=controller||value.pawn!=pawn||
       !controller||!pawn||!key.local||!key.family||!key.source||key.owner!=key.family||
       value.begin_qpc<=0||value.assignment_qpc<value.begin_qpc||
       value.finish_qpc<value.assignment_qpc||value.scene_qpc<value.finish_qpc||value.invocation)return false;
    value.invocation=invocation;value.local=key.local;value.family=key.family;value.source=key.source;
    value.owner=key.owner;value.stamp=key.stamp;value.matrices=key.matrices;return true;
}
// Consume even on rejection. Context is the sole transport to this attempt.
template<class Key> std::optional<Metadata> take(std::optional<Metadata>& pending,Key const& key,
    std::uint64_t invocation,std::uint32_t thread) {
    auto value=pending;pending.reset();
    if(!value||!invocation||value->invocation!=invocation||!thread||value->thread!=thread||
       value->ticket||value->local!=key.local||value->family!=key.family||value->source!=key.source||
       value->owner!=key.owner||value->stamp!=key.stamp||value->matrices!=key.matrices)return {};
    return value;
}
}
