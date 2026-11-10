"""One optional HB park/redraw/resume gesture at the opened magazine.

The caller owns the game, capture and original reload. No engine calls,
inventory writes, old hand grants, input retries or launcher operations.
"""
import json,math,time
from verify_restore_observations import verify


def run(c,*,count_reader,release_left,grip_left,mount_reader,
        clock=time.monotonic,sleep=time.sleep):
    g=c.g;right_rest=dict(g.poses[1]);origin=g.head_rest
    destination=g.directory/'hammerburst-park-restore.json'
    data=dict(game=g.game,checks={},inputs=[],gameplay_setters=False,
              physical_headset_accepted=False,visual_accepted=False)
    def save():destination.write_text(json.dumps(data,indent=2),encoding='utf-8')
    offset=len(c.records('hammerburst_restore'))
    def wait(prefix,start,predicate,label):
        deadline=clock()+8
        while True:
            c.live();rows=c.records(prefix)[start:]
            matches=[row for row in rows if predicate(row)]
            if matches:return matches[-1]
            if clock()>=deadline:raise TimeoutError('No fresh '+label+'; no input retry')
            sleep(.05)
    def relative(x,y,z,**angles):
        return dict(right_rest,x=origin['x']+x,y=origin['y']+y,z=origin['z']+z,**angles)
    def select(wanted):
        chest=relative(.12,-.30,-.31,yaw=0.,pitch=-.15,roll=0.)
        contact=relative(.23,-.115,-.17,yaw=-math.pi/2,pitch=.6,roll=0.)
        withdraw=relative(.30,-.275,-.24,yaw=-math.pi/2,pitch=.6,roll=0.)
        g.pad(1,wait=.2)
        c.reach(chest);c.reach(contact)
        start=len(c.records('native_equipment'))
        data['inputs'].append(dict(action=wanted,slot=3,actual_new_grip=True));save()
        g.rec.event('HB_restore_right_shoulder',wanted=wanted,slot=3)
        g.pad(1,wait=.5,squeeze=1)
        c.reach(withdraw,1);c.reach(chest,1);c.reach(right_rest,1);g.pad(1,wait=.8)
        receipt=wait('native_equipment',start,lambda r:r.get('status')==wanted and r.get('slot')=='3',wanted)
        target=int(receipt.get('target','0'),16)
        if target:assert target==int(data['before']['weapon'],16),'Different shoulder weapon'
        if wanted=='held':
            actual=c.current('HB_restore_actual_redraw')
            assert actual['weapon']==data['before']['weapon'] and actual['cls']=='Weap_LocustAssaultRifle'
            data['actual_redraw']=actual
        data[wanted]=receipt;save()
    try:
        release_left()
        data['before']=count_reader(c,'HB_before_actual_park')
        assert data['before']['game']==g.game
        data['mount_before']=mount_reader();save()
        select('stowed')
        data['parked']=wait('hammerburst_restore',offset,lambda r:r.get('phase')=='parked','parked original transaction')
        assert int(data['parked']['weapon'],16)==int(data['before']['weapon'],16)
        select('held')
        wait('hammerburst_restore',offset,lambda r:r.get('phase')=='display_ready','fresh restored display')
        wait('hammerburst_restore',offset,lambda r:r.get('phase')=='up_baseline','tracked UP after restored display')
        # The remaining original magazine path uses its pre-park local mapping.
        # Require independently re-read same-world geometry before reusing it.
        data['mount_after']=mount_reader()
        before,after=data['mount_before'],data['mount_after']
        assert before['game']==after['game']==g.game,'Different mount-reader game identity'
        assert before['weapon']==after['weapon']==data['before']['weapon']
        assert before['mesh']==after['mesh'],'Different restored mesh component'
        assert after['observed_ns']>before['observed_ns']>=data['before']['observed_ns'],'Mount read not fresh'
        matrices=[m['mesh_component_to_world'] for m in (before,after)]
        assert all(len(m)==16 and all(math.isfinite(float(v)) for v in m) for m in matrices)
        delta=max(abs(float(a)-float(b)) for a,b in zip(*matrices))
        data['mount_max_difference']=delta;save()
        assert delta<=.001,'Restored mount differs; refuse stale magazine planning geometry'
        grip_left() # One real new grip; its actual source/lease is checked by PG.
        resumed=wait('hammerburst_restore',offset,lambda r:r.get('phase')=='resumed','same original transaction resume')
        assert resumed['stage']=='3','Expected opened original magazine after re-hold'
        data['after']=count_reader(c,'HB_after_actual_resume')
        data['restore_rows']=c.records('hammerburst_restore')[offset:]
        data['checks']=verify(data['restore_rows'],data['before'],data['after'],native_reacquired=True)
        data['checks']['fresh_mount_rechecked']=True;save()
        return data
    except BaseException as error:
        data['error']=repr(error);save();raise
