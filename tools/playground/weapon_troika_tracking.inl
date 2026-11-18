// Data-only mounted reader. Reuses the original Native module/client admission
// from handrig; never registers a new owner, selects input or grants C0/fire.
namespace troika_tracking {
namespace api=gears_troika_source;namespace hand=gears_weapon_hand;
api::ReadFn provider{};HMODULE provider_image{};gears_rig_tracking::NativeKey provider_key{};bool attempted{};
unsigned log_rows{},last_status=UINT_MAX;
gears_weapon::TroikaMountedSpace physical_space;
gears_weapon::TroikaMountKey space_owner{};bool have_basis{};unsigned mapping_logs{};
gears_troika_basis::Request native_basis{};bool native_basis_retained{};
struct EntryBasis {gears_weapon::TroikaMountedSpace space;gears_weapon::TroikaMountKey owner{};
    gears_troika_basis::Request native_request{};bool ready{},native_retained{};};
gears_rig_tracking::Reply invoke_basis(gears_troika_basis::Submit fn,gears_troika_basis::Request const* request) noexcept {
    __try{return fn(request);}
    __except(GetExceptionCode()==0xe06d7363ul||GetExceptionCode()==EXCEPTION_STACK_OVERFLOW?
        EXCEPTION_CONTINUE_SEARCH:EXCEPTION_EXECUTE_HANDLER){
        handrig::admission.quarantine();return gears_rig_tracking::Reply::unavailable;
    }
}
bool submit_basis(gears_troika_basis::Request const& request,bool cancel) noexcept {
    if(handrig::reading||!gears_troika_basis::valid(request)||!gears_rig_tracking::same(handrig::key,request.owner.native)||
       !weaponhand::native_module)return false;
    struct Local {handrig::lifetime::Admission::Entry entry{};bool held{};
        ~Local(){if(held){handrig::reading=false;handrig::admission.release(entry);}}} local;
    if(!handrig::admission.acquire(local.entry))return false;local.held=true;handrig::reading=true;
    auto const* lifetime=handrig::admission.view(local.entry);
    const auto current=[&](){return lifetime&&lifetime->context==&weaponhand::client&&
        lifetime->cookie==request.owner.native.registration_cookie&&lifetime->native_started_qpc==request.owner.native.native_started_qpc&&
        weaponhand::client.phase()==hand::ClientPhase::active&&weaponhand::client.cookie()==request.owner.native.registration_cookie&&
        weaponhand::client.native_started_qpc()==request.owner.native.native_started_qpc&&weaponhand::current_gt(nullptr)==request.owner.native.game_thread;};
    if(!current())return false;
    const auto address=GetProcAddress(weaponhand::native_module,cancel?gears_troika_basis::cancel_export:gears_troika_basis::retain_export);
    if(!weaponhand::executable(address,weaponhand::native_module))return false;
    const auto reply=invoke_basis(reinterpret_cast<gears_troika_basis::Submit>(address),&request);
    const bool accepted=reply==gears_rig_tracking::Reply::accepted&&current();
    handrig::reading=false;const bool released=handrig::admission.release(local.entry);local.held=false;return accepted&&released;
}
void reset_provider() noexcept {provider=nullptr;provider_image=nullptr;provider_key={};attempted=false;}
void reset() noexcept {if(native_basis_retained)submit_basis(native_basis,true);
    native_basis_retained=false;native_basis={};reset_provider();physical_space.reset();space_owner={};have_basis=false;}
void interrupt() noexcept {physical_space.interrupt();}
bool prepare_entry_basis(hand::Frame const& frame,gears_rig_tracking::Frame const& raw,
    hand::ObjectIdentity const& turret,hand::ObjectIdentity const& weapon,gears_weapon::TroikaRequest const& request,EntryBasis& output) noexcept {
    output={};const auto stamp=weaponhand::clock_ns(nullptr);
    if(request.kind!=gears_weapon::TroikaAction::mount_original||!request.token||!request.stamp.binding||
       request.stamp.identity.turret!=turret.address||request.stamp.identity.weapon!=weapon.address||
       gears_equipment_rig::match(frame,raw,stamp,unsigned(frame.stereo.source))!=gears_equipment_rig::Status::matched||
       !nativeequipment::current(nullptr,frame))return false;
    output.owner={raw.native,frame.world,frame.controller,frame.pawn,turret,weapon,request.stamp.binding,request.token};
    output.ready=output.space.capture(output.owner,raw,stamp);
    if(output.ready){output.native_request.owner=output.owner;output.native_request.entry_publication=raw.publication_sequence;
        output.native_retained=submit_basis(output.native_request,false);}
    return output.ready;
}
void cancel_prepared_basis(EntryBasis const& prepared) noexcept {if(prepared.native_retained)submit_basis(prepared.native_request,true);}
bool retain_entry_basis(troika_original::EntryOwner const& owner,EntryBasis const& prepared) noexcept {
    const auto& r=owner.receipt;const auto& key=prepared.owner.native;
    if(!prepared.ready||!owner.outstanding||!r.dispatched||r.token!=prepared.owner.token||r.binding!=prepared.owner.binding||
       r.source_key.game_pid!=key.game_pid||r.source_key.game_start!=key.game_start||r.source_key.native_started_qpc!=key.native_started_qpc||
       r.source_key.registration_cookie!=key.registration_cookie||!hand::same(r.world,prepared.owner.world)||
       !hand::same(r.controller,prepared.owner.controller)||!hand::same(r.driver,prepared.owner.driver)||
       !hand::same(r.turret,prepared.owner.turret)||have_basis)return false;
    physical_space=prepared.space;space_owner=prepared.owner;have_basis=true;
    native_basis=prepared.native_request;native_basis_retained=prepared.native_retained;return true;
}
gears_rig_tracking::Reply invoke(api::ReadFn fn,api::Request const* request,api::Frame* output) noexcept {
    __try{return fn(request,output);}
    __except(GetExceptionCode()==0xe06d7363ul||GetExceptionCode()==EXCEPTION_STACK_OVERFLOW?
        EXCEPTION_CONTINUE_SEARCH:EXCEPTION_EXECUTE_HANDLER){
        handrig::admission.quarantine();return gears_rig_tracking::Reply::unavailable;
    }
}
bool same(troika_mounted_context::Context const& a,troika_mounted_context::Context const& b){
    return a.entry_binding==b.entry_binding&&a.entry_token==b.entry_token&&
        hand::same(a.world,b.world)&&hand::same(a.controller,b.controller)&&hand::same(a.driver,b.driver)&&
        hand::same(a.turret,b.turret)&&hand::same(a.driver_mesh,b.driver_mesh)&&hand::same(a.turret_mesh,b.turret_mesh)&&hand::same(a.weapon,b.weapon);
}
bool acquire(troika_original::EntryOwner const& owner,api::Frame& output) noexcept {
    output={};
    if(handrig::reading||!weaponhand::current_gt(nullptr)||!owner.outstanding)return false;
    struct Local {
        handrig::lifetime::Admission::Entry entry{};bool held{};
        ~Local(){if(held){handrig::reading=false;handrig::admission.release(entry);}}
    } local;
    if(!handrig::admission.acquire(local.entry))return false;local.held=true;handrig::reading=true;
    auto const* lifetime=handrig::admission.view(local.entry);const auto key=handrig::key;
    auto const current=[&](){return lifetime&&lifetime->context==&weaponhand::client&&
        lifetime->cookie==key.registration_cookie&&lifetime->native_started_qpc==key.native_started_qpc&&
        weaponhand::client.phase()==hand::ClientPhase::active&&weaponhand::client.cookie()==key.registration_cookie&&
        weaponhand::client.native_started_qpc()==key.native_started_qpc&&weaponhand::current_gt(nullptr)==key.game_thread;};
    const auto& entry=owner.receipt.source_key;
    if(!current()||!weaponhand::native_module||entry.game_pid!=key.game_pid||entry.game_start!=key.game_start||
       entry.native_started_qpc!=key.native_started_qpc||entry.registration_cookie!=key.registration_cookie)return false;
    if(provider_image!=weaponhand::native_module||!gears_rig_tracking::same(provider_key,key)){
        reset_provider();provider_image=weaponhand::native_module;provider_key=key;
    }
    if(!attempted){
        attempted=true;const auto address=GetProcAddress(provider_image,"GearsVrReadTroikaMountedSourceV1");
        if(weaponhand::executable(address,provider_image))provider=reinterpret_cast<api::ReadFn>(address);
    }
    if(!provider)return false;
    troika_mounted_context::Context before{},after{};
    if(!troika_mounted_context::observe(owner,before)||!current())return false;
    api::Request request{};request.native=key;request.world=before.world;request.controller=before.controller;
    request.turret=before.turret;request.driver=before.driver;
    api::Frame candidate{};const auto reply=invoke(provider,&request,&candidate);
    const bool ready=reply==gears_rig_tracking::Reply::accepted&&
        candidate.stereo.source==owner.receipt.source&&
        api::matches(request,candidate,weaponhand::clock_ns(nullptr),owner.receipt.source==gears_vr::StereoSource::MetaOperatorSimulated)&&
        troika_mounted_context::observe(owner,after)&&same(before,after)&&
        hand::same(candidate.driver_mesh,after.driver_mesh)&&hand::same(candidate.turret_mesh,after.turret_mesh)&&
        hand::same(candidate.weapon,after.weapon)&&current();
    if(log_file&&log_rows<12&&last_status!=unsigned(reply)){
        fwprintf(log_file,L"troika_mounted_source reply=%u ready=%u entry=%llu scene=%llu publication=%llu input_sync=%lld raw_data_only=1\n",
            unsigned(reply),unsigned(ready),owner.last_token,candidate.scene_invocation,candidate.publication_sequence,candidate.controllers.sync_ns);
        fflush(log_file);++log_rows;last_status=unsigned(reply);
    }
    handrig::reading=false;const bool released=handrig::admission.release(local.entry);local.held=false;
    if(!ready||!released)return false;
    output=candidate;return true;
}
// Recheck the actual Native ticket without another input selection or another
// PG actor walk. Caller separately retains its original mounted context guard.
bool source_current(api::Frame const& expected) noexcept {
    if(handrig::reading||!provider||!gears_rig_tracking::same(handrig::key,expected.native)||
       provider_image!=weaponhand::native_module||!gears_rig_tracking::same(provider_key,expected.native))return false;
    struct Local {handrig::lifetime::Admission::Entry entry{};bool held{};
        ~Local(){if(held){handrig::reading=false;handrig::admission.release(entry);}}} local;
    if(!handrig::admission.acquire(local.entry))return false;local.held=true;handrig::reading=true;
    auto const* lifetime=handrig::admission.view(local.entry);
    const auto current=[&](){return lifetime&&lifetime->context==&weaponhand::client&&
        lifetime->cookie==expected.native.registration_cookie&&lifetime->native_started_qpc==expected.native.native_started_qpc&&
        weaponhand::client.phase()==hand::ClientPhase::active&&weaponhand::client.cookie()==expected.native.registration_cookie&&
        weaponhand::client.native_started_qpc()==expected.native.native_started_qpc&&weaponhand::current_gt(nullptr)==expected.native.game_thread;};
    if(!current())return false;
    api::Request request{};request.native=expected.native;request.world=expected.world;request.controller=expected.controller;
    request.turret=expected.turret;request.driver=expected.driver;request.minimum_publication_sequence=expected.publication_sequence;
    api::Frame candidate{};const auto reply=invoke(provider,&request,&candidate);
    const bool valid=reply==gears_rig_tracking::Reply::accepted&&candidate.publication_sequence==expected.publication_sequence&&
        candidate.source_ticket==expected.source_ticket&&candidate.native_reference_revision==expected.native_reference_revision&&
        candidate.stereo.source==expected.stereo.source&&
        api::matches(request,candidate,weaponhand::clock_ns(nullptr),expected.stereo.source==gears_vr::StereoSource::MetaOperatorSimulated)&&current();
    handrig::reading=false;const bool released=handrig::admission.release(local.entry);local.held=false;
    return valid&&released;
}
bool map(troika_original::EntryOwner const& owner,api::Frame const& source,gears_weapon::TroikaMountedMapping& output) noexcept {
    output={};const auto& receipt=owner.receipt;
    if(!have_basis||!owner.outstanding||receipt.result!=troika_original::EntryResult::mounted||
       receipt.token!=space_owner.token||receipt.binding!=space_owner.binding||
       !hand::same(receipt.world,space_owner.world)||!hand::same(receipt.controller,space_owner.controller)||
       !hand::same(receipt.driver,space_owner.driver)||!hand::same(receipt.turret,space_owner.turret)){
        interrupt();return false;
    }
    const bool ready=physical_space.map(space_owner,source,weaponhand::clock_ns(nullptr),output);
    if(log_file&&mapping_logs<8&&ready&&output.new_publication){
        fwprintf(log_file,L"troika_fixed_basis entry=%llu scene=%llu input_sync=%lld tracked=%u continuity_boundary=%u data_only=1\n",
            owner.last_token,source.scene_invocation,output.hands.input_sync_ns,output.hands.tracked_mask,unsigned(output.continuity_changed));
        fflush(log_file);++mapping_logs;
    }
    return ready;
}
} // namespace troika_tracking
namespace troika_tracking {
gears_rig_tracking::Reply invoke_control(gears_troika_control::Read fn,gears_troika_geometry::Source const* source,
    gears_troika_control::Decision* output) noexcept {
    __try{return fn(source,output);}
    __except(GetExceptionCode()==0xe06d7363ul||GetExceptionCode()==EXCEPTION_STACK_OVERFLOW?
        EXCEPTION_CONTINUE_SEARCH:EXCEPTION_EXECUTE_HANDLER){handrig::admission.quarantine();return gears_rig_tracking::Reply::unavailable;}
}
bool control_receipt(troika_original::EntryOwner const& owner,api::Frame const& source,gears_troika_control::Decision& output) noexcept {
    output={};
    if(handrig::reading||!owner.outstanding||owner.receipt.result!=troika_original::EntryResult::mounted||
       !weaponhand::native_module||!gears_rig_tracking::same(handrig::key,source.native)||!source_current(source))return false;
    handrig::lifetime::Admission::Entry entry{};if(!handrig::admission.acquire(entry))return false;
    struct Local {handrig::lifetime::Admission::Entry& entry;bool held{true};
        ~Local(){if(held){handrig::reading=false;handrig::admission.release(entry);}}} local{entry};
    handrig::reading=true;const auto* lifetime=handrig::admission.view(entry);
    const auto current=[&](){return lifetime&&lifetime->context==&weaponhand::client&&lifetime->cookie==source.native.registration_cookie&&
        lifetime->native_started_qpc==source.native.native_started_qpc&&weaponhand::client.phase()==hand::ClientPhase::active&&
        weaponhand::client.cookie()==source.native.registration_cookie&&weaponhand::client.native_started_qpc()==source.native.native_started_qpc&&
        weaponhand::current_gt(nullptr)==source.native.game_thread;};
    const auto address=GetProcAddress(weaponhand::native_module,gears_troika_control::read_export);
    if(!current()||!weaponhand::executable(address,weaponhand::native_module))return false;
    gears_troika_geometry::Source q{};q.frame=source;q.entry_binding=owner.receipt.binding;q.entry_token=owner.receipt.token;
    gears_troika_control::Decision value{};const auto reply=invoke_control(reinterpret_cast<gears_troika_control::Read>(address),&q,&value);
    gears_troika_geometry::Request check{};check.query_id=value.query_id;check.source=q;
    const bool okay=reply==gears_rig_tracking::Reply::accepted&&current()&&gears_troika_control::valid(value,check,weaponhand::clock_ns(nullptr));
    handrig::reading=false;const bool released=handrig::admission.release(entry);local.held=false;
    if(!okay||!released||!source_current(source))return false;output=value;return true;
}
unsigned control_log_rows{};hand::Lease previous_control_lease{};
void observe_control(troika_original::EntryOwner const& owner,api::Frame const& source) noexcept {
    gears_troika_control::Decision receipt{};if(!control_receipt(owner,source,receipt))return;
    if(log_file&&control_log_rows<12&&(!control_log_rows||!hand::same(previous_control_lease,receipt.lease))){
        fwprintf(log_file,L"troika_mounted_left query=%llu publication=%llu source_ticket=%llu lifecycle=%llu press=%llu status=%u lease=%u,%llu new_grant=%u original_sync=%lld ownership_only_no_aim_fire_C0=1\n",
            receipt.query_id,receipt.publication_sequence,receipt.source_ticket,receipt.lifecycle,receipt.press_token,unsigned(receipt.status),
            unsigned(receipt.lease.kind),receipt.lease.id,receipt.new_grant,source.controllers.sync_ns);
        fflush(log_file);++control_log_rows;previous_control_lease=receipt.lease;
    }
}
} // namespace troika_tracking
