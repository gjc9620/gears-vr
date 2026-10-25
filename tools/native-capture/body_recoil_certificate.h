#pragma once
#include <array>
#include "cover_pose_code_certificate.h"
namespace gears_body_recoil_certificate {
// Historical identifiers. Two adjacent Compute chunks are ONE invocation.
inline constexpr std::array<gears_cover_pose_certificate::Chunk,3> chunks{{
    {0x3dfc44,37,"d13b50624167c4722abc0e9ae7d8830112fc07e8c062a2d31c065cb6908cc62d"},
    {0x3a3d2c,390,"f2e4a1bd39197576fceec4a450f0c826ab45d9a81a5506cb224af5443ec0e979"},
    {0x3a3eb2,2037,"ef51b8ef288de648edd43f755a87022fa0431f35d84130e76cf54a7677cc3be3"},
}};
inline constexpr uintptr_t anim_tree_class_slot=0x1c21850;
}
