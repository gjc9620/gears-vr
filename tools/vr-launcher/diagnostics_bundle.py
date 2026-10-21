"""Curated local diagnostics only; no game handles, network, or raw native logs."""
import csv
import datetime as dt
import io
import json
import math
import os
from pathlib import Path
import re
import stat
import subprocess
import tempfile
import zipfile
import startup_diagnostics

MAX_INPUT_BYTES = 512 * 1024
MAX_ARCHIVE_BYTES = 2 * 1024 * 1024
MAX_ARCHIVE_FILES = 24
MAX_SCAN = 20000
MAX_CANDIDATES = 512
SESSION_NAME = re.compile(r'continuous-stereo-\d+-\d{8}-\d{6}-[a-f0-9]{8}\.json\Z')
FLAGS = set(('startup_menu anchor_observer ui_observer outage_diagnostics image_ticket_diagnostics shutdown_diagnostics '
             'ui_hud_plane gpu_snapshot graphics_startup_lock graphics_menu_lock menu_recovery cinematic_camera '
             'world_recovery hud_size_control crosshair_assignment_observer movie_display hud_layout_control '
             'camera_distance_control startup_selftests_prelaunch path_choice_center runtime_render_size').split())
SESSION_FIELDS = set(('schema phase passed native_started native_stop_signaled native_cleanup_verified entry_restore_verified '
                      'xr_stop_signaled xr_exit_code startup_cancelled startup_panel_submitted preflight_frames '
                      'operator_motion_requested simulation_not_physical_tracking physical_tracking_verified '
                      'hud_horizontal_fov_degrees camera_closer_metres hud_element_scale hud_center_scale path_choice_center_enabled '
                      'path_choice_center_worker_joined').split())
CONFIG_FIELDS = set(('schema stereo_units_per_metre hud_horizontal_fov_degrees camera_closer_metres '
                     'hud_element_scale hud_center_scale launcher_language render_scale').split()) | FLAGS
ERROR_FIELDS = ('error', 'native_cleanup_error', 'xr_cleanup_error', 'xr_forced_cleanup_reason',
                'path_choice_center_close_error', 'path_choice_center_cleanup_error')
COUNTERS = set(('published epoch_submit_total capture_attempts capture_skipped busy active pending retained world_generation '
                'world_handoff_commits anchor_submission_matched anchor_submission_rejected vr_dof_capture_rejected '
                'width height source_width source_height game_width game_height eye_width eye_height gpu_width gpu_height '
                'ticket_rejected rectangle_rejected path_trial_compiled path_trial_active path_trial_begun '
                'path_trial_restored path_trial_rejected path_trial_failed path_trial_nested path_trial_load_rejected').split())
CSV_FIELDS = set(('frame predicted_time session_state should_render view_flags head_flags head_x head_y head_z qx qy qz qw '
                  'left_x left_y left_z right_x right_y right_z left_angle_left left_angle_right left_angle_up left_angle_down '
                  'left_qx left_qy left_qz left_qw right_qx right_qy right_qz right_qw right_angle_left right_angle_right '
                  'right_angle_up right_angle_down stereo_session stereo_sequence published_ns stereo_published stereo_source').split())
CSV_PERFORMANCE_FIELDS = ('frame', 'predicted_time', 'session_state', 'should_render', 'view_flags', 'head_flags',
                          'stereo_sequence', 'published_ns', 'stereo_published', 'stereo_source')
RATE_FIELDS = ('game_present_calls_per_second', 'stereo_pairs_per_second', 'xr_loop_hz')
SCALAR_SETTINGS = {x.lower() for x in ('ResolutionSizeX ResolutionSizeY LastUserConfirmedResolutionSizeX '
    'LastUserConfirmedResolutionSizeY DesiredScreenWidth DesiredScreenHeight FullscreenMode PreferredFullscreenMode '
    'LastConfirmedFullscreenMode FrameRateLimit bUseVSync bUseDynamicResolution ScreenPercentage TextureQuality '
    'ResX ResY Fullscreen UseVsync bUseHDRDisplayOutput HDRDisplayOutputNits DLSSMode FSRQualityMode UpscalerMode '
    'UpscaleQuality bEnableDLSS bEnableFSR bEnableXeSS MaxAnisotropy MaxMultisamples DetailMode SkeletalMeshLODBias '
    'TextureDetailLevel LODBias MaxLODSize MinLODSize sg.ResolutionQuality sg.ViewDistanceQuality sg.AntiAliasingQuality '
    'sg.ShadowQuality sg.PostProcessQuality sg.TextureQuality sg.EffectsQuality sg.FoliageQuality '
    'StartupResolutionX StartupResolutionY HiStartupResolutionX HiStartupResolutionY LowStartupResolutionX '
    'LowStartupResolutionY RecStartupResolutionX RecStartupResolutionY StartupFullscreen TargetFramerate '
    '120HzOutput Use120ForCampaignWithVRR PowerSaving InactiveWindowEnergySaving CampaignPowerSavingMode '
    'VersusPowerSavingMode FieldOfView DisplayGamma AllowHDR HDRPaperWhite HDRMaxBrightness HDRContrast '
    'HDRBrightness HDRUILevel HDRUIMultiplier HDRUIClamp HDRUIPower HDRColorSpaceRotateVersion '
    'AAQualityMode AADefaultQualityMode AAConsoleMPPerformanceDefaultQualityMode HFXAAPostProcessAAQuality '
    'ALLOWPOSTPROCESSMLAA ALLOWPOSTPROCESSFXAA FSR2 FSR3 MFSR DLSS FSR2Sharpening FSR2Sharpness '
    'VRS VRSBasePass VRSTranslucency ScreenPercentageFinal ScreenPercentageAdaptiveScale '
    'DynamicLights DynamicShadows StaticDecals DynamicDecals LightEnvironmentShadows CompositeDynamicLights '
    'DirectionalLightMaps MotionBlur DepthOfField Bloom QualityBloom SpeedTreeLeaves SpeedTreeFronds '
    'OnlyStreamInTextures LensFlares FloatingPointRenderTargets OneFrameGPULag ParticleLODBias '
    'ALLOWD3D10 SSAOQualityLevel Cacao CacaoQualityLevel CacaoQualityLevelSplitScreen SSRQuality EnableSSR '
    'HalfResShadows WholeSceneDominantShadows MaxShadowResolution MinShadowResolution '
    'MaxWholeSceneDominantShadowResolution HalfResInputSSR OutputSRGB AllowVariableRefreshRate SyncSimToVBlank').split()}
