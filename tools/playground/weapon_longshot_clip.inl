// Gun-only original LS_Reload ownership. The existing animation journal owns
// StopAnim/SetPosition and cleanup. No pawn reload or ammunition notify runs.
#include "../vr-common/weapon_longshot_curve_receipt.h"
namespace longshot_clip {
namespace hand=gears_weapon_hand;namespace anim=hammerburst_animation;
namespace inv=lancerreload_inventory;namespace meta=gripdamage_metadata;
enum class Result {rejected,pending,ready,recovery_required};
std::uint64_t owned_serial{};
inv::Identity owned_identity{},pending_original_identity{};
std::uint64_t pending_original_serial{},pending_attempt_frame{};
bool neutral_pending{};unsigned adopt_gate{},observe_rejection{},capture_gate{},service_gate{};
bool owned() noexcept {
    const auto& owner=anim::ownership;
    return owned_serial&&owner.active&&owner.serial==owned_serial&&owner.inventory==owned_identity;
}
bool original_sequence(hand::Frame const& f,anim::Observation const& value) noexcept {
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    return meta::tables(base,schema.objects,schema.names)&&longshot_animation::original_set(r,value.sequence)&&
        unified_weapon_fire::retained_metadata(r)&&nativeequipment::current(nullptr,f);
}
#include "weapon_longshot_clip_adopt.inl"
#include "weapon_longshot_clip_clock.inl"
void report(hand::Frame const& f,Result result) noexcept {
    static unsigned rows{};static std::array<unsigned,5> previous{{~0u,~0u,~0u,~0u,~0u}};
    const std::array<unsigned,5> key{{unsigned(result),adopt_gate,observe_rejection,capture_gate,service_gate}};
    if(!log_file||rows>=20||key==previous)return;previous=key;++rows;
    const auto& value=anim::ownership.last;
    fwprintf(log_file,L"longshot_clip phase=%u adopt=%u observe=%u capture=%u service=%u frame=%llu serial=%llu pending=%llu node=%llx sequence=%llx playing=%u stopped=%u time=%.9g length=%.9g source_sync_ns=%lld\n",
        unsigned(result),adopt_gate,observe_rejection,capture_gate,service_gate,f.key.frame,owned_serial,
        pending_original_serial,value.node.address,value.sequence.address,unsigned(value.playing),unsigned(anim::ownership.stopped),
        double(value.time),double(value.length),f.controllers.sync_ns);fflush(log_file);
}
bool capture(bool cleanup=false) noexcept {
    capture_gate=1;
    if(!owned()||!weaponhand::current_gt(nullptr)||!clock_current_for_capture(cleanup))return false;
    auto value=anim::ownership.last;const auto& id=anim::ownership.inventory;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    if(!meta::tables(base,schema.objects,schema.names))return false;
    capture_gate=2;
    for(const auto& object:{id.world,id.pawn,id.weapon,value.component,value.asset,value.node})
        if(!weaponhand::object(r,object))return false;
    meta::PointerField mesh{},asset{},sequence{},pawn{},world{};
    capture_gate=3;
    if(!weaponhand::pointer(r,id.weapon.address,L"Instigator",id.pawn.address,pawn)||
       !weaponhand::pointer(r,id.pawn.address,L"WorldInfo",id.world.address,world)||
       !lancermain_asset::mesh_pointer(r,id.weapon.address,mesh)||mesh.value!=value.component.address||
       !weaponhand::pointer(r,value.component.address,L"SkeletalMesh",value.asset.address,asset)||
       !weaponhand::read_pointer(r,value.node.address,L"AnimSeq",sequence)||
       !lancermain_asset::identify(r,sequence.value,value.sequence)||!longshot_animation::original_set(r,value.sequence))return false;
    weaponhand::BoolField name{},duration{};meta::RawName raw{};meta::NameText label{};
    capture_gate=4;
    if(!weaponhand::field(r,value.node.address,L"AnimSeqName",L"NameProperty",8,name)||
       !r.at(value.node.address,name.property.offset,raw)||raw.number||
       !meta::fresh_name(schema.names,raw.index,label)||!meta::exact_text(label,L"LS_Reload")||
       !weaponhand::field(r,value.sequence.address,L"SequenceLength",L"FloatProperty",4,duration)||
       !r.at(value.sequence.address,duration.property.offset,value.length)||!std::isfinite(value.length)||
       value.length<=0||value.length>10||!lancerreload_state::start_time(id.weapon.address,value.reload_start)||
       !unified_weapon_fire::retained_metadata(r))return false;
    value.sequence_name={raw.index,raw.number};anim::ownership.last=value;
    capture_gate=5;
    anim::Observation current{};if(!anim::read_owned(current))return false;
    anim::ownership.last=current;capture_gate=0;return true;
}
Result service(hand::Frame const& f) noexcept {
    service_gate=1;
    if(pending_original_serial){const auto result=try_adopt_original(f);
        if(result!=Result::ready){report(f,result);return result;}}
    if(!owned()||!(anim::ownership.inventory==inv::identity(f))){report(f,Result::rejected);return Result::rejected;}
    service_gate=2;if(!capture()){report(f,Result::pending);return Result::pending;}
    service_gate=3;if(anim::ownership.last.playing){report(f,Result::recovery_required);return Result::recovery_required;}
    anim::ownership.stopped=true;
    anim::Controls controls{};
    service_gate=4;
    if(!nativeequipment::current(nullptr,f)||!anim::prepare_controls(f,anim::ownership.last,controls,true)){
        report(f,Result::pending);return Result::pending;}
    if(!hand::same(controls.node,anim::ownership.controls.node)||!hand::same(controls.stop,anim::ownership.controls.stop))
        {report(f,Result::recovery_required);return Result::recovery_required;}
    anim::ownership.controls=controls;
    service_gate=5;
    if(neutral_pending){
        if(anim::ownership.seek_pending&&!anim::reconcile_seek()){report(f,Result::pending);return Result::pending;}
        // Undo only the automatic pre-acquisition clip advance. Actual bolt
        // completion still requires the independent measured physical stroke.
        if(anim::ownership.last.time!=0&&!anim::seek(f,owned_serial,0.f)){report(f,Result::pending);return Result::pending;}
        neutral_pending=false;
    }
    service_gate=0;report(f,Result::ready);return Result::ready;
}
Result start(hand::Frame const& f,std::uint64_t serial,bool original_started=false) noexcept {
    if(!serial||owned_serial||pending_original_serial||anim::ownership.active||!nativelancerfollow::bound_is_longshot||
       !lancerreload_state::magazine_owned(f)||!nativeequipment::current(nullptr,f))return Result::rejected;
    ui_start_receipt={};
    adopt_gate=observe_rejection=capture_gate=service_gate=0;
    if(original_started){
        pending_original_serial=serial;pending_original_identity=inv::identity(f);pending_attempt_frame=0;
        return service(f);
    }
    lancermain_asset::Evidence asset{};hand::ObjectIdentity node{},stop_function{};
    if(lancermain_asset::read(f,asset,lancermain_asset::AssetScope::basic_hold)!=lancermain_asset::Status::ready||
       !anim::selected_node(f,node))return Result::rejected;
    playground::Args play(f.weapon.address,L"PlayWeaponAnim"),stop(node.address,L"StopAnim");
    if(!lancer_charging::script_call(play,L"PlayWeaponAnim")||!vrequip::result_property(play,L"FloatProperty",4)||
       stop.r.size||!anim::original_node_function(stop,L"StopAnim",stop_function))return Result::rejected;
    struct Parameter {const wchar_t* label;const wchar_t* type;unsigned bytes;};
    const Parameter fields[]={{L"AnimName",L"NameProperty",8},{L"Rate",L"FloatProperty",4},{L"BlendInTime",L"FloatProperty",4},
        {L"BlendOutTime",L"FloatProperty",4},{L"bLooping",L"BoolProperty",4},{L"bOverride",L"BoolProperty",4}};
    std::array<std::pair<unsigned,unsigned>,7> spans{};std::array<unsigned,7> masks{};unsigned n{};
    for(const auto& p:fields){const auto field=play.prop(p.label);
        if(!anim::input_field(play,field,p.type,p.bytes))return Result::rejected;
        if(!std::wcscmp(p.type,L"BoolProperty")){
            masks[n]=get<unsigned>(field+0xd0);if(!masks[n]||(masks[n]&(masks[n]-1)))return Result::rejected;}
        spans[n++]={get<unsigned>(field+0x94),p.bytes};}
    spans[n++]={get<unsigned>(play.prop(L"ReturnValue")+0x94),4};
    for(unsigned i=0;i<n;++i)for(unsigned j=i+1;j<n;++j)
        if(spans[i].first<spans[j].first+spans[j].second&&spans[j].first<spans[i].first+spans[i].second&&
           !(spans[i].first==spans[j].first&&masks[i]&&masks[j]&&!(masks[i]&masks[j])))return Result::rejected;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    weaponhand::BoolField field{};meta::RawName raw{},again{};meta::NameText label{};
    if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(r,f.weapon)||
       !weaponhand::field(r,f.weapon.address,L"WeaponReloadAnim",L"NameProperty",8,field)||
       !r.at(f.weapon.address,field.property.offset,raw)||raw.number||
       !meta::fresh_name(schema.names,raw.index,label)||!meta::exact_text(label,L"LS_Reload")||
       !play.set(L"AnimName",Name{raw.index,raw.number}).valid||!play.set(L"Rate",1.f).valid||
       !play.set(L"BlendInTime",0.f).valid||!play.set(L"BlendOutTime",0.f).valid||
       !nativelancerfollow::bool_arg(play,L"bLooping",false)||!nativelancerfollow::bool_arg(play,L"bOverride",true)||
       !r.at(f.weapon.address,field.property.offset,again)||again.index!=raw.index||again.number!=raw.number||
       !unified_weapon_fire::retained_metadata(r)||!lancerreload_state::magazine_owned(f)||
       !nativeequipment::current(nullptr,f))return Result::rejected;
    anim::ownership={};auto& owner=anim::ownership;owner.inventory=inv::identity(f);owner.serial=serial;
    owner.active=owner.visual_only=true;owner.last.weapon=f.weapon;owner.last.component=asset.component;
    owner.last.asset=asset.asset;owner.last.node=node;owner.controls.node=node;owner.controls.stop=stop_function;
    owned_serial=serial;owned_identity=inv::identity(f);neutral_pending=false;
    // Durable before Play. Stop is the immediate owed half of this operation,
    // so the original clip cannot advance between separate PlayerTicks.
    const bool played=play.run();
    if(!lancerreload_ui::object(node))
        return Result::recovery_required;
    owner.stop_dispatched=true;
    const bool stopped=stop.run();
    if(!played||!stopped)return Result::recovery_required;
    return service(f);
}
bool seek(hand::Frame const& f,float time,gears_longshot_curve::Receipt const* curve_receipt=nullptr) noexcept {
    return owned()&&anim::ownership.stopped&&
        (!curve_receipt||gears_longshot_curve::matches(*curve_receipt,f,anim::ownership.last))&&
        (curve_receipt?anim::seek_impl(f,owned_serial,time,false,true):anim::seek(f,owned_serial,time));
}
bool read_path(hand::Frame const& f,gears_vr::Vector handle_in_bone_raw,gears_weapon::longshot_tracks::BoltPath& out) noexcept {
    if(!owned()||!anim::ownership.stopped||!(anim::ownership.inventory==inv::identity(f)))return false;
    longshot_animation::Curve curve{};
    return longshot_animation::read_curve(f,anim::ownership.last,curve)&&
        out.bind(curve.samples,handle_in_bone_raw,nativelancerfollow::bound_units);
}
bool restore() noexcept {
    ui_start_receipt={};
    pending_original_serial=pending_attempt_frame=0;pending_original_identity={};neutral_pending=false;
    if(!owned_serial)return true;
    if(!owned()||!weaponhand::current_gt(nullptr))return false;
    auto& owner=anim::ownership;
    if(lancerreload_runtime::weapon_retired(owner.inventory.weapon)){
        owner={};owned_serial=0;return true;
    }
    for(const auto& object:{owner.inventory.world,owner.inventory.pawn,owner.inventory.weapon,owner.last.component,owner.last.asset,owner.last.node})
        if(!lancerreload_ui::object(object))return false;
    playground::Args getter(owner.inventory.weapon.address,L"GetWeaponAnimNodeSeq");
    if(getter.r.size!=8||!vrequip::result_property(getter,L"ObjectProperty",8)||!getter.run())return false;
    const auto selected=getter.output<uptr>();
    if(selected!=owner.last.node.address){hand::ObjectIdentity replacement{};
        if(!selected||!nativeequipment::identify(nullptr,selected,replacement)||!playground::instance(selected,L"AnimNode"))return false;
        owner={};owned_serial=0;return true;}
    const auto sequence=playground::pointer(owner.last.node.address,L"AnimSeq");
    if(hand::identity_valid(owner.last.sequence)&&sequence!=owner.last.sequence.address){hand::ObjectIdentity replacement{};
        if(!sequence||!nativeequipment::identify(nullptr,sequence,replacement)||!playground::instance(sequence,L"AnimSequence"))return false;
        owner={};owned_serial=0;return true;
    }
    if(!capture(true))return false;
    playground::Args stop(owner.inventory.weapon.address,L"StopWeaponAnim");
    if(!lancer_charging::script_call(stop,L"StopWeaponAnim")||stop.r.size!=4||
       !anim::input_field(stop,stop.prop(L"BlendOutTime"),L"FloatProperty",4)||!stop.set(L"BlendOutTime",0.f).run())return false;
    owner={};owned_serial=0;return true;
}
}
