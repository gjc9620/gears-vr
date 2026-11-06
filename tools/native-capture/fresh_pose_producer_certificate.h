#pragma once
#include "cover_pose_code_certificate.h"
// Exact historical 911ab chunks; startup checks the CURRENT unpatched image.
namespace gears_fresh_pose_certificate {
inline constexpr std::array<gears_cover_pose_certificate::Chunk,3> chunks{{
    {0xf7940,2569,"418cdc8a350ce1a4b176126345e5c890aef09d4ed9b9599e7f1d7d5a3d6e91b0"},
    {0x8a110e,296,"28b53741cceb91938610e94a9397b544b243dd7688b2099efdd97485935a8902"},
    {0x109970,5433,"ba3725f89400b7cfa88231b8870721f8cf3b599c7717ee07ae6e124a8dc7eb67"},
}};
inline constexpr std::array<gears_cover_pose_certificate::Chunk,3> sequence_pose{{
    {0xf8af4,401,"f3ef3bbe7ad7ec49b6f0ab61184a54027bdf2871701e524ca3e4104c920f8d86"},
    {0x8a127e,96,"b8d6eca20b47ebe27b651c6f316e7b74a4558d1f154a492e0b9aca83ca792bf2"},
    {0xf7858,119,"851d8d464e2f44b132e052988e807d97cdca1253d27866d24b499f9f8ccd4341"},
}};
inline constexpr std::array<gears_cover_pose_certificate::Chunk,2> per_bone_pose{{
    {0xfacb0,4055,"660fe88e7f2237e6aec7f288e2c34ffb69212cf863ce89ce783551ce4e27f7a0"},
    {0x8a1998,982,"b67b229ac5bd3ca5ff0a1b4c6c1e3a3a11f1ead720a0830a3ae37f1d0e5785ee"},
}};
inline constexpr std::array<uintptr_t,3> entries{{0xf7940,0x109970,0xfacb0}};
inline constexpr std::array<gears_cover_pose_certificate::Chunk,7> sequence_set_pose{{
    {0x2fde20,1804,"e568e982c56e616ea136c23114e9f84cf6158e79d5f1a903a1130220053aa26f"},
    {0x8fd40a,159,"82a8127171744f80178a4f5694802c99054d98a5d6d46e9f4948850aaab23ffe"},
    {0xf6cbc,205,"d7a7cda5d19f054329d3c5668f5c3510ea173507c2d7fc7d89b4dd11379c0274"},
    {0x8a0eb4,35,"381a851d0ed79633df3eb32e5591307174daddebf522442a37231dc4423d5240"},
    {0xf7858,119,"851d8d464e2f44b132e052988e807d97cdca1253d27866d24b499f9f8ccd4341"},
    {0xf6d8c,203,"d19a42190a4a4df8ae5817632cc4ea68faf20274a29a362ece14c48da3b30f7a"},
    {0x8a0ed8,61,"a6bb8715572581448702662193cd5f918b3b05882e21bfcfe08fad765cfadb61"},
}};
inline constexpr std::array<uintptr_t,2> returns{{0xf9341,0xf95aa}},calls{{0xf933b,0xf95a5}};
inline constexpr std::array<unsigned,2> call_sizes{{6,5}};
inline constexpr std::array<std::array<unsigned char,32>,3> prefixes{{
{{0x48,0x8b,0xc4,0x48,0x89,0x58,0x10,0x48,0x89,0x70,0x18,0x4c,0x89,0x48,0x20,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57,0x48,0x81,0xec,0x00,0x01,0x00,0x00,0xc5}},
{{0x48,0x8b,0xc4,0x48,0x89,0x58,0x18,0x48,0x89,0x70,0x20,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57,0x48,0x81,0xec,0xf0,0x02,0x00,0x00,0xc5,0xf8,0x29,0x70,0xc8}},
{{0x40,0x53,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x48,0x81,0xec,0xe8,0x01,0x00,0x00,0x48,0x8b,0xbc,0x24,0x40,0x02,0x00,0x00,0x4c,0x8b,0xe9,0x48,0x8b,0x49,0x60}}
}};
}
