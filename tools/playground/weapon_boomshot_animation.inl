// Current original Boomshot AR_Reload samples only. This read prepares sliding-barrel
// geometry; all StopAnim/SetPosition operations remain with the shared owner.
namespace boomshot_animation {
namespace hand=gears_weapon_hand;namespace anim=hammerburst_animation;
namespace meta=gripdamage_metadata;namespace tracks=gears_weapon::boomshot_tracks;
struct Curve {
    hand::ObjectIdentity sequence{};
    std::uint64_t content{};
    tracks::Samples samples{};
};
// POD from the reads already required below. It grants no animation or action
// permission and never triggers a second read for a rejected preparation.
struct CurveReadObservation {
    unsigned gate{};int frames{},offsets{},bytes{};
    std::array<unsigned char,3> formats{};
};
CurveReadObservation last_read{};

#include "weapon_boomshot_animation_identity.inl"
bool track_order(meta::Reader& r,hand::ObjectIdentity const& sequence,std::array<meta::RawName,3>& out) noexcept {
    const auto owner=get<uptr>(sequence.address+0x40);weaponhand::BoolField names{};meta::Table table{};
    if(!weaponhand::field(r,owner,L"TrackBoneNames",L"ArrayProperty",16,names)||
       !r.at(owner,names.property.offset,table)||!table.data||table.count!=3||table.capacity<3||table.capacity>1024)return false;
    const auto property=r.result.identities[names.property.identity].address;uptr element{};meta::Property inner{};
    if(!r.at(property,0xd0,element)||!r.property(element,inner)||r.result.identities[inner.identity].outer!=property||
       inner.dimension!=1||inner.element_bytes!=8||!weaponhand::core_type(r,inner.type,L"NameProperty"))return false;
    const wchar_t* expected[]={L"Boomshot_Root",L"Boomshot_Magazine",L"Boomshot_Barrel"};
    std::array<meta::RawName,3> value{};
    if(!copy(value.data(),table.data,sizeof value))return false;
    for(unsigned i=0;i<3;++i){meta::NameText label{};const auto size=std::wcslen(expected[i]);
        if(value[i].number||!meta::fresh_name(r.result.names,value[i].index,label)||
           label.length!=size||size>=meta::max_name_units||std::wmemcmp(label.units.data(),expected[i],size+1))return false;}
    out=value;return true;
}
bool read_curve(hand::Frame const& f,anim::Observation const& expected,Curve& out) noexcept try {
    last_read={};last_read.gate=1;
    anim::Observation before{};
    if(!anim::observe_kind(f,before,lancermain_asset::HeldAsset::boomshot)||
       !hand::same(before.node,expected.node)||!hand::same(before.sequence,expected.sequence)||
       before.reload_start!=expected.reload_start||before.length!=expected.length)return false;
    last_read.gate=2;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    meta::NameText label{};std::array<meta::RawName,3> order{},after_order{};
    if(!meta::tables(base,schema.objects,schema.names)||!original_set(r,before.sequence)||
       !meta::fresh_name(schema.names,before.sequence_name.index,label)||before.sequence_name.number||
       !meta::exact_text(label,L"AR_Reload")||!track_order(r,before.sequence,order))return false;
    last_read.gate=3;
    last_read.gate=14;if(!weapon_animation_schema::legacy_uniform_schema(r,before.sequence))return false;
    std::array<weaponhand::BoolField,7> fields{};
    last_read.gate=31;if(!weaponhand::field(r,before.sequence.address,L"NumFrames",L"IntProperty",4,fields[0]))return false;
    last_read.gate=32;if(!weaponhand::field(r,before.sequence.address,L"RotationCompressionFormat",L"ByteProperty",1,fields[1]))return false;
    last_read.gate=33;if(!weaponhand::field(r,before.sequence.address,L"TranslationCompressionFormat",L"ByteProperty",1,fields[2]))return false;
    last_read.gate=34;if(!weaponhand::field(r,before.sequence.address,L"CompressedTrackOffsets",L"ArrayProperty",16,fields[4]))return false;
    last_read.gate=35;if(!weaponhand::field(r,before.sequence.address,L"CompressedByteStream",L"ArrayProperty",16,fields[5]))return false;
    last_read.gate=36;if(!weaponhand::field(r,before.sequence.address,L"SequenceLength",L"FloatProperty",4,fields[6]))return false;
    std::int32_t count{};std::array<unsigned char,3> formats{};float length{};meta::Table offsets{},bytes{};
    last_read.gate=4;
    if(!r.at(before.sequence.address,fields[0].property.offset,count)||count<3||count>256)return false;
    last_read.frames=count;last_read.gate=5;
    for(unsigned i=0;i<2;++i)if(!r.at(before.sequence.address,fields[i+1].property.offset,formats[i]))return false;
    last_read.formats=formats;last_read.gate=6;
    if(formats[0]!=1||formats[1]!=0||formats[2]!=0||
       !r.at(before.sequence.address,fields[4].property.offset,offsets)||
       !r.at(before.sequence.address,fields[5].property.offset,bytes)||
       !r.at(before.sequence.address,fields[6].property.offset,length)||length!=before.length)return false;
    last_read.offsets=offsets.count;last_read.bytes=bytes.count;last_read.gate=7;
    for(unsigned i=4;i<6;++i){
        const auto address=schema.identities[fields[i].property.identity].address;uptr element{};meta::Property p{};
        if(!r.at(address,0xd0,element)||!r.property(element,p)||schema.identities[p.identity].outer!=address||
           p.dimension!=1||p.element_bytes!=(i==4?4u:1u)||
           !(i==4?weaponhand::core_type(r,p.type,L"IntProperty"):weaponhand::core_type(r,p.type,L"ByteProperty")))return false;
    }
    last_read.gate=8;
    if(!offsets.data||offsets.count!=12||offsets.capacity<12||offsets.capacity>4096||
       !bytes.data||bytes.count<72||bytes.count>65536||bytes.capacity<bytes.count||bytes.capacity>65536)return false;
    std::array<std::int32_t,12> table{},fresh_table{};
    std::vector<unsigned char> content(std::size_t(bytes.count)),fresh(content.size());
    last_read.gate=9;
    if(!copy(table.data(),offsets.data,sizeof table)||!copy(content.data(),bytes.data,content.size()))return false;
    Curve candidate{};candidate.sequence=before.sequence;
    last_read.gate=10;
    if(!tracks::decode(table,content.data(),content.size(),unsigned(count),length,candidate.samples))return false;
    last_read.gate=15;
    tracks::Path path{};if(!path.bind(candidate.samples))return false;
    std::int32_t after_count{};std::array<unsigned char,3> after_formats{};float after_length{};
    meta::Table after_offsets{},after_bytes{};
    last_read.gate=11;
    if(!r.at(before.sequence.address,fields[0].property.offset,after_count)||after_count!=count)return false;
    for(unsigned i=0;i<2;++i)if(!r.at(before.sequence.address,fields[i+1].property.offset,after_formats[i]))return false;
    last_read.gate=12;
    if(after_formats!=formats||!r.at(before.sequence.address,fields[4].property.offset,after_offsets)||
       !r.at(before.sequence.address,fields[5].property.offset,after_bytes)||
       after_offsets.data!=offsets.data||after_offsets.count!=offsets.count||after_offsets.capacity!=offsets.capacity||
       after_bytes.data!=bytes.data||after_bytes.count!=bytes.count||after_bytes.capacity!=bytes.capacity||
       !r.at(before.sequence.address,fields[6].property.offset,after_length)||after_length!=length||
       !copy(fresh_table.data(),offsets.data,sizeof fresh_table)||fresh_table!=table||
       !copy(fresh.data(),bytes.data,fresh.size())||fresh!=content||!track_order(r,before.sequence,after_order)||
       std::memcmp(order.data(),after_order.data(),sizeof order)||!weapon_animation_schema::legacy_uniform_schema(r,before.sequence)||
       !unified_weapon_fire::retained_metadata(r))return false;
    anim::Observation after{};
    last_read.gate=13;
    if(!anim::observe_kind(f,after,lancermain_asset::HeldAsset::boomshot)||
       !hand::same(after.node,before.node)||!hand::same(after.sequence,before.sequence)||
       after.sequence_name.index!=before.sequence_name.index||after.sequence_name.number!=before.sequence_name.number||
       after.length!=before.length||after.reload_start!=before.reload_start||!nativeequipment::current(nullptr,f))return false;
    candidate.content=14695981039346656037ull;
    const auto hash=[&](void const* data,std::size_t size){const auto p=static_cast<unsigned char const*>(data);
        for(std::size_t i=0;i<size;++i){candidate.content^=p[i];candidate.content*=1099511628211ull;}};
    hash(table.data(),sizeof table);hash(content.data(),content.size());hash(&count,sizeof count);hash(&length,sizeof length);
    out=candidate;last_read.gate=0;return true;
}catch(...){return false;}
}
#include "weapon_boomshot_inspection_source.inl"
