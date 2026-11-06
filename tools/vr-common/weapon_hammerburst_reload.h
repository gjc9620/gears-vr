#pragma once
#include <algorithm>
#include <cstdint>

namespace gears_weapon {

// Recovered evidence: analysis/solo-m1-20261028/hb-original-latch-track.json (WarfareGame.u
// SHA256 91c0f56e9302db2b3991fc3d8646abc1eaff2b33b8b021ce8df436d8cd681d13)
// and the Hammerburst magazine part manifest. HB_Reload
// has 75 rotation keys on b_HB_MagLatch; b_HB_Mag is constant relative to its
// latch parent. Float96NoW decoding finds ~48deg maximum stored local rotation
// about local Y, returning closed. This is not a controller-angle threshold,
// insertion axis, contact calibration or runtime key-time proof. No Lancer mounts.
inline constexpr char hammerburst_original_class[]="Weap_LocustAssaultRifle";
inline constexpr char hammerburst_original_magazine[]=
    "Locust_Hammerburst.Hammerburst_Magazine";
inline constexpr char hammerburst_original_latch_bone[]="b_HB_MagLatch";

enum class HammerburstReloadStage : std::uint8_t {
    idle,opening_latch,take_magazine,holding_magazine,closing_latch,complete,interrupted,paused
};
enum class HammerburstReloadEvent : std::uint8_t {
    rejected,invalid,full,no_ammo,started,opened,grabbed,released,inserted,
    perfect,ordinary,interrupted
};
enum class HammerburstGuidePart : std::uint8_t {
    none,magazine_latch,left_shoulder,feed_port
};
struct HammerburstGuide {
    HammerburstGuidePart part{HammerburstGuidePart::none};
    float red{1},green{1},blue{0},alpha{1}; // Current step's yellow render intent.
};
struct HammerburstReloadIdentity {
    std::uint64_t world{},pawn{},weapon{},world_generation{},session{},
                  space_epoch{},input_epoch{},profile_generation{};
    bool valid() const noexcept {
        return world&&pawn&&weapon&&world_generation&&session&&space_epoch&&
            input_epoch&&profile_generation;
    }
};
inline bool operator==(HammerburstReloadIdentity const& a,
                       HammerburstReloadIdentity const& b) noexcept {
    return a.world==b.world&&a.pawn==b.pawn&&a.weapon==b.weapon&&
        a.world_generation==b.world_generation&&a.session==b.session&&
        a.space_epoch==b.space_epoch&&a.input_epoch==b.input_epoch&&
        a.profile_generation==b.profile_generation;
}
// The adapter assigns one increasing event sequence on the game thread. The
// clock is the original game's reload clock; paused samples may share a tick.
struct HammerburstReloadStamp {
    HammerburstReloadIdentity identity{};
    std::uint64_t sequence{};
    std::int64_t game_tick{};
    bool valid() const noexcept {return identity.valid()&&sequence&&game_tick>=0;}
};
struct HammerburstPerfectWindow {
    HammerburstReloadIdentity identity{};
    std::int64_t end_ticks{};
    bool current_instance_verified{};
};
struct HammerburstReloadStart {
    HammerburstReloadStamp stamp{};
    std::uint64_t r3_press_edge{},original_reload_serial{};
    std::uint32_t capacity{},loaded{},reserve{};
    HammerburstPerfectWindow perfect{};
    // The fixed open/eject/insert/close path below is usable only after the
    // live adapter validates that order on this actual weapon instance.
    bool original_class_verified{},original_reload_accepted{},
         current_instance_sequence_verified{};
};
enum class HammerburstLatchPose : std::uint8_t { unknown,closed,open };
struct HammerburstLatchReceipt {
    std::uint64_t original_reload_serial{},motion_begin_sequence{};
    HammerburstLatchPose from{HammerburstLatchPose::unknown},to{HammerburstLatchPose::unknown};
    bool measured_hammerburst_travel{},old_magazine_released{};
};
enum class HammerburstAmmoSource : std::uint8_t { other,left_shoulder };
enum class HammerburstAmmoKind : std::uint8_t { other,hammerburst_magazine };
struct HammerburstMagazineReceipt {
    std::uint64_t item{};
    HammerburstAmmoSource source{HammerburstAmmoSource::other};
    HammerburstAmmoKind kind{HammerburstAmmoKind::other};
    bool tracked_left_hand{};
};
struct HammerburstInsertionReceipt {
    std::uint64_t item{},stroke_begin_sequence{};
    bool hammerburst_mount_verified{},aligned{},continuous{},seated{},tracked_left_hand{};
};
struct HammerburstReloadCompletion {
    HammerburstReloadIdentity identity{};
    std::uint64_t original_reload_serial{};
    std::uint32_t loaded{},reserve{};
    std::int64_t started_tick{},finished_tick{};
    HammerburstReloadEvent result{HammerburstReloadEvent::rejected};
};

// Pure, Hammerburst-specific transaction. Receipts are adapter contracts, not
// hand-geometry recognition: opening/closing require a verified trace of this
// gun's named latch, and insertion requires its own calibrated continuous feed
// path. A sequence time or HB_Reload finishing alone cannot produce a receipt.
// Live animation phase holding/resuming, timer suppression, inventory commits,
// original perfect reward/sound and physical yellow rendering remain outside.
class HammerburstReload {
    HammerburstReloadStage stage_{HammerburstReloadStage::idle};
    HammerburstReloadStage paused_stage_{HammerburstReloadStage::idle};
    HammerburstReloadIdentity identity_{};
    HammerburstPerfectWindow perfect_{};
    std::uint64_t r3_edge_{},reload_serial_{},start_sequence_{},last_sequence_{},
                  grab_sequence_{},insert_sequence_{},held_item_{},last_item_{},motion_floor_{};
    std::int64_t start_tick_{},last_tick_{},finish_tick_{};
    std::uint32_t capacity_{},loaded_{},reserve_{},held_{},target_{};
    bool latch_closed_{true},feed_present_{true},completion_taken_{};
    HammerburstReloadEvent outcome_{HammerburstReloadEvent::rejected};