TEXTURE_GROUPS = {'texturegroup_' + x.lower() for x in ('World WorldNormalMap WorldSpecular Character CharacterNormalMap '
    'CharacterSpecular Weapon WeaponNormalMap WeaponSpecular Vehicle VehicleNormalMap VehicleSpecular Effects Skybox '
    'UI Lightmap Shadowmap RenderTarget SmallNormalMap LightAndShadowMap 4K ImageBasedReflection Vegetation '
    'VegetationNormalMap VegetationSpecular World_High WorldNormalMap_High WorldSpecular_High World_Low '
    'WorldNormalMap_Low WorldSpecular_Low ColorLookupTable').split()}


def _text(value, limit=300):
    text = str(value)[:2048]
    text = re.sub(r'[A-Z]:[\\/][^\r\n\"\'<>]*|\\\\[^\s\"\'<>]+|(?<![\w:])/(?:[^\s\"\'<>]+)', '<PATH>', text, flags=re.I)
    text = re.sub(r'[\w.+-]+@[\w.-]+\.[A-Za-z]{2,}', '<EMAIL>', text)
    text = re.sub(r'(?<!\d)\d{17}(?!\d)', '<ACCOUNT>', text)
    text = re.sub(r'\b0x[0-9a-f]+\b|\b[0-9a-f]{16,}\b', '<HEX>', text, flags=re.I)
    text = re.sub(r'(?i)\b(password|secret|token|api[_-]?key|username|steamid|account)\s*[:=]\s*(?:"[^"]*"|\'[^\']*\'|\S+)', r'\1=<REDACTED>', text)
    return text[:limit]


def _number(value):
    if isinstance(value, bool):
        return None
    try:
        result = float(value)
        if not math.isfinite(result) or abs(result) > 1e18 or (result.is_integer() and 1e16 <= abs(result) < 1e17):
            return None
        return int(result) if result.is_integer() else result
    except (ValueError, TypeError, OverflowError):
        return None


def _scalar(value):
    if isinstance(value, bool):
        return value
    if isinstance(value, (int, float)):
        return _number(value)
    if isinstance(value, str):
        return _text(value)
    return None


def _pick(data, keys, numeric=False):
    if not isinstance(data, dict):
        return {}
    out = {}
    for key in sorted(keys):
        if key in data:
            value = _number(data[key]) if numeric else _scalar(data[key])
            if value is not None:
                out[key] = value
    return out


def _checked(path, anchor, *, directory=False):
    """Reject all links/reparse points, including ancestor directory redirects."""
    path, anchor = Path(path).absolute(), Path(anchor).absolute()
    if '..' in path.parts:
        raise ValueError('path traversal rejected')
    relative = path.relative_to(anchor)
    items, current = [*reversed(anchor.parents), anchor], anchor
    for part in relative.parts:
        current = current / part
        items.append(current)
    for item in items:
        info = item.lstat()
        if stat.S_ISLNK(info.st_mode) or getattr(info, 'st_file_attributes', 0) & 0x400:
            raise ValueError('symlink/reparse candidate rejected')
    info = path.lstat()
    if not (stat.S_ISDIR(info.st_mode) if directory else stat.S_ISREG(info.st_mode)):
        raise ValueError('candidate is not a regular ' + ('directory' if directory else 'file'))
    return info


class Collector:
    def __init__(self, root):
        self.root = Path(root).absolute()
        self.warnings = []

    def warn(self, label, reason):
        if len(self.warnings) < 80:
            self.warnings.append(_text(label + ': ' + reason))

    def read(self, path, label, anchor=None, *, optional=False):
        try:
            info = _checked(path, anchor or self.root)
            if info.st_size > MAX_INPUT_BYTES:
                raise ValueError('oversized input omitted')
            # Bound the actual read too, including growth between stat and read.
            descriptor = os.open(path, os.O_RDONLY | getattr(os, 'O_BINARY', 0) | getattr(os, 'O_NOFOLLOW', 0))
            with os.fdopen(descriptor, 'rb') as stream:
                opened = os.fstat(stream.fileno())
                if (info.st_dev, info.st_ino) != (opened.st_dev, opened.st_ino):
                    raise ValueError('candidate changed before open')
                raw = stream.read(MAX_INPUT_BYTES + 1)
            if len(raw) > MAX_INPUT_BYTES:
                raise ValueError('input grew beyond limit')
            after = _checked(path, anchor or self.root)
            if (info.st_dev, info.st_ino) != (after.st_dev, after.st_ino):
                raise ValueError('candidate replaced during read')
            encoding = 'utf-16' if raw.startswith((b'\xff\xfe', b'\xfe\xff')) else 'utf-8-sig'
            return raw.decode(encoding, errors='strict')
        except FileNotFoundError:
            if not optional:
                self.warn(label, 'missing')
        except (OSError, ValueError, UnicodeError) as error:
            self.warn(label, _text(error))
        return None

    def document(self, path, label, **kwargs):
        text = self.read(path, label, **kwargs)
        if text is not None:
            try:
                result = json.loads(text)
                if not isinstance(result, dict):
                    raise ValueError('expected JSON object')
                return result
            except (ValueError, RecursionError):
                self.warn(label, 'corrupt JSON omitted')
        return {}

    def window(self, path, label, *, head=0, tail=MAX_INPUT_BYTES, optional=False):
        """Read bounded windows from one authenticated file handle, not full logs."""
        if not 0 <= head <= 65536 or not 0 <= tail <= MAX_INPUT_BYTES:
            raise ValueError('invalid diagnostic window budget')
        try:
            info = _checked(path, self.root)
            descriptor = os.open(path, os.O_RDONLY | getattr(os, 'O_BINARY', 0) | getattr(os, 'O_NOFOLLOW', 0))
            with os.fdopen(descriptor, 'rb') as stream:
                opened = os.fstat(stream.fileno())
                if (info.st_dev, info.st_ino) != (opened.st_dev, opened.st_ino):
                    raise ValueError('candidate changed before open')
                end = opened.st_size
                first = stream.read(min(head, end))
                offset = max(0, end - tail)
                stream.seek(offset)
                last = stream.read(end - offset)
            after = _checked(path, self.root)
            if (info.st_dev, info.st_ino) != (after.st_dev, after.st_ino):
                raise ValueError('candidate replaced during read')
            if end > tail:
                self.warn(label, 'using bounded recent tail')
            return first, last, offset
        except FileNotFoundError:
            if not optional:
                self.warn(label, 'missing')
        except (OSError, ValueError) as error:
            self.warn(label, _text(error))
        return None

    def lines(self, path, label, *, optional=False):
        window = self.window(path, label, optional=optional)
        if window is None:
            return []
        _, tail, offset = window
        try:
            lines = _complete_lines(tail, offset)
        except UnicodeError:
            self.warn(label, 'invalid UTF-8 tail omitted')
            return []
        if len(lines) > 512:
            self.warn(label, 'using latest 512 complete lines')
        rows = []
        for line in lines[-512:]:
            if not line.strip():
                continue
            try:
                value = json.loads(line)
                if not isinstance(value, dict):
                    raise ValueError()
                rows.append(value)
            except (ValueError, RecursionError):
                self.warn(label, 'corrupt or partial JSONL record omitted')
        return rows

    def sessions(self):
        directory = self.root / 'analysis/runtime'
        candidates = []
        try:
            _checked(directory, self.root, directory=True)
            with os.scandir(directory) as entries:
                for count, entry in enumerate(entries):
                    if count >= MAX_SCAN:
                        self.warn('history', 'directory scan limit reached')
                        break
                    if not SESSION_NAME.fullmatch(entry.name):
                        continue
                    if len(candidates) >= MAX_CANDIDATES:
                        self.warn('history', 'candidate limit reached')
                        break
                    try:
                        info = _checked(Path(entry.path), self.root)
                        candidates.append((info.st_mtime_ns, Path(entry.path)))
                    except (OSError, ValueError):
                        self.warn('history', 'unsafe session candidate omitted')
        except FileNotFoundError:
            self.warn('history', 'no session history')
        except (OSError, ValueError):
            self.warn('history', 'unsafe/unreadable history directory')
        return [path for _, path in sorted(candidates, key=lambda row: row[0], reverse=True)[:3]]


