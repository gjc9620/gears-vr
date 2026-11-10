"""Private same-acquisition-Frame HB proposal handoff; no shared Client change."""
from pathlib import Path
import difflib,hashlib,json,shutil,subprocess
ROOT=Path(__file__).resolve().parents[2];A=Path(__file__).resolve().parent
BASE=A/'released-debt-r1';TARGET=A/'supply-frame-r1'
def replace(text,old,new):
    assert text.count(old)==1,old[:100]
    return text.replace(old,new)
def main():
    assert not TARGET.exists(),'Preserve candidate'
    for name in ('playground','vr-common','third-party','native-capture'):
        shutil.copytree(BASE/'source/tools'/name,TARGET/'source/tools'/name)
    for name in ('weapon_hammerburst_supply_publication.h','weapon_hammerburst_supply_publication_test.cpp'):
        shutil.copy2(ROOT/'tools/vr-common'/name,TARGET/'source/tools/vr-common'/name)
    rel='tools/playground/weapon_hammerburst_runtime.inl';before=(BASE/'source'/rel).read_text();after=before
    after='#include "../vr-common/weapon_hammerburst_supply_publication.h"\n'+after
    after=replace(after,'hand::Lease held{};','hand::Lease held{};\ngears_hammerburst_supply_publication::Snapshot supply_publication;')
    after=replace(after,'    if(held.id==f.press_token&&hand::lease_valid(held,f.key.lifecycle)){',
        '''    // Preserve ONLY this callback's original supply proposal through
    // its acquisition Frame. The new incumbent enters NEXT Frame prepare/
    // commit normally. Never relax Client comparison or first-grant checks.
    if(stage==Stage::new_magazine&&supply_publication.same_acquisition_source(f,held,new_item)){
        const P body{f.stereo.head.position,f.reference.orientation};
        game.request_ammo=gears_weapon::body_slot(nativeequipment::calibration,body,f.controllers.hands[0].grip.position,0)==gears_weapon::BodySlot::shoulder_ammo;
        static unsigned rows{};
        if(log_file&&rows<8){fwprintf(log_file,L"hammerburst_supply_frame frame=%llu sequence=%llu token=%llu request_ammo=%u defer_incumbent=1\\n",
            f.key.frame,f.controllers.sequence,f.press_token,unsigned(game.request_ammo));fflush(log_file);++rows;}
        return;
    }
    if(held.id==f.press_token&&hand::lease_valid(held,f.key.lifecycle)){''')
    after=replace(after,'    held={};serial=old_item=new_item=0;',
        '    supply_publication.reset();held={};serial=old_item=new_item=0;')
    after=replace(after,'        held=decision.lease;released=false;stage=Stage::new_magazine;insertion_step(f,new_item);return;',
        '        supply_publication.remember(f,decision);\n        held=decision.lease;released=false;stage=Stage::new_magazine;insertion_step(f,new_item);return;')
    (TARGET/'source'/rel).write_text(after)
    (ROOT/'docs/hammerburst-supply-frame.patch').write_text(''.join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='a/'+rel,tofile='b/'+rel)))
    source=TARGET/'source/tools/vr-common/weapon_hammerburst_supply_publication_test.cpp';exe=TARGET/'supply-test.exe';cmd=TARGET/'supply-test.cmd'
    cmd.write_text('@echo off\ncall "E:/vs/VC/Auxiliary/Build/vcvars64.bat" >nul\nif errorlevel 1 exit /b %errorlevel%\n'+
        f'cl /nologo /O2 /std:c++17 /W4 /WX /UNDEBUG /EHsc "{source}" /Fo"{TARGET / "supply-test.obj"}" /Fe"{exe}"\n'+
        f'if errorlevel 1 exit /b %errorlevel%\n"{exe}"\nexit /b %errorlevel%\n')
    finish()
def finish():
    rel='tools/playground/weapon_hammerburst_runtime.inl';cmd=TARGET/'supply-test.cmd'
    with (TARGET/'supply-test.log').open('wb') as log:
        subprocess.run(['cmd','/c',str(cmd)],check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
    parent=json.loads((BASE/'prepared.json').read_text());configure=list(parent['configure_command']);build=list(parent['build_command'])
    configure[configure.index('-S')+1]=str(TARGET/'source/tools/playground');configure[configure.index('-B')+1]=str(TARGET/'build')
    build[build.index('--build')+1]=str(TARGET/'build')
    for label,command in (('configure',configure),('build',build)):
        with (TARGET/(label+'.log')).open('wb') as log:subprocess.run(command,check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
    dll=TARGET/'build/Release/GearsPlayground6DofUnified3.dll'
    proof=dict(base=str(BASE),full_playground_compiled=True,runtime_exercised=False,gameplay_accepted=False,
        dll=str(dll),dll_sha256=hashlib.sha256(dll.read_bytes()).hexdigest(),configure_command=configure,build_command=build,
        native_rebuilt=False,xr_rebuilt=False,shared_client_changed=False,
        scope='Only HB new supply proposal on EXACT acquisition Frame; next Frame observes actual incumbent. Strict Client prepare/commit/read comparison, input freshness, lease arbitration and clocks unchanged.')
    (TARGET/'prepared.json').write_text(json.dumps(proof,indent=2))
    files=[rel,'tools/vr-common/weapon_hammerburst_supply_publication.h','tools/vr-common/weapon_hammerburst_supply_publication_test.cpp']
    (TARGET/'source-verification.json').write_text(json.dumps({n:hashlib.sha256((TARGET/'source'/n).read_bytes()).hexdigest() for n in files},indent=2))
    print(json.dumps({k:proof[k] for k in ('full_playground_compiled','dll_sha256','shared_client_changed','gameplay_accepted')}))
if __name__=='__main__':main()
