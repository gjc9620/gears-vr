"""Invert observed raw-hand/world differences; no game access or input API."""
import math
def sub(a,b):return tuple(x-y for x,y in zip(a,b))
def dot(a,b):return sum(x*y for x,y in zip(a,b))
def cross(a,b):return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
def norm(a):return math.sqrt(dot(a,a))
def calibration(base,probes,step=.05):
 if len(base)!=3 or len(probes)!=3 or not math.isfinite(step) or step<=0:raise ValueError('invalid probe shape')
 cols=[tuple(v/step for v in sub(p,base)) for p in probes]
 if any(len(c)!=3 or not all(math.isfinite(x) for x in c) for c in cols):raise ValueError('nonfinite probe')
 lengths=[norm(c) for c in cols];scale=sum(lengths)/3
 if not 10<=scale<=500 or any(abs(n-scale)>scale*.002 for n in lengths):raise ValueError('changing or invalid world scale')
 if any(abs(dot(cols[i],cols[j]))>scale*scale*.002 for i,j in ((0,1),(1,2),(0,2))):raise ValueError('nonrigid observed hand basis')
 determinant=dot(cols[0],cross(cols[1],cols[2]))
 if abs(abs(determinant)-scale**3)>scale**3*.006:raise ValueError('singular observed hand basis')
 return dict(columns=cols,determinant=determinant,units_per_metre=scale)
def raw_delta(mapping,world_delta):
 if len(world_delta)!=3 or not all(math.isfinite(x) for x in world_delta):raise ValueError('nonfinite world delta')
 a,b,c=mapping['columns'];d=mapping['determinant']
 return (dot(world_delta,cross(b,c))/d,dot(world_delta,cross(c,a))/d,dot(world_delta,cross(a,b))/d)
