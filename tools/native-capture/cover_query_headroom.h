#pragma once
#include <cstdint>
#include <cstddef>
#include <limits>
// Admission arithmetic only. Original timestamps are immutable inputs, never
// refreshed or converted into a lease. A ready result does not certify the
// source or guarantee completion; all ordinary final checks still apply.
namespace gears_cover_query_headroom {
struct Window {std::int64_t born_ns{};std::uint64_t maximum_age_ns{};};
enum class Code:unsigned {invalid,expired,defer,ready};
struct Report {Code code{};std::int64_t now_ns{},limiting_born_ns{};
    std::uint64_t reserve_ns{},remaining_ns{};unsigned limiting{};};
inline Report assess(std::int64_t now,Window const* windows,std::size_t count,std::uint64_t reserve)noexcept{
    Report result{};result.now_ns=now;result.reserve_ns=reserve;
    if(now<=0||!windows||!count||count>16||!reserve)return result;
    result.remaining_ns=std::numeric_limits<std::uint64_t>::max();bool expired=false;
    for(std::size_t i=0;i<count;++i){const auto& w=windows[i];
      if(w.born_ns<=0||w.born_ns>now||!w.maximum_age_ns){result.code=Code::invalid;return result;}
      const auto age=std::uint64_t(now)-std::uint64_t(w.born_ns);
      const auto remaining=age>w.maximum_age_ns?0:w.maximum_age_ns-age;
      expired|=age>w.maximum_age_ns;
      if(i==0||remaining<result.remaining_ns){result.remaining_ns=remaining;result.limiting=unsigned(i);result.limiting_born_ns=w.born_ns;}
    }
    result.code=expired?Code::expired:result.remaining_ns<reserve?Code::defer:Code::ready;return result;
}
}
