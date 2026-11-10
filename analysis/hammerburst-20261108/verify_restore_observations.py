"""Cross-check transition receipts against independent typed game snapshots.

This is one verifier for the switch/resume test, not whole-weapon acceptance.
"""
import math


def verify(rows,before,after,*,native_reacquired):
    assert rows,'Missing actual restore receipts'
    assert before['game']==after['game'],'Different game identity'
    assert before['weapon']==after['weapon'],'Different actual weapon'
    for field in ('capacity','loaded','reserve','infinite_reserve','active_reload_bonus_shots'):
        assert before[field]==after[field],('Inventory changed during park/restore',field)
    assert math.isfinite(before['game_time']) and math.isfinite(after['game_time']) and after['game_time']>=before['game_time'],'Actual game clock reset'
    expected=['parked','rebound','display_ready','up_baseline','intent']
    if native_reacquired:expected.append('native_reacquired')
    expected.append('resumed')
    assert [r['phase'] for r in rows]==expected,'Missing, duplicate, or reordered restore phase'
    first=rows[0]
    assert int(first['weapon'],16)==int(before['weapon'],16),'Receipt weapon mismatch'
    assert int(first['serial'])>0,'Requires an accepted original reload'
    for row in rows:
        for field in ('weapon','world','serial','old_item','accepted_begin_ms','perfect_end_ms','reward_start'):
            assert row[field]==first[field],('Original ownership/timing changed',field)
        assert int(row['uncertain'])==0,'Uncertain dispatch is not an accepted resume'
        assert int(row['loaded'])==before['loaded'] and int(row['reserve'])==before['reserve']
        assert int(row['capacity'])==before['capacity']
        assert math.isfinite(float(row['angle'])) and abs(float(row['angle'])-float(first['angle']))<=1e-6,'Restore advanced the latch'
    assert int(first['accepted_begin_ms'])>0 and math.isfinite(float(first['reward_start']))
    rebound=rows[1]
    assert int(rebound['hold'])>int(first['hold']),'No fresh actual re-hold'
    assert all(row['hold']==rebound['hold'] for row in rows[1:]),'Re-hold changed during restore'
    up=rows[3];intent=rows[4]
    assert up['left_known']=='1' and up['left_tracked']=='1' and up['left_down']=='0','Missing known tracked UP'
    assert intent['left_known']=='1' and intent['left_tracked']=='1' and intent['left_down']=='1'
    assert int(intent['frame'])>int(up['frame']) and int(intent['press'])>int(first['old_item']),'No later fresh grip'
    assert int(intent['source_sync_ns'])>int(up['source_sync_ns'])>0,'Source did not advance'
    assert rows[-1]['native_owned']==('1' if native_reacquired else '0')
    assert int(rows[-1]['stage']) in ((2,3,5) if native_reacquired else (7,)),'Not restored to a live mechanical phase'
    assert first['feed']==('0' if native_reacquired else '1'),'Wrong pre/post-insertion branch'
    return dict(park_resume_counts_preserved=True,original_clock_preserved=True,fresh_up_then_grant_observed=True,
                native_reacquired=native_reacquired,gameplay_scope='park/restore only; completion/return fire/fit require separate evidence')