def _complete_lines(raw, offset):
    # Conservatively drop the first fragment after a byte-window cut, even if
    # the cut happened to align with a boundary. Never parse a torn UTF-8 unit.
    if offset:
        newline = raw.find(b'\n')
        raw = raw[newline + 1:] if newline >= 0 else b''
    if raw and not raw.endswith(b'\n'):
        raw = raw[:raw.rfind(b'\n') + 1]
    return raw.decode('utf-8-sig', errors='strict').splitlines()


def _stats(values):
    valid = [number for value in values if (number := _number(value)) is not None and number >= 0]
    return {'valid_samples':len(valid), 'min':min(valid) if valid else None,
            'mean':round(math.fsum(valid) / len(valid), 3) if valid else None, 'max':max(valid) if valid else None}


def _rate(rows, time_key, count_key, scale=1e9):
    if len(rows) < 2:
        return None
    points = [(r.get(time_key), r.get(count_key)) for r in rows]
    if any(_number(t) is None or _number(c) is None for t, c in points):
        return None
    points = [(float(t), float(c)) for t, c in points]
    if any(t <= 0 or count < 0 for t, count in points):
        return None
    if any(b[0] <= a[0] or b[1] < a[1] for a, b in zip(points, points[1:])):
        return None
    if points[-1][1] == points[0][1]:
        return 0.0  # Measured unchanged counter over a real interval, not missing data.
    return round((points[-1][1]-points[0][1])*scale/(points[-1][0]-points[0][0]), 3)


def _guard_failure(data):
    """Validate the public receipt independently of the producer; no free text."""
    if not isinstance(data, dict) or type(data.get('schema')) is not int or data['schema'] != 1:
        return None
    enums = {
        'stage': ('initial', 'startup_validation', 'sample', 'sample_before', 'sample_after',
                  'certificate_assembly', 'assembly_after', 'final_identity_validation', 'final_boundary', 'unknown'),
        'cause': ('mounted_target_mismatch', 'timestamp_changed', 'registry_changed', 'read_failed', 'deadline', 'unknown'),
        'target_relation': ('controlled', 'driver', 'null', 'other', 'unknown'),
        'timestamp_state': ('unchanged', 'changed', 'unavailable'),
        'objects_registry_state': ('unchanged', 'changed', 'unavailable'),
        'names_registry_state': ('unchanged', 'changed', 'unavailable'),
    }
    out = {'schema':1}
    for key, allowed in enums.items():
        value = data.get(key)
        out[key] = value if isinstance(value, str) and value in allowed else ('unavailable' if key.endswith('_state') else 'unknown')
    for key in ('retryable', 'strict_guard_accepted', 'consistent_read'):
        out[key] = data.get(key) if type(data.get(key)) is bool else None
    attempts = data.get('attempts')
    out['attempts'] = attempts if type(attempts) is int and 0 <= attempts <= 5 else None
    elapsed = data.get('elapsed_ms')
    out['elapsed_ms'] = _number(elapsed) if type(elapsed) in (int, float) and 0 <= elapsed <= 60000 else None
    stages = ('identity', 'memory', 'header', 'target', 'timestamp', 'target_identity', 'target_field', 'unknown')
    values = data.get('mismatch_stages')
    out['mismatch_stages'] = [value if isinstance(value, str) and value in stages else 'unknown'
                              for value in values[:8]] if isinstance(values, list) else []
    value = data.get('read_error_stage')
    out['read_error_stage'] = value if isinstance(value, str) and value in stages else ('unknown' if value is not None else None)
    return out


def _events(rows):
    out = []
    for row in rows[-64:]:
        entry = _pick(row, ('sequence', 'observed_ns', 'mode', 'reason', 'phase', 'event', 'generation',
                            'world_generation', 'old_generation', 'new_generation', 'committed', 'accepted', 'error'))
        state = row.get('state_evidence', {})
        if isinstance(state, dict):
            entry['state'] = _pick(state, ('read_only', 'consistent_reread', 'input_block_count', 'player_index'))
            entry['top'] = _pick(state.get('top'), ('tag', 'cls', 'input_mode'))
        failure = _guard_failure(row.get('guard_failure'))
        if failure is not None:
            entry['guard_failure'] = failure
        out.append(entry)
    return out


