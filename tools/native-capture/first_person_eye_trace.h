#pragma once
#include "anchor_observer.h"
#include "cover_pose_phase.h"
#include "../vr-common/view_origin.h"
#include <cstdio>
#include <type_traits>

// Optional evidence from the EXISTING eye read/prepare callback. This is a
// bounded CPU read bracket, not an atomic engine pose, GPU draw, collision or
// body-movement certificate. It never supplies a view or a gameplay decision.
namespace gears_first_person_eye_trace {
namespace raw=gears_cover_pose_phase;
inline constexpr unsigned bones=115,capacity=512;
enum class Status:std::uint32_t {not_read,read_failed,reference_changed,payload_changed,nonfinite,ready};
struct Geometry {
    Status status{};std::uint32_t owner_closed{},tick_epoch{},pose_epoch{};
    std::uint64_t world_generation{};
    gears_anchor::Identity mesh{},asset{};
    raw::ArrayHeader reference_header{},space_header{};
    raw::Matrix component{};
    std::array<double,3> mount{};
    std::array<raw::RefBone,bones> reference{};
    std::array<raw::Matrix,bones> space{};
};
struct Row {
    std::uint64_t scene{};std::uint32_t thread{},eye_accepted{},view_prepared{};
    std::int64_t scene_qpc{},family_qpc{},renderer_qpc{},begin_qpc{},end_qpc{};
    std::uintptr_t family{},source{},local{};
    gears_anchor::Identity pawn{};
    std::array<float,3> pawn_location{};
    double units_per_metre{};
    gears_vr::StereoMatrix incoming_view{},prepared_view{};
    gears_vr::Vector eye{};
    Geometry geometry{};
};
static_assert(std::is_trivially_copyable_v<Row>);
struct Header {
    char magic[8]{'E','Y','E','G','E','O','1',0};
    std::uint32_t version=1,row_bytes=sizeof(Row),count{},capacity_limit=capacity;
    std::uint64_t due{},overflow{};
    std::int64_t qpc_frequency{};
};

// No allocation, wall-clock freshness inference, ring overwrite or silent
// truncation. Sampling at most 10 Hz gives a <=51.2 s first-window record.
// Full/saturated is explicit; a later run cannot silently replace old rows.
struct Journal {
    std::array<Row,capacity> rows{};unsigned count{};
    std::uint64_t due_count{},overflow{},last_ms{};bool sampled{};
    bool due(bool enabled,std::uint64_t ms){
        if(!enabled)return false;
        if(sampled&&(ms<last_ms||ms-last_ms<100))return false;
        sampled=true;last_ms=ms;++due_count;
        if(count==capacity){++overflow;return false;}return true;
    }
    bool append(Row const& row){
        if(count==capacity){++overflow;return false;}
        rows[count++]=row;return true;
    }
};

// Exact opt-in; invalid configuration cannot enable a diagnostic by accident.
inline bool option(wchar_t const* value,bool simulator,bool& enabled){
    if(!value||!*value||(value[0]==L'0'&&!value[1])){enabled=false;return true;}
    if(value[0]==L'1'&&!value[1]&&simulator){enabled=true;return true;}
    return false;
}

template<class Read,class Reference> inline void capture(Read& read,Geometry& out,std::uintptr_t mesh,
    std::uintptr_t asset,raw::ArrayHeader reference,raw::ArrayHeader space,
    raw::Matrix const& component,raw::Matrix const& head,
    std::array<Reference,bones> const& cached_reference,std::array<double,3> mount){
    static_assert(sizeof(Reference)==sizeof(raw::RefBone));
    out.status=Status::read_failed;
    if(reference.count!=int(bones)||space.count!=int(bones)||
       !raw::header_valid(reference,sizeof(raw::RefBone))||!raw::header_valid(space,sizeof(raw::Matrix)))return;
    out.reference_header=reference;out.space_header=space;out.component=component;out.mount=mount;
    std::uint32_t tick{},pose{};raw::ArrayHeader end_ref{},end_space{};raw::Matrix end_component{};
    if(!raw::field(read,out.tick_epoch,mesh,0x334)||!raw::field(read,out.pose_epoch,mesh,0x338)||
       !raw::bytes(read,out.reference.data(),reference.data,sizeof out.reference)||
       !raw::bytes(read,out.space.data(),space.data,sizeof out.space))return;
    if(std::memcmp(out.reference.data(),cached_reference.data(),sizeof out.reference)){
        out.status=Status::reference_changed;return;
    }
    if(!raw::finite(component)){out.status=Status::nonfinite;return;}
    for(auto const& matrix:out.space)if(!raw::finite(matrix)){out.status=Status::nonfinite;return;}
    const auto repeated_reference=raw::repeat(read,out.reference.data(),reference.data,sizeof out.reference);
    const auto repeated_space=raw::repeat(read,out.space.data(),space.data,sizeof out.space);
    if(repeated_reference==raw::Reject::read||repeated_space==raw::Reject::read)return;
    if(repeated_reference!=raw::Reject::none||repeated_space!=raw::Reject::none||
       std::memcmp(&head,&out.space[55],sizeof head)){
        out.status=Status::payload_changed;return;
    }
    if(!raw::field(read,tick,mesh,0x334)||!raw::field(read,pose,mesh,0x338)||
       !raw::field(read,end_ref,asset,0xa4)||!raw::field(read,end_space,mesh,0x26c)||
       !raw::field(read,end_component,mesh,0xa0))return;
    if(tick!=out.tick_epoch||pose!=out.pose_epoch||!raw::header_equal(reference,end_ref)||
       !raw::header_equal(space,end_space)||std::memcmp(&component,&end_component,sizeof component)){
        out.status=Status::payload_changed;return;
    }
    out.status=Status::ready;
}

// Called only by the existing drained log exporter. Raw identities/addresses
// are observations for this process/session, never reusable runtime handles.
inline bool write(FILE* file,Journal const& journal,std::int64_t frequency){
    if(!file||frequency<=0)return false;
    Header header{};header.count=journal.count;header.due=journal.due_count;
    header.overflow=journal.overflow;header.qpc_frequency=frequency;
    return fwrite(&header,sizeof header,1,file)==1&&
        fwrite(journal.rows.data(),sizeof(Row),journal.count,file)==journal.count&&!ferror(file);
}
}
