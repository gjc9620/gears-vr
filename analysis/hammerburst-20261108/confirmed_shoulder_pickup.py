"""One real Grip after a current owned Native UP baseline; no old-edge retry."""
import time

def acquire(c,data,*,shoulder=None,clock=time.perf_counter_ns,sleep=time.sleep):
    assert 'pickup_input' not in data,'Pickup input attempt cannot be replayed'
    g=c.g;proof={};data['pickup_input']=proof
    def number(row,key):
        try:return int(row[key])
        except (KeyError,ValueError,TypeError):return -1
    proof['up_sent_ns']=clock();g.pad(1,wait=0,squeeze=0)
    def released_baseline():
        assert g.live(),'Owned runtime ended before shoulder UP baseline'
        rows=c.records('hammerburst_pickup_up')
        for row in reversed(rows):
            stamp=number(row,'admitted_ns');sync=number(row,'sync_ns');now=clock()
            if (sync>=proof['up_sent_ns'] and stamp>=sync and 0<=now-stamp<=100_000_000 and 0<=now-sync<=100_000_000 and
                number(row,'source')==1 and number(row,'right_down')==0 and number(row,'right_pressed')>=0 and
                number(row,'right_pressed')==number(row,'right_released') and row.get('input_status') in ('baseline','quiet')):
                proof['baseline']=row;return True
        return False
    # Observe during the existing finite shoulder gesture. Waiting until its
    # fixed settling tail ends can exhaust the bounded Native UP log budget.
    # The real UP command still precedes every accepted producer receipt.
    if shoulder is not None:c.reach(shoulder,ready=released_baseline)
    deadline=clock()+5_000_000_000
    while clock()<deadline:
        if released_baseline():break
        sleep(.01)
    else:raise TimeoutError('No fresh actual released Native baseline at shoulder; no Grip sent')
    baseline=proof['baseline'];before=len(c.records('native_equipment'))
    gap_before=len(c.records('native_equipment_input_gap'));proof['down_sent_ns']=clock()
    try:
        g.pad(1,wait=0,squeeze=1);deadline=clock()+2_000_000_000
        while clock()<deadline:
            assert g.live(),'Owned runtime ended before actual shoulder hold'
            gaps=c.records('native_equipment_input_gap')[gap_before:]
            rejected=[r for r in gaps if number(r,'right_pressed')==number(baseline,'right_pressed')+1]
            if rejected:
                proof['first_source_gap']=rejected[0]
                raise AssertionError('Original shoulder press rejected by source gap; no input retry')
            for row in c.records('native_equipment')[before:]:
                try:target=int(row.get('target','0'),16)
                except (ValueError,TypeError):continue
                if (row.get('status')=='held' and target==int(c.initial['weapon'],16) and number(row,'event')>0 and
                    number(row,'press')==number(baseline,'right_pressed')+1 and number(row,'frame')>number(baseline,'frame') and
                    number(row,'source_sequence')>number(baseline,'sequence') and number(row,'input_sync_ns')>=proof['down_sent_ns'] and
                    row.get('input_status')=='event' and number(row,'input_control')==6):
                    proof['held']=row;proof['held_observed_ns']=clock();return row
            sleep(.01)
        raise TimeoutError('Single shoulder Grip produced no matching actual held receipt; no input retry')
    except BaseException:
        proof['failure_release_sent_ns']=clock();g.pad(1,wait=0,squeeze=0);raise
