#pragma once
#include "body_tick_source.h"
#include "../vr-common/body_main_mount_provider.h"
#include <cstdio>
namespace body_tick_source_live {
namespace source=gears_body_tick_source;
namespace wire=gears_body_main_mount_provider;
// Native-owned metadata only. No callback pointer or permission is cached.
struct Parent {
    std::uint64_t cookie{},native_started{},revision{},generation{},query{},tick{},compose{};
    std::int64_t equipment_begin_ns{},equipment_end_ns{};
};
#if defined(GEARS_BODY_MAIN_MOUNT_SOURCE) && GEARS_BODY_MAIN_MOUNT_SOURCE
extern bool enabled;
bool configure(wchar_t const*,FILE*,bool);
void begin(std::uint64_t,std::uint64_t,gears_native_rig_tracking::CoreSnapshot const&,std::uint64_t)noexcept;
void end(std::uint64_t)noexcept;
bool copy(std::uint64_t,std::uint64_t,gears_native_rig_tracking::CoreSnapshot const&,source::Input&)noexcept;
gears_body_tick_input::Verdict current(source::Input const&,gears_native_rig_tracking::CoreSnapshot const&,std::uint64_t,std::uint64_t,std::int64_t)noexcept;
bool retained_current(source::Input const&,gears_native_rig_tracking::CoreSnapshot const&,std::uint64_t,std::uint64_t)noexcept;
bool parent_current(Parent const&)noexcept;
bool parent_idle_current(Parent const&)noexcept;
void summary(FILE*)noexcept;
#else
inline constexpr bool enabled=false;
inline bool configure(wchar_t const* ini,FILE*,bool){return GetPrivateProfileIntW(L"BodyMainMountSource",L"enabled",0,ini)==0;}
inline void begin(std::uint64_t,std::uint64_t,gears_native_rig_tracking::CoreSnapshot const&,std::uint64_t)noexcept{}
inline void end(std::uint64_t)noexcept{}
inline bool copy(std::uint64_t,std::uint64_t,gears_native_rig_tracking::CoreSnapshot const&,source::Input&)noexcept{return false;}
inline gears_body_tick_input::Verdict current(source::Input const&,gears_native_rig_tracking::CoreSnapshot const&,std::uint64_t,std::uint64_t,std::int64_t)noexcept{return gears_body_tick_input::Verdict::hard;}
inline bool retained_current(source::Input const&,gears_native_rig_tracking::CoreSnapshot const&,std::uint64_t,std::uint64_t)noexcept{return false;}
inline bool parent_current(Parent const&)noexcept{return false;}
inline bool parent_idle_current(Parent const&)noexcept{return false;}
inline void summary(FILE*)noexcept{}
#endif
}
