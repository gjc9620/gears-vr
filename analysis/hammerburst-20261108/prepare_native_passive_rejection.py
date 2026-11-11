from pathlib import Path
import json,shutil,hashlib,difflib,subprocess
A=Path(__file__).resolve().parent;R=A.parents[1];BASE=A/'native-passive-commit-r1';P=A/'native-passive-rejection-r1'
BROKER='tools/native-capture/weapon_hand_broker.h';GRASP='tools/native-capture/lancer_grasp_live.inl'
OBS=r'''
// Receiver-local provenance from the original admission, never a permit.
struct SocketVisibilityObservation {
    unsigned gate{};uint64_t source_frame{},frozen_frame{},source_sequence{},frozen_sequence{};
    uint64_t source_cookie{},frozen_cookie{},source_lifecycle{},frozen_lifecycle{};
};
'''
DECL=r'''
struct PassiveRejection {
    unsigned gate{};uint64_t frame{},request_sequence{},current_sequence{};
    int64_t observed_ns{},request_sync_ns{};hand::SocketVisibilityObservation socket{};
};
std::array<PassiveRejection,64> passive_rejections{};
std::array<std::atomic<bool>,64> passive_rejection_ready{};unsigned passive_rejection_count{};
'''
LOG=r'''
    for(unsigned i=0;i<passive_rejections.size();++i){
        if(!passive_rejection_ready[i].load(std::memory_order_acquire))continue;
        auto const& r=passive_rejections[i];auto const& s=r.socket;
        fprintf(output,"body_passive_rejection_%u=%u,%llu,%llu,%llu,%lld,%lld,%u,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu ",
            i,r.gate,r.frame,r.request_sequence,r.current_sequence,r.observed_ns,r.request_sync_ns,
            s.gate,s.source_frame,s.frozen_frame,s.source_sequence,s.frozen_sequence,
            s.source_cookie,s.frozen_cookie,s.source_lifecycle,s.frozen_lifecycle);
    }
'''
RECORD=r'''
        if(requested.proximity&&passive_rejection_count<passive_rejections.size()){
            const unsigned index=passive_rejection_count++;
            passive_rejections[index]={gate,f.key.frame,f.controllers.sequence,current.sequence,now,f.controllers.sync_ns,socket_observation};
            passive_rejection_ready[index].store(true,std::memory_order_release);
        }
'''
def main():
    assert not P.exists(),'Preserve candidate identity'
    shutil.copytree(BASE/'source/tools',P/'source/tools');shutil.copytree(BASE/'dependency',P/'dependency')
    before={rel:(P/'source'/rel).read_text() for rel in (BROKER,GRASP)}
    s=before[BROKER];s=s.replace('struct MainMountQueryResult {',OBS+'struct MainMountQueryResult {',1)
    start=s.index('    bool admit_socket_visibility(Frame const& source) noexcept {')
    end=s.index('    bool admit_support_visibility(',start)
    old=s[start:end]
    expression=old.split('            return ',1)[1].split(';',1)[0]
    terms=expression.split('&&')
    checks='&&\n                '.join(f'check({term.strip()},{i+2})' for i,term in enumerate(terms))
    new='''    bool admit_socket_visibility(Frame const& source,SocketVisibilityObservation* observation=nullptr) noexcept {
        if(observation)*observation={};
        try {
            if(reentrant()||action_barrier()||!current_thread(source)){if(observation)observation->gate=1;return false;}
            std::lock_guard<std::mutex> lock(mutex_);
            if(observation){observation->source_frame=source.key.frame;observation->frozen_frame=frozen_.key.frame;
                observation->source_sequence=source.controllers.sequence;observation->frozen_sequence=frozen_.controllers.sequence;
                observation->source_cookie=source.key.registration_cookie;observation->frozen_cookie=frozen_.key.registration_cookie;
                observation->source_lifecycle=source.key.lifecycle;observation->frozen_lifecycle=frozen_.key.lifecycle;}
            const auto check=[&](bool value,unsigned gate) noexcept {if(!value&&observation)observation->gate=gate;return value;};
            return '''+checks+''';
        }catch(...){if(observation)observation->gate=100;return false;}
    }
'''
    s=s[:start]+new+s[end:];(P/'source'/BROKER).write_text(s)
    assert s.replace(new,old).replace(OBS,'')==before[BROKER]
    s=before[GRASP];s=s.replace('void summary(FILE* output) noexcept {',DECL+'void summary(FILE* output) noexcept {',1)
    anchor='    if(!output)return;';assert s.count(anchor)==1;s=s.replace(anchor,anchor+LOG)
    anchor='    const auto rejected=[&](unsigned gate) noexcept {';assert s.count(anchor)==1
    s=s.replace(anchor,'    hand::SocketVisibilityObservation socket_observation{};\n'+anchor+RECORD)
    old_proximity='''        if(current.hands[0].squeeze_active!=1||hand::control_held(current.hands[0],gears_vr::control_squeeze)||
           !broker->admit_socket_visibility(f))return;'''
    new_proximity='''        if(current.hands[0].squeeze_active!=1||hand::control_held(current.hands[0],gears_vr::control_squeeze)){rejected(7);return;}
        if(!broker->admit_socket_visibility(f,&socket_observation)){rejected(8);return;}'''
    assert s.count(old_proximity)==1;s=s.replace(old_proximity,new_proximity)
    reverse=s.replace(new_proximity,old_proximity).replace('    hand::SocketVisibilityObservation socket_observation{};\n','').replace(RECORD,'').replace(LOG,'').replace(DECL,'')
    assert reverse==before[GRASP]
    (P/'source'/GRASP).write_text(s)
    patch=''
    for rel in (BROKER,GRASP):patch+=''.join(difflib.unified_diff(before[rel].splitlines(True),(P/'source'/rel).read_text().splitlines(True),fromfile='a/'+rel,tofile='b/'+rel))
    (R/'docs/hammerburst-native-passive-rejection.patch').write_text(patch)
    parent=json.loads((BASE/'prepared.json').read_text());C=P/'project';C.mkdir()
    (C/'CMakeLists.txt').write_text((BASE/'project/CMakeLists.txt').read_text().replace(str(BASE).replace('\\','/'),str(P).replace('\\','/')))
    commands=[list(c) for c in parent['commands']];commands[0][commands[0].index('-S')+1]=str(C);commands[0][commands[0].index('-B')+1]=str(P/'build');commands[1][commands[1].index('--build')+1]=str(P/'build')
    gates={i+2:term.strip() for i,term in enumerate(terms)}
    (P/'prebuild.json').write_text(json.dumps(dict(parent=parent,commands=commands,exact_policy_reversal=True,socket_gates=gates,broker_original=old,broker_observed=new),indent=2))
    for label,cmd in zip(('configure','build'),commands):
        with (P/(label+'.log')).open('wb') as log:subprocess.run(cmd,check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
    dll=P/'build/Release/GearsCapture6DofM1.dll'
    proof=dict(parent_native=parent['native_sha256'],frozen_native=parent['frozen_native'],native_sha256=hashlib.sha256(dll.read_bytes()).hexdigest(),dll=str(dll),full_native_compiled=True,commands=commands,original_compile_definitions=parent['original_compile_definitions'],exact_policy_reversal=True,socket_gates=gates,runtime_exercised=False,gameplay_accepted=False,root_native_modified=False,xr_changed=False,scope='First64 original passive target refusals; original socket-admission first predicate and original source/frozen frame/seq/cohort. Identical short-circuit evaluation, original clock/admission each once; no new queries/writer/input/gate.')
    (P/'prepared.json').write_text(json.dumps(proof,indent=2));print(proof['native_sha256'])
if __name__=='__main__':main()
