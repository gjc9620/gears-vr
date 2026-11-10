"""Separate existing HB asset-read/attachment/current costs without gate changes."""
from pathlib import Path
import difflib,hashlib,json,shutil,subprocess
A=Path(__file__).resolve().parent;ROOT=A.parents[1];BASE=A/'release-handoff-r1';P=A/'press-assets-r1'
def once(t,a,b):assert t.count(a)==1,a[:120];return t.replace(a,b)
def main():
 assert not P.exists(),'Preserve candidate identity'
 for n in ('playground','vr-common','native-capture','third-party'):shutil.copytree(BASE/'source/tools'/n,P/'source/tools'/n)
 patch=[];rel='tools/playground/weapon_lancer_r2_native.inl';before=(BASE/'source'/rel).read_text()
 old='''        if(lancermain_asset::read(*source,asset,lancermain_asset::AssetScope::supported_instant_fire,&observed_kind)!=lancermain_asset::Status::ready||
            !nativeequipment::attached_frame(*source,asset.component.address)||
            !nativeequipment::current(nullptr,*source))return reject(L"attachment");'''
 new='''        if(hb_timing.asset(lancermain_asset::read(*source,asset,lancermain_asset::AssetScope::supported_instant_fire,&observed_kind)!=lancermain_asset::Status::ready,0)||
            hb_timing.asset(!nativeequipment::attached_frame(*source,asset.component.address),1)||
            hb_timing.asset(!nativeequipment::current(nullptr,*source),2))return reject(L"attachment");'''
 after=once(before,old,new);assert after.replace(new,old)==before
 (P/'source'/rel).write_text(after);patch.extend(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='a/'+rel,tofile='b/'+rel))
 helper='tools/playground/weapon_hammerburst_press_timing.inl';before=(BASE/'source'/helper).read_text();after=before
 after=once(after,'sync{},stamp[8]{};','sync{},stamp[8]{},asset_stamp[3]{};')
 after=once(after,'    ~HammerburstPressTiming() noexcept {','''    bool asset(bool original_failure,unsigned value) noexcept {
        if(active&&value<3)asset_stamp[value]=weaponhand::clock_ns(nullptr);
        return original_failure;
    }
    ~HammerburstPressTiming() noexcept {''')
 after=once(after,'owned_and_dispatch_ns=%lld\\n",','owned_and_dispatch_ns=%lld asset_read_ns=%lld attachment_ns=%lld post_asset_current_ns=%lld\\n",')
 after=once(after,'stamp[6]?stamp[6]-stamp[5]:-1,stamp[7]?stamp[7]-stamp[6]:-1);','''stamp[6]?stamp[6]-stamp[5]:-1,stamp[7]?stamp[7]-stamp[6]:-1,
            asset_stamp[0]&&stamp[2]?asset_stamp[0]-stamp[2]:-1,
            asset_stamp[1]&&asset_stamp[0]?asset_stamp[1]-asset_stamp[0]:-1,
            asset_stamp[2]&&asset_stamp[1]?asset_stamp[2]-asset_stamp[1]:-1);''')
 (P/'source'/helper).write_text(after);patch.extend(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='a/'+helper,tofile='b/'+helper))
 (ROOT/'docs/hammerburst-press-assets.patch').write_text(''.join(patch))
 parent=json.loads((BASE/'prepared.json').read_text());configure=list(parent['configure_command']);build=list(parent['build_command'])
 configure[configure.index('-S')+1]=str(P/'source/tools/playground');configure[configure.index('-B')+1]=str(P/'build');build[build.index('--build')+1]=str(P/'build')
 for label,cmd in (('configure',configure),('build',build)):
  with (P/(label+'.log')).open('wb') as log:subprocess.run(cmd,check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
 dll=P/'build/Release/GearsPlayground6DofUnified3.dll'
 proof=dict(base=str(BASE),full_playground_compiled=True,runtime_exercised=False,gameplay_accepted=False,dll=str(dll),dll_sha256=hashlib.sha256(dll.read_bytes()).hexdigest(),configure_command=configure,build_command=build,native_rebuilt=False,xr_rebuilt=False,production_gate_order_preserved=True,observation_only=True,source_clocks_or_geometry_limits_changed=False,scope='Bounded existing12 first-press rows split actual asset-read/attachment/current costs. Boolean passthrough retains original operand evaluation/short circuit/result. No source/expiry/permit/control changes; includes not-yet-live release-handoff repair.')
 (P/'prepared.json').write_text(json.dumps(proof,indent=2));(P/'source-verification.json').write_text(json.dumps({r:hashlib.sha256((P/'source'/r).read_bytes()).hexdigest() for r in (rel,helper)},indent=2))
 print(json.dumps({k:proof[k] for k in ('full_playground_compiled','dll_sha256','production_gate_order_preserved')}))
if __name__=='__main__':main()
