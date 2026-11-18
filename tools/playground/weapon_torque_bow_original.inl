// Bow-only GT ABI preparation. No flags in an offline model grant engine
// authority: reflection and the original state are read from this live actor.
namespace torquebow_original {
namespace hand=gears_weapon_hand;namespace meta=weaponhand::meta;
struct Preparation {
    hand::ObjectIdentity weapon{};
    uptr clear_pending{},charge_ended{},get_state{},goto_state{},pending_end_property{};
    unsigned goto_bytes{},pending_offset{},pending_mask{};
    lancerreload_inventory::Counts inventory{};
    unsigned gate{};bool ready{};
    std::int64_t started_ns{},finished_ns{},input_age_begin_ns{-1},input_age_end_ns{-1};
    unsigned source_check_known{},source_current{},withdrawal_frame_sequence_match{},withdrawal_candidate_failure{},withdrawal_candidate_difference{};
};
bool original_function(playground::Args const& call,wchar_t const* owner_name,wchar_t const* package_name) noexcept {
    if(!call.valid||!registered(call.r.function)||!playground::named(get<uptr>(call.r.function+0x50),L"Function"))return false;
    const auto owner=get<uptr>(call.r.function+0x40),package=owner?get<uptr>(owner+0x40):0;
    return registered(owner)&&registered(package)&&playground::named(owner,owner_name)&&
        playground::named(package,package_name)&&!get<uptr>(package+0x40);
}
bool input_property(playground::Args& call,wchar_t const* label,wchar_t const* type,unsigned bytes,unsigned offset) noexcept {
    const auto p=call.prop(label);
    return registered(p)&&get<uptr>(p+0x40)==call.r.function&&
        playground::named(get<uptr>(p+0x50),type)&&get<unsigned>(p+0x70)==1&&
        get<unsigned>(p+0x74)==bytes&&get<unsigned>(p+0x94)==offset&&
        (get<unsigned long long>(p+0x78)&0x580)==0x80;
}
bool transition_schema(playground::Args& call,unsigned& bytes) noexcept {
    if(!original_function(call,L"Object",L"Core")||call.r.size<16||call.r.size>24||
       !input_property(call,L"NewState",L"NameProperty",8,0)||
       !input_property(call,L"Label",L"NameProperty",8,8))return false;
    std::array<uptr,16> seen{};unsigned count{},found{};
    for(auto p=get<uptr>(call.r.function+0x80);p;){
        if(count==seen.size()||!registered(p))return false;
        for(unsigned i=0;i<count;++i)if(seen[i]==p)return false;
        seen[count++]=p;const auto flags=get<unsigned long long>(p+0x78);
        if(flags&0x80){
            unsigned bit=playground::named(p,L"NewState")?1:playground::named(p,L"Label")?2:
                playground::named(p,L"bForceEvents")?4:playground::named(p,L"bKeepStack")?8:0;
            if(!bit||(found&bit))return false;found|=bit;
            if(bit>=4){
                const auto offset=get<unsigned>(p+0x94),mask=get<unsigned>(p+0xd0);
                if(!playground::named(get<uptr>(p+0x50),L"BoolProperty")||get<unsigned>(p+0x70)!=1||
                   get<unsigned>(p+0x74)!=4||offset<16||offset>call.r.size-4||
                   !mask||(mask&(mask-1))||(flags&0x580)!=0x80)return false;
            }
        }
        p=get<uptr>(p+0x68);
    }
    if((found&3)!=3)return false;bytes=call.r.size;return true;
}
bool prepare(hand::Frame const& f,Preparation& out) noexcept {
    out={};out.gate=1;out.started_ns=weaponhand::clock_ns(nullptr);
    if(f.controllers.sync_ns>0)out.input_age_begin_ns=out.started_ns-f.controllers.sync_ns;
    struct Finish {
        hand::Frame const& source;Preparation& result;
        ~Finish() noexcept {
            result.finished_ns=weaponhand::clock_ns(nullptr);
            if(source.controllers.sync_ns>0)result.input_age_end_ns=result.finished_ns-source.controllers.sync_ns;
            // Read only the existing withdrawal receipt. It is evidence for
            // the frame/sequence only. This receipt has no full cohort tag,
            // so a match is a candidate hint, NEVER a cause/identity proof.
            // No new source read, re-admission or diagnostic query occurs.
            const auto w=weaponhand::client.withdrawal_on_gt();
            if(w.frame==source.key.frame&&w.sequence==source.controllers.sequence){
                result.withdrawal_frame_sequence_match=1;result.withdrawal_candidate_failure=unsigned(w.failure);result.withdrawal_candidate_difference=w.game_difference;
            }
        }
    } finish{f,out};
    if(!nativeequipment::current(nullptr,f)||!playground::instance(f.weapon.address,L"Weap_Bow_base")||
       !vrequip::model.held_for(f.world.address,f.pawn.address,f.weapon.address))return false;
    out.weapon=f.weapon;const auto weapon=f.weapon.address;
    playground::Args clear(weapon,L"ClearPendingFire"),ended(weapon,L"ChargeEnded"),
        state(weapon,L"GetStateName"),transition(weapon,L"GotoState");
    out.gate=2;
    if(!original_function(clear,L"Weapon",L"Engine")||clear.r.size!=4||
       !input_property(clear,L"FireMode",L"IntProperty",4,0))return false;
    out.clear_pending=clear.r.function;out.gate=3;
    if(!original_function(ended,L"Weap_Bow_base",L"WarfareGame")||ended.r.size)return false;
    out.charge_ended=ended.r.function;out.gate=4;
    if(!original_function(state,L"Object",L"Core")||state.r.size!=8||
       !vrequip::result_property(state,L"NameProperty",8))return false;
    out.get_state=state.r.function;out.gate=5;
    if(!transition_schema(transition,out.goto_bytes))return false;
    out.goto_state=transition.r.function;out.gate=6;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader reader{schema,meta::Stage::function_fields};
    weaponhand::BoolField pending_end{};
    if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(reader,f.weapon)||
       !weaponhand::boolean(reader,weapon,L"bPendingEndFire",pending_end))return false;
    const auto property=schema.identities[pending_end.property.identity].address;
    const auto owner=get<uptr>(property+0x40);
    if(!playground::named(owner,L"Weap_Bow_base"))return false;
    out.pending_end_property=property;out.pending_offset=pending_end.property.offset;out.pending_mask=pending_end.mask;
    out.gate=7;
    if(!lancerreload_inventory::query(weapon,out.inventory))return false;
    out.gate=8;if(out.inventory.capacity!=1)return false;
    out.gate=9;out.source_check_known=1;
    out.source_current=unsigned(nativeequipment::current(nullptr,f));
    if(!out.source_current)return false;
    // No synthetic BeginState or charge call during preparation.
    out.gate=0;out.ready=true;return true;
}
}

