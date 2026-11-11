from pathlib import Path
import json, hashlib, shutil, difflib, subprocess
A=Path(__file__).resolve().parent
R=A.parents[1]
BASE=A/'native-mutex-r1'
P=A/'native-passive-commit-r1'
REL='tools/native-capture/full_body_c0_native_live.inl'
DECL=r'''
// Receiver-local, first512 successful passive C0 commits. Original in-memory
// values only: no clock, query, allocation, input, source or write authority.
struct PassiveCommitObservation {
    uint64_t tick{},compose{},frame{},request_sequence{},current_sequence{};
    int64_t request_sync_ns{},current_sync_ns{};
    unsigned applied{},fitted{},solved_mask{},kind{};double weight{};
    gears_vr::Pose grip{},physical{},adjusted{},solved{},right{};
};
constexpr unsigned passive_commit_capacity=512;
std::array<PassiveCommitObservation,passive_commit_capacity> passive_commit_rows{};
std::array<std::atomic<bool>,passive_commit_capacity> passive_commit_ready{};
unsigned passive_commit_count{};
'''
CAPTURE=r'''
void observe_passive_commit() noexcept {
    auto const& w=state.hand_work;
    if(!w.left_visual.passive_socket||w.left_visual.kind!=gears_hand_object_grasp::Kind::support||
       passive_commit_count>=passive_commit_capacity)return;
    const unsigned i=passive_commit_count++;auto& row=passive_commit_rows[i];
    auto const& requested=lancer_grasp_live::requested.source;
    auto const& current=body_tick_source_live::enabled?state.context.body_source.observed:state.context.body_input;
    row.tick=state.context.tick;row.compose=state.context.copy.invocation;
    row.frame=requested.key.frame;row.request_sequence=requested.controllers.sequence;
    row.current_sequence=current.controllers.sequence;row.request_sync_ns=requested.controllers.sync_ns;
    row.current_sync_ns=current.controllers.sync_ns;row.applied=w.left_visual_applied;
    row.fitted=w.left_blend.position_fitted;row.solved_mask=w.solved_mask;
    row.kind=unsigned(w.left_blend.kind);row.weight=w.left_blend.weight;
    row.grip=w.left_visual.grip;row.physical=w.eye_targets[0].grip_world;
    row.adjusted={w.inputs[0].target.position,w.inputs[0].target.orientation};
    const auto left=body::d::pose(w.body_work.pose.world[11]);
    const auto right=body::d::pose(w.body_work.pose.world[35]);
    row.solved={left.position,left.orientation};row.right={right.position,right.orientation};
    passive_commit_ready[i].store(true,std::memory_order_release);
}
void summarize_passive_commits(FILE* f) noexcept {
    if(!f)return;
    for(unsigned i=0;i<passive_commit_capacity;++i){
        if(!passive_commit_ready[i].load(std::memory_order_acquire))continue;
        auto const& r=passive_commit_rows[i];
        fprintf(f,"body_passive_commit_%u=%llu,%llu,%llu,%llu,%llu,%lld,%lld,%u,%u,%u,%u,%.17g ",
            i,r.tick,r.compose,r.frame,r.request_sequence,r.current_sequence,r.request_sync_ns,
            r.current_sync_ns,r.applied,r.fitted,r.solved_mask,r.kind,r.weight);
        const gears_vr::Pose poses[]={r.grip,r.physical,r.adjusted,r.solved,r.right};
        for(unsigned n=0;n<5;++n){auto const& p=poses[n];
            fprintf(f,"body_passive_commit_%u_pose_%u=%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g ",
                i,n,p.position.x,p.position.y,p.position.z,p.orientation.x,p.orientation.y,p.orientation.z,p.orientation.w);
        }
    }
}
'''
def main():
    assert not P.exists(),'Preserve candidate identity'
    parent=json.loads((BASE/'prepared.json').read_text())
    shutil.copytree(BASE/'source/tools',P/'source/tools')
    shutil.copytree(BASE/'dependency',P/'dependency')
    path=P/'source'/REL;before=path.read_text();s=before
    anchors=[('std::array<std::atomic<uint64_t>,5> contextual_grasp_commits{};',DECL,False),
             ('void trace_hands_commit() noexcept {',CAPTURE,True),
             ('if(state.right_mount==hands::adapter::MountKind::held_lancer)++main_mount_commits;trace_hands_commit();','observe_passive_commit();',True),
             ('void summary(FILE* f){export_pending();','summarize_passive_commits(f);',False)]
    for anchor,addition,precede in anchors:
        assert s.count(anchor)==1,anchor
        s=s.replace(anchor,addition+anchor if precede else anchor+addition)
    path.write_text(s)
    # Exact removal restores every original production byte (text newline policy).
    reversed_source=s
    for anchor,addition,precede in reversed(anchors):
        reversed_source=reversed_source.replace(addition+anchor if precede else anchor+addition,anchor)
    assert reversed_source==before
    (R/'docs/hammerburst-native-passive-commit.patch').write_text(''.join(difflib.unified_diff(before.splitlines(True),s.splitlines(True),fromfile='a/'+REL,tofile='b/'+REL)))
    C=P/'project';C.mkdir()
    (C/'CMakeLists.txt').write_text((BASE/'project/CMakeLists.txt').read_text().replace(str(BASE).replace('\\','/'),str(P).replace('\\','/')))
    commands=[list(c) for c in parent['commands']]
    commands[0][commands[0].index('-S')+1]=str(C)
    commands[0][commands[0].index('-B')+1]=str(P/'build')
    commands[1][commands[1].index('--build')+1]=str(P/'build')
    (P/'prebuild.json').write_text(json.dumps(dict(parent=parent,commands=commands,exact_policy_reversal=True),indent=2))
    for label,cmd in zip(('configure','build'),commands):
        with (P/(label+'.log')).open('wb') as log:
            subprocess.run(cmd,check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
    dll=P/'build/Release/GearsCapture6DofM1.dll'
    proof=dict(parent_native=parent['native_sha256'],frozen_native=parent['frozen_native'],native_sha256=hashlib.sha256(dll.read_bytes()).hexdigest(),dll=str(dll),full_native_compiled=True,commands=commands,original_compile_definitions=parent['original_compile_definitions'],exact_policy_reversal=True,runtime_exercised=False,gameplay_accepted=False,root_native_modified=False,xr_changed=False,scope='First512 original successful passive C0 commits; source/request, physical grip, target, solved world left/right wrists, original blend state. No new clock/read/writer/input/gate.')
    (P/'prepared.json').write_text(json.dumps(proof,indent=2))
    print(proof['native_sha256'])
if __name__=='__main__':main()
