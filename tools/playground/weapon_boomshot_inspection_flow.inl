// Included inside the unique normal-GT Owner. This is the zero-target branch
// of that owner, not a second input/animation/inventory controller.
    core::BoomshotEvent start_inspection(hand::Frame const& f,hand::Decision const& d,core::BoomshotMechanismGrant& grant) noexcept {
        if(busy_||!inspection_mode_||stage_!=Stage::prepared||!weaponhand::current_gt(nullptr)||
           !(context_==inv::identity(f))||!boomshot_inspection_clip::fire_neutral(f)||native::lease.acquired||
           boomshot_clip::serial||boomshot_inspection_clip::owner.active()||boomshot_carrier_original::owner.active())return core::BoomshotEvent::rejected;
        Busy guard(busy_);auto* tx=transaction();auto* stream=events();boomshot_original::Source actual{};
        if(!tx||!stream||!tx->inspection_only()||!original_closed(f,actual)||
           !hand::same(actual.parts.component,original_.parts.component)||!hand::same(actual.parts.asset,original_.parts.asset)||
           !hand::same(actual.carrier,original_.carrier)||!hand::same(actual.carrier_component,original_.carrier_component)||
           !(actual.counts==original_.counts))return core::BoomshotEvent::rejected;
        core::BoomshotOwnerEvents::InputEvent event{};core::BoomshotFrame frame{};
        if(boomshot_core_source::input(f,*tx,*stream,event,frame)!=boomshot_core_source::Result::ready||
           !grant.begin_allowed(f,d,core::BoomshotContact::opening,weaponhand::clock_ns(nullptr)))return core::BoomshotEvent::rejected;
        const auto next_clip=boomshot_inspection_clip::owner.available_serial();if(!next_clip)return core::BoomshotEvent::rejected;
        // Persist before original Play; a reentrant call cannot replay it.
        gun_only_=true;serial_=gun_serial_=next_clip;stage_=Stage::starting;dispatched_=true;
        const auto begun=boomshot_inspection_clip::owner.begin(f,next_clip);
        if(begun==boomshot_inspection_clip::Result::rejected&&!boomshot_inspection_clip::owner.active()){
            gun_only_=false;serial_=gun_serial_=0;dispatched_=false;stage_=Stage::prepared;return core::BoomshotEvent::rejected;
        }
        // Journal the already-dispatched operation with its original input.
        // Later physical motion still needs a fresh retained grant and source.
        core::BoomshotInspectionStart request{tx->binding_token(),serial_,plan_,gun_clip_matches(),!native::lease.acquired};
        const auto accepted=tx->begin_inspection(frame,request,f.press_token);
        if(accepted!=core::BoomshotEvent::started||begun==boomshot_inspection_clip::Result::recovery_required||!gun_clip_owned()){
            fail();return core::BoomshotEvent::interrupted;
        }
        stage_=Stage::mechanical;return accepted;
    }
    core::BoomshotEvent complete_inspection_owned() noexcept {
        if(busy_||!inspection_mode_||!weaponhand::current_gt(nullptr)||
           (stage_!=Stage::mechanical&&stage_!=Stage::completing))return core::BoomshotEvent::rejected;
        Busy guard(busy_);auto* tx=transaction();if(!tx||!tx->inspection_only())return core::BoomshotEvent::rejected;
        if(!inspection_exit_.token){
            core::BoomshotInspectionExit owed{};
            if(tx->stage()!=core::BoomshotStage::completion_commit||!tx->take_inspection_exit(owed))return core::BoomshotEvent::none;
            inspection_exit_=owed;stage_=Stage::completing;
        }
        if(!(inspection_exit_.identity==slot_->identity())||inspection_exit_.binding!=tx->binding_token()||
           inspection_exit_.animation_serial!=serial_||native::lease.acquired||boomshot_clip::serial||
           boomshot_carrier_original::owner.active())return core::BoomshotEvent::none;
        inv::Inventory observed_counts{};core::BoomshotInventory mirrored{};
        if(!observer_.sample_owned(context_,observed_counts)||!mirror(observed_counts,mirrored)||
           mirrored.revision!=tx->inventory().revision||mirrored.loaded!=tx->inventory().loaded||
           mirrored.reserve!=tx->inventory().reserve||mirrored.infinite_reserve!=tx->inventory().infinite_reserve)return core::BoomshotEvent::none;
        if(boomshot_inspection_clip::owner.active()&&(!gun_clip_matches()||!boomshot_inspection_clip::owner.drain()))return core::BoomshotEvent::none;
        if(!boomshot_barrel::retire())return core::BoomshotEvent::none;
        exit_confirmed_=true; // Own gun clip handed back; no native exit call.
        core::BoomshotOriginalReceipt receipt{};
        if(!original_receipt({inspection_exit_.token,true,observed_counts,core::ReloadTxnResult::none},receipt,true))return core::BoomshotEvent::none;
        receipt.original_ready=true;
        const auto result=tx->acknowledge_inspection(receipt);
        if(result!=core::BoomshotEvent::inspection_completed)return result;
        slot_=nullptr;context_={};original_={};plan_={};inspection_exit_={};serial_=item_=gun_serial_=0;
        dispatched_=gun_only_=inspection_mode_=false;stage_=Stage::idle;return result;
    }
    bool drain_inspection_owned() noexcept {
        if(busy_||!inspection_mode_||!weaponhand::current_gt(nullptr))return false;
        auto* tx=transaction();if(!tx)return false;
        if(stage_==Stage::completing||tx->stage()==core::BoomshotStage::completion_commit)
            return complete_inspection_owned()==core::BoomshotEvent::inspection_completed;
        Busy guard(busy_);
        if(!serial_||!dispatched_||native::lease.acquired||boomshot_clip::serial||boomshot_carrier_original::owner.active())return false;
        if(!restoring_debt_){double progress{};if(boomshot_barrel::snapshot_progress(context_,gun_serial_,progress,true)){
            saved_progress_=progress;saved_progress_known_=true;
        }}
        if(stage_!=Stage::recovery)fail();
        inv::Inventory observed_counts{};core::BoomshotInventory mirrored{};core::BoomshotInteraction::ReloadRequest foreign_request{};
        if(tx->dispatched_reload_request(foreign_request)||!tx->inspection_only()||tx->step_index()>1||item_||
           !observer_.sample_owned(context_,observed_counts)||!mirror(observed_counts,mirrored)||mirrored.revision!=tx->inventory().revision||
           mirrored.loaded!=tx->inventory().loaded||mirrored.reserve!=tx->inventory().reserve||
           mirrored.infinite_reserve!=tx->inventory().infinite_reserve)return false;
        if(boomshot_inspection_clip::owner.active()&&(!gun_clip_matches()||!boomshot_inspection_clip::owner.drain()))return false;
        if(!boomshot_barrel::retire())return false;
        exit_confirmed_=true;gun_only_=false;gun_serial_=0;stage_=Stage::parked;return true;
    }
    core::BoomshotEvent resume_inspection(hand::Frame const& f,hand::Decision const& d,core::BoomshotMechanismGrant& grant) noexcept {
        if(busy_||!inspection_mode_||!weaponhand::current_gt(nullptr)||stage_!=Stage::parked||!slot_||!exit_confirmed_||
           !saved_progress_known_||inspection_exit_.token||native::lease.acquired||boomshot_clip::serial||
           boomshot_inspection_clip::owner.active()||boomshot_carrier_original::owner.active()||
           !boomshot_inspection_clip::fire_neutral(f))return core::BoomshotEvent::rejected;
        Busy guard(busy_);auto* tx=transaction();auto* stream=events();const auto next=inv::identity(f);
        if(!tx||!stream||!tx->inspection_only()||tx->stage()!=core::BoomshotStage::interrupted||tx->step_index()>1||
           (!(next==context_)&&!inv::same_instance_across_hold(context_,next)))return core::BoomshotEvent::rejected;
        const auto purpose=tx->step_index()==0?core::BoomshotContact::opening:core::BoomshotContact::closing;
        boomshot_original::Source actual{};inv::ObservedInventory rebound=observer_;inv::Inventory observed_counts{};core::BoomshotInventory mirrored{};
        if(!grant.begin_allowed(f,d,purpose,weaponhand::clock_ns(nullptr))||!boomshot_original::prepare(f,actual)||
           !hand::same(actual.parts.component,original_.parts.component)||!hand::same(actual.parts.asset,original_.parts.asset)||
           !hand::same(actual.carrier,original_.carrier)||!hand::same(actual.carrier_component,original_.carrier_component)||
           !observer_.sample_owned(context_,observed_counts)||!mirror(observed_counts,mirrored)||mirrored.revision!=tx->inventory().revision||
           mirrored.loaded!=tx->inventory().loaded||mirrored.reserve!=tx->inventory().reserve||
           mirrored.infinite_reserve!=tx->inventory().infinite_reserve||!rebound.rebind_hold(f,context_,observed_counts))return core::BoomshotEvent::rejected;
        core::BoomshotOwnerEvents::InputEvent event{};core::BoomshotFrame frame{};
        if(boomshot_core_source::input(f,*tx,*stream,event,frame)!=boomshot_core_source::Result::ready||
           !tx->resume_input_current(frame,f.press_token,0,tx->step_index(),false)||
           !grant.begin_allowed(f,d,purpose,weaponhand::clock_ns(nullptr)))return core::BoomshotEvent::rejected;
        const auto next_clip=boomshot_inspection_clip::owner.available_serial();const auto previous_context=context_;
        const auto previous_observer=observer_;const auto previous_opening=opening_;
        if(!next_clip||!slot_->rebind_source(previous_context,next))return core::BoomshotEvent::rejected;
        if(!grant.started(f,d,purpose,weaponhand::clock_ns(nullptr))){slot_->rebind_source(next,previous_context);return core::BoomshotEvent::rejected;}
        context_=next;observer_=rebound;opening_.identity=next;stage_=Stage::restoring;restoring_debt_=true;
        restore_purpose_=purpose;restore_intent_=f.press_token;restore_grant_started_=true;restore_seek_sent_=false;restore_seek_frame_=0;
        gun_only_=true;gun_serial_=next_clip; // Old core operation serial stays.
        const auto begun=boomshot_inspection_clip::owner.begin(f,next_clip);
        if(begun==boomshot_inspection_clip::Result::rejected&&!boomshot_inspection_clip::owner.active()&&slot_->rebind_source(next,previous_context)){
            context_=previous_context;observer_=previous_observer;opening_=previous_opening;
            gun_only_=false;gun_serial_=0;restoring_debt_=false;stage_=Stage::parked;return core::BoomshotEvent::rejected;
        }
        if(begun==boomshot_inspection_clip::Result::rejected||begun==boomshot_inspection_clip::Result::recovery_required||!gun_clip_owned()){
            fail();return core::BoomshotEvent::interrupted;
        }
        return core::BoomshotEvent::started;
    }
