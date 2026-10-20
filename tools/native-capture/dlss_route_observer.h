#pragma once
#include "death_constant_observer.h"
#include <exception>

// Bounded CPU route diagnostics. The separate private control may re-read the
// same native layout; diagnostic rows themselves never authorize admission.
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
struct InputResource {
    uintptr_t object{},native{},canonical{};uint64_t width{};unsigned height{},dimension{},format{},depth{},mips{},samples{};
    long result{};bool queried{};
};
struct InputSample {
    View view{};Descriptor descriptor{};unsigned mask{},slot{};uint32_t selector{},public_index{};unsigned char highwater{};
    uintptr_t shader_handle{},public_global{},public_pointer{},public_wrapper{},public_object{};
    std::array<unsigned char,0x60> owner_head{};InputResource resource{};
};
struct InputCheckpoint {
    bool present{};uintptr_t caller{},raw_list{},context_list{},canonical_list{};uint64_t generation{};
    unsigned kind{},elements{},instances{};long long qpc{};InputSample sample{};bool list_matches{};
};
struct InputBind {
    bool present{},exited{},unwinding{},owner_descriptor{},output_view_matches{},tracked_slot_known{};
    uintptr_t caller{},descriptor_address{},view_address{},shader_owner{};uint32_t selection{};int32_t alternate{};
    uint64_t result{};long long entry{},exit{};unsigned tracked_slot{},duplicates{};
    InputSample before{},after{};InputCheckpoint quad{},draw{};
};
struct Row {
    uintptr_t pass{},renderer{},caller{};uint64_t ticket{},world{};uint32_t thread{},result{},flags{};unsigned eye{};
    long long entry{},exit{};bool flags_read{},exited{},unwinding{};View before{},after{};
    std::array<Setter,2> setters{};std::array<Quad,2> quads{};
    InputBind input{};
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
