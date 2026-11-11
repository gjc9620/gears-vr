#pragma once
#include <array>
#include <cstdint>
#include <limits>
#include <utility>

// Opt-in synchronous observation only. No game reads, guard decisions or
// permissions. Nested durations overlap; totals must never be added together.
namespace gears_cover_query_timing {
enum class Stage:unsigned {physics_initial,collider_initial,components_initial,
    physics_repeat,collider_repeat,components_repeat,engine,closing,owner_bracket,count};
struct Bin {std::uint64_t calls{},completed{},total_ns{},max_ns{};};
struct Report {std::array<Bin,unsigned(Stage::count)> bins{};std::uint64_t invalid{},overflow{};};
using Clock=std::int64_t(*)()noexcept;
struct Active {Report* report{};Clock clock{};Active* previous{};};
inline thread_local Active* active{};
class Scope {
    Active value_{};bool installed_{};
public:
    Scope(bool enabled,Report& report,Clock clock)noexcept {
        if(!enabled||!clock)return;
        value_={&report,clock,active};active=&value_;installed_=true;
    }
    ~Scope(){if(installed_)active=value_.previous;}
    Scope(Scope const&)=delete;Scope& operator=(Scope const&)=delete;
};
inline bool add(std::uint64_t& target,std::uint64_t value,Report& report)noexcept {
    if(value>std::numeric_limits<std::uint64_t>::max()-target){++report.overflow;return false;}
    target+=value;return true;
}
class Span {
    Active* owner_{};Bin* bin_{};std::int64_t begin_{};
public:
    explicit Span(Stage stage)noexcept {
        if(!active||unsigned(stage)>=unsigned(Stage::count))return;
        owner_=active;bin_=&owner_->report->bins[unsigned(stage)];
        add(bin_->calls,1,*owner_->report);begin_=owner_->clock();
    }
    ~Span(){
        if(!owner_)return;
        const auto end=owner_->clock();auto& report=*owner_->report;
        if(begin_<=0||end<begin_){++report.invalid;return;}
        const auto duration=std::uint64_t(end-begin_);
        add(bin_->completed,1,report);add(bin_->total_ns,duration,report);
        if(duration>bin_->max_ns)bin_->max_ns=duration;
    }
    Span(Span const&)=delete;Span& operator=(Span const&)=delete;
};
template<class Operation> decltype(auto) measure(Stage stage,Operation&& operation){
    Span span(stage);return std::forward<Operation>(operation)();
}
}
