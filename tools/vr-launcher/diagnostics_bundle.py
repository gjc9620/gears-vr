"""Local support diagnostics; no game handles, network, memory dumps or binaries."""
import csv
import datetime as dt
import hashlib
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
MAX_ARCHIVE_BYTES = 32 * 1024 * 1024
MAX_ARCHIVE_FILES = 32
MAX_DEBUG_INPUT_BYTES = 4 * 1024 * 1024
MAX_DEBUG_RECORDS = 8192
EXPORT_REVISION = 'support-evidence-6'
MAX_SCAN = 200000
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
CSV_DIAGNOSTIC_FIELDS = CSV_PERFORMANCE_FIELDS + tuple(('observed_monotonic_ns xr_distinct_stereo_submissions '
    'xr_repeated_stereo_submissions xr_invalid_submission_keys transport_acquired transport_acknowledged '
    'transport_discarded xr_copied_stereo_images xr_submitted_generation xr_submitted_sequence xr_submitted_ticket '
    'xr_copied_slot transport_ready_deferred xr_local_fence_wait_count xr_local_fence_wait_ns '
    'xr_eye0_wait_count xr_eye0_wait_ns xr_eye1_wait_count xr_eye1_wait_ns xr_wait_frame_count '
    'xr_wait_frame_ns xr_end_frame_count xr_end_frame_ns').split())
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

    def read(self, path, label, anchor=None, *, optional=False, max_bytes=MAX_INPUT_BYTES):
        try:
            info = _checked(path, anchor or self.root)
            if info.st_size > max_bytes:
                raise ValueError('oversized input omitted')
            # Bound the actual read too, including growth between stat and read.
            descriptor = os.open(path, os.O_RDONLY | getattr(os, 'O_BINARY', 0) | getattr(os, 'O_NOFOLLOW', 0))
            with os.fdopen(descriptor, 'rb') as stream:
                opened = os.fstat(stream.fileno())
                if (info.st_dev, info.st_ino) != (opened.st_dev, opened.st_ino):
                    raise ValueError('candidate changed before open')
                raw = stream.read(max_bytes + 1)
            if len(raw) > max_bytes:
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
        if not 0 <= head <= max(MAX_INPUT_BYTES, MAX_DEBUG_INPUT_BYTES) or not 0 <= tail <= max(MAX_INPUT_BYTES, MAX_DEBUG_INPUT_BYTES):
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
                    try:
                        info = _checked(Path(entry.path), self.root)
                        candidates.append((info.st_mtime_ns, Path(entry.path)))
                        if len(candidates) > MAX_CANDIDATES:
                            candidates = sorted(candidates, key=lambda row:row[0], reverse=True)[:MAX_CANDIDATES]
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


def _source_states(data):
    if not isinstance(data, dict):
        return {}
    allowed = ('first_sample', 'fresh', 'unchanged', 'unavailable', 'counter_reset',
               'identity_mismatch_or_unavailable')
    return {key:data[key] for key in ('native', 'xr', 'render')
            if isinstance(data.get(key), str) and data[key] in allowed}


def _source_errors(data):
    if not isinstance(data, dict):
        return {}
    allowed = ('OSError', 'IOError', 'FileNotFoundError', 'PermissionError', 'ValueError',
               'UnicodeError', 'UnicodeDecodeError', 'UnicodeEncodeError', 'Error')
    return {key:data[key] if data[key] in allowed else 'unknown_exception_type'
            for key in ('xr', 'render') if isinstance(data.get(key), str)}


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
    result['source_states'] = _source_states(row.get('source_states'))
    result['source_errors'] = _source_errors(row.get('source_errors'))
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
                if re.fullmatch(r'[A-Za-z0-9_. ()/+:-]{1,100}', safe):
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
        pattern = r'\d{1,3}\.\d{1,3}\.\d{1,6}' + (r'(?:-(?:dev|rc\d{1,3}))?' if key == 'version' else '')
        if isinstance(data.get(key), str) and re.fullmatch(pattern, data[key]):
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
                if count >= MAX_SCAN:
                    collector.warn('startup', 'directory scan limit reached')
                    break
                match = pattern.fullmatch(entry.name)
                if not match:
                    continue
                try:
                    path = Path(entry.path)
                    info = _checked(path, collector.root)
                    candidates.append((info.st_mtime_ns, path, match[1]))
                    if len(candidates) > MAX_CANDIDATES:
                        candidates = sorted(candidates, key=lambda row:row[0], reverse=True)[:MAX_CANDIDATES]
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
        first=collector.document(path.with_name(path.name+'.first-failure.json'),'startup first failure',optional=True)
        if first.get('schema')==1 and first.get('kind')=='startup_diagnostic' and first.get('event')=='failure' and first.get('attempt_id')==attempt:
            clean['first_failure']=_startup_event(first)
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
        if attempt.get('first_failure',{}).get('error'):
            lines.append('First failure: '+attempt['first_failure']['error'])
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
        if (not header or len(header) != len(set(header)) or
                any(not re.fullmatch(r'[A-Za-z][A-Za-z0-9_]{0,95}', key) or
                    re.search(r'password|secret|token|account|username|email|steamid', key, re.I) for key in header)):
            raise ValueError('CSV header rejected')
        performance = [key for key in CSV_DIAGNOSTIC_FIELDS if key in header]
        if not performance:
            raise ValueError('no performance columns')
        if set(header) - CSV_FIELDS - set(CSV_DIAGNOSTIC_FIELDS):
            collector.warn(label, 'unknown CSV columns ignored; recognized numeric columns retained')
        rows = []
        invalid = 0
        for values in data:
            if len(values) != len(header):
                invalid += 1
                continue
            row = dict(zip(header, values))
            selected = {key:_number(row[key]) for key in performance}
            if any(value is None for value in selected.values()):
                invalid += 1
                continue
            rows.append(selected)
        summary['invalid_rows'] = invalid
        if invalid:
            collector.warn(label, 'invalid CSV rows omitted; valid numeric rows retained')
        if not rows:
            raise ValueError('no valid numeric samples')
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