def _telemetry(row):
    result = _pick(row, ('event', 'observed_ns', 'phase', 'menu_mode'))
    if isinstance(row.get('event'),str) and row['event'].startswith('path_center_'):
        result.update(_pick(row,('reason','world_generation','duration_ms','shift_x','shift_y')))
    result['native'] = _pick(row.get('native'), COUNTERS | {
        'world_handoff_phase', 'telemetry_present_calls', 'telemetry_present_tests', 'telemetry_present_failures',
        'telemetry_qpc', 'telemetry_qpc_frequency'}, numeric=True)
    result['rates'] = {key:_number(row.get('rates', {}).get(key)) if isinstance(row.get('rates'), dict) else None
        for key in RATE_FIELDS}
    result['rates'] = {key:value if value is not None and value >= 0 else None for key, value in result['rates'].items()}
    result['render'] = _pick(row.get('render'), ('width', 'height', 'client_width', 'client_height',
        'eye_width', 'eye_height', 'source_width', 'source_height'), numeric=True)
    result['xr'] = _pick(row.get('xr'), ('frame', 'predicted_time', 'session_state', 'should_render',
        'view_flags', 'head_flags', 'stereo_published', 'stereo_source'), numeric=True)
    result['source_states'] = {key:value for key, value in _pick(row.get('source_states'), ('native', 'xr')).items()
                               if value in ('fresh', 'unchanged', 'unavailable', 'counter_reset')}
    return result


def _runtime(collector, path, label):
    result = {'runtime_name':'unknown', 'runtime_version':'unknown', 'operator_runtime':'unknown',
              'simulator_operator':None}
    keys = tuple(result)
    result['probe_errors'] = []
    def adopt(data):
        for key in keys:
            value = data.get(key)
            if key == 'simulator_operator':
                if value in (0, 1, '0', '1', False, True):
                    result[key] = str(value).lower() in ('1', 'true')
            elif isinstance(value, (str, int)) and not isinstance(value, bool):
                safe = _text(value, 100).strip('"\' ')
                if re.fullmatch(r'[A-Za-z0-9_. ()-]{1,100}', safe):
                    result[key] = safe
    pattern = re.compile(r'(?:^|\s)(' + '|'.join(keys) + r')=(.*?)(?=\s+(?:' + '|'.join(keys) + r')=|$)')
    for suffix in ('.stdout.txt', '.stderr.txt'):
        window = collector.window(path.with_suffix(suffix), label + ' runtime', head=32768, tail=65536, optional=True)
        if window is None:
            continue
        first, last, offset = window
        # Decode only bounded windows. No log text is retained in the archive.
        for raw, cut in ((first, 0), (last, offset)):
            lines = _complete_lines(raw.decode('utf-8', errors='replace').encode('utf-8'), cut)
            for line in lines:
                if len(line) > 4096:
                    continue
                if line.startswith('Pose probe error: '):
                    message = _text(line[len('Pose probe error: '):])
                    if message not in result['probe_errors'] and len(result['probe_errors']) < 8:
                        result['probe_errors'].append(message)
                if line.lstrip().startswith('{'):
                    try:
                        value = json.loads(line)
                        if isinstance(value, dict):
                            adopt(value)
                    except (ValueError, RecursionError):
                        pass
                adopt(dict(pattern.findall(line)))
    return result


def _artifacts(data):
    if not isinstance(data, dict):
        return {}
    result = {key:value.lower() for key in ('native_sha256', 'xr_sha256', 'game_sha256')
              if isinstance(value := data.get(key), str) and re.fullmatch(r'[a-fA-F0-9]{64}', value)}
    build = data.get('native_build')
    if isinstance(build, str) and re.fullmatch(r'(?:native-)?build-[A-Za-z0-9_.-]{1,64}', build):
        result['native_build'] = build
    return result


def _render_size_plan(data):
    """Export only a consistent launch plan, never infer measured GPU dimensions."""
    if not isinstance(data, dict) or data.get('runtime_render_size_plan') is not True:
        return None
    scale = data.get('render_scale')
    if type(scale) not in (int, float) or not math.isfinite(scale) or not .25 <= scale <= 2:
        return None
    for key in ('recommended', 'eye', 'source'):
        values = data.get(key)
        if not isinstance(values, list) or len(values) != 2 or any(type(v) is not int or not 1 <= v <= 32768 for v in values):
            return None
    rw, rh = data['recommended']
    w, h = data['eye']
    if (not 64 <= w <= 4096 or not 64 <= h <= 8192 or data['source'] != [2*w, h]
            or w != math.ceil(rw*scale-1e-9) or h != math.ceil(rh*scale-1e-9)):
        return None
    return {'render_scale':scale, 'recommended':list(data['recommended']),
            'eye':list(data['eye']), 'source':list(data['source']),
            'evidence':'launcher_preflight_plan_not_gpu_measurement'}


def _launcher_state(collector):
    data = collector.document(collector.root / 'analysis/runtime/vr-launcher-session.json', 'launcher state')
    if data.get('schema') != 1 or data.get('kind') != 'repository_vr_launcher':
        return {'recognized':False}
    phase = data.get('phase')
    known = ('preflight', 'reserving', 'navigating_menu', 'waiting_for_level', 'session_owned',
             'cancelled_before_activation', 'finished', 'incomplete', 'failed', 'running', 'stopped', 'starting', 'stopping')
    result = {'recognized':True, 'phase':phase if phase in known else 'unknown'}
    if 'render_size_plan' in data:
        plan = _render_size_plan(data['render_size_plan'])
        if plan is None:
            collector.warn('launcher state', 'invalid render size plan omitted')
        else:
            result['render_size_plan'] = plan
    for key in ('native_activated', 'passed', 'prime_watchdog_pending'):
        if type(data.get(key)) is bool:
            result[key] = data[key]
    if _number(data.get('control_exit_code')) is not None:
        result['control_exit_code'] = _number(data['control_exit_code'])
    for key in ('error', 'prime_cleanup_error'):
        if isinstance(data.get(key), str):
            result[key] = _text(data[key])
    cleanup = data.get('prime_cleanup', {})
    result['prime_cleanup'] = {key:cleanup[key] for key in ('game_exited', 'handed_off', 'cancelled', 'entry_restore_verified')
                             if isinstance(cleanup, dict) and type(cleanup.get(key)) is bool}
    cleanup = data.get('reservation_failure_cleanup')
    if isinstance(cleanup, dict):
        result['reservation_failure_cleanup'] = _pick(cleanup, ('status', 'error'))
    monitor = data.get('tiny_output_monitor_cleanup')
    if isinstance(monitor, dict):
        joined = monitor.get('joined')
        result['tiny_output_monitor_cleanup'] = {'joined':joined if type(joined) is bool else None}
        if isinstance(monitor.get('error'),str):
            result['tiny_output_monitor_cleanup']['error'] = _text(monitor['error'])
    return result


