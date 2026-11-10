from pathlib import Path
import difflib,hashlib,json,shutil,subprocess
A=Path(__file__).resolve().parent;ROOT=A.parents[1];BASE=A/'reload-priority-r1';P=A/'inventory-timing-r1'
def once(t,a,b):assert t.count(a)==1,a[:80];return t.replace(a,b)
def main():
 assert not P.exists(),'Preserve named candidate'
 for n in ('playground','vr-common','native-capture','third-party'):shutil.copytree(BASE/'source/tools'/n,P/'source/tools'/n)
 helper='tools/playground/weapon_hammerburst_inventory_timing.inl';shutil.copy2(ROOT/helper,P/'source'/helper)
 rel='tools/playground/weapon_equipment_backend.inl';before=(BASE/'source'/rel).read_text();after=before
 after=once(after,'bool inventory(void*,abi::Frame const& frame,Snapshot& out) noexcept {',
  '#include "weapon_hammerburst_inventory_timing.inl"\nbool inventory(void*,abi::Frame const& frame,Snapshot& out) noexcept {\n    HammerburstInventoryTiming inventory_timing(frame);')
 start=after.index('bool inventory(void*');end=after.index('#include "weapon_equipment_idle_inventory.inl"',start)
 body=after[start:end]
 body=once(body,'        if(!current(nullptr,frame))return false;\n        if(!vrequip::snapshot',
  '        if(!current(nullptr,frame))return false;\n        inventory_timing.mark(1);\n        if(!vrequip::snapshot')
 body=once(body,'        inventory_stage=L"frame_binding";','        inventory_timing.mark(2);\n        inventory_stage=L"frame_binding";')
 body=once(body,'        inventory_stage=L"current_after";','        inventory_timing.mark(3);\n        inventory_stage=L"current_after";')
 body=once(body,'        inventory_stage=L"ok";return true;','        inventory_timing.mark(4);\n        inventory_stage=L"ok";return true;')
 after=after[:start]+body+after[end:]
 stripped=after.replace('#include "weapon_hammerburst_inventory_timing.inl"\n','')
 stripped='\n'.join(l for l in stripped.split('\n') if 'inventory_timing.' not in l and 'HammerburstInventoryTiming inventory_timing(' not in l)
 assert stripped==before,'Production statement parity required'
 (P/'source'/rel).write_text(after)
 (ROOT/'docs/hammerburst-inventory-timing.patch').write_text(''.join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='a/'+rel,tofile='b/'+rel)))
 parent=json.loads((BASE/'prepared.json').read_text());configure=list(parent['configure_command']);build=list(parent['build_command'])
 configure[configure.index('-S')+1]=str(P/'source/tools/playground');configure[configure.index('-B')+1]=str(P/'build');build[build.index('--build')+1]=str(P/'build')
 for label,cmd in (('configure',configure),('build',build)):
  with (P/(label+'.log')).open('wb') as log:subprocess.run(cmd,check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
 dll=P/'build/Release/GearsPlayground6DofUnified3.dll'
 proof=dict(base=str(BASE),full_playground_compiled=True,runtime_exercised=False,gameplay_accepted=False,dll=str(dll),
  dll_sha256=hashlib.sha256(dll.read_bytes()).hexdigest(),configure_command=configure,build_command=build,native_rebuilt=False,xr_rebuilt=False,
  source_clocks_or_geometry_limits_changed=False,observation_only=True,production_statement_parity=True,
  scope='Existing inventory current-before/snapshot/binding/current-after timing and same-source withdrawal receipt,4 idle+12 right-grip rows. No source/selection/identity/expiry/dispatch changes; retains HB reload priority/history.')
 (P/'prepared.json').write_text(json.dumps(proof,indent=2))
 (P/'source-verification.json').write_text(json.dumps({n:hashlib.sha256((P/'source'/n).read_bytes()).hexdigest() for n in (rel,helper)},indent=2))
 print(json.dumps({k:proof[k] for k in ('full_playground_compiled','dll_sha256','production_statement_parity')}))
if __name__=='__main__':main()
