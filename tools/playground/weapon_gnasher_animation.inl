// Original SG_Chamber selection/control. Other original clips are read only
// during acquisition. Normal game-thread engine calls; no bone writer.
// This owner is independent of other guns' private animation/cleanup state.
namespace gnasher_animation {
namespace hand=gears_weapon_hand;namespace inv=lancerreload_inventory;
namespace anim=hammerburst_animation;namespace meta=gripdamage_metadata;
#include "weapon_gnasher_animation_acquire.inl"
bool owns_weapon(hand::ObjectIdentity const& weapon) noexcept {
    return owner.active&&hand::same(owner.identity.weapon,weapon);
}
bool support_granted(hand::Frame const& f) noexcept {
    hand::GripSnapshot g{};
    return hand::frame_active(f,weaponhand::clock_ns(nullptr))&&
        weaponhand::client.snapshot_on_gt(g)&&gears_gnasher_raw::support_held(f,g,weaponhand::clock_ns(nullptr));
}
bool shot_admitted(hand::Frame const& f) noexcept {
    const auto* e=gnasher_fire::entry(f,false);
    return e&&e->identity==inv::identity(f)&&nativeequipment::current(nullptr,f)&&
        e->profile==f.controllers.profile_generation&&e->reference==f.reference_generation&&
        !e->cycle.pending()&&!e->cycle.faulted()&&e->cycle.confirmed()&&e->mechanical.mechanism_pending()&&
        e->mechanical_shot==e->cycle.receipt().original_event_sequence&&e->admitted_shot.accepted&&
        e->admitted_shot.original_event_sequence==e->mechanical_shot;
}
bool observe(hand::Frame const& f,anim::Observation& out,bool chamber_only) noexcept {
    out={};lancermain_asset::Evidence asset{};lancermain_asset::HeldAsset kind{};
    if(!nativeequipment::current(nullptr,f)||lancermain_asset::read(f,asset,
       lancermain_asset::AssetScope::basic_hold,&kind)!=lancermain_asset::Status::ready||
       kind!=lancermain_asset::HeldAsset::gnasher)return false;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    if(!meta::tables(base,schema.objects,schema.names))return false;
    meta::PointerField node{},sequence{};anim::Observation value{};
    value.weapon=f.weapon;value.component=asset.component;value.asset=asset.asset;
    if(!weaponhand::read_pointer(r,asset.component.address,L"Animations",node)||
       !lancermain_asset::identify(r,node.value,value.node)||
       !weaponhand::derives(r,node.value,L"AnimNodeSequence",L"Engine")||
       !weaponhand::read_pointer(r,node.value,L"AnimSeq",sequence)||
       !lancermain_asset::identify(r,sequence.value,value.sequence)||
       !weaponhand::derives(r,sequence.value,L"AnimSequence",L"Engine"))return false;
    const auto set=value.sequence.outer;
    if(!set||!weaponhand::derives(r,set,L"AnimSet",L"Engine"))return false;
    auto package=set;bool original_package{};
    for(unsigned i=0;i<8&&package;++i){
        hand::ObjectIdentity id{};if(!lancermain_asset::identify(r,package,id))return false;
        if(!id.outer){unsigned slot{};
            original_package=r.path(package,slot)&&r.named(slot,L"COG_Gnasher")&&
                weaponhand::derives(r,package,L"Package",L"Core");break;}
        package=id.outer;
    }
    if(!original_package)return false;
    // A template node or unrelated node is never writable, even with the right sequence name.
    auto outer=get<uptr>(node.value+0x40);bool attached{};
    for(unsigned i=0;i<8&&outer;++i){
        if(outer==asset.component.address||outer==f.weapon.address){attached=true;break;}
        hand::ObjectIdentity id{};if(!lancermain_asset::identify(r,outer,id))return false;outer=id.outer;
    }
    if(!attached)return false;
    weaponhand::BoolField name{},time{},rate{},length{},playing{},looping{};meta::RawName label{};meta::NameText text{};
    if(!weaponhand::field(r,node.value,L"AnimSeqName",L"NameProperty",8,name)||!r.at(node.value,name.property.offset,label)||label.number||
       !meta::fresh_name(schema.names,label.index,text)||(chamber_only&&!meta::exact_text(text,L"SG_Chamber"))||
       !weaponhand::field(r,node.value,L"CurrentTime",L"FloatProperty",4,time)||!r.at(node.value,time.property.offset,value.time)||
       !weaponhand::field(r,node.value,L"Rate",L"FloatProperty",4,rate)||!r.at(node.value,rate.property.offset,value.rate)||
       !weaponhand::field(r,sequence.value,L"SequenceLength",L"FloatProperty",4,length)||!r.at(sequence.value,length.property.offset,value.length)||
       !weaponhand::boolean(r,node.value,L"bPlaying",playing)||!weaponhand::boolean(r,node.value,L"bLooping",looping)||
       !std::isfinite(value.time)||!std::isfinite(value.rate)||!std::isfinite(value.length)||
       value.length<=0||value.length>10||value.time<0||value.time>value.length)return false;
    value.sequence_name={label.index,label.number};value.playing=(playing.bits&playing.mask)!=0;value.looping=(looping.bits&looping.mask)!=0;
    meta::PointerField n2{},s2{};float t2{},r2{},l2{};meta::RawName name2{};weaponhand::BoolField p2{},b2{};
    if(!weaponhand::read_pointer(r,asset.component.address,L"Animations",n2)||!meta::same_pointer(node,n2)||
       !weaponhand::read_pointer(r,node.value,L"AnimSeq",s2)||!meta::same_pointer(sequence,s2)||
       !r.at(node.value,name.property.offset,name2)||name2.index!=label.index||name2.number!=label.number||
       !r.at(node.value,time.property.offset,t2)||t2!=value.time||!r.at(node.value,rate.property.offset,r2)||r2!=value.rate||
       !r.at(sequence.value,length.property.offset,l2)||l2!=value.length||
       !weaponhand::boolean(r,node.value,L"bPlaying",p2)||p2.bits!=playing.bits||p2.mask!=playing.mask||
       !weaponhand::boolean(r,node.value,L"bLooping",b2)||b2.bits!=looping.bits||b2.mask!=looping.mask||
       !unified_weapon_fire::retained_metadata(r)||!nativeequipment::current(nullptr,f))return false;
    out=value;return true;
}
bool original_weapon_function(playground::Args const& call,const wchar_t* label,
                              hand::ObjectIdentity& out) noexcept {
    if(!call.valid||!registered(call.r.function)||!playground::named(call.r.function,label))return false;
    const auto klass=get<uptr>(call.r.function+0x40),package=klass?get<uptr>(klass+0x40):0;
    return registered(klass)&&registered(package)&&playground::named(klass,L"WarWeapon")&&
        playground::named(package,L"WarfareGame")&&get<uptr>(package+0x40)==0&&
        nativeequipment::identify(nullptr,call.r.function,out);
}
bool prepare_selection(hand::Frame const& f,Selection& out) noexcept {
    out={};Selection value{};
    if(!gnasher_animation::observe(f,value.before,false)||
       !anim::prepare_controls(f,value.before,value.controls,true))return false;
    // The original WarWeapon helper selects GetWeaponAnimNodeSeq(), so prove
    // it is this held component's observed node before calling that helper.
    playground::Args getter(f.weapon.address,L"GetWeaponAnimNodeSeq"),select(f.weapon.address,L"SetWeaponAnim");
    hand::ObjectIdentity getter_identity{};
    if(getter.r.size!=8||!vrequip::result_property(getter,L"ObjectProperty",8)||
       !original_weapon_function(getter,L"GetWeaponAnimNodeSeq",getter_identity)||
       select.r.size!=8||!anim::input_field(select,select.prop(L"AnimName"),L"NameProperty",8)||
       !original_weapon_function(select,L"SetWeaponAnim",value.selector)||
       !nativeequipment::current(nullptr,f)||!getter.run()||getter.output<uptr>()!=value.before.node.address||
       !nativeequipment::current(nullptr,f))return false;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(r,value.before.sequence))return false;
    const auto set=value.before.sequence.outer;
    weaponhand::BoolField list{},cock{};meta::Table table{};meta::RawName wanted{};meta::NameText text{};
    if(!set||!weaponhand::derives(r,set,L"AnimSet",L"Engine")||
       !weaponhand::field(r,set,L"Sequences",L"ArrayProperty",16,list)||!r.at(set,list.property.offset,table)||
       !table.data||table.count<1||table.count>256||table.capacity<table.count||table.capacity>1024||
       !weaponhand::field(r,f.weapon.address,L"WeaponCockAnim",L"NameProperty",8,cock)||
       !r.at(f.weapon.address,cock.property.offset,wanted)||wanted.number||
       !meta::fresh_name(schema.names,wanted.index,text)||!meta::exact_text(text,L"SG_Chamber"))return false;
    const auto property=r.result.identities[list.property.identity].address;
    uptr element{};meta::Property inner{};
    if(!r.at(property,0xd0,element)||!r.property(element,inner)||
       r.result.identities[inner.identity].outer!=property||inner.dimension!=1||inner.element_bytes!=8||
       !weaponhand::core_type(r,inner.type,L"ObjectProperty"))return false;
    std::array<uptr,256> members{},again{};
    if(!copy(members.data(),table.data,std::size_t(table.count)*sizeof(uptr)))return false;
    unsigned target_count{},before_count{};
    for(int i=0;i<table.count;++i){
        const auto address=members[unsigned(i)];hand::ObjectIdentity sequence{};
        weaponhand::BoolField name{};meta::RawName label{};
        if(!lancermain_asset::identify(r,address,sequence)||sequence.outer!=set||
           !weaponhand::derives(r,address,L"AnimSequence",L"Engine")||
           !weaponhand::field(r,address,L"SequenceName",L"NameProperty",8,name)||
           !r.at(address,name.property.offset,label))return false;
        if(hand::same(sequence,value.before.sequence))++before_count;
        if(label.index==wanted.index&&label.number==wanted.number){value.target=sequence;++target_count;}
    }
    meta::Table after{};meta::RawName wanted_after{};anim::Observation selected_after{};
    if(target_count!=1||before_count!=1||!r.at(set,list.property.offset,after)||
       after.data!=table.data||after.count!=table.count||after.capacity!=table.capacity||
       !copy(again.data(),after.data,std::size_t(after.count)*sizeof(uptr))||again!=members||
       !r.at(f.weapon.address,cock.property.offset,wanted_after)||wanted_after.index!=wanted.index||wanted_after.number!=wanted.number||
       !unified_weapon_fire::retained_metadata(r)||!gnasher_animation::observe(f,selected_after,false)||
       !hand::same(selected_after.node,value.before.node)||!hand::same(selected_after.sequence,value.before.sequence)||
       !hand::same(selected_after.component,value.before.component)||!hand::same(selected_after.asset,value.before.asset))return false;
    value.name={wanted.index,wanted.number};value.needed=!hand::same(value.target,value.before.sequence);
    out=value;return true;
}
bool dispatch_selection(hand::Frame const& f,Selection const& prepared,bool& dispatched) noexcept {
    anim::Observation before{};hand::ObjectIdentity fn{};
    playground::Args select(f.weapon.address,L"SetWeaponAnim");
    if(!gnasher_animation::observe(f,before,false)||!hand::same(before.node,prepared.before.node)||
       !hand::same(before.sequence,prepared.before.sequence)||!hand::same(before.component,prepared.before.component)||
       !hand::same(before.asset,prepared.before.asset)||select.r.size!=8||
       !anim::input_field(select,select.prop(L"AnimName"),L"NameProperty",8)||
       !original_weapon_function(select,L"SetWeaponAnim",fn)||!hand::same(fn,prepared.selector)||
       !select.set(L"AnimName",prepared.name).valid||!support_granted(f)||!nativeequipment::current(nullptr,f))return false;
    // Original SetWeaponAnim calls SetAnim only, not PlayAnim. Acquisition
    // must still prove the node stopped; physical seeks use SetPosition(false).
    dispatched=true;return select.run();
}
bool current(hand::Frame const& f,anim::Observation& out) noexcept {
    if(!owner.active||owner.call_in_flight||owner.selection_pending||!owner.stop_dispatched||!(owner.identity==inv::identity(f))||
       !hand::same(owner.controller,f.controller)||owner.profile!=f.controllers.profile_generation||
       owner.reference!=f.reference_generation||!gnasher_animation::observe(f,out))return false;
    const auto& e=owner.last;
    if(!hand::same(e.node,out.node)||!hand::same(e.sequence,out.sequence)||
       !hand::same(e.component,out.component)||!hand::same(e.asset,out.asset)||
       out.sequence_name.index!=e.sequence_name.index||out.sequence_name.number!=e.sequence_name.number||
       out.length!=e.length||out.rate!=e.rate||out.looping!=e.looping)return false;
    if(owner.stop_dispatched&&out.playing)return false;
    return !owner.seek_pending||std::abs(out.time-owner.wanted)<=1e-4f;
}
bool pending_work() noexcept {return owner.active;}
bool owns_controller(uptr pc) noexcept {return owner.active&&owner.controller.address==pc;}
inv::Identity owned_identity() noexcept {return owner.identity;}
// Cleanup lifetime is the already acquired graph, not an input permit. The
// Native registration must still be live; neither a fresh hand Frame nor a
// selected-weapon route is fabricated for this read/stop-only responsibility.
bool observe_owned_target(anim::Observation& out) noexcept {
    out={};if(!owner.active||!weaponhand::current_gt(nullptr)||
       !weaponhand::client.registration_alive(owner.identity.registration,owner.identity.native_start))return false;
    const auto& id=owner.identity;const auto& e=owner.last;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    if(!meta::tables(base,schema.objects,schema.names)||!hand::same(id.weapon,e.weapon))return false;
    for(const auto& object:{id.world,owner.controller,id.pawn,id.weapon,e.component,e.asset,e.node,owner.requested_sequence})
        if(!weaponhand::object(r,object))return false;
    meta::PointerField pawn{},world{},mesh{},asset{},node{},sequence{};
    if(!weaponhand::pointer(r,id.weapon.address,L"Instigator",id.pawn.address,pawn)||
       !weaponhand::pointer(r,id.pawn.address,L"WorldInfo",id.world.address,world)||
       !lancermain_asset::mesh_pointer(r,id.weapon.address,mesh)||mesh.value!=e.component.address||
       !weaponhand::pointer(r,e.component.address,L"SkeletalMesh",e.asset.address,asset)||
       !weaponhand::pointer(r,e.component.address,L"Animations",e.node.address,node)||
       !weaponhand::pointer(r,e.node.address,L"AnimSeq",owner.requested_sequence.address,sequence)||
       !weaponhand::derives(r,id.weapon.address,L"Weap_Shotgun",L"WarfareGame")||
       !weaponhand::derives(r,e.node.address,L"AnimNodeSequence",L"Engine")||
       !weaponhand::derives(r,owner.requested_sequence.address,L"AnimSequence",L"Engine"))return false;
    auto parent=owner.requested_sequence.outer;bool original_package{};
    if(!parent||!weaponhand::derives(r,parent,L"AnimSet",L"Engine"))return false;
    for(unsigned i=0;i<8&&parent;++i){
        hand::ObjectIdentity object{};if(!lancermain_asset::identify(r,parent,object))return false;
        if(!object.outer){unsigned slot{};
            original_package=r.path(parent,slot)&&r.named(slot,L"COG_Gnasher")&&
                weaponhand::derives(r,parent,L"Package",L"Core");break;}
        parent=object.outer;
    }
    if(!original_package)return false;
    auto outer=e.node.outer;bool attached{};
    for(unsigned i=0;i<8&&outer;++i){
        if(outer==e.component.address||outer==id.weapon.address){attached=true;break;}
        hand::ObjectIdentity object{};if(!lancermain_asset::identify(r,outer,object))return false;outer=object.outer;
    }
    if(!attached)return false;
    anim::Observation value{};value.weapon=id.weapon;value.component=e.component;value.asset=e.asset;
    value.node=e.node;value.sequence=owner.requested_sequence;value.sequence_name=owner.requested_name;
    weaponhand::BoolField name{},sequence_name{},time{},rate{},length{},playing{},looping{};
    meta::RawName label{},sequence_label{};meta::NameText text{};
    if(!weaponhand::field(r,e.node.address,L"AnimSeqName",L"NameProperty",8,name)||
       !r.at(e.node.address,name.property.offset,label)||label.index!=owner.requested_name.index||label.number!=owner.requested_name.number||
       !meta::fresh_name(schema.names,label.index,text)||!meta::exact_text(text,L"SG_Chamber")||
       !weaponhand::field(r,value.sequence.address,L"SequenceName",L"NameProperty",8,sequence_name)||
       !r.at(value.sequence.address,sequence_name.property.offset,sequence_label)||sequence_label.index!=label.index||sequence_label.number!=label.number||
       !weaponhand::field(r,e.node.address,L"CurrentTime",L"FloatProperty",4,time)||!r.at(e.node.address,time.property.offset,value.time)||
       !weaponhand::field(r,e.node.address,L"Rate",L"FloatProperty",4,rate)||!r.at(e.node.address,rate.property.offset,value.rate)||
       !weaponhand::field(r,value.sequence.address,L"SequenceLength",L"FloatProperty",4,length)||!r.at(value.sequence.address,length.property.offset,value.length)||
       !weaponhand::boolean(r,e.node.address,L"bPlaying",playing)||!weaponhand::boolean(r,e.node.address,L"bLooping",looping)||
       !std::isfinite(value.time)||!std::isfinite(value.rate)||!std::isfinite(value.length)||
       value.length<=0||value.length>10||value.time<0||value.time>value.length)return false;
    playground::Args stop(e.node.address,L"StopAnim"),seek(e.node.address,L"SetPosition");
    hand::ObjectIdentity stop_id{},seek_id{};const auto t=seek.prop(L"NewTime"),n=seek.prop(L"bFireNotifies");
    if(stop.r.size||seek.r.size!=8||!anim::original_node_function(stop,L"StopAnim",stop_id)||!hand::same(stop_id,owner.controls.stop)||
       !anim::original_node_function(seek,L"SetPosition",seek_id)||!hand::same(seek_id,owner.controls.seek)||
       !anim::input_field(seek,t,L"FloatProperty",4)||!anim::input_field(seek,n,L"BoolProperty",4)||
       get<unsigned>(t+0x94)!=owner.controls.time_offset||get<unsigned>(n+0x94)!=owner.controls.notify_offset||
       get<unsigned>(n+0xd0)!=owner.controls.notify_mask)return false;
    meta::PointerField p2{},w2{},m2{},a2{},n2{},s2{};meta::RawName label2{},sequence2{};
    weaponhand::BoolField playing2{},looping2{};float time2{},rate2{},length2{};
    if(!weaponhand::pointer(r,id.weapon.address,L"Instigator",id.pawn.address,p2)||!meta::same_pointer(pawn,p2)||
       !weaponhand::pointer(r,id.pawn.address,L"WorldInfo",id.world.address,w2)||!meta::same_pointer(world,w2)||
       !lancermain_asset::mesh_pointer(r,id.weapon.address,m2)||!meta::same_pointer(mesh,m2)||
       !weaponhand::pointer(r,e.component.address,L"SkeletalMesh",e.asset.address,a2)||!meta::same_pointer(asset,a2)||
       !weaponhand::pointer(r,e.component.address,L"Animations",e.node.address,n2)||!meta::same_pointer(node,n2)||
       !weaponhand::pointer(r,e.node.address,L"AnimSeq",value.sequence.address,s2)||!meta::same_pointer(sequence,s2)||
       !r.at(e.node.address,name.property.offset,label2)||label2.index!=label.index||label2.number!=label.number||
       !r.at(value.sequence.address,sequence_name.property.offset,sequence2)||sequence2.index!=sequence_label.index||sequence2.number!=sequence_label.number||
       !r.at(e.node.address,time.property.offset,time2)||time2!=value.time||!r.at(e.node.address,rate.property.offset,rate2)||rate2!=value.rate||
       !r.at(value.sequence.address,length.property.offset,length2)||length2!=value.length||
       !weaponhand::boolean(r,e.node.address,L"bPlaying",playing2)||playing2.bits!=playing.bits||playing2.mask!=playing.mask||
       !weaponhand::boolean(r,e.node.address,L"bLooping",looping2)||looping2.bits!=looping.bits||looping2.mask!=looping.mask||
       !unified_weapon_fire::retained_metadata(r))return false;
    value.playing=(playing.bits&playing.mask)!=0;value.looping=(looping.bits&looping.mask)!=0;out=value;return true;
}
bool dispatch_owned_stop(anim::Observation const& before,anim::Controls const& controls,bool& dispatched) noexcept {
    anim::Observation checked{};
    if(!observe_owned_target(checked)||!hand::same(checked.node,before.node)||!hand::same(checked.sequence,before.sequence))return false;
    playground::Args stop(before.node.address,L"StopAnim");hand::ObjectIdentity fn{};
    if(stop.r.size||!anim::original_node_function(stop,L"StopAnim",fn)||!hand::same(fn,controls.stop)||
       !weaponhand::current_gt(nullptr)||!weaponhand::client.registration_alive(owner.identity.registration,owner.identity.native_start))return false;
    dispatched=true;return stop.run();
}
bool seek(hand::Frame const& f,std::uint64_t serial,float position) noexcept {
    anim::Observation before{};
    if(serial!=owner.serial||!support_granted(f)||!current(f,before)||!std::isfinite(position)||position<0||position>before.length)return false;
    anim::Controls controls{};
    if(!anim::prepare_controls(f,before,controls,true)||!hand::same(controls.seek,owner.controls.seek))return false;
    playground::Args call(before.node.address,L"SetPosition");hand::ObjectIdentity fn{};
    const auto t=call.prop(L"NewTime"),n=call.prop(L"bFireNotifies");
    if(call.r.size!=8||!anim::input_field(call,t,L"FloatProperty",4)||!anim::input_field(call,n,L"BoolProperty",4)||
       get<unsigned>(t+0x94)!=controls.time_offset||get<unsigned>(n+0x94)!=controls.notify_offset||get<unsigned>(n+0xd0)!=controls.notify_mask||
       !anim::original_node_function(call,L"SetPosition",fn)||!hand::same(fn,controls.seek)||
       !nativelancerfollow::bool_arg(call,L"bFireNotifies",false)||!call.set(L"NewTime",position).valid||
       !nativeequipment::current(nullptr,f))return false;
    owner.seek_pending=owner.call_in_flight=true;owner.wanted=position;
    const auto completed=call.run();owner.call_in_flight=false;if(!completed)return false;
    anim::Observation after{};if(!current(f,after))return false;
    owner.last=after;owner.seek_pending=false;return true;
}
// Only the authenticated original state-exit adapter may retire this owner.
// It must prove the old node was replaced or remains stopped; never PlayAnim
// an old SG_Chamber to replay shell/sound notifies after inventory committed.
bool release_after_exit(hand::Frame const& f,std::uint64_t serial) noexcept {
    if(!owner.active)return true;
    if(owner.call_in_flight||serial!=owner.serial||!(owner.identity==inv::identity(f))||owner.profile!=f.controllers.profile_generation||
       owner.reference!=f.reference_generation||!nativeequipment::current(nullptr,f))return false;
    bool firing{},reloading{};
    if(!lancerreload_state::bool_call(f.weapon.address,L"IsFiring",firing)||firing||
       !lancerreload_state::bool_call(f.weapon.address,L"IsReloading",reloading)||reloading||
       !nativeequipment::current(nullptr,f))return false;
    const auto node=playground::pointer(owner.last.component.address,L"Animations");
    const auto sequence=node?playground::pointer(node,L"AnimSeq"):0;
    const bool selected_by_us=node==owner.last.node.address&&
        sequence==owner.requested_sequence.address;
    if(!selected_by_us){
        hand::ObjectIdentity replacement{};
        if(!sequence||!nativeequipment::identify(nullptr,sequence,replacement)||!playground::instance(sequence,L"AnimSequence"))return false;
    }else{anim::Observation after{};
        if(!gnasher_animation::observe(f,after,false)||after.playing||after.node.address!=node||after.sequence.address!=sequence)return false;}
    owner={};return true;
}
// Receipt-only retirement after original no-fill exit. This performs no seek,
// playback or input operation and may run while a different gun is selected.
bool release_owned_after_exit(inv::Identity const& identity) noexcept {
    if(!owns_weapon(identity.weapon))return true;
    if(owner.call_in_flight||!(owner.identity==identity)||!weaponhand::current_gt(nullptr)||
       !weaponhand::client.registration_alive(identity.registration,identity.native_start))return false;
    const auto& e=owner.last;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    if(!meta::tables(base,schema.objects,schema.names))return false;
    for(const auto& object:{identity.world,identity.pawn,e.weapon,e.component,e.asset,e.node,e.sequence})
        if(!weaponhand::object(r,object))return false;
    meta::PointerField pawn{},world{},mesh{},asset{},node{},sequence{};
    if(!weaponhand::pointer(r,e.weapon.address,L"Instigator",identity.pawn.address,pawn)||
       !weaponhand::pointer(r,identity.pawn.address,L"WorldInfo",identity.world.address,world)||
       !lancermain_asset::mesh_pointer(r,e.weapon.address,mesh)||mesh.value!=e.component.address||
       !weaponhand::pointer(r,e.component.address,L"SkeletalMesh",e.asset.address,asset)||
       !weaponhand::read_pointer(r,e.component.address,L"Animations",node)||
       !weaponhand::derives(r,node.value,L"AnimNodeSequence",L"Engine")||
       !weaponhand::read_pointer(r,node.value,L"AnimSeq",sequence)||
       !weaponhand::derives(r,sequence.value,L"AnimSequence",L"Engine"))return false;
    bool firing{},reloading{};
    if(!lancerreload_state::bool_call(identity.weapon.address,L"IsFiring",firing)||firing||
       !lancerreload_state::bool_call(identity.weapon.address,L"IsReloading",reloading)||reloading)return false;
    if(node.value==e.node.address&&sequence.value==owner.requested_sequence.address){
        weaponhand::BoolField playing{},name{};meta::RawName label{};
        const auto expected=sequence.value==owner.requested_sequence.address?owner.requested_name:e.sequence_name;
        if(!weaponhand::boolean(r,node.value,L"bPlaying",playing)||(playing.bits&playing.mask)||
           !weaponhand::field(r,node.value,L"AnimSeqName",L"NameProperty",8,name)||
           !r.at(node.value,name.property.offset,label)||label.index!=expected.index||
           label.number!=expected.number)return false;
    }
    meta::PointerField n2{},s2{};
    if(!weaponhand::read_pointer(r,e.component.address,L"Animations",n2)||!meta::same_pointer(node,n2)||
       !weaponhand::read_pointer(r,node.value,L"AnimSeq",s2)||!meta::same_pointer(sequence,s2)||
       !unified_weapon_fire::retained_metadata(r))return false;
    owner={};return true;
}
} // namespace gnasher_animation
