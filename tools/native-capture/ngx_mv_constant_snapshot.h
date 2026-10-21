#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
namespace gears_ngx_mv_constants {
constexpr std::size_t bytes=160,ring_bytes=1024u*1024u;
using Data=std::array<unsigned char,bytes>;
inline bool bounded_upload(uint64_t offset,uint64_t used,uint64_t mapped,uint64_t gpu){
    return used>=bytes&&used<=65536&&offset%256==0&&offset<=ring_bytes-used&&mapped&&gpu&&
        offset<=UINT64_MAX-mapped&&bytes-1<=UINT64_MAX-(mapped+offset)&&offset<=UINT64_MAX-gpu;
}
inline bool same(Data const& a,Data const& b){return !std::memcmp(a.data(),b.data(),bytes);}
inline bool range_contains(uint64_t handle,uint64_t start,uint32_t stride,uint32_t count){
    return stride&&count&&handle>=start&&(handle-start)%stride==0&&(handle-start)/stride<count;
}
// Only table slot zero is joined. The authenticated root layout separately
// establishes that this slot is CB0; this arithmetic supplies no layout proof.
inline bool copied_slot_zero(uint64_t gpu,uint64_t gpu_base,uint64_t cpu_base,uint32_t stride,
                             uint64_t copied_cpu,uint32_t copied_count){
    return gpu&&gpu_base&&cpu_base&&stride&&copied_count&&copied_count<=8&&gpu>=gpu_base&&
        (gpu-gpu_base)%stride==0&&gpu-gpu_base<=UINT64_MAX-cpu_base&&cpu_base+(gpu-gpu_base)==copied_cpu;
}
}
