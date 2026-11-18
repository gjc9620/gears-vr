// Inside troika_entry_runtime, after mounted_geometry. The same ordinary GT
// Args->owned bridge->original Native action/call hooks execute Button R2.
// No direct bFire/PendingFire write or direct InstantFire/EndFire fallback.
namespace mounted_fire {
namespace fire=gears_troika_fire;namespace meta=weaponhand::meta;
fire::Owner responsibility;
gears_troika_source::ConsumerCursor fire_cursor;
using QualifiedAimAndHands=bool(*)(fire::Run const&) noexcept;
gears_equipment_names::Lookup input_names({L"R2",L"Active",L"Inactive"});
bool names(std::array<unsigned,3>& indices) noexcept {
    const auto began=weaponhand::clock_ns(nullptr);
    return input_names.step([](gears_equipment_names::Table& t){const auto a=get<Array>(base+0x1c34800);
        t={a.data,unsigned(a.count),unsigned(a.capacity)};return a.count>=0&&a.capacity>=0;},
        [](gears_equipment_names::Table t,unsigned index,std::array<wchar_t,32>& text){
            const auto p=get<uptr>(t.data+std::size_t(index)*8);return p&&get<unsigned>(p)==index&&copy(text.data(),p+20,sizeof text);},
        [](){return weaponhand::current_gt(nullptr)!=0;},[&](){return weaponhand::clock_ns(nullptr)-began<750000;},indices,2048)==gears_equipment_names::State::ready;
}
template<std::size_t N,std::size_t C,std::size_t P>bool function(meta::Reader& reader,playground::Args& call,
    wchar_t const (&label)[N],wchar_t const (&owner_name)[C],wchar_t const (&package_name)[P]) noexcept {
    unsigned fn{},kind{},declaring{},klass{},package{};meta::Structure structure{};
    return call.valid&&reader.path(call.r.function,fn)&&reader.named(fn,label)&&
        reader.path(reader.result.identities[fn].klass,kind)&&weaponhand::core_type(reader,kind,L"Function")&&
        reader.path(reader.result.identities[fn].outer,declaring)&&reader.named(declaring,owner_name)&&
        reader.path(reader.result.identities[declaring].klass,klass)&&weaponhand::core_type(reader,klass,L"Class")&&
        reader.path(reader.result.identities[declaring].outer,package)&&reader.named(package,package_name)&&
        !reader.result.identities[package].outer&&reader.structure(call.r.function,structure)&&
        unified_weapon_fire::retained_metadata(reader);
}
bool exact_resource(fire::Run const& run,troika_original::EntryOwner const& entry) noexcept {
    auto const& q=run.source;auto const& s=q.frame;auto const& r=entry.receipt;
    if(!weaponhand::current_gt(nullptr)||s.native.game_thread!=weaponhand::current_gt(nullptr)||
       weaponhand::client.cookie()!=s.native.registration_cookie||weaponhand::client.native_started_qpc()!=s.native.native_started_qpc||
       entry.binding!=q.entry_binding||entry.last_token!=q.entry_token||r.binding!=q.entry_binding||r.token!=q.entry_token||
       !hand::same(r.world,s.world)||!hand::same(r.controller,s.controller)||!hand::same(r.driver,s.driver)||!hand::same(r.turret,s.turret))return false;
    for(auto const* id:{&s.world,&s.controller,&s.driver,&s.turret,&s.weapon,&s.driver_mesh,&s.turret_mesh})if(!troika_original::same_object(*id))return false;
    return troika_original::link(s.turret.address,L"WorldInfo",s.world.address)&&
        troika_original::link(s.turret.address,L"Weapon",s.weapon.address)&&troika_original::link(s.weapon.address,L"Instigator",s.turret.address);
}
bool exact_mount(fire::Run const& run,troika_original::EntryOwner const& entry) noexcept {
    auto const& s=run.source.frame;
    return exact_resource(run,entry)&&troika_original::link(s.controller.address,L"Pawn",s.turret.address)&&
        troika_original::link(s.turret.address,L"Controller",s.controller.address)&&
        troika_original::link(s.turret.address,L"Driver",s.driver.address)&&
        troika_original::link(s.driver.address,L"DrivenVehicle",s.turret.address)&&
        troika_original::link(s.turret.address,L"WorldInfo",s.world.address)&&
        troika_original::link(s.turret.address,L"Weapon",s.weapon.address)&&troika_original::link(s.weapon.address,L"Instigator",s.turret.address);
}
bool buttons_up(meta::Reader& reader,uptr pc) noexcept {
    weaponhand::BoolField held{},raw{},fire{};unsigned char value{};
    if(!weaponhand::boolean(reader,pc,L"bHoldingButton_R2",held)||(held.bits&held.mask)||
       !weaponhand::boolean(reader,pc,L"bRawHoldingButton_R2",raw)||(raw.bits&raw.mask)||
       !weaponhand::field(reader,pc,L"bFire",L"ByteProperty",1,fire)||!reader.at(pc,fire.property.offset,value)||value)return false;
    for(auto const& field:{held,raw,fire})if(!troika_reflection::retain(reader,field.property))return false;
    // Keep the initial field binding across quiescence's original getter calls.
    // A moved field or a late-held R2 must not become a false stop receipt.
    for(auto const& field:{held,raw}){unsigned mask{},bits{};
        if(!reader.at(reader.result.identities[field.property.identity].address,0xd0,mask)||mask!=field.mask||
           !reader.at(pc,field.property.offset,bits)||(bits&field.mask))return false;
    }
    return reader.at(pc,fire.property.offset,value)&&!value&&unified_weapon_fire::retained_metadata(reader);
}
template<class Call,std::size_t N>
bool query_layout(meta::Reader& reader,Call& call,wchar_t const (&kind)[N],unsigned bytes,bool fire_mode=false) noexcept {
    meta::Property result{},input{};std::uint16_t parameters{},return_offset{};
    if(!call.valid||call.r.size<bytes+(fire_mode?4u:0u)||call.r.size>sizeof(call.r.params)||
       !reader.at(call.r.function,0x106,parameters)||parameters!=call.r.size||
       !reader.at(call.r.function,0x108,return_offset)||
       !reader.property(call.prop(L"ReturnValue"),result)||
       reader.result.identities[result.identity].outer!=call.r.function||
       !weaponhand::core_type(reader,result.type,kind)||result.dimension!=1||result.element_bytes!=bytes||
       (result.flags&0x580)!=0x480||result.offset!=return_offset||result.offset>parameters||bytes>parameters-result.offset||
       !troika_reflection::retain(reader,result))return false;
    if(fire_mode){
        if(!reader.property(call.prop(L"FireMode"),input)||reader.result.identities[input.identity].outer!=call.r.function||
           !weaponhand::core_type(reader,input.type,L"IntProperty")||input.dimension!=1||input.element_bytes!=4||
           (input.flags&0x580)!=0x80||input.offset>parameters||4>parameters-input.offset||
           !(input.offset+4<=result.offset||result.offset+bytes<=input.offset)||
           !troika_reflection::retain(reader,input))return false;
    }
    // Admit actual parameter padding, but never an unbound extra parameter.
    meta::Structure shape{};if(!reader.structure(call.r.function,shape)||parameters>shape.extent)return false;
    auto address=shape.children;std::array<uptr,64> seen{};unsigned count{},arguments{};
    while(address){
        if(count==seen.size())return false;
        for(unsigned i=0;i<count;++i)if(seen[i]==address)return false;
        seen[count++]=address;meta::Property field{};
        if(!reader.property(address,field)||reader.result.identities[field.identity].outer!=call.r.function||
           !troika_reflection::retain(reader,field))return false;
        if(field.flags&0x80){
            ++arguments;if(field.identity!=result.identity&&(!fire_mode||field.identity!=input.identity))return false;
        }
        address=field.next;
    }
    if(arguments!=(fire_mode?2u:1u))return false;
    return unified_weapon_fire::retained_metadata(reader);
}
fire::Dispatch invoke_button(playground::Args* call) {
    // This is our own admitted button request, not the game's caller wrapper.
    // Native's sole CF owner still invokes original and preserves its outcome.
    // A fault observed by this requester leaves its R2 responsibility unknown.
    __try{return call->run()?fire::Dispatch::returned:fire::Dispatch::uncertain;}
    __except(GetExceptionCode()==0xe06d7363ul||GetExceptionCode()==EXCEPTION_STACK_OVERFLOW?
        EXCEPTION_CONTINUE_SEARCH:EXCEPTION_EXECUTE_HANDLER){return fire::Dispatch::uncertain;}
}
struct Backend final:fire::Backend {
    troika_original::EntryOwner& entry;QualifiedAimAndHands readiness{};
    bool qualification_checked{},qualification_accepted{};
    Backend(troika_original::EntryOwner& value,QualifiedAimAndHands proof) noexcept:entry(value),readiness(proof){}
    bool qualified(fire::Run const& run) noexcept override {
        qualification_checked=true;qualification_accepted=false;
        if(!readiness||!readiness(run)||stop_requested.load(std::memory_order_acquire)||
           !enabled.load(std::memory_order_acquire)||!exact_mount(run,entry)||!troika_tracking::source_current(run.source.frame))return false;
        std::array<unsigned,3> input_indices{};if(!names(input_indices))return false; // Do not consume R2 while its original names are still being resolved.
        meta::Snapshot schema{};schema.module_base=base;meta::Reader reader{schema,meta::Stage::final_check};
        playground::Args state(run.source.frame.controller.address,L"GetStateName");
        if(!meta::tables(base,schema.objects,schema.names)||!function(reader,state,L"GetStateName",L"Object",L"Core")||
           state.r.size!=sizeof(Name)||!vrequip::result_property(state,L"NameProperty",sizeof(Name))||!state.run())return false;
        const auto current_state=state.output<Name>();
        meta::NameText state_text{};
        if(current_state.number||!meta::fresh_name(schema.names,current_state.index,state_text)||
           std::wcscmp(state_text.units.data(),L"PlayerTurreting")||!unified_weapon_fire::retained_metadata(reader))return false;
        gears_troika_control::Decision actual{};troika_mounted_context::Context gameplay{};
        return qualification_accepted=troika_tracking::control_receipt(entry,run.source.frame,actual)&&!std::memcmp(&actual,&run.control,sizeof actual)&&
            troika_mounted_context::observe(entry,gameplay)&&mounted_geometry::input_allowed(gameplay)&&
            readiness(run)&&exact_mount(run,entry);
    }
    fire::Dispatch button(fire::Run const& run,bool press) noexcept {
        bool dispatched=false;
        try{
            if(!exact_mount(run,entry)||!nativeequipment::restoration_quiet()||(press&&!qualified(run)))return fire::Dispatch::not_dispatched;
            std::array<unsigned,3> indices{};if(!names(indices))return fire::Dispatch::not_dispatched;
            meta::Snapshot schema{};schema.module_base=base;meta::Reader reader{schema,meta::Stage::final_check};
            playground::Args call(run.source.frame.controller.address,press?L"ButtonPress":L"ButtonRelease");
            if(!meta::tables(base,schema.objects,schema.names)||
               !(press?function(reader,call,L"ButtonPress",L"WarPC",L"WarfareGame"):function(reader,call,L"ButtonRelease",L"WarPC",L"WarfareGame"))||
               call.r.size!=sizeof(Name)||!hammerburst_animation::input_field(call,call.prop(L"ButtonName"),L"NameProperty",sizeof(Name))||
               !call.set(L"ButtonName",Name{indices[0],0}).valid||!exact_mount(run,entry)||
               (press&&(!buttons_up(reader,run.source.frame.controller.address)||!qualified(run)))||
               !unified_weapon_fire::retained_metadata(reader))return fire::Dispatch::not_dispatched;
            // Owner already recorded this exact run before reaching Args::run.
            dispatched=true;return invoke_button(&call);
        }catch(...){return dispatched?fire::Dispatch::uncertain:fire::Dispatch::not_dispatched;}
    }
    fire::Dispatch start(fire::Run const& run) noexcept override {return button(run,true);}
    fire::Dispatch stop(fire::Run const& run) noexcept override {return button(run,false);}
    bool quiescent(fire::Run const& run) noexcept override {
        try{
            if(!exact_resource(run,entry)||!nativeequipment::restoration_quiet())return false;
            std::array<unsigned,3> indices{};if(!names(indices))return false;
            meta::Snapshot schema{};schema.module_base=base;meta::Reader reader{schema,meta::Stage::final_check};
            playground::Args state(run.source.frame.weapon.address,L"GetStateName"),pending_call(run.source.frame.weapon.address,L"PendingFire"),firing(run.source.frame.weapon.address,L"IsFiring");
            if(!meta::tables(base,schema.objects,schema.names)||!buttons_up(reader,run.source.frame.controller.address)||
               !function(reader,state,L"GetStateName",L"Object",L"Core")||state.r.size!=sizeof(Name)||
               !query_layout(reader,state,L"NameProperty",sizeof(Name))||
               !function(reader,pending_call,L"PendingFire",L"Weapon",L"Engine")||
               !function(reader,firing,L"IsFiring",L"Weapon",L"Engine")||!query_layout(reader,firing,L"BoolProperty",4)||
               !query_layout(reader,pending_call,L"BoolProperty",4,true)||!pending_call.set(L"FireMode",0).valid||
               !unified_weapon_fire::retained_metadata(reader)||!exact_resource(run,entry)||!state.run())return false;
            const auto value=state.output<Name>();
            if(value.number||(value.index!=indices[1]&&value.index!=indices[2])||!exact_resource(run,entry)||
               !pending_call.run()||pending_call.output<unsigned>()||!exact_resource(run,entry)||!firing.run()||firing.output<unsigned>()||
               !exact_resource(run,entry)||!buttons_up(reader,run.source.frame.controller.address)||
               !state.run())return false;
            const auto closing=state.output<Name>();
            return closing.index==value.index&&closing.number==value.number&&exact_resource(run,entry)&&
                buttons_up(reader,run.source.frame.controller.address)&&query_layout(reader,state,L"NameProperty",sizeof(Name))&&
                query_layout(reader,pending_call,L"BoolProperty",4,true)&&query_layout(reader,firing,L"BoolProperty",4)&&
                unified_weapon_fire::retained_metadata(reader);
        }catch(...){return false;}
    }
    bool extinct(fire::Run const&) noexcept override {return false;} // No missing-read/new-occupant retirement shortcut.
};
bool owed() noexcept {return responsibility.owed();}
unsigned fire_log_rows{};std::uint64_t logged_edge{};bool fire_logged{},logged_qualified{},logged_owed{};
void service(bool stop) noexcept {
    if(!owed())return;Backend backend{owner,mounted_aim_candidates::qualified};if(stop)responsibility.request_stop();responsibility.service(backend);
}
void observe(gears_troika_source::Frame const& source) noexcept {
    gears_troika_control::Decision decision{};
    if(!troika_tracking::control_receipt(owner,source,decision)){service(true);return;}
    gears_troika_geometry::Source q{};q.frame=source;q.entry_binding=owner.receipt.binding;q.entry_token=owner.receipt.token;
    if(!fire_cursor.take(source,weaponhand::clock_ns(nullptr),source.stereo.source==gears_vr::StereoSource::MetaOperatorSimulated)){
        if(owed())service(!mounted_aim_candidates::hold_current(fire::Run{q,decision,0}));return;
    }
    std::array<unsigned,3> prepared_names{};names(prepared_names); // Bounded read-only preparation before a press, without renewing its original window.
    // Current original gun/muzzle observation AND same-source Native C0
    // readback, before advancing the next requested direction.
    Backend backend{owner,mounted_aim_candidates::qualified};const bool began=responsibility.offer(q,decision,weaponhand::clock_ns(nullptr),backend);
    if(log_file&&fire_log_rows<16&&(!fire_logged||logged_edge!=decision.fire_edge_sequence||logged_qualified!=backend.qualification_accepted||logged_owed!=owed())){
        ++fire_log_rows;fire_logged=true;logged_edge=decision.fire_edge_sequence;logged_qualified=backend.qualification_accepted;logged_owed=owed();
        fwprintf(log_file,L"troika_original_fire source=%llu edge=%llu requested=%u qualification_checked=%u qualified=%u began=%u owed=%u serial=%llu start_result=%u stop_result=%u\n",
            source.publication_sequence,decision.fire_edge_sequence,decision.fire_requested,unsigned(backend.qualification_checked),unsigned(backend.qualification_accepted),
            unsigned(began),unsigned(owed()),responsibility.run().serial,unsigned(responsibility.start_result()),unsigned(responsibility.stop_result()));fflush(log_file);
    }
}
} // namespace mounted_fire
