#pragma once
#include <array>
#include <cstdint>
#include <exception>
#include <functional>
#include "ngx_stereo_native_bridge.h"
#include "ngx_stereo_native_api.h"
#include "ngx_typed_resources.h"
#include "unpossessed_quad_observer.h"

// Raw CPU wrapper observations only. Resource pointers have no inferred COM
// type, results have no inferred success convention, and samples are not an
// atomic observation of the values later consumed by the tail callee.
namespace gears_ngx_route {
constexpr uintptr_t create_rva=0xbe5b5c,evaluate_rva=0xbe5c58;
constexpr unsigned create_size=0xf9,evaluate_size=0x561;
constexpr const char* create_sha256="53709f650d84bd1ea4cc1ae5e25eb8ef7f3462102fe51d07b790d96c48734879";
constexpr const char* evaluate_sha256="21868e6f8203578f5c4ef689fc4f66a5c79e13fdb243eebe2eb81cc7d6309441";
constexpr uintptr_t handle_cell_rva=0x1c20bd0,parameter_cell_rva=0x1c20bd8;
constexpr std::array<unsigned,4> resource_offsets{0,8,0x18,0x20};
// Render width/height, Reset, jitter X/Y bits, raw MV scale X/Y bits,
// Color, Depth, MV and Output subrect base X/Y pairs.
constexpr std::array<unsigned,15> value_offsets{0x30,0x34,0x38,0x28,0x2c,0x3c,0x40,0x60,0x64,0x68,0x6c,0x70,0x74,0x88,0x8c};
// Association describes the executing TLS view, not the logical eye whose
// queued NGX parameters are being evaluated. It never admits render work.
struct Ownership {
    bool associated{};unsigned mask{},eye{};uint64_t world{},ticket{};
    uintptr_t renderer{},context{},view{},views{};int count{};
};
struct Sample {
    Ownership ownership{};unsigned slots_mask{},fields_mask{};uintptr_t handle{},parameters{};
    std::array<uint32_t,7> create{};std::array<uintptr_t,4> resources{};std::array<uint32_t,15> values{};
};
struct Row {
    bool evaluate{},exited{},returned{},unwinding{},nested{};unsigned depth{},thread{};
    uintptr_t caller{},structure{};std::array<uintptr_t,5> args{};uint64_t result{};
    long long entry{},exit{};Sample before{},after{};
};
struct Recorder {
    std::array<Row,4> creates{};std::array<Row,8> evaluations{};unsigned create_count{},evaluate_count{};
    uint64_t limited_create{},limited_evaluate{},nested{},bypassed{};
    Row* reserve(bool eval){auto& count=eval?evaluate_count:create_count;const unsigned limit=eval?8u:4u;
        if(count==limit){++(eval?limited_evaluate:limited_create);return nullptr;}
        return eval?&evaluations[count++]:&creates[count++];}
};
}
