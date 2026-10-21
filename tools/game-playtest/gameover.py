"""Reviewed GameOver checkpoint recovery and bounded corpse-health evidence."""
import struct

from core import controllable


def reviewed_menu(menu):
    top, leaf = menu.get('top') or {}, menu.get('focused_leaf') or {}
    return (menu.get('read_only') is True and menu.get('consistent_reread') is True
            and menu.get('input_block_count') == 0 and menu.get('player_index') == 0
            and (top.get('tag'), top.get('cls'), top.get('input_mode')) == ('DeltaGameover', 'WarUIScene', 1)
            and (leaf.get('name'), leaf.get('cls')) == ('btnCheckpoint', 'UILabelButton'))


def death_evidence(observer, expected_sample):
    from inspect_ui_state import registered
    observer.verify()
    if not expected_sample or expected_sample['game_identity'] != observer.identities['game_identity']:
        return dict(verified=False, reason='No prior owned pawn observation')
    expected = expected_sample['pawn_identity']
    r = observer.reader.r
    def fresh(address):
        r.ident_cache.pop(address, None)
        return registered(r, address)
    obj = fresh(expected['address'])
    if any(obj.get(key) != expected.get(key) for key in ('address', 'internal_index', 'name', 'number', 'cls', 'outer')):
        raise RuntimeError('Previous controlled pawn no longer has its observed identity')
    prop = r.property(obj['cls'], 'Health', True)
    metadata = r.read(prop['address'] + 0x70, 0x28)
    if (prop['type'] != 'IntProperty' or struct.unpack_from('<II', metadata) != (1, 4)
            or struct.unpack_from('<I', metadata, 0x24)[0] != prop['offset']):
        raise RuntimeError('Post-GameOver health field schema mismatch')
    raw = r.read(obj['address'] + prop['offset'], 4)
    if (r.read(obj['address'] + prop['offset'], 4) != raw or fresh(obj['address']) != obj
            or r.read(prop['address'] + 0x70, 0x28) != metadata):
        raise RuntimeError('Post-GameOver health observation changed')
    observer.verify()
    health = struct.unpack('<i', raw)[0]
    return dict(verified=health <= 0, health=health, pawn_identity=obj,
                source='fresh previous-controlled-pawn health during GameOver', expected_sample=expected_sample.get('sample_id'))


def reload_checkpoint(observer, report, *, expected_sample=None):
    from scenarios import wait_state, tag
    before = wait_state(observer, report, lambda row: reviewed_menu(row.get('menu') or {}), 5)
    generation = before.get('native', {}).get('world_generation')
    if generation is None:
        raise RuntimeError('GameOver world generation is unavailable')
    try:
        death = death_evidence(observer, expected_sample or report.last_controlled_sample)
    except (OSError, RuntimeError, ValueError) as error:
        death = dict(verified=False, reason=str(error))
    report.event('gameover_cause_evidence', **death)
    observer.menu_key('confirm', before['menu'], emit=report.event)
    row = wait_state(observer, report, lambda value: tag(value) != 'DeltaGameover', 30)
    if tag(row) == 'WarfareConfirmation':
        observer.menu_key('confirm', row['menu'], emit=report.event)
    reloaded = wait_state(observer, report, lambda value: controllable(value) and
                          value.get('native', {}).get('world_generation', generation) > generation, 65)
    current, published = reloaded['native']['world_generation'], reloaded['native'].get('published')
    if published is None:
        raise RuntimeError('Reloaded GameOver publication counter is unavailable')
    active = wait_state(observer, report, lambda value: controllable(value) and
                        value.get('native', {}).get('world_generation') == current and
                        value.get('native', {}).get('published', -1) > published, 25)
    evidence = dict(generation_before=generation, generation_after=current,
                    gameover_sample=before['sample_id'], resumed_sample=active['sample_id'],
                    scope='Reviewed checkpoint key, new world, controllable state and advancing publication')
    report.cover('gameover_reload', 'passed', **evidence)
    if death['verified']:
        report.cover('death_reload', 'passed', death=death, **evidence)
    return active
