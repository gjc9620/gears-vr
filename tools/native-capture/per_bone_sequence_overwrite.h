#pragma once
#include "per_bone_overwrite_source.h"
#include "sequence_pose_overwrite_source.h"

// Attested FACB0 FADBE directly forwards child0 into the canonical output when
// no upper child is active. A noncached F8AF4 child supplies full stores via
// its original decoder/reference path. Forwarding alone supplies no coverage.
namespace gears_per_bone_sequence_overwrite {
namespace pb=gears_per_bone_overwrite;
namespace seq=gears_sequence_pose_overwrite;
inline bool starts(pb::Source const& parent,seq::Source const& child) noexcept {
    return pb::base_only(parent)&&parent.child[0].node==child.node&&
        parent.mesh==child.mesh&&parent.mesh_epoch==child.mesh_epoch&&
        child.node_epoch<child.mesh_epoch&&
        pb::pose::header_equal(parent.local,child.local)&&
        pb::pose::header_equal(parent.desired,child.desired);
}
inline bool returned(pb::Source const& before,pb::Source const& after,
    seq::Source const& child_before,seq::Source const& child_after) noexcept {
    return starts(before,child_before)&&pb::same(before,after,false)&&
        after.epoch==before.mesh_epoch&&seq::returned(child_before,child_after);
}
}
