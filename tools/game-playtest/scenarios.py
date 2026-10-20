"""Reviewed UI and weapon scenarios using current state, without visual capture."""
import time

from core import controllable, gameplay


def wait_state(observer, report, predicate, seconds=25):
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        if (report.directory / 'stop').exists() or observer.user.GetAsyncKeyState(0x77) & 0x8000:
            raise RuntimeError('Scenario cancelled')
        row = report.observe(observer.sample())
        menu = row.get('menu') or {}
        ready = (menu.get('consistent_reread') is True and menu.get('read_only') is True
                 and menu.get('input_block_count') == 0)
        if ready and predicate(row):
            return row
        time.sleep(.2)
    raise TimeoutError('Expected scenario state was not observed within bound')


def playable(row):
    return controllable(row)


def combat(observer, report):
    before = wait_state(observer, report, playable)
    bindings = observer.bindings()
    if (bindings.get('RightMouseButton') != 'Game_Target' or
            bindings.get('LeftMouseButton') != 'Game_Shoot'):
        report.cover('aim_fire', 'blocked', reason='Current aim/fire bindings differ', bindings=bindings)
        return
    aim = observer.pulse('RightMouseButton', .55, expected_bindings=bindings,
                         emit=report.event, during=report.observe,
                         stop_requested=lambda: (report.directory / 'stop').exists())
    held = aim.pop('held_observation', None)
    held_fov = ((held or {}).get('pose') or {}).get('fov')
    base_fov = before['pose'].get('fov')
    aim_ok = (held is not None and playable(held) and base_fov is not None
              and held_fov is not None and held_fov < base_fov - 1
              and held.get('pawn_identity') == before.get('pawn_identity'))
    report.event('aim_evidence', before=before['sample_id'], held=(held or {}).get('sample_id'),
                 fov_before=base_fov, fov_held=held_fov, aim_observed=aim_ok)
    time.sleep(.4)
    before = wait_state(observer, report, playable)
    shot = observer.pulse('LeftMouseButton', .45, expected_bindings=bindings,
                          emit=report.event,
                          stop_requested=lambda: (report.directory / 'stop').exists())
    shot.pop('held_observation', None)
    time.sleep(.25)
    after = wait_state(observer, report, playable)
    result = report.action('fire', before, after, dispatched=shot['release_posted'], detail=shot)
    fire_ok = result['result'] == 'observed'
    report.cover('aim_fire', 'passed' if aim_ok and fire_ok else 'blocked',
                 aim_observed=aim_ok, fire_observed=fire_ok, action_id=result['action_id'],
                 scope='FOV and ammunition response only; enemy targeting/clearance not established')


def tag(row):
    return ((row.get('menu') or {}).get('top') or {}).get('tag')


def leaf(row):
    return ((row.get('menu') or {}).get('focused_leaf') or {}).get('name')


def pause_resume(observer, report):
    before = wait_state(observer, report, playable)
    observer.menu_key('escape', before['menu'], emit=report.event)
    paused = wait_state(observer, report, lambda r: tag(r) == 'DeltaPause-Campaign-Host'
                        and leaf(r) == 'LBtnResumeSingle')
    observer.menu_key('escape', paused['menu'], emit=report.event)
    resumed = wait_state(observer, report, playable)
    counter = (resumed.get('native') or {}).get('published')
    if counter is None:
        raise RuntimeError('Resume publication counter unavailable')
    active = wait_state(observer, report, lambda r: playable(r) and
                        (r.get('native') or {}).get('published', -1) > counter)
    report.cover('pause_resume', 'passed', paused_sample=paused['sample_id'],
                 resumed_sample=active['sample_id'], scope='Menu roundtrip and resumed stereo publication')


def checkpoint(observer, report):
    before = wait_state(observer, report, playable)
    generation = (before.get('native') or {}).get('world_generation')
    if generation is None:
        raise RuntimeError('Current world generation is unavailable')
    observer.menu_key('escape', before['menu'], emit=report.event)
    paused = wait_state(observer, report, lambda r: tag(r) == 'DeltaPause-Campaign-Host'
                        and leaf(r) == 'LBtnResumeSingle')
    observer.menu_key('down', paused['menu'], emit=report.event)
    selected = wait_state(observer, report, lambda r: tag(r) == 'DeltaPause-Campaign-Host'
                          and leaf(r) == 'LBtnChapterSingle')
    observer.menu_key('confirm', selected['menu'], emit=report.event)
    confirmation = wait_state(observer, report, lambda r: tag(r) == 'WarfareConfirmation'
                              and ((r.get('menu') or {}).get('dialog_text') or {}).get('Title') ==
                              '<Strings:WarfareGame.Generic.ConfirmCheckPointLoad_Title>')
    observer.menu_key('confirm', confirmation['menu'], emit=report.event)
    reloaded = wait_state(observer, report, lambda r: playable(r) and
                         (r.get('native') or {}).get('world_generation', generation) > generation, 50)
    new_generation = reloaded['native']['world_generation']
    published = reloaded['native'].get('published')
    if published is None:
        raise RuntimeError('New-world publication counter unavailable')
    active = wait_state(observer, report, lambda r: playable(r) and
                        (r.get('native') or {}).get('world_generation') == new_generation
                        and (r.get('native') or {}).get('published', -1) > published)
    report.cover('checkpoint_transition', 'passed', generation_before=generation,
                 generation_after=new_generation, sample=active['sample_id'],
                 scope='Normal checkpoint UI, new world and resumed stereo publication')


def exercise(observer, report):
    for name, scenario in (('aim_fire', combat), ('pause_resume', pause_resume),
                           ('checkpoint_transition', checkpoint)):
        report.save(phase='scenario_' + name)
        report.event('scenario_started', name=name)
        try:
            scenario(observer, report)
        except Exception as error:
            report.cover(name, 'blocked', reason=str(error))
            report.issue('scenario_unverified', name + ': ' + str(error), scope=name,
                         evidence=dict(type=type(error).__name__, sample=report.samples))
            # Unknown UI is never driven by later scenarios.
            raise
        report.event('scenario_finished', name=name)
