// Ordinary GT only. Read the original selected animation node before any
// future lease can stop/seek it; this observation grants no write permission.
namespace hammerburst_animation {
namespace hand=gears_weapon_hand;
namespace meta=gripdamage_metadata;
struct Parts {
    hand::ObjectIdentity component{},asset{};
    std::array<std::array<float,16>,3> world{};
    hand::FrameKey key{};
};
struct Observation {
    hand::ObjectIdentity weapon{},component{},asset{},node{},sequence{};
    Name sequence_name{};
    float time{},rate{},length{},reload_start{};
    bool playing{},looping{};
};
bool selected_node(hand::Frame const& source,hand::ObjectIdentity& out) noexcept {
    static unsigned last_failure{~0u};
    const auto failed=[&](unsigned gate,playground::Args const* call=nullptr,uptr node=0) noexcept {
        if(gate!=last_failure&&log_file){
            fwprintf(log_file,L"hammerburst_animation_node gate=%u bytes=%u function=%llx node=%llx\n",
                gate,call?call->r.size:0,call?call->r.function:0,node);fflush(log_file);last_failure=gate;
        }return false;
    };
    if(!nativeequipment::current(nullptr,source))return failed(1);
    playground::Args getter(source.weapon.address,L"GetWeaponAnimNodeSeq");
    if(getter.r.size!=8||!vrequip::result_property(getter,L"ObjectProperty",8))return failed(2,&getter);
    if(!getter.run())return failed(3,&getter);
    if(!nativeequipment::current(nullptr,source))return failed(4,&getter);
    const auto node=getter.output<uptr>();
    if(!node)return failed(5,&getter);
    if(!nativeequipment::identify(nullptr,node,out)||!playground::instance(node,L"AnimNodeSequence"))return failed(6,&getter,node);
    last_failure=0;return true;
}
bool observe_kind(hand::Frame const& source,Observation& out,
    lancermain_asset::HeldAsset kind) noexcept {
    out={};lancermain_asset::Evidence asset{};
    if((kind!=lancermain_asset::HeldAsset::lancer&&kind!=lancermain_asset::HeldAsset::hammerburst&&
        kind!=lancermain_asset::HeldAsset::pistol)||!nativeequipment::current(nullptr,source)||
       lancermain_asset::read(source,asset,lancermain_asset::AssetScope::basic_hold)!=lancermain_asset::Status::ready||
       !nativeequipment::read_attached_frame(source,asset.component.address))return false;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(r,source.weapon)||
       lancermain_asset::held_asset_name(r,source.weapon.address,lancermain_asset::AssetScope::basic_hold)!=
           kind)return false;
    Observation candidate{};candidate.weapon=source.weapon;candidate.component=asset.component;candidate.asset=asset.asset;
    if(!selected_node(source,candidate.node)||!weaponhand::object(r,candidate.node))return false;
    weaponhand::BoolField named{},time{},rate{},playing{},looping{},duration{};
    meta::PointerField sequence{};meta::RawName raw{};
    const auto node=candidate.node.address;
    if(!weaponhand::field(r,node,L"AnimSeqName",L"NameProperty",8,named)||
       !r.at(node,named.property.offset,raw)||raw.number||
       !weaponhand::read_pointer(r,node,L"AnimSeq",sequence)||
       !lancermain_asset::identify(r,sequence.value,candidate.sequence)||
       !weaponhand::derives(r,sequence.value,L"AnimSequence",L"Engine")||
       !weaponhand::field(r,node,L"CurrentTime",L"FloatProperty",4,time)||
       !r.at(node,time.property.offset,candidate.time)||
       !weaponhand::field(r,node,L"Rate",L"FloatProperty",4,rate)||!r.at(node,rate.property.offset,candidate.rate)||
       !weaponhand::boolean(r,node,L"bPlaying",playing)||!weaponhand::boolean(r,node,L"bLooping",looping)||
       !weaponhand::field(r,sequence.value,L"SequenceLength",L"FloatProperty",4,duration)||
       !r.at(sequence.value,duration.property.offset,candidate.length)||
       !lancerreload_state::start_time(source.weapon.address,candidate.reload_start))return false;
    meta::NameText text{};
    if(!meta::fresh_name(schema.names,raw.index,text)||
       !(kind==lancermain_asset::HeldAsset::hammerburst?meta::exact_text(text,L"HB_Reload"):meta::exact_text(text,L"AR_Reload"))||
       !std::isfinite(candidate.time)||!std::isfinite(candidate.rate)||!std::isfinite(candidate.length)||
       candidate.length<=0||candidate.length>10||candidate.time<0||candidate.time>candidate.length)return false;
    candidate.sequence_name={raw.index,raw.number};
    candidate.playing=(playing.bits&playing.mask)!=0;candidate.looping=(looping.bits&looping.mask)!=0;
    hand::ObjectIdentity current{};meta::RawName again{};float position{},speed{},length{},started{};
    meta::PointerField fresh_sequence{};weaponhand::BoolField fresh_playing{},fresh_looping{};
    if(!selected_node(source,current)||!hand::same(candidate.node,current)||
       !r.at(node,named.property.offset,again)||again.index!=raw.index||again.number!=raw.number||
       !weaponhand::read_pointer(r,node,L"AnimSeq",fresh_sequence)||!meta::same_pointer(sequence,fresh_sequence)||
       !r.at(node,time.property.offset,position)||position!=candidate.time||
       !r.at(node,rate.property.offset,speed)||speed!=candidate.rate||
       !r.at(sequence.value,duration.property.offset,length)||length!=candidate.length||
       !weaponhand::boolean(r,node,L"bPlaying",fresh_playing)||!weaponhand::boolean(r,node,L"bLooping",fresh_looping)||
       fresh_playing.bits!=playing.bits||fresh_playing.mask!=playing.mask||
       fresh_looping.bits!=looping.bits||fresh_looping.mask!=looping.mask||
       !lancerreload_state::start_time(source.weapon.address,started)||started!=candidate.reload_start||
       !unified_weapon_fire::retained_metadata(r)||!nativeequipment::current(nullptr,source))return false;
    out=candidate;return true;
}
bool observe(hand::Frame const& source,Observation& out,bool lancer=false) noexcept {
    return observe_kind(source,out,lancer?lancermain_asset::HeldAsset::lancer:lancermain_asset::HeldAsset::hammerburst);
}
bool observe_expected(hand::Frame const& source,Observation& out) noexcept {
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(r,source.weapon))return false;
    const auto kind=lancermain_asset::held_asset_name(r,source.weapon.address,lancermain_asset::AssetScope::basic_hold);
    if(!unified_weapon_fire::retained_metadata(r)||!nativeequipment::current(nullptr,source))return false;
    return observe_kind(source,out,kind);
}
struct Controls {
    hand::ObjectIdentity node{},stop{},seek{};
    unsigned time_offset{},notify_offset{},notify_mask{};
};
struct LatchCurve {
    hand::ObjectIdentity sequence{};
    gears_weapon::HammerburstLatchCurve curve;
    std::uint64_t content{};
    gears_weapon::HammerburstLatchCurve::Sampling sampling{gears_weapon::HammerburstLatchCurve::Sampling::reference_keys};
    // Stored animation-local axis, not a world-space controller hinge.
    std::array<double,3> local_axis{0,1,0};
};
// Read this instance's legacy three-track Float96 stream. No packaged keys,
// hardcoded key count, offsets or animation duration are substituted. This
// proves a bounded uniform-layout curve; live bone/time agreement is separate.
bool read_latch_curve(hand::Frame const& source,LatchCurve& out,Observation const* owned=nullptr) noexcept {
    out={};Observation before{};
    if(owned){
        before=*owned;
        if(!hand::same(before.weapon,source.weapon)||!nativeequipment::current(nullptr,source))return false;
    }else if(!observe(source,before))return false;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(r,before.sequence))return false;
    weaponhand::BoolField frames{},format{},offsets{},stream{},duration{};
    float live_length{};
    std::int32_t count{};unsigned char compression{};meta::Table offset_array{},byte_array{};
    if(!weaponhand::field(r,before.sequence.address,L"NumFrames",L"IntProperty",4,frames)||
       !weaponhand::field(r,before.sequence.address,L"RotationCompressionFormat",L"ByteProperty",1,format)||
       !weaponhand::field(r,before.sequence.address,L"CompressedTrackOffsets",L"ArrayProperty",16,offsets)||
       !weaponhand::field(r,before.sequence.address,L"CompressedByteStream",L"ArrayProperty",16,stream)||
       !weaponhand::field(r,before.sequence.address,L"SequenceLength",L"FloatProperty",4,duration)||
       !r.at(before.sequence.address,duration.property.offset,live_length)||live_length!=before.length||
       !r.at(before.sequence.address,frames.property.offset,count)||count<3||count>256||
       !r.at(before.sequence.address,format.property.offset,compression)||compression!=1||
       !r.at(before.sequence.address,offsets.property.offset,offset_array)||
       !r.at(before.sequence.address,stream.property.offset,byte_array))return false;
    const auto inner=[&](weaponhand::BoolField const& field,bool bytes) noexcept {
        const auto address=schema.identities[field.property.identity].address;
        uptr element{};meta::Property p{};
        return r.at(address,0xd0,element)&&r.property(element,p)&&
            schema.identities[p.identity].outer==address&&p.dimension==1&&
            p.element_bytes==(bytes?1u:4u)&&
            (bytes?weaponhand::core_type(r,p.type,L"ByteProperty"):weaponhand::core_type(r,p.type,L"IntProperty"));
    };
    if(!inner(offsets,false)||!inner(stream,true)||!offset_array.data||offset_array.count!=12||
       offset_array.capacity<12||offset_array.capacity>4096||!byte_array.data||byte_array.count<72||
       byte_array.count>4096||byte_array.capacity<byte_array.count||byte_array.capacity>65536)return false;
    std::array<std::int32_t,12> tracks{};
    if(!copy(tracks.data(),offset_array.data,sizeof(tracks)))return false;
    // Root and magazine are constant; the only moving track is latch index1.
    if(tracks[1]!=1||tracks[3]!=1||tracks[5]!=1||tracks[7]!=count||tracks[9]!=1||tracks[11]!=1)return false;
    std::int32_t boundary=byte_array.count;
    for(unsigned i=0;i<12;i+=4){
        if(tracks[i]<0||tracks[i+2]<0||tracks[i]>byte_array.count-12||tracks[i+2]>byte_array.count-12)return false;
        for(auto offset:{tracks[i],tracks[i+2]})if(offset>tracks[6])boundary=std::min(boundary,offset);
    }
    if(tracks[6]<0||tracks[6]+24+12*count!=boundary)return false;
    std::array<float,6> range{};std::array<double,256> angles{};
    if(!copy(range.data(),byte_array.data+tracks[6],sizeof(range)))return false;
    for(unsigned i=0;i<6;++i)if(!std::isfinite(range[i])||(i>=3&&range[i]<0))return false;
    std::array<std::array<float,3>,256> keys{};
    if(!copy(keys.data(),byte_array.data+tracks[6]+24,std::size_t(count)*12))return false;
    for(int i=0;i<count;++i){
        const auto& q=keys[unsigned(i)];double squared=0;
        for(unsigned axis=0;axis<3;++axis){
            if(!std::isfinite(q[axis])||q[axis]<range[axis]-1e-5f||q[axis]>range[axis]+range[axis+3]+1e-5f)return false;
            squared+=double(q[axis])*q[axis];
        }
        // Exact recovered profile: identity closed pose, positive localY
        // hinge. A different track is rejected, never projected onto this axis.
        if(squared>1.00001||std::abs(q[0])>1e-5f||std::abs(q[2])>1e-5f||q[1]<0)return false;
        angles[unsigned(i)]=2*std::atan2(double(q[1]),std::sqrt(std::max(0.,1-squared)));
    }
    LatchCurve candidate{};candidate.sequence=before.sequence;
    if(!candidate.curve.bind(angles.data(),std::size_t(count),before.length,true))return false;
    // Change detector only; never a substitute for object/schema admission.
    candidate.content=14695981039346656037ull;
    const auto hash=[&](const void* data,std::size_t size) noexcept {
        const auto* bytes=static_cast<const unsigned char*>(data);
        for(std::size_t i=0;i<size;++i){candidate.content^=bytes[i];candidate.content*=1099511628211ull;}
    };
    hash(&count,sizeof(count));hash(&before.length,sizeof(before.length));
    hash(tracks.data(),sizeof(tracks));hash(range.data(),sizeof(range));hash(keys.data(),std::size_t(count)*12);
    // Repeat mutable values and source selectors before publishing the curve.
    Observation after{};meta::Table offsets_after{},bytes_after{};
    std::array<std::int32_t,12> tracks_after{};std::array<std::array<float,3>,256> keys_after{};
    std::array<float,6> range_after{};std::int32_t count_after{};unsigned char format_after{};
    if(!r.at(before.sequence.address,offsets.property.offset,offsets_after)||
       !r.at(before.sequence.address,stream.property.offset,bytes_after)||
       offsets_after.data!=offset_array.data||offsets_after.count!=offset_array.count||offsets_after.capacity!=offset_array.capacity||
       bytes_after.data!=byte_array.data||bytes_after.count!=byte_array.count||bytes_after.capacity!=byte_array.capacity||
       !copy(tracks_after.data(),offsets_after.data,sizeof(tracks_after))||tracks_after!=tracks||
       !copy(range_after.data(),bytes_after.data+tracks[6],sizeof(range_after))||range_after!=range||
       !copy(keys_after.data(),bytes_after.data+tracks[6]+24,std::size_t(count)*12)||keys_after!=keys||
       !r.at(before.sequence.address,frames.property.offset,count_after)||count_after!=count||
       !r.at(before.sequence.address,format.property.offset,format_after)||format_after!=compression||
       !r.at(before.sequence.address,duration.property.offset,live_length)||live_length!=before.length||
       !unified_weapon_fire::retained_metadata(r)||!nativeequipment::current(nullptr,source))return false;
    if(!owned&&(!observe(source,after)||!hand::same(before.node,after.node)||!hand::same(before.sequence,after.sequence)||
       before.length!=after.length||before.reload_start!=after.reload_start))return false;
    // The owned variant only reads/rechecks asset bytes. seek() independently
    // authenticates the live node/sequence/serial/hand before any dispatch.
    out=candidate;return true;
}
bool original_node_function(playground::Args const& call,const wchar_t* label,
    hand::ObjectIdentity& identity) noexcept {
    if(!call.valid||!registered(call.r.function)||!playground::named(call.r.function,label))return false;
    const auto owner=get<uptr>(call.r.function+0x40);
    const auto package=owner?get<uptr>(owner+0x40):0;
    return registered(owner)&&registered(package)&&playground::named(owner,L"AnimNodeSequence")&&
        playground::named(package,L"Engine")&&get<uptr>(package+0x40)==0&&
        (get<unsigned>(call.r.function+0xf4)&0x400)!=0&&
        weaponhand::executable(reinterpret_cast<FARPROC>(get<uptr>(call.r.function+0x114)),GetModuleHandleW(nullptr))&&
        nativeequipment::identify(nullptr,call.r.function,identity);
}
bool input_field(playground::Args const& call,uptr field,const wchar_t* kind,unsigned bytes) noexcept {
    if(!registered(field)||get<uptr>(field+0x40)!=call.r.function||
       !playground::named(get<uptr>(field+0x50),kind)||get<unsigned>(field+0x70)!=1||
       get<unsigned>(field+0x74)!=bytes||(get<unsigned long long>(field+0x78)&0x580)!=0x80)return false;
    const auto offset=get<unsigned>(field+0x94);
    return offset<=call.r.size&&bytes<=call.r.size-offset;
}
bool component_getter(playground::Args const& call,const wchar_t* label) noexcept {
    if(!call.valid||!registered(call.r.function)||!playground::named(call.r.function,label))return false;
    const auto owner=get<uptr>(call.r.function+0x40),package=owner?get<uptr>(owner+0x40):0;
    return registered(owner)&&registered(package)&&playground::named(owner,L"SkeletalMeshComponent")&&
        playground::named(package,L"Engine")&&get<uptr>(package+0x40)==0&&
        (get<unsigned>(call.r.function+0xf4)&0x400)!=0&&
        weaponhand::executable(reinterpret_cast<FARPROC>(get<uptr>(call.r.function+0x114)),GetModuleHandleW(nullptr));
}
// Actual same-GT parent-composed bone matrices, not reference-pose pivots.
unsigned parts_failure{};
gears_equipment_names::Lookup part_names({L"b_HB_Root",L"b_HB_MagLatch",L"b_HB_Mag"});
gears_equipment_names::Lookup lancer_part_names({L"b_AR_Root",L"b_AR_Receiver",L"b_AR_Latch"});
bool read_part_group(hand::Frame const& source,Parts& out,lancermain_asset::HeldAsset kind,
    gears_equipment_names::Lookup& lookup,std::array<const wchar_t*,3> const& labels,
    std::array<int,3> const& bone_indices) noexcept {
    parts_failure=1;
    out={};lancermain_asset::Evidence asset{};
    if(!nativeequipment::current(nullptr,source)||
       lancermain_asset::read(source,asset,lancermain_asset::AssetScope::basic_hold)!=lancermain_asset::Status::ready)return false;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    if(!meta::tables(base,schema.objects,schema.names)||
       lancermain_asset::held_asset_name(r,source.weapon.address,lancermain_asset::AssetScope::basic_hold)!=
       kind)return false;
    parts_failure=2;
    std::array<unsigned,3> names{};std::int64_t name_started{};
    if(lookup.step(
        [](gears_equipment_names::Table& out_table){const auto table=get<Array>(base+0x1c34800);
            out_table={table.data,unsigned(table.count),unsigned(table.capacity)};return table.count>=0&&table.capacity>=0;},
        [](gears_equipment_names::Table table,unsigned index,std::array<wchar_t,32>& text){
            const auto entry=get<uptr>(table.data+std::size_t(index)*8);unsigned observed_index{};
            return entry&&copy(&observed_index,entry,4)&&observed_index==index&&copy(text.data(),entry+20,sizeof(text));},
        [&](){return nativeequipment::current(nullptr,source);},
        [&](){const auto now=weaponhand::clock_ns(nullptr);if(!name_started)name_started=now;
            return now>=name_started&&now-name_started<3000000;},
        names,8192)!=gears_equipment_names::State::ready)return false;
    Parts value{};value.component=asset.component;value.asset=asset.asset;value.key=source.key;
    for(int index=0;index<3;++index){
        parts_failure=10+unsigned(index)*10;
        playground::Args name(asset.component.address,L"MatchRefBone"),matrix(asset.component.address,L"GetBoneMatrix");
        if(!component_getter(name,L"MatchRefBone")||!component_getter(matrix,L"GetBoneMatrix")||
           !input_field(name,name.prop(L"BoneName"),L"NameProperty",8)||
           !input_field(matrix,matrix.prop(L"BoneIndex"),L"IntProperty",4)||
           !vrequip::result_property(name,L"IntProperty",4)||
           !vrequip::result_property(matrix,L"StructProperty",64))return false;
        const auto result=matrix.prop(L"ReturnValue"),structure=get<uptr>(result+0xd0);
        if(!registered(structure)||!playground::named(structure,L"Matrix")||get<unsigned>(structure+0x88)!=64)return false;
        const auto nonoverlap=[](playground::Args& call,const wchar_t* input,unsigned input_bytes,unsigned bytes) noexcept {
            const auto a=get<unsigned>(call.prop(input)+0x94),b=get<unsigned>(call.prop(L"ReturnValue")+0x94);
            return a+input_bytes<=b||b+bytes<=a;
        };
        parts_failure=11+unsigned(index)*10;
        if(!nonoverlap(name,L"BoneName",8,4)||!nonoverlap(matrix,L"BoneIndex",4,64)||!nativeequipment::current(nullptr,source))return false;
        const Name named{names[unsigned(index)],0};meta::NameText text{};
        if(!meta::fresh_name(schema.names,named.index,text))return false;
        const auto label=labels[unsigned(index)];const auto label_size=std::wcslen(label);
        const bool expected=text.length==label_size&&label_size<meta::max_name_units&&
            !std::wmemcmp(text.units.data(),label,label_size+1);
        const int bone_index=bone_indices[unsigned(index)];
        parts_failure=12+unsigned(index)*10;
        if(!expected||!name.set(L"BoneName",named).run()||name.output<int>()!=bone_index||
           !matrix.set(L"BoneIndex",bone_index).run())return false;
        value.world[unsigned(index)]=matrix.output<std::array<float,16>>();
        for(float scalar:value.world[unsigned(index)])if(!std::isfinite(scalar))return false;
    }
    parts_failure=40;lancermain_asset::Evidence after{};
    if(lancermain_asset::read(source,after,lancermain_asset::AssetScope::basic_hold)!=lancermain_asset::Status::ready||
       !hand::same(asset.component,after.component)||!hand::same(asset.asset,after.asset)||
       !unified_weapon_fire::retained_metadata(r)||!nativeequipment::current(nullptr,source))return false;
    out=value;parts_failure=0;return true;
}
bool read_parts(hand::Frame const& source,Parts& out,bool lancer=false) noexcept {
    if(lancer)return read_part_group(source,out,lancermain_asset::HeldAsset::lancer,lancer_part_names,
        {L"b_AR_Root",L"b_AR_Receiver",L"b_AR_Latch"},{0,5,7});
    return read_part_group(source,out,lancermain_asset::HeldAsset::hammerburst,part_names,
        {L"b_HB_Root",L"b_HB_MagLatch",L"b_HB_Mag"},{0,1,2});
}
// Preflight only. No StopAnim/SetPosition invocation and no write lease is
// created here. Native reload/visual ownership must precede future dispatch.
bool prepare_controls(hand::Frame const& source,Observation const& expected,Controls& out,bool enclosed_observation=false) noexcept {
    out={};Observation before=expected;
    if((!enclosed_observation&&!observe_expected(source,before))||!nativeequipment::current(nullptr,source)||
       !hand::same(before.weapon,expected.weapon)||
       !hand::same(before.component,expected.component)||!hand::same(before.asset,expected.asset)||
       !hand::same(before.node,expected.node)||!hand::same(before.sequence,expected.sequence)||
       before.reload_start!=expected.reload_start)return false;
    playground::Args stop(before.node.address,L"StopAnim"),seek(before.node.address,L"SetPosition");
    Controls candidate{};candidate.node=before.node;
    if(stop.r.size!=0||seek.r.size!=8||!original_node_function(stop,L"StopAnim",candidate.stop)||
       !original_node_function(seek,L"SetPosition",candidate.seek))return false;
    const auto time=seek.prop(L"NewTime"),notifies=seek.prop(L"bFireNotifies");
    if(!input_field(seek,time,L"FloatProperty",4)||!input_field(seek,notifies,L"BoolProperty",4))return false;
    candidate.time_offset=get<unsigned>(time+0x94);candidate.notify_offset=get<unsigned>(notifies+0x94);
    candidate.notify_mask=get<unsigned>(notifies+0xd0);
    if(!candidate.notify_mask||(candidate.notify_mask&(candidate.notify_mask-1))||
       (candidate.time_offset<candidate.notify_offset+4&&candidate.notify_offset<candidate.time_offset+4))return false;
    // Even a prepared zeroed parameter block must explicitly carry false:
    // seeking a hand-driven pose must not replay ammo/throw notifies.
    if(!nativelancerfollow::bool_arg(seek,L"bFireNotifies",false)||
       !seek.set(L"NewTime",before.time).valid||!nativeequipment::current(nullptr,source))return false;
    hand::ObjectIdentity stop_after{},seek_after{};
    if(!original_node_function(stop,L"StopAnim",stop_after)||!hand::same(candidate.stop,stop_after)||
       !original_node_function(seek,L"SetPosition",seek_after)||!hand::same(candidate.seek,seek_after)||
       !nativeequipment::current(nullptr,source))return false;
    // No engine call occurs between the selected-node observation and the
    // dispatcher's final preflight. Repeating that getter here only spends
    // the same input lifetime; native function/argument identity is rechecked.
    out=candidate;return true;
}
struct Ownership {
    Observation initial{},last{};Controls controls{};
    LatchCurve latch{};
    lancerreload_inventory::Identity inventory{};
    std::uint64_t serial{};float wanted_time{};
    bool active{},stop_dispatched{},stopped{},seek_pending{},visual_only{};
};
Ownership ownership;
// A sampling convention is immutable geometry evidence, not a saved hand or
// native action grant. Reuse only for identical live sequence bytes in the
// same actual game/Native lifetime; object and write admission stay fresh.
struct SamplingReceipt {
    hand::ObjectIdentity sequence{};std::uint64_t content{},game_start{},native_start{};
    gears_weapon::HammerburstLatchCurve::Sampling sampling{};bool known{};
};
std::array<SamplingReceipt,3> sampling_receipts{};
unsigned next_sampling_receipt{};
void remember_sampling(hand::Frame const& source) noexcept {
    if(!ownership.active||!nativeequipment::current(nullptr,source)||
       !(ownership.inventory==lancerreload_inventory::identity(source)))return;
    const auto& latch=ownership.latch;
    for(const auto& receipt:sampling_receipts)if(receipt.known&&hand::same(receipt.sequence,latch.sequence)&&
        receipt.content==latch.content&&receipt.game_start==source.key.game_start&&
        receipt.native_start==source.key.native_started_qpc&&receipt.sampling==latch.sampling)return;
    sampling_receipts[next_sampling_receipt++%sampling_receipts.size()]={latch.sequence,latch.content,
        source.key.game_start,source.key.native_started_qpc,latch.sampling,true};
}
// Receipt-only observation of an existing synchronous action. No controller
// pose/freshness is borrowed and this cannot authorize a new hand operation.
bool read_owned(Observation& out) noexcept {
    if(!ownership.active||!weaponhand::current_gt(nullptr))return false;
    const auto& e=ownership.last;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    if(!meta::tables(base,schema.objects,schema.names))return false;
    for(const auto& object:{ownership.inventory.world,ownership.inventory.pawn,e.weapon,e.component,e.asset,e.node,e.sequence})
        if(!weaponhand::object(r,object))return false;
    meta::PointerField pawn{},world{},mesh{},asset{},sequence{};
    if(!weaponhand::pointer(r,e.weapon.address,L"Instigator",ownership.inventory.pawn.address,pawn)||
       !weaponhand::pointer(r,ownership.inventory.pawn.address,L"WorldInfo",ownership.inventory.world.address,world)||
       !lancermain_asset::mesh_pointer(r,e.weapon.address,mesh)||mesh.value!=e.component.address||
       !weaponhand::pointer(r,e.component.address,L"SkeletalMesh",e.asset.address,asset)||
       !weaponhand::pointer(r,e.node.address,L"AnimSeq",e.sequence.address,sequence))return false;
    Observation value=e;weaponhand::BoolField name{},time{},rate{},playing{},looping{};meta::RawName raw{};
    if(!weaponhand::field(r,e.node.address,L"AnimSeqName",L"NameProperty",8,name)||
       !r.at(e.node.address,name.property.offset,raw)||raw.index!=e.sequence_name.index||raw.number!=e.sequence_name.number||
       !weaponhand::field(r,e.node.address,L"CurrentTime",L"FloatProperty",4,time)||!r.at(e.node.address,time.property.offset,value.time)||
       !weaponhand::field(r,e.node.address,L"Rate",L"FloatProperty",4,rate)||!r.at(e.node.address,rate.property.offset,value.rate)||
       !weaponhand::boolean(r,e.node.address,L"bPlaying",playing)||!weaponhand::boolean(r,e.node.address,L"bLooping",looping)||
       !lancerreload_state::start_time(e.weapon.address,value.reload_start)||value.reload_start!=e.reload_start||
       !std::isfinite(value.time)||value.time<0||value.time>value.length||!std::isfinite(value.rate)||
       !unified_weapon_fire::retained_metadata(r))return false;
    value.playing=(playing.bits&playing.mask)!=0;value.looping=(looping.bits&looping.mask)!=0;
    out=value;return true;
}
enum class Acquire {rejected,acquired,recovery_required};
Acquire acquire_prepared(hand::Frame const& source,std::uint64_t serial,Observation const& before,
    Controls const& controls,LatchCurve const& latch={}) noexcept {
    if(ownership.active||!lancerreload_state::owns(source,serial)||
       !lancerreload_state::item_owned(source))return Acquire::rejected;
    const auto actual=before;Controls checked{};
    // The caller just authenticated its selected node/curve. Do not spend
    // the same input deadline repeating synchronous getters before StopAnim.
    if(actual.looping||!hand::same(actual.weapon,source.weapon)||
       !prepare_controls(source,actual,checked,true)||!hand::same(checked.node,controls.node)||
       !hand::same(checked.stop,controls.stop)||!hand::same(checked.seek,controls.seek))return Acquire::rejected;
    playground::Args stop(before.node.address,L"StopAnim");hand::ObjectIdentity fn{};
    if(stop.r.size||!original_node_function(stop,L"StopAnim",fn)||!hand::same(fn,controls.stop)||
       !lancerreload_state::owns(source,serial))return Acquire::rejected;
    ownership={};ownership.initial=actual;ownership.last=actual;ownership.controls=checked;ownership.latch=latch;
    ownership.inventory=lancerreload_inventory::identity(source);ownership.serial=serial;
    ownership.active=true;ownership.stop_dispatched=true;
    // Ownership is durable before the call. Failure must be drained, not
    // retried as another acquisition or forgotten when tracking disappears.
    if(!stop.run())return Acquire::recovery_required;
    Observation after{};
    if(!read_owned(after)||after.playing||after.rate!=actual.rate||after.looping!=actual.looping)
        return Acquire::recovery_required;
    ownership.last=after;ownership.stopped=true;return Acquire::acquired;
}
Acquire acquire(hand::Frame const& source,std::uint64_t serial) noexcept {
    if(ownership.active||!lancerreload_state::owns(source,serial)||
       !lancerreload_state::item_owned(source))return Acquire::rejected;
    Observation before{};Controls controls{};LatchCurve latch{};
    if(!observe(source,before)||before.looping||!read_latch_curve(source,latch,&before)||
       !hand::same(latch.sequence,before.sequence)||!prepare_controls(source,before,controls,true)||
       !lancerreload_state::owns(source,serial))return Acquire::rejected;
    for(const auto& receipt:sampling_receipts)if(receipt.known&&hand::same(receipt.sequence,latch.sequence)&&
        receipt.content==latch.content&&receipt.game_start==source.key.game_start&&receipt.native_start==source.key.native_started_qpc){
        latch.sampling=receipt.sampling;break;
    }
    return acquire_prepared(source,serial,before,controls,latch);
}
bool reconcile_seek() noexcept {
    if(!ownership.active||!ownership.stopped)return false;
    Observation current{};
    if(!read_owned(current)||current.playing||current.rate!=ownership.last.rate||
       current.looping!=ownership.last.looping)return false;
    if(ownership.seek_pending&&std::abs(current.time-ownership.wanted_time)>1e-4f)return false;
    if(!ownership.seek_pending&&current.time!=ownership.last.time)return false;
    ownership.last=current;ownership.seek_pending=false;return true;
}
// HB's live selected-node observation already brackets the current mesh,
// asset, sequence, clock and node values. Reconcile that SAME observation;
// do not read the entire stopped node again before preparing its controls.
// This never substitutes cached identity or content for fresh admission.
bool reconcile_hammerburst_observation(Observation const& current) noexcept {
    if(!ownership.active||!ownership.stopped)return false;
    const auto& expected=ownership.last;
    if(!hand::same(current.weapon,expected.weapon)||!hand::same(current.component,expected.component)||
       !hand::same(current.asset,expected.asset)||!hand::same(current.node,expected.node)||
       !hand::same(current.sequence,expected.sequence)||current.sequence_name.index!=expected.sequence_name.index||
       current.sequence_name.number!=expected.sequence_name.number||current.length!=expected.length||
       current.reload_start!=expected.reload_start||current.playing||current.rate!=expected.rate||
       current.looping!=expected.looping||!std::isfinite(current.time)||current.time<0||current.time>current.length)return false;
    if(ownership.seek_pending&&std::abs(current.time-ownership.wanted_time)>1e-4f)return false;
    if(!ownership.seek_pending&&current.time!=expected.time)return false;
    ownership.last=current;ownership.seek_pending=false;return true;
}
bool seek_impl(hand::Frame const& source,std::uint64_t serial,float time,bool settling) noexcept {
    if(!ownership.active||serial!=ownership.serial||!(lancerreload_inventory::identity(source)==ownership.inventory)||
       !(ownership.visual_only?nativeequipment::current(nullptr,source):lancerreload_state::owns(source,serial))||
       (settling?(!playground::instance(source.weapon.address,L"Weap_COGPistol")||
            !source.left_grip_known||source.left_grip_down||time<ownership.last.time):!lancerreload_state::item_owned(source))||
       !std::isfinite(time)||time<0||time>ownership.last.length)return false;
    Controls controls{};
    if(playground::instance(source.weapon.address,L"Weap_LocustAssaultRifle")){
        Observation current{};
        if(!observe_expected(source,current)||!reconcile_hammerburst_observation(current)||
           !prepare_controls(source,current,controls,true))return false;
    }else if(!reconcile_seek()||!prepare_controls(source,ownership.last,controls))return false;
    if(!hand::same(controls.seek,ownership.controls.seek))return false;
    playground::Args call(ownership.last.node.address,L"SetPosition");hand::ObjectIdentity fn{};
    const auto time_field=call.prop(L"NewTime"),notify_field=call.prop(L"bFireNotifies");
    if(call.r.size!=8||!input_field(call,time_field,L"FloatProperty",4)||
       !input_field(call,notify_field,L"BoolProperty",4)||
       get<unsigned>(time_field+0x94)!=controls.time_offset||get<unsigned>(notify_field+0x94)!=controls.notify_offset||
       get<unsigned>(notify_field+0xd0)!=controls.notify_mask||
       !original_node_function(call,L"SetPosition",fn)||!hand::same(fn,controls.seek)||
       !nativelancerfollow::bool_arg(call,L"bFireNotifies",false)||!call.set(L"NewTime",time).valid||
       !(ownership.visual_only?nativeequipment::current(nullptr,source):lancerreload_state::owns(source,serial)))return false;
    ownership.seek_pending=true;ownership.wanted_time=time;
    return call.run()&&reconcile_seek();
}
bool seek(hand::Frame const& source,std::uint64_t serial,float time) noexcept {
    return seek_impl(source,serial,time,false);
}
// Automatic spring return on the ALREADY owned Snub closing branch. A new
// motion/shot/inventory operation is not allowed; notifies remain disabled.
bool seek_settling(hand::Frame const& source,std::uint64_t serial,float time) noexcept {
    return seek_impl(source,serial,time,true);
}
bool restore_closed_snub() noexcept {
    if(!ownership.active||!ownership.stopped||!playground::instance(ownership.inventory.weapon.address,L"Weap_COGPistol")||
       !weaponhand::current_gt(nullptr))return false;
    Observation actual{};
    if(!read_owned(actual)||actual.playing||actual.rate!=ownership.last.rate||actual.looping!=ownership.last.looping)return false;
    if(ownership.seek_pending){
        // Restoration may settle an uncertain previous setter only after an
        // actual same-node observation proves either its old or wanted time.
        if(std::abs(actual.time-ownership.wanted_time)>1e-4f&&actual.time!=ownership.last.time)return false;
        ownership.seek_pending=false;
    }else if(actual.time!=ownership.last.time)return false;
    ownership.last=actual;
    playground::Args call(ownership.last.node.address,L"SetPosition");hand::ObjectIdentity fn{};
    const auto time=call.prop(L"NewTime"),notify=call.prop(L"bFireNotifies");
    const auto& c=ownership.controls;
    if(call.r.size!=8||!input_field(call,time,L"FloatProperty",4)||!input_field(call,notify,L"BoolProperty",4)||
       get<unsigned>(time+0x94)!=c.time_offset||get<unsigned>(notify+0x94)!=c.notify_offset||get<unsigned>(notify+0xd0)!=c.notify_mask||
       !original_node_function(call,L"SetPosition",fn)||!hand::same(fn,c.seek)||
       !nativelancerfollow::bool_arg(call,L"bFireNotifies",false)||!call.set(L"NewTime",ownership.last.length).valid)return false;
    ownership.seek_pending=true;ownership.wanted_time=ownership.last.length;
    return call.run()&&reconcile_seek();
}
// Caller must first obtain angle from this instance's continuous hand stroke.
// Closing traverses the same opening branch in reverse, without replaying the
// later authored automatic close or any reload inventory/throw notifications.
bool seek_latch_angle(hand::Frame const& source,std::uint64_t serial,double angle) noexcept {
    if(!ownership.active||!ownership.stopped||serial!=ownership.serial)return false;
    LatchCurve current{};float time{};
    if(!read_latch_curve(source,current,&ownership.last)||!hand::same(current.sequence,ownership.latch.sequence)||
       current.content!=ownership.latch.content||!current.curve.time_for_angle(angle,time,ownership.latch.sampling))return false;
    return seek(source,serial,time);
}
// The native reload owner exits first (without refilling). Never restart the
// old HB_Reload: that would replay notifies after manual inventory committed.
bool hand_back_after_native_exit() noexcept {
    if(!ownership.active)return true;
    const auto& e=ownership.last;
    if(!weaponhand::current_gt(nullptr))return false;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    if(!meta::tables(base,schema.objects,schema.names))return false;
    for(const auto& object:{ownership.inventory.world,ownership.inventory.pawn,e.weapon,e.component,e.asset})
        if(!weaponhand::object(r,object))return false;
    meta::PointerField pawn{},world{},mesh{},asset{};
    if(!weaponhand::pointer(r,e.weapon.address,L"Instigator",ownership.inventory.pawn.address,pawn)||
       !weaponhand::pointer(r,ownership.inventory.pawn.address,L"WorldInfo",ownership.inventory.world.address,world)||
       !lancermain_asset::mesh_pointer(r,e.weapon.address,mesh)||mesh.value!=e.component.address||
       !weaponhand::pointer(r,e.component.address,L"SkeletalMesh",e.asset.address,asset))return false;
    bool reloading{};float started{};
    if(!lancerreload_state::start_time(e.weapon.address,started)||started!=e.reload_start||
       !lancerreload_state::bool_call(e.weapon.address,L"IsReloading",reloading)||reloading)return false;
    playground::Args selected(e.weapon.address,L"GetWeaponAnimNodeSeq");
    if(selected.r.size!=8||!vrequip::result_property(selected,L"ObjectProperty",8)||!selected.run())return false;
    hand::ObjectIdentity node{};meta::PointerField sequence{};
    const auto address=selected.output<uptr>();
    if(!lancermain_asset::identify(r,address,node)||!weaponhand::derives(r,address,L"AnimNodeSequence",L"Engine")||
       !weaponhand::read_pointer(r,address,L"AnimSeq",sequence)||!unified_weapon_fire::retained_metadata(r))return false;
    const bool original_replaced=!hand::same(node,e.node)||sequence.value!=e.sequence.address;
    if(!original_replaced){Observation current{};if(!read_owned(current)||current.playing)return false;}
    // Only retire our stopped node. Original state/idle animation owns what
    // follows; no old input, timer, position or PlayAnim edge is replayed.
    ownership={};return true;
}
} // namespace hammerburst_animation
