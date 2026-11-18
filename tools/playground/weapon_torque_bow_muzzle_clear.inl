// Normal-GT original collision query. Never called from a Call notification.
// A gun poking through a wall must not start a projectile on its far side.
namespace torquebow_muzzle_clear {
namespace f=unified_weapon_fire;namespace meta=gripdamage_metadata;
unsigned query_gate{};bool query_called{},query_clear{};
playground::Vector query_start{},query_end{};
#include "weapon_torque_bow_muzzle_schema.inl"
SchemaObservation query_schema{};
bool query(f::MuzzleSample const& sample) noexcept {
    const auto& source=sample.source;
    query_gate=1;query_called=query_clear=false;query_start={};query_end=sample.origin;query_schema={};
    if(!nativeequipment::current(nullptr,source))return false;
    meta::Snapshot trace_schema_snapshot{};trace_schema_snapshot.module_base=base;meta::Reader r{trace_schema_snapshot,meta::Stage::final_check};
    r.reuse_initial_identities=true; // Initial discovery only; disabled before the first complete fresh closure.
    weaponhand::BoolField location{};playground::Vector start{},again{};
    query_gate=2;if(!meta::tables(base,trace_schema_snapshot.objects,trace_schema_snapshot.names)||!weaponhand::object(r,source.pawn)||
       !weaponhand::field(r,source.pawn.address,L"Location",L"StructProperty",12,location)||
       !f::core_struct(r,location.property,L"Vector")||!r.at(source.pawn.address,location.property.offset,start)||
       !std::isfinite(start.x)||!std::isfinite(start.y)||!std::isfinite(start.z))return false;
    query_start=start;query_gate=3;
    if(trace_schema_snapshot.field_count==trace_schema_snapshot.fields.size())return false;trace_schema_snapshot.fields[trace_schema_snapshot.field_count++]=location.property;
    playground::Args call(source.pawn.address,L"FastTrace");Layout before{},after{};
    query_gate=4;if(!schema(call,source.pawn.address,r,before,&query_schema))return false;
    std::memcpy(call.r.params+before.offsets[Layout::trace_end],&sample.origin,12);
    std::memcpy(call.r.params+before.offsets[Layout::trace_start],&start,12);
    const playground::Vector extent{}; // Zero extent preserves an original line query.
    std::memcpy(call.r.params+before.offsets[Layout::box_extent],&extent,12);
    std::memcpy(call.r.params+before.offsets[Layout::trace_bullet],&before.bullet_mask,4);
    r.reuse_initial_identities=false;
    query_gate=5;if(!f::retained_metadata(r)||!nativeequipment::current(nullptr,source))return false;
    query_gate=6;if(!call.run())return false;query_called=true;
    query_gate=7;if(!schema(call,source.pawn.address,r,after,&query_schema)||!(before==after))return false;
    query_gate=8;if(!r.at(source.pawn.address,location.property.offset,again)||std::memcmp(&start,&again,12))return false;
    query_gate=9;if(!f::retained_metadata(r))return false;
    query_gate=10;if(!nativeequipment::current(nullptr,source))return false;
    unsigned result{};std::memcpy(&result,call.r.params+before.offsets[Layout::result],4);
    query_clear=(result&before.result_mask)!=0;query_gate=query_clear?0u:11u;return query_clear;
}
}
