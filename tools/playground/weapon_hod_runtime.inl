// HOD normal-GT preparation, shared R2 edge ledger and exact effect owner.
// Original conditions/warmup/target/Beam stay authoritative.
// Preparing a ray grants no fire, target validity, left lease or resource.
namespace hod_runtime {
namespace hand=gears_weapon_hand;namespace fire=unified_weapon_fire;
unsigned gate{~0u},rows{},success_rows{};std::atomic<bool> stop_requested{};
unsigned gate1_rows{},last_gate1_reason{~0u};
unsigned last_frame_failure{~0u},last_game_difference{~0u},last_game_fields{~0u};
void report(unsigned value) noexcept {
    if(value){hod_input::cancel();hod_aim::cancel();}
    if(gate==value&&!(value==0&&hod_trace::query_ray_commits&&success_rows++<4))return;gate=value;
    if(log_file&&rows++<12){fwprintf(log_file,L"hod_preparation gate=%u origin_writes=%llu aim_writes=%llu query_returns=%llu fx_returns=%llu query_ray_commits=%llu fx_ray_commits=%llu input_compiled=%u\n",
        gate,hod_trace::origin_writes,hod_trace::aim_writes,hod_trace::query_returns,hod_trace::fx_returns,
        hod_trace::query_ray_commits,hod_trace::fx_ray_commits,
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_R2_TRIAL)
        1u
#else
        0u
#endif
        );fflush(log_file);}
}
void request_stop() noexcept {stop_requested.store(true,std::memory_order_release);}
bool owed() noexcept {return hod_input::owed()||hod_aim::owed();}
bool stopped() noexcept {return !owed()&&hod_trace::active_scopes.load(std::memory_order_acquire)==0;}
void tick(uptr pc) noexcept {
    hod_trace::clear();
    hod_trace::service_observation();
    const bool stopping=stop_requested.load(std::memory_order_acquire)||!enabled.load(std::memory_order_acquire);
    hod_input::service(stopping);hod_aim::service(stopping);
    if(stopping)return;
    // A routing hint only. Avoid expensive HOD discovery for every other gun.
    const auto selected=uptr(vrequip::model.held_weapon());
    if(!selected||!playground::instance(selected,L"WEAP_HammerOfDawn")){hod_input::cancel();hod_aim::cancel();return;}
    if(!weaponhand::client.published_main_hand_hint_on_gt(selected)){
        static unsigned wait_rows{},last_wait_state{~0u};
        static std::uint64_t last_withdrawal_frame{};
        const auto withdrawal=weaponhand::client.withdrawal_on_gt();
        const unsigned state=unsigned(weaponhand::client.phase())|
            (unsigned(weaponhand::client.armed())<<4)|(unsigned(weaponhand::held_binding.known)<<5)|
            (unsigned(nativeequipment::domain_ready())<<6);
        if(log_file&&wait_rows<8&&(state!=last_wait_state||withdrawal.frame!=last_withdrawal_frame)){
            ++wait_rows;last_wait_state=state;last_withdrawal_frame=withdrawal.frame;
            fwprintf(log_file,L"hod_source_wait phase=%u armed=%u held_binding=%u generation=%llu domain=%u cookie=%llu disarm=%u withdrawal=%u difference=%u frame=%llu\n",
                unsigned(weaponhand::client.phase()),unsigned(weaponhand::client.armed()),unsigned(weaponhand::held_binding.known),
                weaponhand::held_binding.generation,unsigned(nativeequipment::domain_ready()),weaponhand::client.cookie(),
                weaponhand::client.last_disarm_reason(),unsigned(withdrawal.failure),withdrawal.game_difference,withdrawal.frame);fflush(log_file);
        }
        return report(11);
    }
    hand::Frame f{};hand::GripGameState game{};
    hand::Decision observed_decision{};hand::FrameReadObservation frame_read{};
    const bool copied=weaponhand::client.copy_frame_on_gt(f,game,observed_decision,&frame_read);
    bool gate1=false;unsigned reason=0;
    if(!copied)reason=1;
    else if(f.controller.address!=pc)reason=2;
    else if(f.weapon.address!=selected)reason=3;
    else if(!game.known)reason=4;
    else if(!game.main_hand_held)reason=5;
    else if(game.busy)reason=6;
    else if(game.animation_owns_hands)reason=7;
    else if(!nativeequipment::current(nullptr,f))reason=8;
    gate1=reason!=0;
    if(gate1){
        if(log_file&&(reason!=last_gate1_reason||unsigned(frame_read.failure)!=last_frame_failure||
            frame_read.game_difference!=last_game_difference||frame_read.game_field_difference!=last_game_fields)&&gate1_rows++<8){
            const auto withdrawal=weaponhand::client.withdrawal_on_gt();
            fwprintf(log_file,L"hod_frame_read failure=%u game_difference=%u observed_game_flags=%u committed_game_flags=%u game_field_difference=%u input_age_ns=%lld since_commit_ns=%lld withdrawal=%u withdrawal_difference=%u withdrawal_frame=%llu withdrawal_sequence=%llu\n",
                unsigned(frame_read.failure),frame_read.game_difference,frame_read.observed_game_flags,
                frame_read.committed_game_flags,frame_read.game_field_difference,frame_read.input_age_ns,
                frame_read.since_commit_check_ns,unsigned(withdrawal.failure),withdrawal.game_difference,
                withdrawal.frame,withdrawal.sequence);
            fwprintf(log_file,L"hod_gate1_reason reason=%u copied=%u controller_match=%u weapon_match=%u game_known=%u main_hand_held=%u busy=%u animation_owns_hands=%u equipment_current_checked=%u equipment_current=%u held_generation=%llu frame_controller=%llx frame_weapon=%llx selected=%llx frame=%llu native_started=%llu world_generation=%llu cookie=%llu\n",
                reason,copied?1u:0u,f.controller.address==pc?1u:0u,f.weapon.address==selected?1u:0u,
                game.known?1u:0u,game.main_hand_held?1u:0u,game.busy?1u:0u,game.animation_owns_hands?1u:0u,
                reason==8?1u:0u,0u,weaponhand::held_binding.generation,
                static_cast<unsigned long long>(f.controller.address),static_cast<unsigned long long>(f.weapon.address),
                static_cast<unsigned long long>(selected),static_cast<unsigned long long>(f.key.frame),
                static_cast<unsigned long long>(f.key.native_started_qpc),
                static_cast<unsigned long long>(f.key.world_generation),
                static_cast<unsigned long long>(f.key.registration_cookie));fflush(log_file);
        }
        last_gate1_reason=reason;
        last_frame_failure=unsigned(frame_read.failure);last_game_difference=frame_read.game_difference;
        last_game_fields=frame_read.game_field_difference;
        return report(1);
    }
    last_gate1_reason=0;
    fire::MuzzleSample sample{};sample.source=f;sample.hold_generation=weaponhand::held_binding.generation;
    sample.epoch=fire::control_epoch.load(std::memory_order_acquire);
    if(!sample.hold_generation||lancermain_asset::read(f,sample.asset,lancermain_asset::AssetScope::hod_use)!=lancermain_asset::Status::ready)
        return report(2);
    Name muzzle{};if(!fire::configured_muzzle(f,muzzle))return report(3);
    playground::Args call(sample.asset.component.address,L"GetSocketWorldLocationAndRotation");
    gripdamage_metadata::Snapshot schema{};schema.module_base=base;
    gripdamage_metadata::Reader r{schema,gripdamage_metadata::Stage::final_check};unsigned mask{},after_mask{};
    fire::SocketGetterLayout before{},after{};
    if(!gripdamage_metadata::tables(base,schema.objects,schema.names)||!fire::socket_getter_schema(call,r,mask,&before)||
       !before.write(call,fire::SocketGetterLayout::input_name,muzzle)||!nativeequipment::current(nullptr,f))return report(4);
    const auto identities=schema.identity_count;
    if(!call.run()||!fire::socket_getter_schema(call,r,after_mask,&after)||mask!=after_mask||
       schema.identity_count!=identities||before.function!=after.function||before.bytes!=after.bytes||before.offsets!=after.offsets)
        return report(5);
    unsigned valid{};
    if(!after.read(call,fire::SocketGetterLayout::result,valid)||(valid&mask)==0||
       !after.read(call,fire::SocketGetterLayout::output_location,sample.origin)||
       !after.read(call,fire::SocketGetterLayout::output_rotation,sample.socket_rotation)||
       !std::isfinite(sample.origin.x)||!std::isfinite(sample.origin.y)||!std::isfinite(sample.origin.z))return report(6);
    constexpr double radians=3.14159265358979323846/32768.;
    const auto direction=gears_weapon_hod_geometry::direction({sample.socket_rotation.pitch*radians,
        sample.socket_rotation.yaw*radians,sample.socket_rotation.roll*radians});
    if(!gears_vr::finite(direction))return report(6);
    sample.aim={int(std::lround(std::atan2(direction.z,std::hypot(direction.x,direction.y))/radians)),
        int(std::lround(std::atan2(direction.y,direction.x)/radians)),0};
    sample.original_socket_origin=sample.origin;
    if(!nativelancerfollow::adjust_muzzle(f,sample.asset,sample.origin,sample.aim,nullptr)||
       !nativelancerfollow::sample_target(f,sample.follow_world_grip,sample.follow_target_mesh,
            sample.render_acquisition_ns,sample.tracking_position_difference,sample.tracking_orientation_difference))return report(7);
    sample.follow_target_known=true;sample.sampled_ns=weaponhand::clock_ns(nullptr);
    hand::GripGameState closing{};lancermain_asset::Evidence asset{};
    if(!hand::frame_active(f,sample.sampled_ns)||!weaponhand::client.source_game_on_gt(f.key,closing)||!hand::same_game(game,closing)||
       !nativeequipment::current(nullptr,f)||lancermain_asset::read(f,asset,lancermain_asset::AssetScope::hod_use)!=lancermain_asset::Status::ready||
       !hand::same(asset.component,sample.asset.component)||!hand::same(asset.asset,sample.asset.asset))return report(8);
    sample.valid=true;
    if(!hod_trace::prepare(sample))return report(9);
    hod_state::Observed state_observed{};
    if(!hod_state::read(f,state_observed))return report(10);
    // Bounded read-only guidance for the recorded demonstration. This is the
    // exact ray supplied to the original trace, not a mesh-property estimate.
    static std::int64_t ray_report_ns{};static unsigned ray_reports{};
    if(log_file&&ray_reports<256&&sample.sampled_ns-ray_report_ns>=100000000){
        ray_report_ns=sample.sampled_ns;++ray_reports;
        fwprintf(log_file,L"hod_ray weapon=%llx generation=%llu sample_ns=%lld x=%.9g y=%.9g z=%.9g pitch=%d yaw=%d roll=%d\n",
            f.weapon.address,sample.hold_generation,sample.sampled_ns,
            double(sample.origin.x),double(sample.origin.y),double(sample.origin.z),
            sample.aim.pitch,sample.aim.yaw,sample.aim.roll);fflush(log_file);
    }
    hod_aim::advance(sample,state_observed);
    // Original L2 can change state and invalidate this exact source. The
    // R2 adapter rechecks currentness; it never relabels the pre-aim frame.
    hod_input::advance(sample,state_observed);
    report(0);
}
} // namespace hod_runtime
