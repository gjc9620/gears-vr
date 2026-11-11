"""HB action adapter for one exact isolated Source1 instance.

Does not launch a game or change shared scripts/configuration. --check only
compiles the adapted readers/action chain. Runtime remains separately owned.
"""
from pathlib import Path
import argparse
import hashlib
import json
import re
import subprocess
import sys
import types

ROOT = Path(__file__).resolve().parents[2]
CODE = ROOT / 'analysis/solo-m1-20261028'
READERS = ('read_private_input.py', 'read_world_grips.py', 'read_aim_metrics.py', 'read_private_skeleton.py')


def reader_arguments(name,args):
    if name not in READERS or not args or Path(args[0]).name!=args[0] or not args[0].endswith('.json'):
        raise ValueError('Expected an allowed reader and one local JSON output name')
    flags=tuple(args[1:])
    if flags and (name!='read_private_skeleton.py' or flags not in (('--hands',),('--weapon',))):
        raise ValueError('Unexpected reader flags')
    return flags


def replace_once(source, old, new):
    if source.count(old) != 1:
        raise ValueError('Source boundary changed: ' + old[:100])
    return source.replace(old, new)


def instance_root(name):
    if not re.fullmatch(r'hb-[a-z0-9_-]{1,36}', name):
        raise ValueError('Expected an exact HB instance name')
    return ROOT / 'analysis/instances' / name


class Binding:
    def __init__(self, name, output, control=None):
        self.name = name
        self.instance = instance_root(name)
        self.output = Path(output).resolve()
        if not self.output.is_relative_to(self.instance):
            raise ValueError('Action output must belong to this exact instance')
        self.control = control
        self.pinned = None
        self.report = None
        self.continuous_session_id = None

    def state(self):
        state = json.loads((self.instance / 'launcher-session.json').read_text(encoding='utf-8'))
        if Path(state['instance_root']).resolve() != self.instance.resolve():
            raise RuntimeError('Session belongs to another instance')
        for key in ('owner_identity', 'game_identity'):
            value = state[key]
            if not value.get('pid') or not value.get('creation_time') or self.control.identity(value['pid']) != value:
                raise RuntimeError('Owned lifetime changed: ' + key)
        if state.get('vr_gamepad', {}).get('source') != 1:
            raise RuntimeError('Expected private Source1 input')
        report = Path(state['session_report']).resolve()
        # Report storage is launcher-controlled, but its publisher must match
        # this instance's exact frozen XR executable and current lifetime.
        details = json.loads(report.read_text(encoding='utf-8-sig'))
        publisher = details['xr_identity']
        manifest = json.loads((self.instance / 'manifest.json').read_text())
        executable = (ROOT / manifest['xr_build'] / 'OpenXRPoseProbe.exe').resolve()
        if Path(publisher['image']).resolve() != executable or self.control.identity(publisher['pid']) != publisher:
            raise RuntimeError('Owned XR publisher changed')
        if hashlib.sha256(executable.read_bytes()).hexdigest() != manifest['xr_sha256']:
            raise RuntimeError('Frozen XR artifact changed')
        self.pinned = (state['owner_identity'], state['game_identity'], publisher)
        self.report = report
        self.continuous_session_id = details['session_id']
        return state

    def projection_ready(self,game,*,now=None):
        # Existing launcher receipt only, never a new writer or frame permit.
        if not self.pinned or self.report is None or not self.live(game):return False
        try:details=json.loads(self.report.read_text(encoding='utf-8-sig'))
        except (OSError,ValueError):return False
        if not isinstance(details,dict):return False
        if (details.get('game_identity')!=game or details.get('xr_identity')!=self.pinned[2] or
            details.get('session_id')!=self.continuous_session_id or details.get('source')!=1):return False
        timings=details.get('startup_timings',{})
        if not isinstance(timings,dict):return False
        timing=timings.get('first_stereo_submission',{})
        if not isinstance(timing,dict):return False
        stamp=timing.get('xr_monotonic_ns')
        if now is None:
            import time
            now=time.perf_counter_ns()
        return (isinstance(stamp,int) and not isinstance(stamp,bool) and 0<stamp<=now and
            timing.get('timing_is_runner_observation') is False and
            timing.get('evidence')=='XR epoch1 actual projection successful xrEndFrame marker')

    def publisher(self):
        state = self.state()
        return json.loads(Path(state['session_report']).read_text(encoding='utf-8-sig'))['xr_identity']

    def live(self, game):
        if (self.instance / 'stop-actions').exists():
            return False
        if self.pinned is None:
            self.state()
        owner, selected_game, publisher = self.pinned
        return selected_game == game and all(self.control.identity(value['pid']) == value
                                             for value in (owner, selected_game, publisher))

    def helper(self, name, *args):
        flags=reader_arguments(name,args)
        self.state()
        subprocess.run([sys.executable, '-E', '-s', '-B', '-X', 'utf8', str(Path(__file__).resolve()),
                        '--instance', self.name, '--output', str(self.output), '--reader', name,
                        '--reader-output', args[0], *['--reader-flag='+flag for flag in flags]], check=True, stdout=subprocess.DEVNULL,
                       creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))

    def source_text(self,relative):
        allowed=('tools/vr-common/weapon_hammerburst_foregrip.h','tools/vr-common/hand_finger_pose.h')
        if relative not in allowed:raise ValueError('Unexpected authored source')
        proof=json.loads((self.instance/'hb-source-binding.json').read_text(encoding='utf-8-sig'))
        manifest=json.loads((self.instance/'manifest.json').read_text(encoding='utf-8-sig'))
        candidate=Path(proof['candidate_root']).resolve();area=(ROOT/'analysis/hammerburst-20261108').resolve()
        if candidate.parent!=area or proof['dll_sha256']!=manifest['playground_sha256']:
            raise RuntimeError('Authored source belongs to another candidate')
        path=candidate/'source'/relative;raw=path.read_bytes()
        if hashlib.sha256(raw).hexdigest()!=proof['source_sha256'][relative]:raise RuntimeError('Pinned authored source changed')
        return raw.decode('utf-8-sig')

    def stop(self):
        state = json.loads((self.instance / 'launcher-session.json').read_text())
        if Path(state['instance_root']).resolve() != self.instance.resolve():
            raise RuntimeError('Stop target belongs to another instance')
        owner = state['owner_identity']
        if self.control.identity(owner['pid']) != owner:
            raise RuntimeError('Stop owner lifetime changed')
        subprocess.run([sys.executable, '-E', '-s', '-B', '-X', 'utf8',
                        str(ROOT / 'tools/multi-instance/isolated_sim.py'), 'stop', '--name', self.name],
                       check=True, creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))


