// Normal-GT original collision query. Never called from a Call notification.
// A gun poking through a wall must not start a projectile on its far side.
namespace boomshot_muzzle_clear {
namespace f=unified_weapon_fire;namespace meta=gripdamage_metadata;
#include "weapon_boomshot_muzzle_schema.inl"
enum class QueryStage:unsigned {current_before=1,location,schema_before,context_before,invoke,schema_after,location_after,metadata_after,current_after,blocked,clear};
std::uint64_t query_observations{};
bool observe(f::MuzzleSample const& sample,QueryStage stage,SchemaObservation const& o,
    unsigned raw_result=0,unsigned result_mask=0) noexcept {
    const auto sequence=++query_observations;
    if(log_file&&sequence<=32){
        fwprintf(log_file,L"boomshot_muzzle_clear observation=%llu frame=%llu stage=%u schema_gate=%u seen=%u children=%u bytes=%u native=%u flags=%x return_offset=%u offsets=%u,%u,%u,%u,%u type_gate=%u field_index=%u field_address=%llx field_type=%u field_flags=%llx element_bytes=%u mask_read=%u field_mask=%x raw_result=%x result_mask=%x original_call_returned=%u clear=%u\n",
            sequence,sample.source.key.frame,static_cast<unsigned>(stage),static_cast<unsigned>(o.gate),
            o.seen,o.children,o.bytes,o.native,o.flags,o.return_offset,o.offsets[0],o.offsets[1],o.offsets[2],o.offsets[3],o.offsets[4],
            static_cast<unsigned>(o.type_gate),o.field_index,static_cast<unsigned long long>(o.field_address),o.field_type,
            static_cast<unsigned long long>(o.field_flags),o.element_bytes,o.mask_read,o.mask,
            raw_result,result_mask,stage>=QueryStage::schema_after?1u:0u,stage==QueryStage::clear?1u:0u);fflush(log_file);
    }
    return stage==QueryStage::clear;
}
bool query(f::MuzzleSample const& sample) noexcept {
    const auto& source=sample.source;SchemaObservation trace_observation{};
    if(!nativeequipment::current(nullptr,source))return observe(sample,QueryStage::current_before,trace_observation);
    meta::Snapshot trace_schema_snapshot{};trace_schema_snapshot.module_base=base;meta::Reader r{trace_schema_snapshot,meta::Stage::final_check};
    weaponhand::BoolField location{};playground::Vector start{},again{};
    if(!meta::tables(base,trace_schema_snapshot.objects,trace_schema_snapshot.names)||!weaponhand::object(r,source.pawn)||
       !weaponhand::field(r,source.pawn.address,L"Location",L"StructProperty",12,location)||
       !f::core_struct(r,location.property,L"Vector")||!r.at(source.pawn.address,location.property.offset,start)||
       !std::isfinite(start.x)||!std::isfinite(start.y)||!std::isfinite(start.z)||
       trace_schema_snapshot.field_count==trace_schema_snapshot.fields.size())return observe(sample,QueryStage::location,trace_observation);
    trace_schema_snapshot.fields[trace_schema_snapshot.field_count++]=location.property;
    playground::Args call(source.pawn.address,L"FastTrace");Layout before{},after{};
    if(!schema(call,source.pawn.address,r,before,&trace_observation))return observe(sample,QueryStage::schema_before,trace_observation);
    const playground::Vector extent{}; // Original line query, no invented sweep radius.
    std::memcpy(call.r.params+before.offsets[Layout::trace_end],&sample.origin,12);
    std::memcpy(call.r.params+before.offsets[Layout::trace_start],&start,12);
    std::memcpy(call.r.params+before.offsets[Layout::box_extent],&extent,12);
    std::memcpy(call.r.params+before.offsets[Layout::trace_bullet],&before.bullet_mask,4);
    if(!f::retained_metadata(r)||!nativeequipment::current(nullptr,source))return observe(sample,QueryStage::context_before,trace_observation);
    if(!call.run())return observe(sample,QueryStage::invoke,trace_observation);
    if(!schema(call,source.pawn.address,r,after,&trace_observation)||!(before==after))return observe(sample,QueryStage::schema_after,trace_observation);
    if(!r.at(source.pawn.address,location.property.offset,again)||std::memcmp(&start,&again,12))return observe(sample,QueryStage::location_after,trace_observation);
    if(!f::retained_metadata(r))return observe(sample,QueryStage::metadata_after,trace_observation);
    if(!nativeequipment::current(nullptr,source))return observe(sample,QueryStage::current_after,trace_observation);
    unsigned result{};std::memcpy(&result,call.r.params+before.offsets[Layout::result],4);
    return observe(sample,(result&before.result_mask)?QueryStage::clear:QueryStage::blocked,trace_observation,result,before.result_mask);
}
}
