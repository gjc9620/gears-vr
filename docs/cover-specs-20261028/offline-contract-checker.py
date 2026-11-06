"""Specification reference only. No game imports, process access, native calls or builds.

The synthetic proof labels below are fixture inputs, NEVER production certificates.
The model checks ordering and version contracts, not the existing C++ implementation.
"""
from pathlib import Path
import json, math


class Contract:
    def __init__(self):
        self.context = self.geometry = 1
        self.pose = self.kernel_pose = 0
        self.offset = [0.0, 0.0]
        self.kernel_offset = self.offset.copy()
        self.occupied = True
        self.consumer_uncertain = False
        self.held = False
        self.lease = self.intent_id = 0
        self.axis = 0
        self.anchor_hand = 0.0
        self.anchor_offset = self.offset.copy()
        self.pending = self.proof = self.last_receipt = None
        self.status = 'initial'

    def sync_committed(self):
        # Always before processing input, especially release/regrab/cancel.
        # A committed movement is a fact even if input has not seen its receipt.
        if self.kernel_pose == self.pose:
            return
        self.kernel_pose = self.pose
        self.kernel_offset = self.offset.copy()
        r = self.last_receipt
        if r and r['clipped'] and self.held and r['lease'] == self.lease:
            self.anchor_hand = r['hand']
            self.anchor_offset = self.offset.copy()

    def event(self, e):
        op = e['op']
        if op in ('grab', 'move', 'release', 'invalidate'):
            self.sync_committed()
        if op == 'grab':
            if self.held or not e.get('edge_legal', True) or e.get('incumbent', False):
                self.status = 'grab_denied'; return
            self.lease += 1
            self.held = True
            self.axis = e.get('axis', 0)
            assert self.axis in (0, 1)
            self.anchor_hand = e.get('hand', 0.0)
            self.anchor_offset = self.offset.copy()
            self.status = 'grabbed'
        elif op == 'move':
            if self.consumer_uncertain:
                self.status = 'recovery_needed'; return
            if not self.held:
                self.status = 'move_denied'; return
            self.intent_id += 1
            requested = self.anchor_offset.copy()
            requested[self.axis] -= e['hand'] - self.anchor_hand
            self.pending = dict(id=self.intent_id, context=self.context, geometry=self.geometry,
                                from_pose=self.pose, start=self.offset.copy(), requested=requested,
                                axis=self.axis, hand=e['hand'], lease=self.lease)
            self.proof = None
            self.status = 'pending'
        elif op == 'query':
            p = self.pending
            if not p:
                self.status = 'query_rejected'; return
            expected = dict(intent=p['id'], context=self.context, geometry=self.geometry,
                            from_pose=self.pose)
            actual = {k:e.get(k, v) for k,v in expected.items()}
            if actual != expected or p['geometry'] != self.geometry or p['context'] != self.context:
                self.status = 'query_rejected'; self.proof = None; return
            if e.get('verdict') == 'wait':
                self.status = 'waiting'; self.proof = None; return
            # Fixture labels are assumptions used to explore the specification.
            # They do not certify physics or a region in any real game.
            if not e.get('same_physics_snapshot', True) or not e.get('endpoint_free', True):
                self.status = 'query_rejected'; self.proof = None; return
            accepted = e.get('accepted', p['requested']).copy()
            axis = p['axis']; other = 1-axis
            low,high = sorted((p['start'][axis],p['requested'][axis]))
            if not all(math.isfinite(x) for x in accepted) or not low-1e-9 <= accepted[axis] <= high+1e-9 or abs(accepted[other]-p['start'][other])>1e-9:
                self.status = 'query_rejected'; self.proof = None; return
            self.proof = dict(**actual, accepted=accepted)
            self.status = 'query_ready'
        elif op == 'commit':
            p,q = self.pending,self.proof
            if not p or not q or not self.held or p['lease'] != self.lease or p['from_pose'] != self.pose or q['geometry'] != self.geometry or q['context'] != self.context:
                self.status = 'commit_rejected'; return
            if e.get('consumer_returned', True) is not True:
                # No successful publication is known. This does NOT imply the
                # engine's partially consumed pose can be rolled back safely.
                self.consumer_uncertain = True
                self.occupied = False
                self.status = 'consumer_failed'; self.pending=self.proof=None; return
            self.offset = q['accepted'].copy()
            self.pose += 1
            self.occupied = True
            self.last_receipt = dict(pose=self.pose, offset=self.offset.copy(), lease=self.lease,
                                     hand=p['hand'], clipped=self.offset != p['requested'])
            self.pending=self.proof=None
            self.status = 'committed'
        elif op == 'ack':
            version = e.get('pose', self.pose)
            if version != self.pose:
                self.status = 'ack_rejected'; return
            self.sync_committed(); self.status = 'acknowledged'
        elif op == 'release':
            self.held = False
            self.pending=self.proof=None
            self.status = 'released'
        elif op == 'geometry_change':
            self.geometry += 1
            self.occupied = False
            self.status = 'occupancy_unknown'
        elif op == 'occupancy':
            self.occupied = bool(e['free'])
            self.status = 'occupied_valid' if self.occupied else 'recovery_needed'
        elif op == 'invalidate':
            self.context += 1
            self.held=False; self.pending=self.proof=None; self.occupied=False
            self.status = 'invalidated'
        elif op == 'consume':
            okay = not self.consumer_uncertain and self.occupied and e.get('pose', self.pose)==self.pose and e.get('context', self.context)==self.context and e.get('geometry', self.geometry)==self.geometry
            self.status = 'consumer_ok' if okay else 'consumer_rejected'
        else:
            raise ValueError('Unknown specification event: '+op)

    def view(self):
        return dict(offset=self.offset, kernel_offset=self.kernel_offset, pose=self.pose,
                    kernel_pose=self.kernel_pose, held=self.held, pending=self.pending is not None,
                    status=self.status, occupied=self.occupied, consumer_uncertain=self.consumer_uncertain)


