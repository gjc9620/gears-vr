"""Observe the exact retained rows used by the startup gate; no policy changes."""
import json
from pathlib import Path
import time


def observation(rows, panel_ready):
    tail=rows[-20:]
    def flags(row,key):
        try:return int(row.get(key) or 0)&15==15
        except (ValueError,TypeError):return False
    tests={
        'published':lambda row:row.get('stereo_published')=='1',
        'focused':lambda row:row.get('session_state')=='5',
        'render':lambda row:row.get('should_render')=='1',
        'head_valid':lambda row:flags(row,'head_flags'),
        'views_valid':lambda row:flags(row,'view_flags'),
        'source_zero':lambda row:row.get('stereo_source')=='0',
    }
    counts={name:sum(bool(test(row)) for row in tail) for name,test in tests.items()}
    checks={name:len(tail)==20 and count==20 for name,count in counts.items()}
    checks.update(minimum_rows=len(rows)>=30,panel_ready=bool(panel_ready))
    return dict(retained_rows=len(rows),window_rows=len(tail),required_rows=30,
                required_window_rows=20,checks=checks,passing_rows=counts,
                last_session_state=rows[-1].get('session_state','unknown') if rows else 'unknown',
                ready=all(checks.values()))


class Journal:
    def __init__(self,path,clock=time.perf_counter_ns):
        self.path=Path(path);self.clock=clock;self.started=clock();self.next=0
        self.records=0;self.error=None;self.last=None;self.final_written=False

    def observe(self,rows,panel_ready,*,final=False,outcome='waiting'):
        value=observation(rows,panel_ready);self.last=value
        if self.final_written:return value
        now=self.clock()
        if not final and (now<self.next or self.records>=180):return value
        self.next=now+1_000_000_000
        if final:self.final_written=True
        row=dict(xr_preflight_diagnostics=True,schema=1,observed_ns=now,
                 elapsed_ms=(now-self.started)/1_000_000,final=final,
                 phase=outcome,scope='retained_rows_used_by_gate',**value)
        try:
            with self.path.open('a',encoding='utf-8') as stream:
                stream.write(json.dumps(row,separators=(',',':'))+'\n')
            self.records+=1
        except OSError as error:self.error=type(error).__name__
        return value
