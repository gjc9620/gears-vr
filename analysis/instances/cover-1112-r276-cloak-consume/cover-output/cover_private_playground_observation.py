"""Read-only adaptation for the already-owned empty-UI Playground scene.

The shared campaign Observer and its menu/input policy are unchanged. This
adapter grants no input, query, C0, cover or weapon permission.
"""
import json


def empty_playground(menu):
    return (menu.get('consistent_reread') is True and menu.get('read_only') is True
            and menu.get('input_block_count') == 0 and 'top' in menu and menu['top'] is None
            and menu.get('active_scenes') == [])


def sample(observer, root, expected_controller, expected_pawn):
    from anchor_config import AnchorRegistryChanged
    # Discovery may finish just before an object-registry allocation. A failed
    # bracket is discarded, never accepted or patched. Rebuild discovery once
    # under the same live session; this read-only adapter sends no input.
    session = observer.verify()
    keys = ('session_id', 'game_identity', 'owner_identity', 'xr_identity')
    bound = {key: session[key] for key in keys}
    failures = []
    for attempt in range(2):
        if attempt:
            current = observer.verify()
            if any(current.get(key) != bound[key] for key in keys):
                raise RuntimeError('Playground session changed during registry reacquisition')
        try:
            row = _sample_once(observer, root, expected_controller, expected_pawn)
            if failures:
                row['registry_reacquisition'] = failures
            return row
        except AnchorRegistryChanged as error:
            failures.append(dict(reason=str(error), evidence=getattr(error, 'evidence', None)))
            if attempt:
                error.registry_reacquisition = failures
                raise


def _sample_once(observer, root, expected_controller, expected_pawn):
    from anchor_config import LiveRegistryAnchorSampler, acquire_anchor_sample
    from resolve_camera import Resolver
    row = observer.sample()
    if 'special_move_raw' in row:
        return row
    if row['errors'] or not empty_playground(row.get('menu') or {}):
        raise RuntimeError('No certified player observation: expected owned empty-UI Playground')
    session = observer.verify()
    if session.get('unified_weapons', {}).get('hand_connected') is not True:
        raise RuntimeError('Owned Playground connection is not ready')
    try:
        if observer.sampler is None:
            profile = json.loads((root / 'tools/package-inspector/reloaded-profile.json').read_text(encoding='utf-8'))
            observer.resolver = Resolver(observer.identities['game_identity']['pid'], profile)
            observer.sampler = LiveRegistryAnchorSampler(observer.resolver, cinematic_camera=True)
        sampler = observer.sampler
        anchor, evidence = acquire_anchor_sample(sampler)
        proof = (anchor.get('guard_before') or {}).get('target_proof') or {}
        if (proof.get('cinematic') is not False or sampler.controller != expected_controller
                or sampler.pawn != expected_pawn):
            raise RuntimeError('Playground anchor/controlled actor differs')
        pawn = sampler.fresh(sampler.pawn)
        values = anchor['values']
        state = dict(pawn_identity=pawn, cinematic=False,
                     special_move_raw=observer._byte_property(pawn, 'SpecialMove'),
                     health=observer._int_property(pawn, 'Health'),
                     cover_state_raw={name: observer._byte_property(pawn, name)
                                      for name in ('CoverType', 'CoverAction')},
                     pose=dict(position=values['pawn_location'],
                               yaw_ru=values['controller_rotation'][1],
                               controller_rotation_ru=values['controller_rotation'],
                               camera_position=values['camera_cache_pov']['location'],
                               camera_rotation_ru=values['camera_cache_pov']['rotation_ru'],
                               pawn_rotation_ru=values['pawn_rotation'],
                               fov=values['camera_cache_pov']['fov']),
                     observation_adapter='owned_empty_ui_playground',
                     anchor_acquisition={key: evidence.get(key) for key in
                                         ('started_perf_counter_ns', 'finished_perf_counter_ns')})
        if (state['health'] <= 0 or sampler.fresh(pawn['address']) != pawn or not empty_playground(observer.menu())
                or observer.verify().get('session_id') != session['session_id']):
            raise RuntimeError('Playground observation changed at closing boundary')
        row.update(state)
        return row
    except BaseException:
        observer.close_pose()
        raise