    bool accepts(HammerburstReloadStamp const& stamp) const noexcept {
        return stamp.valid()&&stamp.identity==identity_&&
            stamp.sequence>last_sequence_&&stamp.game_tick>=last_tick_;
    }
    void consume(HammerburstReloadStamp const& stamp) noexcept {
        last_sequence_=stamp.sequence;last_tick_=stamp.game_tick;
    }
    bool latch_receipt(HammerburstReloadStamp const& stamp,
                       HammerburstLatchReceipt const& receipt,
                       HammerburstLatchPose from,HammerburstLatchPose to,
                       std::uint64_t after_sequence) const noexcept {
        return accepts(stamp)&&receipt.original_reload_serial==reload_serial_&&
            receipt.measured_hammerburst_travel&&receipt.from==from&&receipt.to==to&&
            receipt.motion_begin_sequence>=after_sequence&&receipt.motion_begin_sequence>=motion_floor_&&
            receipt.motion_begin_sequence<stamp.sequence;
    }
public:
    // Physical latch intent uses the existing monotonic intent slot. It is
    // not an R3 prerequisite; native acceptance and measured travel remain
    // separately required by the adapter receipts.
    HammerburstReloadEvent begin_latch(HammerburstReloadStart start,std::uint64_t grip_edge) noexcept {
        start.r3_press_edge=grip_edge;return begin_r3(start);
    }
    HammerburstReloadEvent suspend() noexcept {
        if(stage_!=HammerburstReloadStage::opening_latch&&stage_!=HammerburstReloadStage::take_magazine&&
           stage_!=HammerburstReloadStage::holding_magazine&&stage_!=HammerburstReloadStage::closing_latch)
            return HammerburstReloadEvent::rejected;
        if(stage_==HammerburstReloadStage::holding_magazine){
            reserve_+=held_;held_=0;held_item_=0;stage_=HammerburstReloadStage::take_magazine;
        }
        paused_stage_=stage_;stage_=HammerburstReloadStage::paused;
        return HammerburstReloadEvent::interrupted;
    }
    bool resume(HammerburstReloadStamp const& stamp,std::uint64_t serial,std::uint64_t fresh_grip_edge) noexcept {
        if(stage_!=HammerburstReloadStage::paused||!accepts(stamp)||serial!=reload_serial_||
           !fresh_grip_edge||fresh_grip_edge<=r3_edge_)return false;
        consume(stamp);motion_floor_=stamp.sequence;r3_edge_=fresh_grip_edge;stage_=paused_stage_;
        paused_stage_=HammerburstReloadStage::idle;return true;
    }
    // Reset only when the live adapter has reconciled authoritative inventory;
    // resetting is not an inventory rollback and must not replay a reward.
    void reset() noexcept {
        // Keep the last identity and monotonic event keys. Rebinding the same
        // live instance after an interruption must not accept an old R3 edge.
        const auto identity=identity_;
        const auto sequence=last_sequence_,reload=reload_serial_,edge=r3_edge_;
        *this=HammerburstReload{};
        identity_=identity;last_sequence_=sequence;reload_serial_=reload;
        r3_edge_=edge;completion_taken_=true;
    }
    HammerburstReloadEvent begin_r3(HammerburstReloadStart const& start) noexcept {
        if(stage_!=HammerburstReloadStage::idle&&stage_!=HammerburstReloadStage::complete)
            return HammerburstReloadEvent::rejected;
        if(!start.stamp.valid()||!start.r3_press_edge||!start.original_reload_serial||
           !start.original_class_verified||!start.original_reload_accepted||
           !start.current_instance_sequence_verified||
           !start.capacity||start.capacity>10000||start.loaded>start.capacity||
           start.reserve>1000000||start.perfect.end_ticks<0||
           (start.perfect.current_instance_verified&&
            (!(start.perfect.identity==start.stamp.identity)||start.perfect.end_ticks<=0)))
            return HammerburstReloadEvent::invalid;
        if(identity_.valid()&&(!(start.stamp.identity==identity_)||
           start.stamp.sequence<=last_sequence_||start.stamp.game_tick<last_tick_||
           start.r3_press_edge<=r3_edge_||start.original_reload_serial<=reload_serial_||
           !completion_taken_))return HammerburstReloadEvent::rejected;
        if(start.loaded==start.capacity)return HammerburstReloadEvent::full;
        if(!start.reserve)return HammerburstReloadEvent::no_ammo;
        identity_=start.stamp.identity;perfect_=start.perfect;
        r3_edge_=start.r3_press_edge;reload_serial_=start.original_reload_serial;
        capacity_=start.capacity;loaded_=start.loaded;reserve_=start.reserve;
        target_=held_=0;held_item_=last_item_=grab_sequence_=insert_sequence_=0;
        start_tick_=start.stamp.game_tick;start_sequence_=start.stamp.sequence;
        consume(start.stamp);completion_taken_=false;
        latch_closed_=feed_present_=true;outcome_=HammerburstReloadEvent::rejected;
        stage_=HammerburstReloadStage::opening_latch;
        return HammerburstReloadEvent::started;
    }
    HammerburstReloadEvent confirm_latch_open(HammerburstReloadStamp const& stamp,
                                             HammerburstLatchReceipt const& receipt) noexcept {
        if(stage_!=HammerburstReloadStage::opening_latch||!receipt.old_magazine_released||
           !latch_receipt(stamp,receipt,HammerburstLatchPose::closed,HammerburstLatchPose::open,
                          start_sequence_))return HammerburstReloadEvent::rejected;
        consume(stamp);latch_closed_=feed_present_=false;
        // Retain the ejected magazine's unspent rounds in the available pool.
        // This is a conservation rule for the eventual inventory adapter.
        reserve_+=loaded_;loaded_=0;target_=(std::min)(capacity_,reserve_);
        stage_=HammerburstReloadStage::take_magazine;
        return HammerburstReloadEvent::opened;
    }
    HammerburstReloadEvent grab_left_shoulder(HammerburstReloadStamp const& stamp,
                                              HammerburstMagazineReceipt const& receipt) noexcept {
        if(stage_!=HammerburstReloadStage::take_magazine||!accepts(stamp)||!receipt.item||
           receipt.item==last_item_||!receipt.tracked_left_hand||
           receipt.source!=HammerburstAmmoSource::left_shoulder||
           receipt.kind!=HammerburstAmmoKind::hammerburst_magazine||!target_||reserve_<target_)
            return HammerburstReloadEvent::rejected;
        consume(stamp);held_item_=last_item_=receipt.item;grab_sequence_=stamp.sequence;
        held_=target_;reserve_-=held_;stage_=HammerburstReloadStage::holding_magazine;
        return HammerburstReloadEvent::grabbed;
    }
    HammerburstReloadEvent release_magazine(HammerburstReloadStamp const& stamp,
                                            std::uint64_t item) noexcept {
        if(stage_!=HammerburstReloadStage::holding_magazine||!accepts(stamp)||
           !item||item!=held_item_)return HammerburstReloadEvent::rejected;
        consume(stamp);reserve_+=held_;held_=0;held_item_=0;
        stage_=HammerburstReloadStage::take_magazine;
        return HammerburstReloadEvent::released;
    }
    HammerburstReloadEvent insert_magazine(HammerburstReloadStamp const& stamp,
                                           HammerburstInsertionReceipt const& receipt) noexcept {
        if(stage_!=HammerburstReloadStage::holding_magazine||!accepts(stamp)||
           !receipt.item||receipt.item!=held_item_||!receipt.hammerburst_mount_verified||
           !receipt.aligned||!receipt.continuous||!receipt.seated||!receipt.tracked_left_hand||
           receipt.stroke_begin_sequence<grab_sequence_||receipt.stroke_begin_sequence<motion_floor_||
           receipt.stroke_begin_sequence>=stamp.sequence)
            return HammerburstReloadEvent::rejected;
        consume(stamp);loaded_+=held_;held_=0;held_item_=0;feed_present_=true;
        insert_sequence_=stamp.sequence;stage_=HammerburstReloadStage::closing_latch;
        return HammerburstReloadEvent::inserted;
    }
    HammerburstReloadEvent confirm_latch_closed(HammerburstReloadStamp const& stamp,
                                               HammerburstLatchReceipt const& receipt) noexcept {
        if(stage_!=HammerburstReloadStage::closing_latch||
           !latch_receipt(stamp,receipt,HammerburstLatchPose::open,HammerburstLatchPose::closed,
                          insert_sequence_))return HammerburstReloadEvent::rejected;
        consume(stamp);latch_closed_=true;finish_tick_=stamp.game_tick;
        // No duration is inferred from HB_Reload's 2.5 s or from the 0.7 s
        // possible-success start. Unknown current-instance deadline is ordinary.
        const bool perfect=perfect_.current_instance_verified&&
            finish_tick_-start_tick_<=perfect_.end_ticks;
        outcome_=perfect?HammerburstReloadEvent::perfect:HammerburstReloadEvent::ordinary;
        stage_=HammerburstReloadStage::complete;return outcome_;
    }
    HammerburstReloadEvent interrupt() noexcept {
        if(stage_==HammerburstReloadStage::idle||stage_==HammerburstReloadStage::interrupted)
            return HammerburstReloadEvent::rejected;
        // Death/checkpoint/world or weapon changes invalidate the attempt.
        // Already seated ammo remains seated; a held item returns to its pool.
        reserve_+=held_;held_=0;held_item_=0;completion_taken_=true;
        outcome_=HammerburstReloadEvent::rejected;stage_=HammerburstReloadStage::interrupted;
        return HammerburstReloadEvent::interrupted;
    }
    // A single commit intent, keyed by identity + original_reload_serial. It
    // does not call native functions or prove that a reward was applied. The
    // adapter must authenticate again and commit inventory/reward exactly once.
    bool take_completion(HammerburstReloadCompletion& out) noexcept {
        if(stage_!=HammerburstReloadStage::complete||completion_taken_)return false;
        completion_taken_=true;
        out={identity_,reload_serial_,loaded_,reserve_,start_tick_,finish_tick_,outcome_};
        return true;
    }
    HammerburstGuide guide() const noexcept {
        switch(stage_){
            case HammerburstReloadStage::opening_latch:
            case HammerburstReloadStage::closing_latch:return {HammerburstGuidePart::magazine_latch};
            case HammerburstReloadStage::take_magazine:
                return {HammerburstGuidePart::left_shoulder};
            case HammerburstReloadStage::holding_magazine:
                return {HammerburstGuidePart::feed_port};
            default:return {};
        }
    }
    HammerburstReloadStage stage() const noexcept {return stage_;}
    std::uint32_t loaded() const noexcept {return loaded_;}
    std::uint32_t reserve() const noexcept {return reserve_;}
    std::uint32_t held() const noexcept {return held_;}
    std::uint32_t target() const noexcept {return target_;}
    std::uint32_t total() const noexcept {return loaded_+reserve_+held_;}
    bool feed_present() const noexcept {return feed_present_;}
    bool can_fire() const noexcept {
        return stage_==HammerburstReloadStage::complete&&latch_closed_&&feed_present_&&loaded_>0;
    }
};
} // namespace gears_weapon
