// Included inside boltok_runtime. Reuse the original revisioned writer and
// reload owner; no timer completion, fabricated native receipt or ammo poke.
// Last observed feed sample is a diagnostic receipt only. No consumer uses
// it as a pose, input, lease, inventory or motion permission.
struct FeedTrace {
    std::uint64_t item{},lifecycle{},weapon{},frame{};std::int64_t previous_sync{},last_gap{};
    unsigned rows{},last_slot{6};gears_weapon::FeedResult result{},last_result{};
    V logged_left{};bool known{},have_sample{},tail_reported{};
    double physical_scale{};gears_weapon::FeedSample last{};
};
FeedTrace feed_trace;
void emit_feed_trace(wchar_t const* label) noexcept {
    if(!log_file||!feed_trace.have_sample)return;
    auto const& t=feed_trace;auto const& v=t.last;auto const& face=v.weapon_pose;auto const& left=v.left_grip;
    fwprintf(log_file,L"%ls frame=%llu sequence=%llu token=%llu item=%llu sync_ns=%lld gap_ns=%lld result=%u slot=%u physical_scale=%.9g face_m=%.12g,%.12g,%.12g face_q=%.12g,%.12g,%.12g,%.12g left_m=%.12g,%.12g,%.12g left_q=%.12g,%.12g,%.12g,%.12g age_ns=%lld\n",
        label,t.frame,v.sequence,t.item,v.item,v.monotonic_ns,t.last_gap,unsigned(t.last_result),t.last_slot,t.physical_scale,
        face.position.x,face.position.y,face.position.z,face.orientation.x,face.orientation.y,face.orientation.z,face.orientation.w,
        left.position.x,left.position.y,left.position.z,left.orientation.x,left.orientation.y,left.orientation.z,left.orientation.w,
        weaponhand::clock_ns(nullptr)-v.monotonic_ns);fflush(log_file);
}
void report_feed_tail() noexcept {
    if(round_item&&feed_trace.item==round_item&&feed_trace.have_sample&&!feed_trace.tail_reported){
        feed_trace.tail_reported=true;emit_feed_trace(L"boltok_feed_tail");
    }
}
bool recover_ammo() noexcept {
    perfect_window.reset(); // A dispatched completion already owns its immutable result.
    feed_motion::reset();
    inv::Request request{};
    if(transaction.dispatched_request(request)){
        if(request.call!=gears_weapon::ReloadTxnCall::insert){
            // A physically closed completion remains our once-only outcome
            // even if stop arrived before the next ordinary GT service.
            if(lancerreload_state::retains_serial(reload_serial)&&!lancerreload_state::recover_serial(reload_serial))return false;
            if(request.result==gears_weapon::ReloadTxnResult::perfect&&
               !lancerreload_state::perfect_reward(request,opening_inventory,inventory_observer,reward))return false;
            inv::Inventory counts{};if(!inventory_observer.sample_owned(request.identity,counts))return false;
            const auto applied=request.result==gears_weapon::ReloadTxnResult::inspection?
                gears_weapon::ReloadTxnResult::none:request.result;
            if(!transaction.acknowledge({request.token,true,counts,applied}))return false;
        }else{
        if(!lancerreload_state::restore_partial_insert(request,inventory_observer))return false;
        inv::Inventory counts{};if(!inventory_observer.sample_owned(request.identity,counts))return false;
        const bool applied=inv::counts_equal(counts,request.expected);
        if(!applied&&!inv::counts_equal(counts,request.before))return false;
        if(applied){
            const auto quantity=request.expected.loaded-request.before.loaded;
            if(!gears_boltok_group_mask::valid_ack(live_mask,pending_group_mask,quantity))return false;
        }
        if(!transaction.acknowledge({request.token,applied,counts,gears_weapon::ReloadTxnResult::none}))return false;
        if(applied)live_mask|=pending_group_mask;
        }
    }
    if(round_item){report_feed_tail();transaction.release_item(round_item);round_item=0;insertion.reset();}
    transaction.interrupt(); // Cancels any uncommitted logical reservation.
    if(lancerreload_state::retains_serial(reload_serial)&&!lancerreload_state::recover_serial(reload_serial))return false;
    return true;
}
bool finish_ammo(hand::Frame const& f) noexcept {
    if(!cycle||round_item||transaction.pending())return false;
    if(transaction.stage()==gears_weapon::ReloadTxnStage::open&&!transaction.request_close())return false;
    std::int64_t tick=f.controllers.sync_ns/1000000;
    if(reload_serial&&!lancerreload_state::closure_time(f,reload_serial,tick))return false;
    if(!transaction.confirm_closed(tick,perfect_window.window_for_close(reload_serial,tick)))return false;
    if(reload_serial&&!transaction.take_request(completion))return false;
    stage=Stage::completing;native_exited=false;return true;
}
void complete_ammo() noexcept {
    if(!native_exited){
        if(reload_serial&&!lancerreload_state::recover_serial(reload_serial))return;
        native_exited=true;return;
    }
    if(reload_serial){
        if(completion.result==gears_weapon::ReloadTxnResult::perfect&&
           !lancerreload_state::perfect_reward(completion,opening_inventory,inventory_observer,reward))return;
        inv::Inventory counts{};if(!inventory_observer.sample_owned(completion.identity,counts))return;
        const auto result=completion.result==gears_weapon::ReloadTxnResult::inspection?
            gears_weapon::ReloadTxnResult::none:completion.result;
        if(!transaction.acknowledge({completion.token,true,counts,result})){stage=Stage::recovering;return;}
    }
    if(log_file){fwprintf(log_file,L"boltok_reload_complete inserted=%u target=%u result=%u mask=%u reward_called=%u reward_applied=%u reward_eligible=%u bonus=%d physical_headset_pending=1\n",
        transaction.inserted(),transaction.target(),unsigned(transaction.result()),live_mask,
        unsigned(reward.dispatched&&reward.call_confirmed),unsigned(reward.applied),unsigned(reward.eligible),reward.final_bonus);fflush(log_file);}
    transaction={};inventory_writer={};completion={};opening_inventory={};reward={};
    cycle=native_exited=false;reload_serial=0;perfect_ms=0;perfect_window.reset();auto_close.reset();grip_close.reset();grip_closing=false;close_release::reset();stage=Stage::idle;
}
bool ammo_step(hand::Frame const& f,hand::Decision const& d,boltok_parts::Mechanical const& parts,
               gears_weapon::BoltokReloadStamp const& s,std::int64_t) noexcept {
    if(!round_item){
        if(stage!=Stage::opened||!cycle||!boltok_visual::ready||!d.new_grant||
           !granted(f,d,hand::LeaseKind::ammo)||transaction.inserted()>=transaction.target())return false;
        gears_weapon::FeedPortMount socket{};
        if(!boltok_visual::whole_socket(socket)||!transaction.reserve_group_item(f.press_token)||
           !gears_boltok_group_mask::select_missing(live_mask,transaction.reserved(),pending_group_mask)||
           !insertion.arm(s.identity,f.press_token,socket)){restore();return true;}
        feed_motion::reset();round_item=f.press_token;round_sequence=0;held=d.lease;
        if(log_file){auto const& mount=socket.item_in_left_grip;
            fwprintf(log_file,L"boltok_cylinder_mount frame=%llu sequence=%llu token=%llu item=%llu weapon=%llx profile=%llu units=%.12g physical_scale=%.12g item_in_grip_m=%.12g,%.12g,%.12g item_rotation=%.12g,%.12g,%.12g,%.12g quantity=%u group_mask=%u observation_only=1 capture=1794500066511-3\n",
                f.key.frame,f.controllers.sequence,round_item,round_item,f.weapon.address,
                gears_boltok_owner_profiles::ammo.profile_id,parts.units,boltok_visual::geometry.physical_scale(),
                mount.position.x,mount.position.y,mount.position.z,
                mount.orientation.x,mount.orientation.y,mount.orientation.z,mount.orientation.w,
                transaction.reserved(),pending_group_mask);fflush(log_file);
        }
        return true; // Never reread a shoulder grant as an insertion proposal.
    }
    if(!f.left_grip_known||!f.left_tracked){restore();return true;}
    if(!f.left_grip_down){
        perfect_window.alignment_lost();
        report_feed_tail();
        if(!transaction.release_item(round_item)||!boltok_visual::hide_held()){restore();return true;}
        round_item=0;pending_group_mask=0;held={};insertion.reset();feed_motion::reset();return true;
    }
    const auto blocked=[&](unsigned gate) noexcept {
        static unsigned rows{},prior_gate{};static std::uint64_t prior_token{};
        if(log_file&&rows<8&&(prior_token!=f.press_token||prior_gate!=gate)){
            ++rows;prior_token=f.press_token;prior_gate=gate;
            fwprintf(log_file,L"boltok_feed_block gate=%u frame=%llu sequence=%llu token=%llu item=%llu lease=%u held=%llu age_ns=%lld\n",
                gate,f.key.frame,f.controllers.sequence,f.press_token,round_item,unsigned(d.lease.kind),held.id,
                weaponhand::clock_ns(nullptr)-f.controllers.sync_ns);fflush(log_file);
        }
    };
    if(!granted(f,d,hand::LeaseKind::ammo)||!hand::same(held,d.lease)){
        perfect_window.alignment_lost();
        feed_motion::reset();insertion.observe_original_geometry_path(s,weaponhand::clock_ns(nullptr),round_item,nullptr,0,false);blocked(1);return true;}
    if(f.controllers.sequence<=round_sequence)return true;round_sequence=f.controllers.sequence;
    const auto& root=parts.root_metres;const auto& local=parts.in_root[1];
    const P bone{gears_weapon::scaled(gears_weapon::sum(root.position,gears_weapon::rotate(root.orientation,local.position)),parts.units),
        gears_vr::normalized(gears_vr::multiply(root.orientation,local.orientation))};
    P face{};if(!boltok_visual::geometry.cylinder(bone,face)){restore();return true;}
    face.position=gears_weapon::scaled(face.position,1/parts.units);
    P left=actual_left_world;left.position=gears_weapon::scaled(left.position,1/parts.units);
    const gears_weapon::FeedSample sample{f.weapon.address,round_item,f.world.address,f.stereo.space_epoch,
        f.controllers.sequence,f.controllers.sync_ns,face,left,true,true};
    const auto feed_result=feed_motion::observe(f,d,parts,s,sample);
    if(feed_motion::acquired_sync&&!perfect_window.seen_alignment()){
        std::int64_t aligned_tick{};
        if(lancerreload_state::closure_time(f,reload_serial,aligned_tick)&&
           perfect_window.arm_first_alignment(reload_serial,aligned_tick)&&log_file){
            fwprintf(log_file,L"boltok_perfect_window policy=first_alignment serial=%llu anchor_tick=%lld window_ms=600 frame=%llu sequence=%llu observed_ns=%lld\n",
                reload_serial,aligned_tick,f.key.frame,f.controllers.sequence,weaponhand::clock_ns(nullptr));fflush(log_file);
        }
    }else if(!feed_motion::acquired_sync)perfect_window.alignment_lost();
    auto& trace=feed_trace;
    if(trace.item!=round_item||trace.lifecycle!=f.key.lifecycle||trace.weapon!=f.weapon.address){
        trace={};trace.item=round_item;trace.lifecycle=f.key.lifecycle;trace.weapon=f.weapon.address;
    }
    const auto gap=trace.previous_sync?sample.monotonic_ns-trace.previous_sync:0;
    const bool near_part=gears_weapon::length(gears_weapon::difference(left.position,face.position))<.16*boltok_visual::geometry.physical_scale();
    trace.frame=f.key.frame;trace.last=sample;trace.last_gap=gap;trace.last_result=feed_result;
    trace.last_slot=6;trace.physical_scale=boltok_visual::geometry.physical_scale();trace.have_sample=true;
    if(log_file&&trace.rows<20&&(!trace.known||trace.result!=feed_result||(near_part&&
       (gap>100000000||gears_weapon::length(gears_weapon::difference(trace.logged_left,left.position))>.005)))){
        ++trace.rows;trace.known=true;trace.result=feed_result;trace.logged_left=left.position;
        emit_feed_trace(L"boltok_feed_sample");
    }
    trace.previous_sync=sample.monotonic_ns;
    if(feed_result!=gears_weapon::FeedResult::inserted)return false;
    if(!gears_boltok_group_mask::valid_ack(live_mask,pending_group_mask,transaction.reserved())){restore();return true;}
    inv::Request request{};inv::Receipt receipt{};
    if(!transaction.request_insertion(round_item)||!transaction.take_request(request)){restore();return true;}
    const auto result=inventory_writer.commit(f,request,inventory_observer,lancerreload_state::owns,receipt);
    if(!receipt.token){receipt.token=request.token;inventory_observer.sample_owned(request.identity,receipt.observed);}
    if(result==inv::CommitResult::reconciliation_required){transaction.require_reconciliation();restore();return true;}
    if(!transaction.acknowledge(receipt)){restore();return true;}
    if(result==inv::CommitResult::committed){
        if(log_file){fwprintf(log_file,
            L"boltok_cylinder_inserted serial=%llu item=%llu group_mask=%u accepted=%u capacity=%u loaded=%u reserve=%u infinite=%u frame=%llu sequence=%llu token=%llu\n",
            reload_serial,round_item,pending_group_mask,unsigned(receipt.accepted),
            receipt.observed.capacity,receipt.observed.loaded,receipt.observed.reserve,
            unsigned(receipt.observed.infinite_reserve),f.key.frame,f.controllers.sequence,f.press_token);fflush(log_file);}
        live_mask|=pending_group_mask;boltok_sound::event(f,2,round_item);
        std::int64_t inserted_tick{};
        if(!lancerreload_state::closure_time(f,reload_serial,inserted_tick)||
           !perfect_window.confirm_insertion(reload_serial,inserted_tick)){restore();return true;}
        if(!auto_close.begin(request.item_serial,request.reload_serial,receipt.token)){restore();return true;}
        if(!grip_close.begin(s,f.press_token,weaponhand::clock_ns(nullptr))){restore();return true;}
        stage=Stage::closing;closing=true;moving=true;grip_closing=true;angle=0;last_intent=f.press_token;
        // Dispatch while this SAME committed insert Frame still owns its real
        // ammo lease. Later ticks only await a newer native endpoint; they do
        // not recreate a part grant after the hand-held item has disappeared.
        const auto update=function<api::Update>(api::update_export);
        control.source=f;control.angle=0;
        api::Receipt closed_request{};
        const auto close_result=update?update(&control,&closed_request):api::Result::rejected;
        if((close_result!=api::Result::ready&&close_result!=api::Result::pending)||
           closed_request.size!=sizeof closed_request||closed_request.version!=1||
           closed_request.handle!=control.handle||closed_request.result!=close_result||
           closed_request.hidden!=control.hide_cylinder||!std::isfinite(closed_request.angle)||
           std::abs(closed_request.angle)>1e-6||
           !grip_close.dispatch(s,f.press_token,weaponhand::clock_ns(nullptr))){restore();return true;}
        native_receipt=closed_request;last_update_result=close_result;
        if(log_file){fwprintf(log_file,L"boltok_auto_close policy=insert_ack serial=%llu item=%llu receipt_token=%llu frame=%llu sequence=%llu observed_ns=%lld extra_grip=0\n",
            reload_serial,request.item_serial,receipt.token,f.key.frame,f.controllers.sequence,weaponhand::clock_ns(nullptr));fflush(log_file);}

    }
    if(!boltok_visual::hide_held()){restore();return true;}
    round_item=0;pending_group_mask=0;held={};insertion.reset();feed_motion::reset();
    return true;
}