def _startup_context(data):
    if not isinstance(data, dict):
        return {}
    result = {}
    for key in ('version', 'python_version'):
        if isinstance(data.get(key), str) and re.fullmatch(r'\d{1,3}\.\d{1,3}\.\d{1,6}', data[key]):
            result[key] = data[key]
    if isinstance(data.get('diagnostics_revision'), str) and re.fullmatch(r'startup-diagnostics-\d{1,4}', data['diagnostics_revision']):
        result['diagnostics_revision'] = data['diagnostics_revision']
    for key in ('bundled_python', 'foreground'):
        if type(data.get(key)) is bool:
            result[key] = data[key]
    if data.get('python_bits') in (32, 64):
        result['python_bits'] = data['python_bits']
    if data.get('runtime_selection') in ('system', 'override'):
        result['runtime_selection'] = data['runtime_selection']
    for key in ('steam_app_id', 'steam_game_id'):
        if isinstance(data.get(key), str) and re.fullmatch(r'\d{1,10}', data[key]):
            result[key] = data[key]
    if data.get('steam_environment_scope') == 'child_process':
        result['steam_environment_scope'] = 'child_process'
    result['artifacts'] = _artifacts(data.get('artifacts'))
    hashes = data.get('script_hashes', {})
    result['script_hashes'] = {key:value.lower() for key, value in hashes.items()
        if key in startup_diagnostics.CONTEXT_FILES and isinstance(value, str) and re.fullmatch(r'[a-fA-F0-9]{64}', value)} if isinstance(hashes, dict) else {}
    return result


def _startup_event(data):
    events = ('context', 'begin', 'complete', 'game_birth', 'failure', 'observation_unavailable',
              'owner_timeout', 'owner_result', 'owner_failure')
    result = {'event':data.get('event') if data.get('event') in events else 'unknown',
              'phase':data.get('phase') if data.get('phase') in startup_diagnostics.PHASES else 'unknown'}
    result.update(_pick(data, ('elapsed_ms', 'duration_ms', 'scheduled_tests', 'returncode',
        'remote_thread_exit_code', 'loaded_module_count', 'entry_checks', 'graphics_hooks', 'winerror'), numeric=True))
    for key in ('dll_loaded', 'entry_checks_passed'):
        if type(data.get(key)) is bool:
            result[key] = data[key]
    stamp = data.get('timestamp_utc')
    if isinstance(stamp, str) and re.fullmatch(r'[0-9T:+.Z-]{19,40}', stamp):
        result['timestamp_utc'] = stamp
    if isinstance(data.get('error'), str):
        result['error'] = _text(data['error'], 1500)
    if isinstance(data.get('exception_type'), str) and re.fullmatch(r'[A-Za-z][A-Za-z0-9_]{0,79}', data['exception_type']):
        result['exception_type'] = data['exception_type']
    names = data.get('loaded_modules')
    if isinstance(names, list):
        result['loaded_modules'] = [name for name in names[:128] if isinstance(name, str)
            and re.fullmatch(r'[A-Za-z0-9_.-]{1,100}\.(?:dll|exe)', name, re.I)]
    process = data.get('process')
    if isinstance(process, dict):
        state = process.get('state')
        result['process'] = {'state':state if state in ('running', 'exited', 'identity_changed', 'unavailable', 'observed') else 'unknown'}
        result['process'].update(_pick(process, ('pid', 'exit_code', 'winerror'), numeric=True))
        if type(process.get('same_identity')) is bool:
            result['process']['same_identity'] = process['same_identity']
        if isinstance(process.get('exit_code_hex'), str) and re.fullmatch(r'0x[0-9A-Fa-f]{8}', process['exit_code_hex']):
            result['process']['exit_code_hex'] = process['exit_code_hex']
    status = data.get('prime_status')
    if isinstance(status, dict):
        state = status.get('state')
        result['prime_status'] = {'state':state if state in ('captured', 'empty', 'missing', 'stale', 'pid_mismatch', 'unavailable') else 'unknown'}
        result['prime_status'].update({key:status[key] for key in ('pid_verified', 'setup_failed', 'truncated') if type(status.get(key)) is bool})
        hooks = status.get('hooks')
        if isinstance(hooks, list):
            result['prime_status']['hooks'] = [{key:value for key in ('create', 'queue')
                if isinstance(value := row.get(key), str) and re.fullmatch(r'MH_[A-Z_]{1,64}', value)}
                for row in hooks[:16] if isinstance(row, dict)]
        rejection = status.get('graphics_rejection')
        if isinstance(rejection, dict):
            result['prime_status']['graphics_rejection'] = _pick(rejection, ('reason', 'policy_reason'), numeric=True)
    return result


def _startup_attempts(collector):
    directory = collector.root / 'analysis/runtime'
    pattern = re.compile(r'vr-launcher-session\.reserve\.log\.([a-f0-9]{32})\.startup\.jsonl\Z')
    candidates = []
    try:
        _checked(directory, collector.root, directory=True)
        with os.scandir(directory) as entries:
            for count, entry in enumerate(entries):
                if count >= MAX_SCAN or len(candidates) >= MAX_CANDIDATES:
                    collector.warn('startup', 'directory/candidate limit reached')
                    break
                match = pattern.fullmatch(entry.name)
                if not match:
                    continue
                try:
                    path = Path(entry.path)
                    info = _checked(path, collector.root)
                    candidates.append((info.st_mtime_ns, path, match[1]))
                except (OSError, ValueError):
                    collector.warn('startup', 'unsafe startup candidate omitted')
    except (OSError, ValueError):
        collector.warn('startup', 'startup history unavailable')
    attempts = []
    for _, path, attempt in sorted(candidates, key=lambda row:row[0], reverse=True)[:3]:
        text = collector.read(path, 'startup trace')
        rows = []
        for line in (text or '').splitlines(keepends=True):
            if not line.endswith('\n') or len(line) > 32768:
                continue
            try:
                row = json.loads(line)
                if isinstance(row, dict) and type(row.get('schema')) is int and row['schema'] == 1 and row.get('kind') == 'startup_diagnostic':
                    rows.append(row)
            except (ValueError, RecursionError):
                pass
        if not rows or rows[0].get('event') != 'context' or rows[0].get('attempt_id') != attempt:
            collector.warn('startup', 'unrecognized startup trace omitted')
            continue
        clean = dict(attempt_id=attempt[:12], context=_startup_context(rows[0].get('context')),
                     events=[_startup_event(row) for row in rows[1:][-96:]])
        birth_path = path.with_name(path.name[:-len('.startup.jsonl')] + '.birth.json')
        birth = collector.document(birth_path, 'startup birth', optional=True)
        if birth.get('kind') == 'reservation_birth' and birth.get('schema') == 1:
            clean['birth'] = {key:birth[key] for key in ('injection_attempted', 'prime_attempted') if type(birth.get(key)) is bool}
            identity = birth.get('game_identity')
            pid = identity.get('pid') if isinstance(identity, dict) else None
            if type(pid) is int and 0 < pid <= 0xffffffff:
                clean['birth']['pid'] = pid
                launch = collector.document(directory / f'background-launch-{pid}.json', 'background launch', optional=True)
                if launch.get('pid') == pid and launch.get('creation_time') == identity.get('creation_time'):
                    clean['background_launch'] = _pick(launch, ('window_startup_policy', 'background_input_requested', 'test_windows_behind_requested'))
                    clean['background_launch'].update(_startup_context(launch))
        attempts.append(clean)
    return dict(schema=1, kind='startup_diagnostics_summary', attempts=attempts,
                history_available=bool(attempts),
                limitation='Remote thread exit code and game process exit code are separate observations; neither alone establishes a Steam restart or crash cause.')


