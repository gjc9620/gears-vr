#pragma once
#include "body_tick_input.h"
#include <cstdio>
namespace body_tick_input_live {
namespace input=gears_body_tick_input;
// POD selectors only. PG module pointers and permissions are never retained.
struct Parent {std::uint64_t cookie{},native_started{},revision{},generation{},query{},tick{},compose{};};
#if defined(GEARS_BODY_TICK_INPUT) && GEARS_BODY_TICK_INPUT
extern bool enabled;
bool configure(wchar_t const*,FILE*,bool);
void begin(std::uint64_t,std::uint64_t,gears_native_rig_tracking::CoreSnapshot const&)noexcept;
void end(std::uint64_t)noexcept;
bool copy(std::uint64_t,std::uint64_t,gears_native_rig_tracking::CoreSnapshot const&,input::Input&)noexcept;
input::Verdict current(input::Input const&,gears_native_rig_tracking::CoreSnapshot const&,std::uint64_t,std::uint64_t,std::int64_t)noexcept;
bool parent_snapshot(input::Input const&,std::uint64_t,Parent&)noexcept;
bool parent_current(Parent const&)noexcept;
void summary(FILE*)noexcept;
void close_after_drain()noexcept;
#else
inline constexpr bool enabled=false;
inline bool configure(wchar_t const* ini,FILE*,bool){return GetPrivateProfileIntW(L"BodyTickInput",L"enabled",0,ini)==0;}
inline void begin(std::uint64_t,std::uint64_t,gears_native_rig_tracking::CoreSnapshot const&)noexcept{}
inline void end(std::uint64_t)noexcept{}
inline bool copy(std::uint64_t,std::uint64_t,gears_native_rig_tracking::CoreSnapshot const&,input::Input&)noexcept{return false;}
inline input::Verdict current(input::Input const&,gears_native_rig_tracking::CoreSnapshot const&,std::uint64_t,std::uint64_t,std::int64_t)noexcept{return input::Verdict::hard;}
inline bool parent_snapshot(input::Input const&,std::uint64_t,Parent&)noexcept{return false;}
inline bool parent_current(Parent const&)noexcept{return false;}
inline void summary(FILE*)noexcept{}
inline void close_after_drain()noexcept{}
#endif
}
