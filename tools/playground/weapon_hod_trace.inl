// HOD query and UpdateFX use the SAME authenticated weapon ray. This is a
// participant of the existing Native CallFunction owner, never another hook.
// Original hit, collision, paint, cooldown and BeamManager remain authoritative.
// Held-HOD indoor use permits only the original Tick's sky eligibility result.
namespace hod_trace {
namespace fire=unified_weapon_fire;namespace hand=gears_weapon_hand;
namespace meta=gripdamage_metadata;namespace callabi=gears_weapon_call;
struct Root {
    hand::ObjectIdentity function{};std::uint64_t fingerprint{};
    uptr script{};unsigned bytes{};
};
struct Scope {callabi::Invocation call{};Root root{};fire::MuzzleSample sample{};
    bool active{},query{},origin_committed{},aim_committed{},tick{};};
struct Pending {callabi::Invocation call{};fire::ReturnSignature signature{};std::array<unsigned,3> desired{};bool active{},uncertain{};};
struct Eligibility {
    callabi::Invocation call{};hand::ObjectIdentity function{},result_property{};
    std::uint64_t fingerprint{};bool active{},uncertain{};Scope scope{};
};
thread_local fire::MuzzleSample prepared;
thread_local std::array<Root,3> roots;
thread_local std::array<Scope,8> scopes;
thread_local std::array<Pending,16> pending;
thread_local std::array<Eligibility,8> eligibility;
thread_local unsigned depth{};
std::atomic<unsigned> active_scopes{};
thread_local std::uint64_t origin_writes{},aim_writes{},query_returns{},fx_returns{};
// Observation only: both returns must commit in this same original body,
// against its frozen sample, and still be current when that body returns.
thread_local std::uint64_t query_ray_commits{},fx_ray_commits{};
thread_local std::uint64_t eligibility_seen{},eligibility_matched{},eligibility_substituted{};
thread_local bool faulted{};
// Bounded observation of one already-journaled R2 use. This never grants or
// withdraws input/action authority and never renews a source timestamp.
struct UseCounters {
    std::uint64_t query_returns{},query_ray_commits{},fx_returns{},
        fx_origin_bodies{},fx_aim_bodies{},fx_paired_bodies{};
};
struct UseObservation {
    hand::Frame source{};std::uint64_t hold{},epoch{},serial{};UseCounters counters{};
};
thread_local UseObservation use_observation;
void begin_use_observation(fire::MuzzleSample const& sample,std::uint64_t serial) noexcept {
    if(use_observation.serial||!serial||!sample.valid||!sample.hold_generation)return;
    use_observation={sample.source,sample.hold_generation,sample.epoch,serial,{}};
}
void end_use_observation(std::uint64_t serial) noexcept {
    if(serial&&use_observation.serial==serial)use_observation={};
}
bool use_counters(std::uint64_t serial,UseCounters& out) noexcept {
    out={};if(!serial||serial!=use_observation.serial)return false;
    out=use_observation.counters;return true;
}
bool observed_use(fire::MuzzleSample const& sample) noexcept {
    const auto& a=use_observation.source;const auto& b=sample.source;
    return use_observation.serial&&use_observation.hold==sample.hold_generation&&use_observation.epoch==sample.epoch&&
        a.key.game_start==b.key.game_start&&a.key.game_pid==b.key.game_pid&&
        a.key.native_started_qpc==b.key.native_started_qpc&&a.key.registration_cookie==b.key.registration_cookie&&
        a.key.lifecycle==b.key.lifecycle&&a.key.world_generation==b.key.world_generation&&a.key.thread_id==b.key.thread_id&&
        hand::same(a.world,b.world)&&hand::same(a.controller,b.controller)&&hand::same(a.pawn,b.pawn)&&
        hand::same(a.weapon,b.weapon)&&hand::same(a.mesh,b.mesh)&&a.reference_generation==b.reference_generation&&
        a.controllers.session==b.controllers.session&&a.controllers.space_epoch==b.controllers.space_epoch&&
        a.controllers.input_epoch==b.controllers.input_epoch&&a.stereo.session==b.stereo.session&&
        a.stereo.space_epoch==b.stereo.space_epoch;
}
bool root_binding(uptr fn,Root& out) noexcept {
    out={};meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::function_path};
    unsigned function{},kind{},owner{},owner_kind{},package{};meta::Table script{};
    unsigned flags{};std::uint16_t native{};uptr exec{};std::uint64_t hash{};
    if(!meta::tables(base,schema.objects,schema.names)||!r.path(fn,function)||
       !(r.named(function,L"GetTraceImpact")||r.named(function,L"UpdateFX")||r.named(function,L"Tick"))||
       !r.path(schema.identities[function].klass,kind)||!weaponhand::core_type(r,kind,L"Function")||
       !r.path(schema.identities[function].outer,owner)||!r.named(owner,L"WEAP_HammerOfDawn")||
       !r.path(schema.identities[owner].klass,owner_kind)||!weaponhand::core_type(r,owner_kind,L"Class")||
       !r.path(schema.identities[owner].outer,package)||!r.named(package,L"warfaregamecontentweapons")||
       schema.identities[package].outer||!r.at(fn,0x8c,script)||script.count<=0||script.count>script.capacity||
       script.capacity>131072||!r.at(fn,0xf4,flags)||!r.at(fn,0xf8,native)||native||
       !r.at(fn,0x114,exec)||exec!=base+0x1d6ca0||
       !weaponshot::hash_script(script.data,unsigned(script.count),hash)||!fire::retained_metadata(r))return false;
    fire::Fingerprint fingerprint;fire::schema_fingerprint(schema,fingerprint);
    fingerprint.word(flags);fingerprint.word(exec);fingerprint.word(script.data);
    fingerprint.word(unsigned(script.count));fingerprint.word(unsigned(script.capacity));fingerprint.word(hash);
    out={weaponshot::identity(schema.identities[function]),fingerprint.value,script.data,unsigned(script.count)};return true;
}
bool eligibility_signature(callabi::Invocation const& call,hand::ObjectIdentity& fn,
                           hand::ObjectIdentity& result,std::uint64_t& fingerprint) noexcept {
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::function_path};
    std::uint32_t function{},kind{},owner{},owner_kind{},package{};
    if(!call.result||(call.result&3)||call.result>UINTPTR_MAX-12||
       !meta::tables(base,schema.objects,schema.names)||!r.path(call.callee,function)||
       !r.named(function,L"IsAValidHODTarget")||!r.path(schema.identities[function].klass,kind)||
       !weaponhand::core_type(r,kind,L"Function")||!r.path(schema.identities[function].outer,owner)||
       !r.named(owner,L"HammerOfDawnBeamManagerBase")||!r.path(schema.identities[owner].klass,owner_kind)||
       !weaponhand::core_type(r,owner_kind,L"Class")||!r.path(schema.identities[owner].outer,package)||
       !r.named(package,L"WarfareGame")||schema.identities[package].outer)return false;
    meta::Structure body{};std::uint16_t parameter_bytes{},return_offset{};
    if(!r.structure(call.callee,body)||!r.at(call.callee,0x106,parameter_bytes)||parameter_bytes<12||
       parameter_bytes>4096||!r.at(call.callee,0x108,return_offset))return false;
    meta::Property returned{};unsigned vector_parameters{};std::uint32_t result_mask{};
    std::array<uptr,meta::max_fields> visited{};unsigned count{},returns{};
    for(auto child=body.children;child;){
        if(count==visited.size())return false;
        for(unsigned i=0;i<count;++i)if(visited[i]==child)return false;
        visited[count++]=child;meta::Property property{};
        if(!r.property(child,property)||schema.identities[property.identity].outer!=call.callee||
           !meta::property_fits(property,body.extent)||schema.field_count==schema.fields.size())return false;
        schema.fields[schema.field_count++]=property;
        if((property.flags&0x80)&&!(property.flags&0x400)){
            if(property.dimension!=1||property.element_bytes!=12||property.offset+12>parameter_bytes||
               !fire::core_struct(r,property,L"Vector")||++vector_parameters!=1)return false;
        }
        if(property.flags&0x400){
            if(++returns!=1||(property.flags&0x480)!=0x480||property.dimension!=1||property.element_bytes!=4||
               property.offset!=return_offset||!meta::property_fits(property,parameter_bytes)||
               !weaponhand::core_type(r,property.type,L"BoolProperty")||
               !r.at(child,0xd0,result_mask)||result_mask!=1)return false;
            returned=property;
        }
        child=property.next;
    }
    if(vector_parameters!=1||returns!=1||!fire::retained_metadata(r))return false;
    fire::Fingerprint hash{};fire::schema_fingerprint(schema,hash);
    hash.word(parameter_bytes);hash.word(return_offset);hash.word(result_mask);
    fn=weaponshot::identity(schema.identities[function]);
    result=weaponshot::identity(schema.identities[returned.identity]);fingerprint=hash.value;return true;
}
void report_eligibility() noexcept {
    static std::uint64_t seen{},matched{},substituted{};static unsigned reports{};
    if(!log_file)return;
    if(reports<8&&(seen!=eligibility_seen||matched!=eligibility_matched||substituted!=eligibility_substituted)){
        ++reports;
        seen=eligibility_seen;matched=eligibility_matched;substituted=eligibility_substituted;
        fwprintf(log_file,L"hod_eligibility seen=%llu matched=%llu substituted=%llu\n",
            seen,matched,substituted);fflush(log_file);
    }
}
bool same(Root const& a,Root const& b) noexcept {
    return a.fingerprint&&a.fingerprint==b.fingerprint&&hand::same(a.function,b.function)&&a.script==b.script&&a.bytes==b.bytes;
}
void clear() noexcept {prepared={};roots={};}
bool live(fire::MuzzleSample const& sample) noexcept {
    return !faulted&&fire::live_sample(sample,weaponhand::clock_ns(nullptr))&&weaponhand::client.armed();
}
// Only normal GT preparation may invoke a socket getter. The caller supplies
// the existing common mounted gun sample; no hand skin is a measurement input.
bool prepare(fire::MuzzleSample const& sample) noexcept {
    clear();if(depth||!live(sample))return false;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(r,sample.source.weapon)||
       !weaponhand::derives(r,sample.source.weapon.address,L"WEAP_HammerOfDawn",L"warfaregamecontentweapons"))return false;
    std::array<Root,3> entries{};
    const wchar_t* labels[]={L"GetTraceImpact",L"UpdateFX",L"Tick"};
    for(unsigned i=0;i<entries.size();++i){
        playground::Args call(sample.source.weapon.address,labels[i]);
        if(!call.valid||!root_binding(call.r.function,entries[i]))return false;
    }
    if(!fire::preflight_returns(sample)||!live(sample))return false;
    prepared=sample;roots=entries;return true;
}
bool inside(callabi::Invocation const& call,Scope const& scope) noexcept {
    if(!callabi::invocation_valid(call)||call.thread_id!=GetCurrentThreadId()||
       call.hand_cookie!=scope.call.hand_cookie||call.native_started_qpc!=scope.call.native_started_qpc||
       !live(scope.sample))return false;
    Root current{};if(!root_binding(scope.root.function.address,current)||!same(current,scope.root))return false;
    std::array<uptr,gears_ui_call_boundary::frame_limit> visited{};auto frame=uptr(call.caller_frame);
    for(unsigned i=0;frame&&i<visited.size();++i){
        for(unsigned j=0;j<i;++j)if(frame==visited[j])return false;visited[i]=frame;
        gears_ui_call_boundary::Frame f{};if(!gears_ui_call_boundary::frame(weaponshot::read,frame,f))return false;
        if(f.node==scope.root.function.address&&f.object==scope.sample.source.weapon.address)
            return f.previous==scope.call.caller_frame&&gears_weapon_call_trace::script_pc(f.code,scope.root.script,scope.root.bytes);
        if(frame==scope.call.caller_frame)return false;
        frame=f.previous;
    }
    return false;
}
bool eligibility_current(callabi::Invocation const& call,Scope const& scope) noexcept {
    if(!scope.tick||!inside(call,scope)||
       !scope.sample.source.controllers.hands[1].trigger_active||
       scope.sample.source.controllers.hands[1].trigger<=.35f)return false;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    meta::PointerField gri{},manager{};
    return meta::tables(base,schema.objects,schema.names)&&weaponhand::object(r,scope.sample.source.world)&&
        weaponhand::read_pointer(r,scope.sample.source.world.address,L"GRI",gri)&&gri.value&&
        weaponhand::read_pointer(r,gri.value,L"HODBeamManager",manager)&&manager.value==call.self&&
        weaponhand::derives(r,call.self,L"HammerOfDawnBeamManager",L"warfaregamecontentweapons")&&
        fire::retained_metadata(r)&&inside(call,scope);
}
void begin(callabi::Invocation const& call) noexcept {
    if(!callabi::invocation_valid(call)||!weaponhand::current_gt(nullptr)||faulted)return;
    if(call.self==prepared.source.weapon.address&&live(prepared)){
        // Tick is authenticated from the live caller frame for eligibility.
        // It must not hold a trace scope across the next GT preparation.
        for(unsigned root_index=0;root_index<2;++root_index){
            const auto& root=roots[root_index];if(call.callee!=root.function.address)continue;
            if(depth==scopes.size()){faulted=true;return;}
            Root current{};if(!root_binding(call.callee,current)||!same(current,root))return;
            scopes[depth++]={call,root,prepared,true,call.callee==roots[0].function.address,false,false,
                call.callee==roots[2].function.address};++active_scopes;return;
        }
    }
    if(call.self&&live(prepared)&&playground::named(call.callee,L"IsAValidHODTarget")){
        ++eligibility_seen;
        Scope scope{};
        // Actor Tick can enter through ProcessEvent without a CallFunction
        // entry notice. Authenticate its live caller frame directly instead.
        std::array<uptr,gears_ui_call_boundary::frame_limit> visited{};
        auto address=uptr(call.caller_frame);
        for(unsigned i=0;address&&i<visited.size();++i){
            bool repeated=false;for(unsigned j=0;j<i;++j)if(visited[j]==address)repeated=true;
            if(repeated)break;visited[i]=address;
            gears_ui_call_boundary::Frame frame{};
            if(!gears_ui_call_boundary::frame(weaponshot::read,address,frame))break;
            if(frame.node==roots[2].function.address&&frame.object==prepared.source.weapon.address){
                scope={call,roots[2],prepared,true,false,false,false,true};
                scope.call.caller_frame=frame.previous;break;
            }
            address=frame.previous;
        }
        if(eligibility_current(call,scope)){
                hand::ObjectIdentity fn{},result{};std::uint64_t fingerprint{};
                if(eligibility_signature(call,fn,result,fingerprint)&&inside(call,scope)){
                    Eligibility* slot{};
                    for(auto& value:eligibility){if(value.active&&value.call.call_id==call.call_id)return;
                        if(!value.active&&!slot)slot=&value;}
                    if(!slot){faulted=true;return;}
                    *slot={call,fn,result,fingerprint,true,false,scope};++eligibility_matched;
                }
        }
    }
    if(!depth||scopes[depth-1].tick||!call.result||!(playground::named(call.callee,L"GetWeaponStartTraceLocation")||
       playground::named(call.callee,L"GetAdjustedAimFor")))return;
    auto& scope=scopes[depth-1];if(!inside(call,scope))return;
    // A later uncommitted occurrence cannot inherit an earlier return's proof.
    if(playground::named(call.callee,L"GetWeaponStartTraceLocation"))scope.origin_committed=false;
    else scope.aim_committed=false;
    fire::ReturnSignature signature{};if(!fire::typed_result(call,scope.sample,signature))return;
    Pending* slot{};for(auto& p:pending){if(p.active&&p.call.call_id==call.call_id)return;if(!p.active&&!slot)slot=&p;}
    if(!slot)return;
    Pending value{};value.call=call;value.signature=signature;value.active=true;
    if(signature.kind==fire::ResultKind::base_aim)std::memcpy(value.desired.data(),&scope.sample.aim,12);
    else if(signature.kind==fire::ResultKind::trace_origin)std::memcpy(value.desired.data(),&scope.sample.origin,12);
    else return;
    *slot=value;
}
bool returned(callabi::Invocation const& call) noexcept {
    for(auto& value:eligibility)if(value.active&&value.call.call_id==call.call_id){
        if(value.uncertain||!callabi::same(value.call,call)||!eligibility_current(call,value.scope)){
            if(value.uncertain)faulted=true;else value={};return false;
        }
        hand::ObjectIdentity fn{},result{};std::uint64_t fingerprint{};
        if(eligibility_signature(call,fn,result,fingerprint)&&hand::same(fn,value.function)&&
           hand::same(result,value.result_property)&&fingerprint==value.fingerprint&&eligibility_current(call,value.scope)){
            unsigned before{};if(!copy(&before,call.result,4))return false;
            if(before==0){bool exchanged{};
                if(!fire::compare_word(call.result,before,1,exchanged)||!exchanged){
                    value.uncertain=faulted=true;return false;
                }
                if(!eligibility_current(call,value.scope)){bool restored{};
                    if(!fire::compare_word(call.result,1,before,restored)||!restored){value.uncertain=faulted=true;}
                    return false;
                }
                ++eligibility_substituted;
            }
        }
        value={};
    }
    for(auto& p:pending)if(p.active&&p.call.call_id==call.call_id){
        if(p.uncertain||!callabi::same(p.call,call))return false;
        if(depth&&inside(call,scopes[depth-1])){
            auto& scope=scopes[depth-1];fire::ReturnSignature signature{};
            if(fire::typed_result(call,scope.sample,signature)&&fire::same_signature(signature,p.signature)&&inside(call,scope)){
                std::array<unsigned,3> before{};if(!copy(before.data(),call.result,12))return false;
                unsigned changed{};
                for(;changed<3;++changed){bool exchanged{};
                    if(!inside(call,scope)||!fire::compare_word(call.result+4*changed,before[changed],p.desired[changed],exchanged)||!exchanged)break;
                }
                if(changed==3&&inside(call,scope)){
                    if(signature.kind==fire::ResultKind::base_aim){++aim_writes;scope.aim_committed=true;}
                    else {++origin_writes;scope.origin_committed=true;}
                }else{
                    bool restored=true;
                    while(changed){--changed;bool exchanged{};
                        if(!fire::compare_word(call.result+4*changed,p.desired[changed],before[changed],exchanged)||!exchanged)restored=false;
                    }
                    if(!restored){p.uncertain=faulted=true;return false;}
                }
            }
        }
        p={};
    }
    if(depth&&scopes[depth-1].call.call_id==call.call_id){
        if(!callabi::same(scopes[depth-1].call,call))return false;
        const auto& scope=scopes[depth-1];if(scope.tick)return true;Root current{};
        const bool paired=scope.origin_committed&&scope.aim_committed&&live(scope.sample)&&
            root_binding(scope.root.function.address,current)&&same(current,scope.root);
        if(scope.query){++query_returns;if(paired)++query_ray_commits;}
        else {++fx_returns;if(paired)++fx_ray_commits;}
        if(observed_use(scope.sample)){
            auto& counters=use_observation.counters;
            if(scope.query){++counters.query_returns;if(paired)++counters.query_ray_commits;}
            else {++counters.fx_returns;
                if(scope.origin_committed)++counters.fx_origin_bodies;
                if(scope.aim_committed)++counters.fx_aim_bodies;
                if(paired)++counters.fx_paired_bodies;}
        }
    }
    return true;
}
bool cleanup(callabi::Invocation const& call) noexcept {
    bool clean=true;
    for(auto& value:eligibility)if(value.active&&value.call.call_id==call.call_id){
        if(value.uncertain||!callabi::same(value.call,call))clean=false;else value={};
    }
    for(auto& p:pending)if(p.active&&p.call.call_id==call.call_id){
        if(p.uncertain||!callabi::same(p.call,call))clean=false;else p={};
    }
    for(unsigned i=0;i<depth;++i)if(scopes[i].call.call_id==call.call_id){
        if(i+1!=depth||!callabi::same(scopes[i].call,call)){faulted=true;return false;}
        scopes[--depth]={};--active_scopes;break;
    }
    return clean;
}
void service_observation() noexcept {report_eligibility();}
} // namespace hod_trace
