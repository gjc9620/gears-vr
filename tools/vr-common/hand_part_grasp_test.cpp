#include "hand_part_grasp.h"
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <limits>

// The optional extension must not change an existing caller's wire layout.
struct LegacyWire {
    unsigned size,version;gears_weapon_hand::Frame source;
    gears_hand_object_grasp::Kind kind;gears_vr::Pose grip;unsigned proximity;
};
using Wire=gears_hand_object_grasp::Request;
static_assert(sizeof(LegacyWire)==sizeof(Wire));
static_assert(offsetof(LegacyWire,source)==offsetof(Wire,source));
static_assert(offsetof(LegacyWire,kind)==offsetof(Wire,kind));
static_assert(offsetof(LegacyWire,grip)==offsetof(Wire,grip));
static_assert(offsetof(LegacyWire,proximity)==offsetof(Wire,proximity));
int main(){
    namespace g=gears_hand_object_grasp;namespace f=gears_hand_fingers;
    for(auto kind:{g::Kind::magazine,g::Kind::loading,g::Kind::starter,g::Kind::support})
        assert(g::finger_shape(kind,g::Preset::automatic)==static_cast<f::Shape>(kind));
    assert(g::finger_shape(g::Kind::magazine,g::Preset::precision)==f::Shape::loading);
    assert(g::finger_shape(g::Kind::magazine,g::Preset::cylinder)==f::Shape::starter);
    assert(g::finger_shape(g::Kind::support,g::Preset::support_surface)==f::Shape::authored);
    assert(!g::valid_for(g::Kind::magazine,g::Preset::support_surface));
    assert(g::finger_shape(g::Kind::magazine,g::Preset::support_surface)==f::Shape::free);
    g::Target target{g::Kind::magazine,{{.01,.02,.03},{}},1.,true,g::Preset::precision};
    gears_vr::Pose physical{},visual{};
    gears_hand_wrist::Calibration calibration{};calibration.known=true;
    assert(g::apply(target,physical,calibration,100,visual));
    g::BlendState blend{};
    for(int i=0;i<=8;++i)blend.update(target,physical,calibration,100,1000000000+i*20000000,visual);
    assert(blend.preset==g::Preset::precision&&blend.weight==1);
    target.preset=g::Preset::cylinder;
    assert(blend.update(target,physical,calibration,100,1180000000,visual));
    assert(blend.preset==g::Preset::cylinder&&blend.weight<.2); // A different grasp starts the existing blend.
    const auto untouched=physical;
    for(int i=0;i<9;++i)blend.update({},physical,calibration,100,1200000000+i*20000000,visual);
    assert(blend.kind==g::Kind::none&&blend.preset==g::Preset::automatic&&blend.weight==0);
    assert(!std::memcmp(&physical,&untouched,sizeof physical));
    target.preset=static_cast<g::Preset>(99);
    assert(!g::apply(target,physical,calibration,100,visual));
    assert(!blend.update(target,physical,calibration,100,1400000000,visual));
    // Caller-supplied local rotations stay cosmetic and survive the normal
    // relative-pose release blend; no gun module is included by this test.
    g::Target authored{g::Kind::support,{{.01,.02,.03},{}},1.,true,g::Preset::support_surface};
    assert(!g::apply(authored,physical,calibration,100,visual)); // No table.
    authored.has_authored_pose=true;
    for(unsigned i=0;i<15;++i)authored.authored_pose[i]=gears_vr::normalized({.01*i,0,0,1});
    assert(g::apply(authored,physical,calibration,100,visual));
    g::BlendState fitted{};
    for(int i=0;i<=8;++i)assert(fitted.update(authored,physical,calibration,100,1500000000+i*20000000,visual)||i==0);
    assert(fitted.weight==1&&fitted.preset==g::Preset::support_surface);
    assert(!std::memcmp(fitted.authored_pose.data(),authored.authored_pose.data(),sizeof authored.authored_pose));
    auto invalid=authored;invalid.authored_pose[7].x=std::numeric_limits<double>::quiet_NaN();
    assert(!g::apply(invalid,physical,calibration,100,visual));
    assert(!fitted.update(invalid,physical,calibration,100,1680000000,visual));
    for(int i=0;i<9;++i)fitted.update({},physical,calibration,100,1700000000+i*20000000,visual);
    assert(fitted.kind==g::Kind::none&&fitted.preset==g::Preset::automatic&&fitted.weight==0);
    authored.kind=g::Kind::magazine;authored.preset=g::Preset::authored;
    assert(g::finger_shape(authored.kind,authored.preset)==f::Shape::authored);
    assert(g::apply(authored,physical,calibration,100,visual));
    authored.has_authored_pose=false;
    assert(!g::apply(authored,physical,calibration,100,visual));
    assert(!std::memcmp(&physical,&untouched,sizeof physical));
    std::puts("Optional part grasp: legacy V2 wire layout/default shapes, explicit precision/cylinder presets, transition/release and untouched physical input passed; no skin/game proof");
}
