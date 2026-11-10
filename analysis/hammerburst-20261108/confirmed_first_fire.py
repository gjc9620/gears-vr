"""One physical test press, bracketed by actual owned PG consumption receipts."""
import time


def first_fire(c,data,*,clock=time.perf_counter_ns,sleep=time.sleep):
    g=c.g
    receipt=data.setdefault('first_fire_input',{})
    def number(row,key):
        try:return int(row[key])
        except (KeyError,ValueError,TypeError):return -1
    def candidate(row,sent,down):
        stamp=number(row,'admitted_ns');now=clock()
        try:value=float(row.get('trigger_value','nan'))
        except (ValueError,TypeError):return False
        return (number(row,'input_sync_ns')>=sent and stamp>=sent and 0<=now-stamp<=100_000_000 and
                all(number(row,key)==1 for key in ('current','source_status','trigger_active','active_at_admission','game_known')) and
                (value>.35 if down else value<=.35 and number(row,'status') in (1,2) and
                 number(row,'owned')==0 and number(row,'game_r2')==0))
    def consumed(sent,down,seconds):
        deadline=clock()+int(seconds*1e9)
        while clock()<deadline:
            assert g.live(),'Owned runtime ended before input consumption'
            rows=c.records('hammerburst_input_consumed')
            for row in reversed(rows):
                if candidate(row,sent,down):return row
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
        while clock()<deadline:
            assert g.live(),'Owned runtime ended before original first shot'
            first=c.records('native_lancer_shot_trajectory')[before:]
            if first:
                assert all(x.get('trace_valid')=='1' for x in first),'Invalid original first HB trajectory'
                receipt['first_shot_observed_ns']=clock();receipt['shots_at_release']=len(first)
                break
            sleep(.01)
        else:raise TimeoutError('Consumed DOWN did not produce an original first HB trajectory')
    finally:
        receipt['release_sent_ns']=clock();g.pad(1,wait=0)
    sleep(.7) # Drain the existing original finite burst; send no second press.
    shots=c.records('native_lancer_shot_trajectory')[before:];data['shots']=shots
    assert shots and all(x.get('trace_valid')=='1' for x in shots),'No authoritative HB trajectory after consumed DOWN'
    assert len(shots)==6,('Original HB burst must export exactly six valid shots',len(shots))
    data['checks']['actual_fire']=True
