"""Offline diagnostics ZIP tests: temporary files and fake system providers only."""
import json
import csv
import io
import os
from pathlib import Path
import tempfile
import unittest
from unittest import mock
import zipfile

import diagnostics_bundle as bundle


class BundleTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.runtime = self.root / 'analysis/runtime'
        self.runtime.mkdir(parents=True)
        self.settings = self.root / 'settings'
        self.settings.mkdir()
        (self.root / 'tools/vr-launcher').mkdir(parents=True)
        self.output = self.root / 'feedback.zip'
        self.system = lambda: {'cpu':[{'name':'Test CPU', 'cores':8, 'logical_processors':16}],
            'gpu':[{'name':'Test GPU', 'driver_version':'1.2.3'}], 'physical_ram_bytes':32 * 1024**3,
            'os':{'caption':'Windows', 'version':'10.0', 'build':'26100', 'architecture':'64-bit'}}
        self.no_process = mock.patch.object(bundle.subprocess, 'run', side_effect=AssertionError('no process expected'))
        self.no_process.start()
        self.addCleanup(self.no_process.stop)

    def session(self, number=1, phase='running', **extra):
        path = self.runtime / f'continuous-stereo-{100+number}-20261017-12000{number}-{number:08x}.json'
        data = {'schema':1, 'kind':'continuous_strict_stereo', 'phase':phase, 'passed':phase=='stopped',
                'game_identity':{'pid':100+number, 'image':r'C:\Users\PrivateUser\Steam\76561198123456789\game.exe'},
                'owner_identity':{'pid':200+number, 'creation_time':123}, **extra}
        path.write_text(json.dumps(data))
        os.utime(path, (100+number, 100+number))
        return path

    def export(self, **kwargs):
        return bundle.export_bundle(self.root, self.output, system_reader=self.system, settings_root=self.settings, **kwargs)

    def read_zip(self):
        with zipfile.ZipFile(self.output) as archive:
            names = archive.namelist()
            raw = '\n'.join(archive.read(n).decode() for n in names)
            docs = {n:json.loads(archive.read(n)) for n in names if n.endswith('.json')}
            self.assertLessEqual(len(names), bundle.MAX_ARCHIVE_FILES)
            self.assertLessEqual(sum(x.file_size for x in archive.infolist()), bundle.MAX_ARCHIVE_BYTES)
            self.assertTrue(all('..' not in n and not n.startswith(('/', '\\')) for n in names))
            return names, raw, docs

    def test_latest_three_include_partial_live_and_stopped_without_claiming_zero_fps(self):
        self.session(1, 'failed')
        self.session(2, 'incomplete', error='startup failed', native_cleanup_verified=False)
        self.session(3, 'running')
        self.session(4, 'stopped', native_cleanup_verified=True,
            continuous_session_evidence={'continuous_session_evidence':True, 'submitted_frames':121,
                'span_seconds':2, 'maximum_submission_gap_ms':30})
        result = self.export()
        names, raw, docs = self.read_zip()
        self.assertEqual(result['files'], len(names))
        self.assertTrue(result['diagnostics_exported'])
        summaries = [docs[f'sessions/session-{n}.json'] for n in (1,2,3)]
        self.assertEqual([x['phase'] for x in summaries], ['stopped','running','incomplete'])
        self.assertEqual(summaries[0]['final_projection']['average_submissions_per_second'], 60)
        self.assertFalse(summaries[2]['native_cleanup_verified'])
        self.assertIn('startup failed', summaries[2]['errors']['error'])
        self.assertIsNone(summaries[1]['csv']['xr_prediction_hz'])
        self.assertIsNone(summaries[1]['final_projection']['maximum_submission_gap_ms'])
        self.assertNotIn('PrivateUser', raw)

    def test_numeric_csv_rates_are_labeled_and_unsafe_csv_is_not_archived(self):
        path = self.session()
        path.with_suffix('.csv').write_text('frame,predicted_time,stereo_sequence,published_ns,stereo_published\n'
            '1,1000000000,11,1000000000,1\n2,1020000000,12,1025000000,1\n3,1040000000,13,1050000000,1\n')
        self.export()
        names, _, docs = self.read_zip()
        summary = docs['sessions/session-1.json']['csv']
        self.assertEqual(summary['xr_prediction_hz'], 50)
        self.assertEqual(summary['stereo_publication_hz'], 40)
        self.assertIn('sessions/session-1.csv', names)
        path.with_suffix('.csv').write_text('frame,predicted_time,password\n1,100,secret-account\n')
        self.export(overwrite=True)
        names, raw, docs = self.read_zip()
        self.assertNotIn('sessions/session-1.csv', names)
        self.assertNotIn('secret-account', raw)
        self.assertIsNone(docs['sessions/session-1.json']['csv']['xr_prediction_hz'])
        self.assertTrue(any('CSV' in x for x in docs['manifest.json']['warnings']))

    def test_telemetry_rotations_render_dimensions_and_nullable_rates(self):
        path = self.session()
        older = {'schema':1, 'event':'started', 'observed_ns':1000, 'rates':{}}
        sample = {'schema':1, 'event':'sample', 'observed_ns':2000, 'phase':'running',
            'native':{'published':50, 'telemetry_present_calls':100, 'secret':'hidden'},
            'rates':{'game_present_calls_per_second':72.5, 'stereo_pairs_per_second':60, 'xr_loop_hz':None},
            'render':{'width':2534, 'height':1009, 'client_width':3840, 'client_height':2160, 'pointer':12345},
            'source_states':{'native':'fresh', 'xr':'unavailable'}, 'command':'never export me'}
        path.with_suffix('.telemetry.1.jsonl').write_text(json.dumps(older)+'\n')
        path.with_suffix('.telemetry.jsonl').write_text(json.dumps(sample)+'\n')
        self.export()
        _, raw, docs = self.read_zip()
        telemetry = docs['sessions/session-1.json']['telemetry']
        self.assertEqual(len(telemetry), 2)
        self.assertEqual(telemetry[-1]['rates']['game_present_calls_per_second'], 72.5)
        self.assertIsNone(telemetry[-1]['rates']['xr_loop_hz'])
        self.assertEqual(telemetry[-1]['render']['width'], 2534)
        self.assertEqual(docs['sessions/session-1.json']['actual_gpu_dimensions'], {'width':2534, 'height':1009})
        self.assertNotIn('pointer', telemetry[-1]['render'])
        self.assertNotIn('never export me', raw)
        self.assertNotIn('hidden', raw)

    def test_privacy_allowlists_never_follow_session_paths_or_include_proprietary_files(self):
        outside = self.root / 'outside-secret.json'
        outside.write_text('SHOULD_NOT_BE_READ')
        path = self.session(error=r'76561198123456789EngineUserSettings.ini user@example.com token=secret123 0x140296178 failed C:\Users\PrivateUser\76561198123456789\file.ini',
            session_report=str(outside), native_command=['dump',str(outside)], executable='proprietary.exe',
            pawn_location=[1,2,3], native_status={'published':'3','gpu_width':'2534','gpu_height':'1009','raw_bytes':'498b00498bc848ffa0a8040000'})
        path.with_suffix('.native-start.log').write_text('PROPRIETARY_NATIVE_CODE')
        path.with_suffix('.reserve.json').write_text('PROPRIETARY_NATIVE_CODE')
        (self.runtime / 'capture-status.txt').write_text('PROPRIETARY_NATIVE_CODE')
        (self.runtime / 'private.sqlite').write_text('PROPRIETARY_DATABASE')
        (self.root / 'tools/vr-launcher/config.json').write_text(json.dumps({'game_path':str(outside),
            'stereo_units_per_metre':100, 'password':'never-in-zip', 'launcher_language':'en'}))
        (self.root / 'tools/vr-launcher/manifest.json').write_text(json.dumps({'game_sha256':'a'*64,
            'native_build':r'C:\Users\PrivateUser\build-vrdof69', 'ui_observer':True, 'game_path':str(outside)}))
        (self.settings / '76561198123456789EngineUserSettings.ini').write_text('[Graphics]\nResolutionSizeX=3840\n'
            'ResolutionSizeY=2160\nbUseVSync=False\nAccountId=76561198123456789\nUserName=PrivateUser\n'
            'TextureGroup_World=(MinLODSize=256,MaxLODSize=2048,LODBias=-1,Owner=PrivateUser)\n')
        original_open = bundle.os.open
        with mock.patch.object(bundle.os, 'open', wraps=original_open) as opened:
            self.export()
        names, raw, docs = self.read_zip()
        for forbidden in ('PrivateUser','76561198123456789','user@example.com','secret123','0x140296178',
                          'PROPRIETARY_NATIVE_CODE','PROPRIETARY_DATABASE','SHOULD_NOT_BE_READ','never-in-zip'):
            self.assertNotIn(forbidden, raw)
        self.assertFalse(any('EngineUserSettings' in n or 'native-start' in n for n in names))
        self.assertFalse(any(str(call.args[0]) == str(outside) for call in opened.call_args_list))
        self.assertEqual(docs['game-settings.json'][0]['ResolutionSizeX'], 3840)
        self.assertEqual(docs['game-settings.json'][0]['TextureGroup_World']['LODBias'], -1)
        self.assertEqual(docs['configuration.json']['manifest']['native_build'], 'build-vrdof69')

    def test_world_guard_failure_zip_keeps_typed_causes_and_excludes_private_evidence(self):
        path = self.session()
        stable = dict(schema=1, stage='sample_before', cause='mounted_target_mismatch', attempts=1,
            retryable=False, strict_guard_accepted=False, elapsed_ms=12.5, target_relation='driver',
            consistent_read=True, timestamp_state='unchanged', objects_registry_state='unchanged',
            names_registry_state='unchanged', mismatch_stages=[], read_error_stage=None,
            roles={'driver':{'address':0x12345678,'name':'PRIVATE_OBJECT_NAME'}},
            expected_raw='PRIVATE_BYTES', error='PRIVATE_ERROR')
        changing = dict(stable, consistent_read=False, timestamp_state='changed',
            objects_registry_state='changed', mismatch_stages=['timestamp','header'], read_error_stage='memory')
        hostile = dict(stable, stage='PRIVATE_STAGE', cause='PRIVATE_CAUSE', target_relation='PRIVATE_NAME',
            timestamp_state='PRIVATE_BYTES', objects_registry_state=[], names_registry_state=12345678,
            attempts=12345678, elapsed_ms=float('inf'), retryable='PRIVATE_VALUE',
            mismatch_stages=['PRIVATE_STAGE']*20, read_error_stage='PRIVATE_ERROR')
        path.with_suffix('.world.jsonl').write_text(''.join(json.dumps(dict(event='world_rebind_failed',
            reason='AnchorSamplingRejected', detail='PRIVATE_DETAILS', guard_failure=summary,
            camera_observation={'address':0x12345678,'name':'PRIVATE_OBJECT_NAME'}))+'\n'
            for summary in (stable, changing, hostile)))
        self.export()
        _, raw, docs = self.read_zip()
        events = docs['sessions/session-1.json']['world_transitions']
        first, second, third = [event['guard_failure'] for event in events]
        self.assertEqual('mounted_target_mismatch', first['cause'])
        self.assertEqual('driver', first['target_relation'])
        self.assertTrue(first['consistent_read'])
        self.assertEqual('changed', second['timestamp_state'])
        self.assertEqual('changed', second['objects_registry_state'])
        self.assertEqual('memory', second['read_error_stage'])
        self.assertEqual(['timestamp','header'], second['mismatch_stages'])
        self.assertEqual('unknown', third['cause'])
        self.assertEqual('unavailable', third['timestamp_state'])
        self.assertIsNone(third['attempts']);self.assertIsNone(third['elapsed_ms'])
        self.assertIsNone(third['retryable'])
        self.assertEqual(['unknown']*8, third['mismatch_stages'])
        for private in ('PRIVATE', '12345678', 'expected_raw', 'camera_observation', 'roles', 'detail'):
            self.assertNotIn(private, raw)

    def test_render_size_feedback_keeps_typed_settings_and_filters_hostile_values(self):
        config = self.root / 'tools/vr-launcher/config.json'
        manifest = self.root / 'tools/vr-launcher/manifest.json'
        config.write_text(json.dumps({'render_scale': .7, 'game_path': 'PRIVATE_GAME_PATH'}))
        manifest.write_text(json.dumps({'runtime_render_size': True, 'unlisted_render_setting': 'PRIVATE'}))
        self.export()
        _, raw, docs = self.read_zip()
        self.assertEqual(docs['configuration.json']['config']['render_scale'], .7)
        self.assertIs(docs['configuration.json']['manifest']['runtime_render_size'], True)
        self.assertNotIn('PRIVATE', raw)
        for scale, flag in ((True, 1), ('0.7', 'true'), (None, None),
                            (float('nan'), []), (float('inf'), {})):
            config.write_text(json.dumps({'render_scale': scale}))
            manifest.write_text(json.dumps({'runtime_render_size': flag}))
            self.export(overwrite=True)
            _, _, docs = self.read_zip()
            self.assertNotIn('render_scale', docs['configuration.json']['config'])
            self.assertNotIn('runtime_render_size', docs['configuration.json']['manifest'])
        config.write_text(json.dumps({'render_scale': 1}))
        manifest.write_text(json.dumps({'runtime_render_size': False}))
        self.export(overwrite=True)
        _, _, docs = self.read_zip()
        self.assertEqual(docs['configuration.json']['config']['render_scale'], 1)
        self.assertIs(docs['configuration.json']['manifest']['runtime_render_size'], False)

    def test_corrupt_oversized_logs_and_partial_jsonl_create_warnings_not_fatal(self):
        broken = self.session(1)
        broken.write_text('{')
        large = self.session(2)
        large.write_text('x'*(bundle.MAX_INPUT_BYTES+1))
        valid = self.session(3, 'incomplete')
        valid.with_suffix('.menu.jsonl').write_text(json.dumps({'sequence':1,'reason':'valid',
            'state_evidence':{'top':{'tag':'DeltaPause','address':'0x1234'}}})+'\n{"partial":')
        self.export()
        _, raw, docs = self.read_zip()
        self.assertTrue(any('oversized' in x for x in docs['manifest.json']['warnings']))
        self.assertTrue(any('corrupt' in x for x in docs['manifest.json']['warnings']))
        summaries = [value for name, value in docs.items() if name.startswith('sessions/')]
        valid_summary = next(value for value in summaries if value['recognized'])
        self.assertEqual(valid_summary['menu_transitions'][0]['top']['tag'], 'DeltaPause')
        self.assertNotIn('0x1234', raw)
        self.assertEqual(sum(not value['recognized'] for value in summaries), 2)

    def test_links_and_reparse_candidates_are_rejected_without_reading(self):
        path = self.session()
        info = path.lstat()
        reparse = type('Info', (), dict(st_mode=info.st_mode, st_file_attributes=0x400))()
        original = Path.lstat
        def fake(item, *args, **kwargs):
            return reparse if item == path else original(item, *args, **kwargs)
        with mock.patch.object(Path, 'lstat', fake):
            self.export()
        _, _, docs = self.read_zip()
        self.assertEqual(docs['manifest.json']['sessions'], 0)
        self.assertTrue(any('unsafe session' in x for x in docs['manifest.json']['warnings']))
        with self.assertRaises(ValueError):bundle._checked(self.root / 'settings/../outside', self.root)

    def test_symlink_attachment_never_reads_external_content(self):
        path = self.session()
        outside = self.root / 'private.txt'
        outside.write_text('PRIVATE_SYMLINK_CONTENT')
        try:
            path.with_suffix('.menu.jsonl').symlink_to(outside)
        except OSError:
            self.skipTest('symlink creation is not permitted on this test host')
        self.export()
        _, raw, docs = self.read_zip()
        self.assertNotIn('PRIVATE_SYMLINK_CONTENT', raw)
        self.assertTrue(any('symlink/reparse' in x for x in docs['manifest.json']['warnings']))

    def test_no_history_or_system_inventory_still_produces_bundle(self):
        def unavailable():raise TimeoutError('username=PrivateUser')
        result = bundle.export_bundle(self.root, self.output, system_reader=unavailable, settings_root=self.settings)
        self.assertTrue(result['diagnostics_exported'])
        _, raw, docs = self.read_zip()
        self.assertFalse(docs['system.json']['available'])
        self.assertNotIn('PrivateUser', raw)
        self.assertEqual(docs['manifest.json']['sessions'], 0)
        self.assertTrue(any('system' in x for x in result['warnings']))

    def test_overwrite_is_explicit_and_atomic_failure_preserves_existing_file(self):
        self.output.write_bytes(b'PREVIOUS ZIP')
        with self.assertRaises(FileExistsError):self.export()
        self.assertEqual(self.output.read_bytes(), b'PREVIOUS ZIP')
        with mock.patch.object(bundle.os, 'replace', side_effect=OSError('disk error')):
            with self.assertRaises(OSError):self.export(overwrite=True)
        self.assertEqual(self.output.read_bytes(), b'PREVIOUS ZIP')
        self.assertEqual(list(self.root.glob('.gears-diagnostics-*')), [])
        self.export(overwrite=True)
        self.assertTrue(zipfile.is_zipfile(self.output))
        names, _, _ = self.read_zip()
        self.assertNotIn('feedback.zip', names)

    def test_create_new_race_and_archive_budget_failure_leave_no_partial_output(self):
        original = bundle.os.link
        def race(source, destination):
            Path(destination).write_bytes(b'RACED FILE')
            return original(source, destination)
        with mock.patch.object(bundle.os, 'link', side_effect=race):
            with self.assertRaises(FileExistsError):self.export()
        self.assertEqual(self.output.read_bytes(), b'RACED FILE')
        self.output.unlink()
        with mock.patch.object(bundle, 'MAX_ARCHIVE_BYTES', 128):
            with self.assertRaises(ValueError):self.export()
        self.assertFalse(self.output.exists())
        self.assertEqual(list(self.root.glob('.gears-diagnostics-*')), [])

    def test_system_provider_fields_are_curated_and_scalar_ids_sanitized(self):
        self.system = lambda: {'username':'PrivateUser','hostname':'PrivateMachine','serial':'SERIAL-SECRET',
            'cpu':[{'name':'CPU user@example.com','cores':8,'ProcessorId':'PROCESSOR-SECRET'}],
            'gpu':[{'name':'GPU','driver_version':'1.2','serial':'GPU-SECRET'}],
            'os':{'caption':'Windows','version':'10','hostname':'PrivateMachine'},'physical_ram_bytes':123}
        self.export()
        _, raw, _ = self.read_zip()
        for value in ('PrivateUser','PrivateMachine','SERIAL-SECRET','PROCESSOR-SECRET','GPU-SECRET','user@example.com'):
            self.assertNotIn(value, raw)

    def test_output_directory_failure_and_nonzip_target_do_not_touch_files(self):
        with self.assertRaises(FileNotFoundError):
            bundle.export_bundle(self.root, self.root/'missing/out.zip', system_reader=self.system, settings_root=self.settings)
        with self.assertRaises(ValueError):
            bundle.export_bundle(self.root, self.root/'game.exe', system_reader=self.system, settings_root=self.settings)
        self.assertFalse((self.root/'game.exe').exists())

    def test_csv_counter_reset_nonfinite_or_oversized_rates_are_unavailable(self):
        path = self.session()
        for content in ('frame,predicted_time\n10,1000000000\n1,2000000000\n',
                        'frame,predicted_time\n1,NaN\n2,2000000000\n',
                        'x'*(bundle.MAX_INPUT_BYTES+1)):
            with self.subTest(case=content[:30]):
                path.with_suffix('.csv').write_text(content)
                self.export(overwrite=self.output.exists())
                _, _, docs = self.read_zip()
                self.assertIsNone(docs['sessions/session-1.json']['csv']['xr_prediction_hz'])

    def test_zip_write_failure_leaves_no_output_or_temporary_file(self):
        with mock.patch.object(bundle.zipfile.ZipFile, 'writestr', side_effect=OSError('disk full')):
            with self.assertRaises(OSError):self.export()
        self.assertFalse(self.output.exists())
        self.assertEqual(list(self.root.glob('.gears-diagnostics-*')), [])

    def test_utf16_settings_export_only_allowed_numeric_fields(self):
        (self.settings/'EngineUserSettings.ini').write_text('[Display]\nResX=1920\nResY=1080\n'
            'DLSSMode=0\nHostName=PrivateMachine\n', encoding='utf-16')
        self.export()
        _, raw, docs = self.read_zip()
        self.assertEqual(docs['game-settings.json'][0], {'ResX':1920, 'ResY':1080, 'DLSSMode':0})
        self.assertNotIn('PrivateMachine', raw)

    def test_long_rotated_jsonl_uses_recent_complete_records_and_rate_statistics(self):
        path = self.session()
        for suffix, rate in (('.telemetry.2.jsonl',20), ('.telemetry.1.jsonl',30), ('.telemetry.jsonl',60)):
            lines = []
            for index in range(800):
                row = {'schema':1, 'event':'sample', 'observed_ns':1000+index,
                       'rates':{'game_present_calls_per_second':rate, 'stereo_pairs_per_second':rate,
                                'xr_loop_hz':None}, 'private_padding':''}
                base = json.dumps(row, separators=(',', ':'))
                row['private_padding'] = 'x' * (1023-len(base))
                line = json.dumps(row, separators=(',', ':')) + '\n'
                self.assertEqual(len(line.encode()),1024)
                lines.append(line)
            text = ''.join(lines)
            if suffix == '.telemetry.jsonl':text += '{"observed_ns":9999,"partial":'
            target = path.with_suffix(suffix)
            target.write_text(text, encoding='utf-8', newline='')
            self.assertGreater(target.stat().st_size,bundle.MAX_INPUT_BYTES)
        self.export()
        _, raw, docs = self.read_zip()
        summary = docs['sessions/session-1.json']
        self.assertEqual(summary['telemetry'][-1]['observed_ns'],1799)
        stats = summary['rate_statistics']['game_present_calls_per_second']
        self.assertEqual(stats,{'valid_samples':1533,'min':20,'mean':36.667,'max':60})
        self.assertEqual(summary['rate_statistics']['xr_loop_hz'],
                         {'valid_samples':0,'min':None,'mean':None,'max':None})
        self.assertNotIn('private_padding',raw)
        self.assertTrue(any('bounded recent tail' in x for x in docs['manifest.json']['warnings']))

    def test_long_csv_reads_latest_samples_and_exports_no_head_pose_columns(self):
        path = self.session()
        text = 'frame,predicted_time,head_x,head_y,stereo_sequence,published_ns,stereo_published\n'
        pose = '0.' + '0'*200 + '1'
        text += ''.join(f'{i},{1000000000+i*20000000},{pose},987.12345,{i},{1000000000+i*25000000},1\n'
                        for i in range(1,4001))
        text += '4001,partial-tail'
        target = path.with_suffix('.csv');target.write_text(text)
        self.assertGreater(target.stat().st_size,bundle.MAX_INPUT_BYTES)
        self.export()
        _, raw, docs = self.read_zip()
        summary = docs['sessions/session-1.json']['csv']
        self.assertEqual(summary['rows'],512)
        self.assertEqual(summary['xr_prediction_hz'],50)
        self.assertEqual(summary['stereo_publication_hz'],40)
        self.assertEqual(summary['rate_statistics']['xr_prediction_hz'],
                         {'valid_samples':511,'min':50,'mean':50,'max':50})
        with zipfile.ZipFile(self.output) as archive:
            rows = list(csv.DictReader(io.StringIO(archive.read('sessions/session-1.csv').decode())))
        self.assertEqual(int(rows[0]['frame']),3489)
        self.assertEqual(int(rows[-1]['frame']),4000)
        self.assertNotIn('head_x',rows[0]);self.assertNotIn('head_y',rows[0])
        self.assertNotIn('987.12345',raw)

    def test_tail_cut_discards_torn_utf8_and_partial_or_corrupt_records(self):
        path = self.session()
        target = path.with_suffix('.menu.jsonl')
        text = '中'*(bundle.MAX_INPUT_BYTES//3+100)+'\n'+json.dumps({'sequence':42,'reason':'latest'})+'\n{"partial":'
        target.write_text(text,encoding='utf-8',newline='')
        self.export()
        _, _, docs = self.read_zip()
        self.assertEqual(docs['sessions/session-1.json']['menu_transitions'][0]['sequence'],42)

    def test_runtime_identity_uses_only_known_fields_from_bounded_fixed_logs(self):
        path = self.session(xr_command=['secret-runtime-path'])
        path.with_suffix('.stdout.txt').write_text('runtime_name=Virtual Desktop XR\nruntime_version=1.2.3\n'
            + 'DO_NOT_EXPORT_NATIVE_OR_PERSONAL_DETAILS\n'*30000
            + json.dumps({'operator_runtime':'VDXR','username':'PrivateUser','secret':'TOKEN-SECRET'})+'\n')
        path.with_suffix('.stderr.txt').write_text('runtime_name=Meta XR Simulator runtime_version=281474976710656 simulator_operator=1\n')
        self.export()
        _, raw, docs = self.read_zip()
        runtime = docs['sessions/session-1.json']['runtime']
        self.assertEqual(runtime['runtime_name'],'Meta XR Simulator')
        self.assertEqual(runtime['operator_runtime'],'VDXR')
        self.assertTrue(runtime['simulator_operator'])
        self.assertNotIn('DO_NOT_EXPORT_NATIVE_OR_PERSONAL_DETAILS',raw)
        self.assertNotIn('TOKEN-SECRET',raw);self.assertNotIn('PrivateUser',raw)
        self.assertNotIn('secret-runtime-path',raw)

    def test_missing_or_sensitive_runtime_fields_remain_unknown(self):
        path = self.session()
        path.with_suffix('.stdout.txt').write_text(json.dumps({'runtime_name':r'C:\Users\PrivateUser\secret.dll',
            'runtime_version':'76561198123456789','operator_runtime':'user@example.com'})+'\n')
        self.export()
        _, raw, docs = self.read_zip()
        self.assertEqual(docs['sessions/session-1.json']['runtime'],
            {'runtime_name':'unknown','runtime_version':'unknown','operator_runtime':'unknown','simulator_operator':None,'probe_errors':[]})
        self.assertNotIn('PrivateUser',raw);self.assertNotIn('76561198123456789',raw)

    def test_runtime_log_symlink_is_not_followed(self):
        path = self.session()
        outside = self.root/'outside-runtime.txt'
        outside.write_text('runtime_name=PRIVATE_RUNTIME\n')
        try:path.with_suffix('.stderr.txt').symlink_to(outside)
        except OSError:self.skipTest('symlink creation is not permitted on this test host')
        self.export()
        _, raw, docs = self.read_zip()
        self.assertEqual(docs['sessions/session-1.json']['runtime']['runtime_name'],'unknown')
        self.assertTrue(any('symlink/reparse' in x for x in docs['manifest.json']['warnings']))
        self.assertNotIn('PRIVATE_RUNTIME',raw)

    def test_xr_probe_failure_is_preserved_without_raw_log_or_personal_path(self):
        path=self.session(phase='incomplete')
        path.with_suffix('.stderr.txt').write_text('PROPRIETARY_OTHER_LOG\nPose probe error: invalid dimensions 1920x764; C:\\Users\\PrivateUser\\file.txt\n')
        self.export();_,raw,docs=self.read_zip()
        errors=docs['sessions/session-1.json']['runtime']['probe_errors']
        self.assertEqual(len(errors),1)
        self.assertIn('invalid dimensions 1920x764',errors[0])
        self.assertNotIn('PrivateUser',raw);self.assertNotIn('PROPRIETARY_OTHER_LOG',raw)

    def test_actual_graphics_key_names_and_texture_groups_remain_scalar_only(self):
        (self.settings/'EngineUserSettings.ini').write_text('[Graphics]\nStartupResolutionX=2534\nStartupResolutionY=1009\n'
            'StartupFullscreen=False\nTargetFramerate=0.000000\n120HzOutput=False\nDLSS=False\nFSR3=False\n'
            'DepthOfField=True\nAAQualityMode=0\nPowerSaving=False\nHDRPaperWhite=140.0\n'
            'TEXTUREGROUP_LightAndShadowMap=(MinLODSize=64,MaxLODSize=128,LODBias=1,Secret=PrivateUser)\n'
            'TEXTUREGROUP_World_High=(MinLODSize=64,MaxLODSize=256,LODBias=0)\n'
            '[Other]\nMusicVolume=6\nAccountId=76561198123456789\nGamePath=C:\\PrivateUser\\game.exe\n')
        self.export()
        _, raw, docs = self.read_zip()
        settings = docs['game-settings.json'][0]
        self.assertEqual(settings['StartupResolutionX'],2534)
        self.assertFalse(settings['StartupFullscreen'])
        self.assertTrue(settings['DepthOfField'])
        self.assertEqual(settings['TEXTUREGROUP_World_High']['MaxLODSize'],256)
        self.assertNotIn('MusicVolume',settings)
        self.assertNotIn('PrivateUser',raw)

    def test_rate_statistics_exclude_missing_invalid_and_negative_values(self):
        path = self.session()
        values = [None, True, float('nan'), -1, 0, 60, 90]
        path.with_suffix('.telemetry.jsonl').write_text(''.join(json.dumps({'event':'sample',
            'observed_ns':index+1,'rates':{'game_present_calls_per_second':value}})+'\n'
            for index,value in enumerate(values)))
        self.export()
        _, _, docs = self.read_zip()
        stats = docs['sessions/session-1.json']['rate_statistics']
        self.assertEqual(stats['game_present_calls_per_second'], {'valid_samples':3,'min':0,'mean':50,'max':90})
        self.assertEqual(stats['stereo_pairs_per_second'], {'valid_samples':0,'min':None,'mean':None,'max':None})

    def test_render_plan_export_is_curated_and_not_attributed_to_old_sessions(self):
        self.session()
        plan = {'runtime_render_size_plan':True, 'render_scale':.7,
                'recommended':[3072,3264], 'eye':[2151,2285], 'source':[4302,2285],
                'runtime_path':'SECRET_RUNTIME_PATH'}
        (self.runtime/'vr-launcher-session.json').write_text(json.dumps({
            'schema':1,'kind':'repository_vr_launcher','phase':'preflight','render_size_plan':plan}))
        self.export()
        _, raw, docs = self.read_zip()
        exported = docs['launcher-state.json']['render_size_plan']
        self.assertEqual(exported['eye'],[2151,2285])
        self.assertEqual(exported['recommended'],[3072,3264])
        self.assertEqual(exported['render_scale'],.7)
        self.assertEqual(exported['evidence'],'launcher_preflight_plan_not_gpu_measurement')
        self.assertNotIn('SECRET_RUNTIME_PATH',raw)
        self.assertNotIn('render_size_plan',docs['sessions/session-1.json'])

    def test_render_plan_rejects_invalid_dimensions_scale_and_relationships(self):
        valid = {'runtime_render_size_plan':True,'render_scale':1.,
                 'recommended':[3072,3264],'eye':[3072,3264],'source':[6144,3264]}
        for changes in ({'render_scale':True}, {'render_scale':float('nan')},
                        {'render_scale':float('inf')}, {'render_scale':0},
                        {'eye':[True,3264]}, {'eye':[3071,3264]},
                        {'source':[3072,3264]}, {'recommended':[32769,3264]},
                        {'recommended':[3072]}, {'runtime_render_size_plan':1}):
            with self.subTest(changes=changes):
                self.assertIsNone(bundle._render_size_plan({**valid,**changes}))
        (self.runtime/'vr-launcher-session.json').write_text(json.dumps({
            'schema':1,'kind':'repository_vr_launcher','render_size_plan':{'eye':[1,2]}}))
        self.export()
        _, raw, docs = self.read_zip()
        self.assertNotIn('render_size_plan',docs['launcher-state.json'])
        self.assertIn('invalid render size plan omitted',raw)

    def test_launcher_failure_before_continuous_session_is_retained_without_paths(self):
        index = self.runtime/'vr-launcher-session.json'
        index.write_text(json.dumps({'schema':1,'kind':'repository_vr_launcher','phase':'reserving',
            'error':r'Cannot start C:\Users\PrivateUser\game.exe', 'native_activated':False, 'passed':False,
            'control_exit_code':1, 'session_report':'DO_NOT_FOLLOW', 'native_command':['SECRET_COMMAND'],
            'prime_cleanup':{'cancelled':True,'entry_restore_verified':True,'game_exited':False,'native_bytes':'PROPRIETARY'},
            'tiny_output_monitor_cleanup':{'joined':False,'error':r'busy C:\Users\PrivateUser\monitor.py','raw':'PROPRIETARY'}}))
        self.export()
        _, raw, docs = self.read_zip()
        state = docs['launcher-state.json']
        self.assertEqual(state['phase'],'reserving');self.assertFalse(state['native_activated'])
        self.assertEqual(state['control_exit_code'],1)
        self.assertEqual(state['prime_cleanup'], {'cancelled':True,'entry_restore_verified':True,'game_exited':False})
        self.assertIs(state['tiny_output_monitor_cleanup']['joined'],False)
        self.assertNotIn('raw',state['tiny_output_monitor_cleanup'])
        for secret in ('PrivateUser','DO_NOT_FOLLOW','SECRET_COMMAND','PROPRIETARY'):
            self.assertNotIn(secret,raw)

    def test_session_artifact_identity_is_validated_and_not_borrowed_from_default(self):
        path = self.session(diagnostics_artifacts={'native_sha256':'B'*64,'xr_sha256':'c'*64,
            'game_sha256':'not-a-hash','native_build':'native-build-ui-root69','extra_path':'PRIVATE_PATH'})
        (self.root/'tools/vr-launcher/manifest.json').write_text(json.dumps({'native_sha256':'a'*64}))
        self.export()
        _, raw, docs = self.read_zip()
        self.assertEqual(docs['sessions/session-1.json']['diagnostics_artifacts'],
                         {'native_sha256':'b'*64,'xr_sha256':'c'*64,'native_build':'native-build-ui-root69'})
        self.assertNotIn('PRIVATE_PATH',raw)
        path.write_text(json.dumps({'schema':1,'kind':'continuous_strict_stereo','phase':'running',
            'diagnostics_artifacts':{'native_sha256':'invalid','native_build':'../native-build-ui-root70'}}))
        self.export(overwrite=True)
        _, _, docs = self.read_zip()
        self.assertEqual(docs['sessions/session-1.json']['diagnostics_artifacts'],{})


    def test_startup_reservation_cause_survives_export_before_any_stereo_session(self):
        import launcher
        cause='RuntimeError: Persisted DLSS must be OFF before release startup'
        detail=launcher.reservation_failure_detail('', 'Traceback (most recent call last):\n'+cause+'\n'
            'subprocess.CalledProcessError: wrapper failed\n')
        error=RuntimeError('Fresh reservation failed; no retry (exit code 1): '+detail+
            '\nDetails: C:\\Users\\PrivateUser\\Mod\\analysis\\runtime\\vr-launcher-session.reserve.log')
        (self.runtime/'vr-launcher-session.json').write_text(json.dumps({
            'schema':1,'kind':'repository_vr_launcher','phase':'incomplete','error':repr(error)}))
        (self.runtime/'vr-launcher-session.reserve.log').write_text('PROPRIETARY_NATIVE_CODE')
        self.export()
        names,raw,docs=self.read_zip()
        self.assertIn(cause,raw)
        self.assertNotIn('CalledProcessError',raw)
        self.assertNotIn('PrivateUser',raw)
        self.assertNotIn('PROPRIETARY_NATIVE_CODE',raw)
        self.assertFalse(any(name.endswith('.reserve.log') for name in names))

    def startup(self, number, events, **context):
        attempt = f'{number:032x}'
        path = self.runtime/f'vr-launcher-session.reserve.log.{attempt}.startup.jsonl'
        base = dict(schema=1, kind='startup_diagnostic')
        rows = [dict(base, event='context', phase='launcher_subprocess', attempt_id=attempt,
                     context=dict(version='0.0.2', diagnostics_revision='startup-diagnostics-1', **context))]
        rows += [dict(base, **event) for event in events]
        path.write_text(''.join(json.dumps(row)+'\n' for row in rows))
        os.utime(path, (number, number))
        return path

    def test_startup_failure_exports_phase_codes_component_hashes_without_a_stereo_session(self):
        self.startup(1, [dict(event='begin', phase='target_priming', elapsed_ms=100),
            dict(event='complete', phase='target_priming', remote_thread_exit_code=1, duration_ms=12),
            dict(event='failure', phase='target_priming', error='RuntimeError: Target priming failed (1)',
                process=dict(state='exited', same_identity=True, pid=55, exit_code=1, exit_code_hex='0x00000001'),
                prime_status=dict(state='captured', setup_failed=True, hooks=[dict(create='MH_ERROR_MEMORY_ALLOC', queue='MH_ERROR_NOT_CREATED')],
                    raw='PRIVATE_NATIVE_BYTES'), raw='PROPRIETARY_NATIVE_CODE')],
            python_version='3.12.7', bundled_python=True, steam_app_id='2523720', steam_game_id='2523720',
            steam_environment_scope='child_process', script_hashes={'tools/native-capture/launch_reserve.py':'a'*64,
                'C:/PrivateUser/private.py':'b'*64}, artifacts={'native_sha256':'c'*64},
            environment={'PASSWORD':'PRIVATE_SECRET'})
        self.export()
        _, raw, docs = self.read_zip()
        value = docs['startup-attempts.json']['attempts'][0]
        self.assertEqual(value['context']['script_hashes'], {'tools/native-capture/launch_reserve.py':'a'*64})
        self.assertTrue(value['context']['bundled_python'])
        self.assertEqual(value['events'][1]['remote_thread_exit_code'], 1)
        self.assertEqual(value['events'][2]['process']['exit_code'], 1)
        self.assertEqual(value['events'][2]['prime_status']['hooks'][0]['create'], 'MH_ERROR_MEMORY_ALLOC')
        self.assertEqual(docs['manifest.json']['sessions'], 0)
        for forbidden in ('PRIVATE', 'PROPRIETARY', '"environment":', 'password'):
            self.assertNotIn(forbidden, raw)
        with zipfile.ZipFile(self.output) as archive:
            summary = archive.read('startup-summary.txt').decode()
        self.assertIn('target_priming: failure', summary)
        self.assertIn('Game process: exited', summary)

    def test_startup_keeps_latest_three_and_complete_rows_and_reads_only_correlated_launch(self):
        for number in (1, 2, 3, 4):
            path = self.startup(number, [dict(event='begin', phase='launch_script', elapsed_ms=number)])
        with path.open('a') as stream:stream.write('{"partial":')
        birth = dict(schema=1, kind='reservation_birth', injection_attempted=True, prime_attempted=True,
                     game_identity=dict(pid=55, creation_time=123, image='C:/PrivateUser/game.exe'),
                     record_path='C:/PrivateUser/must-not-follow.json')
        path.with_name(path.name[:-len('.startup.jsonl')]+'.birth.json').write_text(json.dumps(birth))
        (self.runtime/'background-launch-55.json').write_text(json.dumps(dict(pid=55, creation_time=123,
            steam_app_id='2523720', steam_game_id='2523720', steam_environment_scope='child_process',
            window_startup_policy='foreground', background_input_requested=False, raw='PRIVATE_BYTES')))
        with mock.patch.object(bundle.startup_diagnostics, 'ProcessObservation', side_effect=AssertionError('export must not inspect games')):
            self.export()
        _, raw, docs = self.read_zip()
        attempts = docs['startup-attempts.json']['attempts']
        self.assertEqual(len(attempts), 3)
        self.assertEqual([a['events'][0]['elapsed_ms'] for a in attempts], [4,3,2])
        self.assertTrue(attempts[0]['birth']['prime_attempted'])
        self.assertEqual(attempts[0]['background_launch']['steam_game_id'], '2523720')
        self.assertNotIn('PRIVATE', raw); self.assertNotIn('PrivateUser', raw)
        self.assertNotIn('partial', raw)

    def test_startup_reparse_candidates_and_bad_context_are_omitted(self):
        path = self.startup(1, [])
        info = path.lstat()
        reparse = type('Info', (), dict(st_mode=info.st_mode, st_file_attributes=0x400))()
        original = Path.lstat
        def fake(item, *args, **kwargs):
            return reparse if item == path else original(item, *args, **kwargs)
        with mock.patch.object(Path, 'lstat', fake):
            self.export()
        _, raw, docs = self.read_zip()
        self.assertFalse(docs['startup-attempts.json']['history_available'])
        self.assertTrue(any('unsafe startup' in warning for warning in docs['manifest.json']['warnings']))
        path.write_text(json.dumps(dict(schema=1, kind='startup_diagnostic', event='context', attempt_id='wrong'))+'\n')
        self.export(overwrite=True)
        _, _, docs = self.read_zip()
        self.assertFalse(docs['startup-attempts.json']['history_available'])

    def test_old_install_reports_missing_startup_trace_without_fabricated_exit_information(self):
        self.export()
        _, _, docs = self.read_zip()
        self.assertFalse(docs['startup-attempts.json']['history_available'])
        self.assertEqual(docs['startup-attempts.json']['attempts'], [])
        self.assertEqual(docs['build-info.json']['diagnostics_revision'], 'startup-diagnostics-1')


if __name__ == '__main__':
    unittest.main()
