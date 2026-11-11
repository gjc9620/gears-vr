"""Respect the original Gate's age-only continuity; never accept stale output."""
from pathlib import Path
import json,hashlib,shutil,difflib,subprocess
A=Path(__file__).resolve().parent;R=A.parents[1];BASE=A/'native-input-loss-r1';P=A/'native-render-age-r1';S=P/'source'
POLICY='''#pragma once
#include "../vr-common/stereo_frame.h"
namespace gears_stereo_hand_age {
// Decides ONLY whether to keep logical registration/debt for a future fresh
// Scene. It supplies no pose, Frame, input, lease, clock or dispatch authority.
inline bool preserve(gears_vr::StereoSample const& sample,std::int64_t now,
    gears_vr::StereoResult const& result,gears_vr::ReferenceContinuity before,
    gears_vr::ReferenceContinuity after) noexcept {
    return result.status==gears_vr::StereoStatus::stale&&sample.focused==1&&
        before.epoch&&before.epoch==after.epoch&&before.stable&&after.stable&&
        !before.pending&&!after.pending&&!before.exhausted&&!after.exhausted&&
        sample.received_ns>0&&now>=sample.received_ns&&now-sample.received_ns>100000000;
}
}
'''
def main():
 assert not P.exists(),'Preserve candidate'
 parent=json.loads((BASE/'prepared.json').read_text());assert parent['full_native_compiled'] and parent['broker_failures']==0
 shutil.copytree(BASE/'source/tools',S/'tools');shutil.copytree(BASE/'dependency',P/'dependency')
 header='tools/native-capture/stereo_hand_age_policy.h';(S/header).write_text(POLICY)
 rel='tools/native-capture/capture.cpp';before=(S/rel).read_text();after=before
 needle='std::array<unsigned long long,11> stereo_rejections{};'
 extra='''
struct StereoHandLossObservation {
    unsigned reason{},kept{},focused{};std::uint64_t ordinal{},session{},sequence{},space{};
    std::int64_t now_ns{},received_ns{};gears_vr::ReferenceContinuity before{},after{};
};
std::array<StereoHandLossObservation,32> stereo_hand_losses{};
std::uint64_t stereo_hand_loss_count{},stereo_hand_age_kept{};
// Existing stereo_mutex protects both append and print_stereo readback.
void record_stereo_hand_loss(unsigned reason,bool kept,gears_vr::StereoSample const& sample,
    std::int64_t stamp,gears_vr::ReferenceContinuity before,gears_vr::ReferenceContinuity after) noexcept {
    if(stereo_hand_loss_count==UINT64_MAX)return;
    const auto ordinal=++stereo_hand_loss_count;
    stereo_hand_losses[(ordinal-1)%stereo_hand_losses.size()]={reason,unsigned(kept),sample.focused,
        ordinal,sample.session,sample.sequence,sample.space_epoch,stamp,sample.received_ns,before,after};
    if(kept&&stereo_hand_age_kept!=UINT64_MAX)++stereo_hand_age_kept;
}
'''
 assert after.count(needle)==1;after=after.replace(needle,needle+extra)
 # Include after an existing include, outside any capture namespace.
 after='#include "stereo_hand_age_policy.h"\n'+after
 old='''if(!stereo_bridge.read(sample)){++stereo_rejections[8];if(weapon_hand_runtime::enabled)cover_shared_input_live::invalidate(gears_weapon_hand::InvalidateReason::input_lost);return false;}'''
 new='''if(!stereo_bridge.read(sample)){++stereo_rejections[8];
            const auto continuity=stereo_gate.continuity();record_stereo_hand_loss(8,false,sample,0,continuity,continuity);
            if(weapon_hand_runtime::enabled)cover_shared_input_live::invalidate(gears_weapon_hand::InvalidateReason::input_lost);return false;}'''
 assert after.count(old)==1;after=after.replace(old,new)
 old='''        auto result=stereo_gate.update(sample,sample_time,simulator_operator);'''
 new='''        const auto continuity_before=stereo_gate.continuity();
        auto result=stereo_gate.update(sample,sample_time,simulator_operator);'''
 assert after.count(old)==1;after=after.replace(old,new)
 old='''        if(!result){++stereo_rejections[static_cast<unsigned>(result.status)];
            if(weapon_hand_runtime::enabled)cover_shared_input_live::invalidate(gears_weapon_hand::InvalidateReason::input_lost);'''
 new='''        if(!result){++stereo_rejections[static_cast<unsigned>(result.status)];
            const auto continuity_after=stereo_gate.continuity();
            const bool age_only=gears_stereo_hand_age::preserve(sample,sample_time,result,continuity_before,continuity_after);
            record_stereo_hand_loss(unsigned(result.status),age_only,sample,sample_time,continuity_before,continuity_after);
            if(weapon_hand_runtime::enabled&&!age_only)cover_shared_input_live::invalidate(gears_weapon_hand::InvalidateReason::input_lost);'''
 assert after.count(old)==1;after=after.replace(old,new)
 needle='''    fprintf(file,"stereo_recenter policy=local_y_up_heading'''
 extra='''    fprintf(file,"stereo_hand_loss_summary total=%llu age_only_kept=%llu retained=%zu\\n",stereo_hand_loss_count,stereo_hand_age_kept,(std::min)(stereo_hand_loss_count,std::uint64_t(stereo_hand_losses.size())));
    const auto loss_first=stereo_hand_loss_count>(stereo_hand_losses.size())?stereo_hand_loss_count-stereo_hand_losses.size():0;
    for(auto n=loss_first;n<stereo_hand_loss_count;++n){const auto& r=stereo_hand_losses[n%stereo_hand_losses.size()];
        fprintf(file,"stereo_hand_loss ordinal=%llu reason=%u kept=%u focused=%u session=%llu sequence=%llu space=%llu now_ns=%lld received_ns=%lld before_epoch=%llu before_stable=%u before_pending=%u before_exhausted=%u after_epoch=%llu after_stable=%u after_pending=%u after_exhausted=%u\\n",
            r.ordinal,r.reason,r.kept,r.focused,r.session,r.sequence,r.space,r.now_ns,r.received_ns,r.before.epoch,unsigned(r.before.stable),unsigned(r.before.pending),unsigned(r.before.exhausted),r.after.epoch,unsigned(r.after.stable),unsigned(r.after.pending),unsigned(r.after.exhausted));}
'''
 assert after.count(needle)==1;after=after.replace(needle,extra+needle);(S/rel).write_text(after)
 (R/'docs/hammerburst-native-render-age.patch').write_text(''.join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='a/'+rel,tofile='b/'+rel)))
 C=P/'project';C.mkdir();cmake=(BASE/'project/CMakeLists.txt').read_text().replace(str(BASE).replace('\\','/'),str(P).replace('\\','/'));(C/'CMakeLists.txt').write_text(cmake)
 commands=[['E:/CMake/bin/cmake.exe','-S',str(C),'-B',str(P/'build'),'-G','Visual Studio 18 2026','-A','x64','-DCMAKE_GENERATOR_INSTANCE=E:/vs'],['E:/CMake/bin/cmake.exe','--build',str(P/'build'),'--config','Release','--target','GearsCapture','--parallel','2']]
 for label,cmd in zip(('configure','build'),commands):
  with (P/(label+'.log')).open('wb') as log:subprocess.run(cmd,check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
 dll=P/'build/Release/GearsCapture6DofM1.dll';proof=dict(parent_native=parent['native_sha256'],frozen_native=parent['parent_native'],native_sha256=hashlib.sha256(dll.read_bytes()).hexdigest(),dll=str(dll),full_native_compiled=True,source_changed=[rel,header],source_sha256={n:hashlib.sha256((S/n).read_bytes()).hexdigest() for n in (rel,header)},commands=commands,original_compile_definitions=parent['original_compile_definitions'],runtime_exercised=False,gameplay_accepted=False,root_native_modified=False,xr_changed=False,scope='Gate-certified focused age-only rejection retains logical ancestry only; rejected render still returnsfalse. Bridge failure and broken/pending/new origin/future clock/real tracking/session failure keep original invalidation. Original100ms/source/action guards unchanged;32 passive original-timestamp records.')
 (P/'prepared.json').write_text(json.dumps(proof,indent=2));print(proof['native_sha256'])
if __name__=='__main__':main()
