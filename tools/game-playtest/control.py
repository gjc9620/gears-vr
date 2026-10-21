"""Run-local, identity-bound input hold and target requests. No game writes here."""
import argparse
import hashlib
import json
import math
from pathlib import Path

from core import atomic_json, controllable


def validate(request, identities, sample, last_sequence):
    if not isinstance(request, dict):
        raise ValueError('Control request must be an object')
    if (request.get('schema') != 1 or type(request.get('sequence')) is not int or
            request['sequence'] <= last_sequence or
            request.get('game_identity') != identities['game_identity'] or
            request.get('owner_identity') != identities['owner_identity']):
        raise ValueError('Control request identity/schema/sequence mismatch')
    action = request.get('action')
    if action not in ('hold', 'resume', 'target', 'auto'):
        raise ValueError('Unsupported control action')
    if action in ('target', 'auto'):
        if not controllable(sample) or sample.get('errors'):
            return False
        if (request.get('pawn_identity') != sample.get('pawn_identity') or
                request.get('world_generation') != sample.get('native', {}).get('world_generation')):
            raise ValueError('Target request pawn/world context is stale')
        goal = request.get('target')
        if action == 'target' and (request.get('kind') not in ('walk', 'interaction') or not isinstance(goal, list) or len(goal) != 3 or
                not all(type(v) in (int, float) and math.isfinite(v) for v in goal) or
                math.dist(goal, sample['pose']['position']) > 30000):
            raise ValueError('Target request is outside bounded current-world navigation')
    return True


