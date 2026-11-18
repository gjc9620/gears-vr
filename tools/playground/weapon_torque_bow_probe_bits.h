#pragma once
#include <cstdint>
namespace gears_torque_bow_probe_bits {
struct Sample {std::uintptr_t frame{},state{};std::uint64_t mask{};};
inline bool same(Sample const& a,Sample const& b) noexcept {
 return a.frame==b.frame&&a.state==b.state&&a.mask==b.mask;
}
template<class Reader>bool read(Reader& r,std::uintptr_t actor,unsigned tick_name,Sample& out) noexcept {
 out={};if(!actor||tick_name<300||tick_name>363)return false;
 Sample a{},b{};
 if(!r.at(actor,0x24,a.frame)||!a.frame||!r.at(a.frame,0x48,a.state)||!a.state||
    !r.at(a.frame,0x50,a.mask)||!r.at(actor,0x24,b.frame)||!b.frame||
    !r.at(b.frame,0x48,b.state)||!r.at(b.frame,0x50,b.mask)||!same(a,b))return false;
 out=a;return true;
}
inline bool enabled(Sample const& s,unsigned tick_name) noexcept {
 return s.frame&&s.state&&tick_name>=300&&tick_name<=363&&(s.mask&(std::uint64_t{1}<<(tick_name-300)))!=0;
}
}
