// Inside mounted_aim_candidates, after the observed-gun reader. Uses the
// existing Native module admission; no new tracking source/hand Frame/writer.
namespace c0_api=gears_troika_c0;
c0_api::Frame hand_receipt{};unsigned hand_log_rows{};bool last_hand_ready{};
gears_rig_tracking::Reply invoke_c0(c0_api::Read fn,c0_api::Request const* q,c0_api::Frame* out) noexcept {
    __try{return fn(q,out);}
    __except(GetExceptionCode()==0xe06d7363ul||GetExceptionCode()==EXCEPTION_STACK_OVERFLOW?
        EXCEPTION_CONTINUE_SEARCH:EXCEPTION_EXECUTE_HANDLER){handrig::admission.quarantine();return gears_rig_tracking::Reply::unavailable;}
}
bool read_hands(fire::Run const& run,c0_api::Frame& output) noexcept {
    if(handrig::reading||!weaponhand::native_module||!gears_rig_tracking::same(handrig::key,run.source.frame.native))return false;
    handrig::lifetime::Admission::Entry entry{};if(!handrig::admission.acquire(entry))return false;
    struct Local {handrig::lifetime::Admission::Entry& entry;bool held{true};
        ~Local(){if(held){handrig::reading=false;handrig::admission.release(entry);}}} local{entry};
    handrig::reading=true;auto const* lifetime=handrig::admission.view(entry);auto const& source=run.source.frame;
    const auto live=[&](){return lifetime&&lifetime->context==&weaponhand::client&&lifetime->cookie==source.native.registration_cookie&&
        lifetime->native_started_qpc==source.native.native_started_qpc&&weaponhand::client.phase()==hand::ClientPhase::active&&
        weaponhand::client.cookie()==source.native.registration_cookie&&weaponhand::client.native_started_qpc()==source.native.native_started_qpc&&
        weaponhand::current_gt(nullptr)==source.native.game_thread;};
    const auto address=GetProcAddress(weaponhand::native_module,c0_api::read_export);
    if(!live()||!weaponhand::executable(address,weaponhand::native_module))return false;
    c0_api::Request q{};q.source=run.source;q.control=run.control;c0_api::Frame value{};
    const auto reply=invoke_c0(reinterpret_cast<c0_api::Read>(address),&q,&value);
    const bool okay=reply==gears_rig_tracking::Reply::accepted&&live()&&c0_api::valid(value,q,weaponhand::clock_ns(nullptr));
    handrig::reading=false;const bool released=handrig::admission.release(entry);local.held=false;
    if(!okay||!released)return false;output=value;return true;
}
bool qualified(fire::Run const& run) noexcept {
    try{
        auto check=run;
        if(applying.load()||!proposal_ready||!gun_receipt.ready||!current(&check)||
           std::memcmp(&proposal.run.source,&run.source,sizeof run.source)||std::memcmp(&proposal.run.control,&run.control,sizeof run.control)||
           std::memcmp(&gun_receipt.run.source,&run.source,sizeof run.source)||std::memcmp(&gun_receipt.run.control,&run.control,sizeof run.control)||
           gun_receipt.calibration!=anchors.calibration)return false;
        std::array<std::int32_t,3> controller{},aim{};
        if(!controller_direction(check,controller)||!aim_direction(check,aim)||!action::rotation_matches(controller,gun_receipt.actor_direction)||
           !action::rotation_matches(aim,controller)||!read_hands(run,hand_receipt)||!current(&check))return false;
        gears_weapon::TroikaMountedMapping mapped{};
        if(!troika_tracking::map(owner,run.source.frame,mapped)||mapped.units_per_metre!=hand_receipt.units_per_metre||mapped.hands.tracked_mask!=3)return false;
        for(unsigned side=0;side<2;++side)if(!c0_api::pose_matches(mapped.hands.world[side],hand_receipt.grip_world[side],0,1e-6))return false;
        return true;
    }catch(...){return false;}
}
// Duplicate service calls do not offer/reissue an input edge or authorize a
// new aim. Maintain only an already owned original action, with the SAME live
// source/control and actual C0 buffers; otherwise the normal owner stops it.
bool hold_current(fire::Run const& run) noexcept {
    try{auto check=run;c0_api::Frame held_pose{};
        return gun_receipt.ready&&!std::memcmp(&gun_receipt.run.source,&run.source,sizeof run.source)&&
            !std::memcmp(&gun_receipt.run.control,&run.control,sizeof run.control)&&current(&check)&&read_hands(run,held_pose)&&current(&check);
    }catch(...){return false;}
}
void observe_hands(fire::Run const& run) noexcept {
    c0_api::Frame hand_pose{};const bool ready=read_hands(run,hand_pose);
    if(ready)hand_receipt=hand_pose;
    if(log_file&&hand_log_rows<12&&(!hand_log_rows||ready!=last_hand_ready)){++hand_log_rows;last_hand_ready=ready;
        fwprintf(log_file,L"troika_C0_observed source=%llu ready=%u tick=%llu compose=%llu kernel=%llu solved=%u projected=%u actual_local_space=%u fit_accepted=0\n",
            run.source.frame.publication_sequence,unsigned(ready),hand_pose.tick,hand_pose.compose,hand_pose.kernel_serial,hand_pose.solved_mask,hand_pose.projected_mask,unsigned(ready));fflush(log_file);}
}
