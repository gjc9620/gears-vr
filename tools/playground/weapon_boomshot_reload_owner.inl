// Sole normal-GT reload/zero-target inspection owner. Native callbacks retain
// grants only; original animation/actions and their owed receipts stay here.
#include "../vr-common/weapon_boomshot_insertion.h"
namespace boomshot_reload_owner {
namespace hand=gears_weapon_hand;namespace inv=lancerreload_inventory;namespace core=gears_weapon;
namespace native=lancerreload_state;namespace motion=core::boomshot_barrel;
std::uint64_t next_item_serial{1};
class Owner {
public:
    enum class Stage {idle,prepared,starting,mechanical,seating,completing,recovery,parked,restoring};
    enum class EndpointKind {none,opened,seated,closed};
    struct Endpoint {
        EndpointKind kind{};inv::Identity identity{};hand::FrameKey key{};
        std::uint64_t ordinal{},serial{},reference{},profile{},raw_sequence{};
        std::int64_t raw_sync_ns{},observed_ns{};unsigned source{};
    };
private:
    Stage stage_{};boomshot_sessions::Slot* slot_{};inv::Identity context_{};
    boomshot_original::Source original_{};core::BoomshotPlan plan_{};
    std::uint64_t serial_{},item_{};bool dispatched_{},busy_{};boomshot_core_motion::Bridge bridge_{};
    inv::ObservedInventory observer_{};inv::InsertionWriter writer_{};inv::Inventory opening_{};
    inv::Request original_request_{};std::uint64_t core_revision_base_{},original_revision_base_{};
    inv::Request completion_{};native::RewardReceipt reward_{};
    bool inventory_sent_{},exit_sent_{},exit_confirmed_{};
    core::boomshot_insertion::Stroke insertion_{};core::BoomshotOwnerEvents::InputEvent entry_event_{};
    std::uint64_t entry_frame_{};bool insertion_armed_{};
    native::Lease suspended_native_{};bool suspended_known_{};
    double saved_progress_{};bool saved_progress_known_{},restoring_debt_{},restore_grant_started_{},restore_seek_sent_{};
    std::uint64_t restore_intent_{},restore_seek_frame_{},saved_curve_content_{},writer_token_floor_{};
    core::BoomshotIdentity writer_actor_{};core::BoomshotContact restore_purpose_{};
    bool gun_only_{},inspection_mode_{};std::uint64_t gun_serial_{};
    core::BoomshotInspectionExit inspection_exit_{};
    bool removing_{},old_pulled_{};
    std::uint64_t removal_item_{};
    gears_vr::Vector removal_origin_{};
    unsigned insertion_step()const noexcept{return plan_.direct_exchange?0u:1u;}
    static gears_vr::Vector hand_offset(hand::Frame const& f)noexcept{
        const auto a=f.controllers.hands[0].grip.position,b=f.controllers.hands[1].grip.position;
        return {a.x-b.x,a.y-b.y,a.z-b.z};
    }
    std::array<Endpoint,3> endpoints_{};unsigned endpoint_count_{};
    std::uint64_t next_endpoint_{1},endpoint_drops_{};Endpoint seating_source_{};
    static Endpoint endpoint_source(hand::Frame const& f) noexcept {
        Endpoint value{};value.key=f.key;value.reference=f.reference_generation;value.profile=f.controllers.profile_generation;
        value.raw_sequence=f.controllers.sequence;value.raw_sync_ns=f.controllers.sync_ns;value.source=unsigned(f.stereo.source);return value;
    }
    void endpoint(EndpointKind kind,Endpoint source) noexcept {
        // Cosmetic receipt only, emitted AFTER an already accepted physical or
        // original component acknowledgement. Never an action/clock authority.
        if(!weaponhand::current_gt(nullptr)||kind==EndpointKind::none||!serial_||!context_.valid()||
           !next_endpoint_||next_endpoint_==UINT64_MAX||!source.raw_sequence||source.raw_sync_ns<=0)return;
        if(endpoint_count_==endpoints_.size()){++endpoint_drops_;return;}
        source.kind=kind;source.identity=context_;source.ordinal=next_endpoint_++;source.serial=serial_;
        source.observed_ns=weaponhand::clock_ns(nullptr);endpoints_[endpoint_count_++]=source;
    }
    struct Busy {bool& flag;explicit Busy(bool& f):flag(f){flag=true;}~Busy(){flag=false;}};
    core::BoomshotInteraction* transaction() noexcept {return slot_?slot_->core(context_):nullptr;}
    core::BoomshotOwnerEvents* events() noexcept {return slot_?slot_->events(context_):nullptr;}
    bool gun_clip_matches() const noexcept {
        return gun_only_&&gun_serial_&&boomshot_inspection_clip::owner.active()&&
            boomshot_inspection_clip::owner.serial()==gun_serial_&&boomshot_inspection_clip::backend.serial==gun_serial_&&
            boomshot_inspection_clip::backend.identity==context_;
    }
    bool gun_clip_owned() const noexcept {
        return gun_clip_matches()&&!native::lease.acquired&&boomshot_inspection_clip::backend.context()&&
            boomshot_inspection_clip::backend.mine();
    }
    unsigned reclaim_dead_idle_slots() noexcept {
        unsigned retired{};
        // Capacity pressure only: no extra registry scan on ordinary holding
        // frames. Never evict an interrupted actor or this current owner.
        for(auto& candidate:boomshot_sessions::slots){
            if(&candidate==slot_||!candidate.known)continue;
            auto* tx=candidate.core(candidate.first);if(!tx)continue;
            if(tx->stage()!=core::BoomshotStage::unbound&&tx->stage()!=core::BoomshotStage::ready)continue;
            if(lancerreload_runtime::weapon_retired(candidate.first.weapon)&&candidate.retire_source(candidate.first))++retired;
        }
        return retired;
    }
    bool mirror(inv::Inventory const& source,core::BoomshotInventory& out) const noexcept {
        out={};if(!slot_||!source.valid()||!(source.identity==context_)||source.capacity!=1||
           source.revision<original_revision_base_||source.revision-original_revision_base_>UINT64_MAX-core_revision_base_)return false;
        out={slot_->identity(),core_revision_base_+source.revision-original_revision_base_,source.loaded,source.reserve,source.infinite_reserve};return true;
    }
    bool original_receipt(inv::Receipt const& source,core::BoomshotOriginalReceipt& out,bool after_exit=false) noexcept {
        out={};auto* stream=events();core::BoomshotInventory receipt_counts{};inv::Inventory closing{};float time{},again{};
        bool reloading{};unsigned char mode{};
        if(!stream||!source.token||!mirror(source.observed,receipt_counts)||
           (after_exit?(!exit_confirmed_||native::retains_serial(serial_)||
               !native::bool_call(context_.weapon.address,L"IsReloading",reloading)||reloading||
               !native::byte_value(context_.weapon.address,L"CurrentFireMode",mode)||mode!=0):
               (!native::retains_serial(serial_)||!(native::lease.identity==context_)||!native::owned_state()))||
           !native::time_field(context_.world.address,L"TimeSeconds",time)||!std::isfinite(time)||time<0||time>1000000000.f||
           !observer_.sample_owned(context_,closing)||!inv::counts_equal(source.observed,closing)||closing.revision!=source.observed.revision||
           !native::time_field(context_.world.address,L"TimeSeconds",again)||again!=time)return false;
        core::BoomshotOwnerEvents::OriginalEvent event{};
        if(stream->original(weaponhand::clock_ns(nullptr),std::int64_t(double(time)*1000000.),event)!=core::BoomshotOwnerEvents::Result::accepted)return false;
        out={event.stamp(),source.token,event.ordinal(),receipt_counts,source.accepted,source.accepted,core::ReloadTxnResult::none,false};return true;
    }
    core::BoomshotEvent seat_completed() noexcept {
        if(!plan_.direct_exchange)return core::BoomshotEvent::inventory_committed;
        auto* tx=transaction();auto* stream=events();inv::Inventory counts{};core::BoomshotInventory seat_counts{};
        core::BoomshotOwnerEvents::OriginalEvent event{};float time{},again{};bool reloading{};
        if(!tx||!stream||!serial_||!native::retains_serial(serial_)||!(native::lease.identity==context_)||!native::owned_state()||
           !observer_.sample_owned(context_,counts)||!mirror(counts,seat_counts)||
           seat_counts.loaded!=tx->inventory().loaded||seat_counts.loaded!=1||seat_counts.revision!=tx->inventory().revision||seat_counts.reserve!=tx->inventory().reserve||
           seat_counts.infinite_reserve!=tx->inventory().infinite_reserve||
           !native::time_field(context_.world.address,L"TimeSeconds",time)||!std::isfinite(time)||time<0||time>1000000000.f||
           !native::time_field(context_.world.address,L"TimeSeconds",again)||again!=time||
           !native::bool_call(context_.weapon.address,L"IsReloading",reloading)||!reloading||
           stream->original(weaponhand::clock_ns(nullptr),std::int64_t(double(time)*1000000.),event)!=core::BoomshotOwnerEvents::Result::accepted)return fail_event();
        core::BoomshotOriginalReceipt receipt{event.stamp(),original_request_.token,event.ordinal(),seat_counts,true,true,core::ReloadTxnResult::none,false};
        const auto result=tx->acknowledge_seated(receipt);
        if(result==core::BoomshotEvent::request_completion&&log_file){
            fwprintf(log_file,L"boomshot_reload_seated serial=%llu game_tick_us=%lld original_component_seated=1\n",serial_,receipt.stamp.game_tick);fflush(log_file);
        }
        return result;
    }
    core::BoomshotEvent fail_event(unsigned line=0) noexcept {if(log_file){fwprintf(log_file,L"boomshot_owner_failure line=%u serial=%llu writer_phase=%u\n",line,serial_,writer_.phase);fflush(log_file);}fail();return core::BoomshotEvent::interrupted;}
    bool original_closed(hand::Frame const& f,boomshot_original::Source& current_source) noexcept {
        return boomshot_original::prepare(f,current_source)&&nativeequipment::current(nullptr,f);
    }
    void fail() noexcept {
        insertion_.release();if(auto* tx=transaction()){bridge_.release(*tx);tx->interrupt();}stage_=Stage::recovery;
    }
public:
    Owner()=default;Owner(Owner const&)=delete;Owner& operator=(Owner const&)=delete;
    bool take_endpoint(Endpoint& out) noexcept {
        out={};if(!weaponhand::current_gt(nullptr)||!endpoint_count_)return false;
        out=endpoints_[0];for(unsigned i=1;i<endpoint_count_;++i)endpoints_[i-1]=endpoints_[i];
        endpoints_[--endpoint_count_]={};return true;
    }
    void discard_endpoints() noexcept {if(weaponhand::current_gt(nullptr)){endpoints_={};endpoint_count_=0;}}
    std::uint64_t endpoint_drops()const noexcept{return endpoint_drops_;}
    Stage stage()const noexcept{return stage_;}
    bool pending()const noexcept{return stage_!=Stage::idle;}
    bool lifecycle_pending()const noexcept{return stage_!=Stage::idle&&stage_!=Stage::parked;}
    enum class VisualPart {none,opening,closing,carrier,carrier_seating};
    struct VisualState {VisualPart part{};std::uint64_t serial{},item{};};
    enum class GuidePart {none,opening,shoulder,entry,closing};
    struct GuideState {GuidePart part{};bool entered{};};
    bool guide_state(hand::Frame const& f,GuideState& out) const noexcept {
        out={};if(!weaponhand::current_gt(nullptr))return false;
        if(stage_==Stage::idle){out.part=GuidePart::opening;return true;}
        const auto current=inv::identity(f);
        if(!slot_||(!(context_==current)&&!(stage_==Stage::parked&&inv::same_instance_across_hold(context_,current))))return false;
        auto* tx=slot_->core(context_);if(!tx)return false;
        if(stage_==Stage::prepared){out.part=GuidePart::opening;return true;}
        if(stage_==Stage::parked){
            if(plan_.direct_exchange){out.part=GuidePart::entry;return true;}
            out.part=tx->step_index()==0?GuidePart::opening:
                inspection_mode_||tx->step_index()>1?GuidePart::closing:GuidePart::shoulder;return true;
        }
        if(stage_!=Stage::mechanical)return false;
        if(tx->stage()==core::BoomshotStage::mechanism){out.part=tx->step_index()==0?GuidePart::opening:GuidePart::closing;return true;}
        if(tx->stage()==core::BoomshotStage::take_ammunition){out.part=removing_?GuidePart::opening:GuidePart::shoulder;return true;}
        if(tx->stage()==core::BoomshotStage::holding_ammunition&&item_){out={GuidePart::entry,entry_frame_&&insertion_.active()};return true;}
        return false;
    }
    bool visual_state(hand::Frame const& f,VisualState& out) const noexcept {
        out={};if(!weaponhand::current_gt(nullptr)||!(context_==inv::identity(f))||!slot_)return false;
        auto* tx=slot_->core(context_);if(!tx)return false;
        if(stage_==Stage::prepared){if(plan_.direct_exchange)return false;out={VisualPart::opening,0,0};return true;}
        if(stage_!=Stage::mechanical)return false;
        if(removing_&&removal_item_){out={VisualPart::carrier,serial_,removal_item_};return true;}
        if(tx->stage()==core::BoomshotStage::mechanism){
            out={tx->step_index()==0?VisualPart::opening:VisualPart::closing,serial_,0};return true;
        }
        if(tx->stage()==core::BoomshotStage::holding_ammunition&&item_){
            out={entry_frame_&&insertion_.active()?VisualPart::carrier_seating:VisualPart::carrier,serial_,item_};return true;
        }
        return false;
    }
    bool selected(std::uint64_t weapon)const noexcept{return pending()&&context_.weapon.address==weapon;}
    bool removing_old_carrier()const noexcept{return removing_;}
    bool blocks_fire(hand::Frame const& f)const noexcept {
        return pending()&&boomshot_sessions::same_actor(context_,inv::identity(f));
    }
    std::uint64_t original_serial()const noexcept{return serial_;}
    core::BoomshotPlan const& plan()const noexcept{return plan_;}
    bool retire_parked() noexcept {
        if(busy_||!weaponhand::current_gt(nullptr)||stage_!=Stage::parked||!slot_||!exit_confirmed_||
           native::retains_serial(serial_)||boomshot_clip::serial||boomshot_carrier_original::owner.active()||
           boomshot_inspection_clip::owner.active()||!lancerreload_runtime::weapon_retired(context_.weapon))return false;
        Busy guard(busy_);
        if(!slot_->retire_source(context_))return false;
        slot_=nullptr;context_={};original_={};plan_={};serial_=item_=0;dispatched_=false;
        observer_={};writer_={};opening_={};original_request_={};completion_={};reward_={};
        core_revision_base_=original_revision_base_=entry_frame_=0;entry_event_={};insertion_.release();insertion_armed_=false;
        suspended_native_={};suspended_known_=false;inventory_sent_=exit_sent_=exit_confirmed_=false;
        saved_progress_=0;saved_progress_known_=restoring_debt_=restore_grant_started_=restore_seek_sent_=false;
        restore_intent_=restore_seek_frame_=saved_curve_content_=writer_token_floor_=0;writer_actor_={};restore_purpose_={};gun_only_=inspection_mode_=false;inspection_exit_={};gun_serial_=0;stage_=Stage::idle;return true;
    }
    bool prepare(hand::Frame const& f,hand::Decision const& d,core::BoomshotMechanismGrant& grant,bool insertion_ready=true) noexcept {
        const auto rejected=[&](unsigned line)noexcept{static unsigned last{},rows{};if(log_file&&line!=last&&rows++<16){fwprintf(log_file,L"boomshot_prepare_reject line=%u frame=%llu lease=%llu\n",line,f.key.frame,d.lease.id);fflush(log_file);last=line;}return false;};
        if(stage_==Stage::parked)retire_parked();
        if(!grant.begin_allowed(f,d,core::BoomshotContact::opening,weaponhand::clock_ns(nullptr))||busy_||pending()||!weaponhand::current_gt(nullptr)||!boomshot_inspection_clip::fire_neutral(f)||
           boomshot_clip::serial||boomshot_inspection_clip::owner.active()||boomshot_carrier_original::owner.active()||native::lease.acquired||
           !grant.begin_allowed(f,d,core::BoomshotContact::opening,weaponhand::clock_ns(nullptr)))return rejected(__LINE__);
        Busy guard(busy_);boomshot_original::Source current_source{};boomshot_core_source::Snapshot current_input{};
        if(!original_closed(f,current_source)||!boomshot_core_source::read(f,current_input)||
           !(current_source.counts==current_input.counts)||current_input.counts.used<0||current_input.counts.used>1||
           !grant.begin_allowed(f,d,core::BoomshotContact::opening,weaponhand::clock_ns(nullptr)))return rejected(__LINE__);
        const bool inspecting=current_input.counts.used==0;
        if(current_input.counts.used==1&&!current_input.counts.infinite&&!current_input.counts.reserve)return rejected(__LINE__);
        if(!inspecting&&!insertion_ready)return rejected(__LINE__);
        auto* slot=boomshot_sessions::prepare(current_input.context);
        if(!slot&&reclaim_dead_idle_slots())slot=boomshot_sessions::prepare(current_input.context);
        if(slot&&!slot->core(current_input.context)&&!slot->rebind_idle_source(slot->first,current_input.context))return rejected(__LINE__);
        auto* tx=slot?slot->core(current_input.context):nullptr;auto* stream=slot?slot->events(current_input.context):nullptr;
        if(!tx||!stream||(tx->stage()!=core::BoomshotStage::unbound&&tx->stage()!=core::BoomshotStage::ready))return rejected(__LINE__);
        const auto revision=tx->inventory().revision;if(revision==UINT64_MAX||!tx->reset())return rejected(__LINE__);
        const core::BoomshotReloadConfig config{slot->identity(),slot->generation,current_source.carrier.address,
            core::BoomshotAmmoKind::verified_carrier,1,250000000,true,true};
        const core::BoomshotInventory current_counts{config.identity,revision+1,unsigned(1-current_input.counts.used),unsigned(current_input.counts.reserve),current_input.counts.infinite};
        const core::BoomshotInspectionConfig inspection_config{config.identity,config.config_id,1,100000000,true,true};
        if(!(inspecting?tx->bind_inspection(inspection_config,current_counts):tx->bind_reload(config,current_counts))||
           !stream->bind(config.identity,tx->binding_token()))return rejected(__LINE__);
        core::BoomshotOwnerEvents::InputEvent event{};core::BoomshotFrame frame{};
        if(boomshot_core_source::input(f,*tx,*stream,event,frame,0,nullptr,&current_input)!=boomshot_core_source::Result::ready||
           tx->observe(frame)!=core::BoomshotEvent::motion_reset)return rejected(__LINE__);
        inv::ObservedInventory next_observer{};inv::Inventory opening{};
        if(!next_observer.sample(f,opening)||opening.capacity!=1||opening.loaded!=current_counts.loaded||opening.reserve!=unsigned(current_input.counts.reserve)||
           opening.infinite_reserve!=current_input.counts.infinite)return rejected(__LINE__);
        native::RewardReceipt eligibility{};if(!inspecting&&!native::capture_reward_eligibility(opening,eligibility))return rejected(__LINE__);
        slot_=slot;context_=current_input.context;original_=current_source;observer_=next_observer;writer_={};opening_=opening;reward_=eligibility;
        core_revision_base_=tx->inventory().revision;original_revision_base_=opening.revision;original_request_={};inventory_sent_=false;
        completion_={};exit_sent_=exit_confirmed_=false;
        saved_progress_=0;saved_progress_known_=true;restoring_debt_=restore_grant_started_=restore_seek_sent_=false;
        restore_intent_=restore_seek_frame_=saved_curve_content_=0;restore_purpose_={};gun_only_=false;gun_serial_=0;
        inspection_mode_=inspecting;inspection_exit_={};removing_=old_pulled_=false;removal_item_=0;
        if(!(writer_actor_==slot->identity())){writer_actor_=slot->identity();writer_token_floor_=0;}
        plan_={};plan_.identity=config.identity;plan_.config_id=config.config_id;plan_.plan_id=tx->binding_token();
        plan_.condition=core::BoomshotCondition::empty;
        plan_.current_instance_verified=plan_.original_order_verified=plan_.quantity_verified=true;
        // Empty reload is a direct drum exchange: no barrel/handle phase and
        // no post-seat closing stroke. The original detach/attach calls and
        // the one original reload serial still authenticate the operation.
        plan_.direct_exchange=!inspecting;plan_.count=1;
        plan_.steps[0]={core::BoomshotStepKind::insert_ammunition,core::BoomshotPart::feed_port,1,slot->generation,1,0,false};
        if(inspecting){
            // Zero-target barrel inspection retains its separate historical
            // open/close plan and never gains a drum/inventory transfer.
            plan_.condition=core::BoomshotCondition::inspection;plan_.direct_exchange=false;plan_.count=2;
            plan_.steps[0]={core::BoomshotStepKind::move_mechanism,core::BoomshotPart::barrel,1,slot->generation,0,0,false};
            plan_.steps[1]={core::BoomshotStepKind::move_mechanism,core::BoomshotPart::barrel,2,slot->generation,0,0,true};
        }
        stage_=Stage::prepared;return true;
    }
    #include "weapon_boomshot_inspection_flow.inl"
    bool cancel_unstarted() noexcept {
        if(busy_||!weaponhand::current_gt(nullptr)||stage_!=Stage::prepared||dispatched_||serial_)return false;
        auto* tx=transaction();if(!tx||(tx->stage()!=core::BoomshotStage::ready&&tx->stage()!=core::BoomshotStage::unbound))return false;
        slot_=nullptr;context_={};original_={};plan_={};inspection_mode_=false;inspection_exit_={};stage_=Stage::idle;return true;
    }
    core::BoomshotEvent start(hand::Frame const& f,hand::Decision const& d,core::BoomshotMechanismGrant& grant) noexcept {
        if(inspection_mode_)return start_inspection(f,d,grant);
        if(busy_||stage_!=Stage::prepared||!weaponhand::current_gt(nullptr)||!(context_==inv::identity(f))||
           !boomshot_inspection_clip::fire_neutral(f))return core::BoomshotEvent::rejected;
        Busy guard(busy_);auto* tx=transaction();auto* stream=events();if(!tx||!stream)return core::BoomshotEvent::rejected;
        boomshot_original::Source current_source{};
        if(!original_closed(f,current_source)||!hand::same(current_source.parts.component,original_.parts.component)||
           !hand::same(current_source.parts.asset,original_.parts.asset)||!hand::same(current_source.carrier,original_.carrier)||
           !(current_source.counts==original_.counts))return core::BoomshotEvent::rejected;
        core::BoomshotOwnerEvents::InputEvent event{};core::BoomshotFrame frame{};
        if(boomshot_core_source::input(f,*tx,*stream,event,frame)!=boomshot_core_source::Result::ready)return core::BoomshotEvent::rejected;
        if(!tx->reload_input_current(frame))return tx->observe(frame);
        const bool exchange=plan_.direct_exchange;
        const auto purpose=core::BoomshotContact::opening;
        if(!grant.begin_allowed(f,d,purpose,weaponhand::clock_ns(nullptr))){
            tx->observe(frame);return core::BoomshotEvent::rejected;
        }
        // Persist ownership BEFORE original ForceReload, including callbacks.
        // Its pre-dispatch original GAME time anchors the core clock; post-call
        // validation never buys another four seconds or renews tracking.
        stage_=Stage::starting;dispatched_=false;
        const auto accepted=native::begin_for_part(f,serial_,core::boomshot_contact_lease(purpose),nullptr,&dispatched_);
        if(accepted!=native::BeginResult::acquired||!serial_||!native::retains_serial(serial_)||
           !(native::lease.identity==context_)||!native::owned_state()){
            if(dispatched_||serial_){return fail_event(__LINE__);}
            stage_=Stage::prepared;return core::BoomshotEvent::rejected;
        }
        core::BoomshotReloadStart request{};request.binding=tx->binding_token();request.original_reload_serial=serial_;request.plan=plan_;
        request.original_reload_accepted=request.automatic_completion_suppressed=true;
        const auto result=tx->begin_reload(frame,request,f.press_token,core::ReloadIntentSource::mechanism_open);
        if(result!=core::BoomshotEvent::started){fail();return result;}
        stage_=Stage::mechanical;
        if(exchange){
            if(log_file){fwprintf(log_file,L"boomshot_reload_started serial=%llu game_tick_us=%lld direct=1\n",serial_,frame.stamp.game_tick);fflush(log_file);}
            removing_=true;old_pulled_=false;removal_item_=0;removal_origin_=hand_offset(f);
            if(!grant.started(f,d,purpose,weaponhand::clock_ns(nullptr))){return fail_event(__LINE__);}
        }
        return result;
    }
    static bool shoulder(hand::Frame const& f) noexcept {
        const gears_vr::Pose body{f.stereo.head.position,f.reference.orientation};const auto& left=f.controllers.hands[0];
        return gears_vr::controller_pose_usable(left.grip,left.grip_flags,left.grip_active)&&
            core::body_slot(nativeequipment::calibration,body,left.grip.position,0)==core::BodySlot::shoulder_ammo;
    }
    core::BoomshotContact contact(hand::Frame const& f) noexcept {
        if(!weaponhand::current_gt(nullptr))return core::BoomshotContact::none;
        const auto current=inv::identity(f);
        if(stage_==Stage::parked&&(current==context_||inv::same_instance_across_hold(context_,current))){
            auto* tx=transaction();if(!tx)return core::BoomshotContact::none;
            if(plan_.direct_exchange)return removing_?core::BoomshotContact::opening:core::BoomshotContact::take_carrier;
            return tx->step_index()==0?core::BoomshotContact::opening:inspection_mode_?core::BoomshotContact::closing:
                tx->step_index()==1?core::BoomshotContact::take_carrier:core::BoomshotContact::closing;
        }
        if(!(context_==current))return core::BoomshotContact::none;
        if(stage_==Stage::restoring)return restore_purpose_;
        auto* tx=transaction();if(!tx)return core::BoomshotContact::none;
        if(stage_==Stage::prepared||removing_)return core::BoomshotContact::opening;
        if(stage_!=Stage::mechanical)return core::BoomshotContact::none;
        if(tx->stage()==core::BoomshotStage::mechanism)return tx->step_index()==0?core::BoomshotContact::opening:core::BoomshotContact::closing;
        if(tx->stage()==core::BoomshotStage::take_ammunition||tx->stage()==core::BoomshotStage::holding_ammunition||tx->stage()==core::BoomshotStage::inventory_commit)return core::BoomshotContact::take_carrier;
        return core::BoomshotContact::none;
    }
    // Pre-q1 recovery only. A loaded q1 must use gun-only mechanical recovery;
    // never call ForceReload for it or invent spare ammunition to pass CanReload.
    core::BoomshotEvent resume_empty(hand::Frame const& f,hand::Decision const& d,core::BoomshotMechanismGrant& grant) noexcept {
        if(busy_||!weaponhand::current_gt(nullptr)||stage_!=Stage::parked||!slot_||!suspended_known_||!saved_progress_known_||
           native::lease.acquired||boomshot_clip::serial||boomshot_inspection_clip::owner.active()||boomshot_carrier_original::owner.active()||
           !boomshot_inspection_clip::fire_neutral(f))return core::BoomshotEvent::rejected;
        Busy guard(busy_);auto* tx=transaction();auto* stream=events();const auto next=inv::identity(f);
        if(!tx||!stream||tx->stage()!=core::BoomshotStage::interrupted||tx->step_index()>1||tx->inventory().loaded||
           (!(next==context_)&&!inv::same_instance_across_hold(context_,next)))return core::BoomshotEvent::rejected;
        const auto purpose=plan_.direct_exchange?(removing_?core::BoomshotContact::opening:core::BoomshotContact::take_carrier):
            tx->step_index()==0?core::BoomshotContact::opening:core::BoomshotContact::take_carrier;
        if((purpose==core::BoomshotContact::take_carrier&&!shoulder(f))||
           !grant.begin_allowed(f,d,purpose,weaponhand::clock_ns(nullptr)))return core::BoomshotEvent::rejected;
        boomshot_original::Source actual{};inv::ObservedInventory rebound=observer_;inv::Inventory observed_inventory{};core::BoomshotInventory mirrored{};
        if(!boomshot_original::prepare(f,actual)||!hand::same(actual.parts.component,original_.parts.component)||
           !hand::same(actual.parts.asset,original_.parts.asset)||!hand::same(actual.carrier,original_.carrier)||
           !hand::same(actual.carrier_component,original_.carrier_component)||
           !observer_.sample_owned(context_,observed_inventory)||!mirror(observed_inventory,mirrored)||
           mirrored.revision!=tx->inventory().revision||mirrored.loaded!=tx->inventory().loaded||
           mirrored.reserve!=tx->inventory().reserve||mirrored.infinite_reserve!=tx->inventory().infinite_reserve||
           !rebound.rebind_hold(f,context_,observed_inventory))return core::BoomshotEvent::rejected;
        core::BoomshotOwnerEvents::InputEvent event{};core::BoomshotFrame frame{};
        if(boomshot_core_source::input(f,*tx,*stream,event,frame)!=boomshot_core_source::Result::ready||
           !tx->resume_input_current(frame,f.press_token,serial_,tx->step_index(),false)||
           !grant.begin_allowed(f,d,purpose,weaponhand::clock_ns(nullptr)))return core::BoomshotEvent::rejected;
        const auto old=context_;const auto old_observer=observer_;const auto old_opening=opening_;
        if(!slot_->rebind_source(old,next))return core::BoomshotEvent::rejected;
        if(!grant.started(f,d,purpose,weaponhand::clock_ns(nullptr))){
            slot_->rebind_source(next,old);return core::BoomshotEvent::rejected;
        }
        context_=next;observer_=rebound;opening_.identity=next;stage_=Stage::restoring;restoring_debt_=true;
        restore_purpose_=purpose;restore_intent_=f.press_token;restore_grant_started_=true;restore_seek_sent_=false;restore_seek_frame_=0;
        // The prior exit belongs to the parked lease. A new dispatch owes its
        // own exit even when post-call validation fails before adoption.
        const auto old_serial=serial_;dispatched_=false;exit_sent_=exit_confirmed_=false;
        const auto accepted=native::begin_for_part(f,serial_,core::boomshot_contact_lease(purpose),&suspended_native_,&dispatched_);
        if(accepted!=native::BeginResult::acquired||serial_!=old_serial||!native::retains_serial(serial_)||
           !(native::lease.identity==next)||!native::owned_state()||native::lease.reward_start!=suspended_native_.reward_start||
           native::lease.accepted_begin_ms!=suspended_native_.accepted_begin_ms||native::lease.perfect_end_ms!=suspended_native_.perfect_end_ms){
            if(!dispatched_&&!serial_&&slot_->rebind_source(next,old)){
                context_=old;observer_=old_observer;opening_=old_opening;serial_=old_serial;dispatched_=true;exit_sent_=exit_confirmed_=true;restoring_debt_=false;stage_=Stage::parked;
                return core::BoomshotEvent::rejected;
            }
            return fail_event(__LINE__);
        }
        // The existing ledger supplies subsequent request tokens. Rebinding
        // this writer cannot replay a prior token; the independent floor stays.
        writer_={};exit_sent_=exit_confirmed_=false;item_=0;insertion_.release();entry_event_={};entry_frame_=0;
        return core::BoomshotEvent::started;
    }
    core::BoomshotEvent service_resume(hand::Frame const& f,hand::Decision const& d,core::BoomshotMechanismGrant& grant) noexcept {
        if(busy_||!weaponhand::current_gt(nullptr)||stage_!=Stage::restoring||!(context_==inv::identity(f))||
           !boomshot_inspection_clip::fire_neutral(f))return core::BoomshotEvent::rejected;
        Busy guard(busy_);auto* tx=transaction();auto* stream=events();
        if(!tx||!stream||tx->stage()!=core::BoomshotStage::interrupted||
           (gun_only_?!gun_clip_owned():!native::owns(f,serial_)))return core::BoomshotEvent::rejected;
        if(!restore_grant_started_){
            if(!grant.started(f,d,restore_purpose_,weaponhand::clock_ns(nullptr)))return core::BoomshotEvent::none;
            restore_grant_started_=true;
        }
        if(!grant.retained(f,d,restore_purpose_,weaponhand::clock_ns(nullptr)))return core::BoomshotEvent::rejected;
        if(plan_.direct_exchange){
            // No animation seek is owed by a drum-only exchange. Restore its
            // original inventory transaction using the current held input.
            core::BoomshotOwnerEvents::InputEvent event{};core::BoomshotFrame frame{};
            if(boomshot_core_source::input(f,*tx,*stream,event,frame,serial_)!=boomshot_core_source::Result::ready)
                return core::BoomshotEvent::none;
            if(!tx->resume(frame,restore_intent_,serial_,tx->step_index(),false))return core::BoomshotEvent::none;
            inventory_sent_=false;original_request_={};completion_={};removal_item_=0;
            if(old_pulled_)removing_=false;
            stage_=Stage::mechanical;restoring_debt_=false;return core::BoomshotEvent::restored;
        }
        std::uint64_t content{};
        if(gun_only_){
            const auto serviced=boomshot_inspection_clip::owner.service(f);
            if(serviced==boomshot_inspection_clip::Result::pending)return core::BoomshotEvent::none;
            if(serviced!=boomshot_inspection_clip::Result::ready){return fail_event(__LINE__);}
            content=boomshot_inspection_clip::backend.curve.content;
        }else{
            if(!boomshot_clip::serial){
                const auto begun=boomshot_clip::start(f,serial_);
                if(begun==boomshot_clip::Result::rejected||begun==boomshot_clip::Result::recovery_required){return fail_event(__LINE__);}
            }
            const auto serviced=boomshot_clip::service(f);
            if(serviced==boomshot_clip::Result::pending)return core::BoomshotEvent::none;
            if(serviced!=boomshot_clip::Result::ready){return fail_event(__LINE__);}
            content=boomshot_clip::curve.content;
        }
        if(saved_curve_content_&&content!=saved_curve_content_){return fail_event(__LINE__);}
        // The original animation's closed bone may not have evaluated in the
        // dispatch frame. Await that real observation before calibration/seek.
        if(!boomshot_barrel::prepare(f))return core::BoomshotEvent::none;
        if(!restore_seek_sent_){
            restore_seek_sent_=true;restore_seek_frame_=f.key.frame;
            const auto direction=tx->step_index()==0?motion::Direction::opening:motion::Direction::closing;
            if(!(gun_only_?boomshot_inspection_clip::owner.seek(f,saved_progress_,direction):
                 boomshot_clip::seek(f,saved_progress_,direction))){return fail_event(__LINE__);}
            return core::BoomshotEvent::none;
        }
        if(f.key.frame<=restore_seek_frame_)return core::BoomshotEvent::none;
        gears_vr::Pose root{},relative{};double scale{},progress{};
        if(!boomshot_barrel::bone(f,root,relative,scale)||!motion::progress(relative.position,progress)||
           std::abs(progress-saved_progress_)>.015)return core::BoomshotEvent::none;
        core::BoomshotOwnerEvents::InputEvent event{};core::BoomshotFrame frame{};
        if(boomshot_core_source::input(f,*tx,*stream,event,frame,gun_only_?0:serial_)!=boomshot_core_source::Result::ready)return core::BoomshotEvent::none;
        if(!grant.retained(f,d,restore_purpose_,weaponhand::clock_ns(nullptr))||
           !tx->resume(frame,restore_intent_,inspection_mode_?0:serial_,tx->step_index(),false)){return fail_event(__LINE__);}
        if(!gun_only_){inventory_sent_=false;original_request_={};}completion_={};
        stage_=Stage::mechanical;restoring_debt_=false;return core::BoomshotEvent::restored;
    }
    // q1 already belongs to this ledger. Reuse the original gun-only clip;
    // no ForceReload, ammo notification or inventory writer may run here.
    core::BoomshotEvent resume_loaded(hand::Frame const& f,hand::Decision const& d,core::BoomshotMechanismGrant& grant) noexcept {
        if(busy_||!weaponhand::current_gt(nullptr)||stage_!=Stage::parked||!slot_||!exit_confirmed_||!suspended_known_||!saved_progress_known_||
           !inventory_sent_||native::lease.acquired||boomshot_clip::serial||boomshot_inspection_clip::owner.active()||
           boomshot_carrier_original::owner.active()||!boomshot_inspection_clip::fire_neutral(f))return core::BoomshotEvent::rejected;
        Busy guard(busy_);auto* tx=transaction();auto* stream=events();const auto next=inv::identity(f);
        if(!tx||!stream||tx->stage()!=core::BoomshotStage::interrupted||tx->step_index()!=2||tx->inventory().loaded!=1||
           (!(next==context_)&&!inv::same_instance_across_hold(context_,next))||
           !grant.begin_allowed(f,d,core::BoomshotContact::closing,weaponhand::clock_ns(nullptr)))return core::BoomshotEvent::rejected;
        boomshot_original::Source actual{};inv::ObservedInventory rebound=observer_;inv::Inventory observed_inventory{};core::BoomshotInventory mirrored{};
        if(!boomshot_original::prepare(f,actual)||!hand::same(actual.parts.component,original_.parts.component)||
           !hand::same(actual.parts.asset,original_.parts.asset)||!hand::same(actual.carrier,original_.carrier)||
           !hand::same(actual.carrier_component,original_.carrier_component)||
           !observer_.sample_owned(context_,observed_inventory)||!mirror(observed_inventory,mirrored)||
           mirrored.revision!=tx->inventory().revision||mirrored.loaded!=tx->inventory().loaded||
           mirrored.reserve!=tx->inventory().reserve||mirrored.infinite_reserve!=tx->inventory().infinite_reserve||
           !rebound.rebind_hold(f,context_,observed_inventory))return core::BoomshotEvent::rejected;
        core::BoomshotOwnerEvents::InputEvent event{};core::BoomshotFrame frame{};
        if(boomshot_core_source::input(f,*tx,*stream,event,frame)!=boomshot_core_source::Result::ready||
           !tx->resume_input_current(frame,f.press_token,serial_,2,false)||
           !grant.begin_allowed(f,d,core::BoomshotContact::closing,weaponhand::clock_ns(nullptr)))return core::BoomshotEvent::rejected;
        const auto candidate=boomshot_inspection_clip::owner.available_serial();
        const auto old=context_;const auto old_observer=observer_;const auto old_opening=opening_;
        if(!candidate||!slot_->rebind_source(old,next))return core::BoomshotEvent::rejected;
        if(!grant.started(f,d,core::BoomshotContact::closing,weaponhand::clock_ns(nullptr))){
            slot_->rebind_source(next,old);return core::BoomshotEvent::rejected;
        }
        context_=next;observer_=rebound;opening_.identity=next;stage_=Stage::restoring;restoring_debt_=true;
        restore_purpose_=core::BoomshotContact::closing;restore_intent_=f.press_token;restore_grant_started_=true;restore_seek_sent_=false;restore_seek_frame_=0;
        gun_only_=true;gun_serial_=candidate; // Durable BEFORE PlayWeaponAnim.
        const auto begun=boomshot_inspection_clip::owner.begin(f,candidate);
        if(begun==boomshot_inspection_clip::Result::rejected&&!boomshot_inspection_clip::owner.active()&&slot_->rebind_source(next,old)){
            context_=old;observer_=old_observer;opening_=old_opening;gun_only_=false;gun_serial_=0;restoring_debt_=false;stage_=Stage::parked;
            return core::BoomshotEvent::rejected;
        }
        if(begun==boomshot_inspection_clip::Result::rejected||begun==boomshot_inspection_clip::Result::recovery_required||!gun_clip_owned()){
            return fail_event(__LINE__);
        }
        return core::BoomshotEvent::started;
    }
    core::BoomshotEvent remove_old(hand::Frame const& f,hand::Decision const& d,core::BoomshotMechanismGrant& grant) noexcept {
        if(busy_||!removing_||stage_!=Stage::mechanical||!(context_==inv::identity(f))||!weaponhand::current_gt(nullptr))
            return core::BoomshotEvent::rejected;
        Busy guard(busy_);auto* tx=transaction();
        if(!tx||!f.left_tracked||!f.left_grip_known||!hand::frame_active(f,weaponhand::clock_ns(nullptr))||
           !native::retains_serial(serial_)||!native::owned_state())return core::BoomshotEvent::none;
        if(!f.left_grip_down){
            if(!removal_item_)return core::BoomshotEvent::none;
            const auto p=hand_offset(f);
            if(boomshot_carrier_original::owner.holding(f,serial_,removal_item_))
                old_pulled_=old_pulled_||std::hypot(std::hypot(p.x-removal_origin_.x,p.y-removal_origin_.y),p.z-removal_origin_.z)>=.06;
            if(!boomshot_carrier_original::owner.detach(!old_pulled_))return core::BoomshotEvent::none;
            removal_item_=0;
            if(old_pulled_){
                removing_=false;
                if(log_file){fwprintf(log_file,L"boomshot_old_carrier_removed serial=%llu sequence=%llu pulled=1 released=1\n",serial_,f.controllers.sequence);fflush(log_file);}
            }
            return core::BoomshotEvent::none;
        }
        if(!removal_item_){
            if(next_item_serial==UINT64_MAX||
               (!grant.retained(f,d,core::BoomshotContact::opening,weaponhand::clock_ns(nullptr))&&
                !grant.started(f,d,core::BoomshotContact::opening,weaponhand::clock_ns(nullptr))))return core::BoomshotEvent::none;
            boomshot_original::Source source=original_;
            if(!boomshot_original::carrier_current(f,source))return core::BoomshotEvent::none;
            removal_item_=next_item_serial++;old_pulled_=false;
            if(!boomshot_carrier_original::owner.begin(f,serial_,removal_item_,source)){return fail_event(__LINE__);}
        }
        if(!grant.retained(f,d,core::BoomshotContact::opening,weaponhand::clock_ns(nullptr)))return core::BoomshotEvent::none;
        const auto visual=boomshot_carrier_original::owner.service(f,serial_,removal_item_);
        if(visual==boomshot_carrier_original::RawResult::unavailable){return fail_event(__LINE__);}
        if(visual==boomshot_carrier_original::RawResult::ready){
            const auto p=hand_offset(f);
            old_pulled_=old_pulled_||std::hypot(std::hypot(p.x-removal_origin_.x,p.y-removal_origin_.y),p.z-removal_origin_.z)>=.06;
        }
        return core::BoomshotEvent::none;
    }
    core::BoomshotEvent take(hand::Frame const& f,hand::Decision const& d,core::BoomshotMechanismGrant& grant) noexcept {
        if(busy_||stage_!=Stage::mechanical||!weaponhand::current_gt(nullptr)||!(context_==inv::identity(f))||
           !shoulder(f)||!boomshot_inspection_clip::fire_neutral(f)||boomshot_carrier_original::owner.active()||
           !grant.begin_allowed(f,d,core::BoomshotContact::take_carrier,weaponhand::clock_ns(nullptr)))return core::BoomshotEvent::rejected;
        Busy guard(busy_);auto* tx=transaction();auto* stream=events();
        if(!tx||!stream||tx->stage()!=core::BoomshotStage::take_ammunition||next_item_serial==UINT64_MAX)return core::BoomshotEvent::rejected;
        boomshot_original::Source current_source{};gears_vr::Pose root{},relative{};double scale{},opened{};
        if((!plan_.direct_exchange&&(!boomshot_barrel::bone(f,root,relative,scale)||!motion::progress(relative.position,opened)||opened<.985))||
           !boomshot_original::prepare(f,current_source)||!hand::same(current_source.carrier,original_.carrier)||
           !hand::same(current_source.carrier_component,original_.carrier_component))return core::BoomshotEvent::rejected;
        core::BoomshotOwnerEvents::InputEvent event{};core::BoomshotFrame frame{};
        const auto sampled=boomshot_core_source::input(f,*tx,*stream,event,frame,serial_);
        if(sampled==boomshot_core_source::Result::waiting_input)return core::BoomshotEvent::none;
        if(sampled!=boomshot_core_source::Result::ready)return core::BoomshotEvent::rejected;
        if(!grant.begin_allowed(f,d,core::BoomshotContact::take_carrier,weaponhand::clock_ns(nullptr)))return core::BoomshotEvent::rejected;
        const auto item=next_item_serial++;
        const core::BoomshotAmmoReceipt receipt{tx->binding_token(),serial_,plan_.plan_id,plan_.steps[insertion_step()].phase_id,item,
            original_.carrier.address,1,core::BoomshotAmmoKind::verified_carrier,core::BoomshotAmmoSource::left_shoulder,true};
        const auto result=tx->grab_left_shoulder(frame,receipt);if(result!=core::BoomshotEvent::grabbed)return result;
        item_=item;
        insertion_.release();entry_event_={};entry_frame_=0;insertion_armed_=false;
        if(!grant.started(f,d,core::BoomshotContact::take_carrier,weaponhand::clock_ns(nullptr))||
           !boomshot_carrier_original::owner.begin(f,serial_,item_,current_source)){
            return fail_event(__LINE__);
        }
        return result; // Local reservation only; original counts are unchanged.
    }
    core::BoomshotEvent carry(hand::Frame const& f,hand::Decision const& d,core::BoomshotMechanismGrant& grant) noexcept {
        if(busy_||stage_!=Stage::mechanical||!weaponhand::current_gt(nullptr)||!(context_==inv::identity(f)))return core::BoomshotEvent::rejected;
        Busy guard(busy_);auto* tx=transaction();auto* stream=events();
        if(!tx||!stream||!item_||tx->stage()!=core::BoomshotStage::holding_ammunition)return core::BoomshotEvent::rejected;
        core::BoomshotOwnerEvents::InputEvent event{};core::BoomshotFrame frame{};
        const auto sampled=boomshot_core_source::input(f,*tx,*stream,event,frame,serial_);
        if(sampled==boomshot_core_source::Result::waiting_input)return core::BoomshotEvent::none;
        if(sampled!=boomshot_core_source::Result::ready){boomshot_carrier_original::owner.detach();return fail_event(__LINE__);}
        if(f.left_grip_known&&f.left_tracked&&!f.left_grip_down){
            insertion_.release();entry_event_={};entry_frame_=0;
            if(!boomshot_carrier_original::owner.detach()){return fail_event(__LINE__);}
            const auto result=tx->release_ammunition(frame,item_);
            if(result==core::BoomshotEvent::released)item_=0;
            return result;
        }
        if(!grant.retained(f,d,core::BoomshotContact::take_carrier,weaponhand::clock_ns(nullptr))){
            boomshot_carrier_original::owner.detach();return fail_event(__LINE__);
        }
        const auto visual=boomshot_carrier_original::owner.service(f,serial_,item_);
        if(visual==boomshot_carrier_original::RawResult::pending)return tx->observe(frame);
        if(visual!=boomshot_carrier_original::RawResult::ready){return fail_event(__LINE__);}
        return tx->observe(frame);
    }
    core::BoomshotEvent insert(hand::Frame const& f,hand::Decision const& d,core::BoomshotMechanismGrant& grant,
        core::boomshot_insertion::Geometry const& geometry) noexcept {
        namespace physical=core::boomshot_insertion;
        if(busy_||stage_!=Stage::mechanical||!weaponhand::current_gt(nullptr)||!(context_==inv::identity(f))||
           !boomshot_inspection_clip::fire_neutral(f)||!physical::valid(geometry)||geometry.id!=plan_.steps[insertion_step()].geometry_id)return core::BoomshotEvent::rejected;
        Busy guard(busy_);auto* tx=transaction();auto* stream=events();
        if(!tx||!stream||!item_||tx->stage()!=core::BoomshotStage::holding_ammunition)return core::BoomshotEvent::rejected;
        if(!grant.retained(f,d,core::BoomshotContact::take_carrier,weaponhand::clock_ns(nullptr)))return core::BoomshotEvent::none;
        core::BoomshotOwnerEvents::InputEvent event{};core::BoomshotFrame frame{};boomshot_core_source::Snapshot source{};
        const auto sampled=boomshot_core_source::input(f,*tx,*stream,event,frame,serial_,&source);
        if(sampled==boomshot_core_source::Result::waiting_input)return core::BoomshotEvent::none;
        if(sampled==boomshot_core_source::Result::inventory_changed){return fail_event(__LINE__);}
        if(sampled!=boomshot_core_source::Result::ready)return core::BoomshotEvent::none;
        const auto visual=boomshot_carrier_original::owner.service(f,serial_,item_);
        if(visual==boomshot_carrier_original::RawResult::pending)return tx->observe(frame);
        if(visual!=boomshot_carrier_original::RawResult::ready)return core::BoomshotEvent::none;
        gears_vr::Pose raw{};const auto read=boomshot_carrier_original::owner.sample_raw(f,serial_,item_,source,raw,plan_.direct_exchange?&geometry:nullptr);
        if(read==boomshot_carrier_original::RawResult::unavailable)return core::BoomshotEvent::none;
        if(read==boomshot_carrier_original::RawResult::pending)return tx->observe(frame);
        if(log_file){static unsigned rows{};if(rows++<112){fwprintf(log_file,L"boomshot_insert_raw active=%u x=%.6g y=%.6g z=%.6g frame=%llu\n",unsigned(insertion_.active()),raw.position.x,raw.position.y,raw.position.z,f.key.frame);fflush(log_file);}}
        physical::Sample input{};
        input.stamp={{f.world.address,f.pawn.address,f.weapon.address,f.key.lifecycle},serial_,f.controllers.sequence,
            source.rig.native_reference.epoch,source.input.game_tick,f.controllers.sync_ns};
        input.raw_carrier_in_root_m=raw;input.frame=f.key.frame;input.lease=d.lease.id;input.press=f.press_token;
        input.item=item_;input.operation=serial_;input.geometry_id=geometry.id;
        input.grip_held=input.left_tracked=input.right_tracked=input.lease_current=true;
        if(!insertion_.active()){
            const auto observed_event=tx->observe(frame);
            if(observed_event!=core::BoomshotEvent::none&&observed_event!=core::BoomshotEvent::motion_reset)return observed_event;
            if(!tx->current_insertion_seed(event.stamp()))return observed_event;
            // The original take receipt was consumed by THIS owner for THIS
            // item. Arming its held stroke once does not manufacture a new
            // Native Decision or renew the original acquisition's timestamp.
            input.new_grant=!insertion_armed_;
            const bool armed=insertion_armed_?insertion_.resume(geometry,input,weaponhand::clock_ns(nullptr)):
                insertion_.begin(geometry,input,weaponhand::clock_ns(nullptr));
            if(armed){insertion_armed_=true;entry_event_={};entry_frame_=0;}
            return observed_event;
        }
        if(!tx->next_insertion_sample(event.stamp())){insertion_.release();tx->tracking_lost();return core::BoomshotEvent::motion_reset;}
        const auto result=insertion_.observe(input,weaponhand::clock_ns(nullptr));
        if(log_file){static unsigned rows{};if(rows++<48){fwprintf(log_file,L"boomshot_insert_sample result=%u x=%.6g y=%.6g z=%.6g frame=%llu\n",unsigned(result),raw.position.x,raw.position.y,raw.position.z,f.key.frame);fflush(log_file);}}
        if(result==physical::Result::interrupted||result==physical::Result::rejected){tx->tracking_lost();return core::BoomshotEvent::motion_reset;}
        // A release before the new drum is seated cancels the exchange. No
        // inventory debit occurs and the old empty state remains authoritative.
        if(f.left_tracked&&f.left_grip_known&&!f.left_grip_down){
            insertion_.release();entry_event_={};entry_frame_=0;
            if(!boomshot_carrier_original::owner.detach()){return fail_event(__LINE__);}
            const auto released=tx->release_ammunition(frame,item_);
            if(released==core::BoomshotEvent::released)item_=0;
            return released;
        }
        if(insertion_.began().frame==f.key.frame){entry_event_=event;entry_frame_=f.key.frame;}
        if(result!=physical::Result::complete)return tx->observe(frame);
        physical::Completion proof{};
        if(!insertion_.take_completion(proof)||!proof.valid()){return fail_event(__LINE__);}
        const auto& first=proof.began();const auto& last=proof.ended();
        if(!entry_event_.stamp().valid()||first.frame!=entry_frame_||first.stamp.sequence!=entry_event_.raw_sequence()||
           first.stamp.monotonic_ns!=entry_event_.stamp().monotonic_ns||last.frame!=f.key.frame||last.stamp.sequence!=event.raw_sequence()||
           last.stamp.monotonic_ns!=event.stamp().monotonic_ns||first.item!=item_||last.item!=item_||first.operation!=serial_||last.operation!=serial_||
           first.lease!=d.lease.id||last.lease!=d.lease.id||first.press!=f.press_token||last.press!=f.press_token||
           first.geometry_id!=geometry.id||last.geometry_id!=geometry.id||entry_event_.stamp().motion_epoch!=event.stamp().motion_epoch){return fail_event(__LINE__);}
        const auto& step=plan_.steps[insertion_step()];
        const core::BoomshotInsertionReceipt receipt{{tx->binding_token(),serial_,plan_.plan_id,step.phase_id,step.geometry_id,
            entry_event_.stamp().motion_epoch,entry_event_.stamp().sequence,entry_event_.stamp().monotonic_ns,true,true,true,false,true},item_,true,true,true};
        return tx->insert_ammunition(frame,receipt);
    }
    core::BoomshotEvent commit_inventory(hand::Frame const& f,hand::Decision const& d,core::BoomshotMechanismGrant& grant) noexcept {
        if(busy_||stage_!=Stage::mechanical||inventory_sent_||!weaponhand::current_gt(nullptr)||
           !(context_==inv::identity(f))||!boomshot_inspection_clip::fire_neutral(f))return core::BoomshotEvent::rejected;
        Busy guard(busy_);auto* tx=transaction();if(!tx||tx->stage()!=core::BoomshotStage::inventory_commit)return core::BoomshotEvent::rejected;
        inv::Inventory before{};core::BoomshotInventory core_before{};
        // Insertion is already certified and its one-shot request queued. Releasing Grip after seating is allowed.
        (void)d;(void)grant;
        if(!native::owns(f,serial_)||
           !boomshot_carrier_original::owner.holding(f,serial_,item_)||!observer_.sample(f,before)||!mirror(before,core_before)||
           core_before.revision!=tx->inventory().revision||core_before.loaded!=tx->inventory().loaded||
           core_before.reserve!=tx->inventory().reserve||core_before.infinite_reserve!=tx->inventory().infinite_reserve){return fail_event(__LINE__);}
        core::BoomshotInteraction::ReloadRequest request{};
        if(!tx->take_reload_request(request))return core::BoomshotEvent::none;
        // The core has already accepted the continuous insertion. This bridge
        // cannot construct one, rewind its source or request another q1.
        inventory_sent_=true;
        if(request.call!=core::ReloadTxnCall::insert||request.token<=writer_token_floor_||!(request.identity==writer_actor_)||
           !(request.identity==slot_->identity())||request.reload_serial!=serial_||
           request.item_serial!=item_||!(request.before.identity==slot_->identity())||!(request.expected.identity==slot_->identity())||
           request.before.revision!=core_before.revision||request.before.capacity!=1||
           request.before.loaded||request.before.reserve!=before.reserve||request.before.infinite_reserve!=before.infinite_reserve||
           request.expected.capacity!=1||request.expected.loaded!=1||request.expected.infinite_reserve!=before.infinite_reserve||
           (before.infinite_reserve?request.expected.reserve!=before.reserve:
            (!before.reserve||request.expected.reserve!=before.reserve-1))){return fail_event(__LINE__);}
        writer_token_floor_=request.token;original_request_={context_,request.token,serial_,item_,request.call,before,
            {context_,0,1,1,request.expected.reserve,before.infinite_reserve},request.result};
        inv::Receipt receipt{};
        const auto result=writer_.commit(f,original_request_,observer_,native::owns,receipt);
        // A consumed request is never reissued, including a preflight refusal.
        // Unknown/partial effects retain both journals and the fire veto.
        if(result==inv::CommitResult::reconciliation_required){return fail_event(__LINE__);}
        if(!receipt.token){receipt.token=request.token;if(!observer_.sample_owned(context_,receipt.observed)){return fail_event(__LINE__);}}
        core::BoomshotOriginalReceipt original_receipt_value{};
        if(!original_receipt(receipt,original_receipt_value)){return fail_event(__LINE__);}
        const auto acknowledged=tx->acknowledge_inventory(original_receipt_value);
        if(result!=inv::CommitResult::committed||acknowledged!=core::BoomshotEvent::inventory_committed){return fail_event(__LINE__);}
        seating_source_=endpoint_source(f);
        if(!boomshot_carrier_original::owner.seat(f,serial_,item_,receipt.observed)){
            if(boomshot_carrier_original::owner.seat_pending()){stage_=Stage::seating;return acknowledged;}
            return fail_event(__LINE__);
        }
        const auto completed=seat_completed();
        if(completed==core::BoomshotEvent::request_completion||(!plan_.direct_exchange&&completed==core::BoomshotEvent::inventory_committed)){
            endpoint(EndpointKind::seated,seating_source_);seating_source_={};item_=0;
        }
        return completed;
    }
    bool service_seating() noexcept {
        if(busy_||!weaponhand::current_gt(nullptr)||stage_!=Stage::seating)return false;
        Busy guard(busy_);auto* tx=transaction();
        if(!tx||tx->stage()!=(plan_.direct_exchange?core::BoomshotStage::completion_commit:core::BoomshotStage::mechanism)||!boomshot_carrier_original::owner.reconcile_seat())return false;
        const auto completed=seat_completed();
        if(completed!=(plan_.direct_exchange?core::BoomshotEvent::request_completion:core::BoomshotEvent::inventory_committed))return false;
        endpoint(EndpointKind::seated,seating_source_);seating_source_={};item_=0;stage_=Stage::mechanical;return true;
    }
    // Only already certified physical close can enter this receipt-only path.
    // Tracking is not renewed or required to finish an original effect owed
    // before input/drain ended. Current original objects/counts/serial remain.
    core::BoomshotEvent complete_owned() noexcept {
        if(inspection_mode_)return complete_inspection_owned();
        if(busy_||!weaponhand::current_gt(nullptr)||
           (stage_!=Stage::mechanical&&stage_!=Stage::completing))return core::BoomshotEvent::rejected;
        Busy guard(busy_);auto* tx=transaction();if(!tx||tx->stage()!=core::BoomshotStage::completion_commit)return core::BoomshotEvent::rejected;
        if(stage_==Stage::mechanical){
            inv::Inventory before{};core::BoomshotInventory core_before{};
            if(item_||boomshot_carrier_original::owner.active()||!inventory_sent_||
               (gun_only_?(!exit_confirmed_||!gun_clip_owned()):(!native::retains_serial(serial_)||
                !(native::lease.identity==context_)||!native::owned_state()||(!plan_.direct_exchange&&!boomshot_clip::owned())))||
               !observer_.sample_owned(context_,before)||!mirror(before,core_before)||core_before.revision!=tx->inventory().revision||
               core_before.loaded!=1||core_before.loaded!=tx->inventory().loaded||core_before.reserve!=tx->inventory().reserve||
               core_before.infinite_reserve!=tx->inventory().infinite_reserve){return fail_event(__LINE__);}
            core::BoomshotInteraction::ReloadRequest request{};
            if(!tx->take_reload_request(request))return core::BoomshotEvent::none;
            if(request.call!=core::ReloadTxnCall::finish||request.reload_serial!=serial_||!(request.identity==slot_->identity())||
               request.before.revision!=core_before.revision||request.before.loaded!=1||request.expected.loaded!=1||
               request.before.reserve!=before.reserve||request.expected.reserve!=before.reserve||
               (request.result!=core::ReloadTxnResult::ordinary&&request.result!=core::ReloadTxnResult::perfect)){
                return fail_event(__LINE__);
            }
            completion_={context_,request.token,serial_,request.item_serial,request.call,before,before,request.result};
            stage_=Stage::completing;
        }
        // Original component was detached by visual-seat while this lease was
        // still owned. Never replay an uncertain original exit to regain fire.
        if(!gun_only_&&!exit_sent_){exit_sent_=true;exit_confirmed_=native::recover_serial(serial_);}
        if(!exit_confirmed_){return fail_event(__LINE__);}
        if(gun_only_){
            if(boomshot_inspection_clip::owner.active()&&(!gun_clip_matches()||!boomshot_inspection_clip::owner.drain()))return core::BoomshotEvent::none;
        }else if(!plan_.direct_exchange&&!boomshot_clip::hand_back_after_native_exit())return core::BoomshotEvent::none;
        if(!boomshot_barrel::retire())return core::BoomshotEvent::none;
        if(completion_.result==core::ReloadTxnResult::perfect&&!native::perfect_reward(completion_,opening_,observer_,reward_)){
            if(reward_.failed||(reward_.dispatched&&!reward_.call_confirmed)){return fail_event(__LINE__);}
            return core::BoomshotEvent::none;
        }
        inv::Inventory counts{};core::BoomshotOriginalReceipt receipt{};
        if(!observer_.sample_owned(context_,counts)||!inv::counts_equal(counts,completion_.expected)||
           !original_receipt({completion_.token,true,counts,completion_.result},receipt,true))return core::BoomshotEvent::none;
        receipt.result_applied=completion_.result;receipt.original_ready=true;
        const auto result=tx->acknowledge_completion(receipt);
        if(result!=core::BoomshotEvent::reload_completed){return fail_event(__LINE__);}
        if(log_file){fwprintf(log_file,L"boomshot_reload_finished serial=%llu outcome=%ls original_reward_applied=%u\n",serial_,completion_.result==core::ReloadTxnResult::perfect?L"perfect":L"ordinary",unsigned(reward_.applied));fflush(log_file);}
        slot_=nullptr;context_={};original_={};plan_={};serial_=item_=gun_serial_=0;dispatched_=gun_only_=false;stage_=Stage::idle;return result;
    }
    core::BoomshotEvent complete(hand::Frame const& f) noexcept {
        if(!(context_==inv::identity(f))||!nativeequipment::current(nullptr,f))return core::BoomshotEvent::rejected;
        return complete_owned();
    }
    // Existing original effects may be reconciled without tracking. This is
    // cleanup of THIS saved serial, never an action/rebind/current-hand permit.
    // Park only after original resources drain; keep the actor's mechanical
    // core, inventory, phase, original clock and fire veto for later recovery.
    bool drain() noexcept {
        if(busy_||!weaponhand::current_gt(nullptr))return false;
        if(stage_==Stage::idle||stage_==Stage::parked)return true;
        if(stage_==Stage::prepared&&!dispatched_&&!serial_)return cancel_unstarted();
        if(inspection_mode_)return drain_inspection_owned();
        if(stage_==Stage::seating&&!service_seating())return false;
        if(auto* tx=transaction();tx&&(stage_==Stage::completing||tx->stage()==core::BoomshotStage::completion_commit))
            return complete_owned()==core::BoomshotEvent::reload_completed;
        Busy guard(busy_);auto* tx=transaction();
        if(!tx||!serial_||!dispatched_)return false; // Unknown ForceReload outcome is not absence.
        core::BoomshotInteraction::ReloadRequest pending_request{};
        const bool pending_request_known=tx->dispatched_reload_request(pending_request);
        // An authenticated physical close already owes its completion/reward;
        // do not erase it by pretending that it was an interrupted open phase.
        if(pending_request_known&&pending_request.call==core::ReloadTxnCall::finish)return false;
        // A direct exchange has no barrel stroke to restore. Mark restoration
        // as already owed so a parked seat can resume without fabricated pose.
        if(plan_.direct_exchange&&!gun_only_)saved_progress_known_=true;
        else if(!restoring_debt_){
            double progress{};
            if(boomshot_barrel::snapshot_progress(context_,gun_only_?gun_serial_:serial_,progress,gun_only_)){
                saved_progress_=progress;saved_progress_known_=true;
            }
        }
        if(stage_!=Stage::recovery)fail();
        if(gun_only_){
            // Already committed q1 and an already confirmed native exit.
            // This branch owes only its own gun animation, never another fill.
            inv::Inventory counts{};core::BoomshotInventory mirrored{};
            if(pending_request_known||tx->step_index()!=2||tx->inventory().loaded!=1||item_||!exit_confirmed_||
               native::lease.acquired||boomshot_clip::serial||boomshot_carrier_original::owner.active()||
               !observer_.sample_owned(context_,counts)||!mirror(counts,mirrored)||
               mirrored.revision!=tx->inventory().revision||mirrored.loaded!=tx->inventory().loaded||
               mirrored.reserve!=tx->inventory().reserve||mirrored.infinite_reserve!=tx->inventory().infinite_reserve)return false;
            if(boomshot_inspection_clip::owner.active()&&(!gun_clip_matches()||!boomshot_inspection_clip::owner.drain()))return false;
            if(!boomshot_barrel::retire())return false;
            gun_only_=false;gun_serial_=0;stage_=Stage::parked;return true;
        }
        if(!exit_confirmed_&&(!native::retains_serial(serial_)||!(native::lease.identity==context_)||!native::owned_state()))return false;
        if(pending_request_known){
            if(exit_sent_||pending_request.call!=core::ReloadTxnCall::insert||pending_request.token!=original_request_.token||
               pending_request.reload_serial!=serial_||pending_request.item_serial!=original_request_.item_serial||
               !(original_request_.identity==context_)||!native::restore_partial_insert(original_request_,observer_))return false;
            inv::Inventory counts{};
            if(!observer_.sample_owned(context_,counts))return false;
            const bool inserted=inv::counts_equal(counts,original_request_.expected);
            if(!inserted&&!inv::counts_equal(counts,original_request_.before))return false;
            core::BoomshotOriginalReceipt receipt{};
            if(!original_receipt({pending_request.token,inserted,counts,core::ReloadTxnResult::none},receipt))return false;
            const auto acknowledged=tx->acknowledge_inventory(receipt);
            if(acknowledged!=(inserted?core::BoomshotEvent::inventory_committed:core::BoomshotEvent::rejected)||
               tx->dispatched_reload_request(pending_request))return false;
            if(inserted&&boomshot_carrier_original::owner.active()&&!boomshot_carrier_original::owner.seat_owned(serial_,item_,counts)){
                // A committed drum must still be attached before recovery can
                // release the original lease. Never fake a visual seat.
                if(!boomshot_carrier_original::owner.reconcile_seat())return false;
            }
        }
        if(boomshot_carrier_original::owner.active()&&!boomshot_carrier_original::owner.detach(!(plan_.direct_exchange&&old_pulled_)))return false;
        removal_item_=0;
        item_=0;insertion_.release();entry_event_={};entry_frame_=0;
        if(!exit_sent_){
            if(!native::retains_serial(serial_)||!(native::lease.identity==context_)||!native::owned_state())return false;
            // A failed restoration cannot replace the original saved clock or
            // serial with the newly rejected lease's values.
            if(!restoring_debt_||!suspended_known_){suspended_native_=native::lease;suspended_known_=true;}
            exit_sent_=true;exit_confirmed_=native::recover_serial(serial_);
        }
        // Direct exchange never acquired the original barrel clip. The
        // original reload action/serial still gets its normal exit receipt.
        if(!exit_confirmed_||!suspended_known_||(!plan_.direct_exchange&&!boomshot_clip::hand_back_after_native_exit())||
           !boomshot_barrel::retire())return false;
        stage_=Stage::parked;return true;
    }
    core::BoomshotEvent move(hand::Frame const& f,hand::Decision const& d,core::BoomshotMechanismGrant& grant) noexcept {
        if(busy_||stage_!=Stage::mechanical||!weaponhand::current_gt(nullptr)||!(context_==inv::identity(f)))return core::BoomshotEvent::rejected;
        Busy guard(busy_);auto* tx=transaction();auto* stream=events();if(!tx||!stream){return fail_event(__LINE__);}
        if(tx->stage()!=core::BoomshotStage::mechanism)return core::BoomshotEvent::none;
        core::BoomshotOwnerEvents::InputEvent event{};core::BoomshotFrame frame{};
        if(gun_only_&&!gun_clip_owned()){return fail_event(__LINE__);}
        const auto sampled=boomshot_core_source::input(f,*tx,*stream,event,frame,gun_only_?0:serial_);
        if(sampled==boomshot_core_source::Result::waiting_input)return core::BoomshotEvent::none;
        if(sampled==boomshot_core_source::Result::inventory_changed){return fail_event(__LINE__);}
        if(sampled!=boomshot_core_source::Result::ready){bridge_.release(*tx);tx->tracking_lost();return core::BoomshotEvent::motion_reset;}
        const auto direction=tx->step_index()==0?motion::Direction::opening:motion::Direction::closing;
        if(!gun_only_&&!boomshot_clip::serial){
            if(boomshot_clip::start(f,serial_)==boomshot_clip::Result::rejected)return core::BoomshotEvent::none;
        }
        if(gun_only_)saved_curve_content_=boomshot_inspection_clip::backend.curve.content;
        else if(boomshot_clip::owned())saved_curve_content_=boomshot_clip::curve.content;
        if(bridge_.active()){
            const auto result=bridge_.observe(f,d,grant,*tx,event,frame);
            if(result.physical==motion::Result::complete&&
               (result.event==core::BoomshotEvent::step_completed||result.event==core::BoomshotEvent::request_completion))
                endpoint(direction==motion::Direction::opening?EndpointKind::opened:EndpointKind::closed,endpoint_source(f));
            return result.event;
        }
        const auto observed_event=tx->observe(frame);
        if(observed_event!=core::BoomshotEvent::none&&observed_event!=core::BoomshotEvent::motion_reset)return observed_event;
        bridge_.begin(f,d,grant,*tx,event,frame,direction);return observed_event;
    }
    // Sole normal-GT action entry. Native commit only records a grant; engine
    // work happens here later with its ORIGINAL current source. One physical
    // input consumer per phase; original receipts may finish in this same GT.
    core::BoomshotEvent advance(hand::Frame const& f,hand::Decision const& d,core::BoomshotMechanismGrant& grant,
        core::boomshot_insertion::Geometry const& certified_geometry) noexcept {
        if(busy_||!weaponhand::current_gt(nullptr))return core::BoomshotEvent::rejected;
        const auto geometry_known=core::boomshot_insertion::valid(certified_geometry);
        if(geometry_known&&boomshot_carrier_original::owner.active())boomshot_carrier_original::owner.configure_relation(f,certified_geometry);
        switch(stage_){
        case Stage::idle:
            // A zero-target check needs only this original mechanism. A normal
            // reload still refuses when its insertion source is unverified.
            return prepare(f,d,grant,geometry_known)?core::BoomshotEvent::none:core::BoomshotEvent::rejected;
        case Stage::prepared:
            if((!inspection_mode_&&!geometry_known)||!boomshot_inspection_clip::fire_neutral(f)||
               !grant.begin_allowed(f,d,core::BoomshotContact::opening,weaponhand::clock_ns(nullptr))){
                cancel_unstarted();return core::BoomshotEvent::rejected;
            }
            return start(f,d,grant);
        case Stage::starting:return core::BoomshotEvent::none;
        case Stage::restoring:return service_resume(f,d,grant);
        case Stage::parked:{
            auto* tx=transaction();if(!tx)return core::BoomshotEvent::rejected;
            if(inspection_mode_)return resume_inspection(f,d,grant);
            if(tx->inventory().loaded)return resume_loaded(f,d,grant);
            return geometry_known?resume_empty(f,d,grant):core::BoomshotEvent::rejected;
        }
        case Stage::recovery:
            drain();return core::BoomshotEvent::interrupted;
        case Stage::seating:
            service_seating();return core::BoomshotEvent::none;
        case Stage::completing:return complete_owned();
        case Stage::mechanical:break;
        }
        auto* tx=transaction();if(!tx)return core::BoomshotEvent::rejected;
        switch(tx->stage()){
        case core::BoomshotStage::mechanism:{
            const auto result=move(f,d,grant);
            return result==core::BoomshotEvent::request_completion?complete_owned():result;
        }
        case core::BoomshotStage::take_ammunition:
            if(removing_)return remove_old(f,d,grant);
            // Keep the existing physical input stream observed while reaching
            // for supply. Entering the shoulder need not spend the acquisition
            // edge merely rebuilding a continuity baseline after the old drum.
            if(f.left_tracked&&f.left_grip_known&&!f.left_grip_down){
                auto* stream=events();core::BoomshotOwnerEvents::InputEvent event{};core::BoomshotFrame frame{};
                if(stream&&boomshot_core_source::input(f,*tx,*stream,event,frame,serial_)==boomshot_core_source::Result::ready)
                    return tx->observe(frame);
                return core::BoomshotEvent::none;
            }
            return geometry_known?take(f,d,grant):core::BoomshotEvent::rejected;
        case core::BoomshotStage::holding_ammunition:{
            // carry() and insert() both consume the typed original input.
            // A release owns detach/reservation return; a held item owns ONLY
            // insert(), whose existing service also waits for original attach.
            if(f.left_tracked&&f.left_grip_known&&!f.left_grip_down)return carry(f,d,grant);
            if(!geometry_known)return core::BoomshotEvent::rejected;
            auto geometry=certified_geometry;geometry.id=plan_.steps[insertion_step()].geometry_id;
            if(plan_.direct_exchange){
                // The real held drum must enter the socket corridor. Its last
                // few centimetres then snap to the original seated mesh.
                geometry.magnetic_snap=true;
                geometry.capture_radius_m=.035;geometry.retain_radius_m=.055;
                geometry.capture_angle_degrees=35;geometry.retain_angle_degrees=50;
                geometry.approach_margin_m=(std::min)(.025,geometry.travel_m*.15);
                geometry.seat_tolerance_m=(std::min)(.03,geometry.travel_m*.2);
            }
            const auto result=insert(f,d,grant,geometry);
            // Geometry sampling may use most of this frame's input budget.
            // Keep the queued request; execute its original transfer on the next fresh GT frame.
            return result;
        }
        case core::BoomshotStage::inventory_commit:return commit_inventory(f,d,grant);
        case core::BoomshotStage::completion_commit:return complete_owned();
        case core::BoomshotStage::interrupted:fail();drain();return core::BoomshotEvent::interrupted;
        default:return core::BoomshotEvent::none;
        }
    }
};
Owner owner;
}
