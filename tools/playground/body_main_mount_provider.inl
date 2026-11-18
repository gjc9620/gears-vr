// Distinct typed child of the existing parent. Parent counters, image pin,
// reentry exclusion and drain remain shared with the original V1 child.
namespace bodymountprovider {
namespace wire=gears_body_main_mount_provider;
namespace sender=gears_body_main_mount_sender;
namespace hand=gears_weapon_hand;
struct Workspace {
    wire::Request request{};wire::Response response{};
    hand::EquipmentIdentity identity{};hand::EquipmentObservation observation{};
    weapon_metadata_parse::Workspace parsing{};
};
Workspace scratch;

// Only immutable calibration and its identity cohort are retained here.
struct BoomshotCalibration {
    hand::EquipmentIdentity actor{};
    std::uint64_t hold{},binding{},reference_epoch{},session{},space{},input{},profile{};
    double units{},scale{};
};
BoomshotCalibration boomshot_calibration{};
bool boomshot_model_scale(hand::Frame const& frame,gears_rig_tracking::Frame const& rig,double& scale) noexcept {
    const auto& c=boomshot_calibration;
    const auto& key=c.actor.key;
    if(!c.hold||c.hold!=vrequip::model.hold_generation()||
       key.game_start!=frame.key.game_start||key.native_started_qpc!=frame.key.native_started_qpc||
       key.registration_cookie!=frame.key.registration_cookie||key.lifecycle!=frame.key.lifecycle||
       key.world_generation!=frame.key.world_generation||
       !hand::same(c.actor.world,frame.world)||!hand::same(c.actor.controller,frame.controller)||
       !hand::same(c.actor.pawn,frame.pawn)||!hand::same(c.actor.mesh,frame.mesh)||
       !hand::same(c.actor.weapon,frame.weapon)||!bodymountequipment::held_current(c.actor,c.hold)||
       c.actor.reference_generation!=rig.reference_generation||c.binding!=rig.binding_generation||
       c.reference_epoch!=rig.native_reference.epoch||c.session!=rig.stereo.session||
       c.space!=rig.stereo.space_epoch||c.input!=rig.controllers.input_epoch||
       c.profile!=rig.controllers.profile_generation||c.units!=rig.units_per_metre)return false;
    scale=c.scale;
    return true;
}
const wire::Response empty_response{};
static_assert(std::is_trivially_copyable_v<Workspace>);
struct Receipt {
    std::uint64_t query{},cookie{},scene{},publication{},tick{},compose{};
    std::int64_t begin{},finish{},sync{},equipment_begin{},equipment_end{};
    unsigned stage{},status{UINT32_MAX},returned{};
};
Receipt active_receipt{};std::array<Receipt,32> receipts{};unsigned receipt_count{};
// Failure-only Boltok metadata ring; no extra reads, permissions or writes.
std::array<Receipt,32> boltok_failures{};unsigned boltok_failure_count{},boltok_failure_next{};
std::uint64_t receipt_attempts{};bool diagnostics_written{};
void reset_diagnostics_after_drain() noexcept {
    if(!mainmountprovider::diagnostics_drained())return;
    active_receipt={};receipts={};receipt_count=0;receipt_attempts=0;diagnostics_written=false;
    boltok_failures={};boltok_failure_count=boltok_failure_next=0;
    bodymountequipment::asset_results={};
}
void write_diagnostics_after_drain() noexcept {
    if(!mainmountprovider::diagnostics_drained()||!log_file||diagnostics_written)return;
    diagnostics_written=true;
    for(unsigned i=0;i<receipt_count;++i){const auto& r=receipts[i];
        fwprintf(log_file,L"native_body_mount_query query=%llu cookie=%llu scene=%llu publication=%llu tick=%llu compose=%llu begin_ns=%lld finish_ns=%lld sync_ns=%lld equipment_begin_ns=%lld equipment_end_ns=%lld stage=%u status=%u returned=%u\n",
            r.query,r.cookie,r.scene,r.publication,r.tick,r.compose,r.begin,r.finish,r.sync,r.equipment_begin,r.equipment_end,r.stage,r.status,r.returned);
    }
    for(unsigned i=0;i<boltok_failure_count;++i){
        const unsigned slot=(boltok_failure_next+unsigned(boltok_failures.size())-boltok_failure_count+i)%unsigned(boltok_failures.size());
        const auto& r=boltok_failures[slot];
        fwprintf(log_file,L"native_body_mount_boltok_failure query=%llu cookie=%llu scene=%llu publication=%llu tick=%llu compose=%llu begin_ns=%lld finish_ns=%lld sync_ns=%lld equipment_begin_ns=%lld equipment_end_ns=%lld stage=%u status=%u returned=%u\n",
            r.query,r.cookie,r.scene,r.publication,r.tick,r.compose,r.begin,r.finish,r.sync,r.equipment_begin,r.equipment_end,r.stage,r.status,r.returned);
    }
    fwprintf(log_file,L"native_body_mount_query_summary attempts=%llu retained=%u truncated=%u\n",receipt_attempts,receipt_count,unsigned(receipt_attempts>receipts.size()));
    fwprintf(log_file,L"native_body_mount_asset_summary");
    for(unsigned i=0;i<bodymountequipment::asset_results.size();++i)
        fwprintf(log_file,L" status_%u=%llu",i,bodymountequipment::asset_results[i]);
    fwprintf(log_file,L"\n");
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_L2_TRIAL)
    const auto& d=bodymountequipment::unheld_witness;
    fwprintf(log_file,L"native_body_mount_unheld queries=%llu generation=%llu model_held=%llu binding_known=%llu binding_generation=%llu binding_world=%llu binding_controller=%llu binding_pawn=%llu binding_weapon=%llu binding_native=%llu first_scene=%llu first_weapon=%llx first_binding_weapon=%llx\n",
        d.queries,d.generation,d.model_held,d.binding_known,d.binding_generation,d.binding_world,
        d.binding_controller,d.binding_pawn,d.binding_weapon,d.binding_native,
        d.first_after_generation_scene,d.first_after_generation_weapon,d.first_after_generation_binding_weapon);
#endif
    fflush(log_file);
}
bool publish(wire::Response* out,wire::Response const& value) noexcept {
    __try{if(!out)return false;std::memcpy(out,&value,sizeof(value));return true;}
    __except(GetExceptionCode()==0xe06d7363ul||GetExceptionCode()==EXCEPTION_STACK_OVERFLOW?
        EXCEPTION_CONTINUE_SEARCH:EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool running(sender::Parent const& parent,std::uint64_t generation) noexcept {
    return enabled.load(std::memory_order_acquire)&&weaponhand::current_gt(nullptr)==parent.game_thread&&
        nativeequipment::domain_ready()&&weaponhand::client.registration_alive(parent.cookie,parent.native_started_qpc)&&
        generation&&weaponhand::client.metadata_generation_on_gt()==generation;
}
std::uint32_t prepare_data(wire::Request const* request) noexcept {
    active_receipt.stage=1;std::uint32_t header[2]{};
    if(!request||!copy(header,reinterpret_cast<uptr>(request),sizeof(header)))return ~0u;
    if(header[0]!=sizeof(wire::Request)||header[1]!=wire::version)return unsigned(wire::Status::source_mismatch);
    if(!copy(&scratch.request,reinterpret_cast<uptr>(request),sizeof(scratch.request)))return ~0u;
    const auto& r=scratch.request;
    active_receipt.query=r.query_id;active_receipt.cookie=r.source.tracking.parent_cookie;
    active_receipt.scene=r.source.original_scene.scene_invocation;active_receipt.publication=r.source.original_scene.publication_sequence;
    active_receipt.tick=r.source.tracking.tick;active_receipt.compose=r.source.basis.compose;active_receipt.sync=r.source.tracking.controllers.sync_ns;
    const sender::Parent parent{weaponhand::client.cookie(),weaponhand::client.native_started_qpc(),
        GetCurrentProcessId(),weaponhand::current_gt(nullptr)};
    const auto generation=weaponhand::client.metadata_generation_on_gt();
    if(!running(parent,generation))return unsigned(wire::Status::inactive);
    active_receipt.stage=2;
    auto status=sender::context(r,parent,weaponhand::clock_ns(nullptr),scratch.identity);
    if(status!=wire::Status::ready)return unsigned(status);
    active_receipt.stage=3;
    status=bodymountequipment::observe(r,parent,generation,scratch.identity,scratch.observation);
    active_receipt.equipment_begin=scratch.observation.begin_ns;active_receipt.equipment_end=scratch.observation.end_ns;
    if(status!=wire::Status::ready)return unsigned(status);
    if(!running(parent,generation)||!bodymountequipment::held_current(scratch.identity,scratch.observation.hold_generation))
        return unsigned(wire::Status::inactive);
    active_receipt.stage=4;
    status=sender::response(r,parent,scratch.identity,scratch.observation,weaponhand::clock_ns(nullptr),scratch.response);
    if(!running(parent,generation)||!bodymountequipment::held_current(scratch.identity,scratch.observation.hold_generation)){
        scratch.response=empty_response;return unsigned(wire::Status::inactive);
    }
    return unsigned(status);
}
std::uint32_t __cdecl prepare(void* context,wire::Request const* request,wire::Response* out) noexcept {
    callbacks.fetch_add(1,std::memory_order_acq_rel);
    bool claimed{},parse_installed{};weapon_metadata_parse::Workspace* previous_parse{};
    std::uint32_t result=unsigned(wire::Status::inactive);
    __try{
        if(!publish(out,empty_response))return ~0u;
        if(context!=&weaponhand::client||!weaponhand::current_gt(nullptr)||
           !enabled.load(std::memory_order_acquire)||!nativeequipment::domain_ready())return result;
        if(mainmountprovider::entered.test_and_set(std::memory_order_acquire))return ~0u;
        claimed=true;active_receipt={};active_receipt.begin=weaponhand::clock_ns(nullptr);
        if(receipt_attempts!=UINT64_MAX)++receipt_attempts;
        previous_parse=weapon_metadata_parse::activate(scratch.parsing);parse_installed=true;
        result=prepare_data(request);
        if(wire::mount::status_valid(result)&&result==unsigned(wire::Status::ready)){
            active_receipt.stage=5;
            if(!publish(out,scratch.response))result=~0u;
            else if(scratch.response.hold_kind==wire::HoldKind::boomshot){
                const auto& source=scratch.request.source;
                const auto& tracking=source.tracking;
                const auto& geometry=source.basis.geometry;
                boomshot_calibration={scratch.identity,scratch.observation.hold_generation,
                    geometry.revision,tracking.native_reference.epoch,tracking.stereo.session,
                    tracking.stereo.space_epoch,tracking.controllers.input_epoch,
                    tracking.controllers.profile_generation,geometry.units_per_metre,geometry.model_scale};
            }
        }
        active_receipt.status=result;active_receipt.returned=1;active_receipt.finish=weaponhand::clock_ns(nullptr);
        return result;
    }__finally{
        if(claimed){
            if(parse_installed)weapon_metadata_parse::restore(previous_parse);
            if(receipt_count<receipts.size())receipts[receipt_count++]=active_receipt;
            if(nativelancerfollow::bound_key_known&&nativelancerfollow::bound_is_boltok&&
               hand::same(scratch.identity.weapon,nativelancerfollow::bound_key.weapon)&&
               (!active_receipt.returned||active_receipt.status!=unsigned(wire::Status::ready))){
                boltok_failures[boltok_failure_next]=active_receipt;
                boltok_failure_next=(boltok_failure_next+1)%unsigned(boltok_failures.size());
                if(boltok_failure_count<boltok_failures.size())++boltok_failure_count;
            }
            std::memset(&scratch,0,sizeof(scratch));mainmountprovider::entered.clear(std::memory_order_release);
        }
        callbacks.fetch_sub(1,std::memory_order_release);
    }
}
bool call_attach(wire::Attach fn,wire::Registration const& registration,hand::RegistrationResult& receipt,hand::Reply& reply) noexcept {
    __try{reply=fn(&registration,&receipt);return true;}
    __except(GetExceptionCode()==0xe06d7363ul||GetExceptionCode()==EXCEPTION_STACK_OVERFLOW?
        EXCEPTION_CONTINUE_SEARCH:EXCEPTION_EXECUTE_HANDLER){return false;}
}
hand::Reply attach(HMODULE native) noexcept {
    if(weaponhand::current_gt(nullptr)||!enabled.load(std::memory_order_acquire)||!native||native!=weaponhand::native_module)
        return hand::Reply::unavailable;
    const auto address=GetProcAddress(native,wire::attach_export);
    if(!weaponhand::executable(address,native))return hand::Reply::unavailable;
    wire::Registration registration{};registration.hand_cookie=weaponhand::client.cookie();
    registration.native_started_qpc=weaponhand::client.native_started_qpc();registration.context=&weaponhand::client;registration.prepare=prepare;
    if(!wire::registration_shape_valid(registration)||!weaponhand::client.registration_alive(registration.hand_cookie,registration.native_started_qpc))
        return hand::Reply::unavailable;
    hand::RegistrationResult receipt{};hand::Reply reply=hand::Reply::unavailable;
    if(!call_attach(reinterpret_cast<wire::Attach>(address),registration,receipt,reply))return hand::Reply::unavailable;
    if(reply!=hand::Reply::accepted)return reply;
    if(receipt.size!=sizeof(receipt)||receipt.abi_version!=hand::version||receipt.cookie!=registration.hand_cookie||
       receipt.native_started_qpc!=registration.native_started_qpc)return hand::Reply::invalid;
    return weaponhand::client.registration_alive(registration.hand_cookie,registration.native_started_qpc)?hand::Reply::accepted:hand::Reply::draining;
}
} // namespace bodymountprovider
