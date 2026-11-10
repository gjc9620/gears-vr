"""Independent exact-rational checks of emitted native float geometry."""
from fractions import Fraction as F
from pathlib import Path
import copy
import hashlib
import json
import struct
import subprocess

HERE=Path(__file__).resolve().parent
EXE=HERE/'checks-v8/cover_physx_capsule_path.exe'


def number(bits):
    value=struct.unpack('<f',struct.pack('<I',bits))[0]
    return F.from_float(value)


def vector(bits):
    assert len(bits)==3
    return tuple(number(value) for value in bits)


def check(plan):
    count=plan['count']
    assert 0<count<=64 and count&(count-1)==0 and len(plan['segments'])==count
    displacement=vector(plan['requested'])
    original=[vector(plan[key]) for key in ('original_first','original_second')]
    radius=number(plan['original_radius'])
    maximum_error=F.from_float(plan['maximum_error'])
    expansions=[]
    assert radius>0 and original[0]!=original[1]
    for index,segment in enumerate(plan['segments']):
        step=vector(segment['movement'])
        assert all(step[axis]*count==displacement[axis] for axis in range(3))
        assert sum(value*value for value in step)<=10000
        padding=number(segment['radius'])-radius
        expansions.append(padding)
        assert padding>=0
        for endpoint,key in enumerate(('first','second')):
            native=vector(segment[key])
            exact=tuple(original[endpoint][axis]+displacement[axis]*F(index,count) for axis in range(3))
            errors=tuple(native[axis]-exact[axis] for axis in range(3))
            assert sum(abs(value) for value in errors)<=maximum_error
            assert sum(value*value for value in errors)<=padding*padding
            # The movement equals the ideal increment exactly. Endpoint error
            # is therefore constant over the entire segment, not just samples.
            assert all(native[axis]+step[axis]-(exact[axis]+displacement[axis]/count)==errors[axis]
                       for axis in range(3))
        # A shaft point is a convex combination of its endpoint errors, whose
        # Euclidean norm is <=padding. Minkowski addition of the original ball
        # is covered by the inflated capsule over the complete time interval.
    final_padding=number(plan['endpoint_radius'])-radius
    expansions.append(final_padding)
    assert final_padding>=0
    for endpoint,key in enumerate(('endpoint_first','endpoint_second')):
        actual=vector(plan[key])
        error=[actual[axis]-original[endpoint][axis]-displacement[axis] for axis in range(3)]
        assert sum(abs(value) for value in error)<=maximum_error
        assert sum(value*value for value in error)<=final_padding*final_padding
    assert max(expansions)==F.from_float(plan['maximum_expansion'])


run=subprocess.run([str(EXE),'--emit-witness'],capture_output=True,check=True,
                   creationflags=subprocess.CREATE_NO_WINDOW)
rows=[json.loads(line) for line in run.stdout.splitlines() if line]
assert len(rows)==27
for row in rows:
    check(row)
negative=[]
tampered=copy.deepcopy(rows[2]);tampered['segments'][1]['movement'][0]^=1;negative.append(tampered)
tampered=copy.deepcopy(rows[2]);tampered['segments'].pop();negative.append(tampered)
tampered=copy.deepcopy(rows[2])
for segment in tampered['segments']:
    segment['radius']=tampered['original_radius']
negative.append(tampered)
for row in negative:
    try:
        check(row)
    except AssertionError:
        pass
    else:
        raise RuntimeError('The independent checker accepted damaged path geometry')
(HERE/'geometry-witness.jsonl').write_bytes(run.stdout)
result=dict(plans=len(rows),segments=sum(row['count'] for row in rows),negative_controls=len(negative),
            exact_rational_coverage=True,sampled_time_proof=False,
            scope='Supplied rigid translating capsule plus final capsule; no original query, world, articulated pose, body or occupancy proof.',
            exe_sha256=hashlib.sha256(EXE.read_bytes()).hexdigest(),
            witness_sha256=hashlib.sha256(run.stdout).hexdigest(),
            checker_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
            movement_authorized=False,game_started=False)
(HERE/'exact-coverage.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
print(json.dumps(result))
