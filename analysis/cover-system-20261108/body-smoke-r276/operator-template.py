"""One ordinary cover entry and a short view/body observation, no queries.

Reuse the owned VR input/menu/navigation path. Source transformations are
checked during offline preparation; no observer, setter or writer is added.
"""
from pathlib import Path
import sys, types

assert sys.argv.count('--cover-eye-observation') == 1
assert not any(value.startswith(('--cover-physics', '--cover-volume', '--cover-cylinder', '--cover-hit', '--cover-physx')) for value in sys.argv)
O = Path(__file__).resolve().parents[3] / 'analysis/solo-m1-20261028'
sys.path.insert(0, str(O))
for name in ('package-inspector', 'game-io', 'native-capture'):
    sys.path.insert(0, str(O.parents[1] / 'tools' / name))

s = (O / 'cover_boundary_chain.py').read_text(encoding='utf-8')
needle = "        assert observed['values']['CoverType'] == 2 and observed['values']['CoverAction'] == 0"
assert s.count(needle) == 1
s = s.replace(needle, needle + '''
        report['scope'] = 'Original CT2 entry; production C0 cover/body/view and L2 points; five bounded HMD phases; no collision query or grip action in this first slice'
        report['eye_observation'] = []
        rest = dict(g.head_rest)
        import ctypes
        def qpc():
            value = ctypes.c_longlong()
            assert ctypes.windll.kernel32.QueryPerformanceCounter(ctypes.byref(value))
            return value.value
        def eye_step(label):
            sample = current()
            assert sample['cover_state_raw']['CoverType'] == 2, 'Original cover lost during bounded eye observation'
            phase = dict(label=label, at_ns=time.perf_counter_ns(), begin_qpc=qpc(), end_qpc=None, observation=sample)
            report['eye_observation'].append(phase)
            g.rec.event(label, visual_accepted=False, CPU_eye_pose_join_pending=True)
            save()
            time.sleep(.8)
            assert g.live()
            phase['end_qpc'] = qpc()
            import launcher as cover_launcher
            deadline=time.monotonic()+8
            while True:
                session_now=observer.verify()
                fields=cover_launcher.current_native_status(observer.backend,session_now)
                phase['native']={k:v for k,v in fields.items() if k.startswith(('cover_input_','cover_l2_','cover_camera_','cover_body_','body_goal_','body_cover_'))}
                save()
                consumed=int(fields.get('body_goal_cover_consumed','0'))
                previous=int(report.get('last_cover_consumed',0))
                point_calls=fields.get('cover_l2_reference_calls','0,0,0,0').split(',')
                if consumed>previous and len(point_calls)==4 and int(point_calls[1])>0:break
                if time.monotonic()>=deadline:raise RuntimeError('No new current cover C0 consumption/L2 points: '+str(phase['native']))
                time.sleep(.1);assert g.live()
            assert fields.get('cover_input_enabled')=='1' and fields.get('cover_camera_enabled')=='1'
            assert fields.get('cover_input_c0_owner')=='1' and fields.get('body_goal_cover_enabled')=='1'
            assert fields.get('cover_body_live_published')=='0', 'Legacy pose writer must remain inactive'
            report['last_cover_consumed']=consumed;report['body_modified']=True
            phase['body_consumption_observed']=True

            save()
        eye_step('cover_eye_neutral')
        turned = dict(rest, yaw=rest['yaw'] + .16)
        g.head_move(rest, turned, 'cover_eye_fixed_position_turn')
        eye_step('cover_eye_turned')
        g.head_move(turned, rest, 'cover_eye_turn_returns')
        eye_step('cover_eye_turn_restored')
        raised = dict(rest, y=rest['y'] + .10)
        g.head_move(rest, raised, 'cover_eye_tracked_rise')
        eye_step('cover_eye_raised')
        g.head_move(raised, rest, 'cover_eye_rise_returns')
        eye_step('cover_eye_rise_restored')
        report['passed'] = True
        report['visual_accepted'] = False
        report['headset_accepted'] = False
        report['limits'] = 'Input/observation sequence only. Native same-callback CPU bracket and actual stereo review required; no complete lean/body/collision/exit acceptance.'
        g.result['checks']['cover_eye_observation_sequence_completed'] = True
        g.rec.event('cover_eye_observation_completed', visual_accepted=False)
        return
''')
needle = "        ctx.navigate(approach, arrival_distance=35.)"
assert s.count(needle) == 1
s = s.replace(needle, "        from cover_ordinary_approach import navigate as cover_navigate\n        cover_navigate(ctx, approach, arrival_distance=35.)")
m = types.ModuleType('cover_boundary_chain')
m.__file__ = str(O / 'cover_boundary_chain.py')
exec(compile(s, m.__file__, 'exec'), m.__dict__)
sys.modules[m.__name__] = m

s = (O / 'equipment_controller_chain.py').read_text(encoding='utf-8')
needle = "        initial=current('before_original_X_pickup')"
assert s.count(needle) == 1
s = s.replace(needle, needle + '\n        from types import SimpleNamespace\n        from cover_boundary_chain import run as cover_run\n        cover_run(SimpleNamespace(**locals()))\n        return')
m = types.ModuleType('equipment_controller_chain')
m.__file__ = str(O / 'equipment_controller_chain.py')
exec(compile(s, m.__file__, 'exec'), m.__dict__)
sys.modules[m.__name__] = m

s = (O / 'run_menu_firstperson_check.py').read_text(encoding='utf-8')
needle = "            session=json.loads(Path(state['session_report']).read_text(encoding='utf-8-sig'))"
assert s.count(needle) == 1
s = s.replace(needle, """            try:
                session=json.loads(Path(state['session_report']).read_text(encoding='utf-8-sig'))
            except (PermissionError,json.JSONDecodeError) as read_error:
                result['startup_status_read_retries']=result.get('startup_status_read_retries',0)+1
                observations=result.setdefault('startup_status_read_errors',[])
                if len(observations)<4:
                    observations.append(dict(error=type(read_error).__name__,message=str(read_error)))
                time.sleep(.02)
                continue""")
start = "    tap('menuButton',0);wait_pause(True,'VR_menu_pause');rec.event('pause_stable');time.sleep(1.5)"
end = "    tap('buttonB');wait_pause(False,'VR_B_resume');result['checks']['pause_B_resume']=True;time.sleep(1.5)"
assert s.count(start) == s.count(end) == 1
begin = s.index(start);finish = s.index(end, begin) + len(end)
s = s[:begin] + "    rec.event('cover_eye_sequence_finished', pause_B_failure_still_open=True)" + s[finish:]
exec(compile(s, str(O / 'run_menu_firstperson_check.py'), 'exec'),
     {'__file__': str(O / 'run_menu_firstperson_check.py'), '__name__': '__main__'})