def close(a,b):
    if isinstance(a,list) and isinstance(b,list):
        return len(a)==len(b) and all(close(x,y) for x,y in zip(a,b))
    if type(a) is float or type(b) is float:
        return isinstance(a,(int,float)) and isinstance(b,(int,float)) and abs(a-b)<1e-8
    return a==b


def run(path):
    data=json.loads(path.read_text(encoding='utf-8')); rows=[]
    for case in data['cases']:
        c=Contract();checks=0
        for i,event in enumerate(case['events']):
            c.event(event)
            for key,want in event.get('expect',{}).items():
                actual=c.view()[key]; checks+=1
                assert close(actual,want), (case['id'],i,key,actual,want)
        for key,want in case['expect'].items():
            actual=c.view()[key];checks+=1
            assert close(actual,want),(case['id'],'final',key,actual,want)
        rows.append(dict(id=case['id'],passed=True,assertions=checks))
    # Algebra examples: real HMD translation and virtual translation each once.
    extra=[]
    for example in data['composition_examples']:
        actual=[a+b for a,b in zip(example['virtual'],example['physical'])]
        assert close(actual,example['expected']),example['id']
        extra.append(dict(id=example['id'],passed=True))
    return dict(schema=1,kind='specification reference model, not production tests',passed=True,
                cases=len(rows),assertions=sum(r['assertions'] for r in rows),
                composition_examples=len(extra),rows=rows,composition=extra,
                native_calls=0,production_cpp_executed=False,game_run=False,
                note='Synthetic proof labels explore contract ordering; no real collision, damage or headset validation.')


if __name__=='__main__':
    import argparse
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--vectors',type=Path,default=Path(__file__).with_name('contract-vectors.json'))
    p.add_argument('--output',type=Path)
    args=p.parse_args();result=run(args.vectors)
    if args.output:
        if args.output.resolve() in (args.vectors.resolve(),Path(__file__).resolve()):
            p.error('Output must not overwrite the vectors or checker.')
        args.output.write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({k:v for k,v in result.items() if k not in ('rows','composition')},ensure_ascii=False))
