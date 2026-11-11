#pragma once
#include <array>
#include <cstdint>
#include <limits>

// Copies existing clock/sample values only. No reads, clocks, decisions,
// leases or input. Fixed first/first-age rows for each of two existing checks.
namespace gears_body_source_age_observation {
struct Key {std::uint64_t world{},pawn{},scene{},compose{};unsigned thread{};};
struct Row {
    bool observed{};std::uint64_t tick{},world{},pawn{},scene{},stereo_sequence{},controller_sequence{};unsigned thread{},verdict{};bool latched{};
    std::int64_t now{},metadata_begin{},metadata_end{},tracking_begin{},tracking_end{},stereo_received{},controller_received{},sync{},capture_begin{};
};
struct Report {Key key{};std::array<Row,4> rows{};std::array<unsigned,2> calls{};bool overflow{};};
struct Active {Report* output{};Active* previous{};};
inline thread_local Active* active{};
inline bool armed()noexcept{return active!=nullptr;}
class Scope {
    Active value_{};bool installed_{};
public:
    Scope(bool enabled,Key key,Report& report)noexcept{
        if(!enabled||!key.world||!key.pawn||!key.scene||!key.compose||!key.thread)return;
        report={};report.key=key;value_={&report,active};active=&value_;installed_=true;
    }
    ~Scope(){if(installed_)active=value_.previous;}
    Scope(Scope const&)=delete;Scope& operator=(Scope const&)=delete;
};
inline void note(unsigned stage,Row row)noexcept{
    if(!active||stage>1)return;
    auto& out=*active->output;auto const& key=out.key;
    if(row.world!=key.world||row.pawn!=key.pawn||row.scene!=key.scene||row.thread!=key.thread)return;
    auto& calls=out.calls[stage];if(calls==std::numeric_limits<unsigned>::max())out.overflow=true;else ++calls;
    row.observed=true;
    if(!out.rows[stage].observed)out.rows[stage]=row;
    if(row.verdict==1&&!out.rows[stage+2].observed)out.rows[stage+2]=row;
}
}