_PRIVATE_FIELD = re.compile(r'(?:^|_)(?:password|secret|token|api_key|username|account|steamid|email|serial|'
    r'hostname|machine|raw|bytes|dump|instruction|disassembly|registers|'
    r'address|pointer|ptr|rva|caller|callee|vtable|trampoline|module_base)(?:_|$)', re.I)
_POINTER_FIELDS = set(('target queue buffer resource family renderer view canvas entry base self node '
    'destination original replacement state_original state_temporary local_player controller pawn weapon '
    'camera actor component world handle path image command commands filename directory').split())
_DEBUG_TEXT_FIELDS = set(('error message exception_type runtime_name runtime_version operator_runtime '
    'native_prepare_stage native_preparation_waiting_for_camera reason last_reason stop_reason phase stage '
    'event clock kind record cause error_reported stop_requested_by xr_cleanup_actions scope last_session_state xr_preflight_diagnostic_error').split())
_DEBUG_SUFFIXES = ('.native-start.log', '.session-status.txt', '.native-status.txt', '.stdout.txt', '.stderr.txt', '.csv',
    '.menu.jsonl', '.world.jsonl', '.telemetry.2.jsonl', '.telemetry.1.jsonl', '.telemetry.jsonl',
    '.support.jsonl.1', '.support.jsonl', '.first-failure.json', '.native-live-status.txt', '.native-startup.jsonl', '.thread-waits.jsonl', '.xr-preflight.jsonl')


def _support_record(data, depth=0):
    if not isinstance(data, dict) or data.get('support_diagnostics') is not True or data.get('schema') != 1 or depth>2:
        return {}
    roles=('controller','native','xr','game','environment','observer')
    events=('context','system','adapter','adapter_required','adapter_comparison','graphics_event','graphics_snapshot',
            'software_inventory','loaded_modules','process_state','first_failure','pipeline_progress',
            'observer_error','native_snapshot','native_stop_observed','child_exit','startup_stage','startup_wait','thread_wait','startup_trace_gap',
            'xr_focus_capability','xr_focus_event','xr_focus_snapshot')
    out=dict(support_diagnostics=True,schema=1,role=data.get('role') if data.get('role') in roles else 'unknown',
             event=data.get('event') if data.get('event') in events else 'unknown')
    numeric=set(('pid creation_time observed_qpc qpc_frequency observed_ns event_qpc game_pid game_creation_time '
        'adapter_luid_high adapter_luid_low vendor_id device_id maximum_width maximum_height '
        'hresult error_code device_removed_reason winerror event_code chain_id generation width height format '
        'buffer_count flags accepted source_line snapshot_index present_entries present1_entries present_failures '
        'overflow lock_skips write_failures presents tests unbound resizing rotations exit_code total_modules_observed '
        'thread_id sequence crt_errno skipped trace_io_error elapsed_ms thread_creation_time kernel_100ns user_100ns '
        'thread_cycles cpu_qpc node_count cpu_winerror object_type object_status context_switches '
        'thread_cpu_delta_100ns sample_interval_ms previous_sequence '
        'event_type session_state previous_session_state reported_session_state xr_event_time presence_state events_lost omitted frames focused_frames visible_frames events_observed').split())
    boolean=set(('available adapter_match binding_accepted first_observed_failure software_adapter position_tracking_supported orientation_tracking_supported '
                 'truncated gpu_query_available succeeded cpu_available wait_chain_available cycle_detected '
                 'identity_verified thread_alive in_game_process extension_advertised presence_extension_enabled '
                 'presence_system_supported session_matches should_render final').split())
    texts=set(('system_name adapter_name module_name file_version component version source scope category '
               'message action phase stage reason state exception_type connection_method error_domain ordering').split())
    for key in numeric:
        if key in data:
            value=_number(data[key])
            if value is not None:out[key]=value
            elif data[key] is None:out[key]=None
    for key in boolean:
        if type(data.get(key)) is bool:out[key]=data[key]
        elif key=='cycle_detected' and key in data and data[key] is None:out[key]=None
    for key in texts:
        if isinstance(data.get(key),str):out[key]=_debug_text(data[key])
        elif key=='file_version' and key in data and data[key] is None:out[key]=None
    for key in ('modules','software','chains','wait_nodes'):
        if isinstance(data.get(key),list):
            out[key]=[]
            for row in data[key][:(16 if key=='wait_nodes' else 512)]:
                if not isinstance(row,dict):continue
                clean=_support_record(dict(row,support_diagnostics=True,schema=1,role=out['role'],event=out['event']),depth+1)
                out[key].append({k:v for k,v in clean.items() if k not in ('schema','support_diagnostics','role','event')})
    for key in ('counters','unchanged_ms'):
        if isinstance(data.get(key),dict):
            allowed=COUNTERS | set(('telemetry_present_calls telemetry_present_failures support_capture_attempts '
                'support_capture_skipped support_publisher_state support_publisher_busy support_frame_unbound '
                'support_frame_changed owned_consumed owned_published owned_error owned_pipeline_rejected '
                'owned_list_failure_hr owned_execute_failure_count').split())
            out[key]=_pick(data[key],allowed,numeric=True)
    if isinstance(data.get('runtime_event'),dict):out['runtime_event']=_support_record(data['runtime_event'],depth+1)
    known=numeric|boolean|texts|{'schema','support_diagnostics','role','event','modules','software','chains','wait_nodes','counters','unchanged_ms','runtime_event'}
    unknown=[key for key in data if key not in known and isinstance(key,str) and re.fullmatch(r'[A-Za-z][A-Za-z0-9_]{0,79}',key) and not _PRIVATE_FIELD.search(key)]
    if unknown:out['unrecognized_field_names']=unknown[:32]
    return out


