"""Bounded diagnostic sampling; never controls the game or the XR session.

Present calls, published stereo pairs and XR loop frequency are distinct
measurements. None establishes the frame rate physically displayed by a headset.
"""
import csv
import json
import os
from pathlib import Path
import stat
import time

NATIVE_FIELDS = (
    'published', 'epoch_submit_total', 'world_generation', 'world_handoff_phase',
    'world_prepare_requests', 'world_prepare_rejected', 'world_handoff_commits',
    'anchor_submission_matched', 'anchor_submission_rejected', 'reference_generation',
    'vr_dof_capture_rejected', 'vr_dof_no_site', 'vr_dof_zero_proved',
    'telemetry_schema', 'telemetry_qpc', 'telemetry_qpc_frequency',
    'telemetry_present_enabled', 'telemetry_present_calls', 'telemetry_present_tests',
    'telemetry_present_failures', 'path_trial_compiled', 'path_trial_active', 'path_trial_begun',
    'path_trial_restored', 'path_trial_rejected', 'path_trial_failed', 'path_trial_nested', 'path_trial_load_rejected')
XR_FIELDS = ('frame', 'predicted_time', 'session_state', 'should_render', 'view_flags',
             'head_flags', 'stereo_published', 'stereo_source')
RATE_DEFINITIONS = {
    'game_present_calls_per_second': 'Top-level bound game DXGI Present observations, excluding TEST calls; not engine-update or displayed FPS.',
    'stereo_pairs_per_second': 'New native stereo image publications; separate from repeated XR submissions.',
    'xr_loop_hz': 'CSV frame delta / runtime predicted-display-time delta; not verified projection or physical display FPS.',
    'missing': 'null means unavailable, first sample, stale sample, changed identity or reset; it does not mean zero FPS.'}


def integer(value):
    if type(value) is int:
        return value if 0 <= value < 1 << 64 else None
    if type(value) is str and value.isascii() and value.isdecimal() and len(value) <= 20:
        n = int(value)
        return n if n < 1 << 64 else None
    return None


def regular(path):
    """These are owned diagnostic files, not arbitrary linked input sources."""
    info = Path(path).lstat()
    if not stat.S_ISREG(info.st_mode) or getattr(info, 'st_file_attributes', 0) & 0x400:
        raise OSError('Diagnostic source is not a regular unlinked file')
    return info


def latest_csv(path):
    regular(path)
    with Path(path).open('rb') as stream:
        header_raw = stream.readline(4097)
        if len(header_raw) > 4096 or not header_raw.endswith(b'\n'):
            raise ValueError('Incomplete/oversized XR CSV header')
        header = next(csv.reader([header_raw.decode('ascii').strip()]))
        if len(header) != len(set(header)) or not set(XR_FIELDS) <= set(header):
            raise ValueError('Unsupported XR CSV columns')
        stream.seek(0, 2)
        start = max(len(header_raw), stream.tell() - 32768)
        stream.seek(start)
        if start > len(header_raw):stream.readline()
        raw = stream.read(32768)
    # A concurrently appended last row is not a sample until its newline exists.
    end = raw.rfind(b'\n')
    if end < 0:return None
    for line in reversed(raw[:end].splitlines()):
        values = next(csv.reader([line.decode('ascii')]))
        if len(values) != len(header):continue
        row = dict(zip(header, values))
        result = {key: integer(row[key]) for key in XR_FIELDS}
        if any(value is None for value in result.values()):continue
        if result['predicted_time'] and result['session_state'] <= 8 and result['should_render'] in (0, 1):
            return result
    return None


def output_dimensions(path, game):
    info = regular(path)
    if info.st_size > 8192:raise ValueError('Oversized output status')
    values = dict(item.split('=', 1) for item in Path(path).read_text(encoding='utf-8').split() if '=' in item)
    if (integer(values.get('pid')) != game['pid'] or
            integer(values.get('creation_time')) != game['creation_time'] or
            values.get('complete') != '1' or values.get('valid') != '1'):
        return None
    result = {key: integer(values.get(key)) for key in ('width', 'height', 'windowed')}
    if any(value is None for value in result.values()) or not 1 <= result['width'] <= 32768 or not 1 <= result['height'] <= 32768:
        return None
    return result


class RateTracker:
    def __init__(self):self.native = self.xr = None

    def calculate(self, native, xr):
        rates = dict.fromkeys(('game_present_calls_per_second', 'stereo_pairs_per_second', 'xr_loop_hz'))
        states = dict(native='unavailable', xr='unavailable')
        periods = {}
        if (native.get('telemetry_schema') == 1 and native.get('telemetry_qpc') is not None
                and (native.get('telemetry_qpc_frequency') or 0) > 0):
            current = dict(native)
            old = self.native
            states['native'] = 'first_sample'
            if old:
                delta = current['telemetry_qpc'] - old['telemetry_qpc']
                if delta == 0:
                    states['native'] = 'unchanged' if current == old else 'counter_reset'
                elif delta < 0 or current['telemetry_qpc_frequency'] != old['telemetry_qpc_frequency']:
                    states['native'] = 'counter_reset'
                else:
                    seconds = delta / current['telemetry_qpc_frequency']
                    periods['native_seconds'] = seconds
                    reset = current.get('world_generation') != old.get('world_generation')
                    present = ('telemetry_present_calls', 'telemetry_present_tests')
                    if not reset and all(current.get(k) is not None and old.get(k) is not None for k in present):
                        count = current[present[0]] - current[present[1]]
                        previous = old[present[0]] - old[present[1]]
                        if count >= previous >= 0 and current.get('telemetry_present_enabled') == old.get('telemetry_present_enabled') == 1:
                            rates['game_present_calls_per_second'] = (count-previous)/seconds
                        else:reset = True
                    if not reset and current.get('published') is not None and old.get('published') is not None:
                        if current['published'] >= old['published']:
                            rates['stereo_pairs_per_second'] = (current['published']-old['published'])/seconds
                        else:reset = True
                    if reset:
                        rates['game_present_calls_per_second'] = rates['stereo_pairs_per_second'] = None
                    states['native'] = 'counter_reset' if reset else 'fresh'
            if states['native'] != 'unchanged':self.native = current
        else:self.native = None
        if xr:
            old = self.xr
            states['xr'] = 'first_sample'
            if old:
                elapsed = xr['predicted_time']-old['predicted_time']
                delta = xr['frame']-old['frame']
                if elapsed == 0 and delta == 0:states['xr'] = 'unchanged'
                elif elapsed > 0 and delta >= 0:
                    rates['xr_loop_hz'] = delta/(elapsed/1e9)
                    periods['xr_predicted_seconds'] = elapsed/1e9
                    states['xr'] = 'fresh'
                else:states['xr'] = 'counter_reset'
            if states['xr'] != 'unchanged':self.xr = dict(xr)
        else:self.xr = None
        return rates, states, periods


