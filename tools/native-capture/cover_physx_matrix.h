#pragma once
// Original PhysX row-major3x3 followed by translation, in PhysX units.
namespace gears_cover_physx_transform {
struct Matrix34 {float values[12]{};};
static_assert(sizeof(Matrix34)==48&&alignof(Matrix34)==alignof(float));
}
