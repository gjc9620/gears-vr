"""Opt-in unattended-test audio guard. No system/master volume controls."""
import ctypes
from ctypes import wintypes
import json
import hashlib
import math
import os
from pathlib import Path
import subprocess
import time
import uuid

ROOT = Path(__file__).resolve().parents[2]
HELPER = ROOT / 'analysis/runtime/test-audio-guard-build/GearsTestAudioGuard.exe'
MANUAL_HELPER = ROOT / 'tools/game-audio/GearsTestAudioGuard.exe'
EVIDENCE_HELPER = ROOT / 'tools/game-audio/build-mute-evidence1110b/Release/GearsTestAudioGuard.exe'


def enabled():
    return os.environ.get('GEARSVR_TEST_MUTE_GAME') == '1'


def identity(value):
    if not isinstance(value, dict) or any(type(value.get(k)) is not int or value[k] <= 0
                                          for k in ('pid', 'creation_time')):
        raise ValueError('Exact PID and creation_time are required for test audio')
    return dict(pid=value['pid'], creation_time=value['creation_time'])


class Events:
    def __init__(self):
        self.k = ctypes.WinDLL('kernel32', use_last_error=True)
        self.k.CreateEventW.argtypes = [ctypes.c_void_p, wintypes.BOOL, wintypes.BOOL, wintypes.LPCWSTR]
        self.k.CreateEventW.restype = wintypes.HANDLE
        self.k.SetEvent.argtypes = [wintypes.HANDLE]; self.k.SetEvent.restype = wintypes.BOOL
        self.k.CloseHandle.argtypes = [wintypes.HANDLE]; self.k.CloseHandle.restype = wintypes.BOOL

    def create(self, name):
        ctypes.set_last_error(0)
        handle = self.k.CreateEventW(None, True, False, name)
        error = ctypes.get_last_error()
        if not handle: raise ctypes.WinError(error)
        if error == 183:
            self.k.CloseHandle(handle)
            raise RuntimeError('Test audio stop event already exists')
        return handle

    def signal(self, handle):
        if not self.k.SetEvent(handle): raise ctypes.WinError(ctypes.get_last_error())

    def close(self, handle):
        self.k.CloseHandle(handle)


def records(path):
    try:
        if path.stat().st_size > 128 * 1024: return []
        lines=path.read_text(encoding='utf-8', errors='replace').splitlines()
    except OSError:
        return []  # Publisher sharing lock: caller keeps bounded waiting.
    result = []
    for line in lines:
        try: value = json.loads(line)
        except ValueError: continue
        if isinstance(value, dict): result.append(value)
    return result


