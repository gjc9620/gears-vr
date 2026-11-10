"""Owned HB original-return or explicit replacement check with simulated input.

No launcher, inventory setter, GiveTo, animation call or production grant.
The caller owns startup/recording/shutdown. Not yet game-validated.
"""
import json, math, time, sys
import numpy as np
from human_pose_path import gesture, quaternion, ease
from reload_original_inventory_chain import matrix, quat
from hammerburst_count_snapshot import sample as counts

def verify_empty_original_begin(begin_rows,transition):
    serial=int(transition.get('serial','0'))
    assert serial>0 and transition.get('from')=='1' and transition.get('to')=='2'
    assert transition.get('loaded')=='0' and transition.get('reserve')=='78'
    matching=[r for r in begin_rows if int(r.get('serial','0'))==serial]
    assert len(matching)==1,'Missing or ambiguous fresh original reload acceptance'
    accepted=matching[0]
    assert int(accepted.get('accepted_ms','0'))>0 and float(accepted.get('game_start','0'))>0
    return dict(transition=transition,original_acceptance=accepted)

def observe_return_burst(c, after, shot_start, deadline, rejection_offsets, data, save,
                         *, clock=time.monotonic, sleep=time.sleep):
    """Observe the same already pressed burst; no input or deadline renewal."""
    expected=min(6,int(after['loaded']))
    assert expected>0,'No loaded ammunition for authoritative return-fire'
    previous=after['loaded']
    while True:
        c.live()
        shots=c.records('native_lancer_shot_trajectory')[shot_start:]
        assert shots and len(shots)<=expected and all(
            r.get('trace_valid')=='1' and r.get('trace_source_consistent')=='1' and
            r.get('write_source_consistent')=='1' for r in shots),'Invalid authoritative return-fire'
        current=counts(c,'hb_original_after_return_fire')
        data['shots']=shots;data['after_return_fire']=current
        data.setdefault('return_fire_observations',[]).append(dict(counts=current,exported_rays=len(shots)))
        save()
        assert all(after[k]==current[k] for k in ('capacity','reserve','infinite_reserve'))
        spent=after['loaded']-current['loaded']
        assert current['loaded']<=previous and 0<=spent<=expected,'Unexpected return-fire inventory progress'
        previous=current['loaded']
        assert clock()<=deadline,'Original return burst observation exceeded existing reload deadline'
        if spent==expected and len(shots)==expected:return shots,current
        for prefix,offset in rejection_offsets.items():
            assert not any(0<int(r.get('count',0))<6 for r in c.records(prefix)[offset:]),\
                'Original return burst explicitly rejected; stop without retry'
        remaining=deadline-clock()
        assert remaining>0,'Original return burst did not complete within existing reload deadline'
        sleep(min(.1,remaining))
        assert clock()<=deadline,'Original return burst exceeded existing reload deadline'

