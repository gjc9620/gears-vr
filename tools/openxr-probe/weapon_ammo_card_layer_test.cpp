// Header compilation and layer construction only; no XR session or GPU work.
#include "weapon_ammo_card_renderer.h"
#include <cassert>
#include <cstdio>

int main(){
    using namespace weapon_ammo_card;
    Plan plan{};plan.identity={1,2,3,4};plan.frame={5,6,7,8};
    assert(original_firearm_counts(60,13,240,false,plan.counts));
    plan.pose_in_local_m.position={.2,-.1,-.6};plan.width_m=.15;plan.height_m=.05;
    const auto space=reinterpret_cast<XrSpace>(std::uintptr_t(1));
    const auto chain=reinterpret_cast<XrSwapchain>(std::uintptr_t(2));
    XrCompositionLayerQuad quad{};assert(make_layer(plan,space,chain,quad));
    assert(quad.type==XR_TYPE_COMPOSITION_LAYER_QUAD&&quad.space==space&&quad.eyeVisibility==XR_EYE_VISIBILITY_BOTH);
    assert(quad.layerFlags==0&&!quad.next&&quad.subImage.swapchain==chain&&!quad.subImage.imageArrayIndex);
    assert(quad.subImage.imageRect.offset.x==0&&quad.subImage.imageRect.offset.y==0);
    assert(quad.subImage.imageRect.extent.width==768&&quad.subImage.imageRect.extent.height==256);
    assert(std::abs(quad.pose.position.x-.2)<1e-6&&std::abs(quad.pose.position.z+.6)<1e-6);
    assert(std::abs(quad.size.width/quad.size.height-3)<1e-6);
    assert(quad.pose.orientation.w==1); // Fixed gun pose, no HMD lookup.
    const auto old=quad;plan.width_m=0;assert(!make_layer(plan,space,chain,quad));
    assert(quad.size.width==old.size.width&&quad.space==old.space);
    plan.width_m=.15;assert(!make_layer(plan,XR_NULL_HANDLE,chain,quad));
    assert(!make_layer(plan,space,XR_NULL_HANDLE,quad));
    plan.pose_in_local_m.position.x=(std::numeric_limits<double>::max)();
    assert(!make_layer(plan,space,chain,quad));
    std::puts("WeaponAmmoCardLayerTest passed (no XR/GPU session)");
}