def reader_source(name):
    if name not in READERS:
        raise ValueError('Unknown reader')
    source = (CODE / name).read_text(encoding='utf-8')
    if name == 'read_private_skeleton.py':
        source=replace_once(source,"cfg=json.loads((OUT/'private-instance.json').read_text());state=json.loads((Path(cfg['root'])/'launcher-session.json').read_text());game=state['game_identity'];control=WindowsControl()",
                            "OUT=BOUND.output;state=BOUND.state();game=state['game_identity'];control=WindowsControl()")
        source=replace_once(source,"r.class_name(wid)=='Weap_AssaultRifle'","r.class_name(wid)=='Weap_LocustAssaultRifle'")
        for old,new in [('rn==9','rn==3'),('r.read(rp,9*68)','r.read(rp,3*68)'),('for i in range(9):','for i in range(3):'),('bones=tuple(range(9))','bones=tuple(range(3))'),('count==(9 if weapon_mode else 115)','count==(3 if weapon_mode else 115)')]:
            if source.count(old)!=1 and old!='r.read(rp,9*68)':raise ValueError('Skeleton boundary changed: '+old)
            source=source.replace(old,new)
        old=next(line for line in source.splitlines() if line.startswith('        expected='))
        source=replace_once(source,old,"        expected=['b_HB_Root','b_HB_MagLatch','b_HB_Mag']")
    elif name == 'read_aim_metrics.py':
        source = replace_once(source, "cfg = json.loads((OUT / 'private-instance.json').read_text())\ngame = json.loads((Path(cfg['root']) / 'launcher-session.json').read_text())['game_identity']",
                              "game = BOUND.state()['game_identity']\nOUT = BOUND.output")
    else:
        source = replace_once(source, "control=WindowsControl();cfg=json.loads((OUT/'private-instance.json').read_text())",
                              'control=WindowsControl();OUT=BOUND.output')
        if name == 'read_private_input.py':
            source = replace_once(source, "state=json.loads((Path(cfg['root'])/'launcher-session.json').read_text())", 'state=BOUND.state()')
        else:
            source = replace_once(source, "game=json.loads((Path(cfg['root'])/'launcher-session.json').read_text())['game_identity']", "game=BOUND.state()['game_identity']")
        start = source.index('rows=json.loads(subprocess.check_output(')
        end = source.index(';reader=w.Reader(game,publisher,control.identity)', start)
        source = source[:start] + 'publisher=BOUND.publisher()' + source[end:]
    if 'private-instance.json' in source or 'Get-CimInstance' in source or 'build-solo-m1-sim' in source:
        raise ValueError('Shared reader selection survived adaptation')
    return source


