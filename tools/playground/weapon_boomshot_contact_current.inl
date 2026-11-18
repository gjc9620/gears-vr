// Read-only hand-observation path. Current original gun matrices replace the
// old same-Frame GT cache; raw left input still comes from exact Native Rig.
namespace boomshot_contact {
bool observe(gears_weapon_hand::Frame const& f,gears_boomshot_contact::Geometry& out) noexcept {
    namespace data=gears_boomshot_contact;out={};
    boomshot_pose::Source pose{};gears_rig_tracking::Frame rig{};
    const auto source=unsigned(f.stereo.source);
    if(!weaponhand::current_gt(nullptr)||!boomshot_pose::ready(f,pose)||
       handrig::read(rig,0,source).reply!=gears_rig_tracking::Reply::accepted||
       gears_equipment_rig::match(f,rig,weaponhand::clock_ns(nullptr),source)!=gears_equipment_rig::Status::matched)return false;
    data::P root{},barrel{},relative{};double size{},barrel_size{};
    if(!hammerburst_runtime::matrix(pose.world[0],root,size)||!hammerburst_runtime::matrix(pose.world[2],barrel,barrel_size)||
       std::abs(size-barrel_size)>.001)return false;
    relative.position=data::local(root,barrel.position,size);
    relative.orientation=gears_vr::normalized(gears_vr::multiply(gears_vr::inverse(root.orientation),barrel.orientation));
    data::Geometry result{};
    if(!data::spatial(root,relative,size,rig,result))return false;
    // This is the actual data-read time, not a renewed input/tracking ticket.
    // Native's original Scene/source timestamps above retain the100ms gate.
    result.geometry_acquired_ns=pose.observed_begin_ns;result.input_sync_ns=f.controllers.sync_ns;
    out=result;return true;
}
}
