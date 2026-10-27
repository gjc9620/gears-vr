#pragma once
#include "body_tick_input_live.h"
#include "full_body_c0_planner.h"
#include "../vr-common/weapon_main_mount_provider.h"
namespace gears_body_tick_commit {
struct Record {
    gears_body_tick_input::Input input{};
    body_tick_input_live::Parent parent{};
    std::uint64_t sequence{},tick{},compose{};std::int64_t committed_ns{},capture_ns{};
    gears_vr::StereoMatrix base_view{};
    gears_full_body_c0::Matrix64 component{};
    gears_full_body_c0::rig::Signature geometry{};
    gears_weapon_hand::ObjectIdentity avatar_asset{};
    gears_weapon_main_mount_provider::Response mount{};
};
struct Header {
    char magic[8]{'B','T','I','C','M','T','0','1'};
    std::uint32_t version{1},header_bytes{sizeof(Header)},record_bytes{sizeof(Record)},count{},capacity{32},reserved{};
    std::uint64_t pid{},native_started{},readonly{1},writer{};
};
static_assert(sizeof(Header)==64&&std::is_trivially_copyable_v<Record>);
}
