"""One owned CT2 left/right Grip chain; imports input only from its owner."""
import math,time
from grip_mapping import calibration,raw_delta,sub,norm

def run(g,observer,report,save,current):
 import launcher
 report['grab_authorized']=True
 report['grip_scope']='One CT2 left/right original-L2 Grip acquire/pull/release-hold/regrab/reverse; no hand-fitting or collision acceptance'
 rows=report['grip_observation']=[];last={}
 def vector(value):
  out=tuple(float(x) for x in value.split(','));assert len(out)==3 and all(math.isfinite(x) for x in out);return out
 def virtual(f):return tuple(float(x) for x in f['cover_input_virtual'].split(','))
 def observe(label,predicate=lambda f:True):
  nonlocal last
  deadline=time.monotonic()+8;old_seq=int(last.get('cover_input_sequence','0'));old_body=int(last.get('body_goal_cover_consumed','0'));old_view=int(last.get('body_goal_cover_view_used','0'))
  while True:
   assert g.live();session=observer.verify();f=launcher.current_native_status(observer.backend,session)
   fresh=int(f.get('cover_input_sequence','0'))>old_seq and int(f.get('body_goal_cover_consumed','0'))>old_body and int(f.get('body_goal_cover_view_used','0'))>old_view
   if fresh and f.get('cover_input_hand_world_valid')=='1' and predicate(f):break
   if time.monotonic()>=deadline:
    report['grip_failure']=dict(label=label,native={k:v for k,v in f.items() if k.startswith(('cover_input_','cover_l2_','body_goal_cover_','body_native_','cover_pending_'))});save()
    raise RuntimeError('Current cover Grip phase not reached: '+label)
   time.sleep(.1)
  assert f['cover_input_c0_owner']=='1' and f['cover_body_live_published']=='0'
  assert norm(vector(f['cover_input_physical']))<.005,'Head must remain neutral during hand-only interaction'
  sampled=current();assert sampled['cover_state_raw']['CoverType']==2
  row=dict(label=label,at_ns=time.perf_counter_ns(),native={k:v for k,v in f.items() if k.startswith(('cover_input_','cover_l2_','body_goal_cover_','body_cover_pose_','cover_full_goal_','cover_pending_'))})
  rows.append(row);save();last=f;return f
 def move(pose,held=False):
  if not held:g.hand_move(0,pose);return
  start=g.poses[0];g.rec.event('cover_grip_held_motion',hand=0,start=start,end=pose)
  for _,value,levels,interval in g.gesture(start,pose,{'trigger':0,'squeeze':1},{'trigger':0,'squeeze':1},profile=g.profile_for(start,pose)):
   g.put(0,value,**levels);time.sleep(interval)
  g.poses[0]=pose;time.sleep(.35)
 def translated(pose,delta):return dict(pose,**{k:pose[k]+delta[i] for i,k in enumerate(('x','y','z'))})
 def offset_is(f,right):
  v=virtual(f)
  return abs(v[0]-right)<.02 and abs(v[1])<.01 and f.get('cover_input_offset_certified')=='1' and abs(float(f['cover_full_goal_right'])-right)<.025
 try:
  g.pad(0,wait=.35,squeeze=0);base_pose=dict(g.poses[0]);base=observe('cover_grip_mapping_base',lambda f:offset_is(f,0))
  base_world=vector(base['cover_input_hand_world']);probes=[]
  for axis in ('x','y','z'):
   pose=dict(base_pose);pose[axis]+=.05;move(pose)
   f=observe('cover_grip_mapping_'+axis,lambda f:offset_is(f,0));probes.append(vector(f['cover_input_hand_world']))
   move(base_pose);f=observe('cover_grip_mapping_'+axis+'_return',lambda f:offset_is(f,0))
   assert norm(sub(vector(f['cover_input_hand_world']),base_world))<max(norm(sub(probes[-1],base_world))*.4,1e-4),'Neutral mapping changed while probing'
  mapping=calibration(base_world,probes);units=mapping['units_per_metre'];report['grip_mapping']=mapping;save()
  points=[]
  for i in (0,1):
   values=last['cover_l2_reference_point_'+str(i)].split(',');assert values[0]=='1';points.append(tuple(float(x) for x in values[1:]))
  wall_right=sub(points[1],points[0]);length=norm(wall_right);assert length>.1*units and abs(wall_right[2])<.02*units
  wall_right=tuple(x/length for x in wall_right)
  for edge,(point,sign) in enumerate(zip(points,(-1,1)),1):
   delta=raw_delta(mapping,sub(point,base_world));assert norm(delta)<3
   contact=translated(base_pose,delta);move(contact)
   def at_edge(f):
    info=tuple(int(x) for x in f['cover_input_l2_edge'].split(','))
    return info==(edge,edge,1,1) and norm(sub(vector(f['cover_input_hand_world']),point))<.08*units and offset_is(f,0)
   observe(f'cover_grip_{edge}_contact',at_edge)
   g.pad(0,wait=.35,squeeze=1)
   def gripping(f):return f.get('cover_input_interaction_phase')=='5' and f.get('cover_input_interaction_owns')=='1' and int(f.get('cover_input_interaction_edge','0'))==edge
   acquired=observe(f'cover_grip_{edge}_acquired',gripping);request=int(acquired['cover_input_interaction_request']);assert request>0
   pulled=translated(contact,raw_delta(mapping,tuple(-sign*.12*units*x for x in wall_right)))
   move(pulled,True);observe(f'cover_grip_{edge}_pulled',lambda f:gripping(f) and offset_is(f,sign*.12))
   g.pad(0,wait=.35,squeeze=0)
   observe(f'cover_grip_{edge}_released_hold',lambda f:f.get('cover_input_interaction_phase')=='3' and f.get('cover_input_interaction_owns')=='0' and offset_is(f,sign*.12))
   time.sleep(.6);observe(f'cover_grip_{edge}_hold_persists',lambda f:f.get('cover_input_interaction_owns')=='0' and offset_is(f,sign*.12))
   g.pad(0,wait=.35,squeeze=1)
   observe(f'cover_grip_{edge}_regrab',lambda f:gripping(f) and int(f['cover_input_interaction_request'])>request and offset_is(f,sign*.12))
   move(contact,True);observe(f'cover_grip_{edge}_reversed',lambda f:gripping(f) and offset_is(f,0))
   g.pad(0,wait=.35,squeeze=0);observe(f'cover_grip_{edge}_released_neutral',lambda f:f.get('cover_input_interaction_owns')=='0' and offset_is(f,0))
   move(base_pose);observe(f'cover_grip_{edge}_hand_returns',lambda f:offset_is(f,0))
  report['grip_sequence_passed']=True;report['grip_visual_accepted']=False;report['grab_pose_hand_fit_accepted']=False;save()
 finally:
  if g.live():g.pad(0,wait=.25,squeeze=0) # Normal release on success or bounded failure.
