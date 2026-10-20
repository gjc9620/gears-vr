import json
from pathlib import Path
import tempfile
import unittest
from unittest import mock

from session_telemetry import RateTracker, SessionTelemetry, XR_FIELDS, latest_csv


def native(qpc=100, **extra):
    return dict(telemetry_schema=1,telemetry_qpc=qpc,telemetry_qpc_frequency=100,
        telemetry_present_enabled=1,telemetry_present_calls=100,telemetry_present_tests=10,
        published=60,world_generation=1,**extra)


class RateTests(unittest.TestCase):
    def test_distinct_rates_and_measured_zero(self):
        rates=RateTracker();a=native();b=dict(a,telemetry_qpc=200,telemetry_present_calls=160,
            telemetry_present_tests=20,published=100)
        self.assertTrue(all(x is None for x in rates.calculate(a,{'frame':1,'predicted_time':1000000000})[0].values()))
        result,states,_=rates.calculate(b,{'frame':91,'predicted_time':2000000000})
        self.assertEqual(result,dict(game_present_calls_per_second=50,stereo_pairs_per_second=40,xr_loop_hz=90))
        self.assertEqual(states,dict(native='fresh',xr='fresh'))
        self.assertIsNone(rates.calculate(b,{'frame':91,'predicted_time':2000000000})[0]['stereo_pairs_per_second'])
        result,_,_=rates.calculate(dict(b,telemetry_qpc=300),{'frame':91,'predicted_time':3000000000})
        self.assertEqual(set(result.values()),{0.0})

    def test_resets_and_old_native_never_manufacture_fps(self):
        for field,value in (('world_generation',2),('published',0),('telemetry_present_calls',0),('telemetry_qpc_frequency',50)):
            rates=RateTracker();a=native();rates.calculate(a,None)
            result,states,_=rates.calculate(dict(a,telemetry_qpc=200,**{field:value}),None)
            self.assertTrue(all(x is None for x in result.values()),field)
            self.assertEqual(states['native'],'counter_reset')
        rates=RateTracker()
        self.assertTrue(all(x is None for x in rates.calculate({'telemetry_schema':None,'telemetry_qpc_frequency':None},None)[0].values()))
        rates.calculate(native(),None)
        self.assertEqual(rates.calculate(dict(native(),published=61),None)[1]['native'],'counter_reset')


class SamplingTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup);self.root=Path(self.temp.name)
        self.csv=self.root/'xr.csv';self.output=self.root/'output.txt';self.path=self.root/'session.telemetry.jsonl'
        self.game=dict(pid=7,creation_time=8);self.owner=dict(pid=9,creation_time=10);self.clock=0
        self.header=','.join(XR_FIELDS)+'\n'
        self.row=dict(frame=1,predicted_time=1000000000,session_state=5,should_render=1,view_flags=15,
            head_flags=15,stereo_published=1,stereo_source=1)
        self.csv.write_text(self.header+','.join(str(self.row[x]) for x in XR_FIELDS)+'\n999,partial')
        self.output.write_text('pid=7 creation_time=8 complete=1 valid=1 width=1920 height=1080 windowed=1')

    def logger(self,**kwargs):
        return SessionTelemetry(self.path,self.csv,self.output,self.game,self.owner,clock=lambda:self.clock,**kwargs)

    def status(self,**kwargs):
        return dict({k:str(v) for k,v in native().items()},pid='7',owner_pid='9',owner_creation_time='10',started_qpc='11',complete='1',**kwargs)

    def test_partial_csv_identity_throttle_and_dimensions(self):
        self.assertEqual(latest_csv(self.csv),self.row)
        log=self.logger();log.bind_native(11);log.sample(self.status(),'running');log.sample(self.status(),'running')
        self.assertEqual(log.samples,1)
        self.clock+=1000000000
        wrong=self.status();wrong['owner_creation_time']='20';log.sample(wrong,'running',4)
        rows=[json.loads(x) for x in self.path.read_text().splitlines() if json.loads(x)['event']=='sample']
        self.assertEqual(rows[0]['render'],dict(width=1920,height=1080,windowed=1))
        self.assertEqual(rows[1]['source_states']['native'],'identity_mismatch_or_unavailable')
        self.assertTrue(all(x is None for x in rows[1]['rates'].values()))

    def test_rotation_keeps_recent_bounded_history(self):
        log=self.logger(max_bytes=4096)
        for index in range(70):log.event('state_change',phase='running',sequence=index,padding='x'*180)
        log.close('stopped')
        files=list(self.root.glob('session.telemetry*.jsonl'))
        self.assertEqual(len(files),3)
        self.assertTrue(all(x.stat().st_size<=4096 for x in files))
        self.assertGreater(log.rotations,0)
        self.assertEqual(json.loads(self.path.read_text().splitlines()[-1])['event'],'stopped')

    def test_unwritable_or_invalid_records_disable_diagnostics_only(self):
        with mock.patch.object(Path,'open',side_effect=PermissionError('denied')):log=self.logger()
        self.assertFalse(log.enabled);log.sample(None,'running');log.close('incomplete')
        log=self.logger();log.event('bad',value=object());self.assertFalse(log.enabled)
        self.assertIn('TypeError',log.summary()['error'])

    def test_legacy_fields_and_missing_sources_are_explicitly_unavailable(self):
        log=self.logger();log.bind_native(11)
        self.csv.unlink();self.output.unlink()
        status=self.status()
        for key in list(status):
            if key.startswith('telemetry_'):status.pop(key)
        log.sample(status,'running')
        row=json.loads(self.path.read_text().splitlines()[-1])
        self.assertTrue(all(x is None for x in row['rates'].values()))
        self.assertEqual(row['source_errors'],dict(xr='FileNotFoundError',render='FileNotFoundError'))


if __name__=='__main__':unittest.main()