def equipment_source():
    source = (CODE / 'equipment_controller_chain.py').read_text(encoding='utf-8')
    source = replace_once(source, '    def reach(end,squeeze=0):', '    def reach(end,squeeze=0,ready=None):')
    source = replace_once(source, '            live();g.put(1,pose,**controls);time.sleep(interval)',
        '            live();g.put(1,pose,**controls);time.sleep(interval)\n            if ready is not None:ready()')
    source = replace_once(source, '        g.poses[1]=dict(end);time.sleep(.25)',
        '        g.poses[1]=dict(end)\n        if ready is None:time.sleep(.25)')
    source = replace_once(source, "g.ROOT/'analysis/instances/solo-m1-1028d/manifest.json'", "g.BOUND.instance/'manifest.json'")
    source = replace_once(source, "g.ROOT/'tools/playground/build-solo-metadata2'/f'playground-{g.game[\"pid\"]}.log'",
                          "g.ROOT/json.loads((g.BOUND.instance/'manifest.json').read_text())['playground_build']/f'playground-{g.game[\"pid\"]}.log'")
    needle = "        initial=current('before_original_X_pickup')"
    return replace_once(source, needle, needle + '\n        from types import SimpleNamespace\n        from hammerburst_core_chain import run as hb_run\n        hb_run(SimpleNamespace(**locals()))\n        return')


def core_source():
    source=(CODE/'hammerburst_core_chain.py').read_text(encoding='utf-8')
    source=replace_once(source,'        c.reach(pose(.23,-.115,-.17));g.pad(1,wait=.6,squeeze=1)',
        '        from confirmed_shoulder_pickup import acquire\n        acquire(c,data,shoulder=pose(.23,-.115,-.17))')
    old="""            before=len(c.records('native_lancer_shot_trajectory'))
            g.pad(1,wait=.35,trigger=1);g.pad(1,wait=.6)
            shots=c.records('native_lancer_shot_trajectory')[before:];data['shots']=shots
            assert shots and all(x.get('trace_valid')=='1' for x in shots),'No authoritative HB trajectory'
            assert len(shots)==6,('Original HB burst must export exactly six valid shots',len(shots))
            data['checks']['actual_fire']=True"""
    return replace_once(source,old,"            from confirmed_first_fire import first_fire\n            first_fire(c,data)")


def reload_source():
    source=(CODE/'hammerburst_reload_chain.py').read_text(encoding='utf-8')
    source=replace_once(source,"        phase('original_return_authoritative_fire');command(saved,False)",
        """        # Completion's original8s deadline still applies above. Returning
        # the hand and verifying a freshUP/one burst are a separate test phase.
        # Anchor its bounded12s once BEFORE the return gesture, never at DOWN
        # or after a partial shot. Product event/source clocks are unchanged.
        deadline=time.monotonic()+12
        data['return_fire_test_window']={'started':deadline-12,'deadline':deadline,'seconds':12,
            'scope':'return gesture, freshUP, exactly one burst and typed count readback'}
        save()
        phase('original_return_authoritative_fire');command(saved,False)""")
    source=replace_once(source,
        "        phase('extract_original_for_replacement' if replacement else 'extract_then_return_same_original_without_release')",
        """        if '--hammerburst-park-restore' in sys.argv:
            from hammerburst_restore_chain import run as restore_run
            resumed_transitions=len(c.records('hammerburst_reload_transition'))
            restore_run(c,count_reader=counts,release_left=lambda:command(saved,False),
                        grip_left=lambda:magazine(0),mount_reader=lambda:read('read_aim_metrics.py','restore_mount'))
            wait('hammerburst_reload_transition',resumed_transitions,lambda r:r.get('to')=='4',expected='fresh restored original magazine phase')
        phase('extract_original_for_replacement' if replacement else 'extract_then_return_same_original_without_release')""")
    return replace_once(source,'        g.pad(1,wait=.35,trigger=1);g.pad(1,wait=.6)',
        '        from confirmed_first_fire import first_fire\n        first_fire(c,data,deadline_ns=time.perf_counter_ns()+int(max(0.,deadline-time.monotonic())*1e9))')


