"""Extend exact-source handoff to actual55584 initial latch acquisition."""
from pathlib import Path
import difflib,hashlib,json,shutil,subprocess
ROOT=Path(__file__).resolve().parents[2];A=Path(__file__).resolve().parent
BASE=A/'supply-frame-r1';TARGET=A/'part-frame-r1'
def replace(text,old,new):
    assert text.count(old)==1,old[:100]
    return text.replace(old,new)
def main():
    assert not TARGET.exists(),'Preserve frozen candidate'
    for name in ('playground','vr-common','third-party','native-capture'):
        shutil.copytree(BASE/'source/tools'/name,TARGET/'source/tools'/name)
    for name in ('weapon_hammerburst_supply_publication.h','weapon_hammerburst_supply_publication_test.cpp'):
        shutil.copy2(ROOT/'tools/vr-common'/name,TARGET/'source/tools/vr-common'/name)
    rel='tools/playground/weapon_hammerburst_runtime.inl';before=(BASE/'source'/rel).read_text();after=before
    after=replace(after,'gears_hammerburst_supply_publication::Snapshot supply_publication;',
        'gears_hammerburst_supply_publication::Snapshot supply_publication,latch_publication;')
    after=replace(after,'    if(held.id==f.press_token&&hand::lease_valid(held,f.key.lifecycle)){',
        '''    if((stage==Stage::acquiring||stage==Stage::opening)&&latch_publication.same_acquisition_source(f,held,old_item)){
        game.request_magazine=at_latch(f);
        static unsigned rows{};
        if(log_file&&rows<8){fwprintf(log_file,L"hammerburst_latch_frame frame=%llu sequence=%llu token=%llu request_magazine=%u defer_incumbent=1\\n",
            f.key.frame,f.controllers.sequence,f.press_token,unsigned(game.request_magazine));fflush(log_file);++rows;}
        return;
    }
    if(held.id==f.press_token&&hand::lease_valid(held,f.key.lifecycle)){''')
    after=replace(after,'    supply_publication.reset();held={};serial=old_item=new_item=0;',
        '    supply_publication.reset();latch_publication.reset();held={};serial=old_item=new_item=0;')
    after=replace(after,'        opening=before;old_item=f.press_token;held=decision.lease;ui_pending=needs_reload;',
        '        latch_publication.remember(f,decision,hand::LeaseKind::magazine);\n        opening=before;old_item=f.press_token;held=decision.lease;ui_pending=needs_reload;')
    after=replace(after,'    if(stage!=Stage::idle&&!(inv::identity(f)==opening.identity)){cleanup(__LINE__);return true;}',
        '''    if(stage!=Stage::idle){
        const auto actual_identity=inv::identity(f);
        if(!(actual_identity==opening.identity)){
            const auto& old=opening.identity;const auto& now=actual_identity;
            const unsigned difference=unsigned(!hand::same(old.world,now.world))|
                (unsigned(!hand::same(old.pawn,now.pawn))<<1)|(unsigned(!hand::same(old.weapon,now.weapon))<<2)|
                (unsigned(old.game_start!=now.game_start)<<3)|(unsigned(old.native_start!=now.native_start)<<4)|
                (unsigned(old.registration!=now.registration)<<5)|(unsigned(old.lifecycle!=now.lifecycle)<<6)|
                (unsigned(old.world_generation!=now.world_generation)<<7)|(unsigned(old.hold_generation!=now.hold_generation)<<8)|
                (unsigned(old.session!=now.session)<<9)|(unsigned(old.space!=now.space)<<10)|(unsigned(old.input!=now.input)<<11);
            static unsigned rows{};
            if(log_file&&rows<8){fwprintf(log_file,L"hammerburst_identity_lost difference=%u frame=%llu sequence=%llu stage=%u old_hold=%llu now_hold=%llu old_lifecycle=%llu now_lifecycle=%llu old_world_generation=%llu now_world_generation=%llu old_input=%llu now_input=%llu old_space=%llu now_space=%llu\\n",
                difference,f.key.frame,f.controllers.sequence,unsigned(stage),old.hold_generation,now.hold_generation,
                old.lifecycle,now.lifecycle,old.world_generation,now.world_generation,old.input,now.input,old.space,now.space);
                fflush(log_file);++rows;}
            cleanup(__LINE__);return true;
        }
    }''')
    (TARGET/'source'/rel).write_text(after)
    (ROOT/'docs/hammerburst-part-frame.patch').write_text(''.join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='a/'+rel,tofile='b/'+rel)))
    source=TARGET/'source/tools/vr-common/weapon_hammerburst_supply_publication_test.cpp';cmd=TARGET/'part-test.cmd';exe=TARGET/'part-test.exe'
    cmd.write_text('@echo off\ncall "E:/vs/VC/Auxiliary/Build/vcvars64.bat" >nul\nif errorlevel 1 exit /b %errorlevel%\n'+
        f'cl /nologo /O2 /std:c++17 /W4 /WX /UNDEBUG /EHsc "{source}" /Fo"{TARGET / "part-test.obj"}" /Fe"{exe}"\n'+
        f'if errorlevel 1 exit /b %errorlevel%\n"{exe}"\nexit /b %errorlevel%\n')
    with (TARGET/'part-test.log').open('wb') as log:subprocess.run(['cmd','/c',str(cmd)],check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
    parent=json.loads((BASE/'prepared.json').read_text());configure=list(parent['configure_command']);build=list(parent['build_command'])
    configure[configure.index('-S')+1]=str(TARGET/'source/tools/playground');configure[configure.index('-B')+1]=str(TARGET/'build')
    build[build.index('--build')+1]=str(TARGET/'build')
    for label,command in (('configure',configure),('build',build)):
        with (TARGET/(label+'.log')).open('wb') as log:subprocess.run(command,check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
    dll=TARGET/'build/Release/GearsPlayground6DofUnified3.dll'
    proof=dict(base=str(BASE),full_playground_compiled=True,runtime_exercised=False,gameplay_accepted=False,
        dll=str(dll),dll_sha256=hashlib.sha256(dll.read_bytes()).hexdigest(),configure_command=configure,build_command=build,
        native_rebuilt=False,xr_rebuilt=False,shared_client_changed=False,
        scope='Exact acquisition Frame HB initial latch plus shoulder supply proposal handoffs; strict next Frame Client prepare/commit/read and all identity/input/mount/deadline checks unchanged. Bounded identity rejection classification only.')
    (TARGET/'prepared.json').write_text(json.dumps(proof,indent=2))
    files=[rel,'tools/vr-common/weapon_hammerburst_supply_publication.h','tools/vr-common/weapon_hammerburst_supply_publication_test.cpp']
    (TARGET/'source-verification.json').write_text(json.dumps({n:hashlib.sha256((TARGET/'source'/n).read_bytes()).hexdigest() for n in files},indent=2))
    print(json.dumps({k:proof[k] for k in ('full_playground_compiled','dll_sha256','shared_client_changed','gameplay_accepted')}))
if __name__=='__main__':main()