def _startup_text(summary):
    lines = ['GearsVR startup diagnostics', summary['limitation'], '']
    if not summary['attempts']:
        lines.append('No startup trace from the enhanced logger is available. Reproduce with the updated launcher and export again.')
    for attempt in summary['attempts']:
        context = attempt['context']
        lines.extend(['Attempt ' + attempt['attempt_id'],
            'Version: ' + context.get('version', 'unknown') + '; diagnostics: ' + context.get('diagnostics_revision', 'unknown'),
            'Python: ' + context.get('python_version', 'unknown') + '; bundled: ' + str(context.get('bundled_python', 'unknown'))])
        for event in attempt['events']:
            if event['event'] in ('begin', 'complete', 'failure', 'owner_timeout'):
                line = event['phase'] + ': ' + event['event']
                for key in ('elapsed_ms', 'duration_ms', 'remote_thread_exit_code'):
                    if key in event:
                        line += '; ' + key + '=' + str(event[key])
                lines.append(line)
            if 'process' in event:
                process = event['process']
                lines.append('Game process: ' + process['state'] + '; PID=' + str(process.get('pid', 'unknown')) +
                    '; exit_code=' + str(process.get('exit_code', 'unavailable')) + ' ' + process.get('exit_code_hex', ''))
        errors = [event['error'] for event in attempt['events'] if 'error' in event]
        lines.extend('Error: ' + error for error in errors[-4:])
        lines.append('')
    return '\n'.join(lines) + '\n'


def _csv(collector, path, label):
    summary = {'xr_prediction_hz': None, 'stereo_publication_hz': None,
               'limitation': 'XR prediction/publication cadence is not measured headset display FPS.'}
    summary['rate_statistics'] = {key:_stats([]) for key in ('xr_prediction_hz', 'stereo_publication_hz')}
    window = collector.window(path, label, head=4096, optional=True)
    if window is None:
        collector.warn(label, 'rates unavailable')
        return summary, None
    try:
        first, tail, offset = window
        if b'\n' not in first:
            raise ValueError('missing bounded CSV header')
        header = next(csv.reader([first.split(b'\n', 1)[0].decode('utf-8-sig')]))
        data_lines = _complete_lines(tail, offset)
        if not offset:
            data_lines = data_lines[1:]  # The header is only present in a whole-file tail.
        data = list(csv.reader(data_lines[-512:]))
        if not header or len(header) != len(set(header)) or set(header) - CSV_FIELDS:
            raise ValueError('CSV header rejected')
        performance = [key for key in CSV_PERFORMANCE_FIELDS if key in header]
        if not performance:
            raise ValueError('no performance columns')
        rows = []
        for values in data:
            if len(values) != len(header) or any(_number(x) is None for x in values):
                raise ValueError('CSV contains nonnumeric or invalid data')
            rows.append(dict(zip(header, (_number(x) for x in values))))
        summary.update(rows=len(rows), xr_prediction_hz=_rate(rows, 'predicted_time', 'frame'))
        published = []
        for row in rows:
            if row.get('stereo_published') == 1 and (not published or row.get('stereo_sequence') != published[-1].get('stereo_sequence')):
                published.append(row)
        summary['stereo_publication_hz'] = _rate(published, 'published_ns', 'stereo_sequence')
        for key, samples, time_key, count_key in (
                ('xr_prediction_hz', rows, 'predicted_time', 'frame'),
                ('stereo_publication_hz', published, 'published_ns', 'stereo_sequence')):
            summary['rate_statistics'][key] = _stats(_rate([a, b], time_key, count_key)
                                                   for a, b in zip(samples, samples[1:]))
        sanitized = io.StringIO()
        writer = csv.DictWriter(sanitized, fieldnames=performance, extrasaction='ignore', lineterminator='\n')
        writer.writeheader()
        writer.writerows(rows)
        return summary, sanitized.getvalue()
    except (ValueError, UnicodeError, csv.Error):
        collector.warn(label, 'unsafe/corrupt CSV omitted; rates unavailable')
        return summary, None


