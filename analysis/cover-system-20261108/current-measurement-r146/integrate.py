"""Reuse the existing current-Tick measurement; preserve the rest of r143."""
from pathlib import Path
import subprocess
R=Path(__file__).resolve().parents[3]
def once(s,old,new):
    assert s.count(old)==1,old
    return s.replace(old,new)
def integrate(source):
    name='tools/native-capture/body_goal_view_adapter.h'
    existing=subprocess.check_output(['git','show','2f042279:'+name],cwd=R,text=True,encoding='utf-8')
    begin=existing.index('// Math over an independently admitted source.');end=existing.index('inline Status place_cover(',begin)
    block=existing[begin:end]
    path=source/name;s=path.read_text(encoding='utf-8-sig')
    a=s.index('inline Status measure_cover_pose(');b=s.index('inline Status place_cover(',a)
    path.write_text(s[:a]+block+s[b:],encoding='utf-8')
    path=source/'tools/native-capture/body_c0_goal_policy_live.inl';s=path.read_text(encoding='utf-8-sig')
    old='measurement_status=a::measure_cover_pose(s.context.source,s.model,s.source,marker,policy,s.workspace,candidate);'
    new='measurement_status=body_tick_source_live::enabled?\n                a::measure_cover_pose(s.context.body_source,s.model,s.source,marker,policy,s.workspace,candidate):\n                a::measure_cover_pose(s.context.source,s.model,s.source,marker,policy,s.workspace,candidate);'
    s=once(s,old,new)
    old='slot.row.core=gears_cover_measurement_core::observe(s.context.source,policy->now_ns,a::maximum_age_ns,policy->allow_sim);'
    new='slot.row.core=body_tick_source_live::enabled?\n            gears_cover_measurement_core::observe(s.context.body_source,policy->now_ns,a::maximum_age_ns,policy->allow_sim):\n            gears_cover_measurement_core::observe(s.context.source,policy->now_ns,a::maximum_age_ns,policy->allow_sim);'
    path.write_text(once(s,old,new),encoding='utf-8')
