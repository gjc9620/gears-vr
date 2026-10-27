#pragma once
#include "cover_pose_code_certificate.h"
// One original scalar getter, observed in frozen Native25 receipts.
// Full saved fixtures stay in ignored analysis; no permission follows this hash.
namespace gears_cover_future_g_certificate {
inline constexpr gears_cover_pose_certificate::Chunk getter{0x516100,9,"4e7e32463cdcd515d8110b2622016b149a68c350316163a474744f77969ac04b"};
inline constexpr std::array<unsigned char,9> prefix{{0xc5,0xfa,0x10,0x81,0xac,0,0,0,0xc3}};
inline constexpr uintptr_t call_rva=0x12946e,return_rva=0x129474;
inline constexpr unsigned call_size=6;
inline constexpr auto caller=gears_cover_pose_certificate::chunks[0]; // Full original Compose chunk.
}