def run(c, *, empty_supply_only=False, require_empty_original_begin=False):
    g=c.g; saved=dict(g.poses[0]); levels=dict(trigger=0,squeeze=0)
    data=dict(game=g.game,checks={},phases=[],observations=[],gameplay_setters=False,
              visual_accepted=False,physical_headset_accepted=False)
    replacement='--hammerburst-replacement' in sys.argv
    data['replacement_mode']=replacement
    destination=g.directory/('hammerburst-replacement.json' if replacement else 'hammerburst-original-return.json')
    def save():destination.write_text(json.dumps(data,indent=2),encoding='utf-8')
    def phase(label):
        data['phases'].append(dict(label=label,ns=time.perf_counter_ns()))
        g.rec.event('hammerburst_reload_phase',phase=label);save()
    def wait(prefix,start=0,condition=lambda row:True,seconds=8,expected='matching receipt'):
        deadline=time.monotonic()+seconds
        while time.monotonic()<deadline:
            c.live(); rows=[r for r in c.records(prefix)[start:] if condition(r)]
            if rows:return rows[-1]
            time.sleep(.1)
        data.setdefault('wait_failures',[]).append(dict(prefix=prefix,start=start,expected=expected,
                                                      latest=c.records(prefix)[-8:]))
        save()
        raise AssertionError('No fresh '+prefix+' '+expected+' in '+str(seconds)+' seconds')
    def command(end,down=True,delay=.15):
        nonlocal levels
        wanted=dict(trigger=0,squeeze=int(down))
        for _,pose,buttons,interval in gesture(dict(g.poses[0]),end,levels,wanted,profile='contact'):
            c.live();g.put(0,pose,**buttons);time.sleep(interval)
        levels=wanted;g.poses[0]=dict(end);time.sleep(delay)
    def read(helper,label):
        name=f'hb-original-{g.game["pid"]}-{label}.json';g.call(helper,name)
        value=json.loads((g.OUT/name).read_text(encoding='utf-8'))
        assert value['game']==g.game;data['observations'].append(dict(label=label,value=value));save();return value
    try:
        phase('original_return_prepare')
        before=counts(c,'hb_original_before');data['before']=before;save()
        if empty_supply_only:
            assert replacement and before['loaded']==0 and before['reserve']==0 and not before['infinite_reserve']
        geometry=wait('hammerburst_reload_geometry')
        world=read('read_world_grips.py','world');gun=read('read_aim_metrics.py','gun')
        assert world['weapon']==gun['weapon']==c.current('hb_original_current')['weapon']
        right=np.array(world['world'][1]['xyz']);P=np.array([[0,1,0],[0,0,1],[-1,0,0]])
        rows=[r for r in c.records('native_lancer_follow') if r.get('accepted')=='1' and
              r.get('mesh_read')=='1' and np.linalg.norm(np.fromstring(r['grip_world'],sep=',')-right)<.5]
        assert rows,'No current matching production mount; no fallback to an unrelated gun basis'
        plan=rows[-1];units=float(geometry['units']);assert 10<=units<=500
        observed=np.array(gun['mesh_component_to_world']).reshape(4,4).T
        scale=np.linalg.norm(observed[:3,:3],axis=0);assert np.ptp(scale)<.001
        model=matrix(np.fromstring(plan['target_rot'],sep=',')*math.pi/32768)*scale
        origin=np.fromstring(plan['target_world'],sep=',')
        mapping=matrix(world['world'][1]['rotation'])@P.T@quat(world['raw'][1]['quat']).T
        raw_right=np.array(world['raw'][1]['xyz'])
        def target(local):
            raw=raw_right+mapping.T@(origin+model@(np.asarray(local)*units)-right)/units
            return dict(g.poses[0],**dict(zip(('x','y','z'),map(float,raw))))
        vector=lambda row,key:np.fromstring(row[key],sep=',')
        pivot=vector(geometry,'pivot');axis=vector(geometry,'axis');contact=vector(geometry,'contact')
        def latch_target(angle):
            v=contact-pivot
            point=pivot+v*math.cos(angle)+np.cross(axis,v)*math.sin(angle)+axis*np.dot(axis,v)*(1-math.cos(angle))
            return target(point)
        def latch(angle,down=True):command(latch_target(angle),down)
        def sweep(start,end):
            # One continuous, minimum-jerk physical arc. The former nineteen
            # stop/settle/restart gestures took over five seconds. Do not
            # change the product's100ms observation or input deadlines.
            trace=dict(start=start,end=end,samples=[],input_only=True)
            data.setdefault('latch_sweeps',[]).append(trace)
            steps=46;duration=1.6
            try:
                for i in range(1,steps+1):
                    angle=start+(end-start)*ease(i/steps);pose=latch_target(angle)
                    c.live();g.put(0,pose,trigger=0,squeeze=1);g.poses[0]=dict(pose)
                    trace['samples'].append(dict(ns=time.perf_counter_ns(),angle=angle,
                                                xyz=[pose[k] for k in ('x','y','z')]))
                    time.sleep(duration/steps)
            finally:save()
        def sweep_with_one_regrab(start,end,stage):
            first=len(c.records('hammerburst_latch_rejected'));sweep(start,end)
            rejected=c.records('hammerburst_latch_rejected')[first:]
            if not rejected:return
            if require_empty_original_begin:
                data['first_latch_rejection']=rejected;save()
                raise AssertionError('First latch rejection stops ordinary empty reload; no recovery input retry')
            # Preserve the first actual failure. Only an explicit release
            # and fresh physical regrab at the retained angle can recover.
            assert len(rejected)==1 and rejected[0].get('reason')=='5' and rejected[0].get('stage')==str(stage),rejected
            actual=float(rejected[0]['angle']);assert 0<=actual<=maximum
            data.setdefault('latch_regrabs',[]).append(dict(rejection=rejected[0],at_actual_angle=actual,
                                                          normal_uninterrupted_pass=False))
            phase('actual_release_and_regrab_latch_stage_'+str(stage))
            latch(actual,False);latch(actual,True)
            sweep(actual,end)
            assert len(c.records('hammerburst_latch_rejected'))==first+1,'Second latch rejection; no retry loop'
        data['input_planning']=dict(production=plan,geometry=geometry,actual_mesh=observed.tolist(),
                                   planning_is_not_outcome_evidence=True);save()
        transitions=len(c.records('hammerburst_reload_transition'))
        begin_rows=len(c.records('hammerburst_reload_begin'))
        phase('open_original_latch');latch(0,False);latch(0)
        if require_empty_original_begin:
            begun=wait('hammerburst_reload_transition',transitions,lambda r:r.get('from')=='1' and r.get('to')=='2',expected='1->2 (fresh empty original reload acquisition)')
            data['empty_original_begin']=verify_empty_original_begin(c.records('hammerburst_reload_begin')[begin_rows:],begun);save()
        wait('hammerburst_reload_transition',transitions,lambda r:r.get('to')=='2',expected='to=2 (opening)')
        maximum=float(wait('hammerburst_reload_latch')['open_angle'])
        assert .1<maximum<math.pi
        phase('continuous_latch_opening');sweep_with_one_regrab(0,maximum,2)
        wait('hammerburst_reload_transition',transitions,lambda r:r.get('to')=='4',expected='to=4 (original magazine)')
        feed=wait('hammerburst_reload_feed');port=vector(feed,'port');outward=vector(feed,'outward')
        offset=vector(feed,'item_offset')
        # All inputs since the authenticated read changed position only.
        # Use its actual UE grip basis. The former XR expression multiplied
        # the axis conversion in the wrong order and missed the right basis
        # conversion, leaving the real magazine about2cm short of its seat.
        before_q=np.asarray(world['raw'][0]['quat']);now_q=np.asarray(quaternion(g.poses[0]))
        assert abs(float(before_q@now_q))>1-1e-6,'Grip orientation changed since authenticated planning read'
        left_basis=np.linalg.solve(model/scale,matrix(world['world'][0]['rotation']))
        data['magazine_input_planning']=dict(left_in_model=left_basis.tolist(),
            observed_world_grip_rotation=world['world'][0]['rotation'],raw_orientation_unchanged=True,
            actual_seating_must_be_observed=True);save()
        def magazine(depth):command(target(port+outward*depth-left_basis@offset))
        phase('extract_original_for_replacement' if replacement else 'extract_then_return_same_original_without_release')
        magazine(.075)
        if replacement:
            # Only an actual continuous extraction plus real release opens the
            # existing shoulder route. No inventory or transaction setter.
            command(dict(g.poses[0]),False)
            wait('hammerburst_reload_transition',transitions,lambda r:r.get('to')=='5')
            phase('left_shoulder_actual_replacement')
            for point in ((-.10,-.28,-.29),(-.23,-.115,-.17)):
                shoulder=dict(g.poses[0],**{k:g.head_rest[k]+v for k,v in zip(('x','y','z'),point)})
                command(shoulder,False)
            empty_start=len(c.records('hammerburst_empty_supply'))
            supply_start=len(c.records('hammerburst_supply'))
            command(dict(g.poses[0]),True)
            if empty_supply_only:
                row=wait('hammerburst_empty_supply',empty_start,expected='fresh zero-total refusal')
                assert int(row['token'])>0 and row.get('sound')=='1' and row.get('replacement_created')=='0'
                time.sleep(.5);c.live()
                refusal=c.records('hammerburst_empty_supply')[empty_start:]
                assert len(refusal)==1,'Held same Grip repeated empty feedback'
                assert not c.records('hammerburst_supply')[supply_start:],'Zero-total supply created a magazine'
                assert not [r for r in c.records('hammerburst_reload_transition')[transitions:] if r.get('to')=='6']
                after=counts(c,'hb_zero_total_after');data['after']=after
                assert all(before[k]==after[k] for k in ('loaded','used','reserve','capacity','infinite_reserve'))
                data['empty_supply_receipt']=row
                data['checks']['zero_total_refuses_once_without_inventory_change']=True
                save();return
            wait('hammerburst_reload_transition',transitions,lambda r:r.get('to')=='6')
            magazine(.075)
        for depth in np.linspace(.075,0,18)[1:]:magazine(float(depth))
        wait('hammerburst_reload_transition',transitions,lambda r:r.get('to')=='7',expected='to=7 (closing after actual seating)')
        phase('close_original_latch');latch(maximum,False);latch(maximum)
        completion_start=len(c.records('hammerburst_reload_complete'))
        inspection_start=len(c.records('hammerburst_inspection_complete'))
        phase('continuous_latch_closing');sweep_with_one_regrab(maximum,0,7)
        deadline=time.monotonic()+8
        while time.monotonic()<deadline:
            c.live()
            if c.records('hammerburst_reload_complete')[completion_start:] or c.records('hammerburst_inspection_complete')[inspection_start:]:break
            time.sleep(.1)
        else:raise AssertionError('No original-magazine completion receipt')
        after=counts(c,'hb_original_after');data['after']=after
        if replacement:
            assert not before['infinite_reserve'],'Finite supply test requires actual finite reserve'
            added=min(int(before['used']),int(before['reserve']))
            data['checks']['replacement_fills_and_conserves']=(after['capacity']==before['capacity'] and
                after['used']==before['used']-added and after['reserve']==before['reserve']-added and
                after['infinite_reserve']==before['infinite_reserve'])
            save();assert data['checks']['replacement_fills_and_conserves'],('Replacement inventory mismatch',before,after)
        else:
            data['checks']['original_rounds_preserved']=all(before[k]==after[k] for k in ('capacity','used','reserve','infinite_reserve','active_reload_bonus_shots'))
            save();assert data['checks']['original_rounds_preserved'],('Original return changed inventory',before,after)
        phase('original_return_authoritative_fire');command(saved,False)
        shot_start=len(c.records('native_lancer_shot_trajectory'))
        rejection_offsets={p:len(c.records(p)) for p in (
            'native_hammerburst_continuation_rejected','native_hammerburst_release_rejected',
            'native_hammerburst_owned_timer_blocked')}
        g.pad(1,wait=.35,trigger=1);g.pad(1,wait=.6)
        # Reuse the existing closure/completion deadline, not a new wait budget.
        shots,after_fire=observe_return_burst(c,after,shot_start,deadline,rejection_offsets,data,save)
        data['checks']['return_fire_actual_ammo_matches_traces']=(
            after['loaded']-after_fire['loaded']==len(shots) and
            all(after[k]==after_fire[k] for k in ('capacity','reserve','infinite_reserve')))
        save();assert data['checks']['return_fire_actual_ammo_matches_traces'],('Return-fire count mismatch',after,after_fire,shots)
        data['checks']['actual_return_fire']=True;save()
    except BaseException as error:data['error']=repr(error);raise
    finally:
        if g.live():command(saved,False)
        save()
