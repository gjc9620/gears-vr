// Gun-only inspection; no ForceReload, pawn animation, notify or inventory write.
// Original calls remain in the existing shared animation ownership journal.
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_R2_TRIAL)
namespace vrfire {bool boomshot_inspection_neutral(gears_weapon_hand::Frame const&) noexcept;}
#endif
namespace boomshot_inspection_clip {
namespace hand=gears_weapon_hand;namespace anim=hammerburst_animation;
namespace inv=lancerreload_inventory;namespace meta=gripdamage_metadata;
#include "weapon_boomshot_inspection_owner.inl"
#include "weapon_boomshot_inspection_play_schema.inl"
#include "weapon_boomshot_node_getter_schema.inl"
bool fire_neutral(hand::Frame const& f) noexcept {
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_R2_TRIAL)
    return vrfire::boomshot_inspection_neutral(f);
#else
    (void)f;return false;
#endif
}
struct Backend {
    struct Prepared {boomshot_inspection_source::Source source{};PlayLayout play{};NodeGetterLayout getter{};float length{},reload_start{};};
    inv::Identity identity{};inv::Counts counts{};hand::ObjectIdentity expected_sequence{};
    Name reload_name{};boomshot_animation::Curve curve{};std::uint64_t serial{};
    bool play_sent{},finish_sent{},finish_returned{};
    bool mine() const noexcept {
        return serial&&anim::ownership.active&&anim::ownership.serial==serial&&
            anim::ownership.visual_only&&anim::ownership.inventory==identity;
    }
    bool context() const noexcept {
        // Owed receipt/stop/handback may outlive tracking or client drain.
        // Fresh registration is still required by prepare/play/controls/seek;
        // cleanup authenticates the exact shared serial and original objects.
        if(!mine()||!weaponhand::current_gt(nullptr))return false;
        meta::Snapshot snapshot{};snapshot.module_base=base;meta::Reader r{snapshot,meta::Stage::final_check};
        if(!meta::tables(base,snapshot.objects,snapshot.names))return false;
        const auto& e=anim::ownership.last;
        for(const auto& object:{identity.world,identity.pawn,identity.weapon,e.component,e.asset,expected_sequence})
            if(!weaponhand::object(r,object))return false;
        meta::PointerField pawn{},world{},mesh{},asset{};inv::Counts actual{};bool reloading{};
        return weaponhand::pointer(r,identity.weapon.address,L"Instigator",identity.pawn.address,pawn)&&
            weaponhand::pointer(r,identity.pawn.address,L"WorldInfo",identity.world.address,world)&&
            lancermain_asset::mesh_pointer(r,identity.weapon.address,mesh)&&mesh.value==e.component.address&&
            weaponhand::pointer(r,e.component.address,L"SkeletalMesh",e.asset.address,asset)&&
            inv::query(identity.weapon.address,actual)&&actual==counts&&
            lancerreload_state::bool_call(identity.weapon.address,L"IsReloading",reloading)&&!reloading&&
            unified_weapon_fire::retained_metadata(r);
    }
    bool prepare(hand::Frame const& f,Prepared& out) noexcept {
        if(lancerreload_runtime::stop_requested.load(std::memory_order_acquire)||boomshot_clip::serial||
           !fire_neutral(f)||!lancerreload_state::magazine_owned(f))return false;
        Prepared value{};if(!boomshot_inspection_source::read(f,value.source))return false;
        playground::Args play(f.weapon.address,L"PlayWeaponAnim");
        if(!play_layout(play,value.play)||!lancerreload_state::start_time(f.weapon.address,value.reload_start))return false;
        meta::Snapshot snapshot{};snapshot.module_base=base;meta::Reader r{snapshot,meta::Stage::final_check};weaponhand::BoolField length{};
        if(!meta::tables(base,snapshot.objects,snapshot.names)||!weaponhand::object(r,value.source.sequence)||
           !weaponhand::field(r,value.source.sequence.address,L"SequenceLength",L"FloatProperty",4,length)||
           !r.at(value.source.sequence.address,length.property.offset,value.length)||!std::isfinite(value.length)||
           value.length<=0||value.length>10||!unified_weapon_fire::retained_metadata(r)||
           !nativeequipment::current(nullptr,f)||!lancerreload_state::magazine_owned(f))return false;
        playground::Args getter(f.weapon.address,L"GetWeaponAnimNodeSeq");
        if(!node_getter_schema(getter,f.weapon.address,r,value.getter)||!nativeequipment::current(nullptr,f))return false;
        out=value;return true;
    }
    bool journal(hand::Frame const& f,std::uint64_t id,Prepared const& p) noexcept {
        if(serial||anim::ownership.active||!id||!hand::same(f.key,p.source.key)||!nativeequipment::current(nullptr,f)||
           !lancerreload_state::magazine_owned(f))return false;
        identity=inv::identity(f);if(!identity.valid())return false;
        counts=p.source.counts;expected_sequence=p.source.sequence;reload_name=p.source.reload_name;serial=id;
        play_sent=finish_sent=finish_returned=false;curve={};
        auto& owner=anim::ownership;owner={};owner.active=owner.visual_only=true;owner.inventory=identity;owner.serial=id;
        owner.last.weapon=f.weapon;owner.last.component=p.source.component;owner.last.asset=p.source.asset;
        owner.last.sequence=expected_sequence;owner.last.sequence_name=reload_name;
        owner.last.length=p.length;owner.last.reload_start=p.reload_start;
        return true;
    }
    bool source_unchanged() const noexcept {
        if(!mine())return false;
        meta::Snapshot snapshot{};snapshot.module_base=base;meta::Reader r{snapshot,meta::Stage::final_check};
        weaponhand::BoolField name{},length{};meta::RawName raw{},again{};float duration{},after{};
        if(!meta::tables(base,snapshot.objects,snapshot.names)||!weaponhand::object(r,identity.weapon)||
           !boomshot_animation::inspection_no_notifies(r,expected_sequence)||
           !weaponhand::field(r,identity.weapon.address,L"WeaponReloadAnim",L"NameProperty",8,name)||
           !r.at(identity.weapon.address,name.property.offset,raw)||raw.index!=reload_name.index||raw.number!=reload_name.number||
           !weaponhand::field(r,expected_sequence.address,L"SequenceLength",L"FloatProperty",4,length)||
           !r.at(expected_sequence.address,length.property.offset,duration)||duration!=anim::ownership.last.length||
           snapshot.field_count+2>snapshot.fields.size())return false;
        snapshot.fields[snapshot.field_count++]=name.property;snapshot.fields[snapshot.field_count++]=length.property;
        return r.at(identity.weapon.address,name.property.offset,again)&&again.index==raw.index&&again.number==raw.number&&
            r.at(expected_sequence.address,length.property.offset,after)&&after==duration&&unified_weapon_fire::retained_metadata(r);
    }
    Dispatch play(hand::Frame const& f,Prepared const& p) noexcept {
        if(!mine()||play_sent||!(identity==inv::identity(f)))return Dispatch::not_dispatched;
        playground::Args call(f.weapon.address,L"PlayWeaponAnim");PlayLayout current{};
        playground::Args getter(f.weapon.address,L"GetWeaponAnimNodeSeq");NodeGetterLayout getter_now{};
        meta::Snapshot snapshot{};snapshot.module_base=base;meta::Reader reader{snapshot,meta::Stage::final_check};
        if(!meta::tables(base,snapshot.objects,snapshot.names)||!node_getter_schema(getter,f.weapon.address,reader,getter_now)||
           !(getter_now==p.getter))return Dispatch::not_dispatched;
        if(!play_layout(call,current)||!(current==p.play)||!context()||!source_unchanged()||
           !call.set(L"AnimName",reload_name).valid||!call.set(L"Rate",1.f).valid||
           !call.set(L"BlendInTime",0.f).valid||!call.set(L"BlendOutTime",0.f).valid||
           !nativelancerfollow::bool_arg(call,L"bLooping",false)||!nativelancerfollow::bool_arg(call,L"bOverride",true)||
           lancerreload_runtime::stop_requested.load(std::memory_order_acquire)||
           !nativeequipment::current(nullptr,f)||!lancerreload_state::magazine_owned(f)||!fire_neutral(f)||
           !nativeequipment::current(nullptr,f))return Dispatch::not_dispatched;
        play_sent=true;return call.run()?Dispatch::returned:Dispatch::unknown;
    }
    bool drop_unstarted() noexcept {
        if(!mine()||play_sent)return false;anim::ownership={};clear();return true;
    }
    bool capture() noexcept {
        if(!context())return false;
        // Receipt-only lookup of the POST-Play node; never uses the old hand
        // frame as a new input/action permit. A lost lookup leaves debt live.
        playground::Args getter(identity.weapon.address,L"GetWeaponAnimNodeSeq");
        meta::Snapshot snapshot{};snapshot.module_base=base;meta::Reader r{snapshot,meta::Stage::final_check};
        NodeGetterLayout before{},after{};hand::ObjectIdentity node{};meta::PointerField sequence{};
        if(!meta::tables(base,snapshot.objects,snapshot.names)||!node_getter_schema(getter,identity.weapon.address,r,before)||
           !context()||!getter.run()||!node_getter_schema(getter,identity.weapon.address,r,after)||!(before==after))return false;
        const auto address=getter.output<uptr>();auto& owner=anim::ownership;
        if(!lancermain_asset::identify(r,address,node)||
           !weaponhand::derives(r,address,L"AnimNodeSequence",L"Engine")||
           !weaponhand::pointer(r,address,L"AnimSeq",expected_sequence.address,sequence)||
           (hand::identity_valid(owner.last.node)&&!hand::same(owner.last.node,node))||
           !boomshot_animation::inspection_no_notifies(r,expected_sequence)||!context())return false;
        owner.last.node=node;anim::Observation actual{};
        if(!anim::read_owned(actual)||actual.looping)return false;
        owner.initial=owner.last=actual;return true;
    }
    Dispatch stop() noexcept {
        if(!mine()||anim::ownership.stop_dispatched||!context()||!hand::identity_valid(anim::ownership.last.node))return Dispatch::not_dispatched;
        playground::Args call(anim::ownership.last.node.address,L"StopAnim");hand::ObjectIdentity function{};
        if(call.r.size||!anim::original_node_function(call,L"StopAnim",function))return Dispatch::not_dispatched;
        anim::ownership.controls.node=anim::ownership.last.node;anim::ownership.controls.stop=function;
        anim::ownership.stop_dispatched=true;return call.run()?Dispatch::returned:Dispatch::unknown;
    }
    bool stopped() noexcept {
        if(!mine()||!anim::ownership.stop_dispatched||!context())return false;
        anim::Observation actual{};if(!anim::read_owned(actual)||actual.playing||actual.looping)return false;
        anim::ownership.last=actual;anim::ownership.stopped=true;return true;
    }
    bool controls(hand::Frame const& f) noexcept {
        if(!mine()||!anim::ownership.stopped||!(identity==inv::identity(f))||
           !nativeequipment::current(nullptr,f)||!lancerreload_state::magazine_owned(f))return false;
        boomshot_animation::Curve candidate{};anim::Controls controls{};
        if(!boomshot_animation::read_curve(f,anim::ownership.last,candidate)||
           !hand::same(candidate.sequence,expected_sequence)||(curve.content&&candidate.content!=curve.content)||
           !anim::prepare_controls(f,anim::ownership.last,controls,true)||
           !hand::same(controls.stop,anim::ownership.controls.stop))return false;
        curve=candidate;anim::ownership.controls=controls;return true;
    }
    bool seek(hand::Frame const& f,double q,gears_weapon::boomshot_barrel::Direction direction) noexcept {
        gears_weapon::boomshot_tracks::Path path{};float time{};
        return controls(f)&&path.bind(curve.samples)&&path.time(q,direction,time)&&anim::seek(f,serial,time);
    }
    bool finish_started() const noexcept {return finish_sent;}
    bool retire_if_replaced() noexcept {
        // Only stable registry removal/identity replacement is retirement.
        // Unreadable, stale tracking or a foreign shared serial are not proof.
        if(!mine()||!lancerreload_runtime::weapon_retired(identity.weapon))return false;
        anim::ownership={};clear();return true;
    }
    bool finish() noexcept {
        if(!mine()||!context()||!hand::identity_valid(anim::ownership.last.node))return false;
        if(!finish_sent){
            playground::Args call(identity.weapon.address,L"StopWeaponAnim");
            if(!lancer_charging::script_call(call,L"StopWeaponAnim")||call.r.size!=4||
               !anim::input_field(call,call.prop(L"BlendOutTime"),L"FloatProperty",4)||
               !call.set(L"BlendOutTime",0.f).valid||!context())return false;
            finish_sent=true;finish_returned=call.run();
        }
        // Unknown same-node dispatch is not replayed or called a completed
        // handback. Preserve it for explicit recovery, even after tracking loss.
        if(!finish_returned||!context()||!anim::hand_back_after_native_exit())return false;
        clear();return true;
    }
    void clear() noexcept {identity={};counts={};expected_sequence={};reload_name={};curve={};serial=0;play_sent=finish_sent=finish_returned=false;}
};
Backend backend;Owner<Backend> owner(backend);
}
