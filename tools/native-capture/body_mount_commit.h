#pragma once
#include "body_tick_source_live.h"
#include "../vr-common/body_main_mount_provider.h"
// Bounded drained observation only. This is a different record from the old
// TickInput contract: no reader may interpret it as an old V1 fresh sample.
namespace gears_body_mount_commit {
struct Record {
    std::uint64_t sequence{},tick{},compose{};
    std::int64_t committed_ns{},capture_ns{};
    gears_body_tick_source::Input input{};
    body_tick_source_live::Parent parent{},queried_parent{};
    gears_body_main_mount_provider::Source query_source{};
    gears_body_main_mount_provider::Response response{};
    gears_vr::StereoMatrix base_view{},component{};
    gears_weapon_main_mount_provider::Signature geometry{};
    gears_weapon_hand::ObjectIdentity avatar_asset{};
};
struct Header {
    char magic[8]{'B','M','S','C','O','M','0','2'};
    std::uint32_t version{2},header_bytes{sizeof(Header)},record_bytes{sizeof(Record)},count{},capacity{32},reserved{};
    std::uint64_t pid{},native_started{},read_only{1},writer{};
};
static_assert(sizeof(Header)==64&&std::is_standard_layout_v<Record>&&std::is_trivially_copyable_v<Record>);
}