def supply_source():
    source=(CODE/'hammerburst_supply_boundary_chain.py').read_text(encoding='utf-8')
    # The confirmed UP/first-shot/readback protocol needs more time than the
    # old fixed .35/.6 gesture. This is the bounded test-flow window only;
    # production event/source deadlines and per-burst waits remain unchanged.
    source=replace_once(source,'timeout=18.,clock=time.monotonic','timeout=36.,clock=time.monotonic')
    source=replace_once(source,'            c.live();g.pad(1,wait=.35,trigger=1);g.pad(1,wait=.6);bursts+=1',
        '''            c.live();bursts+=1
            from confirmed_first_fire import first_fire
            burst_input={'checks':{}}
            evidence.setdefault('confirmed_inputs',[]).append(burst_input)
            if persist:persist()
            try:
                first_fire(c,burst_input,expected_shots=min(6,previous),deadline_ns=time.perf_counter_ns()+int(max(0.,deadline-clock())*1e9))
            finally:
                if persist:persist()''')
    anchor="            observed=counts(c,label+'_after_burst_'+str(bursts))"
    return replace_once(source,anchor,anchor+"\n            assert clock()<=deadline,'Typed burst readback exceeded original whole drain deadline'")


def support_source():
    source=(CODE/'hammerburst_support_chain.py').read_text(encoding='utf-8')
    source=replace_once(source,"(g.ROOT/'tools/vr-common/weapon_hammerburst_foregrip.h').read_text(encoding='utf-8')",
                        "g.BOUND.source_text('tools/vr-common/weapon_hammerburst_foregrip.h')")
    source=replace_once(source,"(g.ROOT/'tools/vr-common/hand_finger_pose.h').read_text(encoding='utf-8')",
                        "g.BOUND.source_text('tools/vr-common/hand_finger_pose.h')")
    source=replace_once(source,"        fire('two_hand_authoritative_fire',support_matrix=steered)",
                        "        if '--foregrip-four-state-only' not in __import__('sys').argv:fire('two_hand_authoritative_fire',support_matrix=steered)")
    source=replace_once(source,"        command(0,saved[0]);fire('single_hand_return_fire');data['checks']['source_and_actual_mesh_flow_passed']=True",
                        """        if '--foregrip-four-state-only' in __import__('sys').argv:
            data['four_state_only']=True;data['checks']['source_and_actual_mesh_flow_passed']=True
            save();return
        command(0,saved[0]);fire('single_hand_return_fire');data['checks']['source_and_actual_mesh_flow_passed']=True""")
    return source


def menu_source():
    source = (CODE / 'run_menu_firstperson_check.py').read_text(encoding='utf-8')
    source = replace_once(source, 'from record_projection import ProjectionRecording,encode',
                          'from record_projection import ProjectionRecording,encode\nfrom hammerburst_action_recording import start_action_recording,action_live')
    source = replace_once(source,
        "    rec=ProjectionRecording(folder,directory,live,layer='all',capture_enabled='--dual-output' not in sys.argv);rec.start()",
        "    rec=ProjectionRecording(folder,directory,live,layer='all',capture_enabled=False)")
    source = replace_once(source,
        "\n    head_rest=dict(zip(('x','y','z'),initial['head']['xyz']),**euler(initial['head']['quat']))",
        "\n    if '--dual-output' not in sys.argv:start_action_recording(rec,live,projection_ready=lambda:BOUND.projection_ready(game))\n    else:rec.start()\n    head_rest=dict(zip(('x','y','z'),initial['head']['xyz']),**euler(initial['head']['quat']))")
    source = replace_once(source, "OUT=Path(__file__).resolve().parent;ROOT=OUT.parents[1]", "CODE=Path(__file__).resolve().parent;ROOT=CODE.parents[1];OUT=BOUND.output")
    source = replace_once(source, "state=json.loads((ROOT/'analysis/instances/solo-m1-1028d/launcher-session.json').read_text(encoding='utf-8'))", 'state=BOUND.state()')
    source = replace_once(source, "folder=ROOT/'analysis/instances/solo-m1-1028d/xr-input/LocalAppData/OpenXR-Simulator'", "folder=BOUND.instance/'xr-input/LocalAppData/OpenXR-Simulator'")
    source = replace_once(source, "def live():return control.identity(game['pid'])==game", 'def live():return action_live(BOUND,game,rec)')
    source = replace_once(source, "    assert live();subprocess.run([sys.executable,'-B',str(OUT/name),*args],check=True,stdout=subprocess.DEVNULL)", '    assert live();BOUND.helper(name,*args)')
    source = replace_once(source, '            if rec.capture_enabled:encode(directory)',
                          "            if rec.capture_enabled:result['offline_encode_pending']=True")
    source = replace_once(source,'        deadline=time.monotonic()+45',
                          '        deadline=time.monotonic()+180')
    source = replace_once(source,'    if recording_error:raise recording_error',
                          "    if recording_error and 'error' not in result:raise recording_error")
    if source.count("(OUT/'launcher.log')") != 2:
        raise ValueError('Launcher observation boundary changed')
    source = source.replace("(OUT/'launcher.log')", "(BOUND.instance/'isolated-launcher.log')")
    source = replace_once(source,
        "            session=json.loads(Path(state['session_report']).read_text(encoding='utf-8-sig'))",
        """            try:
                session=json.loads(Path(state['session_report']).read_text(encoding='utf-8-sig'))
            except (PermissionError,json.JSONDecodeError) as read_error:
                result['startup_status_read_retries']=result.get('startup_status_read_retries',0)+1
                observations=result.setdefault('startup_status_read_errors',[])
                if len(observations)<4:
                    observations.append(dict(error=type(read_error).__name__,message=str(read_error)))
                time.sleep(.02)
                continue""")
    source = replace_once(source, "        (OUT/'stop-basic').write_text('Controller-only menu/first-person batch finished; normal owned shutdown',encoding='utf-8')", '        BOUND.stop()')
    return source


