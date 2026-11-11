#pragma once
#include <cstdint>

// Copies the first already-read failing relation in a scoped diagnostic query.
// No reads, clocks, original calls, grants, filtering or changed return codes.
namespace gears_cover_owner_link_observation {
enum class Reader:unsigned {none,asset,collider,components};
enum class Phase:unsigned {none,before_dispatch,after_dispatch};
enum class Branch:unsigned {none,scene_or_internal,reverse,embedded_owner};
struct Key {std::uint64_t world{},pawn{},scene{},compose{};unsigned thread{};};
struct Row {
    bool observed{};Reader reader{};Phase phase{};Branch branch{};unsigned thread{};
    std::uintptr_t actor{},table{},expected_scene{},observed_scene{},internal{},reverse{};
    bool reverse_read{};
};
struct Report {Key key{};Row first{};};
struct Active {Report* output{};Phase phase{};Active* previous{};};
inline thread_local Active* active{};
class Scope {
    Active value_{};bool installed_{};
public:
    Scope(bool enabled,Key key,Report& report)noexcept{
        if(!enabled||!key.world||!key.pawn||!key.scene||!key.compose||!key.thread)return;
        report={};report.key=key;value_={&report,Phase::none,active};active=&value_;installed_=true;
    }
    ~Scope(){if(installed_)active=value_.previous;}
    Scope(Scope const&)=delete;Scope& operator=(Scope const&)=delete;
};
class PhaseScope {
    Active* owner_{};Phase previous_{};
public:
    explicit PhaseScope(Phase phase)noexcept:owner_(active){
        if(owner_){previous_=owner_->phase;owner_->phase=phase;}
    }
    ~PhaseScope(){if(owner_)owner_->phase=previous_;}
    PhaseScope(PhaseScope const&)=delete;PhaseScope& operator=(PhaseScope const&)=delete;
};
inline void note(Row row)noexcept{
    if(!active||active->phase==Phase::none||row.thread!=active->output->key.thread||
       row.reader==Reader::none||row.branch==Branch::none||active->output->first.observed)return;
    row.observed=true;row.phase=active->phase;active->output->first=row;
}
}
