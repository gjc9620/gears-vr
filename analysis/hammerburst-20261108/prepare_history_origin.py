from pathlib import Path
import json,hashlib,shutil,difflib,subprocess
A=Path(__file__).resolve().parent;R=A.parents[1];BASE=A/'withdrawal-clock-r1';P=A/'history-origin-r1'
def main():
 assert not P.exists(),'Preserve candidate identity'
 for n in ('playground','vr-common','native-capture','third-party'):shutil.copytree(BASE/'source/tools'/n,P/'source/tools'/n)
 patch=''
 def edit(rel,fn):
  nonlocal patch
  p=P/'source'/rel;before=p.read_text();after=fn(before);assert after!=before;p.write_text(after);patch+=''.join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='a/'+rel,tofile='b/'+rel))
 def bridge(s):
  needle='class InputBridge {';assert s.count(needle)==1
  s=s.replace(needle,'struct MotionReadObservation {\n    std::uint64_t latest{},selected{},stopped{},stored{};\n    std::int64_t read_ns{};unsigned reason{}; // 1evicted,2missing,3original pair reject;0no break.\n};\n'+needle+'\n    MotionReadObservation motion_observation_{};')
  needle='    bool read_matching(gears_vr::StereoSample const& stereo';assert s.count(needle)==1
  s=s.replace(needle,'    MotionReadObservation motion_observation() const noexcept {return motion_observation_;}\n'+needle)
  needle='        if(contended)*contended=false;\n        out={};publisher={};if(right_press)*right_press={};if(right_motion)*right_motion={};';assert s.count(needle)==1
  s=s.replace(needle,'        if(right_motion)motion_observation_={};\n'+needle)
  needle='        std::memcpy(&packet,data_,sizeof(packet));ReleaseMutex(mutex_);';assert s.count(needle)==1
  s=s.replace(needle,needle+'\n        if(right_motion)motion_observation_={packet.latest_sequence,stereo.sequence,0,0,now_ns,0};')
  needle='                if(packet.latest_sequence-sequence>=ContinuityPacket::capacity)break;';assert s.count(needle)==1
  s=s.replace(needle,'                if(packet.latest_sequence-sequence>=ContinuityPacket::capacity){motion_observation_.reason=1;motion_observation_.stopped=sequence;break;}')
  needle='                if(prior.input.sequence!=sequence||!motion_pair(prior,history.frames[0],1,100000000))break;';assert s.count(needle)==1
  s=s.replace(needle,'                if(prior.input.sequence!=sequence){motion_observation_.reason=2;motion_observation_.stopped=sequence;motion_observation_.stored=prior.input.sequence;break;}\n                if(!motion_pair(prior,history.frames[0],1,100000000)){motion_observation_.reason=3;motion_observation_.stopped=sequence;motion_observation_.stored=prior.input.sequence;break;}')
  return s
 edit('tools/vr-common/controller_continuity_bridge.h',bridge)
 def reader(s):
  needle='    Reader()=default;Reader(Reader const&)=delete;';assert s.count(needle)==1
  return s.replace(needle,'    gears_hand_continuity::MotionReadObservation motion_observation() const noexcept {return continuity_.motion_observation();}\n'+needle)
 edit('tools/vr-common/current_tracking_reader.h',reader)
 def latch(s):
  needle='        hb_history_rejection::capture(log_file,f.key.frame,f.controllers.sequence,control,built,segment.count,';assert s.count(needle)==1
  s=s.replace(needle,'        const auto before_record=hb_history_rejection::records;\n'+needle)
  needle='            previous.raw,sampled.raw,history,sampled.mount,sampled.size);';assert s.count(needle)==1
  return s.replace(needle,needle+'''
        if(hb_history_rejection::records>before_record){
            const auto origin=nativeequipment::input_continuity_reader.motion_observation();
            fwprintf(log_file,L"hammerburst_history_origin record=%u latest=%llu selected=%llu stopped=%llu stored=%llu read_ns=%lld reason=%u action=0\\n",
                hb_history_rejection::records,origin.latest,origin.selected,origin.stopped,origin.stored,origin.read_ns,origin.reason);fflush(log_file);
        }''')
 edit('tools/playground/weapon_hammerburst_latch_history.inl',latch)
 (R/'docs/hammerburst-history-origin.patch').write_text(patch)
 parent=json.loads((BASE/'prepared.json').read_text());configure=list(parent['configure_command']);build=list(parent['build_command']);configure[configure.index('-S')+1]=str(P/'source/tools/playground');configure[configure.index('-B')+1]=str(P/'build');build[build.index('--build')+1]=str(P/'build')
 for label,cmd in (('configure',configure),('build',build)):
  with (P/(label+'.log')).open('wb') as log:subprocess.run(cmd,check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
 dll=P/'build/Release/GearsPlayground6DofUnified3.dll'
 proof=dict(base=str(BASE),full_playground_compiled=True,runtime_exercised=False,gameplay_accepted=False,dll=str(dll),dll_sha256=hashlib.sha256(dll.read_bytes()).hexdigest(),configure_command=configure,build_command=build,native_rebuilt=False,xr_rebuilt=False,scope='Receiver-local original ring latest/selected and first loop stop reason retained from same packet/read. Existing history rejection only, cap4. No new ring/clock/motion_pair calls, packet/writer/capacity/source/gates unchanged.')
 (P/'prepared.json').write_text(json.dumps(proof,indent=2));print(proof['dll_sha256'])
if __name__=='__main__':main()
