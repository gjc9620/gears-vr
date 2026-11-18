namespace inv=lancerreload_inventory;
#include "weapon_boomshot_find_sequence.inl"
struct Binding {Cues cues{};PlayLayout play{};};
bool current(hand::Frame const& f) noexcept {
    return weaponhand::current_gt(nullptr)&&f.left_tracked==1&&f.left_grip_known==1&&
        hand::frame_active(f,weaponhand::clock_ns(nullptr))&&nativeequipment::current(nullptr,f);
}
bool bind(hand::Frame const& f,playground::Args& sound,meta::Reader& r,Binding& out) noexcept {
    out={};weaponhand::BoolField field{},sequence_field{};meta::RawName name{},closing_name{},sequence_name{};meta::NameText label{};
    if(!current(f)||!weaponhand::object(r,f.weapon)||!weaponhand::object(r,f.mesh)||
       !weaponhand::derives(r,f.weapon.address,L"Weap_Boomer_base",L"WarfareGame")||
       !weaponhand::field(r,f.weapon.address,L"WeaponReloadAnim",L"NameProperty",8,field)||!retain(r,field.property)||
       !r.at(f.weapon.address,field.property.offset,name)||name.number||!meta::fresh_name(r.result.names,name.index,label)||!meta::exact_text(label,L"AR_Reload"))return false;
    playground::Args find(f.mesh.address,L"FindAnimSequence");FindLayout first{},closing{};Binding value{};
    if(!find_schema(find,f.mesh.address,r,first)||!play_schema(sound,f.weapon.address,r,value.play)||!retained(r)||!current(f))return false;
    std::memcpy(find.r.params+first.input,&name,8);if(!find.run())return false;uptr sequence{};std::memcpy(&sequence,find.r.params+first.result,8);
    hand::ObjectIdentity identity{};Cues repeated{};
    if(!lancermain_asset::identify(r,sequence,identity)||
       !weaponhand::field(r,sequence,L"SequenceName",L"NameProperty",8,sequence_field)||!retain(r,sequence_field.property)||
       !r.at(sequence,sequence_field.property.offset,sequence_name)||sequence_name.index!=name.index||sequence_name.number||!cues(r,identity,value.cues)||!current(f))return false;
    std::memset(find.r.params+first.result,0,8);if(!find.run()||!find_schema(find,f.mesh.address,r,closing)||!(first==closing))return false;
    uptr again{};std::memcpy(&again,find.r.params+first.result,8);PlayLayout final_play{};
    if(again!=sequence||!cues(r,identity,repeated)||!(value.cues==repeated)||
       !r.at(f.weapon.address,field.property.offset,closing_name)||closing_name.index!=name.index||closing_name.number!=name.number||
       !r.at(sequence,sequence_field.property.offset,sequence_name)||sequence_name.index!=name.index||sequence_name.number||
       !play_schema(sound,f.weapon.address,r,final_play)||!(value.play==final_play)||!retained(r)||!current(f))return false;
    out=value;return true;
}
std::uint64_t consumed_ordinal{};unsigned rows{};
bool play(hand::Frame const& f,Cue cue,std::uint64_t ordinal,std::int64_t receipt_ns) noexcept {
    if(!weaponhand::current_gt(nullptr)||!ordinal||ordinal<=consumed_ordinal)return false;consumed_ordinal=ordinal;
    const auto fresh=[&]()noexcept{const auto observed_now=weaponhand::clock_ns(nullptr);return receipt_ns>0&&observed_now>=receipt_ns&&observed_now-receipt_ns<=100000000&&current(f);};
    if(unsigned(cue)>=3||!fresh())return false;
    inv::Counts before{},after{};if(!inv::read(f,before)||before.capacity!=1||before.used<0||before.used>1)return false;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader reader{schema,meta::Stage::function_fields};
    playground::Args sound(f.weapon.address,L"WeaponPlaySoundLocal");Binding binding{};
    if(!meta::tables(base,schema.objects,schema.names)||!bind(f,sound,reader,binding)||!fresh())return false;
    const auto object=binding.cues.cue[unsigned(cue)].address;const float no_noise=0;
    std::memcpy(sound.r.params+binding.play.sound,&object,8);std::memcpy(sound.r.params+binding.play.noise,&no_noise,4);
    // The receipt ordinal was consumed before reflection and original calls.
    // Unknown/false returns never retry this effect or alter action ownership.
    const bool returned=sound.run();PlayLayout closing{};Cues closing_cues{};
    const bool verified=returned&&inv::read(f,after)&&before==after&&play_schema(sound,f.weapon.address,reader,closing)&&binding.play==closing&&
        cues(reader,binding.cues.sequence,closing_cues)&&binding.cues==closing_cues&&retained(reader)&&fresh();
    if(log_file&&rows<24){++rows;fwprintf(log_file,L"boomshot_endpoint_sound kind=%u ordinal=%llu cue=%llx returned=%u unchanged_counts_source_verified=%u local=1 noise=0 no_retry=1\n",
        unsigned(cue),ordinal,object,unsigned(returned),unsigned(verified));fflush(log_file);}
    return verified;
}
