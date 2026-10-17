#pragma once
#include "death_constant_observer.h"
#include <exception>

// Bounded CPU evidence only; never used for render admission or argument repair.
namespace gears_dlss_route {
using gears_death_constants::Descriptor;
using gears_death_constants::Float4;
using gears_death_constants::Destination;
inline unsigned setter_role(uintptr_t caller){return caller==0xd83a92?0u:caller==0xd83b69?1u:2u;}
inline bool eligible(bool world,bool enabled,bool recording,bool frame_link,bool renderer,bool frozen,
                     uint64_t ticket,uint64_t frozen_ticket,uint64_t generation,uint64_t frozen_world){
    return world&&enabled&&recording&&frame_link&&renderer&&frozen&&ticket&&ticket==frozen_ticket&&generation&&generation==frozen_world;
}
struct View {
    uintptr_t address{},family{},context{};
    gears_unpossessed_quad::NativeView native{};
    Float4 source{},sampling{};std::array<int32_t,4> target{};
    unsigned mask{},eye{};uint32_t marker{};bool eye_valid{},same_current{};
};
struct Setter {
    uintptr_t caller{},rcx{},descriptor_address{},source_address{},owner{};unsigned index{},mask{};
    Descriptor descriptor{};Float4 payload{};Destination before{},after{};
    uint64_t result{};long long entry{},exit{};bool exited{},unwinding{};
};
struct Quad {
    uintptr_t caller{};long long qpc{};View view{};
    std::array<float,8> args{};std::array<unsigned,4> dimensions{};
    std::array<Destination,2> consumed{};std::array<bool,2> descriptor_matches{};
};
struct Row {
    uintptr_t pass{},renderer{},caller{};uint64_t ticket{},world{};uint32_t thread{},result{},flags{};unsigned eye{};
    long long entry{},exit{};bool flags_read{},exited{},unwinding{};View before{},after{};
    std::array<Setter,2> setters{};std::array<Quad,2> quads{};
    unsigned setter_count{},quad_count{},setter_overflow{},quad_overflow{};
};
struct Recorder {
    std::array<Row,8> rows{};unsigned size{};uint64_t missing{},nested{},limited{};
    Row* reserve(bool ready){if(!ready){++missing;return nullptr;}if(size==rows.size()){++limited;return nullptr;}return &rows[size++];}
};
struct Context {uintptr_t pass{},view{};Row* row{};};
// Even bypassed/nested entries replace TLS until exit, preventing inner calls
// from attaching to an outer row. Stack restoration also runs on C++ unwinding.
struct Binding {
    Context*& current;Context* parent;bool nested;
    Binding(Context*& slot,Context& value):current(slot),parent(slot),nested(slot!=nullptr){current=&value;}
    ~Binding(){current=parent;}
    Binding(Binding const&)=delete;Binding& operator=(Binding const&)=delete;
};
}