def _debug_text(value):
    # Preserve 32-bit HRESULT/Win32 error codes, while retaining the ordinary
    # path/account/credential filtering. Native pointers/instruction bytes do
    # not enter this free-text channel.
    codes = []
    def code(match):
        context=str(value)[max(0,match.start()-40):match.start()]
        if not re.search(r'\b(?:HRESULT|hr|winerror|error_code|result)\s*[:=]?\s*$',context,re.I):
            return match[0]
        codes.append(match[0])
        return 'DIAGNOSTICERRORCODE' + str(len(codes) - 1)
    text = re.sub(r'\b0x[0-9a-fA-F]{1,8}\b', code, str(value))
    text = re.sub(r"(?i)Command\s+.+?(?=\s+(?:returned non-zero|timed out))", 'Command <COMMAND>', text)
    text = _text(text, 1800)
    for index, value in enumerate(codes):
        text = text.replace('DIAGNOSTICERRORCODE' + str(index), value)
    return text


def _debug_clean(value, key='', depth=0):
    """Diagnostic scalar/tree projection, never a raw JSON/native dump."""
    name = key.lower()
    if isinstance(value,dict) and value.get('support_diagnostics') is True:
        return _support_record(value)
    if name == 'guard_failure':
        return _guard_failure(value)
    if name == 'source_states':
        return _source_states(value)
    if name == 'source_errors':
        return _source_errors(value)
    if depth > 7 or _PRIVATE_FIELD.search(name) or name in _POINTER_FIELDS:
        return None
    if name.endswith(('_path', '_file', '_filename', '_directory', '_command', '_commands', '_command_line',
                      '_pointer', '_address', '_handle', '_base', '_resource', '_buffer', '_queue')):
        return None
    if re.search(r'(?:^|_)(?:head|eye[01]?|left|right)_(?:x|y|z|qx|qy|qz|qw)$', name):
        return None
    if name.endswith(('_head', '_eye0', '_eye1', '_fov0', '_fov1', '_pose', '_location', '_matrix')):
        return None
    if isinstance(value, bool):
        return value
    if isinstance(value, (int, float)):
        return _number(value)
    if isinstance(value, str):
        if name.endswith('sha256') and re.fullmatch(r'[a-fA-F0-9]{64}', value):
            return value.lower()
        if re.fullmatch(r'[0-9a-fA-F]{8}',value) and name.endswith(('_hr','_hresult','_error_code')):
            return '0x'+value.lower()
        if re.fullmatch(r'-?\d+(?:\.\d+)?(?:[eE][+-]?\d+)?', value):
            return _number(value)
        if re.fullmatch(r'0x[0-9a-fA-F]{1,8}', value) and re.search(r'(?:result|error|hresult|hr|reason|code)$', name):
            return value.lower()
        if re.fullmatch(r'-?\d+(?:\.\d+)?(?:,-?\d+(?:\.\d+)?){1,15}', value):
            values = [_number(item) for item in value.split(',')]
            return values if all(item is not None for item in values) else None
        if name in _DEBUG_TEXT_FIELDS or name.endswith(('_reason', '_phase', '_stage', '_fault_name', '_reject_name')):
            return _debug_text(value)
        if value in ('true', 'false', 'none', 'unknown', 'unavailable'):
            return value
        if re.fullmatch(r'(?:MH|XR|DXGI|D3D12|ERROR)_[A-Z0-9_]+', value):
            return value
        return None
    if isinstance(value, dict):
        result = {}
        for child, item in list(value.items())[:1024]:
            if not isinstance(child, str) or not re.fullmatch(r'[A-Za-z][A-Za-z0-9_]{0,95}', child):
                continue
            clean = _debug_clean(item, child, depth + 1)
            if clean is not None:
                result[child] = clean
        return result
    if isinstance(value, list):
        return [clean for item in value[:128] if (clean := _debug_clean(item, key, depth + 1)) is not None]
    return None


