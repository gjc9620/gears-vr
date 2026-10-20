"""Explicit owned-test health assistance; excluded from normal survival evidence."""
import struct

from axis_input import PrivateWriter
from core import controllable
from world_state import Query


def maintain_health(observer, report, sample, *, writer_factory=PrivateWriter):
    from live import InputNotReady, background_input
    if observer.action_owner != observer.identities['owner_identity']:
        raise RuntimeError('Health assistance requires this test owned launcher')
    if sample.get('errors') or not controllable(sample):
        raise InputNotReady('Health assistance requires a live controllable pawn')
    if sample['health'] >= 1500:
        return None
    identity = observer.identities['game_identity']
    observer.verify()
    if background_input.change(identity['pid'], 'status', expected_creation=identity['creation_time'])['state'] != 'original':
        raise RuntimeError('Physical background controller input must remain disabled')
    desk = observer.desktop()
    if desk['occlusion'] or desk['foreground'] == observer.backend.game_window(identity['pid']):
        raise InputNotReady('Health assistance requires a nonforeground game behind other windows')
    if (report.directory / 'stop').exists() or observer.user.GetAsyncKeyState(0x77) & 0x8000:
        raise InputNotReady('Health assistance cancelled')
    q = Query(observer)
    controller, pawn = q.obj(observer.sampler.controller), q.obj(observer.sampler.pawn)
    world = q.pointer(pawn, 'WorldInfo')
    if (pawn != sample['pawn_identity'] or not q.s.derives(pawn['cls'], 'WarPawn') or
            q.pointer(controller, 'Pawn') != pawn or q.pointer(pawn, 'Controller') != controller or
            world is None or world != q.pointer(controller, 'WorldInfo')):
        raise InputNotReady('Health assistance pawn ownership/world changed')
    prop = q.prop(pawn, 'Health', 'IntProperty')
    before = q.scalar(pawn, 'Health', 'IntProperty', '<i')
    if prop['size'] != 4 or not 0 < before < 1500:
        raise InputNotReady('Health assistance requires a positive certified IntProperty below its floor')
    q.finish(allow_registry_growth=True)
    fresh = observer.sample()
    if (fresh.get('errors') or not controllable(fresh) or fresh.get('pawn_identity') != pawn or
            fresh['native']['world_generation'] != sample['native']['world_generation']):
        raise InputNotReady('Health assistance control context changed before write')
    q.finish(allow_registry_growth=True)
    report.assisted = True
    state = report.provenance.setdefault('health_assistance', dict(enabled=True, writes=0, attempts=0,
        floor=1500, target=9999, survival_acceptance=False))
    state['attempts'] += 1
    report.event('test_health_assistance_started', pawn=pawn, property=prop, health_before=before, requested=9999)
    writer = writer_factory(identity)
    try:
        writer.write(pawn['address'] + prop['offset'], struct.pack('<i', 9999))
        after = q.scalar(pawn, 'Health', 'IntProperty', '<i')
        q.finish(allow_registry_growth=True)
        if not 1500 <= after <= 9999:
            raise RuntimeError('Test health assistance readback did not remain above its floor')
    finally:
        writer.close()
    receipt = dict(pawn=pawn, world_generation=sample['native']['world_generation'],
                   property=prop, health_before=before, requested=9999, health_after=after,
                   writer_closed=True, survival_acceptance=False)
    report.event('test_health_assistance', **receipt)
    state['writes'] += 1
    state['last_receipt'] = receipt
    return receipt
