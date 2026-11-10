// Observations of the existing inventory path; never source or selection authority.
struct HammerburstInventoryTiming {
    inline static unsigned idle_rows{},down_rows{},shoulder_rows{};
    abi::Frame const& frame;bool active{},shoulder_up{};unsigned phase{};std::int64_t stamp[5]{};
    explicit HammerburstInventoryTiming(abi::Frame const& f) noexcept:frame(f){
        const bool down=f.controllers.hands[1].squeeze_active==1&&f.controllers.hands[1].squeeze>.35f;
        const auto& right=f.controllers.hands[1];const auto& head=f.stereo.head.position;
        shoulder_up=f.stereo.source==gears_vr::StereoSource::MetaOperatorSimulated&&right.squeeze_active==1&&right.squeeze<=.35f&&
            right.pressed[gears_vr::control_squeeze]==right.released[gears_vr::control_squeeze]&&
            std::abs(right.grip.position.x-head.x-.23)<.04&&std::abs(right.grip.position.y-head.y+.115)<.04&&
            std::abs(right.grip.position.z-head.z+.17)<.04;
        auto& count=shoulder_up?shoulder_rows:down?down_rows:idle_rows;const unsigned limit=shoulder_up?16:down?12:4;
        active=log_file&&count<limit;if(active){++count;mark(0);}
    }
    void mark(unsigned value) noexcept {if(active&&value<5){phase=value;stamp[value]=clock_ns(nullptr);}}
    ~HammerburstInventoryTiming() noexcept {
        if(!active)return;
        const auto ended=clock_ns(nullptr);const auto w=weaponhand::client.withdrawal_on_gt();
        const bool associated=w.frame==frame.key.frame&&w.sequence==frame.controllers.sequence;
        fwprintf(log_file,L"hammerburst_inventory_timing frame=%llu sequence=%llu phase=%u stage=%ls entry_age_ns=%lld exit_age_ns=%lld elapsed_ns=%lld current_before_ns=%lld snapshot_ns=%lld binding_ns=%lld current_after_ns=%lld right_pressed=%u right_released=%u right_down=%u withdrawal_same=%u withdrawal_failure=%u withdrawal_difference=%u withdrawal_age_ns=%lld\n",
            frame.key.frame,frame.controllers.sequence,phase,inventory_stage,stamp[0]-frame.controllers.sync_ns,
            ended-frame.controllers.sync_ns,ended-stamp[0],stamp[1]?stamp[1]-stamp[0]:-1,
            stamp[2]?stamp[2]-stamp[1]:-1,stamp[3]?stamp[3]-stamp[2]:-1,
            stamp[4]?stamp[4]-stamp[3]:phase==3?ended-stamp[3]:-1,
            frame.controllers.hands[1].pressed[gears_vr::control_squeeze],frame.controllers.hands[1].released[gears_vr::control_squeeze],
            unsigned(frame.controllers.hands[1].squeeze>.35f),unsigned(associated),unsigned(w.failure),w.game_difference,w.input_age_ns);
        fflush(log_file);
        if(shoulder_up&&phase==4&&input_evidence.sequence==frame.controllers.sequence&&input_evidence.sync_ns==frame.controllers.sync_ns&&
           (input_evidence.status==gears_equipment_input::Status::baseline||input_evidence.status==gears_equipment_input::Status::quiet)){
            fwprintf(log_file,L"hammerburst_pickup_up frame=%llu sequence=%llu sync_ns=%lld admitted_ns=%lld frame_now_ns=%lld source=%u right_pressed=%u right_released=%u right_down=0 input_status=%hs session=%llu space=%llu input=%llu profile=%llu reference=%llu lifecycle=%llu world_generation=%llu\n",
                frame.key.frame,frame.controllers.sequence,frame.controllers.sync_ns,stamp[4],frame.now_ns,unsigned(frame.stereo.source),
                frame.controllers.hands[1].pressed[gears_vr::control_squeeze],frame.controllers.hands[1].released[gears_vr::control_squeeze],
                gears_equipment_input::status_name(input_evidence.status),frame.controllers.session,frame.controllers.space_epoch,
                frame.controllers.input_epoch,frame.controllers.profile_generation,frame.reference_generation,frame.key.lifecycle,frame.key.world_generation);
            fflush(log_file);
        }
    }
};
