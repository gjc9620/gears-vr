from pathlib import Path
import difflib,hashlib,json,shutil,subprocess
A=Path(__file__).resolve().parent;ROOT=A.parents[1];BASE=A/'inventory-timing-r1';P=A/'pickup-baseline-r1'
def main():
 assert not P.exists(),'Preserve named candidate'
 for n in ('playground','vr-common','native-capture','third-party'):shutil.copytree(BASE/'source/tools'/n,P/'source/tools'/n)
 rel='tools/playground/weapon_hammerburst_inventory_timing.inl';before=(BASE/'source'/rel).read_text();after=(ROOT/rel).read_text()
 shutil.copy2(ROOT/rel,P/'source'/rel)
 (ROOT/'docs/hammerburst-pickup-baseline.patch').write_text(''.join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='a/'+rel,tofile='b/'+rel)))
 parent=json.loads((BASE/'prepared.json').read_text());configure=list(parent['configure_command']);build=list(parent['build_command'])
 configure[configure.index('-S')+1]=str(P/'source/tools/playground');configure[configure.index('-B')+1]=str(P/'build');build[build.index('--build')+1]=str(P/'build')
 for label,cmd in (('configure',configure),('build',build)):
  with (P/(label+'.log')).open('wb') as log:subprocess.run(cmd,check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
 dll=P/'build/Release/GearsPlayground6DofUnified3.dll'
 proof=dict(base=str(BASE),full_playground_compiled=True,runtime_exercised=False,gameplay_accepted=False,dll=str(dll),
  dll_sha256=hashlib.sha256(dll.read_bytes()).hexdigest(),configure_command=configure,build_command=build,native_rebuilt=False,xr_rebuilt=False,
  source_clocks_or_geometry_limits_changed=False,observation_only=True,
  scope='Existing successful Native inventory/current path reports at most16 simulated shoulder UP baselines matching exact Input evidence. Private driver sends ONE new Grip after real baseline and waits held receipt; production source/edge/identity/stock/100ms checks unchanged. Includes HB reload priority/history.')
 (P/'prepared.json').write_text(json.dumps(proof,indent=2))
 (P/'source-verification.json').write_text(json.dumps({rel:hashlib.sha256((P/'source'/rel).read_bytes()).hexdigest()},indent=2))
 print(json.dumps({k:proof[k] for k in ('full_playground_compiled','dll_sha256','observation_only')}))
if __name__=='__main__':main()
