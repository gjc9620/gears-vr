"""Arithmetic witness for the retained vertex branch; not a native replay."""
from pathlib import Path
import hashlib
import json
import math
import struct

HERE=Path(__file__).resolve().parent
f32=lambda x:struct.unpack('<f',struct.pack('<f',x))[0]
face=json.loads((HERE/'sphere-face.json').read_text())
assembly=face['raw_result']['assembly']
assert '0x1800d823b: VMOVSS XMM2, dword ptr [0x18022a510]' in assembly
assert '0x1800d824c: JA 0x1800d829a' in assembly
assert '0x1800d826c: JA 0x1800d829a' in assembly
assert '0x1800d829a: XOR AL, AL' in assembly
# A front-facing triangle at X=150, with the near vertex at (150,0,0).
# The ray's projected face contact is outside both vertex-adjacent edges.
origin=(0.,-.5,-.5)
triangle=((150.,0.,0.),(150.,0.,10.),(150.,10.,0.))
direction=(1.,0.,0.)
radius=1.
relative=tuple(f32(triangle[0][i]-origin[i]) for i in range(3))
projection=f32(sum(f32(relative[i]*direction[i]) for i in range(3)))
distance2=f32(sum(f32(x*x) for x in relative))
perpendicular2=f32(distance2-f32(projection*projection))
root=f32(projection-f32(math.sqrt(f32(radius*radius-perpendicular2))))
requested=200.
assert 0<root<requested and radius<f32(projection-100.)
result=dict(scope='Analyst-derived float32 vertex-branch arithmetic; original binary never loaded or executed. Not a full triangle/native call replay.',
    evidence_id=face['evidence_id'],origin=origin,triangle=triangle,direction=direction,radius=radius,
    vertex_projection=projection,vertex_distance2=distance2,perpendicular2=perpendicular2,
    theoretical_vertex_entry=root,caller_requested_distance=requested,
    retained_branch_rejects_at_lower_bound=True,cutoff_native_units=100.,
    full_dispatch_applicability_certified=False,movement_authorized=False,
    source_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
(HERE/'vertex-cutoff-witness.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
print(json.dumps(result))