class Guard:
    def __init__(self, game, owner, log, *, helper=HELPER, events=None,
                 popen=subprocess.Popen, clock=time.monotonic, sleep=time.sleep, manual=False, ready_timeout=15.):
        if manual and enabled(): raise ValueError('Unattended mute takes precedence over manual audio')
        self.manual = manual
        self.game, self.owner = identity(game), identity(owner)
        if self.game['pid'] == self.owner['pid']: raise ValueError('Game and owner must differ')
        helper = Path(helper)
        if not helper.is_file(): raise FileNotFoundError('Build/package the Core Audio helper first: ' + str(helper))
        self.log = Path(log); self.events = events or Events(); self.process = None; self.handle = None
        self.receipt = None
        if isinstance(ready_timeout,bool) or not isinstance(ready_timeout,(int,float)) or not math.isfinite(ready_timeout) or not 0<ready_timeout<=30:
            raise ValueError('Audio ready timeout must be finite and bounded to 0..30 seconds')
        self.startup={'ready_observed':False,'ready_timeout_seconds':ready_timeout}
        nonce = uuid.uuid4().hex
        self.handle = self.events.create('Local\\GearsVR.TestAudio.Stop.' + nonce)
        command = [str(helper), '--manual-volume' if manual else '--guard', '--pid', str(self.game['pid']), '--creation-time', str(self.game['creation_time']),
                   '--owner-pid', str(self.owner['pid']), '--owner-creation-time', str(self.owner['creation_time']),
                   '--max-seconds', '21600', '--nonce', nonce]
        try:
            self.log.parent.mkdir(parents=True, exist_ok=True)
            with self.log.open('xb') as output:
                self.process = popen(command, stdin=subprocess.DEVNULL, stdout=output, stderr=subprocess.STDOUT,
                    creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0), close_fds=True)
            started=clock();deadline = started + ready_timeout
            while True:
                ready = [r for r in records(self.log) if r.get('event') == 'guard_ready']
                code=self.process.poll()
                if ready:
                    if len(ready)!=1 or any(r.get(k) != self.game[k] for k in ('pid', 'creation_time') for r in ready):
                        raise RuntimeError('Test audio helper identity receipt mismatch')
                    if any(ready[0].get('owner_'+k,self.owner[k])!=self.owner[k] for k in ('pid','creation_time')):
                        raise RuntimeError('Test audio helper owner identity receipt mismatch')
                    if code is not None:raise RuntimeError('Test audio helper exited before readiness was accepted; no mute claimed')
                    self.startup.update(ready_observed=True,waited_seconds=clock()-started)
                    return
                now=clock();self.startup.update(waited_seconds=now-started,helper_exit_code=code)
                if code is not None or now>=deadline:break
                sleep(min(.025,deadline-now))
            raise RuntimeError('Test audio helper did not acknowledge exact identity within the bounded ready wait (no mute claimed)')
        except BaseException:
            self.close()
            raise

    def close(self):
        if self.receipt is not None: return self.receipt
        result = dict(enabled=True, game_identity=self.game, owner_identity=self.owner,
                      log=str(self.log), restored=False,startup=self.startup)
        if self.manual:
            result.pop('restored')
            result.update(manual=True,configured=False)
        try:
            if self.handle: self.events.signal(self.handle)
            if self.process is not None:
                try: code = self.process.wait(timeout=4)
                except subprocess.TimeoutExpired:
                    result.update(helper_running=True, error='Audio guard stop deadline expired; guardian was not killed')
                else:
                    reports = [r for r in records(self.log) if r.get('event') == ('manual_audio_cleanup' if self.manual else 'guard_cleanup')]
                    result.update(exit_code=code)
                    if len(reports) == 1:
                        result.update(reports[0])
                        if self.manual:
                            result.pop('restored',None)
                            result['manual'] = True
                            result['configured'] = code == 0 and reports[0].get('sessions_configured',0)>0
                        else:
                            result['restored'] = code == 0 and reports[0].get('restore_pending') == 0
                            result['sessions_restored'] = reports[0].get('restored')
                            verification=[r for r in records(self.log) if r.get('event')=='mute_verification_summary' and r.get('pid')==self.game['pid']]
                            result['mute_verification']=verification[0] if len(verification)==1 else {'observed':False,'reason':'No unique explicit mute-state summary; zero mutation/errors are not proof of mute'}
                    else: result['error'] = 'No unique audio cleanup receipt'
            else:
                result.update(helper_started=False)
                if not self.manual: result['restored']=True
        except Exception as error:
            result['error'] = repr(error)
        finally:
            if self.handle:
                self.events.close(self.handle); self.handle = None
        self.receipt = result
        return result


def verify_manual_helper(helper=MANUAL_HELPER):
    helper = Path(helper)
    receipt = helper.with_name('audio-helper-provenance.json')
    value = json.loads(receipt.read_text(encoding='utf-8'))
    if (value.get('schema') != 1 or value.get('policy') != 'manual_game_session_v1' or
            value.get('helper') != helper.name or not helper.is_file() or
            hashlib.sha256(helper.read_bytes()).hexdigest() != value.get('helper_sha256')):
        raise ValueError('Manual game audio helper provenance/hash mismatch')
    return value


def begin_manual(game, owner, log):
    if enabled(): return None
    verify_manual_helper(MANUAL_HELPER)
    return Guard(game, owner, log, helper=MANUAL_HELPER, manual=True)


def begin(game, owner, log):
    if not enabled(): return None
    if os.environ.get('GEARSVR_TEST_AUDIO_EVIDENCE')=='1':
        receipt=EVIDENCE_HELPER.parent/'mute-evidence-provenance.json'
        proof=json.loads(receipt.read_text(encoding='utf-8'))
        if proof.get('policy')!='exclusive-session-mute-readback-v1' or hashlib.sha256(EVIDENCE_HELPER.read_bytes()).hexdigest()!=proof.get('sha256'):
            raise ValueError('Verified test audio helper identity differs')
        return Guard(game,owner,log,helper=EVIDENCE_HELPER)
    # A staged package uses the same verified helper for unattended mute.
    # Developer test builds retain their existing fallback when no package exists.
    if MANUAL_HELPER.exists() or MANUAL_HELPER.with_name('audio-helper-provenance.json').exists():
        verify_manual_helper(MANUAL_HELPER)
        return Guard(game, owner, log, helper=MANUAL_HELPER)
    return Guard(game, owner, log)
