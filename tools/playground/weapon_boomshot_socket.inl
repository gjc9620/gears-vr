#include "weapon_boomshot_socket_metadata.inl"
namespace boomshot_socket {
struct Observation {Source source{};gears_vr::Pose world{};hand::FrameKey key{};};
bool read(hand::Frame const& f,Observation& out) noexcept {
    lancermain_asset::Evidence asset{},after_asset{};
    if(!nativeequipment::current(nullptr,f)||lancermain_asset::read(f,asset,lancermain_asset::AssetScope::boomshot_projectile)!=lancermain_asset::Status::ready)return false;
    meta::Snapshot snapshot{};snapshot.module_base=base;meta::Reader r{snapshot,meta::Stage::final_check};
    Reads reads{};Observation candidate{};Source after{};
    if(!meta::tables(base,snapshot.objects,snapshot.names)||!metadata(f,asset,r,reads,candidate.source))return false;
    playground::Args call(asset.component.address,L"GetSocketWorldLocationAndRotation");
    unified_weapon_fire::SocketGetterLayout before{},returned{};unsigned mask{},after_mask{},result{};
    if(!unified_weapon_fire::socket_getter_schema(call,r,mask,&before)||
       !before.write(call,unified_weapon_fire::SocketGetterLayout::input_name,candidate.source.name)||!call.run()||
       !unified_weapon_fire::socket_getter_schema(call,r,after_mask,&returned)||
       before.function!=returned.function||before.bytes!=returned.bytes||before.offsets!=returned.offsets||mask!=after_mask||
       !returned.read(call,unified_weapon_fire::SocketGetterLayout::result,result)||(result&mask)==0)return false;
    playground::Vector location{};playground::Rotation rotation{};
    if(!returned.read(call,unified_weapon_fire::SocketGetterLayout::output_location,location)||
       !returned.read(call,unified_weapon_fire::SocketGetterLayout::output_rotation,rotation))return false;
    constexpr double radians=3.14159265358979323846/32768.;
    const auto matrix=gears_vr::ue_matrix({rotation.pitch*radians,rotation.yaw*radians,rotation.roll*radians});
    candidate.world.position={location.x,location.y,location.z};
    if(!gears_vr::rig_tracking::detail::quaternion(matrix,candidate.world.orientation)||!gears_vr::stereo_pose_valid(candidate.world))return false;
    reads.repeat=true;
    if(!metadata(f,asset,r,reads,after)||reads.cursor!=reads.count||reads.field_cursor!=reads.field_count||reads.name_cursor!=reads.name_count||
       !hand::same(candidate.source.socket,after.socket)||!meta::same_property(candidate.source.inner,after.inner)||
       lancermain_asset::read(f,after_asset,lancermain_asset::AssetScope::boomshot_projectile)!=lancermain_asset::Status::ready||
       !hand::same(asset.component,after_asset.component)||!hand::same(asset.asset,after_asset.asset)||!nativeequipment::current(nullptr,f))return false;
    candidate.key=f.key;out=candidate;return true;
}
}