def _debug_record(line):
    line = line.strip()
    if not line:
        return None
    if line.startswith('{'):
        try:
            raw = json.loads(line)
        except (ValueError, RecursionError):
            return None
        return _debug_clean(raw) if isinstance(raw, dict) else None
    frame = re.fullmatch(r'\s*File "([^"]+)", line (\d+), in ([A-Za-z0-9_<>.]+)', line)
    if frame:
        basename = frame[1].replace('\\', '/').rsplit('/', 1)[-1]
        return dict(kind='traceback_frame', file=basename if re.fullmatch(r'[A-Za-z0-9_.-]{1,96}', basename) else '<FILE>',
                    line=int(frame[2]), function=frame[3])
    if line == 'Traceback (most recent call last):':
        return dict(kind='traceback')
    if re.match(r'^(?:[A-Za-z_][\w.]*?(?:Error|Exception|Rejected|Unavailable|Cancelled)|'
                r'Pose probe error|Fatal error|ERROR|Error):', line):
        return dict(kind='error', message=_debug_text(line))
    active = re.fullmatch(r'Capture active for PID (\d+);.*', line)
    if active:
        return dict(kind='native_activation', active=True, pid=int(active[1]))
    if line.startswith(('runtime_name=', 'runtime_version=', 'operator_runtime=')):
        keys = 'runtime_name|runtime_version|operator_runtime|simulator_operator'
        return _debug_clean(dict(re.findall(r'(' + keys + r')=(.*?)(?=\s+(?:' + keys + r')=|$)', line)))
    pairs = re.findall(r'\b([A-Za-z][A-Za-z0-9_]{0,95})=([^\s]+)', line)
    clean = _debug_clean(dict(pairs)) if pairs else None
    if clean:
        prefix = line.split(None, 1)[0]
        if '=' not in prefix and re.fullmatch(r'(?:continuous|anchor|crosshair|stereo|frame|gpu|world|owned|'
                r'ui|native|shutdown|session|source|render|final|view|state|split|path|left|right|ngx|'
                r'projection|wgc|menu|transport|startup|hook)[A-Za-z0-9_]*', prefix):
            clean = dict(record=prefix, **clean)
    return clean


def _error_record(row):
    return (row.get('kind') in ('error', 'traceback', 'traceback_frame') or
            any(isinstance(row.get(key),(int,float)) and row[key]<0 for key in ('hresult','error_code','device_removed_reason')) or
            any(value not in (None, False, '', 0, [], {}) for key, value in row.items()
                if key == 'error' or key.endswith('_error')) or
            any(re.search(r'(?:^|_)(?:failed|failure|error)(?:_|$)', str(row.get(key, '')))
                for key in ('event', 'phase')))