def _session(collector, path, label):
    data = collector.document(path, label)
    if data.get('schema') != 1 or data.get('kind') != 'continuous_strict_stereo':
        collector.warn(label, 'unrecognized owned session metadata; no attachments read')
        return {'recognized': False}, None
    out = _pick(data, SESSION_FIELDS | {k + '_enabled' for k in FLAGS})
    for key in list(out):
        if key == 'phase':
            if out[key] not in ('initializing', 'preflight', 'starting', 'running', 'stopping', 'stopped', 'failed', 'incomplete', 'cancelled'):
                out[key] = 'unknown'
        elif not isinstance(out[key], (bool, int, float)):
            del out[key]
    out['recognized'] = True
    out['diagnostics_artifacts'] = _artifacts(data.get('diagnostics_artifacts'))
    out['errors'] = {k:_text(data[k]) for k in ERROR_FIELDS if isinstance(data.get(k), str)}
    out['native_counters'] = _pick(data.get('native_status'), COUNTERS, numeric=True)
    out['lifecycle'] = _pick(data.get('session_exit_evidence'), ('last_state', 'stopping_observed', 'end_session_succeeded',
        'terminal_exiting_observed', 'terminal_exit_without_end', 'loss_observed', 'objects_destroyed', 'shutdown_completed'))
    final = data.get('continuous_session_evidence', {})
    out['final_projection'] = {'average_submissions_per_second': None, 'maximum_submission_gap_ms': None}
    if isinstance(final, dict) and final.get('continuous_session_evidence') is True:
        out['final_projection'].update(_pick(final, ('submitted_frames', 'scene_copies', 'span_seconds',
            'orderly_stop', 'structural_failure', 'shutdown_completed', 'error_reported')))
        count, span = _number(final.get('submitted_frames')), _number(final.get('span_seconds'))
        if count is not None and count >= 2 and span is not None and span > 0:
            out['final_projection']['average_submissions_per_second'] = round((count - 1) / span, 3)
            out['final_projection']['maximum_submission_gap_ms'] = _number(final.get('maximum_submission_gap_ms'))
        out['final_projection']['definition'] = '(submitted_frames - 1) / final span_seconds; final session evidence only'
    timings = data.get('startup_timings')
    if isinstance(timings, dict):
        out['startup_timings_ms'] = {key:_pick(timings.get(key), ('elapsed_ms', 'since_runner_start_ms'), numeric=True)
            for key in ('xr_preflight', 'camera_ready', 'native_prepare', 'native_running', 'first_stereo_submission') if key in timings}
    # Derive all attachment paths locally; never follow a report's paths/commands.
    out['menu_transitions'] = _events(collector.lines(path.with_suffix('.menu.jsonl'), label + ' menu'))
    out['world_transitions'] = _events(collector.lines(path.with_suffix('.world.jsonl'), label + ' world'))
    telemetry = []
    for suffix in ('.telemetry.2.jsonl', '.telemetry.1.jsonl', '.telemetry.jsonl'):
        telemetry.extend(collector.lines(path.with_suffix(suffix), label + ' telemetry', optional=suffix != '.telemetry.jsonl'))
    recent = [_telemetry(row) for row in telemetry]
    out['telemetry'] = recent[-64:]
    out['rate_statistics'] = {key:_stats(row['rates'][key] for row in recent) for key in RATE_FIELDS}
    out['rate_statistics_scope'] = 'bounded recent complete rows from up to three telemetry files; arithmetic sample mean'
    out['runtime'] = _runtime(collector, path, label)
    out['actual_gpu_dimensions'] = None
    for row in out['telemetry']:
        render = row['render']
        if all(isinstance(render.get(key), (int, float)) and render[key] > 0 for key in ('width', 'height')):
            out['actual_gpu_dimensions'] = {key:render[key] for key in ('width', 'height')}
    if out['actual_gpu_dimensions'] is None:
        counters = out['native_counters']
        if all(counters.get(key, 0) > 0 for key in ('gpu_width', 'gpu_height')):
            out['actual_gpu_dimensions'] = {'width':counters['gpu_width'], 'height':counters['gpu_height']}
    out['rate_limitations'] = ('Game present rate counts observed bound top-level DXGI Present calls excluding '
        'DXGI_PRESENT_TEST; it is not physical display or engine update FPS. Stereo-pair publication and XR loop '
        'rates do not prove headset display FPS. Missing metrics are null/unavailable.')
    out['csv'], raw = _csv(collector, path.with_suffix('.csv'), label + ' CSV')
    return out, raw


