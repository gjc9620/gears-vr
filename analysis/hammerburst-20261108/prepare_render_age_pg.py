"""Combine certified logical age continuity with the tested real-UP debt WAIT."""
from pathlib import Path
import json,hashlib,shutil,subprocess
A=Path(__file__).resolve().parent;BASE=A/'latch-seek-gap-r1';DONOR=A/'released-pose-gap-r1';P=A/'render-age-pg-r1'
def main():
 assert not P.exists(),'Preserve candidate'
 rel='tools/playground/weapon_hammerburst_burst_event.inl'
 assert (BASE/'source'/rel).read_bytes()==(A/'extraction-observe-r1/source'/rel).read_bytes()
 assert json.loads((DONOR/'release-handoff-test-result.json').read_text())['total_checks']==457
 for n in ('playground','vr-common','native-capture','third-party'):shutil.copytree(BASE/'source/tools'/n,P/'source/tools'/n)
 for n in (rel,'tools/playground/weapon_hammerburst_burst_event_test.cpp'):
  shutil.copy2(DONOR/'source'/n,P/'source'/n)
 parent=json.loads((BASE/'prepared.json').read_text());configure=list(parent['configure_command']);build=list(parent['build_command'])
 configure[configure.index('-S')+1]=str(P/'source/tools/playground');configure[configure.index('-B')+1]=str(P/'build');build[build.index('--build')+1]=str(P/'build')
 for label,cmd in (('configure',configure),('build',build)):
  with (P/(label+'.log')).open('wb') as log:subprocess.run(cmd,check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
 dll=P/'build/Release/GearsPlayground6DofUnified3.dll'
 proof=dict(base=str(BASE),full_playground_compiled=True,runtime_exercised=False,gameplay_accepted=False,dll=str(dll),dll_sha256=hashlib.sha256(dll.read_bytes()).hexdigest(),configure_command=configure,build_command=build,native_rebuilt=False,xr_rebuilt=False,scope='Existing real-UP certified finite debt WAIT only; full original lifetime/input/event expiry and fresh source/muzzle20ms requirements remain. Paired with Native Gate-certified age-only logical continuity, never a stale dispatch. Includes existing strict latch pre-dispatch rollback and new-mag incumbent repairs, neither live-exercised yet.')
 (P/'prepared.json').write_text(json.dumps(proof,indent=2));(P/'source-verification.json').write_text(json.dumps({n:hashlib.sha256((P/'source'/n).read_bytes()).hexdigest() for n in (rel,'tools/playground/weapon_hammerburst_burst_event_test.cpp')},indent=2))
 # Run the existing 457 checks against THIS combined source, not the donor.
 script=(A/'check_released_pose_gap_handoff.py').read_text().replace("P=A/'released-pose-gap-r1'","P=A/'render-age-pg-r1'")
 target=A/'check_render_age_pg_handoff.py';target.write_text(script)
 subprocess.run([__import__('sys').executable,'-E','-s','-B','-X','utf8',str(target)],check=True,creationflags=subprocess.CREATE_NO_WINDOW)
 print(proof['dll_sha256'])
if __name__=='__main__':main()
