// Ordinary PostPlayerTick entry producer. Shared Native input/rig reads and
// public hand arbitration remain the only input authority. No C0/pose writer.
namespace troika_entry_runtime {
namespace hand=gears_weapon_hand;namespace rig=gears_rig_tracking;
struct Candidate {hand::ObjectIdentity turret{};gears_weapon::TroikaEntryContact contact;};
std::array<Candidate,64> candidates{};
uptr registry_data{};int registry_capacity{},scan_cursor{};
hand::ObjectIdentity pawn_scan_world{},pawn_scan_next{};
std::uint64_t pawn_scan_generation{};
std::array<uptr,256> pawn_scan_seen{};std::size_t pawn_scan_count{};
hand::Frame previous{};rig::Frame previous_rig{};bool have_previous{},candidate_overflow{};
std::uint64_t next_request{1};
troika_original::EntryOwner owner;
gears_troika_source::ConsumerCursor observation_cursor;
std::atomic<bool> stop_requested{},owed_snapshot{};
std::atomic<std::uint64_t> last_cookie{},stopped_cookie{};
unsigned log_rows{};
bool configured() noexcept {
#if defined(GEARS_PLAYGROUND_TROIKA_INTERACTION)
    return true;
#else
    return false;
#endif
}
bool owed() noexcept {return owed_snapshot.load(std::memory_order_acquire);}
void request_stop() noexcept {
    const auto cookie=weaponhand::client.cookie();
    stopped_cookie.store(cookie?cookie:last_cookie.load(std::memory_order_acquire),std::memory_order_release);
    stop_requested.store(true,std::memory_order_release);
}
bool drain() noexcept {
    if(!configured()&&!owed())return true;
    request_stop();
    if(weaponhand::current_gt(nullptr))return false;
    const auto until=GetTickCount64()+2000;
    while((owed()||callbacks.load(std::memory_order_acquire))&&GetTickCount64()<until)Sleep(1);
    return !owed()&&!callbacks.load(std::memory_order_acquire);
}
void reset_contacts() noexcept {for(auto& c:candidates)c.contact.reset();have_previous=false;}
// Private, bounded observation of the already evaluated entry predicates.
// Does not reacquire input/objects, renew a ticket, or grant an action.
void observe_entry(unsigned gate,unsigned detail,hand::Frame const& f,hand::GripGameState const& game,
                   hand::Decision const& decision) noexcept {
#if defined(GEARS_TROIKA_ENTRY_DIAGNOSTIC)
    static std::array<unsigned,24> rows{},last{};static unsigned total{};
    if(!log_file||gate>=rows.size()||total>=48||rows[gate]>=4||(rows[gate]&&last[gate]==detail))return;
    ++rows[gate];++total;last[gate]=detail;
    fwprintf(log_file,L"troika_entry_predicate gate=%u detail=%u frame=%llu sequence=%llu known=%u held=%u busy=%u animation=%u lease=%u left_incumbent=%u right_incumbent=%u\n",
        gate,detail,static_cast<unsigned long long>(f.key.frame),static_cast<unsigned long long>(f.controllers.sequence),
        unsigned(game.known),unsigned(game.main_hand_held),unsigned(game.busy),unsigned(game.animation_owns_hands),
        unsigned(decision.lease.kind),unsigned(game.object_incumbents[0].kind),unsigned(game.object_incumbents[1].kind));
    fflush(log_file);
#else
    (void)gate;(void)detail;(void)f;(void)game;(void)decision;
#endif
}
void report(unsigned gate,unsigned mask=0,troika_original::EntryReceipt const* attempt=nullptr) noexcept {
    if(log_file&&log_rows<12){++log_rows;
        const auto& r=attempt?*attempt:owner.receipt;
        fwprintf(log_file,L"troika_entry gate=%u contacts=%u token=%llu binding=%llu result=%u dispatched=%u owed=%u\n",
            gate,mask,r.token,r.binding,unsigned(r.result),unsigned(r.dispatched),unsigned(owner.outstanding));
        fflush(log_file);
    }
}
#if defined(GEARS_TROIKA_ENTRY_DIAGNOSTIC)
// Log the already computed, unsnapped contact observation, not the test's
// planned target. No second input/geometry read and no new action authority.
void observe_contact(std::size_t slot,hand::Frame const& f,rig::Frame const& raw,troika_parts::Parts const& parts,
                     unsigned mask,std::array<double,4> const& distances,bool valid) noexcept {
    struct Seen {hand::ObjectIdentity turret{};double nearest{10};unsigned mask{},rows{};bool valid{};};
    static std::array<Seen,64> seen{};static unsigned total{};
    if(!log_file||slot>=seen.size()||total>=24)return;
    auto& last=seen[slot];if(!hand::same(last.turret,parts.turret)){last={};last.turret=parts.turret;}
    double nearest=10;for(double d:distances)nearest=(std::min)(nearest,d);
    if(last.rows>=6||(valid&&nearest>.15&&!last.rows)||
       (last.rows&&last.mask==mask&&last.valid==valid&&std::abs(nearest-last.nearest)<.015))return;
    ++last.rows;++total;last.nearest=nearest;last.mask=mask;last.valid=valid;
    fwprintf(log_file,L"troika_entry_contact frame=%llu scene=%llu sequence=%llu sync_ns=%lld rig_ns=%llu turret=%llu mask=%u geometry_valid=%u units=%.9g distances=%.9g,%.9g,%.9g,%.9g palms=%.9g,%.9g,%.9g,%.9g,%.9g,%.9g handleroot=",
        static_cast<unsigned long long>(f.key.frame),static_cast<unsigned long long>(raw.scene_invocation),
        static_cast<unsigned long long>(f.controllers.sequence),static_cast<long long>(f.controllers.sync_ns),
        static_cast<unsigned long long>(raw.published_ns),static_cast<unsigned long long>(parts.turret.address),mask,unsigned(valid),
        raw.units_per_metre,distances[0],distances[1],distances[2],distances[3],
        raw.world_grip[0].position.x,raw.world_grip[0].position.y,raw.world_grip[0].position.z,
        raw.world_grip[1].position.x,raw.world_grip[1].position.y,raw.world_grip[1].position.z);
    for(unsigned i=0;i<16;++i)fwprintf(log_file,i?L",%.9g":L"%.9g",double(parts.handleroot[i]));
    fwprintf(log_file,L"\n");fflush(log_file);
}
#endif
bool cohort(hand::Frame const& f,rig::Frame const& r) noexcept {
    if(!have_previous||f.recentered||f.heading_deferred)return false;
    const auto& a=previous;const auto& b=previous_rig;
    return a.key.game_pid==f.key.game_pid&&a.key.game_start==f.key.game_start&&a.key.thread_id==f.key.thread_id&&
        a.key.native_started_qpc==f.key.native_started_qpc&&a.key.registration_cookie==f.key.registration_cookie&&
        a.key.lifecycle==f.key.lifecycle&&a.key.world_generation==f.key.world_generation&&
        hand::same(a.world,f.world)&&hand::same(a.controller,f.controller)&&hand::same(a.pawn,f.pawn)&&hand::same(a.mesh,f.mesh)&&
        a.reference_generation==f.reference_generation&&a.controllers.session==f.controllers.session&&
        a.controllers.space_epoch==f.controllers.space_epoch&&a.controllers.profile_generation==f.controllers.profile_generation&&
        a.controllers.input_epoch==f.controllers.input_epoch&&f.controllers.sequence>=a.controllers.sequence&&
        f.controllers.sync_ns>=a.controllers.sync_ns&&f.controllers.sync_ns-a.controllers.sync_ns<=100000000&&
        b.observed.stream==r.observed.stream&&b.observed.reference==r.observed.reference&&
        b.observed.hand_tracking[0]==r.observed.hand_tracking[0]&&b.observed.hand_tracking[1]==r.observed.hand_tracking[1]&&
        b.producer.stream==r.producer.stream&&b.producer.tracking[0]==r.producer.tracking[0]&&b.producer.tracking[1]==r.producer.tracking[1];
}
bool remember_candidate(hand::Frame const& f,uptr address) noexcept {
    if(!playground::instance(address,L"Turret_TroikaCabal_base"))return true;
    hand::ObjectIdentity id{};
    if(!nativeequipment::identify(nullptr,address,id)||!troika_original::link(address,L"WorldInfo",f.world.address))return true;
    for(auto const& c:candidates)if(hand::same(c.turret,id))return true;
    for(auto& c:candidates){if(!c.turret.address||!troika_original::same_object(c.turret)||
        !troika_original::link(c.turret.address,L"WorldInfo",f.world.address)){
        c={};c.turret=id;return true;
    }}
    candidate_overflow=true;return false;
}
// Original WorldInfo.PawnList / Pawn.NextPawn are discovery hints only.
// Re-read typed links and exact identities; never turn a failed read into null.
template<std::size_t N,std::size_t K>
bool pawn_link(hand::Frame const& f,hand::ObjectIdentity const& object,wchar_t const (&type)[N],
               wchar_t const (&label)[K],hand::ObjectIdentity& next) noexcept {
    namespace meta=weaponhand::meta;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader reader{schema,meta::Stage::final_check};
    meta::PointerField link{},owner_world{},target_world{};uptr again{},owner_again{},target_again{};hand::ObjectIdentity value{};
    const bool world_head=hand::same(object,f.world);
    if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(reader,object)||
       !weaponhand::derives(reader,object.address,type,L"Engine")||
       !weaponhand::read_pointer(reader,object.address,label,link)||!troika_reflection::retain(reader,link.property))return false;
    if(!world_head&&(!weaponhand::pointer(reader,object.address,L"WorldInfo",f.world.address,owner_world)||
       !troika_reflection::retain(reader,owner_world.property)))return false;
    if(link.value&&(!nativeequipment::identify(nullptr,link.value,value)||!weaponhand::object(reader,value)||
       !weaponhand::derives(reader,value.address,L"Pawn",L"Engine")||
       !weaponhand::pointer(reader,value.address,L"WorldInfo",f.world.address,target_world)||
       !troika_reflection::retain(reader,target_world.property)))return false;
    if(!reader.at(object.address,link.property.offset,again)||again!=link.value||
       (!world_head&&(!reader.at(object.address,owner_world.property.offset,owner_again)||owner_again!=f.world.address))||
       (link.value&&(!reader.at(value.address,target_world.property.offset,target_again)||target_again!=f.world.address))||
       !unified_weapon_fire::retained_metadata(reader)||!nativeequipment::current(nullptr,f))return false;
    next=value;return true;
}
void discover_pawns(hand::Frame const& f) noexcept {
    if(!hand::same(pawn_scan_world,f.world)||pawn_scan_generation!=f.key.world_generation){
        candidates={};scan_cursor=0;candidate_overflow=false;
        pawn_scan_world=f.world;pawn_scan_generation=f.key.world_generation;pawn_scan_next={};pawn_scan_count=0;
    }
    if(!pawn_scan_next.address){
        pawn_scan_count=0;
        if(!pawn_link(f,f.world,L"WorldInfo",L"PawnList",pawn_scan_next))return;
    }
    const auto started=weaponhand::clock_ns(nullptr);
    for(unsigned n=0;n<4&&pawn_scan_next.address;++n){
        if(n&&weaponhand::clock_ns(nullptr)-started>=250000)return;
        const auto current=pawn_scan_next;
        bool repeated=false;for(std::size_t i=0;i<pawn_scan_count;++i)if(pawn_scan_seen[i]==current.address){repeated=true;break;}
        if(repeated||pawn_scan_count==pawn_scan_seen.size()){pawn_scan_next={};pawn_scan_count=0;return;}
        hand::ObjectIdentity next{};
        if(!pawn_link(f,current,L"Pawn",L"NextPawn",next)){pawn_scan_next={};pawn_scan_count=0;return;}
        pawn_scan_seen[pawn_scan_count++]=current.address;pawn_scan_next=next;
        if(!remember_candidate(f,current.address))return;
    }
}
void discover(hand::Frame const& f) noexcept {
    const auto table=get<Array>(base+0x1c24730);
    const bool table_valid=table.data&&table.count>=1000&&table.count<=table.capacity&&table.capacity<4000000;
    if(!table_valid){discover_pawns(f);return;}
    if(registry_data!=table.data||registry_capacity!=table.capacity){registry_data=table.data;registry_capacity=table.capacity;scan_cursor=0;}
    if(scan_cursor>=table.count){scan_cursor=0;candidate_overflow=false;}
    discover_pawns(f);
    // Keep advancing the fallback even after overflow, so its next full pass
    // can re-evaluate capacity after a candidate has departed.
    const auto started=weaponhand::clock_ns(nullptr);
    for(unsigned n=0;n<512&&scan_cursor<table.count;++n,++scan_cursor){
        if((n&31u)==0&&weaponhand::clock_ns(nullptr)-started>=750000)break;
        const auto address=get<uptr>(table.data+std::size_t(scan_cursor)*8);
        if(!remember_candidate(f,address)){++scan_cursor;return;}
    }
}
bool within_reach(hand::Frame const& f,rig::Frame const& raw,hand::ObjectIdentity const& turret) noexcept {
    namespace meta=weaponhand::meta;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader reader{schema,meta::Stage::final_check};
    weaponhand::BoolField field{};playground::Vector location{};
    if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(reader,turret)||
       !weaponhand::field(reader,turret.address,L"Location",L"StructProperty",12,field)||
       !reader.at(turret.address,field.property.offset,location)||!unified_weapon_fire::retained_metadata(reader))return false;
    if(!std::isfinite(location.x)||!std::isfinite(location.y)||!std::isfinite(location.z))return false;
    for(unsigned side=0;side<2;++side){const auto p=raw.world_grip[side].position;
        if(std::hypot(std::hypot(p.x-location.x,p.y-location.y),p.z-location.z)<=2.5*raw.units_per_metre)
            return nativeequipment::current(nullptr,f);}
    return false;
}
#include "weapon_troika_geometry.inl"
namespace mounted_aim_candidates {bool qualified(gears_troika_fire::Run const&) noexcept;bool hold_current(gears_troika_fire::Run const&) noexcept;}
#include "weapon_troika_fire.inl"
#include "weapon_troika_aim_candidates.inl"
// true reserves this occupied/changed controller context from ordinary equipment
// and firearm consumers in the SAME tick. It does not grant any fire permission.
bool tick(uptr pc) noexcept {
    if(!configured()||!weaponhand::current_gt(nullptr))return false;
    if(owner.outstanding||mounted_fire::owed()){
        if(stop_requested.load(std::memory_order_acquire))mounted_aim_candidates::interrupt();
        mounted_fire::service(stop_requested.load(std::memory_order_acquire));
        hand::Frame new_world{};hand::GripGameState new_game{};
        bool retired=false;
        if(!mounted_fire::owed()&&weaponhand::client.copy_frame_on_gt(new_world,new_game)&&new_game.known&&new_world.controller.address==pc)
            retired=troika_original::retire_for_context_change(owner,new_world);
        if(pc!=owner.receipt.controller.address&&!retired){mounted_fire::service(true);return false;}
        troika_original::observe_result(owner);
        troika_original::observe_departed(owner,owner.binding,owner.last_token);
        if(owner.outstanding&&!mounted_fire::owed()&&stop_requested.load(std::memory_order_acquire))troika_original::leave(owner);
        if(owner.outstanding&&!stop_requested.load(std::memory_order_acquire)){
            gears_troika_source::Frame mounted_source{};
            if(troika_tracking::acquire(owner,mounted_source)){
                troika_tracking::observe_control(owner,mounted_source);
                gears_weapon::TroikaMountedMapping mapped{};
                const bool mapped_current=troika_tracking::map(owner,mounted_source,mapped);
                if(mapped_current&&observation_cursor.take(mounted_source,
                   weaponhand::clock_ns(nullptr),owner.receipt.source==gears_vr::StereoSource::MetaOperatorSimulated)){
                    troika_mounted_parts::Observation original_parts{};
                    if(troika_mounted_parts::observe(owner,mounted_source,mapped,original_parts,true))
                        mounted_aim_candidates::observe(mounted_source,mapped,original_parts);
                    else mounted_aim_candidates::interrupt();
                }
                if(!mapped_current)mounted_aim_candidates::interrupt();
                mounted_fire::observe(mounted_source);
                mounted_aim_candidates::advance();
            }else {troika_tracking::interrupt();mounted_aim_candidates::interrupt();mounted_fire::service(true);}
        }
        if(!owner.outstanding){mounted_aim_candidates::interrupt();mounted_fire::service(true);}
        owed_snapshot.store(owner.outstanding||mounted_fire::owed(),std::memory_order_release);
        if(!owner.outstanding&&!mounted_fire::owed()){reset_contacts();troika_tracking::reset();}
        return true; // even a completed exit changes the source association this tick
    }
    hand::Frame f{};hand::GripGameState game{};hand::Decision decision{};
    unsigned rejected{};
    if(!weaponhand::client.copy_frame_on_gt(f,game,decision))rejected=1;
    else if(f.controller.address!=pc)rejected=2;
    else if(!game.known)rejected=3;
    else if(!nativeequipment::current(nullptr,f))rejected=4;
    else if(!nativeequipment::restoration_quiet())rejected=5;
    if(rejected){
        observe_entry(10,rejected,f,game,decision);
        reset_contacts();return false;
    }
    const auto cookie=f.key.registration_cookie;last_cookie.store(cookie,std::memory_order_release);
    mounted_geometry::attach(); // Same participant pin; geometry only.
    if(stop_requested.load(std::memory_order_acquire)){
        if(cookie==stopped_cookie.load(std::memory_order_acquire)){observe_entry(11,1,f,game,decision);reset_contacts();return false;}
        stop_requested.store(false,std::memory_order_release); // new authenticated registration only
    }
    unsigned occupied{};
    if(game.main_hand_held)occupied=1;
    else if(game.busy)occupied=2;
    else if(game.animation_owns_hands)occupied=3;
    else if(vrequip::model.held())occupied=4;
    else if(vrequip::model.switch_pending())occupied=5;
    else if(vrequip::model.animation_owns_hands())occupied=6;
    else if(decision.lease.kind!=hand::LeaseKind::none)occupied=7;
    else if(game.object_incumbents[0].kind!=hand::LeaseKind::none)occupied=8;
    else if(game.object_incumbents[1].kind!=hand::LeaseKind::none)occupied=9;
    const bool empty=occupied==0;
    if(!empty){observe_entry(12,occupied,f,game,decision);reset_contacts();return false;}
    // Discovery requires the authenticated empty-hand frame, not a successful
    // hand-rig sample. Contact and entry still require the full rig below.
    discover(f);if(candidate_overflow){observe_entry(14,1,f,game,decision);reset_contacts();report(1);return false;}
    rig::Frame raw{};
#if defined(GEARS_PLAYGROUND_NATIVE_EQUIPMENT_RIG_READER)
    const auto rig_status=nativeequipment_rig::acquire(f,raw).status;
    if(rig_status!=nativeequipment_rig::Status::ready||
       (raw.world_pose_mask&(rig::left_grip_pose|rig::right_grip_pose))!=(rig::left_grip_pose|rig::right_grip_pose)){
        observe_entry(13,unsigned(rig_status)|(unsigned(raw.world_pose_mask)<<8),f,game,decision);
        reset_contacts();return false;
    }
#else
    reset_contacts();return false;
#endif
    const bool continuous=cohort(f,raw);
    if(!continuous){for(auto& c:candidates)c.contact.reset();}
    previous=f;previous_rig=raw;have_previous=true;
    std::array<unsigned,64> masks{};std::array<bool,64> geometry_known{};unsigned touching{};std::size_t selected{};
    unsigned discovered{},in_reach{},parts_read{};
    for(std::size_t i=0;i<candidates.size();++i){auto& c=candidates[i];
        if(!c.turret.address)continue;
        ++discovered;
        if(!within_reach(f,raw,c.turret)){c.contact.reset();continue;}
        ++in_reach;
        troika_parts::Parts parts{};
        if(!troika_parts::read(f,c.turret,parts)){c.contact.reset();continue;}
        ++parts_read;
        geometry_known[i]=true;
#if defined(GEARS_TROIKA_ENTRY_DIAGNOSTIC)
        std::array<double,4> distances{10,10,10,10};bool contact_geometry_valid=true;
#endif
        for(unsigned side=0;side<2;++side)for(unsigned handle=0;handle<2;++handle){
            gears_weapon::troika_handle_geometry::Contact contact{};
            const bool valid=gears_weapon::troika_handle_geometry::contact(parts.handleroot,raw.world_grip[side].position,
                handle,raw.units_per_metre,contact);
            if(valid&&contact.touched)masks[i]|=1u<<side;
#if defined(GEARS_TROIKA_ENTRY_DIAGNOSTIC)
            contact_geometry_valid=contact_geometry_valid&&valid;
            if(valid)distances[side*2+handle]=(std::min)(contact.distance_metres,10.);
#endif
        }
#if defined(GEARS_TROIKA_ENTRY_DIAGNOSTIC)
        observe_contact(i,f,raw,parts,masks[i],distances,contact_geometry_valid);
#endif
        if(masks[i]){++touching;selected=i;}
    }
    bool request=false;
    for(std::size_t i=0;i<candidates.size();++i)if(candidates[i].turret.address&&geometry_known[i]){
        const bool edge=candidates[i].contact.observe(masks[i],empty?3u:0u,continuous,true,empty&&touching==1);
        if(edge&&i==selected)request=true;
    }
    observe_entry(touching?17u:16u,discovered|(in_reach<<7)|(parts_read<<14)|(unsigned(continuous)<<21)|(unsigned(request)<<22)|(touching<<23),f,game,decision);
    if(!request||!nativeequipment::current(nullptr,f))return false;
    if(next_request==(std::numeric_limits<std::uint64_t>::max)()||
       f.controllers.sync_ns>(std::numeric_limits<std::int64_t>::max)()-100000000)return false;
    const auto serial=next_request++;const auto& turret=candidates[selected].turret;
    hand::ObjectIdentity weapon{};namespace meta=weaponhand::meta;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader reader{schema,meta::Stage::final_check};
    if(!meta::tables(base,schema.objects,schema.names)||!troika_mounted_context::pointer(reader,turret.address,L"Weapon",weapon)||
       !weaponhand::derives(reader,weapon.address,L"Weap_Troika_base",L"WarfareGame")||
       !troika_original::link(weapon.address,L"Instigator",turret.address)||!unified_weapon_fire::retained_metadata(reader)){
        observe_entry(18,1,f,game,decision);return false;}
    gears_weapon::TroikaRequest q{};
    q.kind=gears_weapon::TroikaAction::mount_original;q.token=serial;q.stamp.binding=serial;
    q.stamp.sequence=f.key.frame;q.stamp.monotonic_ns=weaponhand::clock_ns(nullptr);
    q.frame_ticket=f.key.frame;q.valid_until_ns=f.controllers.sync_ns+100000000;
    q.stamp.identity={f.world.address,f.key.world_generation,f.pawn.address,f.controller.address,turret.address,serial,
        weapon.address,serial,f.controllers.session,f.controllers.space_epoch,f.controllers.input_epoch,f.controllers.profile_generation};
    troika_tracking::EntryBasis prepared_basis{};
    troika_tracking::prepare_entry_basis(f,raw,turret,weapon,q,prepared_basis);
    const auto receipt=troika_original::enter(owner,f,turret,q,stop_requested);
    if(!owner.outstanding||!troika_tracking::retain_entry_basis(owner,prepared_basis))troika_tracking::cancel_prepared_basis(prepared_basis);
    owed_snapshot.store(owner.outstanding,std::memory_order_release);report(2,masks[selected],&receipt);
    return receipt.dispatched;
}
} // namespace troika_entry_runtime