def _system():
    if os.name != 'nt':
        raise OSError('Windows system inventory unavailable')
    script = "$ErrorActionPreference='Stop';[Console]::OutputEncoding=New-Object System.Text.UTF8Encoding;[ordered]@{cpu=@(Get-CimInstance Win32_Processor | Select-Object -First 4 | ForEach-Object {@{name=$_.Name;cores=$_.NumberOfCores;logical_processors=$_.NumberOfLogicalProcessors}});gpu=@(Get-CimInstance Win32_VideoController | Select-Object -First 8 | ForEach-Object {@{name=$_.Name;driver_version=$_.DriverVersion}});physical_ram_bytes=(Get-CimInstance Win32_ComputerSystem).TotalPhysicalMemory;os=(Get-CimInstance Win32_OperatingSystem | ForEach-Object {@{caption=$_.Caption;version=$_.Version;build=$_.BuildNumber;architecture=$_.OSArchitecture}})}|ConvertTo-Json -Depth 4 -Compress"
    powershell = Path(os.environ.get('SystemRoot', 'C:/Windows')) / 'System32/WindowsPowerShell/v1.0/powershell.exe'
    result = subprocess.run([str(powershell), '-NoProfile', '-NonInteractive', '-Command', script],
        capture_output=True, timeout=8, creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
    if result.returncode or len(result.stdout) > 16384:
        raise OSError('system inventory failed')
    return json.loads(result.stdout.decode('utf-8-sig', errors='replace'))


def _system_summary(data):
    if not isinstance(data, dict):
        raise ValueError('invalid system inventory')
    out = {'available': True, 'physical_ram_bytes':_number(data.get('physical_ram_bytes')),
           'os':_pick(data.get('os'), ('caption', 'version', 'build', 'architecture'))}
    for key, fields in (('cpu', ('name', 'cores', 'logical_processors')), ('gpu', ('name', 'driver_version'))):
        rows = data.get(key, [])
        if isinstance(rows, dict):
            rows = [rows]
        out[key] = [_pick(row, fields) for row in rows[:8]] if isinstance(rows, list) else []
        if key == 'cpu':
            for row in out[key]:
                for field in ('cores', 'logical_processors'):
                    if field in row:
                        row[field] = _number(row[field])
    if not out['cpu'] and not out['gpu'] and not out['os'] and out['physical_ram_bytes'] is None:
        raise ValueError('empty system inventory')
    return out


def _settings(collector, directory):
    result = []
    try:
        _checked(directory, Path(directory).anchor, directory=True)
        with os.scandir(directory) as scan:
            files = []
            for index, entry in enumerate(scan):
                if index >= 64:
                    collector.warn('game settings', 'directory limit reached')
                    break
                if entry.name.lower().endswith('engineusersettings.ini'):
                    files.append(Path(entry.path))
        for path in sorted(files)[:3]:
            text = collector.read(path, 'game settings', anchor=directory)
            if text is None:
                continue
            values = {}
            for line in text.splitlines():
                if '=' not in line or line.lstrip().startswith((';', '#')):
                    continue
                key, value = (x.strip() for x in line.split('=', 1))
                if key.lower() in SCALAR_SETTINGS:
                    if value.lower() in ('true', 'false'):
                        values[key] = value.lower() == 'true'
                    elif _number(value) is not None:
                        values[key] = _number(value)
                elif key.lower() in TEXTURE_GROUPS:
                    fields = dict(re.findall(r'\b(MinLODSize|MaxLODSize|LODBias|NumStreamedMips)\s*=\s*(-?\d+)\b', value))
                    if fields:
                        values[key] = {k:_number(v) for k, v in fields.items() if _number(v) is not None}
            result.append(values)
        if not files:
            collector.warn('game settings', 'EngineUserSettings unavailable')
    except (OSError, ValueError):
        collector.warn('game settings', 'settings directory unavailable or unsafe')
    return result


def export_bundle(root, output, overwrite=False, *, system_reader=None, settings_root=None):
    """Write a bounded curated ZIP atomically. Never open a game process."""
    if type(overwrite) is not bool:
        raise ValueError('overwrite must be explicitly boolean')
    collector = Collector(root)
    output = Path(output).absolute()
    if output.suffix.lower() != '.zip':
        raise ValueError('diagnostics output must have a .zip extension')
    _checked(output.parent, Path(output.anchor), directory=True)
    if output.exists() or output.is_symlink():
        _checked(output, output.parent)
        if not overwrite:
            raise FileExistsError('diagnostics output already exists')
    entries = {}
    sessions = collector.sessions()
    for index, path in enumerate(sessions, 1):
        label = 'session-' + str(index)
        summary, raw_csv = _session(collector, path, label)
        entries['sessions/' + label + '.json'] = json.dumps(summary, indent=2).encode('utf-8')
        if raw_csv:
            entries['sessions/' + label + '.csv'] = raw_csv.encode('utf-8')
    configuration = {}
    for name in ('config', 'manifest'):
        raw = collector.document(collector.root / ('tools/vr-launcher/' + name + '.json'), name)
        clean = _pick(raw, CONFIG_FIELDS)
        for key in list(clean):
            if key == 'launcher_language':
                if clean[key] not in ('en', 'zh-CN'):
                    del clean[key]
            elif key in FLAGS:
                if type(clean[key]) is not bool:
                    del clean[key]
            elif not isinstance(clean[key], (int, float)) or isinstance(clean[key], bool):
                del clean[key]
        for key in ('native_sha256', 'xr_sha256', 'game_sha256'):
            if isinstance(raw.get(key), str) and re.fullmatch('[a-fA-F0-9]{64}', raw[key]):
                clean[key] = raw[key].lower()
        native_build = str(raw.get('native_build', '')).replace('\\', '/').rsplit('/', 1)[-1]
        if re.fullmatch(r'build-[A-Za-z0-9-]{1,64}', native_build):
            clean['native_build'] = native_build
        configuration[name] = clean
    try:
        system = _system_summary((system_reader or _system)())
    except Exception as error:
        collector.warn('system', 'inventory unavailable (' + type(error).__name__ + ')')
        system = {'available': False}
    settings_root = Path(settings_root) if settings_root is not None else Path(os.environ.get('LOCALAPPDATA', str(Path.home() / 'AppData/Local'))) / 'Coalition/WarGame/Config'
    settings = _settings(collector, settings_root)
    startup = _startup_attempts(collector)
    entries['startup-attempts.json'] = json.dumps(startup, indent=2).encode('utf-8')
    entries['startup-summary.txt'] = _startup_text(startup).encode('utf-8')
    entries['build-info.json'] = json.dumps(_startup_context(startup_diagnostics.context(collector.root, {}, include_hashes=False)), indent=2).encode('utf-8')
    entries['configuration.json'] = json.dumps(configuration, indent=2).encode('utf-8')
    entries['launcher-state.json'] = json.dumps(_launcher_state(collector), indent=2).encode('utf-8')
    entries['system.json'] = json.dumps(system, indent=2).encode('utf-8')
    entries['game-settings.json'] = json.dumps(settings, indent=2).encode('utf-8')
    entries['README.txt'] = ("Gears VR local diagnostics\nCurated summaries from up to three recent sessions. Failed/incomplete/live sessions are retained.\n"
        "Start with startup-summary.txt for launch failures; startup-attempts.json retains the latest three startup timelines and recorded component hashes.\n"
        "No game binaries, native instruction logs, saves, profiles, screenshots or analysis databases are included.\n"
        "Missing rates are null/unavailable. XR prediction/publication cadence is not headset display FPS.\n"
        "Session files may change while a live session is being exported; this is not an atomic game snapshot.\n").encode()
    entries['ISSUE-REPORT.txt'] = b'Gears VR feedback\nWhat happened?\nExpected behavior:\nSteps to reproduce / chapter and checkpoint:\nHeadset and connection method:\nApproximate time / selected session:\n'
    # Reserve room for the small manifest; omit optional CSV before exceeding the budget.
    for name in list(entries):
        if sum(map(len, entries.values())) <= MAX_ARCHIVE_BYTES - 32768:
            break
        if name.endswith('.csv'):
            entries.pop(name)
            collector.warn('archive', 'numeric CSV omitted for size budget')
    manifest = {'schema':1, 'kind':'gears_vr_diagnostics', 'created_utc':dt.datetime.now(dt.timezone.utc).isoformat(),
                'sessions':len(sessions), 'warnings':collector.warnings, 'files':sorted([*entries, 'manifest.json'])}
    entries['manifest.json'] = json.dumps(manifest, indent=2).encode('utf-8')
    if len(entries) > MAX_ARCHIVE_FILES or sum(map(len, entries.values())) > MAX_ARCHIVE_BYTES:
        raise ValueError('diagnostics archive budget exceeded')
    descriptor, temporary = tempfile.mkstemp(prefix='.gears-diagnostics-', suffix='.tmp', dir=output.parent)
    os.close(descriptor)
    try:
        with zipfile.ZipFile(temporary, 'w', compression=zipfile.ZIP_DEFLATED) as archive:
            for name, data in entries.items():
                archive.writestr(name, data)
        _checked(output.parent, Path(output.anchor), directory=True)
        if overwrite:
            if output.exists() or output.is_symlink():
                _checked(output, output.parent)
            os.replace(temporary, output)
        else:
            os.link(temporary, output)  # Atomic create-new: a raced output is never replaced.
            os.unlink(temporary)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)
    return {'diagnostics_exported':True, 'output':str(output), 'files':len(entries), 'warnings':collector.warnings}
