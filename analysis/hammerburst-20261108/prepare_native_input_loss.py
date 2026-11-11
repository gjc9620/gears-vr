"""Passive input-loss classification on exact frozen806dcbd6 Native source."""
from pathlib import Path
import hashlib,json,shutil,subprocess,difflib
A=Path(__file__).resolve().parent;R=A.parents[1]
BASE=R/'analysis/solo-m1-20261028/hammerburst-prepared-20261110-return-observation-r1/native-source'
P=A/'native-input-loss-r1';S=P/'source'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def once(s,a,b):assert s.count(a)==1,a[:80];return s.replace(a,b)
def main():
 assert not P.exists(),'Preserve named candidate'
 parent=json.loads((BASE.parent/'native-binding-build.json').read_text())
 assert parent['sha256']=='806dcbd6a92ecd337923f4ecf315e56ab7afd475e247a8a3cacdfe2ab30b8b0c'
 assert sha(BASE/'tools/native-capture/capture.cpp')==parent['sources']['tools\\native-capture\\capture.cpp']
 shutil.copytree(BASE/'tools',S/'tools')
 rel='tools/native-capture/weapon_hand_broker.h';before=(S/rel).read_text();after=before
 after=once(after,'class Broker {','''class Broker {
public:
    struct InputLossObservation {
        unsigned stage{},flags{};std::uint64_t frame{},sequence{};
        std::int64_t now_ns{},frame_ns{},sync_ns{},controller_ns{},stereo_ns{};
    };
private:
    std::array<InputLossObservation,16> input_losses_{};
    std::uint64_t input_loss_count_{};
    // Caller already holds mutex_. No additional clock, input, callback or permit.
    void record_input_loss(Frame const& f,std::int64_t stamp,unsigned stage) noexcept {
        if(input_loss_count_==UINT64_MAX)return;
        input_losses_[input_loss_count_++%input_losses_.size()]={stage,f.flags,f.key.frame,f.controllers.sequence,
            stamp,f.now_ns,f.controllers.sync_ns,f.controllers.received_ns,f.stereo.received_ns};
    }
    bool observed_active_now(Frame const& f,std::int64_t stamp,unsigned stage) noexcept {
        const bool accepted=active_now(f,stamp);
        if(!accepted)record_input_loss(f,stamp,stage);
        return accepted;
    }''')
 after=once(after,'else if(!active_now(local,backend_.clock_ns(backend_.context)))','else if(!observed_active_now(local,backend_.clock_ns(backend_.context),1))')
 after=once(after,'else if(!active_now(local,stamp))','else if(!observed_active_now(local,stamp,2))')
 after=once(after,'if(stamp<=0){invalidate_locked(InvalidateReason::input_lost,false,invalidation);','if(stamp<=0){record_input_loss(local,stamp,3);invalidate_locked(InvalidateReason::input_lost,false,invalidation);')
 needle='invalidate_locked(reason,reason==InvalidateReason::native_stopping||reason==InvalidateReason::unregistering||reason==InvalidateReason::callback_fault,call);'
 after=once(after,needle,'if(reason==InvalidateReason::input_lost)record_input_loss(attempt_,0,4);\n            '+needle)
 needle='    // Caller holds its Scene admission/stereo lock and uses one immutable Frame'
 getter='''    std::uint64_t input_loss_observations(std::array<InputLossObservation,16>& out,unsigned& count) {
        std::lock_guard<std::mutex> lock(mutex_);out={};
        count=unsigned((std::min)(input_loss_count_,std::uint64_t(out.size())));
        const auto first=input_loss_count_-count;
        for(unsigned i=0;i<count;++i)out[i]=input_losses_[(first+i)%input_losses_.size()];
        return input_loss_count_;
    }
'''
 after=once(after,needle,getter+needle);(S/rel).write_text(after)
 patches=list(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='a/'+rel,tofile='b/'+rel))
 rel2='tools/native-capture/weapon_hand_runtime.inl';before=(S/rel2).read_text()
 needle='    if(!file)return;auto* b=current();const auto binding=session();'
 extra='''
    if(b){std::array<api::Broker::InputLossObservation,16> rows{};unsigned count{};
        const auto total=b->input_loss_observations(rows,count);
        std::fprintf(file,"weapon_hand_input_loss_total=%llu weapon_hand_input_loss_rows=%u ",total,count);
        for(unsigned i=0;i<count;++i){const auto& r=rows[i];
            std::fprintf(file,"weapon_hand_input_loss_%u_stage=%u weapon_hand_input_loss_%u_frame=%llu weapon_hand_input_loss_%u_sequence=%llu weapon_hand_input_loss_%u_now_ns=%lld weapon_hand_input_loss_%u_frame_ns=%lld weapon_hand_input_loss_%u_sync_ns=%lld weapon_hand_input_loss_%u_controller_ns=%lld weapon_hand_input_loss_%u_stereo_ns=%lld weapon_hand_input_loss_%u_flags=%u ",
                i,r.stage,i,r.frame,i,r.sequence,i,r.now_ns,i,r.frame_ns,i,r.sync_ns,i,r.controller_ns,i,r.stereo_ns,i,r.flags);
        }
    }
'''
 after=once(before,needle,needle+extra);(S/rel2).write_text(after)
 patches.extend(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='a/'+rel2,tofile='b/'+rel2))
 (R/'docs/hammerburst-native-input-loss.patch').write_text(''.join(patches))
 # Reuse original production flags, source list and exact pinned dependency.
 dep=P/'dependency';shutil.copytree(A/'native-binding-dependency',dep)
 assert sha(dep/'minhook.x64.lib')==parent['minhook_library_sha256']
 assert sha(dep/'MinHook.h')==parent['minhook_header_sha256']
 for rel3,h in parent['sources'].items():
  target=S/rel3
  if not target.exists():shutil.copy2(A/'native-binding-source'/rel3,target)
  assert sha(target)==h,rel3
 cmake=(A/'native-binding-production/CMakeLists.txt').read_text()
 cmake=cmake.replace(str(A/'native-binding-source').replace('\\','/'),str(S).replace('\\','/')).replace(str(A/'native-binding-dependency').replace('\\','/'),str(dep).replace('\\','/'))
 C=P/'project';C.mkdir();(C/'CMakeLists.txt').write_text(cmake)
 commands=[['E:/CMake/bin/cmake.exe','-S',str(C),'-B',str(P/'build'),'-G','Visual Studio 18 2026','-A','x64','-DCMAKE_GENERATOR_INSTANCE=E:/vs'],['E:/CMake/bin/cmake.exe','--build',str(P/'build'),'--config','Release','--target','GearsCapture','--parallel','2']]
 for label,cmd in zip(('configure','build'),commands):
  with (P/(label+'.log')).open('wb') as log:subprocess.run(cmd,check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
 dll=P/'build/Release/GearsCapture6DofM1.dll'
 proof=dict(parent_native=parent['sha256'],native_sha256=sha(dll),dll=str(dll),full_native_compiled=True,source_changed=[rel,rel2],source_sha256={n:sha(S/n) for n in (rel,rel2)},commands=commands,original_compile_definitions=parent['definitions'],runtime_exercised=False,gameplay_accepted=False,root_native_modified=False,xr_changed=False,scope='16-entry bounded passive ring:1 prepare freshness,2 commit freshness,3 commit nonpositive clock,4 external input_lost with unknown timestamp0. Original active_now called once using original clock; original rejection/invalidation unchanged.')
 (P/'prepared.json').write_text(json.dumps(proof,indent=2));print(json.dumps(dict(native_sha256=proof['native_sha256'],source_changed=proof['source_changed'])))
if __name__=='__main__':main()
