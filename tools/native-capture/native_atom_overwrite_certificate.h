#pragma once
#include <array>
#include <cstdint>
namespace gears_atom_overwrite_certificate {
struct Chunk {std::uintptr_t rva;unsigned size;char const* sha256;};
inline constexpr std::array<Chunk,7> chunks{{
    {0x129cc0,2132,"d10570b6a96cc9d16c3938e2999be6288d7603127ac402eb908f6e57af0a2fa9"},
    {0x8a9738,1179,"4e81e649c3aa5c9fb28188adfc4d09b3837d46e73ac65cdd467b09b66617c70e"},
    {0x3b57b4,188,"b702a3235d03ea344670eb4e757b88953a2f7e9f0c86ffda270d85d47535808d"},
    {0xc89a78,208,"7abbfe1184e51c1e483bedbb9e527c73a43c83395173875edb652c8999ad4a84"},
    {0xc89b48,213,"b6f1e8fd5a87dbb79a39d69835faae5cf3aee530e80e8446489768d2a33a5fea"},
    {0xe08a0,950,"3c2edcd6aff2c9d8890193a5b287665c529443f256c036ba23b824196ca36c86"},
    {0x89cff2,21,"a0abe652fca63f3fd9a9813f2d17d6f1f99e76510fec6518d1ba3d2d079aea5a"},
}};
inline constexpr std::array<unsigned char,32> prefix{{0x48,0x8b,0xc4,0x48,0x89,0x58,0x10,0x48,0x89,0x70,0x18,0x48,0x89,0x78,0x20,0x55,0x41,0x56,0x41,0x57,0x48,0x8d,0x68,0xb1,0x48,0x81,0xec,0xf0,0x00,0x00,0x00,0x83}};
inline constexpr std::array<unsigned char,5> call{{0xe8,0x4d,0x92,0x04,0x00}};
inline constexpr std::uintptr_t entry=0x129cc0,call_rva=0xe0a6e,return_rva=0xe0a73;
}
