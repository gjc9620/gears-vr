"""Read the player's candidate view-state and compare bounded history fields.

No native calls, hooks or writes. A prior CSV is a historical reference, not
an atomic observation of the current frame. Pointer equality is process-local.
"""
import argparse
import csv
import json
import math
import pathlib
import re
import struct
from resolve_camera import Resolver


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--pid',type=int,required=True)
    p.add_argument('--capture',type=pathlib.Path,required=True)
    p.add_argument('--output',type=pathlib.Path,required=True)
    a=p.parse_args()
    status=a.capture.with_suffix('.status.txt').read_text()
    if not re.search(rf'^capturing pid={a.pid} ',status,re.MULTILINE):raise RuntimeError('Capture belongs to a different process')
    last_scene=None;associated=set();scene_count=0
    with a.capture.with_suffix('.csv').open(newline='') as stream:
        for row in csv.DictReader(stream):
            if row['kind']=='1' and int(row['valid'])&5==5:last_scene=row;scene_count+=1
            if row['kind']=='12' and int(row['valid'])&8192:associated.add(int(row['renderer_source0'],16))
            if len(associated)>64:raise RuntimeError('Associated-pointer inventory exceeds bound')
    if last_scene is None:raise RuntimeError('No valid source-view reference')
    profile=json.loads(pathlib.Path(__file__).with_name('reloaded-profile.json').read_text())
    r=Resolver(a.pid,profile)
    try:
        camera=r.resolve();local=int(camera['local_player_address'],16)
        state=r.u64(local+0xcc);table=r.u64(state)
        if table!=r.base+0x1353918:raise RuntimeError('Unrecognized candidate view-state table')
        before=r.read(state,0x280);after=r.read(state,0x280)
        if r.u64(local+0xcc)!=state:raise RuntimeError('Player view-state changed during read')
        reference=[float(last_scene[f'matrix{i}']) for i in range(32)]
        comparisons=[]
        for offset,indices in ((0x210,range(16)),(0xe0,range(16,32))):
            values=struct.unpack_from('<16f',after,offset);expected=[reference[i] for i in indices]
            finite=all(math.isfinite(v) for v in values)
            comparisons.append({'state_offset':hex(offset),'finite':finite,
                'max_abs_difference_from_last_recorded_source':max(abs(v-e) for v,e in zip(values,expected)) if finite else None})
        report={'pid':a.pid,'local_player':hex(local),'state':hex(state),'table_rva':hex(table-r.base),
            'two_immediate_reads_equal':before==after,'matches_recorded_associated_pointer':state in associated,
            'associated_pointer_variants':len(associated),'reference_source_samples':scene_count,
            'history_comparisons':comparisons,
            'limits':['Candidate field relationships, not symbol/type proof.',
                'Prior capture and current state are not an atomic frame pair.',
                'Immediate byte equality does not prove thread safety.',
                'No independent per-eye state was created or used.']}
        a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2))
        print(json.dumps(report))
    finally:r.p.close()


if __name__=='__main__':main()
