#pragma once
#include "../vr-common/original_r2_fire.h"
#include "../vr-common/weapon_hand_participant.h"

// PG-local input/ownership policy only. Caller authenticates each Frame and
// freshly certifies Game on the owning GT; Execute supplies the original call.
// Exclusive R2 source ownership remains a caller obligation. No engine reader,
// Native permission, retained Frame, pose or tracking sample lives here.
namespace gears_lancer_r2 {
namespace hand=gears_weapon_hand;
using Status=gears_weapon::OriginalR2Status;
struct Actors {hand::ObjectIdentity world{},controller{},pawn{},mesh{},weapon{};};
struct Game {Actors actors{};bool known{},lancer{},main_hand_held{},animation{},busy{},r2_known{},r2_held{};};
enum class SourceStatus {unavailable,ready,inactive,stale,sync_invalid,regressed,cohort_changed,actors_mismatch,yielded,stopping};
inline bool same(Actors const& a,Actors const& b) noexcept {
    return hand::same(a.world,b.world)&&hand::same(a.controller,b.controller)&&hand::same(a.pawn,b.pawn)&&
        hand::same(a.mesh,b.mesh)&&hand::same(a.weapon,b.weapon);
}
inline bool valid(Actors const& a) noexcept {
    return hand::identity_valid(a.world)&&hand::identity_valid(a.controller)&&hand::identity_valid(a.pawn)&&
        hand::identity_valid(a.mesh)&&hand::identity_valid(a.weapon);
}
// Dispatch admission only, never input loss. A retained owner's other PC
// ticks are ignored; a freshly authenticated new owner is allowed through so
// the ownership policy can retain/fault an owed old release explicitly.
inline bool tick_admitted(bool quiet,std::uint64_t pc,std::uint64_t source_pc,std::uint64_t retained_pc) noexcept {
    return quiet&&pc&&(source_pc?pc==source_pc:!retained_pc||pc==retained_pc);
}
class Input {
    gears_weapon::OriginalR2Fire ledger_;
    Actors actors_{};hand::FrameKey cohort_{}; // frame is always zero: not a retained publication.
    std::uint64_t sequence_{};std::int64_t sync_{},observed_{};
    std::uint32_t source_{};bool actors_known_{},bound_{},sealed_{};
    SourceStatus status_{SourceStatus::unavailable};
    bool safe(Game const& game) const noexcept {
        return game.known&&game.lancer&&game.r2_known&&valid(game.actors)&&actors_known_&&same(actors_,game.actors);
    }
    gears_weapon::OriginalR2Context context(Game const& game,bool allow) const noexcept {
        return {game.actors.world.address,game.actors.controller.address,game.actors.weapon.address,
            game.known&&game.lancer&&game.r2_known&&valid(game.actors),game.r2_held,allow,safe(game)};
    }
    template<class Execute>Status seal(Game const& game,Execute&& execute) {
        bound_=false;sealed_=true;
        if(ledger_.owns_press()){
            if(!safe(game))return Status::abort_required;
            // Empty source + press disabled invokes the legacy owned hand-back.
            return ledger_.advance({}, {},context(game,false),0,execute);
        }
        if(!game.known||!game.lancer||!game.r2_known||!valid(game.actors))return Status::rejected;
        if(!actors_known_||!same(actors_,game.actors)){
            ledger_.reset();actors_=game.actors;actors_known_=true;
        }
        return ledger_.yield(context(game,false),execute);
    }
public:
    bool owns_press() const noexcept {return ledger_.owns_press();}
    bool has_actors() const noexcept {return actors_known_;}
    Actors const& actors() const noexcept {return actors_;}
    SourceStatus source_status() const noexcept {return status_;}
    // Diagnostic scalars only; neither value extends source validity.
    std::int64_t last_sync_ns() const noexcept {return sync_;}
    std::int64_t last_observed_ns() const noexcept {return observed_;}
    // Caller has drained its callback lifetime. Never erase an owed release.
    bool reset_drained() noexcept {
        if(owns_press())return false;
        ledger_.reset();actors_={};cohort_={};sequence_=0;sync_=observed_=0;source_=0;
        actors_known_=bound_=sealed_=false;status_=SourceStatus::unavailable;return true;
    }
    template<class Execute>Status yield(Game const& game,Execute&& execute) {
        status_=SourceStatus::yielded;return seal(game,execute);
    }
    template<class Execute>Status advance(hand::Frame const* frame,bool source_current,Game const& game,
        std::int64_t now,Execute&& execute,bool stopping=false) {
        if(stopping){status_=SourceStatus::stopping;
            if(!owns_press()){bound_=false;sealed_=true;return Status::accepted;}
            return seal(game,execute);}
        const auto reject=[&](SourceStatus why){status_=why;return seal(game,execute);};
        // This guard precedes every possible legacy release, including edges.
        if(owns_press()&&!safe(game))return reject(SourceStatus::actors_mismatch);
        if(!frame||!source_current)return reject(SourceStatus::unavailable);
        const auto& f=*frame;
        if(!hand::frame_active(f,now))return reject(SourceStatus::inactive);
        constexpr std::int64_t age=100000000;
        if(now-f.now_ns>age)return reject(SourceStatus::stale);
        if(f.controllers.sync_ns>now||now-f.controllers.sync_ns>age)return reject(SourceStatus::sync_invalid);
        const Actors actual{f.world,f.controller,f.pawn,f.mesh,f.weapon};
        if(!game.known||!game.lancer||!game.r2_known||!valid(game.actors)||!same(actual,game.actors))
            return reject(SourceStatus::actors_mismatch);
        auto key=f.key;key.frame=0;
        const auto source=static_cast<std::uint32_t>(f.stereo.source);
        if(bound_&&(!hand::same(key,cohort_)||source!=source_||!same(actors_,actual)))
            return reject(SourceStatus::cohort_changed);
        if(bound_&&(now<observed_||f.controllers.sequence<sequence_||f.controllers.sync_ns<sync_||
           (f.controllers.sequence==sequence_&&f.controllers.sync_ns!=sync_)))return reject(SourceStatus::regressed);
        // Current sample age is checked above. Continuity measures producer
        // sync-to-sync spacing, not previous-sync-to-consumer time: otherwise
        // a fresh 70ms interval plus 40ms read latency is falsely sealed.
        if(bound_&&f.controllers.sequence>sequence_&&(f.controllers.sync_ns<=sync_||f.controllers.sync_ns-sync_>age))
            return reject(SourceStatus::sync_invalid);
        if(!actors_known_||!same(actors_,actual)){
            // No owned press can pass the safe guard with a changed actor.
            ledger_.reset();actors_=actual;actors_known_=true;
        }
        const bool allow=game.main_hand_held&&!game.animation&&!game.busy&&f.controllers.hands[1].trigger_active==1;
        if(sealed_){const auto sealed=ledger_.yield(context(game,false),execute);
            if(sealed==Status::abort_required)return sealed;sealed_=false;}
        cohort_=key;source_=source;bound_=true;sequence_=f.controllers.sequence;sync_=f.controllers.sync_ns;observed_=now;
        status_=SourceStatus::ready;
        return ledger_.advance(f.controllers,f.stereo,context(game,allow),now,execute,source==1);
    }
};
} // namespace gears_lancer_r2