class SessionTelemetry:
    def __init__(self, path, csv_path, output_path, game, owner, *, clock=time.perf_counter_ns,
                 interval_ns=1000000000, max_bytes=2*1024*1024, backups=2):
        self.path, self.csv_path, self.output_path = Path(path), Path(csv_path), Path(output_path)
        self.game, self.owner, self.clock = dict(game), dict(owner), clock
        self.interval_ns, self.max_bytes, self.backups = interval_ns, max_bytes, backups
        self.last_ns = None;self.expected_started = None;self.tracker = RateTracker()
        self.last_state = None;self.samples = 0;self.rotations = 0;self.error = None;self.enabled = True
        try:
            if type(backups) is not int or not 1 <= backups <= 4 or max_bytes < 4096 or interval_ns <= 0:
                raise ValueError('Invalid diagnostic rotation or sampling bounds')
            with self.path.open('x', encoding='utf-8'):pass
        except (OSError, ValueError) as error:self._disable(error)
        self.event('started', rate_definitions=RATE_DEFINITIONS)

    def _disable(self, error):
        self.enabled = False
        self.error = type(error).__name__ + ': ' + str(error)[:256]

    def _write(self, row):
        if not self.enabled:return
        try:
            encoded = json.dumps(dict(diagnostics_schema=1, **row), allow_nan=False, separators=(',', ':'))+'\n'
            if len(encoded.encode('utf-8')) > self.max_bytes:raise ValueError('Diagnostic record exceeds bound')
            info = regular(self.path)
            if info.st_size + len(encoded.encode('utf-8')) > self.max_bytes:
                for index in range(self.backups, 0, -1):
                    source = self.path if index == 1 else self.path.with_suffix(f'.{index-1}.jsonl')
                    target = self.path.with_suffix(f'.{index}.jsonl')
                    if source.exists():
                        regular(source)
                        if target.exists():regular(target)
                        os.replace(source, target)
                with self.path.open('x', encoding='utf-8'):pass
                self.rotations += 1
            with self.path.open('a', encoding='utf-8') as stream:stream.write(encoded)
        except (OSError, ValueError, TypeError) as error:self._disable(error)

    def event(self, name, **fields):
        self._write(dict(event=name, observed_ns=self.clock(), **fields))

    def bind_native(self, started_qpc):
        self.expected_started = str(started_qpc)
        self.tracker = RateTracker()
        self.event('native_ready')

    def sample(self, status, phase, menu_mode=None):
        if not self.enabled:return
        observed = self.clock()
        if self.last_ns is not None and observed-self.last_ns < self.interval_ns:return
        self.last_ns = observed
        valid = (isinstance(status, dict) and status.get('pid') == str(self.game['pid'])
                 and status.get('owner_pid') == str(self.owner['pid'])
                 and status.get('owner_creation_time') == str(self.owner['creation_time'])
                 and status.get('started_qpc') == self.expected_started and status.get('complete') == '1')
        native = {key: integer(status.get(key)) for key in NATIVE_FIELDS} if valid else {}
        source_errors = {}
        try:xr = latest_csv(self.csv_path)
        except (OSError, ValueError, UnicodeError, csv.Error) as error:
            xr = None;source_errors['xr'] = type(error).__name__
        try:render = output_dimensions(self.output_path, self.game)
        except (OSError, ValueError, UnicodeError) as error:
            render = None;source_errors['render'] = type(error).__name__
        rates, states, periods = self.tracker.calculate(native, xr)
        states['render'] = 'fresh' if render else 'unavailable'
        if not valid:states['native'] = 'identity_mismatch_or_unavailable'
        state = (phase, menu_mode, native.get('world_generation'), native.get('world_handoff_phase'),
                 (render or {}).get('width'), (render or {}).get('height'))
        if state != self.last_state:
            self._write(dict(event='state_change', observed_ns=observed, phase=phase, menu_mode=menu_mode,
                             native=native, render=render, source_states=states))
            self.last_state = state
        self._write(dict(event='sample', observed_ns=observed, phase=phase, menu_mode=menu_mode,
                         native=native, render=render, xr=xr, rates=rates, periods=periods,
                         source_states=states, source_errors=source_errors))
        self.samples += 1

    def close(self, phase, **fields):
        self.event('stopped', phase=phase, samples=self.samples, rotations=self.rotations, **fields)

    def summary(self):
        return dict(schema=1, enabled=self.enabled, samples=self.samples, rotations=self.rotations,
                    error=self.error, interval_seconds=self.interval_ns/1e9,
                    rate_definitions=RATE_DEFINITIONS)
