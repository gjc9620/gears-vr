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
READERS = ('read_private_input.py', 'read_world_grips.py', 'read_aim_metrics.py')


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
        return state

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
        if name not in READERS or len(args) != 1 or Path(args[0]).name != args[0]:
            raise ValueError('Unexpected HB reader or output name')
        self.state()
        subprocess.run([sys.executable, '-E', '-s', '-B', '-X', 'utf8', str(Path(__file__).resolve()),
                        '--instance', self.name, '--output', str(self.output), '--reader', name,
                        '--reader-output', args[0]], check=True, stdout=subprocess.DEVNULL,
                       creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))

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
    if name == 'read_aim_metrics.py':
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
    source = replace_once(source, "g.ROOT/'analysis/instances/solo-m1-1028d/manifest.json'", "g.BOUND.instance/'manifest.json'")
    source = replace_once(source, "g.ROOT/'tools/playground/build-solo-metadata2'/f'playground-{g.game[\"pid\"]}.log'",
                          "g.ROOT/json.loads((g.BOUND.instance/'manifest.json').read_text())['playground_build']/f'playground-{g.game[\"pid\"]}.log'")
    needle = "        initial=current('before_original_X_pickup')"
    return replace_once(source, needle, needle + '\n        from types import SimpleNamespace\n        from hammerburst_core_chain import run as hb_run\n        hb_run(SimpleNamespace(**locals()))\n        return')


def core_source():
    source=(CODE/'hammerburst_core_chain.py').read_text(encoding='utf-8')
    old="""            before=len(c.records('native_lancer_shot_trajectory'))
            g.pad(1,wait=.35,trigger=1);g.pad(1,wait=.6)
            shots=c.records('native_lancer_shot_trajectory')[before:];data['shots']=shots
            assert shots and all(x.get('trace_valid')=='1' for x in shots),'No authoritative HB trajectory'
            assert len(shots)==6,('Original HB burst must export exactly six valid shots',len(shots))
            data['checks']['actual_fire']=True"""
    return replace_once(source,old,"            from confirmed_first_fire import first_fire\n            first_fire(c,data)")


def menu_source():
    source = (CODE / 'run_menu_firstperson_check.py').read_text(encoding='utf-8')
    source = replace_once(source, "OUT=Path(__file__).resolve().parent;ROOT=OUT.parents[1]", "CODE=Path(__file__).resolve().parent;ROOT=CODE.parents[1];OUT=BOUND.output")
    source = replace_once(source, "state=json.loads((ROOT/'analysis/instances/solo-m1-1028d/launcher-session.json').read_text(encoding='utf-8'))", 'state=BOUND.state()')
    source = replace_once(source, "folder=ROOT/'analysis/instances/solo-m1-1028d/xr-input/LocalAppData/OpenXR-Simulator'", "folder=BOUND.instance/'xr-input/LocalAppData/OpenXR-Simulator'")
    source = replace_once(source, "def live():return control.identity(game['pid'])==game", 'def live():return BOUND.live(game)')
    source = replace_once(source, "    assert live();subprocess.run([sys.executable,'-B',str(OUT/name),*args],check=True,stdout=subprocess.DEVNULL)", '    assert live();BOUND.helper(name,*args)')
    source = replace_once(source, '            if rec.capture_enabled:encode(directory)',
                          "            if rec.capture_enabled:result['offline_encode_pending']=True")
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
        if not args.reader_output or Path(args.reader_output).name != args.reader_output:
            raise ValueError('Expected one local reader output name')
        sys.argv = [str(CODE / args.reader), args.reader_output]
        exec(compile(reader_source(args.reader), str(CODE / args.reader), 'exec'),
             {'__file__': str(CODE / args.reader), '__name__': '__main__', 'BOUND': binding})
        return
    sys.argv = [str(CODE / 'run_menu_firstperson_check.py'), *actions]
    core=types.ModuleType('hammerburst_core_chain')
    core.__file__=str(CODE/'hammerburst_core_chain.py')
    exec(compile(core_source(),core.__file__,'exec'),core.__dict__)
    sys.modules[core.__name__]=core
    module = types.ModuleType('equipment_controller_chain')
    module.__file__ = str(CODE / 'equipment_controller_chain.py')
    exec(compile(equipment_source(), module.__file__, 'exec'), module.__dict__)
    sys.modules[module.__name__] = module
    exec(compile(menu_source(), str(CODE / 'run_menu_firstperson_check.py'), 'exec'),
         {'__file__': str(CODE / 'run_menu_firstperson_check.py'), '__name__': '__main__', 'BOUND': binding})


if __name__ == '__main__':
    main()