class RunControl:
    def __init__(self, observer, report, *, persistent=False, automatic=False):
        self.observer, self.report = observer, report
        self.path = report.directory / 'control-request.json'
        self.sequence, self.digest, self.deferred = 0, None, None
        self.held = self.awaiting_target = False
        self.persistent = persistent
        self.target_context = None
        self.automatic = automatic
        self.last_position = None

    def status(self, sample, reason):
        row = dict(last_sequence=self.sequence, held=self.held, awaiting_target=self.awaiting_target,
                   reason=reason, sample=sample.get('sample_id'), game_identity=self.observer.identities['game_identity'],
                   owner_identity=self.observer.identities['owner_identity'], pawn_identity=sample.get('pawn_identity'),
                   world_generation=sample.get('native', {}).get('world_generation'),
                   owned_call_completed=True, active_mouse_holds=list(getattr(self.observer, '_mouse_holds', {})))
        if row['active_mouse_holds']:
            raise RuntimeError('Control acknowledgement cannot overlap an owned mouse hold')
        atomic_json(self.report.directory / 'control-status.json', row)
        self.report.event('control_status', **row)

    def hold(self, sample, reason, *, awaiting_target=False):
        self.held, self.awaiting_target = True, awaiting_target
        self.persistent = True
        self.status(sample, reason)

    def poll(self, sample, *, target_allowed=True):
        position = (sample.get('pose') or {}).get('position')
        if (self.observer.action_owner == self.observer.identities['owner_identity'] and
                not sample.get('errors') and sample.get('pawn_identity') and
                sample.get('native', {}).get('world_generation') is not None and
                isinstance(position, (tuple, list)) and len(position) == 3 and
                all(type(v) in (int, float) and math.isfinite(v) for v in position)):
            context = (sample['pawn_identity'], sample['native']['world_generation'])
            previous = self.last_position
            self.last_position = (context, tuple(position), sample.get('sample_id'))
            if previous and previous[0] == context and math.dist(previous[1], position) > 1000:
                # Authored repositioning can retain pawn, world generation and
                # cinematic=false. A previous route cannot survive this jump.
                self.observer.verify()
                self.target_context = None
                self.report.event('control_position_discontinuity', before=previous[1], after=position,
                                  distance=math.dist(previous[1], position), before_sample=previous[2],
                                  sample=sample.get('sample_id'), automatic=self.automatic,
                                  cause_verified=False)
                if self.automatic:
                    return dict(action='reposition')
                self.hold(sample, 'Large position change; a fresh target is required', awaiting_target=True)
                return dict(action='invalidate')
        if (self.target_context and sample.get('pawn_identity') is not None and
                sample.get('native', {}).get('world_generation') is not None and
                self.target_context != (sample['pawn_identity'], sample['native']['world_generation'])):
            self.target_context = None
            self.hold(sample, 'Target context changed; a fresh target is required', awaiting_target=True)
            return dict(action='invalidate')
        if not self.path.exists():
            return None
        with self.path.open('rb') as stream:
            raw = stream.read(16385)
        digest = hashlib.sha256(raw).hexdigest()
        if digest == self.digest:
            return None
        try:
            with self.path.open('rb') as stream:
                reread = stream.read(16385)
            if len(raw) > 16384 or reread != raw:
                raise ValueError('Control request changed or exceeds size bound')
            request = json.loads(raw)
            if self.observer.action_owner != self.observer.identities['owner_identity']:
                raise ValueError('Control requires this run owned launcher')
            self.observer.verify()
            ready = validate(request, self.observer.identities, sample, self.sequence)
            if request['action'] in ('target', 'auto') and (not ready or not target_allowed):
                if self.deferred != digest:
                    self.report.event('control_request_deferred', sequence=request['sequence'], sha256=digest)
                    self.deferred = digest
                return None
            if request['action'] == 'resume' and self.awaiting_target:
                raise ValueError('A new target is required before resuming')
            self.digest, self.sequence, self.persistent = digest, request['sequence'], True
            if request['action'] == 'hold':
                self.held = True
            elif request['action'] == 'resume':
                self.held = False
            else:
                self.awaiting_target = False
                self.automatic = request['action'] == 'auto'
                self.target_context = ((sample['pawn_identity'], sample['native']['world_generation'])
                                       if request['action'] == 'target' else None)
            self.report.event('control_request_accepted', request=request, sha256=digest)
            self.status(sample, request['action'])
            return request
        except (OSError, RuntimeError, ValueError, TypeError, KeyError) as error:
            self.digest = digest
            self.report.event('control_request_rejected', reason=str(error), sha256=digest)
            return None


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run', type=Path)
    parser.add_argument('action', choices=('hold', 'resume', 'target', 'auto'))
    parser.add_argument('--target', type=float, nargs=3)
    parser.add_argument('--kind', choices=('walk', 'interaction'), default='walk')
    args = parser.parse_args()
    from live import Observer
    report = json.loads((args.run / 'report.json').read_text(encoding='utf8'))
    if report['mode'] != 'run' or report['phase'] in ('finished', 'incomplete', 'cleanup'):
        raise RuntimeError('Control requires a currently running owned test')
    o = Observer(report['provenance']['session_path'])
    try:
        sample = o.sample()
        status_path = args.run / 'control-status.json'
        status = json.loads(status_path.read_text(encoding='utf8')) if status_path.exists() else {}
        request = dict(schema=1, sequence=status.get('last_sequence', 0) + 1, action=args.action,
                       game_identity=o.identities['game_identity'], owner_identity=o.identities['owner_identity'])
        if args.action in ('target', 'auto'):
            request.update(pawn_identity=sample.get('pawn_identity'),
                           world_generation=sample.get('native', {}).get('world_generation'))
        if args.action == 'target':
            request.update(target=args.target, kind=args.kind)
        if not validate(request, o.identities, sample, status.get('last_sequence', 0)):
            raise RuntimeError('Fresh controllable state required for a target request')
        atomic_json(args.run / 'control-request.json', request)
        print(json.dumps(dict(submitted=True, action=args.action, sequence=request['sequence'],
                              acknowledgement=str(status_path))))
    finally:
        o.close()


if __name__ == '__main__':
    main()
