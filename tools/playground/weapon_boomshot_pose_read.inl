#include "weapon_boomshot_pose_metadata.inl"
namespace boomshot_pose {
namespace inv=lancerreload_inventory;
struct Certification {inv::Identity identity{};Source source{};bool known{};};
Certification certification{};
// This path is legal inside HandClient observation: all reads are reflected
// metadata/data, with no client reentry, named_parts engine call or input writer.
bool read(hand::Frame const& f,Source& out) noexcept {
    out={};lancermain_asset::Evidence asset{},closing_asset{};
    const auto started=weaponhand::clock_ns(nullptr);
    if(!weaponhand::current_gt(nullptr)||
       lancermain_asset::read(f,asset,lancermain_asset::AssetScope::boomshot_projectile)!=lancermain_asset::Status::ready)return false;
    meta::Snapshot snapshot{};snapshot.module_base=base;meta::Reader r{snapshot,meta::Stage::final_check};Reads reads{};
    Source first{},last{};
    if(!meta::tables(base,snapshot.objects,snapshot.names)||!weaponhand::object(r,f.world)||!weaponhand::object(r,f.pawn)||
       !metadata(f,asset,r,reads,first))return false;
    reads.repeat=true;
    if(!metadata(f,asset,r,reads,last)||reads.cursor!=reads.count||reads.field_cursor!=reads.field_count||
       reads.name_cursor!=reads.name_count||!same_reference(first,last)||first.world!=last.world||
       lancermain_asset::read(f,closing_asset,lancermain_asset::AssetScope::boomshot_projectile)!=lancermain_asset::Status::ready||
       !hand::same(asset.component,closing_asset.component)||!hand::same(asset.asset,closing_asset.asset))return false;
    first.observed_begin_ns=started;first.observed_end_ns=weaponhand::clock_ns(nullptr);
    if(started<=0||first.observed_end_ns<started)return false;
    out=first;return true;
}
bool bound(hand::Frame const& f,Source const& source) noexcept {
    return nativelancerfollow::bound_key_known&&nativelancerfollow::mount.bound()&&
        nativelancerfollow::bound_key.hold_generation==vrequip::model.hold_generation()&&
        hand::same(f.weapon,nativelancerfollow::bound_key.weapon)&&
        hand::same(source.component,nativelancerfollow::bound_key.component)&&hand::same(source.asset,nativelancerfollow::bound_key.asset);
}
bool ready(hand::Frame const& f,Source& out) noexcept {
    out={};
    if(!weaponhand::current_gt(nullptr)||!certification.known||!(certification.identity==inv::identity(f))||
       !bound(f,certification.source))return false;
    Source current{};if(!read(f,current))return false;
    if(!same_reference(current,certification.source)){certification.known=false;return false;}
    // World/component/bone matrices ALWAYS come from this query, never from
    // the old getter comparison saved in the static reference certificate.
    out=current;return true;
}
bool original_agrees(Source const& a,Source const& b,hammerburst_animation::Parts const& named_parts) noexcept {
    if(!same_reference(a,b)||!hand::same(a.component,named_parts.component)||!hand::same(a.asset,named_parts.asset)||a.world!=b.world)return false;
    // Existing independent world readback uses .002 game units for position.
    // Basis agreement is separately relative to scale, not that world-unit
    // tolerance. Neither limit grants a contact or mechanical completion.
    for(unsigned bone=0;bone<3;++bone){
        auto const& matrix=named_parts.world[bone];
        const bool collapsed=bone==1&&a.magazine_collapsed&&collapsed_magazine(matrix);
        if(!collapsed&&!affine(matrix))return false;
        const auto scale=collapsed?0.:std::hypot(double(matrix[0]),double(matrix[1]),double(matrix[2]));
        for(unsigned value=0;value<16;++value){
            const double tolerance=value>=12&&value<=14?.002:1e-5*scale;
            if(std::abs(double(a.world[bone][value])-matrix[value])>tolerance)return false;
        }
    }
    return true;
}
bool certify(hand::Frame const& f) noexcept {
    // Only the normal GT owner may call named_parts named getters. A cached
    // certificate grants nothing until ready() freshly rechecks all data.
    if(!weaponhand::current_gt(nullptr)||!nativeequipment::current(nullptr,f))return false;
    const auto identity=inv::identity(f);if(!identity.valid())return false;
    if(certification.known&&certification.identity==identity&&bound(f,certification.source))return true;
    certification={};Source before{},after{};hammerburst_animation::Parts named_parts{};
    if(!read(f,before)||!bound(f,before)||!boomshot_parts::read(f,named_parts)||!read(f,after)||
       !original_agrees(before,after,named_parts)||!bound(f,after)||!(identity==inv::identity(f))||!nativeequipment::current(nullptr,f))return false;
    certification={identity,after,true};return true;
}
}
