// GT-only original bone measurement bracketed by the actual committed C0
// endpoint. No pose, input, lease, animation or inventory writer lives here.
namespace boomshot_contact {
namespace data=gears_boomshot_contact;namespace hand=gears_weapon_hand;
data::Measurement measurement{};
bool sample(hand::Frame const& f,data::Geometry& out) noexcept {
    out={};gears_rig_tracking::Frame rig{};gears_committed_hand::Frame actual{};
    if(!weaponhand::current_gt(nullptr)||!handrig::read_endpoint(actual)||
       handrig::read(rig,0,unsigned(f.stereo.source)).reply!=gears_rig_tracking::Reply::accepted)return false;
    return data::evaluate(measurement,f,rig,actual,vrequip::model.hold_generation(),weaponhand::clock_ns(nullptr),out);
}
bool measure(hand::Frame const& f) noexcept {
    if(!weaponhand::current_gt(nullptr))return false;
    measurement={};
    if(!boomshot_carrier::selected(f))return false;
    gears_committed_hand::Frame before{},after{};gears_rig_tracking::Frame rig{};
    if(!handrig::read_endpoint(before)||
       handrig::read(rig,0,unsigned(f.stereo.source)).reply!=gears_rig_tracking::Reply::accepted)return false;
    hammerburst_animation::Parts parts{},closing{};data::P root{},barrel{},relative{};double size{},barrel_size{};
    const auto now=weaponhand::clock_ns(nullptr);
    if(!gears_committed_hand::matches(before,f,rig,vrequip::model.hold_generation(),now,unsigned(f.stereo.source)==1)||
       !boomshot_parts::read(f,parts)||!hammerburst_runtime::matrix(parts.world[0],root,size)||
       !hammerburst_runtime::matrix(parts.world[2],barrel,barrel_size)||std::abs(size-barrel_size)>.001||
       !boomshot_parts::read(f,closing)||parts.world!=closing.world||
       !hand::same(parts.component,closing.component)||!hand::same(parts.asset,closing.asset)||
       !handrig::read_endpoint(after)||!data::same_endpoint(before,after)||!nativeequipment::current(nullptr,f))return false;
    relative.position=data::local(root,barrel.position,size);
    relative.orientation=gears_vr::normalized(gears_vr::multiply(gears_vr::inverse(root.orientation),barrel.orientation));
    data::Measurement value{f,after,parts.component,parts.asset,root,relative,size,weaponhand::clock_ns(nullptr),true};
    data::Geometry result{};
    if(!data::evaluate(value,f,rig,after,vrequip::model.hold_generation(),weaponhand::clock_ns(nullptr),result))return false;
    measurement=value;return true;
}
void clear() noexcept {if(weaponhand::current_gt(nullptr))measurement={};}
}
