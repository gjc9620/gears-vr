// Inspection/full/no-spare and resumed empty-chamber work own a gun-only
// original clip, not ForceReload. Reuse the SAME animation ownership journal.
// No original inventory/reward/timer acceptance is invented for inspection.
namespace snub_visual_clip {
namespace hand=gears_weapon_hand;namespace anim=hammerburst_animation;namespace inv=lancerreload_inventory;
namespace meta=gripdamage_metadata;
unsigned failure{};
bool capture_started() noexcept {
    if(!anim::ownership.active||!anim::ownership.visual_only||!weaponhand::current_gt(nullptr))return false;
    const auto& id=anim::ownership.inventory;auto value=anim::ownership.last;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    if(!meta::tables(base,schema.objects,schema.names))return false;
    for(auto const& object:{id.world,id.pawn,id.weapon,value.component,value.asset,value.node})if(!weaponhand::object(r,object))return false;
    meta::PointerField mesh{},asset{},sequence{},instigator{},world{};
    if(!weaponhand::pointer(r,id.weapon.address,L"Instigator",id.pawn.address,instigator)||
       !weaponhand::pointer(r,id.pawn.address,L"WorldInfo",id.world.address,world)||
       !lancermain_asset::mesh_pointer(r,id.weapon.address,mesh)||mesh.value!=value.component.address||
       !weaponhand::pointer(r,value.component.address,L"SkeletalMesh",value.asset.address,asset)||
       !weaponhand::read_pointer(r,value.node.address,L"AnimSeq",sequence)||
       !lancermain_asset::identify(r,sequence.value,value.sequence)||!snub_animation::original_set(r,value.sequence))return false;
    weaponhand::BoolField name{},duration{};meta::RawName raw{};meta::NameText label{};
    if(!weaponhand::field(r,value.node.address,L"AnimSeqName",L"NameProperty",8,name)||!r.at(value.node.address,name.property.offset,raw)||raw.number||
       !meta::fresh_name(schema.names,raw.index,label)||!meta::exact_text(label,L"AR_Reload")||
       !weaponhand::field(r,value.sequence.address,L"SequenceLength",L"FloatProperty",4,duration)||
       !r.at(value.sequence.address,duration.property.offset,value.length)||!std::isfinite(value.length)||value.length<=0||value.length>10||
       !lancerreload_state::start_time(id.weapon.address,value.reload_start)||!unified_weapon_fire::retained_metadata(r))return false;
    value.sequence_name={raw.index,raw.number};anim::ownership.last=value;
    anim::Observation current{};if(!anim::read_owned(current))return false;
    anim::ownership.last=current;return true;
}
anim::Acquire start(hand::Frame const& f,std::uint64_t transaction) noexcept {
    failure=1;
    if(!transaction||anim::ownership.active||!playground::instance(f.weapon.address,L"Weap_COGPistol")||
       !lancerreload_state::item_owned(f)||!nativeequipment::current(nullptr,f)||!lancer_charging::prepare_reload_name(f))return anim::Acquire::rejected;
    lancermain_asset::Evidence asset{};hand::ObjectIdentity node{};
    if(lancermain_asset::read(f,asset,lancermain_asset::AssetScope::basic_hold)!=lancermain_asset::Status::ready||
       !anim::selected_node(f,node))return anim::Acquire::rejected;
    playground::Args play(f.weapon.address,L"PlayWeaponAnim");
    if(!lancer_charging::script_call(play,L"PlayWeaponAnim")||!vrequip::result_property(play,L"FloatProperty",4))return anim::Acquire::rejected;
    struct Parameter {const wchar_t* label;const wchar_t* type;unsigned bytes;};
    const Parameter fields[]={{L"AnimName",L"NameProperty",8},{L"Rate",L"FloatProperty",4},{L"BlendInTime",L"FloatProperty",4},
        {L"BlendOutTime",L"FloatProperty",4},{L"bLooping",L"BoolProperty",4},{L"bOverride",L"BoolProperty",4}};
    std::array<std::pair<unsigned,unsigned>,7> spans{};std::array<unsigned,7> masks{};unsigned n=0;
    for(auto const& p:fields){const auto field=play.prop(p.label);if(!anim::input_field(play,field,p.type,p.bytes))return anim::Acquire::rejected;
        if(!std::wcscmp(p.type,L"BoolProperty")){masks[n]=get<unsigned>(field+0xd0);if(!masks[n]||(masks[n]&(masks[n]-1)))return anim::Acquire::rejected;}
        spans[n++]={get<unsigned>(field+0x94),p.bytes};}
    spans[n++]={get<unsigned>(play.prop(L"ReturnValue")+0x94),4};
    for(unsigned i=0;i<n;++i)for(unsigned j=i+1;j<n;++j)
        if(spans[i].first<spans[j].first+spans[j].second&&spans[j].first<spans[i].first+spans[i].second&&
           !(spans[i].first==spans[j].first&&masks[i]&&masks[j]&&!(masks[i]&masks[j])))return anim::Acquire::rejected;
    if(!play.set(L"AnimName",Name{lancer_charging::reload_indices[0],0}).valid||!play.set(L"Rate",1.f).valid||
       !play.set(L"BlendInTime",0.f).valid||!play.set(L"BlendOutTime",0.f).valid||
       !nativelancerfollow::bool_arg(play,L"bLooping",false)||!nativelancerfollow::bool_arg(play,L"bOverride",true)||
       !nativeequipment::current(nullptr,f))return anim::Acquire::rejected;
    anim::ownership={};auto& owner=anim::ownership;owner.inventory=inv::identity(f);owner.serial=transaction;
    owner.active=owner.visual_only=true;owner.last.weapon=f.weapon;owner.last.component=asset.component;
    owner.last.asset=asset.asset;owner.last.node=node;
    // Durable BEFORE start. Failure is cleanup, not permission to replay Play.
    failure=2;if(!play.run()||!capture_started())return anim::Acquire::recovery_required;
    playground::Args stop(owner.last.node.address,L"StopAnim");hand::ObjectIdentity function{};
    failure=3;if(stop.r.size||!anim::original_node_function(stop,L"StopAnim",function))return anim::Acquire::recovery_required;
    owner.controls.node=owner.last.node;owner.controls.stop=function;owner.stop_dispatched=true;
    if(!stop.run()||!capture_started()||owner.last.playing)return anim::Acquire::recovery_required;
    owner.stopped=true;failure=4;
    // SetPosition's typed schema does not need a tracking/pose permission.
    // Establish only this owned clip's neutral closed state; no notifies.
    playground::Args seek(owner.last.node.address,L"SetPosition");
    if(seek.r.size!=8||!anim::original_node_function(seek,L"SetPosition",owner.controls.seek)||
       !anim::input_field(seek,seek.prop(L"NewTime"),L"FloatProperty",4)||
       !anim::input_field(seek,seek.prop(L"bFireNotifies"),L"BoolProperty",4))return anim::Acquire::recovery_required;
    auto& c=owner.controls;c.time_offset=get<unsigned>(seek.prop(L"NewTime")+0x94);
    c.notify_offset=get<unsigned>(seek.prop(L"bFireNotifies")+0x94);c.notify_mask=get<unsigned>(seek.prop(L"bFireNotifies")+0xd0);
    if(!c.notify_mask||(c.notify_mask&(c.notify_mask-1))||
       !(c.time_offset+4<=c.notify_offset||c.notify_offset+4<=c.time_offset)||!anim::restore_closed_snub())return anim::Acquire::recovery_required;
    failure=0;return anim::Acquire::acquired;
}
bool restore() noexcept {
    if(!anim::ownership.active)return true;
    if(!anim::ownership.visual_only||!weaponhand::current_gt(nullptr))return false;
    auto& owner=anim::ownership;
    for(auto const& object:{owner.inventory.world,owner.inventory.pawn,owner.inventory.weapon,owner.last.component,owner.last.asset,owner.last.node})
        if(!lancerreload_ui::object(object))return false;
    // An actual replacement owner must not be stopped by this retired clip.
    const auto selected=playground::pointer(owner.last.component.address,L"Animations");
    if(selected!=owner.last.node.address){hand::ObjectIdentity replacement{};
        if(!selected||!nativeequipment::identify(nullptr,selected,replacement)||!playground::instance(selected,L"AnimNode"))return false;
        owner={};return true;
    }
    const auto sequence=playground::pointer(owner.last.node.address,L"AnimSeq");
    if(hand::identity_valid(owner.last.sequence)&&sequence!=owner.last.sequence.address){hand::ObjectIdentity replacement{};
        if(!sequence){
            namespace meta=gripdamage_metadata;meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
            weaponhand::BoolField field{};meta::RawName raw{};meta::PointerField current{};meta::NameText label{};
            if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(r,owner.last.node)||
               !weaponhand::field(r,owner.last.node.address,L"AnimSeqName",L"NameProperty",8,field)||!r.at(owner.last.node.address,field.property.offset,raw)||raw.number||
               !meta::fresh_name(schema.names,raw.index,label)||!meta::exact_text(label,L"None")||
               !weaponhand::read_pointer(r,owner.last.node.address,L"AnimSeq",current)||current.value||
               !unified_weapon_fire::retained_metadata(r))return false;
        }else if(!nativeequipment::identify(nullptr,sequence,replacement)||!playground::instance(sequence,L"AnimSequence"))return false;
        owner={};return true;
    }
    if(owner.stopped&&hand::identity_valid(owner.last.sequence)&&!anim::restore_closed_snub())return false;
    playground::Args stop(owner.inventory.weapon.address,L"StopWeaponAnim");
    if(!lancer_charging::script_call(stop,L"StopWeaponAnim")||stop.r.size!=4||
       !anim::input_field(stop,stop.prop(L"BlendOutTime"),L"FloatProperty",4)||!stop.set(L"BlendOutTime",0.f).run())return false;
    owner={};return true;
}
} // namespace snub_visual_clip
