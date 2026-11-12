from pathlib import Path
import struct,json,hashlib,sys,math
folder=Path(sys.argv[1]);report=Path(sys.argv[2]);parts={name:(folder/('cover-first-consumption-'+name+'.bin')).read_bytes() for name in ['source','output','observed','core','stamp']}
assert [len(parts[x]) for x in parts]==[11592,4184,12192,1864,80]
f32=lambda b,o,n:struct.unpack_from('<'+'f'*n,b,o)
source=[f32(parts['source'],7844+i*32,8) for i in range(115)]
planned=[f32(parts['output'],i*32,8) for i in range(115)]
observed=[f32(parts['observed'],728+i*32,8) for i in range(115)]
space=[f32(parts['observed'],4408+i*64,16) for i in range(115)]
parents=[struct.unpack_from('<i',parts['source'],24+i*68+56)[0] for i in range(115)]
component=f32(parts['source'],11524,16);actual_component=f32(parts['observed'],596,16)
def matrix(a):
 x,y,z,w,tx,ty,tz,s=a
 return [s*(1-2*y*y-2*z*z),s*(2*x*y+2*w*z),s*(2*x*z-2*w*y),0,s*(2*x*y-2*w*z),s*(1-2*x*x-2*z*z),s*(2*y*z+2*w*x),0,s*(2*x*z+2*w*y),s*(2*y*z-2*w*x),s*(1-2*x*x-2*y*y),0,tx,ty,tz,1]
def multiply(a,b):return [sum(a[r*4+k]*b[k*4+c] for k in range(4)) for r in range(4) for c in range(4)]
def differences(a,b,eps=1e-6):return [(i,x,y,abs(x-y)) for i,(x,y) in enumerate(zip(a,b)) if abs(x-y)>eps or not math.isfinite(x+y)]
rebuilt=[];space_mismatches=[];norms=[];local_changes=[];source_changes=[]
for i in range(115):
 m=matrix(observed[i]);rebuilt.append(multiply(m,rebuilt[parents[i]]) if i else m)
 delta=differences(space[i],rebuilt[i],2e-5)
 if delta:space_mismatches.append(dict(bone=i,parent=parents[i],largest=sorted(delta,key=lambda x:x[3],reverse=True)[:5]))
 norm=math.sqrt(sum(x*x for x in observed[i][:4]));s=math.sqrt(sum(x*x for x in source[i][:4]))
 if abs(norm-1)>.001:norms.append(dict(bone=i,source_norm=s,observed_norm=norm,source=source[i],observed=observed[i]))
 if observed[i]!=planned[i]:local_changes.append(dict(bone=i,source=source[i],planned=planned[i],observed=observed[i]))
 if planned[i]!=source[i]:source_changes.append(i)
selected=[]
for j,i in enumerate([0,1,55,93,97,8,32]):
 target=struct.unpack_from('<7d',parts['output'],3696+j*56);world=multiply(space[i],actual_component)
 selected.append(dict(bone=i,target_position=list(target[:3]),actual_position=world[12:15],distance=math.dist(target[:3],world[12:15])))
result=dict(bytes={k:len(v) for k,v in parts.items()},sha256={k:hashlib.sha256(v).hexdigest() for k,v in parts.items()},
 component_source=list(component),component_observed=list(actual_component),source_changed_bones=source_changes,nonunit_observed=norms,
 local_changed_after_consumer=local_changes,space_mismatches=space_mismatches,selected_targets=selected,
 observed_qpc=struct.unpack_from('<q',parts['observed'],12160)[0],residuals=struct.unpack_from('<3d',parts['observed'],12168),
 note='Saved unchecked private observation only. Offsets checked against emitted compiled sizes; not a new current pose or write permit.')
report.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(local_changed=len(local_changes),space_mismatches=len(space_mismatches),nonunit_bones=[x['bone'] for x in norms],selected=selected),indent=2))
