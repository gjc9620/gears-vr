// Exercise the production OFF branch in capture's anonymous namespace shape.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define GEARS_FULL_BODY_C0_NATIVE_BACKEND 0
#include <windows.h>
#include <intrin.h>
#include <cstdio>
#include <memory>
#include "held_outcome_journal.h"
#include "ready_wrist_entry_journal.h"
#include "full_body_c0_transaction.h"
#include "body_mount_commit.h"
#include "native_atom_overwrite_receipt.h"
#include "full_body_c0_first_failure.h"
#include "body_c0_arm_alpha_policy.h"
#include "body_head_authored_mount.h"
#include "lancer_saw_pose_policy.h"
#include "../vr-common/weapon_main_mount_provider.h"
#include "continuous_hand_owner_entry.h"
#include "../vr-common/committed_hand_endpoint.h"
#include "../vr-common/committed_hand_interaction.h"
#include "../vr-common/weapon_lancer_held_contact.h"
namespace {
namespace continuous_hand_owner_live {}
namespace cover_pose_phase_live {struct Scope;}
namespace weapon_hand_scene_binding {}
#include "full_body_c0_native_live.h"
#include "full_body_c0_native_live.inl"
int run_off_test(){
    namespace live=full_body_c0_native_live;using Pose=gears_full_body_c0_native::ConsumedPose;
    gears_continuous_hand_entry::Entry entry{};gears_native_rig_tracking::CoreSnapshot source{};
    auto out=std::make_unique<Pose>();out->observed_qpc=123;const auto before=*out;
    bool okay=!live::enabled&&!live::read_completed_cover(entry,source,*out)&&
        !std::memcmp(out.get(),&before,sizeof before)&&live::pending_count()==0;
    live::pose_begin(entry);live::pose_end(entry,true,true);live::request_drain();
    okay=okay&&!live::read_completed_cover(entry,source,*out)&&!std::memcmp(out.get(),&before,sizeof before)&&!live::pending_count();
    std::printf("Cover completed OFF: %s; no observation, hooks, write or debt; anonymous capture namespace\n",okay?"PASS":"FAIL");
    return okay?0:1;
}

}
int main(){return run_off_test();}