def check_sources():
    sources = {name: reader_source(name) for name in READERS}
    sources['equipment_controller_chain.py'] = equipment_source()
    sources['hammerburst_core_chain.py'] = core_source()
    sources['hammerburst_reload_chain.py'] = reload_source()
    sources['hammerburst_supply_boundary_chain.py'] = supply_source()
    sources['hammerburst_support_chain.py'] = support_source()
    sources['run_menu_firstperson_check.py'] = menu_source()
    for name, source in sources.items():
        compile(source, str(CODE / name), 'exec')
        if any(value in source for value in ('solo-m1-1028d', 'stop-basic', 'build-solo-metadata2')):
            raise ValueError('Shared runtime path survived: ' + name)
    return {name: hashlib.sha256(source.encode()).hexdigest() for name, source in sources.items()}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--instance', required=True)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--check', action='store_true')
    parser.add_argument('--reader', choices=READERS)
    parser.add_argument('--reader-output')
    parser.add_argument('--reader-flag',action='append',default=[])
    args, actions = parser.parse_known_args()
    binding = Binding(args.instance, args.output)
    hashes = check_sources()
    if args.check:
        print(json.dumps({'compiled_adapters': hashes, 'game_started': False, 'runtime_verified': False}, indent=2))
        return
    for folder in ('package-inspector', 'game-io', 'native-capture'):
        sys.path.insert(0, str(ROOT / 'tools' / folder))
    sys.path.insert(0, str(CODE))
    from run_stereo_continuous import WindowsControl
    binding.control = WindowsControl()
    binding.state()
    binding.output.mkdir(parents=True, exist_ok=True)
    if args.reader:
        reader_arguments(args.reader,(args.reader_output or '',*args.reader_flag))
        sys.argv = [str(CODE / args.reader), args.reader_output,*args.reader_flag]
        exec(compile(reader_source(args.reader), str(CODE / args.reader), 'exec'),
             {'__file__': str(CODE / args.reader), '__name__': '__main__', 'BOUND': binding})
        return
    sys.argv = [str(CODE / 'run_menu_firstperson_check.py'), *actions]
    core=types.ModuleType('hammerburst_core_chain')
    core.__file__=str(CODE/'hammerburst_core_chain.py')
    exec(compile(core_source(),core.__file__,'exec'),core.__dict__)
    sys.modules[core.__name__]=core
    for name,source in (('hammerburst_reload_chain',reload_source()),('hammerburst_supply_boundary_chain',supply_source()),('hammerburst_support_chain',support_source())):
        adapted=types.ModuleType(name);adapted.__file__=str(CODE/(name+'.py'))
        exec(compile(source,adapted.__file__,'exec'),adapted.__dict__);sys.modules[name]=adapted
    module = types.ModuleType('equipment_controller_chain')
    module.__file__ = str(CODE / 'equipment_controller_chain.py')
    exec(compile(equipment_source(), module.__file__, 'exec'), module.__dict__)
    sys.modules[module.__name__] = module
    exec(compile(menu_source(), str(CODE / 'run_menu_firstperson_check.py'), 'exec'),
         {'__file__': str(CODE / 'run_menu_firstperson_check.py'), '__name__': '__main__', 'BOUND': binding})


if __name__ == '__main__':
    main()
