#pragma once
// Exact historical chunks also matched in Actual130; require current bytes again.
namespace gears_ui_scene_code {
struct Span {std::uintptr_t rva;unsigned size;char const* sha256;};
inline constexpr Span spans[]={
    {0x296188,139,"2b27c1c70b9bc3b6cf2b550e19cb7eafeddb18b70aeaf8f97ac544a8da07f6af"},
    {0x2c6ecc,1509,"a897d8ec9e04baf27beb8af9cd3383b32b44d0edf3f6eafa361f8284f1e2bb80"},
    {0x2c7650,587,"d680969b30fdd84e097aa1e2f53fd014771b6fa28626ac1d9626e4c34c4f4153"},
    {0x2c789c,85,"c03b63158828241d5253e1214a35c641d10f0011460b2a87ce391b0054e14e36"},
    {0x138d70,803,"005f0ca921eaeae189835c7e86b63fea828c229b17890975ae46387798c297c5"},
    {0x8aacbc,45,"a430c7abd66d2894555eaf775a3e9284d0f9598866d90d84f3a16f891678f104"},
};
}
