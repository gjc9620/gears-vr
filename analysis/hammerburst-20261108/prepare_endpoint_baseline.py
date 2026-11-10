"""Narrow only the private UP observer region, preserving actual pickup gates."""
from pathlib import Path
import difflib,hashlib,json,shutil,subprocess
A=Path(__file__).resolve().parent;ROOT=A.parents[1];BASE=A/'press-assets-r1';P=A/'endpoint-baseline-r1'
def main():
 assert not P.exists(),'Preserve named candidate'
 for n in ('playground','vr-common','native-capture','third-party'):shutil.copytree(BASE/'source/tools'/n,P/'source/tools'/n)
 rel='tools/playground/weapon_hammerburst_inventory_timing.inl';before=(BASE/'source'/rel).read_text()
 assert before.count('<.04')==3;after=before.replace('<.04','<.005');assert after.replace('<.005','<.04')==before
 (P/'source'/rel).write_text(after);(ROOT/'docs/hammerburst-endpoint-baseline.patch').write_text(''.join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='a/'+rel,tofile='b/'+rel)))
 parent=json.loads((BASE/'prepared.json').read_text());configure=list(parent['configure_command']);build=list(parent['build_command'])
 configure[configure.index('-S')+1]=str(P/'source/tools/playground');configure[configure.index('-B')+1]=str(P/'build');build[build.index('--build')+1]=str(P/'build')
 for label,cmd in (('configure',configure),('build',build)):
  with (P/(label+'.log')).open('wb') as log:subprocess.run(cmd,check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
 dll=P/'build/Release/GearsPlayground6DofUnified3.dll'
 proof=dict(base=str(BASE),full_playground_compiled=True,runtime_exercised=False,gameplay_accepted=False,dll=str(dll),dll_sha256=hashlib.sha256(dll.read_bytes()).hexdigest(),configure_command=configure,build_command=build,native_rebuilt=False,xr_rebuilt=False,observation_only=True,source_clocks_or_geometry_limits_changed=False,scope='Private bounded shoulder-UP observer region only narrowed from40mm to5mm around existing exact endpoint. Actual shoulder/equipment geometry, input/current/source checks, clocks,16-row cap unchanged. Driver separately preserves full original gesture to exact requested endpoint; never sends Grip on observer-region entry alone. Includes unexercised press-assets/handoff.')
 (P/'prepared.json').write_text(json.dumps(proof,indent=2));(P/'source-verification.json').write_text(json.dumps({rel:hashlib.sha256((P/'source'/rel).read_bytes()).hexdigest()},indent=2));print(json.dumps({k:proof[k] for k in ('full_playground_compiled','dll_sha256')}))
if __name__=='__main__':main()
