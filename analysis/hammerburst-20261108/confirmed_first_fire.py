"""One physical test press, bracketed by actual owned PG consumption receipts."""
import time


def first_fire(c,data,*,clock=time.perf_counter_ns,sleep=time.sleep,expected_shots=6,deadline_ns=None):
    g=c.g
    assert 1<=expected_shots<=6
    receipt=data.setdefault('first_fire_input',{})
    rejection_offsets={p:len(c.records(p)) for p in ('native_hammerburst_continuation_rejected','native_hammerburst_release_rejected')}
    def check_rejection():
        for prefix,offset in rejection_offsets.items():
            assert not any(0<=int(row.get('count',-1))<expected_shots for row in c.records(prefix)[offset:]),'Original burst explicitly rejected; no input retry'
    def number(row,key):
        try:return int(row[key])
        except (KeyError,ValueError,TypeError):return -1
    def candidate(row,sent,down):
        stamp=number(row,'admitted_ns');now=clock()
        try:value=float(row.get('trigger_value','nan'))
        except (ValueError,TypeError):return False
        # A recent log receipt can still describe a nearly expired producer
        # sample. Choose a stricter UP-only test setup window before the one
        # press; this does not change the production100ms/20ms/250ms gates.
        return (number(row,'input_sync_ns')>=sent and stamp>=number(row,'input_sync_ns') and 0<=now-stamp<=100_000_000 and
                all(number(row,key)==1 for key in ('current','source_status','trigger_active','active_at_admission','game_known')) and
                (value>.35 if down else value<=.35 and number(row,'status') in (1,2) and
                 number(row,'owned')==0 and number(row,'game_r2')==0 and
                 number(row,'preparation_budget')==1 and
                 0<=now-number(row,'input_sync_ns')<=60_000_000))
    def consumed(sent,down,seconds):
        deadline=clock()+int(seconds*1e9)
        if deadline_ns is not None:deadline=min(deadline,deadline_ns)
        while clock()<deadline:
            live_started=clock()
            assert g.live(),'Owned runtime ended before input consumption'
            read_started=clock()
            rows=c.records('hammerburst_input_consumed')
            observed=clock()
            # Bounded test-reader diagnostics only. Keep original producer/log
            # times and the unchanged acceptance predicate/deadline.
            wait=receipt.setdefault('down_wait' if down else 'baseline_wait',
                                    dict(samples=[],distinct_rows=0))
            newest=next((r for r in reversed(rows) if number(r,'input_sync_ns')>=sent),None)
            if newest is not None and (not wait['distinct_rows'] or newest.get('sequence')!=wait.get('last_sequence')):
                sample=dict(row=dict(newest),observed_ns=observed,
                            source_age_ns=observed-number(newest,'input_sync_ns'),
                            receipt_age_ns=observed-number(newest,'admitted_ns'),
                            liveness_ns=read_started-live_started,read_ns=observed-read_started)
                wait['last_sequence']=newest.get('sequence');wait['distinct_rows']+=1
                wait['samples'].append(sample);wait['samples']=wait['samples'][-8:]
                if 'minimum_source_age' not in wait or sample['source_age_ns']<wait['minimum_source_age']['source_age_ns']:
                    wait['minimum_source_age']=sample
            for row in reversed(rows):
                if candidate(row,sent,down):
                    if down and number(row,'status')==8:
                        receipt['first_down_rejection']=row
                        raise AssertionError('Original DOWN blocked by Input; no press retry')
                    return row
            sleep(.01)
        raise TimeoutError('No fresh actual '+('DOWN' if down else 'UP baseline')+' consumption receipt')
    receipt['baseline_sent_ns']=clock();g.pad(1,wait=0)
    receipt['baseline']=consumed(receipt['baseline_sent_ns'],False,5)
    before=len(c.records('native_lancer_shot_trajectory'))
    receipt['down_sent_ns']=clock()
    try:
        g.pad(1,wait=0,trigger=1)
        receipt['down']=consumed(receipt['down_sent_ns'],True,2)
        deadline=clock()+2_000_000_000
        if deadline_ns is not None:deadline=min(deadline,deadline_ns)
        while clock()<deadline:
            assert g.live(),'Owned runtime ended before original first shot'
            check_rejection()
            first=c.records('native_lancer_shot_trajectory')[before:]
            if first:
                assert all(x.get('trace_valid')=='1' for x in first),'Invalid original first HB trajectory'
                receipt['first_shot_observed_ns']=clock();receipt['shots_at_release']=len(first)
                break
            sleep(.01)
        else:raise TimeoutError('Consumed DOWN did not produce an original first HB trajectory')
    finally:
        data['shots']=c.records('native_lancer_shot_trajectory')[before:]
        receipt['release_sent_ns']=clock();g.pad(1,wait=0)
    remaining=.7 if deadline_ns is None else max(0,min(.7,(deadline_ns-clock())/1e9))
    sleep(remaining) # Drain this original burst inside the caller's original deadline.
    if deadline_ns is not None:assert clock()<=deadline_ns,'Original whole-flow deadline expired'
    shots=c.records('native_lancer_shot_trajectory')[before:];data['shots']=shots
    completion_deadline=deadline_ns if deadline_ns is not None else receipt['release_sent_ns']+1_500_000_000
    while 0<len(shots)<expected_shots:
        check_rejection();assert clock()<completion_deadline,'Same original burst did not complete before original observation deadline'
        sleep(min(.1,(completion_deadline-clock())/1e9));assert g.live()
        shots=c.records('native_lancer_shot_trajectory')[before:];data['shots']=shots
    assert shots and all(x.get('trace_valid')=='1' for x in shots),'No authoritative HB trajectory after consumed DOWN'
    assert len(shots)==expected_shots,('Original HB burst exported unexpected shot count',len(shots),expected_shots)
    data['checks']['actual_fire']=True