def _retain_records(artifact, limit, status):
    """Keep chronology, first/last context and errors; disclose every omitted row."""
    records = artifact['records']
    if len(records) <= limit:
        return
    head = min(512, max(1, limit // 4))
    tail = min(512, max(1, limit // 4))
    keep = set(range(head)) | set(range(len(records) - tail, len(records)))
    errors = [index for index, row in enumerate(records) if _error_record(row)]
    metadata=[index for index,row in enumerate(records) if row.get('event') in ('system','adapter','adapter_required','adapter_comparison','software_inventory')]
    # Alternate earliest/latest errors, so a repetitive error cannot bury the
    # original cause or the terminal failure. Fill remaining room with the tail.
    priority = []
    for index in range((len(errors) + 1) // 2):
        priority.append(errors[index])
        priority.append(errors[-index - 1])
    for index in priority+metadata:
        if len(keep) >= limit:
            break
        keep.add(index)
    for index in range(len(records) - 1, -1, -1):
        if len(keep) >= limit:
            break
        keep.add(index)
    artifact['records'] = [records[index] for index in sorted(keep)]
    artifact['retained_records'] = len(keep)
    artifact['omitted_records'] = artifact.get('parsed_records', len(records)) - len(keep)
    artifact['omitted_error_records'] = artifact.get('omitted_error_records', 0) + len(set(errors) - keep)
    artifact['status'] = status


def _debug_artifact(collector, path, label):
    """Fixed same-session siblings only; retain every diagnostic record in-budget."""
    result = {'source_name':path.name, 'status':'missing', 'records':[]}
    try:
        before = _checked(path, collector.root)
    except FileNotFoundError:
        return result
    except (OSError, ValueError):
        result['status'] = 'unsafe_or_unreadable'
        collector.warn(label, 'unsafe/unreadable diagnostic attachment')
        return result
    window = collector.window(path, label, head=65536, tail=MAX_DEBUG_INPUT_BYTES - 65536, optional=True)
    if window is None:
        result['status'] = 'unreadable'
        return result
    first, tail, offset = window
    size = before.st_size
    if not offset:
        regions = [(0, tail)]
    elif offset <= len(first):
        regions = [(0, first + tail[len(first) - offset:])]
    else:
        regions = [(0, first), (offset, tail)]
    result.update(source_bytes=size, read_bytes=sum(len(raw) for _, raw in regions),
                  byte_ranges=[[start, start + len(raw)] for start, raw in regions],
                  status='complete' if len(regions) == 1 and regions[0][0] == 0 and len(regions[0][1]) >= size else 'truncated',
                  filtering='diagnostic fields and errors only; pointers, poses, private fields and unrelated text removed')
    csv_header = None
    if path.suffix == '.csv':
        try:
            csv_header = next(csv.reader([first.split(b'\n', 1)[0].decode('utf-8-sig')]))
            if (not csv_header or len(csv_header) != len(set(csv_header)) or
                    any(not re.fullmatch(r'[A-Za-z][A-Za-z0-9_]{0,95}', name) or
                        re.search(r'password|secret|token|account|username|email|steamid', name, re.I)
                        for name in csv_header)):
                raise ValueError('unsafe CSV header')
            result['numeric_columns'] = [name for name in CSV_DIAGNOSTIC_FIELDS if name in csv_header]
            if not result['numeric_columns']:
                raise ValueError('no known numeric columns')
        except (ValueError, UnicodeError, csv.Error, StopIteration):
            result['status'] = 'invalid_format'
            return result
    try:
        after = _checked(path, collector.root)
        result['changed_during_export'] = (before.st_size, before.st_mtime_ns) != (after.st_size, after.st_mtime_ns)
    except (OSError, ValueError):
        result['changed_during_export'] = True
    records, ignored = [], 0
    for start, raw in regions:
        if start:
            newline = raw.find(b'\n')
            raw = raw[newline + 1:] if newline >= 0 else b''
        elif raw.startswith((b'\xff\xfe', b'\xfe\xff')):
            raw = raw.decode('utf-16', errors='replace').encode('utf-8')
        if len(regions) > 1 and start == 0 and not raw.endswith(b'\n'):
            raw = raw[:raw.rfind(b'\n') + 1]
        if path.suffix == '.jsonl' and not raw.endswith(b'\n'):
            result['partial_last_record'] = True
            raw = raw[:raw.rfind(b'\n') + 1]
        text = raw.decode('utf-8-sig', errors='replace')
        for line in text.splitlines():
            if len(line) > 65536 or '\x00' in line:
                ignored += 1
                continue
            if csv_header is not None:
                try:
                    values = next(csv.reader([line]))
                    if values == csv_header:
                        continue
                    if len(values) != len(csv_header):
                        raise ValueError('partial CSV row')
                    raw_row = dict(zip(csv_header, values))
                    record = {name:_number(raw_row[name]) for name in result['numeric_columns']}
                    if any(value is None for value in record.values()):
                        raise ValueError('invalid numeric field')
                except (ValueError, csv.Error, StopIteration):
                    ignored += 1
                    continue
            else:
                if path.name.endswith(('.menu.jsonl', '.world.jsonl')):
                    try:
                        event = json.loads(line)
                        record = _events([event])[0] if isinstance(event, dict) else None
                    except (ValueError, RecursionError):
                        record = None
                else:
                    record = _debug_record(line)
            if record:
                records.append(record)
            elif line.strip():
                ignored += 1
    result['parsed_records'] = len(records)
    result['filtered_or_invalid_lines'] = ignored
    result['records'] = records
    result['retained_records'] = len(records)
    _retain_records(result, MAX_DEBUG_RECORDS, 'truncated')
    if result['status'] == 'truncated':
        collector.warn(label, 'diagnostic history truncated; byte ranges/record counts retained')
    return result


def _debug_session(collector, path, label):
    data = collector.document(path, label + ' detailed controller', max_bytes=MAX_DEBUG_INPUT_BYTES)
    if data.get('schema') != 1 or data.get('kind') != 'continuous_strict_stereo':
        return {'recognized':False, 'artifacts':{}}
    fields = SESSION_FIELDS | set(('stop_requested_by xr_cleanup_actions xr_cleanup_error native_cleanup_error '
        'native_status session_exit_evidence continuous_session_evidence continuous_owner_evidence '
        'startup_menu_evidence startup_menu_handoff menu_recovery_evidence startup_timings '
        'menu_observation_worker native_diagnostics support_diagnostics requested_configuration path_choice_center_close_error '
        'path_choice_center_start_error unsafe_xr_teardown error xr_preflight_diagnostics xr_preflight_diagnostic_error').split())
    controller = _debug_clean({key:data[key] for key in fields if key in data})
    result = {'recognized':True, 'source_session':path.name, 'controller':controller,
            'not_an_atomic_process_snapshot':True,
            'artifacts':{suffix:_debug_artifact(collector, path.with_suffix(suffix), label + suffix)
                         for suffix in _DEBUG_SUFFIXES}}
    records=[row for suffix in ('.support.jsonl.1','.support.jsonl') for row in result['artifacts'][suffix]['records']]
    result['runtime_observation_coverage']=_observation_coverage(result)
    return result


def _observation_coverage(result):
    records=[row for suffix in ('.support.jsonl.1','.support.jsonl') for row in result['artifacts'][suffix]['records']]
    return {
        'xr_system':any(row.get('event')=='system' for row in records),
        'game_adapter':any(row.get('event')=='adapter' and row.get('role')=='native' for row in records),
        'xr_adapter':any(row.get('event')=='adapter' and row.get('role')=='xr' for row in records),
        'loaded_graphics_modules':any(row.get('event')=='loaded_modules' and row.get('available') is True for row in records),
        'software_versions':any(row.get('event')=='software_inventory' and row.get('available') is True for row in records),
        'swapchain_progress':any(row.get('event')=='graphics_snapshot' for row in records),
        'first_failure_file':result['artifacts']['.first-failure.json']['status'],
        'native_live_snapshot':result['artifacts']['.native-live-status.txt']['status'],
        'native_startup_timeline':result['artifacts']['.native-startup.jsonl']['status'],
        'thread_wait_evidence':result['artifacts']['.thread-waits.jsonl']['status'],
        'xr_focus_events':any(row.get('event')=='xr_focus_event' for row in records),
        'xr_presence_capability':any(row.get('event')=='xr_focus_capability' for row in records),
        'xr_preflight_checks':result['artifacts']['.xr-preflight.jsonl']['status'],
        'scope':'false means not captured in retained evidence; it does not mean the feature/device is absent'}


def _session(collector, path, label):
    data = collector.document(path, label, max_bytes=MAX_DEBUG_INPUT_BYTES)
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
    out['source_session'] = path.name
    if isinstance(data.get('stop_requested_by'), str):
        out['stop_requested_by'] = _debug_text(data['stop_requested_by'])
    out['support_collection'] = _debug_clean(data.get('support_diagnostics',{}))
    out['requested_configuration']={key:value for key,value in _pick(data.get('requested_configuration'),
        ('render_scale','stereo_units_per_metre','camera_closer_metres','first_person_preview','right_stick_deadzone_percent',
         'hud_horizontal_fov_degrees','hud_element_scale','hud_center_scale')).items() if isinstance(value,(bool,int,float))}
    out['native_lifecycle'] = _debug_clean(data.get('native_status', {}))
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


def _openxr_inventory():
    """Read registered runtime/layer manifests, not DLLs or environment dumps."""
    if os.name != 'nt':
        raise OSError('Windows OpenXR inventory unavailable')
    import winreg
    result = {'available':True, 'observed_at_export':True, 'registrations':[],
              'environment_override_present':bool(os.environ.get('XR_RUNTIME_JSON'))}
    reader = Collector(Path.cwd())
    def manifest(path, source, kind, enabled=None):
        row = dict(source=source, kind=kind, manifest_status='unreadable')
        if enabled is not None:
            row['registered_enabled'] = enabled
        try:
            file = Path(path).absolute()
            text = reader.read(file, 'OpenXR manifest', anchor=Path(file.anchor), optional=True)
            if text is None:
                return row
            data = json.loads(text)
            item = data.get('runtime' if kind == 'runtime' else 'api_layer')
            if not isinstance(item, dict):
                return row
            row.update(manifest_status='readable', manifest_sha256=hashlib.sha256(text.encode('utf-8')).hexdigest())
            library = str(item.get('library_path', '')).replace('\\', '/').rsplit('/', 1)[-1]
            if re.fullmatch(r'[A-Za-z0-9_. -]{1,100}\.dll', library, re.I):
                row['library_name'] = library
            for key in ('name', 'api_version', 'implementation_version'):
                if isinstance(item.get(key), (str, int)) and not isinstance(item[key], bool):
                    row[('runtime_name' if kind == 'runtime' else 'layer_name') if key == 'name' else key] = _text(item[key], 100)
            disable = item.get('disable_environment')
            if isinstance(disable, str) and re.fullmatch(r'[A-Z][A-Z0-9_]{0,95}', disable):
                row['disabled_by_environment'] = bool(os.environ.get(disable))
        except (OSError, ValueError, TypeError):
            pass
        return row
    override = os.environ.get('XR_RUNTIME_JSON')
    if override:
        result['registrations'].append(manifest(override, 'export_process_environment', 'runtime'))
    for hive, label in ((winreg.HKEY_LOCAL_MACHINE, 'HKLM'), (winreg.HKEY_CURRENT_USER, 'HKCU')):
        for view, architecture in ((winreg.KEY_WOW64_64KEY, '64'), (winreg.KEY_WOW64_32KEY, '32')):
            for suffix, kind in (('', 'runtime'), ('\\ApiLayers\\Implicit', 'implicit_layer'),
                                 ('\\ApiLayers\\Explicit', 'explicit_layer')):
                try:
                    with winreg.OpenKey(hive, r'SOFTWARE\Khronos\OpenXR\1' + suffix, 0, winreg.KEY_READ | view) as key:
                        if kind == 'runtime':
                            value, value_type = winreg.QueryValueEx(key, 'ActiveRuntime')
                            if value_type in (winreg.REG_SZ, winreg.REG_EXPAND_SZ) and isinstance(value, str):
                                result['registrations'].append(manifest(value, label + architecture, kind))
                        else:
                            for index in range(min(winreg.QueryInfoKey(key)[1], 32)):
                                name, value, value_type = winreg.EnumValue(key, index)
                                if value_type == winreg.REG_DWORD and isinstance(name, str):
                                    result['registrations'].append(manifest(name, label + architecture, kind, value == 0))
                except OSError:
                    continue
    # An export-time registry snapshot must never be attributed to an older run.
    result['limitation'] = 'Registered manifests and disable flags at export; not proof which layers a past session loaded.'
    return result


def _openxr_summary(data):
    if not isinstance(data, dict):
        raise ValueError('invalid OpenXR inventory')
    result = {'available':data.get('available') is True, 'observed_at_export':True,
              'environment_override_present':data.get('environment_override_present') is True,
              'limitation':'Export-time registrations are not proof of active runtime/layers in a past session.'}
    rows = data.get('registrations', [])
    result['registrations'] = []
    for row in rows[:128] if isinstance(rows, list) else []:
        if not isinstance(row, dict):
            continue
        clean = _pick(row, ('source', 'kind', 'manifest_status', 'library_name', 'layer_name', 'runtime_name',
                           'api_version', 'implementation_version'))
        for key in ('registered_enabled', 'disabled_by_environment'):
            if type(row.get(key)) is bool:
                clean[key] = row[key]
        if isinstance(row.get('manifest_sha256'), str) and re.fullmatch(r'[a-fA-F0-9]{64}', row['manifest_sha256']):
            clean['manifest_sha256'] = row['manifest_sha256'].lower()
        result['registrations'].append(clean)
    return result


def _software_inventory():
    import sys
    directory=str(Path(__file__).resolve().parents[1]/'native-capture')
    sys.path.insert(0,directory)
    try:
        from support_diagnostics import WindowsInventory
        provider=WindowsInventory()
        try:return provider.software()
        finally:provider.close()
    finally:sys.path.remove(directory)


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


def export_bundle(root, output, overwrite=False, *, system_reader=None, settings_root=None, openxr_reader=None, software_reader=None):
    """Write a bounded support ZIP atomically. Never open a game process."""
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
    detailed = {}
    sessions = collector.sessions()
    for index, path in enumerate(sessions, 1):
        label = 'session-' + str(index)
        summary, raw_csv = _session(collector, path, label)
        if summary.get('recognized'):
            debug_name = 'debug/' + label + '.json'
            detailed[debug_name] = _debug_session(collector, path, label)
            summary['support_evidence'] = debug_name
            entries[debug_name] = json.dumps(detailed[debug_name], separators=(',', ':')).encode('utf-8')
        entries['sessions/' + label + '.json'] = json.dumps(summary, indent=2).encode('utf-8')
        if raw_csv:
            entries['sessions/' + label + '.csv'] = raw_csv.encode('utf-8')
    configuration = {}
    configuration['scope']='export_time_current_configuration_not_historical_session_arguments'
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
    try:
        openxr = _openxr_summary((openxr_reader or _openxr_inventory)())
    except Exception as error:
        collector.warn('OpenXR', 'registry inventory unavailable (' + type(error).__name__ + ')')
        openxr = {'available':False, 'observed_at_export':True}
    try:
        software=_support_record(dict((software_reader or _software_inventory)(),support_diagnostics=True,schema=1,role='environment',event='software_inventory'))
        software['observed_at_export']=True
    except Exception as error:
        collector.warn('VR software','inventory unavailable ('+type(error).__name__+')')
        software={'available':False,'observed_at_export':True}
    settings_root = Path(settings_root) if settings_root is not None else Path(os.environ.get('LOCALAPPDATA', str(Path.home() / 'AppData/Local'))) / 'Coalition/WarGame/Config'
    settings = _settings(collector, settings_root)
    startup = _startup_attempts(collector)
    entries['startup-attempts.json'] = json.dumps(startup, indent=2).encode('utf-8')
    entries['startup-summary.txt'] = _startup_text(startup).encode('utf-8')
    entries['build-info.json'] = json.dumps(_startup_context(startup_diagnostics.context(collector.root, {}, include_hashes=False)), indent=2).encode('utf-8')
    entries['configuration.json'] = json.dumps(configuration, indent=2).encode('utf-8')
    entries['launcher-state.json'] = json.dumps(_launcher_state(collector), indent=2).encode('utf-8')
    entries['system.json'] = json.dumps(system, indent=2).encode('utf-8')
    entries['openxr-environment.json'] = json.dumps(openxr, indent=2).encode('utf-8')
    entries['software-environment.json'] = json.dumps(software,indent=2).encode('utf-8')
    entries['game-settings.json'] = json.dumps(settings, indent=2).encode('utf-8')
    entries['README.txt'] = ("Gears VR support diagnostics\nSummaries and extended sanitized evidence from up to three recent sessions. Failed/incomplete/live sessions are retained.\n"
        "Start with startup-summary.txt for launch failures; startup-attempts.json retains the latest three startup timelines, component hashes and an independent earliest-failure receipt when available.\n"
        "debug/session-N.json retains native activation errors/traceback locations, native lifecycle/capture/binding counters, XR output/errors, and numeric CSV/menu/world/telemetry histories.\n"
        "Each artifact states source size, retained byte ranges, missing/truncated/changed status and filtering. Read manifest.json for coverage before diagnosing missing data as zero.\n"
        "Private paths/accounts/credentials, pointers, poses, native instructions, binaries, saves, profiles, screenshots and analysis databases are excluded.\n"
        "openxr-environment.json describes export-time registered runtime/layers; it does not prove what an older session loaded.\n"
        "software-environment.json is an export-time fallback. Per-session support journals record runtime-provided system identity/tracking, actual game/XR adapters and their per-chain comparison, loaded graphics module versions, and related VR software versions. Module presence does not prove an active hook; installed software is distinct from running components. Unreported headset model, transport and vendor versions remain unavailable.\n"
        "runtime_observation_coverage reports the retained runtime evidence. Support journals retain recent events and repeat immutable device context with its original timestamps across rotations. Native event codes: 1=factory result, 2=binding result, 3=resize begin, 4=resize result, 5=failed Present, 6=stop request, 9=device removed reason. For code 6, accepted holds the stop-reason code (0=none, 1=event, 2=F8, 3=focus lost, 4=proof failure, 5=owner exited, 6=owner error); source_line identifies the native stop site.\n"
        "The .native-live-status.txt artifact saves identity-verified native state before teardown. The .first-failure.json artifact preserves the controller's first observed failure in receipt order; it is normally absent in a healthy session. Native first failed API returns retain event_qpc and qpc_frequency. Use the clocks and surrounding timeline to establish order; the first error is not automatically the root cause. Cleanup errors do not overwrite the first receipt.\n"
        "Missing rates are null/unavailable. XR prediction/publication cadence is not headset display FPS.\n"
        "Session files may change while a live session is being exported; this is not an atomic game snapshot.\n").encode()
    entries['ISSUE-REPORT.txt'] = b'Gears VR feedback\nWhat happened?\nExpected behavior:\nSteps to reproduce / chapter and checkpoint:\nHeadset and connection method:\nApproximate time / selected session:\n'
    # Preserve core activation/status/errors first. Long histories retain their
    # beginning/end and explicit gap counts if the total support budget is hit.
    for name, debug in reversed(list(detailed.items())):
        if sum(map(len, entries.values())) <= MAX_ARCHIVE_BYTES - 256 * 1024:
            break
        for suffix in ('.telemetry.2.jsonl', '.telemetry.1.jsonl', '.telemetry.jsonl', '.csv',
                       '.menu.jsonl', '.world.jsonl', '.support.jsonl.1', '.support.jsonl', '.stdout.txt', '.stderr.txt'):
            artifact = debug['artifacts'][suffix]
            records = artifact['records']
            if len(records) <= 64:
                continue
            _retain_records(artifact, min(192, max(64, len(records) // 4)), 'truncated_for_archive_budget')
            entries[name] = json.dumps(debug, separators=(',', ':')).encode('utf-8')
            collector.warn(name + suffix, 'history reduced for total budget; activation/status/errors retained')
            if sum(map(len, entries.values())) <= MAX_ARCHIVE_BYTES - 256 * 1024:
                break
    # Reserve room for the manifest; summary CSV is duplicated in debug evidence.
    for name in list(entries):
        if sum(map(len, entries.values())) <= MAX_ARCHIVE_BYTES - 256 * 1024:
            break
        if name.endswith('.csv'):
            entries.pop(name)
            collector.warn('archive', 'numeric CSV omitted for size budget')
    for name,debug in detailed.items():
        debug['runtime_observation_coverage']=_observation_coverage(debug)
        entries[name]=json.dumps(debug,separators=(',', ':')).encode('utf-8')
    observation_coverage={name:debug['runtime_observation_coverage'] for name,debug in detailed.items()}
    coverage = {name:{suffix:{key:artifact[key] for key in
                  ('status', 'source_bytes', 'read_bytes', 'byte_ranges', 'parsed_records', 'retained_records',
                   'omitted_records', 'omitted_error_records', 'filtered_or_invalid_lines', 'changed_during_export', 'partial_last_record')
                  if key in artifact} for suffix, artifact in debug['artifacts'].items()}
                for name, debug in detailed.items()}
    manifest = {'schema':2, 'kind':'gears_vr_diagnostics', 'export_revision':EXPORT_REVISION,
                'created_utc':dt.datetime.now(dt.timezone.utc).isoformat(), 'sessions':len(sessions),
                'not_an_atomic_process_snapshot':True, 'artifact_coverage':coverage,
                'runtime_observation_coverage':observation_coverage,
                'limits':{'archive_uncompressed_bytes':MAX_ARCHIVE_BYTES,
                          'diagnostic_input_bytes_per_artifact':MAX_DEBUG_INPUT_BYTES,
                          'diagnostic_records_per_artifact':MAX_DEBUG_RECORDS},
                'warnings':collector.warnings, 'files':sorted([*entries, 'manifest.json'])}
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
